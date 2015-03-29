#include "statsutil.hpp"
#include <ppmdu/utils/utility.hpp>
#include <ppmdu/utils/cmdline_util.hpp>
#include <ppmdu/pmd2/game_stats.hpp>
#include <ppmdu/fmts/waza_p.hpp>
#include <ppmdu/fmts/text_str.hpp>
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

    const std::string CStatsUtil::DefExportStrName   = "game_strings.txt"; 
    const std::string CStatsUtil::DefExportPkmnName  = "pkmn_data.xml";
    const std::string CStatsUtil::DefExportPkmnOutDir= "pkmn_data";
    const std::string CStatsUtil::DefExportMvName    = "moves_data.xml";
    const std::string CStatsUtil::DefExportItemsName = "items_data.xml";
    const std::string CStatsUtil::DefLangConfFile    = "gamelang.xml";

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
        //Force Import
        {
            "i",
            0,
            "Specifying this will force import!",
            "-i",
            std::bind( &CStatsUtil::ParseOptionForceImport, &GetInstance(), placeholders::_1 ),
        },
        //Force Export
        {
            "e",
            0,
            "Specifying this will force export!",
            "-e",
            std::bind( &CStatsUtil::ParseOptionForceExport, &GetInstance(), placeholders::_1 ),
        },
        //pokemon stats only
        {
            "pk",
            0,
            "Specifying this will import or export only Pokemon data!",
            "-pk",
            std::bind( &CStatsUtil::ParseOptionPk, &GetInstance(), placeholders::_1 ),
        },
        //Move data only
        {
            "mvd",
            0,
            "Specifying this will import or export only move data!",
            "-mvd",
            std::bind( &CStatsUtil::ParseOptionMvD, &GetInstance(), placeholders::_1 ),
        },
        //Items data only
        {
            "items",
            0,
            "Specifying this will import or export only items data!",
            "-items",
            std::bind( &CStatsUtil::ParseOptionItems, &GetInstance(), placeholders::_1 ),
        },
        //Game Strings only
        {
            "str",
            0,
            "Specifying this will import or export only the game strings specified!",
            "-str",
            std::bind( &CStatsUtil::ParseOptionStrings, &GetInstance(), placeholders::_1 ),
        },
        //Force a locale string
        {
            "locale",
            1,
            "Force the utility to use the following locale string when importing/exporting the string file!",
            "-locale",
            std::bind( &CStatsUtil::ParseOptionLocaleStr, &GetInstance(), placeholders::_1 ),
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
        m_operationMode   = eOpMode::Invalid;
        m_force           = eOpForce::Export; //Default to export
        m_hndlStrings     = false;
        m_hndlItems       = false;
        m_hndlMoves       = false;
        m_hndlPkmn        = false;
        m_langconf        = DefLangConfFile;
        m_flocalestr      = "";
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

    bool CStatsUtil::ParseOptionPk( const std::vector<std::string> & optdata )
    {
        return m_hndlPkmn = true;
    }

    bool CStatsUtil::ParseOptionMvD( const std::vector<std::string> & optdata )
    {
        return m_hndlMoves = true;
    }

    bool CStatsUtil::ParseOptionItems( const std::vector<std::string> & optdata )
    {
        return m_hndlItems = true;
    }

    bool CStatsUtil::ParseOptionStrings( const std::vector<std::string> & optdata )
    {
        return m_hndlStrings = true;
    }

    bool CStatsUtil::ParseOptionForceImport( const std::vector<std::string> & optdata )
    {
        m_force = eOpForce::Import;
        return true;
    }

    bool CStatsUtil::ParseOptionForceExport( const std::vector<std::string> & optdata )
    {
        m_force = eOpForce::Export;
        return true;
    }

    bool CStatsUtil::ParseOptionLocaleStr  ( const std::vector<std::string> & optdata )
    {
        if( optdata.size() > 1 )
        {
            try
            {
                std::locale( optdata[1] );
            }
            catch(exception & )
            {
                cerr << "ERROR: Invalid locale string specified : \"" <<optdata[1] <<"\"\n";
                clog << "ERROR: Invalid locale string specified : \"" <<optdata[1] <<"\"\n";
                return false;
            }
            m_flocalestr = optdata[1];
            return true;
        }
        return false;
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
        //Poco::Path outpath( m_outputPath ); //if empty crash, poco is
        Poco::File infile( inpath );

        if( m_operationMode != eOpMode::Invalid )
            return; //Skip if we have a forced mode         

        if( !m_outputPath.empty() && !Poco::File( Poco::Path( m_outputPath ).makeAbsolute().parent() ).exists() )
        {
            throw runtime_error("ERROR: Specified output path does not exists!");
        }

        if( infile.exists() )
        {
            if( infile.isFile() && m_force == eOpForce::Import )
            {
                //if( inpath.getExtension() == "txt" || 
                //    ( !m_outputPath.empty() && Poco::Path(m_outputPath).getExtension() == pmd2::filetypes::TextStr_FExt ) )
                //{
                //    m_operationMode = eOpMode::ImportGameStrings;
                //}
                //else if( inpath.getExtension() == pmd2::filetypes::TextStr_FExt || 
                //        ( !m_outputPath.empty() && Poco::Path(m_outputPath).getExtension() == "txt" ) )
                //{
                //    m_operationMode = eOpMode::ExportGameStrings;
                //}
                //else
                //    throw runtime_error("ERROR: Input text file has an unexpected file extension! Cannot determine whether import or export is implied..");
                if( m_hndlStrings )
                    m_operationMode = eOpMode::ImportGameStrings;
                else if( m_hndlItems )
                    m_operationMode = eOpMode::ImportItemsData;
                else if( m_hndlMoves )
                    m_operationMode = eOpMode::ImportMovesData;
                //else if( m_hndlPkmn )
                //    m_operationMode = eOpMode::ImportPokemonData;
                else
                    throw runtime_error("ERROR: Can't import all from a file!");
            }
            else if( infile.isDirectory() )
            {
                if( m_hndlStrings )
                {
                    if( m_force == eOpForce::Export )
                        m_operationMode = eOpMode::ExportGameStrings;
                    else
                        throw runtime_error("ERROR: Can't import game strings from a directory : " + m_inputPath);
                }
                else if( m_hndlItems )
                {
                    if( m_force == eOpForce::Export )
                        m_operationMode = eOpMode::ExportItemsData;
                    else
                        throw runtime_error("ERROR: Can't import Pokemon data from a directory : " + m_inputPath);

                }
                else if( m_hndlMoves )
                {
                    if( m_force == eOpForce::Export )
                        m_operationMode = eOpMode::ExportMovesData;
                    else
                        throw runtime_error("ERROR: Can't import move data from a directory : " + m_inputPath);
                }
                else if( m_hndlPkmn )
                {
                    if( m_force == eOpForce::Export )
                        m_operationMode = eOpMode::ExportPokemonData;
                    else
                        m_operationMode = eOpMode::ImportPokemonData;
                        /*throw runtime_error("ERROR: Can't import Pokemon data from a directory : " + m_inputPath);*/
                }
                else
                {
                    if( m_force == eOpForce::Export )
                        m_operationMode = eOpMode::ExportAll;
                    else if( m_force == eOpForce::Import )
                        m_operationMode = eOpMode::ImportAll;
                }
            }
            else
                throw runtime_error("ERROR: Cannot determine the desired operation!");
        }
        else
            throw runtime_error("ERROR: The input path does not exists!");

    }

    int CStatsUtil::Execute()
    {
        int returnval = -1;
        try
        {
            utils::MrChronometer chronoexecuter("Total time elapsed");
            switch(m_operationMode)
            {
                case eOpMode::ImportGameData:
                {
                    cout << "=== Importing game data ===\n";
                    returnval = DoImportGameData();
                    break;
                }
                case eOpMode::ExportGameData:
                {
                    cout << "=== Exporting game data ===\n";
                    returnval = DoExportGameData();
                    break;
                }
                case eOpMode::ImportPokemonData:
                {
                    cout << "=== Importing Pokemon data ===\n";
                    returnval = DoImportPokemonData();
                    break;
                }
                case eOpMode::ExportPokemonData:
                {
                    cout << "=== Exporting Pokemon data ===\n";
                    returnval = DoExportPokemonData();
                    break;
                }
                case eOpMode::ImportItemsData:
                {
                    cout << "=== Importing items data ===\n";
                    returnval = DoImportItemsData();
                    break;
                }
                case eOpMode::ExportItemsData:
                {
                    cout << "=== Exporting items data ===\n";
                    returnval = DoExportItemsData();
                    break;
                }
                case eOpMode::ImportMovesData:
                {
                    cout << "=== Importing moves data ===\n";
                    returnval = DoImportMovesData();
                    break;
                }
                case eOpMode::ExportMovesData:
                {
                    cout << "=== Exporting moves data ===\n";
                    returnval = DoExportMovesData();
                    break;
                }
                case eOpMode::ImportGameStrings:
                {
                    cout << "=== Importing game strings ===\n";
                    returnval = DoImportGameStrings();
                    break;
                }
                case eOpMode::ExportGameStrings:
                {
                    cout << "=== Exporting game strings ===\n";
                    returnval = DoExportGameStrings();
                    break;
                }
                case eOpMode::ImportAll:
                {
                    cout << "=== Importing ALL ===\n";
                    returnval = DoImportAll();
                    break;
                }
                case eOpMode::ExportAll:
                {
                    cout << "=== Exporting ALL ===\n";
                    returnval = DoExportAll();
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
    int CStatsUtil::DoImportGameData()
    {
        int returnval = -1;
        return returnval;
    }

    int CStatsUtil::DoExportGameData()
    {
        int returnval = -1;
        return returnval;
    }

    int CStatsUtil::DoImportPokemonData()
    {
        Poco::Path inpath(m_inputPath);
        Poco::Path outpath;
        
        if( m_outputPath.empty() )
        {
            throw runtime_error("Output path is empty!");
        }
        else
            outpath = Poco::Path(m_outputPath);

        CGameStats gstats( m_outputPath, m_langconf );
        gstats.ImportPkmn( m_inputPath );
        gstats.WritePkmn(m_outputPath);

        return 0;
    }

    int CStatsUtil::DoExportPokemonData()
    {
        Poco::Path inpath(m_inputPath);
        Poco::Path outpath;
        
        if( m_outputPath.empty() )
        {
            outpath = inpath.absolute().makeParent().append(DefExportPkmnOutDir);
        }
        else
        {
            outpath = Poco::Path(m_outputPath).makeAbsolute();
        }

        //Test output path
        Poco::File fTestOut = outpath;
        if( ! fTestOut.exists() )
        {
            cout << "Created output directory \"" << fTestOut.path() <<"\"!\n";
            fTestOut.createDirectory();
        }

        CGameStats gstats( m_inputPath, m_langconf );
        gstats.LoadPkmn();
        gstats.ExportPkmn( outpath.toString() );

        return 0;
    }

    int CStatsUtil::DoImportItemsData()
    {
        int returnval = -1;
        return returnval;
    }

    int CStatsUtil::DoExportItemsData()
    {
        int returnval = -1;
        return returnval;
    }

    int CStatsUtil::DoImportMovesData()
    {
        int returnval = -1;
        return returnval;
    }

    int CStatsUtil::DoExportMovesData()
    {
        int returnval = -1;
        return returnval;
    }

    int CStatsUtil::DoImportGameStrings()
    {
        Poco::Path inpath(m_inputPath);
        Poco::Path outpath;
        
        if( m_outputPath.empty() )
        {
            outpath = inpath.parent().append(pmd2::filetypes::TextStr_EngFName).makeFile();
        }
        else
            outpath = Poco::Path(m_outputPath);

        if( ! m_flocalestr.empty() )
        {
            auto myloc = std::locale( m_flocalestr );
            pmd2::filetypes::WriteTextStrFile( outpath.toString(), utils::io::ReadTextFileLineByLine( m_inputPath, myloc ), myloc );
        }
        else
        {
            pmd2::filetypes::WriteTextStrFile( outpath.toString(), utils::io::ReadTextFileLineByLine(m_inputPath) );
        }

        return 0;
    }

    int CStatsUtil::DoExportGameStrings()
    {
        Poco::Path inpath(m_inputPath);
        Poco::Path outpath;
        
        if( m_outputPath.empty() )
        {
            outpath = inpath.parent().append(DefExportStrName).makeFile();
        }
        else
            outpath = Poco::Path(m_outputPath);

        CGameStats mystats( m_inputPath, m_langconf );
        mystats.LoadStringsOnly();
        cout << "Writing...\n";
        WriteTextFileLineByLine( mystats.Strings(), outpath.toString() );
        return 0;
    }

    int CStatsUtil::DoImportAll()
    {
        int returnval = -1;
        return returnval;
    }

    int CStatsUtil::DoExportAll()
    {
        int returnval = -1;
        return returnval;
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

#include <ppmdu/fmts/m_level.hpp>
#include <ppmdu/fmts/monster_data.hpp>
#include <ppmdu/fmts/integer_encoding.hpp>
#include <ppmdu/fmts/text_str.hpp>

int TextStringExtractor( int argc, const char *argv[] )
{
    using namespace pmd2::filetypes;
    {
        utils::MrChronometer chronoTester("Total time elapsed");

        //vector<string> text = ParseTextStrFile( argv[1] );
        //WriteTextFileLineByLine(text, "out_strings.txt" );

        vector<string> intext = ReadTextFileLineByLine("out_strings.txt");
        WriteTextStrFile("newtext_e.str", intext );
    }
    return 0;
}

int Sir0Decoder( int argc, const char *argv[] )
{
    using namespace statsutil;
    {
        utils::MrChronometer chronoTester("Total time elapsed");
        cout<<"Decoding..\n";
        vector<uint8_t> encodedptrs = ReadFileToByteVector( argv[1] );
        auto result = utils::DecodeIntegers<uint32_t>( encodedptrs.begin(), encodedptrs.end() );
        cout<<"Done! Decoded " <<result.size() <<" values.\n";

        uint32_t accumulator = 0;
        for( const auto & val : result )
        {
            accumulator += val;
            cout<<"0x"<<hex<<accumulator<<"\n";
        }
        cout<<dec<<"Done!\n";
    }

    return 0;
}

int IntegerDecoder( int argc, const char *argv[] )
{
    using namespace statsutil;
    {
        utils::MrChronometer chronoTester("Total time elapsed");
        vector<uint8_t> encodedints = ReadFileToByteVector( argv[1] );
        auto itRead = encodedints.begin();
        auto itEnd  = encodedints.end();

        auto itfindend = encodedints.begin();
        //Exclude padding if neccessary
        for( ; itfindend != itEnd; ++itfindend )
        {
            if( (*itfindend) == 0xAA && ( std::distance( itfindend, itEnd ) <= 15u ) && std::all_of( itfindend, itEnd, [](uint8_t val){return val == 0xAA;} ) )
                break;
        }
        itEnd = itfindend;


        unsigned int cnt = 0;
        cout<<"Decoding integer lists..\n";
        while( itRead != itEnd )
        {
            cout<<"List #" <<dec <<cnt <<"\n";
        
            vector<uint32_t> result; 
            itRead = utils::DecodeIntegers( itRead, itEnd, back_inserter(result) );
            cout <<"Has " <<result.size() <<" values.\n";

            for( const auto & val : result )
            {
                cout <<"0x" <<uppercase <<hex <<val <<nouppercase <<"\n";
            }
            ++cnt;
        }
        cout<<dec<<"Done!\n";
    }

    return 0;
}

//#TODO: Move the main function somewhere else !
int main( int argc, const char * argv[] )
{
    using namespace statsutil;
    try
    {
        CStatsUtil & application = CStatsUtil::GetInstance();
        return application.Main(argc,argv);
        //utils::MrChronometer chronoTester("Total time elapsed");

        //TESTING
        //cout <<"Loading monster.md..\n";
        //vector<pmd2::stats::PokeMonsterData> md;
        //pmd2::filetypes::ParsePokemonBaseData( "monster.md", md );
        //cout <<"Done\n";

        //cout <<"Writing new monster.md..\n";
        //pmd2::filetypes::WritePokemonBaseData( md, "newmonster.md" );
        //cout <<"Done\n";

        ////TESTING
        //cout <<"Loading m_level.bin..\n";
        //vector<pmd2::stats::PokeStatsGrowth> mlvl;
        //pmd2::filetypes::ParseLevelGrowthData( "m_level.bin", mlvl );
        //cout <<"Done\n";

        //cout <<"Writing new m_level.bin..\n";
        //pmd2::filetypes::WriteLevelGrowthData( mlvl, "newm_level.bin" );
        //cout <<"Done\n";

        //TESTING
        //cout <<"Loading waza_p.bin and waza_p2.bin..\n";
        //auto pairdata = pmd2::filetypes::ParseMoveAndLearnsets( "waza" );
        //cout <<"Done\n";

        //cout <<"Writing new/waza_p.bin and new/waza_p2..\n";
        //Poco::File newdir("new");
        //newdir.createDirectory();
        //pmd2::filetypes::WriteMoveAndLearnsets( "new", pairdata.first, pairdata.second );
        //cout <<"Done\n";

        //TextStringExtractor(argc,argv);
    }
    catch( exception & e )
    {
        cout<< "<!>-ERROR:" <<e.what()<<"\n"
            << "If you get this particular error output, it means an exception got through, and the programmer should be notified!\n";
    }
    //system("pause");

    return 0;
}