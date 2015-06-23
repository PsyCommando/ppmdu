#include "audioutil.hpp"
#include <ppmdu/utils/utility.hpp>
#include <ppmdu/utils/cmdline_util.hpp>
#include <ppmdu/pmd2/pmd2_audio_data.hpp>


#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>

#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/Exception.h>
using namespace ::utils::cmdl;
using namespace ::utils::io;
using namespace ::std;

namespace audioutil
{
//=================================================================================================
//  CAudioUtil
//=================================================================================================

//------------------------------------------------
//  Constants
//------------------------------------------------
    const string CAudioUtil::Exe_Name            = "ppmd_audioutil.exe";
    const string CAudioUtil::Title               = "Music and sound import/export tool.";
    const string CAudioUtil::Version             = "0.1";
    const string CAudioUtil::Short_Description   = "A utility to export and import music and sounds from the PMD2 games.";
    const string CAudioUtil::Long_Description    = 
        "#TODO";
    const string CAudioUtil::Misc_Text           = 
        "Named in honour of Baz, the awesome Poochyena of doom ! :D\n"
        "My tools in binary form are basically Creative Commons 0.\n"
        "Free to re-use in any ways you may want to!\n"
        "No crappyrights, all wrongs reversed! :3";

//
//
//
    void WriteMusicDump( const pmd2::audio::MusicSequence & seq, const std::string & fname );

//------------------------------------------------
//  Arguments Info
//------------------------------------------------
    /*
        Data for the automatic argument parser to work with.
    */
    const vector<argumentparsing_t> CAudioUtil::Arguments_List =
    {{
        //Input Path argument
        { 
            0,      //first arg
            false,  //false == mandatory
            true,   //guaranteed to appear in order
            "input path", 
            "Path to the file/directory to export, or the directory to assemble.",
#ifdef WIN32
            "\"c:/pmd_romdata/data.bin\"",
#elif __linux__
            "\"/pmd_romdata/data.bin\"",
#endif
            std::bind( &CAudioUtil::ParseInputPath, &GetInstance(), placeholders::_1 ),
        },
        //Output Path argument
        { 
            1,      //second arg
            true,   //true == optional
            true,   //guaranteed to appear in order
            "output path", 
            "Output path. The result of the operation will be placed, and named according to this path!",
#ifdef WIN32
            "\"c:/pmd_romdata/data\"",
#elif __linux__
            "\"/pmd_romdata/data\"",
#endif
            std::bind( &CAudioUtil::ParseOutputPath, &GetInstance(), placeholders::_1 ),
        },
    }};




//------------------------------------------------
//  Options Info
//------------------------------------------------

    /*
        Information on all the switches / options to allow the automated parser 
        to parse them.
    */
    const vector<optionparsing_t> CAudioUtil::Options_List=
    {{
        //Force Import
        //{
        //    "i",
        //    0,
        //    "Specifying this will force import!",
        //    "-i",
        //    std::bind( &CAudioUtil::ParseOptionForceImport, &GetInstance(), placeholders::_1 ),
        //},
    }};


//------------------------------------------------
// Misc Methods
//------------------------------------------------

    CAudioUtil & CAudioUtil::GetInstance()
    {
        static CAudioUtil s_util;
        return s_util;
    }

    CAudioUtil::CAudioUtil()
        :CommandLineUtility()
    {
        m_operationMode = eOpMode::Invalid;
    }

    const vector<argumentparsing_t> & CAudioUtil::getArgumentsList   ()const { return Arguments_List;    }
    const vector<optionparsing_t>   & CAudioUtil::getOptionsList     ()const { return Options_List;      }
    const argumentparsing_t         * CAudioUtil::getExtraArg        ()const { return nullptr;           } //No extra args
    const string                    & CAudioUtil::getTitle           ()const { return Title;             }
    const string                    & CAudioUtil::getExeName         ()const { return Exe_Name;          }
    const string                    & CAudioUtil::getVersionString   ()const { return Version;           }
    const string                    & CAudioUtil::getShortDescription()const { return Short_Description; }
    const string                    & CAudioUtil::getLongDescription ()const { return Long_Description;  }
    const string                    & CAudioUtil::getMiscSectionText ()const { return Misc_Text;         }

//--------------------------------------------
//  Parse Args
//--------------------------------------------
    bool CAudioUtil::ParseInputPath( const string & path )
    {
        Poco::File inputfile(path);

        //check if path exists
        if( inputfile.exists() && ( inputfile.isFile() || inputfile.isDirectory() ) )
        {
            m_inputPath = path;
            return true;
        }
        return false;
    }
    
    bool CAudioUtil::ParseOutputPath( const string & path )
    {
        Poco::Path outpath(path);

        if( outpath.isFile() || outpath.isDirectory() )
        {
            m_outputPath = path;
            return true;
        }
        return false;
    }


//
//  Parse Options
//

    //bool CAudioUtil::ParseOptionPk( const std::vector<std::string> & optdata )
    //{
    //}


//
//  Program Setup and Execution
//
    int CAudioUtil::GatherArgs( int argc, const char * argv[] )
    {
        int returnval = 0;
        //Parse arguments and options
        try
        {
            if( !SetArguments(argc,argv) )
                return -3;

            DetermineOperation();
        }
        catch( Poco::Exception pex )
        {
            cerr <<"\n<!>-POCO Exception - " <<pex.name() <<"(" <<pex.code() <<") : " << pex.message() <<"\n" <<endl;
            cout <<"=======================================================================\n"
                 <<"Readme\n"
                 <<"=======================================================================\n";
            PrintReadme();
            return pex.code();
        }
        catch( exception e )
        {
            cerr <<"\n<!>-Exception: " << e.what() <<"\n" <<endl;
            cout <<"=======================================================================\n"
                 <<"Readme\n"
                 <<"=======================================================================\n";
            PrintReadme();
            return -3;
        }
        return returnval;
    }

    void CAudioUtil::DetermineOperation()
    {
        Poco::Path inpath( m_inputPath );
        Poco::File infile( inpath );

        if( m_operationMode != eOpMode::Invalid )
            return; //Skip if we have a forced mode         

        if( !m_outputPath.empty() && !Poco::File( Poco::Path( m_outputPath ).makeAbsolute().parent() ).exists() )
            throw runtime_error("Specified output path does not exists!");

        if( infile.exists() )
        {
            if( infile.isFile() )
            {
                const string fext = inpath.getExtension();

                if( fext == pmd2::audio::SMDL_FileExtension )
                    m_operationMode = eOpMode::ExportSMDL;
                else if( fext == pmd2::audio::SEDL_FileExtension )
                    m_operationMode = eOpMode::ExportSEDL;
                else if( fext == pmd2::audio::SWDL_FileExtension )
                    m_operationMode = eOpMode::ExportSWDL;
                else
                    throw runtime_error("Can't import this file format!");
            }
            else if( infile.isDirectory() )
            {

                //if( m_hndlStrings )
                //{
                //    if( m_force == eOpForce::Export )
                //        m_operationMode = eOpMode::ExportGameStrings;
                //    else
                //        throw runtime_error("Can't import game strings from a directory : " + m_inputPath);
                //}
                //else if( m_hndlItems )
                //{
                //    if( m_force == eOpForce::Export )
                //        m_operationMode = eOpMode::ExportItemsData;
                //    else
                //        m_operationMode = eOpMode::ImportItemsData;
                //}
                //else if( m_hndlMoves )
                //{
                //    if( m_force == eOpForce::Export )
                //        m_operationMode = eOpMode::ExportMovesData;
                //    else
                //        m_operationMode = eOpMode::ImportMovesData;

                //}
                //else if( m_hndlPkmn )
                //{
                //    if( m_force == eOpForce::Export )
                //        m_operationMode = eOpMode::ExportPokemonData;
                //    else
                //        m_operationMode = eOpMode::ImportPokemonData;
                //}
                //else
                //{
                //    if( m_force == eOpForce::Import || isImportAllDir(m_inputPath) )
                //        m_operationMode = eOpMode::ImportAll;
                //    else
                //        m_operationMode = eOpMode::ExportAll; //If all else fails, try an export all!
                //}
            }
            else
                throw runtime_error("Cannot determine the desired operation!");
        }
        else
            throw runtime_error("The input path does not exists!");

    }

    int CAudioUtil::Execute()
    {
        int returnval = -1;
        utils::MrChronometer chronoexecuter("Total time elapsed");
        try
        {
            switch(m_operationMode)
            {
                case eOpMode::ExportSWDLBank:
                {
                    cout << "=== Exporting SWD Bank ===\n";
                    returnval = ExportSWDLBank();
                    break;
                }
                case eOpMode::ExportSWDL:
                {
                    cout << "=== Exporting SWD ===\n";
                    returnval = ExportSWDL();
                    break;
                }
                case eOpMode::ExportSMDL:
                {
                    cout << "=== Exporting SMD ===\n";
                    returnval = ExportSMDL();
                    break;
                }
                case eOpMode::ExportSEDL:
                {
                    cout << "=== Exporting SED ===\n";
                    returnval = ExportSEDL();
                    break;
                }
                case eOpMode::BuildSWDL:
                {
                    cout << "=== Building SWD ===\n";
                    returnval = BuildSWDL();
                    break;
                }
                case eOpMode::BuildSMDL:
                {
                    cout << "=== Building SMD ===\n";
                    returnval = BuildSMDL();
                    break;
                }
                case eOpMode::BuildSEDL:
                {
                    cout << "=== Building SED ===\n";
                    returnval = BuildSEDL();
                    break;
                }
                default:
                {
                    throw runtime_error( "Invalid operation mode. Something is wrong with the arguments!" );
                }
            };
        }
        catch(Poco::Exception & e )
        {
            cerr <<"\n" << "<!>- POCO Exception - " <<e.name() <<"(" <<e.code() <<") : " << e.message() <<"\n" <<endl;
        }
        catch( exception &e )
        {
            cerr <<"\n" << "<!>- Exception - " <<e.what() <<"\n" <<"\n";
        }
        return returnval;
    }


//--------------------------------------------
//  Operation
//--------------------------------------------
    int CAudioUtil::ExportSWDLBank()
    {
        return 0;
    }
    
    int CAudioUtil::ExportSWDL()
    {
        return 0;
    }

    int CAudioUtil::ExportSMDL()
    {
        using namespace pmd2::audio;
        Poco::Path inputfile(m_inputPath);
        Poco::Path outputfile;
        string     outfname;

        if( ! m_outputPath.empty() )
            outputfile = Poco::Path(m_outputPath);
        else
            outputfile = inputfile.parent().append( inputfile.getBaseName() ).makeFile();

        outfname = outputfile.getBaseName();

        //Export
        MusicSequence smd = LoadSequence( inputfile.toString() );

        //Write output
        ofstream outstr( outputfile.toString() );
        outstr<<smd.tostr();
        outstr.flush();
        outstr.close();
        /*WriteMusicDump( smd, outputfile.toString() );*/

        //Write meta

        //Finish

        return 0;
    }

    //void WriteMusicDump( const pmd2::audio::MusicSequence & seq, const std::string & fname )
    //{

    //}

    int CAudioUtil::ExportSEDL()
    {
        return 0;
    }

    int CAudioUtil::BuildSWDL()
    {
        return 0;
    }

    int CAudioUtil::BuildSMDL()
    {
        return 0;
    }

    int CAudioUtil::BuildSEDL()
    {
        return 0;
    }

//--------------------------------------------
//  Main Methods
//--------------------------------------------
    int CAudioUtil::Main(int argc, const char * argv[])
    {
        int returnval = -1;
        PrintTitle();

        //Handle arguments
        returnval = GatherArgs( argc, argv );
        if( returnval != 0 )
            return returnval;
        
        //Execute the utility
        returnval = Execute();

#ifdef _DEBUG
        system("pause");
#endif

        return returnval;
    }
};

//=================================================================================================
// Main Function
//=================================================================================================

//#TODO: Move the main function somewhere else !
int main( int argc, const char * argv[] )
{
    using namespace audioutil;
    try
    {
        CAudioUtil & application = CAudioUtil::GetInstance();
        return application.Main(argc,argv);
    }
    catch( exception & e )
    {
        cout<< "<!>-ERROR:" <<e.what()<<"\n"
            << "If you get this particular error output, it means an exception got through, and the programmer should be notified!\n";
    }

    return 0;
}