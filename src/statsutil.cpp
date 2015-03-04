#include "statsutil.hpp"
#include <ppmdu/utils/utility.hpp>
#include <ppmdu/utils/cmdline_util.hpp>
#include <ppmdu/pmd2/game_stats.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/Exception.h>
using namespace ::std;
using namespace ::utils::cmdl;
using namespace ::utils::io;
using namespace ::pmd2::stats;

namespace statsutil
{
    static const string TXTPAL_Filext = "txt";

//=================================================================================================
//  CStatsUtil
//=================================================================================================

//------------------------------------------------
//  Constants
//------------------------------------------------
    const string CStatsUtil::Exe_Name            = "ppmd_statsutil.exe";
    const string CStatsUtil::Title               = "Game data importer/exporter";
    const string CStatsUtil::Version             = "0.1";
    const string CStatsUtil::Short_Description   = "A utility to export and import various game statistics/data, such as pokemon stats.";
    const string CStatsUtil::Long_Description    = 
        "#TODO";
    const string CStatsUtil::Misc_Text           = 
        "Named in honour of Baz, the awesome Poochyena of doom ! :D\n"
        "My tools in binary form are basically public domain / CC0.\n"
        "Free to re-use in any ways you may want to!\n"
        "No crappyrights, all wrongs reversed! :3";

//------------------------------------------------
//  Arguments Info
//------------------------------------------------
    /*
        Data for the automatic argument parser to work with.
    */
    const vector<argumentparsing_t> CStatsUtil::Arguments_List =
    {{
        //Input Path argument
        { 
            0,      //first arg
            false,  //mandatory
            true,   //guaranteed to appear in order
            "input path", 
            "Path to the file to export, or the directory to assemble.",
#ifdef WIN32
            "\"c:/pmd_romdata/data.bin\"",
#elif __linux__
            "\"/pmd_romdata/data.bin\"",
#endif
            std::bind( &CStatsUtil::ParseInputPath, &GetInstance(), placeholders::_1 ),
        },
        //Output Path argument
        { 
            1,      //second arg
            true,   //optional
            true,   //guaranteed to appear in order
            "output path", 
            "Output path. The result of the operation will be placed, and named according to this path!",
#ifdef WIN32
            "\"c:/pmd_romdata/data\"",
#elif __linux__
            "\"/pmd_romdata/data\"",
#endif
            std::bind( &CStatsUtil::ParseOutputPath, &GetInstance(), placeholders::_1 ),
        },
    }};

//------------------------------------------------
//  Options Info
//------------------------------------------------

    /*
        Information on all the switches / options to allow the automated parser 
        to parse them.
    */
    const vector<optionparsing_t> CStatsUtil::Options_List=
    {{
        //Export data to text files!
        {
            "totxt",
            0,
            "Specifying this will export the data to text files instead of to XML!",
            "-totxt",
            std::bind( &CStatsUtil::ParseOptionOutTxt, &GetInstance(), placeholders::_1 ),
        },
        //Export data to a single file
        {
            "sf",
            0,
            "This will force the utility to export all data to a single file, in cases when it exports several by default!",
            "-sf",
            std::bind( &CStatsUtil::ParseOptionExpSingleFile, &GetInstance(), placeholders::_1 ),
        },
    }};

//------------------------------------------------
// Misc Methods
//------------------------------------------------

    CStatsUtil & CStatsUtil::GetInstance()
    {
        static CStatsUtil s_util;
        return s_util;
    }

    CStatsUtil::CStatsUtil()
        :CommandLineUtility()
    {
        m_outputFormat    = eOutFormat::XML;
        m_operationMode   = eOpMode::Invalid;
    }

    const vector<argumentparsing_t> & CStatsUtil::getArgumentsList   ()const { return Arguments_List;          }
    const vector<optionparsing_t>   & CStatsUtil::getOptionsList     ()const { return Options_List;            }
    const argumentparsing_t         * CStatsUtil::getExtraArg        ()const { return &Arguments_List.front(); }
    const string                    & CStatsUtil::getTitle           ()const { return Title;                   }
    const string                    & CStatsUtil::getExeName         ()const { return Exe_Name;                }
    const string                    & CStatsUtil::getVersionString   ()const { return Version;                 }
    const string                    & CStatsUtil::getShortDescription()const { return Short_Description;       }
    const string                    & CStatsUtil::getLongDescription ()const { return Long_Description;        }
    const string                    & CStatsUtil::getMiscSectionText ()const { return Misc_Text;               }

//--------------------------------------------
//  Parse Args
//--------------------------------------------
    bool CStatsUtil::ParseInputPath( const string & path )
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
    
    bool CStatsUtil::ParseOutputPath( const string & path )
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
    bool CStatsUtil::ParseOptionOutTxt( const std::vector<std::string> & optdata )
    {
        m_outputFormat = eOutFormat::TXT;
        return true;
    }

    bool CStatsUtil::ParseOptionExpSingleFile( const std::vector<std::string> & optdata )
    {
        return m_bExpSingleFile = true;
    }

//
//
//
    int CStatsUtil::GatherArgs( int argc, const char * argv[] )
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
            cerr <<"\n<!>-POCO Exception - " <<pex.name() <<"(" <<pex.code() <<") : " << pex.message() <<endl;
            PrintReadme();
            return pex.code();
        }
        catch( exception e )
        {
            cerr <<"\n<!>-Exception: " << e.what() <<endl;
            PrintReadme();
            return -3;
        }
        return returnval;
    }

    void CStatsUtil::DetermineOperation()
    {
        Poco::Path inpath( m_inputPath );

        if( m_operationMode != eOpMode::Invalid )
            return; //Skip if we have a forced mode

        if( inpath.isFile() )
        {
            //Analyse input content
            eGameDataTy dataty = GetGameDataTyForFile(m_inputPath);

            if( dataty == eGameDataTy::StatsGrowthData )
                m_operationMode = eOpMode::ExportPokeStatsGrowth;
        }
        else if( inpath.isDirectory() )
        {
            //Probe the directory to find out the content
            eGameDataTy dataty = GetGameDataTyForDirectory(m_inputPath);

            if( dataty == eGameDataTy::StatsGrowthData )
                m_operationMode = eOpMode::ImportPokeStatsGrowth;
        }
        else
            throw runtime_error("ERROR: The input path is neither a file, or a directory!");
    }

    int CStatsUtil::Execute()
    {
        int returnval = -1;
        try
        {
            utils::MrChronometer chronoexecuter("Total time elapsed");
            switch(m_operationMode)
            {
                case eOpMode::ExportPokeStatsGrowth:
                {
                    returnval = ExportPokeStatsGrowth();
                    break;
                }
                case eOpMode::ImportPokeStatsGrowth:
                {
                    returnval = ImportPokeStatsGrowth();
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
            cerr <<"\n" << "<!>- POCO Exception - " <<e.name() <<"(" <<e.code() <<") : " << e.message() <<endl;
        }
        catch( exception &e )
        {
            cerr <<"\n" << "<!>- Exception - " <<e.what() <<"\n";
        }
        return returnval;
    }

//--------------------------------------------
//  Operation
//--------------------------------------------
    int CStatsUtil::ExportPokeStatsGrowth()
    {
        //

        return 0;
    }
    
    int CStatsUtil::ImportPokeStatsGrowth()
    {
        //

        return 0;
    }

//--------------------------------------------
//  Main Methods
//--------------------------------------------
    int CStatsUtil::Main(int argc, const char * argv[])
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
    using namespace statsutil;
    CStatsUtil & application = CStatsUtil::GetInstance();
    return application.Main(argc,argv);
}