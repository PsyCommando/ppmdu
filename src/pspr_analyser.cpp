#include "pspr_analyser.hpp"
#include <ppmdu/basetypes.hpp>
#include <ppmdu/pmd2/pmd2_sprites.hpp>
#include <ppmdu/utils/utility.hpp>
#include <ppmdu/pmd2/pmd2_filetypes.hpp>
#include <types/content_type_analyser.hpp>
#include <ppmdu/utils/multiple_task_handler.hpp>
#include <ppmdu/utils/library_wide.hpp>
#include <ppmdu/utils/cmdline_util.hpp>
#include <Poco/DirectoryIterator.h>
#include <Poco/RegularExpression.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cassert>
#include <future>
#include <thread>
using namespace ::utils::io;
using namespace utils::cmdl;
using namespace std;
using namespace utils;
using namespace pmd2;
using namespace pmd2::graphics;
using namespace multitask;

namespace pspr_analyser
{
//=================================================================================================
// Constants 
//=================================================================================================
    static const string                          FILEXT_FILTER_SYMBOL       = "ext";
    static const string                          NB_THREADS_OVERRIDE_SYMBOL = "t";
    static const array<optionparsing_t,2>        MY_OPTIONS =
    {{
        { FILEXT_FILTER_SYMBOL,       1 },  //File extension filtering
        { NB_THREADS_OVERRIDE_SYMBOL, 1 }  //For specifying a forced amount of threads
    }};

    static const string                          EXE_NAME                   = "ppmd_spranalyser";
    static const string                          PVERSION                   = "0.1";

//=================================================================================================
// Structs
//=================================================================================================
    struct completion_t
    {
        atomic_int     nbcompleted;
        const size_t   nbtotaltodo;
        atomic_int     currentstep;
        const uint32_t totalnbsteps;
    };

//=================================================================================================
// Sprite Anylysis
//=================================================================================================


    void WriteProgressConsole( completion_t & mycompletion, const string & textbefore )
    {
        static std::mutex PROGRESS_MUTEX;
        try
        {
            unsigned int percent = (mycompletion.nbcompleted * 100) / mycompletion.nbtotaltodo;
            std::lock_guard<mutex> lck(PROGRESS_MUTEX);
            cout<<"\r" <<"Step " <<mycompletion.currentstep <<"of" <<mycompletion.totalnbsteps <<": " <<textbefore <<" : "
                <<std::setfill(' ') <<std::setw(4) <<std::dec <<percent  <<"%";
        }
        catch( std::exception e ){SimpleHandleException(e);}
    }

    /**********************************************************************************************************
        Write a single file report to a log file!
    **********************************************************************************************************/
    bool WriteOutAReport( CCharSpriteData & asprite, string outputpath, string & report, completion_t * pcompletion )
    {
        try
        {
            ofstream outputfile( outputpath );

            if(!outputfile.good())
            {
                cerr <<"\n<!>-ERROR: File \"" <<outputpath <<"\" couldn't be opened!\n\n";
                return false;
            }

            outputfile <<report <<asprite.WriteReport();
            outputfile.close();
        }
        catch(exception e){SimpleHandleException(e);}
        
        ++(pcompletion->nbcompleted);
        WriteProgressConsole( *pcompletion, "Writing Reports" );

        //Dummy return because of a bug in the MSVC 2012 compiler..
        return true;
    }


    /**********************************************************************************************************
        Read all sprites over several worker threads!
    **********************************************************************************************************/
    vector< vector<uint8_t> > ReadSpritesToBuffers( const vector<string> & spritespaths, CMultiTaskHandler & taskmanager )
    {
        vector< vector<uint8_t> > buffers(spritespaths.size());
        completion_t              mycompletion = { 0, spritespaths.size(), 1, 3 };

        //A lambda wrapper to get over the bug in msvc2012 where a packaged_task can't have a void return value..
        auto lambdaread = [&mycompletion](const std::string & path, std::vector<uint8_t> & out_filedata )->bool
        {
            ReadFileToByteVector( path, out_filedata );
            ++(mycompletion.nbcompleted);
            WriteProgressConsole(mycompletion, "Reading files");
            return true;
        };

        for( unsigned int i = 0; i < buffers.size(); ++i )
        {
            taskmanager.AddTask( pktask_t( bind( lambdaread, std::ref( spritespaths[i] ), std::ref( buffers[i] ) ) ) );
        }
        taskmanager.Execute();
        taskmanager.BlockUntilTaskQueueEmpty();

        return std::move( buffers );
    }

    /**********************************************************************************************************
        Parse raw sprites into CCharSpriteData objects
    **********************************************************************************************************/
    vector<CCharSpriteData> ParseSprites( const vector<vector<uint8_t> > & filesRawData, 
                                          vector<string>                 & spritereports, 
                                          CMultiTaskHandler              & taskmanager, 
                                          const vector<string>           & spritespaths )
    {
        typedef tuple<types::constitbyte_t,types::constitbyte_t, vector<CCharSpriteData>::iterator> inputdata; 
        vector<CCharSpriteData> filesSpriteData( filesRawData.size() );
        completion_t            mycompletion = { 0, spritespaths.size(), 2, 3 };

        //A lambda wrapper to get over the bug in msvc2012 where a packaged_task can't have a void return value..
        auto lambdaparse = [&mycompletion](inputdata indata, std::string & report, const std::string & curfilepath)->bool
        {
            sprite_parser( *(get<2>(indata)) )( get<0>(indata), get<1>(indata), report );
            ++(mycompletion.nbcompleted);
            WriteProgressConsole( mycompletion, "Parsing Sprites");
            return true;
        };

        for( unsigned int i = 0; i < filesRawData.size(); ++i )
        {
            filetypes::analysis_parameter param( filesRawData[i].begin(), filesRawData[i].end() );
            auto                          content = filetypes::CContentHandler::GetInstance().AnalyseContent( param );

            //Validate file first, and make sure its a sprite file
            // Also, given the format analyser can be a little wonky at time, better double check !
            if( content._type == filetypes::e_ContentType::WAN_SPRITE_CONTAINER )
            {
                auto myfunction = bind( lambdaparse, 
                                        make_tuple(filesRawData[i].begin(), filesRawData[i].end(), filesSpriteData.begin()+i),  
                                        std::ref( spritereports[i] ), 
                                        std::ref( spritespaths[i]  ) 
                                       );
                taskmanager.AddTask( pktask_t(std::move(myfunction)) );
            }
        }
        taskmanager.Execute();
        taskmanager.BlockUntilTaskQueueEmpty();

        return std::move(filesSpriteData);
    }


    /**********************************************************************************************************
        Generates and output the reports for all sprite files we parsed!
    **********************************************************************************************************/
    void WriteAllReports( vector< CCharSpriteData > & spritedata, const string & outputpath, const vector<string> & spritespaths, vector< string >& spritereports, CMultiTaskHandler & taskhandler )
    {
        completion_t completion = { 0, spritedata.size(), 3, 3 };

        for( unsigned int i = 0; i < spritedata.size(); ++i )
        {
            stringstream reportfilename;
            reportfilename <<AppendTraillingSlashIfNotThere( outputpath ) 
                           <<GetPathWithoutFileExt( GetFilenameFromPath( spritespaths[i] ) )
                           <<".log";

            taskhandler.AddTask( 
                                 pktask_t( 
                                 bind( WriteOutAReport, std::ref(spritedata[i]), reportfilename.str(), std::ref(spritereports[i]), &completion )
                                         )
                                );
        }
        taskhandler.Execute();
        taskhandler.BlockUntilTaskQueueEmpty();
        cerr <<"\n";
    }

    /**********************************************************************************************************
        Read the files, and write the actual reports!
    **********************************************************************************************************/
    int HandleAllSprites( const vector<string> & spritespaths, const string & outputpath )
    {
        vector<string>          spritereports(spritespaths.size());
        vector<vector<uint8_t> > filesRawData;
        vector<CCharSpriteData> filesSpriteData;
        CMultiTaskHandler       taskhandler;

        cerr <<"Analysing " <<spritespaths.size() <<" file(s)..\n"
             <<"Outputing reports to \"" <<outputpath <<"\"\n\n";

        //#1 - Read all sprite files
        filesRawData = ReadSpritesToBuffers( spritespaths, taskhandler );

        //#2 - Parse all sprite files
        filesSpriteData = ParseSprites( filesRawData, spritereports, taskhandler, spritespaths );

        //#3 - Generate and output report for all sprite files.
        WriteAllReports( filesSpriteData, outputpath, spritespaths, spritereports, taskhandler );

        taskhandler.StopExecute();

        return 0;
    }

//=================================================================================================
// Utility
//=================================================================================================

    /**********************************************************************************************************
        Prints the readme
    **********************************************************************************************************/
    void PrintUsage()
    {
	    cout << EXE_NAME <<" (option \"optionvalue\") \"inputpath\" \"outputpath\"\n"
            << "\n" 
             << "-> option          : Option from the list below..\n"
             << "-> optionvalue     : An optional value for the specified option..\n"
		     << "-> inputpath       : sprite file/folder to read from.\n"
		     << "-> outputpath(opt) : folder to output the log file(s) to.\n\n\n"
             << "\n\n"
             << "Options:\n"
             << "      -" <<FILEXT_FILTER_SYMBOL <<" \"filextension\" : Specifying this will make the program scan\n"
             << "                              only the files ending with the specified\n"
             << "                              file extendsion! Do not put a dot in there!\n"
             << "                              If used on a single file, will not do anything!\n"
             << "      -" <<NB_THREADS_OVERRIDE_SYMBOL <<" \"nbthreds\"       : This option forces the amount specified of \n"
             << "                              worker threads to handle the analysis!\n"
             << "                              DO NOT set this too high, or you'll choke your\n"
             << "                              system to death.. Use a sane value of 1 to 8..\n"
             << "\n"
		     << "Example:\n"
             << "---------\n"
             << EXE_NAME <<" ./file.sir0\n"
		     << EXE_NAME <<" ./ ./outputfolder/\n"
             << EXE_NAME <<" -" <<FILEXT_FILTER_SYMBOL <<" \"sir0\" ./ ./outputfolder/\n"
             << EXE_NAME <<" -" <<FILEXT_FILTER_SYMBOL <<" \"wan\" ./ ./outputfolder/\n"
             << "\n"
             << "To sum it up :\n"
             << "--------------\n"
             << "Output detailed information on each sprites into log files.\n"
             << "-> If path is directory, all files within the folder will be\n"
             << "   scanned regardless of their extension.\n"
             << "\n"
             << "----------------------------------------------------------\n"
		     << "Named in honour of Baz, the awesome Poochyena of doom, which was my hero character in my PMD2 run ! :D\n\n"
		     << "No crappyrights, all wrongs reversed !\n"
             << "(In short, consider this Public Domain, or CC0!)\n"
             << "Sources and specs Included in original package!\n" <<endl;
    }


    /**********************************************************************************************************
        Handle the arguments passed to the main function
    **********************************************************************************************************/
    int HandleArguments( int argc, const char * argv[], vector<string> & inputpaths, string & outputpath )
    {
        //#0 - Handle options
        CArgsParser        argsparser( vector<optionparsing_t>( MY_OPTIONS.begin(), MY_OPTIONS.end() ), argv, argc );
        auto               optionsfound = argsparser.getAllFoundOptions();
        string             firstarg     = argsparser.getNextParam(),
                           secondarg    = argsparser.getNextParam();

        //#1 - Handle the parameters
        if( !firstarg.empty() )
        {
            Poco::File cmdlineinpath(firstarg);

            //If we got an output path specified, just put it the output path string
            if( !secondarg.empty() )
                outputpath = secondarg;
            else
            {
                //If not, make an output path from the input path
                outputpath = utils::GetPathOnly( firstarg );
            }

            //Might as well check if the files exist..
            if( !cmdlineinpath.exists() )
            {
                cerr << "!-Fatal Error: Input file or path invalid!\n";
                return -1;
            }

            //The output is always a folder
            if( !utils::isFolder(outputpath) )
            {
                cerr << "!-Fatal Error: Output path invalid!\n";
                return -1;
            }

            //Handle options
            string filetyperegex = "*";
            for( auto & anoption : optionsfound )
            {
                if( anoption.front() == FILEXT_FILTER_SYMBOL )
                {
                    stringstream regexsstr;
                    regexsstr << "*." << optionsfound.front().back();
                    filetyperegex = regexsstr.str();
                    cout <<"\"-" <<FILEXT_FILTER_SYMBOL <<"\" specified! Processing only files with \"" <<filetyperegex <<"\" as extension!\n";
                }

                if( anoption.front() == NB_THREADS_OVERRIDE_SYMBOL )
                {
                    unsigned int nbthreads = 1;
                    stringstream strs;
                    strs << anoption.back();
                    strs >> nbthreads;
                    nbthreads = std::max( nbthreads, 1u );
                    LibraryWide::getInstance().Data().setNbThreadsToUse( nbthreads );
                    cout <<"\"-" <<NB_THREADS_OVERRIDE_SYMBOL <<"\" specified! Using " <<nbthreads <<" thread(s) for processing!\n";
                }
            }

            //Handle a folder differently from a file
            if( cmdlineinpath.isDirectory() )
            {
                //if( !optionsfound.empty() && optionsfound.front().front() == FILEXT_FILTER_SYMBOL )
                Poco::DirectoryIterator itdir(cmdlineinpath),
                                        itdirend;
                Poco::RegularExpression fileregex(filetyperegex);
                unsigned int            nbfiles = 0; 

                //A lambda to avoid having to re-write the valid file condition
                auto lambdaValidFile = []( const Poco::File & f, Poco::RegularExpression & fileregex )->bool
                {
                    return ( f.isFile() && !(f.isHidden()) && fileregex.match(f.path()) );
                };
                
                //Count files first
                for( auto itcount = itdir; itcount != itdirend; ++itcount )
                {
                    if( lambdaValidFile(*itcount, fileregex) )
                        ++nbfiles;
                }

                //Resize our target string vector
                inputpaths.reserve( nbfiles );
                inputpaths.resize(0);

                //Add the path to all files within the folder
                for( ; itdir != itdirend; ++itdir )
                {
                    if( lambdaValidFile(*itdir, fileregex) )
                        inputpaths.push_back( itdir->path() );
                }
            }
            else
            {
                //Add only a single path
                inputpaths.push_back( cmdlineinpath.path() );
            }
        }
        else
        {
            PrintUsage();
            return 1;
        }
        
        return 0;
    }
};

//=================================================================================================
// Main Function
//=================================================================================================
int main( int argc, const char * argv[] )
{
    using namespace pspr_analyser;
    int            returnval  = 0;
    vector<string> inputpaths;
    string         outputpath = Poco::Path::current();

    //Until the bug is fixed, lock it to 1 thread unless specified !!!
    LibraryWide::getInstance().Data().setNbThreadsToUse( 1 ); //TODO: remove this !

	cout << "============================================================\n"
            << "==  Baz the Poochyena's PMD:EoS/T/D Sprite Analyser - " <<PVERSION <<" ==\n" 
            << "============================================================\n"
            << "Write detailed logs about pmd2 sprite files.\n"
            << endl;

    if( HandleArguments( argc, argv, inputpaths, outputpath ) == 0 )
    {
        MrChronometer mychrono("Total");
        HandleAllSprites( inputpaths, outputpath );
        cout <<"Analysis complete!\n" <<endl;
    }
    else
        returnval = -1;

#ifdef _DEBUG
    #ifdef WIN32
	    system("pause");
    #elif  _linux_
        char a;
        std::cin >> a;
    #endif
#endif

    return returnval;
}