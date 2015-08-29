#include "ppx_extractor.hpp"
#include <types/content_type_analyser.hpp>
#include <ppmdu/pmd2/pmd2_filetypes.hpp>
#include <ppmdu/fmts/at4px.hpp>
#include <ppmdu/fmts/pkdpx.hpp>
#include <ppmdu/pmd2/pmd2_palettes.hpp>
#include <utils/utility.hpp>
#include <utils/library_wide.hpp>
#include <utils/cmdline_util.hpp>
#include <ppmdu/fmts/sir0.hpp>
#include <ppmdu/fmts/at4px.hpp>
#include <ppmdu/fmts/pkdpx.hpp>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <chrono>
#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
using namespace ::utils::io;
using namespace utils::cmdl;
using namespace std;
using namespace pmd2;
using namespace pmd2::filetypes;
using namespace utils;
using namespace ::filetypes;


namespace ppx_extract
{
//=================================================================================================
// Constants
//=================================================================================================
    static const string                          OPT_WRITE_LOG_SYMBOL          = "wl";
    static const string                          OPT_FORCE_FILEXTENSION_SYMBOL = "fext";
    static const string                          OPT_QUIET_SYMBOL              = "q";
    static const array<optionparsing_t,3>        MY_OPTIONS     = 
    {{
        //Switch to enable logging the decompression process
        { 
            OPT_WRITE_LOG_SYMBOL, 
            0, 
            "Makes the program output a log in the current working directory!", 
            //{ utils::optionparsing_t::opt_t::P_NONE } 
        }, 
        //Switch to force a file extension for the outputed files!
        {
            OPT_FORCE_FILEXTENSION_SYMBOL, 
            1, 
            "Forces the file extensions of the outputed files to the one specified as parameter!",
            //{ utils::optionparsing_t::opt_t::P_STRING } 
        },
        //Switch to disable progress output to console (faster)
        {
            OPT_QUIET_SYMBOL,
            0,
            "Disable progress output to console! (faster!)",
        },
    }};

    static const string EXE_NAME             = "ppmd_unpx.exe";
    static const string PVERSION             = "0.41";

    //A little struct to make it easier to throw around any new parsed parameters !
    struct pxextract_params
    {
        vector<Poco::Path> inputpaths;
        vector<Poco::Path> outputpaths; 
        bool               isLogEnabled;
        bool               isQuiet;
        string             forcedextension;
    };

//=================================================================================================
// Utils
//=================================================================================================

    inline bool IsValidPXCompressedFile( const Poco::File & thefile )
    {
        const string fileext = Poco::Path(thefile.path()).getExtension();
        return( 
                thefile.isFile() && 
                !thefile.isHidden() && 
                ( fileext.compare( AT4PX_FILEX ) == 0 || fileext.compare( PKDPX_FILEX ) == 0 ) 
              );
    }

    inline bool IsValidSIR0WrappedPXCompressedFile( const Poco::File & thefile )
    {
        const string fileext = Poco::Path(thefile.path()).getExtension();
        return( 
                thefile.isFile() && 
                !thefile.isHidden() && 
                ( fileext.compare( SIR0_AT4PX_FILEX ) == 0 || fileext.compare( SIR0_PKDPX_FILEX ) == 0 ) 
              );
    }

//=================================================================================================
// Decompression Handlers
//=================================================================================================
    void DoDecompressPKDPX( vector<uint8_t>::const_iterator itdatabeg, vector<uint8_t>::const_iterator itdataend, 
                            const Poco::Path & outfilepath, bool blogenabled, bool isQuiet )
    {
        //MrChronometer   mychrono("DoDecompressPKDPX");
        vector<uint8_t> decompressed;
        Poco::Path      outputpath(outfilepath);

        //Decompress
        //filetypes::pkdpx_handler( itdatabeg, itdataend, decompressed ).Decompress(blogenabled);
        DecompressPKDPX( itdatabeg, itdataend, decompressed, !isQuiet, blogenabled );

        if( outfilepath.getExtension().empty() )
        {
            //Set the file extension only
            outputpath.setExtension( GetAppropriateFileExtension( decompressed.begin(), decompressed.end() ) );
        }

        //Write file out
        WriteByteVectorToFile( outputpath.toString(), decompressed );
    }

    void DoDecompressAT4PX( vector<uint8_t>::const_iterator itdatabeg, vector<uint8_t>::const_iterator itdataend, 
                            const Poco::Path & outfilepath, bool blogenabled, bool isQuiet )
    {
        using namespace pmd2::filetypes;
        //MrChronometer   mychrono("DoDecompressAT4PX");
        vector<uint8_t> decompressed;
        Poco::Path      outputpath(outfilepath);


        if( outputpath.getExtension().empty() )
        {
            //Set the file extension only
            outputpath.setExtension( IMAGE_RAW_FILEX );
        }

        //Decompress
        //pmd2::filetypes::at4px_decompress decomp( decompressed ); //#TODO: the decompressors for at4px and pkdpx should really all have the same syntax... >_<
        //decomp( itdatabeg, itdataend, blogenabled );

        DecompressAT4PX( itdatabeg, itdataend, decompressed, !isQuiet, blogenabled );

        WriteByteVectorToFile( outputpath.toString(), decompressed ); 
    }

    void DoDecompressSIR0AT4PX( vector<uint8_t>::const_iterator itdatabeg, vector<uint8_t>::const_iterator itdataend, 
                                const Poco::Path & outfilepath, bool blogenabled, bool isQuiet )
    {
        sir0_header hdr;
        hdr.ReadFromContainer( itdatabeg );
        DoDecompressAT4PX( itdatabeg + hdr.subheaderptr, itdatabeg + hdr.ptrPtrOffsetLst, outfilepath, blogenabled, isQuiet );
    }

    void DoDecompressSIR0PKDPX( vector<uint8_t>::const_iterator itdatabeg, vector<uint8_t>::const_iterator itdataend, 
                                const Poco::Path & outfilepath, bool blogenabled, bool isQuiet )
    {
        sir0_header hdr;
        hdr.ReadFromContainer( itdatabeg );
        DoDecompressPKDPX( itdatabeg + hdr.subheaderptr, itdatabeg + hdr.ptrPtrOffsetLst, outfilepath, blogenabled, isQuiet );
    }

//=================================================================================================
// Utility
//=================================================================================================
    void PrintUsage()
    {
	    cout << EXE_NAME <<" (-option (optionval)) \"inputpath\" (\"outputpath\") (+\"addinputpath\")...\n\n"
             << "-> option          : optionally, an option from the list below.\n"
             << "-> optionval       : optionally, for options that have a parameter,\n"
             << "                     the value of that parameter.\n"
		     << "-> inputpath       : file/folder to decompress. If is a folder,\n" 
             << "                     only files with a\n"
             << "                     \"." <<AT4PX_FILEX <<"\" or \n"
             << "                     \"." <<PKDPX_FILEX <<"\" will be processed!\n"
		     << "-> outputpath      : optionally, either a file or folder to output the\n"
             << "                     decompressed file(s) to.\n"
             << "-> addinputpath    : optionally, one can add more than a single input\n"
             << "                     to decompress by appending them\n"
             << "                     after the main parameters, preceded by a \"+\"!\n"
             << "                     Directory or file are handled the\n"
             << "                     same way as they would if they were specified\n"
             << "                     in \"inputpath\"!\n"
             << "\n\n"
             << "Options:\n";

        //List our options
        for( auto & anoption : MY_OPTIONS )
            cout <<"    -" <<left  <<setw(10) <<setfill(' ') <<anoption.optionsymbol <<right <<" : " <<anoption.description <<"\n";

		cout << "\nExample:\n"
             << EXE_NAME <<" ./file.at4px\n"
		     << EXE_NAME <<" ./file.pkdpx ./output/path/somefile.out\n"
             << EXE_NAME <<" -" <<OPT_WRITE_LOG_SYMBOL <<" ./file.pkdpx ./output/path/\n"
             << EXE_NAME <<" -" <<OPT_WRITE_LOG_SYMBOL <<" -" <<OPT_FORCE_FILEXTENSION_SYMBOL <<" \"sir0\"" <<" ./file.pkdpx ./output/path/\n"
             << EXE_NAME <<" -" <<OPT_WRITE_LOG_SYMBOL <<" -" <<OPT_FORCE_FILEXTENSION_SYMBOL <<" \"sir0\"" <<" ./file.pkdpx ./output/path/ +./another/inputpath/ +./and/another/file.pkdpx\n"
             << EXE_NAME <<" -" <<OPT_WRITE_LOG_SYMBOL <<" -" <<OPT_FORCE_FILEXTENSION_SYMBOL <<" \"sir0\"" <<" ./file.pkdpx ./output/path/ +./another/inputpath/ +./and/another/file.pkdpx\n"
             << EXE_NAME <<" -" <<OPT_WRITE_LOG_SYMBOL <<" -" <<OPT_FORCE_FILEXTENSION_SYMBOL <<" \"sir0\"" <<" ./file.pkdpx +./another/inputpath/ +./and/another/file.pkdpx\n"
             << EXE_NAME <<" ./file.pkdpx +./another/inputpath +./and/another/file.pkdpx\n"
             << "\n\n"
             << "-> Note that the last example will output files with unspecified\n" 
             << "   file extensions, depending on their individual content!\n"
             << "-> If you specify a file path as an \"outputpath\" when handling several\n"
             << "   files, the files will be outputed to \"outputpath\"'s parent \n"
             << "   directory, with no effect on their respective filenames\n"
             << "   or extensions.\n"
             << "-> !! Log files are cleared at each new session !!\n"
             << "----------------------------------------------------------\n"
		     << "Named in honour of Baz, the awesome Poochyena of doom, which\n"
             << "was my hero character in my PMD2 run ! :D\n\n"
		     << "No crappyrights, all wrongs reversed !\n"
             << "(In short, consider this Public Domain, or CC0!)\n"
             << "Sources and specs Included in original package!\n"
             <<endl;
    }

    //Parse the options and interpret their value accordingly
    void GetOurOptions( const vector<vector<string> > & optionlist, pxextract_params & params )//bool & benablelog, string & forcedextension, bool )
    {
        for( auto & anoption : optionlist )
        {
            if( anoption.size() == 1 )
            {
                //NOTE TO SELF : Do this only once. Do not assign the result of this boolean exp at each turn !

                if( anoption.front().compare( OPT_WRITE_LOG_SYMBOL ) == 0 )
                {
                    params.isLogEnabled = true;
                    cout <<"-" <<OPT_WRITE_LOG_SYMBOL <<" was specified. Logging enabled!(Warning slight perfomance hit!)\n";
                }
                else if( anoption.front().compare( OPT_QUIET_SYMBOL ) == 0 )
                {
                    params.isQuiet = true;
                    //Don't write anything to console!
                }
                else
                    cerr<<"Ecountered invalid option " <<anoption.front() <<" !\n";
            }
            else if( anoption.size() == 2 )
            {
                if( anoption.front().compare( OPT_FORCE_FILEXTENSION_SYMBOL ) == 0 )
                {
                    params.forcedextension = anoption[1];
                    cout <<"-" <<OPT_FORCE_FILEXTENSION_SYMBOL <<" was specified. Forcing output file extension to \"*." <<params.forcedextension <<"\"!\n";
                }
                else
                    cerr<<"Ecountered invalid option " <<anoption.front() <<" !\n";
            }
        }
    }

    //Make the output file names, paths and extensions depending on the context!
    void RewritePathsToOutput( const string & directorytooutputto, pxextract_params & params )
                       //const string & forcedextension, 
                       //const vector<Poco::Path> & inputpaths, 
                       //vector<Poco::Path> & out_outputpaths )
    {

        for( auto & aninputpath : params.inputpaths )
        {
            Poco::Path mypath;

            if( !directorytooutputto.empty() )
            {
                //If we force an output directory, go for it !
                mypath.parseDirectory(directorytooutputto);
                mypath.setFileName( aninputpath.getBaseName() );
            }
            else
                mypath = aninputpath; //If we don't force a specific directory, just make output paths to each file's respective directories

            if( !params.forcedextension.empty() )
                mypath.setExtension(params.forcedextension); //Append the forced file extension if is the case
            else
                mypath.setFileName( mypath.getBaseName() ); //Remove any extensions, so that we can add one later on

            params.outputpaths.push_back( mypath );
        }
    }

    void MakeAndAppendOutputPaths( const string &             secondarg, pxextract_params & params )
                                   //const string &             forcedfilext, 
                                   //const vector<Poco::Path> & inputpaths, 
                                   //vector<Poco::Path> &       out_outputpaths,
                                   //bool                       isQuiet )
    {
        //Get the output path, if possible
        Poco::Path outputtestpath;

        //Allocate first
        params.outputpaths.reserve(params.inputpaths.size());

        //CASES:
        // 1- Second arg is absent
        //    output all files to their respective directories
        // 2- Second arg is there and points to file
        //   2.1- If we got more than one file - output into Second arg directory
        //   2.2- If we got a single file      - output file with the path specified in Second arg 
        // 3- Second arg is there and points to directory
        //   3.1- If we get more than a single file, output to directory
        //   3.2- If we get a single file, output to directory

        //#1 - Check if the path is valid
        if( !secondarg.empty() && outputtestpath.tryParse(secondarg) )
        {        
            //First check if the path points to a file or directory
            if( outputtestpath.isFile() )
            {
                // 2 cases depending on how many items we handled
                if( params.inputpaths.size() > 1 )
                {
                    //We output more than one file, so use our path's parent directory as output!
                    outputtestpath.makeParent();
                }
                else
                {
                    params.outputpaths.push_back( outputtestpath );
                    return; //We don't need to run the loop below, so just return now !
                }
            }
            //Test if directory exists
            Poco::File outputdir(outputtestpath);

            try // I had to try/catch this, because POCO doesn't throw "useful" exceptions, only "unknow exceptions".. 
            {
                if( !outputdir.exists() )
                {
                    if( !outputdir.createDirectory() )
                        throw std::runtime_error("Error while creating output directory !");
                }
            }
            catch( exception e ){throw std::runtime_error( string("Error while creating output directory ! : ") + e.what() );}

            //Then make our outputed files childs of the directory we tested
            RewritePathsToOutput( outputdir.path(), params ); //forcedfilext, inputpaths, out_outputpaths );
        }
        else
        {
            //The output is missing or invalid, output all to their respective directories
            RewritePathsToOutput( string(), params );// forcedfilext, inputpaths, out_outputpaths ); //The empty string tells it to use the input dir as path
        }
    }

    //Returns the amount of files added to the queue
    unsigned int HandleASingleArgument( const string & directoryorfilepath, 
                                        vector<Poco::Path> & out_pathlisttoappendto )
    {
        if( !directoryorfilepath.empty() )
        {
            Poco::Path intest;

            if( intest.tryParse(directoryorfilepath) )
            {
                if( intest.isFile() )
                {
                    //If a file we want to append to our input list
                    out_pathlisttoappendto.push_back(std::move(intest));
                    return 1;
                }
                else if( intest.isDirectory() )
                {
                    //If a directory, we want to build a list of all the .pkdpx and .at4px files and use that as input !
                    Poco::DirectoryIterator diritend;

                    //Count nb valid
                    unsigned int nbvalidfiles = 0;
                    for( Poco::DirectoryIterator dirit(intest); dirit != diritend; ++dirit )
                    {
                        if( IsValidPXCompressedFile(*dirit) )
                            ++nbvalidfiles;
                    }

                    //Knowing how many there are we avoid allocating and copying the vectors several times..
                    out_pathlisttoappendto.reserve( out_pathlisttoappendto.size() + nbvalidfiles );

                    //Add the filenapaths
                    for( Poco::DirectoryIterator diritread(intest); diritread != diritend ; ++diritread )
                    {
                        if( IsValidPXCompressedFile(*diritread) )
                            out_pathlisttoappendto.push_back( diritread->path() );
                    }
                    return nbvalidfiles;
                }
            }
            else
            {
                cerr << "<!>-Error: " <<directoryorfilepath <<" is Invalid path !\n";
                return 0;
            }
        }
        return 0;
    }


    bool HandleArguments( int argc, const char * argv[], pxextract_params & params )//vector<Poco::Path> & out_inputpaths, vector<Poco::Path> & out_outputpaths, bool & benablelog )
    {
        //#0 - Handle options
        CArgsParser             argsparser( vector<optionparsing_t>( MY_OPTIONS.begin(), MY_OPTIONS.end() ), argv, argc );
        vector<vector<string> > optionsfound = argsparser.getAllFoundOptions();
        string                  firstarg     = argsparser.getNextParam(),
                                secondarg    = argsparser.getNextParam();
        vector<string>          additionalpaths;
        //string                  forcedfilext;

        //Get extra input paths preceded by "+"
        argsparser.appendAllAdditionalInputParams(additionalpaths);

        //#1 - Handle the parameters
        if( !firstarg.empty() && HandleASingleArgument( firstarg, params.inputpaths ) == 1 )
        {
            //#2 - Get the options
            GetOurOptions( optionsfound, params ); //.isLogEnabled, forcedfilext );

            //#3 - Get the additional input arguments!
            unsigned int nbvalidinputs = 1;
            for( auto & entry : additionalpaths )
                nbvalidinputs += HandleASingleArgument( entry, params.inputpaths );

            if( !params.isQuiet )
                cout <<"Added " <<nbvalidinputs <<" file(s) to processing queue!\n";

            //#4 - Make our output paths for each input files
            MakeAndAppendOutputPaths( secondarg, params ); // forcedfilext, params.inputpaths, params.outputpaths, params.isQuiet );
        }
        else
            PrintUsage();
        
        return true;
    }

    void DetermineAndRunHandler( const Poco::Path & inputpath, Poco::Path & outputpath, bool blogenabled, bool isQuiet )
    {

        vector<uint8_t> filedata;

        //#1 - Copy the whole file to a vector
        ReadFileToByteVector( inputpath.toString(), filedata );

        //#2 - Run analysis on the file content
        auto contentInfo = CContentHandler::GetInstance().AnalyseContent( analysis_parameter( filedata.begin(), 
                                                                                              filedata.end() ) );

        //#3 - Determine what handler to run!
        if( contentInfo._type == CnTy_AT4PX )
            DoDecompressAT4PX( filedata.begin(), filedata.end(), outputpath, blogenabled, isQuiet );
        else if( contentInfo._type == CnTy_PKDPX )
            DoDecompressPKDPX( filedata.begin(), filedata.end(), outputpath, blogenabled, isQuiet );
        else if( contentInfo._type == CnTy_SIR0_AT4PX )
            DoDecompressSIR0AT4PX( filedata.begin(), filedata.end(), outputpath, blogenabled, isQuiet );
        else if( contentInfo._type == CnTy_SIR0_PKDPX )
            DoDecompressSIR0PKDPX( filedata.begin(), filedata.end(), outputpath, blogenabled, isQuiet );
        else
        {
            cerr << "<!>-Error: The content of \"" <<inputpath.toString() <<"\" was not recognized as a valid PX compressed file! Skipping!\n";
            return;
        }
    }

    //Decompress all our input files !
    void DecompressAll( pxextract_params & params )//const vector<Poco::Path> & inputpaths, vector<Poco::Path> & outputpaths, bool blogenabled )
    {
        if( params.isLogEnabled )
            compression::CleanExistingCompressionLogs(); //Do a little clean up

        if( !params.isQuiet )
        {
            cout <<"Decompressing all..\n";
            for( unsigned int i = 0; i < params.inputpaths.size(); )
            {
                DetermineAndRunHandler( params.inputpaths[i], params.outputpaths[i], params.isLogEnabled, params.isQuiet );
                ++i;
                cout <<"\r" <<((i * 100) / params.inputpaths.size()) <<"%";
            }
            cout<<"\n";
        }
        else
        {
            for( unsigned int i = 0; i < params.inputpaths.size(); ++i )
                DetermineAndRunHandler( params.inputpaths[i], params.outputpaths[i], params.isLogEnabled, params.isQuiet );
        }
    }
};

//=================================================================================================
// Main Function
//=================================================================================================
int main( int argc, const char * argv[] )
{
    using namespace ppx_extract;
    vector<Poco::Path> inputpaths,
                        outputpaths;
    bool               benablelogging = false;

    pxextract_params params =
    {
        vector<Poco::Path>(), //Input paths
        vector<Poco::Path>(), //Output paths
        false,                //Enable logging
        false,                //Disable progress output to console
        "",                   //Forced file extension
    };


	cout << "====================================================\n"
            << "==  Baz the Poochyena's PMD:EoS/T/D UnPX-er - " <<PVERSION <<" ==\n" 
            << "====================================================\n"
            << "A PX file decompressor.\n"
            << endl;

    try
    {
        if( HandleArguments( argc, argv, params ) )//inputpaths, outputpaths, benablelogging ) )
        {
            MrChronometer mychrono("Total");
            DecompressAll( params );// inputpaths, outputpaths, benablelogging );
        }
        else
            return -1;
    }
    catch( std::exception & e )
    {
        cout<<"<!>-Exception: " <<e.what() <<endl;
        return -1;
    }

//#ifdef _DEBUG
//#ifdef WIN32
//	system("pause");
//#elif  _linux_
//    char a;
//    std::cin >> a;
//#endif
//#endif

    return 0;
}