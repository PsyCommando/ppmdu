#include "statsutil.hpp"
#include <utils/utility.hpp>
#include <utils/cmdline_util.hpp>
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
    const string CStatsUtil::Version             = "0.21";
    const string CStatsUtil::Short_Description   = "A utility to export and import various game statistics/data, such as pokemon stats.";
    const string CStatsUtil::Long_Description    = 
        "To export game data to XML, you have to append \"-e\" to the\ncommandline, followed with the option corresponding to what to export.\n"
        "You can import data from XML into a PMD2 game's data, by\nspecifying \"-i\" at the commandline, followed with the\noption corresponding to what to import.\n"
        "\n"
        "When importing data from XML, the output path must be a PMD2\ngame's root data directory!\n"
        "When exporting the input path must be a PMD2 game's\nroot data directory!\n";
    const string CStatsUtil::Misc_Text           = 
        "Named in honour of Baz, the awesome Poochyena of doom ! :D\n"
        "My tools in binary form are basically Creative Commons 0.\n"
        "Free to re-use in any ways you may want to!\n"
        "No crappyrights, all wrongs reversed! :3";

    const std::string CStatsUtil::DefExportStrName   = "game_strings.txt"; 
    //const std::string CStatsUtil::DefExportPkmnOutDir= "pkmn_data";
    //const std::string CStatsUtil::DefExportMvDir     = "moves_data";
    //const std::string CStatsUtil::DefExportItemsDir  = "items_data";
    const std::string CStatsUtil::DefExportAllDir    = "exported_data";
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
            "mv",
            0,
            "Specifying this will import or export only move data!",
            "-mv",
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
            "Force the utility to use the following locale string when importing/exporting the string file!"
            " Using this option will bypass using the gamelang.xml file to figure out the game's language when"
            " exporting game strings!",
            "-locale \"C\"",
            std::bind( &CStatsUtil::ParseOptionLocaleStr, &GetInstance(), placeholders::_1 ),
        },
        //Specify path to gamelang.xml
        {
            "gl",
            1,
            "Set the path to the file to use as the \"gamelang.xml\" file!",
            "-gl \"PathToGameLangFile/gamelang.xml\"",
            std::bind( &CStatsUtil::ParseOptionGameLang, &GetInstance(), placeholders::_1 ),
        },

        //Specify path to gamelang.xml
        {
            "log",
            0,
            "Turn on logging to file.",
            "-log",
            std::bind( &CStatsUtil::ParseOptionLog, &GetInstance(), placeholders::_1 ),
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
        m_forcedLocale    = false;
        m_hndlStrings     = false;
        m_hndlItems       = false;
        m_hndlMoves       = false;
        m_hndlPkmn        = false;
        m_langconf        = DefLangConfFile;
        m_flocalestr      = "";
        m_shouldlog       = false;
    }

    const vector<argumentparsing_t> & CStatsUtil::getArgumentsList   ()const { return Arguments_List;    }
    const vector<optionparsing_t>   & CStatsUtil::getOptionsList     ()const { return Options_List;      }
    const argumentparsing_t         * CStatsUtil::getExtraArg        ()const { return nullptr;           } //No extra args
    const string                    & CStatsUtil::getTitle           ()const { return Title;             }
    const string                    & CStatsUtil::getExeName         ()const { return Exe_Name;          }
    const string                    & CStatsUtil::getVersionString   ()const { return Version;           }
    const string                    & CStatsUtil::getShortDescription()const { return Short_Description; }
    const string                    & CStatsUtil::getLongDescription ()const { return Long_Description;  }
    const string                    & CStatsUtil::getMiscSectionText ()const { return Misc_Text;         }

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
                clog << "ERROR: Invalid locale string specified : \"" <<optdata[1] <<"\"\n";
                return false;
            }
            m_flocalestr   = optdata[1];
        }
        //Will use default locale when no locale string is specified
        else
            m_flocalestr   = "";

        m_forcedLocale = true;
        return true;
    }


    bool CStatsUtil::ParseOptionGameLang( const std::vector<std::string> & optdata )
    {
        if( optdata.size() > 1 )
        {
            if( utils::isFile( optdata[1] ) )
            {
                m_langconf = optdata[1];
                cout << "<!>- Set \"" <<optdata[1]  <<"\" as path to gamelang file!\n";
            }
            else
                throw runtime_error("New path to gamelang file does not exists, or is inaccessible!");
        }
        return true;
    }

    bool CStatsUtil::ParseOptionLog( const std::vector<std::string> & optdata )
    {
        m_shouldlog = true;
        m_redirectClog.Redirect( "log.txt" );
        return true;
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

    void CStatsUtil::DetermineOperation()
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
                if( m_hndlStrings )
                {
                    if(m_force == eOpForce::Import)
                    {
                        m_operationMode = eOpMode::ImportGameStrings;
                    }
                    else if( m_force == eOpForce::Export )
                    {
                        m_operationMode = eOpMode::ExportGameStringsFromFile;
                    }
                }
                else
                    throw runtime_error("Can't import anything else than strings from a file!");
            }
            else if( infile.isDirectory() )
            {
                if( m_hndlStrings )
                {
                    if( m_force == eOpForce::Export )
                        m_operationMode = eOpMode::ExportGameStrings;
                    else
                        throw runtime_error("Can't import game strings from a directory : " + m_inputPath);
                }
                else if( m_hndlItems )
                {
                    if( m_force == eOpForce::Export )
                        m_operationMode = eOpMode::ExportItemsData;
                    else
                        m_operationMode = eOpMode::ImportItemsData;
                }
                else if( m_hndlMoves )
                {
                    if( m_force == eOpForce::Export )
                        m_operationMode = eOpMode::ExportMovesData;
                    else
                        m_operationMode = eOpMode::ImportMovesData;

                }
                else if( m_hndlPkmn )
                {
                    if( m_force == eOpForce::Export )
                        m_operationMode = eOpMode::ExportPokemonData;
                    else
                        m_operationMode = eOpMode::ImportPokemonData;
                }
                else
                {
                    if( m_force == eOpForce::Import || isImportAllDir(m_inputPath) )
                        m_operationMode = eOpMode::ImportAll;
                    else
                        m_operationMode = eOpMode::ExportAll; //If all else fails, try an export all!
                }
            }
            else
                throw runtime_error("Cannot determine the desired operation!");
        }
        else
            throw runtime_error("The input path does not exists!");

    }

    int CStatsUtil::Execute()
    {
        int returnval = -1;
        utils::MrChronometer chronoexecuter("Total time elapsed");
        try
        {
            switch(m_operationMode)
            {
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
                case eOpMode::ExportGameStringsFromFile:
                {
                    cout << "=== Exporting game strings from file directly ===\n";
                    returnval = DoExportGameStringsFromFile();
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
        if( m_outputPath.empty() )
            throw runtime_error("Output path is empty!");
        if( !utils::isFolder( m_outputPath ) )
            throw runtime_error("Output path doesn't exist, or isn't a directory!");

        CGameStats gstats( m_outputPath, m_langconf );
        gstats.ImportPkmn( m_inputPath );
        gstats.WritePkmn( m_outputPath );
        return 0;
    }

    int CStatsUtil::DoExportPokemonData()
    {
        Poco::Path inpath(m_inputPath);
        Poco::Path outpath;
        
        if( m_outputPath.empty() )
        {
            outpath = inpath.absolute().makeParent().append(CGameStats::DefPkmnDir);
        }
        else
        {
            outpath = Poco::Path(m_outputPath).makeAbsolute();
        }

        CGameStats gstats( m_inputPath, m_langconf );
        gstats.LoadPkmn();

        //Test output path
        Poco::File fTestOut = outpath;
        if( ! fTestOut.exists() )
        {
            cout << "Created output directory \"" << fTestOut.path() <<"\"!\n";
            fTestOut.createDirectory();
        }
        gstats.ExportPkmn( outpath.toString() );

        return 0;
    }

    int CStatsUtil::DoImportItemsData()
    {
        if( m_outputPath.empty() )
            throw runtime_error("Output path is empty!");
        if( !utils::isFolder( m_outputPath ) )
            throw runtime_error("Output path doesn't exist, or isn't a directory!");

        CGameStats gstats( m_outputPath, m_langconf );
        gstats.ImportItems( m_inputPath );
        gstats.WriteItems( m_outputPath );
        return 0;
    }

    int CStatsUtil::DoExportItemsData()
    {
        Poco::Path inpath(m_inputPath);
        Poco::Path outpath;
        
        if( m_outputPath.empty() )
        {
            outpath = inpath.absolute().makeParent().append(CGameStats::DefItemsDir);
        }
        else
        {
            outpath = Poco::Path(m_outputPath).makeAbsolute();
        }

        CGameStats gstats( m_inputPath, m_langconf );
        gstats.LoadItems();

        //Test output path
        Poco::File fTestOut = outpath;
        if( ! fTestOut.exists() )
        {
            cout << "Created output directory \"" << fTestOut.path() <<"\"!\n";
            fTestOut.createDirectory();
        }
        gstats.ExportItems( outpath.toString() );

        return 0;
    }

    int CStatsUtil::DoImportMovesData()
    {
        Poco::Path inpath(m_inputPath);
        Poco::Path outpath;
        
        if( m_outputPath.empty() )
            throw runtime_error("Output path is empty!");
        if( !utils::isFolder( m_outputPath ) )
            throw runtime_error("Output path doesn't exist, or isn't a directory!");

        outpath = Poco::Path(m_outputPath);

        CGameStats gstats ( m_outputPath, m_langconf );
        gstats.ImportMoves( m_inputPath );
        gstats.WriteMoves ( m_outputPath );

        return 0;
    }

    int CStatsUtil::DoExportMovesData()
    {
        Poco::Path inpath(m_inputPath);
        Poco::Path outpath;
        
        if( m_outputPath.empty() )
        {
            outpath = inpath.absolute().makeParent().append(CGameStats::DefMvDir);
        }
        else
        {
            outpath = Poco::Path(m_outputPath).makeAbsolute();
        }

        CGameStats gstats( m_inputPath, m_langconf );
        gstats.LoadMoves();

        //Test output path
        Poco::File fTestOut = outpath;
        if( ! fTestOut.exists() )
        {
            cout << "Created output directory \"" << fTestOut.path() <<"\"!\n";
            fTestOut.createDirectory();
        }
        gstats.ExportMoves( outpath.toString() );

        return 0;
    }

    int CStatsUtil::DoImportGameStrings()
    {
        Poco::Path inpath(m_inputPath);
        Poco::Path outpath;
        
        if( m_outputPath.empty() )
            throw runtime_error("Output path unspecified!");

        if( utils::isFolder( m_outputPath ) )
        {
            outpath = Poco::Path(m_outputPath).makeAbsolute().makeDirectory();
            CGameStats gstats( outpath.toString(), m_langconf );
            gstats.AnalyzeGameDir();
            gstats.ImportStrings( m_inputPath );
            gstats.WriteStrings();
        }
        else
        {
            outpath = Poco::Path(m_outputPath).makeAbsolute();

            if( ! m_flocalestr.empty() )
            {
                auto myloc = std::locale( m_flocalestr );
                pmd2::filetypes::WriteTextStrFile( outpath.toString(), utils::io::ReadTextFileLineByLine( m_inputPath, myloc ), myloc );
            }
            else
            {
                pmd2::filetypes::WriteTextStrFile( outpath.toString(), utils::io::ReadTextFileLineByLine(m_inputPath) );
            }
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
        {
            Poco::File outfilecheck(m_outputPath);  //Check if output is a directory

            if( outfilecheck.exists() && outfilecheck.isDirectory() )
                outpath = Poco::Path(m_outputPath).append(DefExportStrName).makeFile();
            else
                outpath = Poco::Path(m_outputPath);
        }

        if( !m_forcedLocale )
        {
            cout << "Detecting game language...\n";
            CGameStats mystats( m_inputPath, m_langconf );
            mystats.LoadStrings();
            cout << "Writing...\n";
            mystats.ExportStrings( outpath.toString() );
        }
        else
        {
            cout << "A forced locale string was specified! Skipping game language detection.\n";
            vector<string> gamestrings;
            pmd2::filetypes::ParseTextStrFile( inpath.toString(), std::locale(m_flocalestr) );
            WriteTextFileLineByLine( gamestrings, outpath.toString() );
        }
        return 0;
    }

    //For exporting the game strings from the text_*.str file directly
    int CStatsUtil::DoExportGameStringsFromFile()
    {
        Poco::Path inpath(m_inputPath);
        Poco::Path outpath;
        
        if( m_outputPath.empty() )
        {
            outpath = inpath.parent().append(DefExportStrName).makeFile();
        }
        else
            outpath = Poco::Path(m_outputPath);

        vector<string> gamestrings = pmd2::filetypes::ParseTextStrFile( inpath.toString(), std::locale(m_flocalestr) );
        WriteTextFileLineByLine( gamestrings, outpath.toString() );
        return 0;
    }

    int CStatsUtil::DoImportAll()
    {
        Poco::Path inpath(m_inputPath);
        Poco::Path outpath;
        
        if( m_outputPath.empty() )
            throw runtime_error("Output path is empty!");
            
        if( !utils::isFolder( m_outputPath ) )
            throw runtime_error("Output path doesn't exist, or isn't a directory!");

        outpath = Poco::Path(m_outputPath);

        CGameStats gstats ( m_outputPath, m_langconf );
        gstats.ImportAll( m_inputPath );
        gstats.Write();
        return 0;
    }

    int CStatsUtil::DoExportAll()
    {
        Poco::Path inpath(m_inputPath);
        Poco::Path outpath;
        
        if( m_outputPath.empty() )
        {
            outpath = inpath.absolute().makeParent().append(DefExportAllDir);
        }
        else
        {
            outpath = Poco::Path(m_outputPath).makeAbsolute();
        }

        CGameStats gstats( m_inputPath, m_langconf );
        gstats.Load();

        //Test output path
        Poco::File fTestOut = outpath;
        if( ! fTestOut.exists() )
        {
            cout << "Created output directory \"" << fTestOut.path() <<"\"!\n";
            fTestOut.createDirectory();
        }
        gstats.ExportAll(outpath.toString());
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
        utils::PortablePause();
#endif

        return returnval;
    }
};

//=================================================================================================
// Main Function
//=================================================================================================

#include <ppmdu/fmts/integer_encoding.hpp>

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
    }
    catch( exception & e )
    {
        cout<< "<!>-ERROR:" <<e.what()<<"\n"
            << "If you get this particular error output, it means an exception got through, and the programmer should be notified!\n";
    }

    return 0;
}