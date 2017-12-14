#include "statsutil.hpp"
#include <utils/utility.hpp>
#include <utils/cmdline_util.hpp>
#include <ppmdu/pmd2/pmd2_gameloader.hpp>
#include <types/content_type_analyser.hpp>
//#include <ppmdu/pmd2/game_stats.hpp>
#include <ppmdu/fmts/waza_p.hpp>
#include <ppmdu/fmts/text_str.hpp>
#include <utils/library_wide.hpp>
//#include <ppmdu/pmd2/pmd2_scripts.hpp>
#include <ppmdu/pmd2/pmd2_xml_sniffer.hpp>
#include <ppmdu/pmd2/pmd2_asm.hpp>
#include <utils/poco_wrapper.hpp>
#include <utils/whereami_wrapper.hpp>
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
using namespace ::pmd2;


namespace statsutil
{
    const string XML_FExt = "xml";

//=================================================================================================
//  CStatsUtil
//=================================================================================================

//------------------------------------------------
//  Constants
//------------------------------------------------
    const string CStatsUtil::Exe_Name            = "ppmd_statsutil.exe";
    const string CStatsUtil::Title               = "Game data importer/exporter";
    const string CStatsUtil::Version             = "0.24";
    const string CStatsUtil::Short_Description   = "A utility to export and import various game statistics/data, such as pokemon stats.";
    const string CStatsUtil::Long_Description    = 
        "To export game data to XML, you have to append \"-e\" to the\ncommandline, followed with the option corresponding to what to export.\n"
        "You can import data from XML into a PMD2 game's data, by\nspecifying \"-i\" at the commandline, followed with the\noption corresponding to what to import.\n"
        "\n";
    const string CStatsUtil::Misc_Text           = 
        "Named in honour of Baz, the awesome Poochyena of doom ! :D\n"
        "My tools in binary form are basically Creative Commons 0.\n"
        "Free to re-use in any ways you may want to!\n"
        "No crappyrights, all wrongs reversed! :3";

    //const std::string CStatsUtil::DefExportStrName       = "game_strings.txt"; 
    const std::string CStatsUtil::DefExportStrDirName    = "game_strings";
    const std::string CStatsUtil::DefExportScriptDirName = "scripts";
    //const std::string CStatsUtil::DefExportPkmnOutDir= "pkmn_data";
    //const std::string CStatsUtil::DefExportMvDir     = "moves_data";
    //const std::string CStatsUtil::DefExportItemsDir  = "items_data";
    const std::string CStatsUtil::DefExportAllDir        = "exported_data";
    //const std::string CStatsUtil::DefLangConfFile        = "gamelang.xml";

    //const std::string CStatsUtil::DefExportScriptsDir = "exported_scripts";

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
            "first argument", 
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
            std::bind( &CStatsUtil::ParseOutputPath,       &GetInstance(), placeholders::_1 ),
            std::bind( &CStatsUtil::ShouldParseOutputPath, &GetInstance(), placeholders::_1, placeholders::_2, placeholders::_3 ),
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
//!#TODO: Fix the command line parameters because this is really confusing!!!!!!!!!!!!!!!!!
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
            "pokemon",
            0,
            "Specifying this will import or export only Pokemon data!",
            "-pokemon",
            std::bind( &CStatsUtil::ParseOptionPk, &GetInstance(), placeholders::_1 ),
        },
        //Move data only
        {
            "moves",
            0,
            "Specifying this will import or export only move data!",
            "-moves",
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
            "text",
            0,
            "Specifying this will import or export only the game strings specified!",
            "-text",
            std::bind( &CStatsUtil::ParseOptionStrings, &GetInstance(), placeholders::_1 ),
        },
        //Game Scripts only
        {
            "scripts",
            0,
            "Specifying this will import or export only the game scripts!",
            "-scripts",
            std::bind( &CStatsUtil::ParseOptionScripts, &GetInstance(), placeholders::_1 ),
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

        //Set path to PMD2 Config file
        {
            "scriptdebug",
            0,
            "When specified, all debug related commands in the scripts are tweaked so they're enabled by default!",
            "-scriptdebug",
            std::bind( &CStatsUtil::ParseOptionScriptEnableDebugInstr, &GetInstance(), placeholders::_1 ),
        },

        //Set path to PMD2 Config file
        {
            "cfg",
            1,
            "Set a non-default path to the pmd2data.xml file.",
            "-cfg \"path/to/pmd2/config/data/file\"",
            std::bind( &CStatsUtil::ParseOptionConfig, &GetInstance(), placeholders::_1 ),
        },

        //Escape text using XML escape sequences
        {
            "xmlesc",
            0,
            "If present, this makes the program use XML/HTML escape sequences, instead of C ones. "
            "AKA &#x0A; instead of \\n for example. "
            "This is mainly interesting for dealing with a XML parser that requires standard XML escape chars!",
            "-xmlesc",
            std::bind( &CStatsUtil::ParseOptionEscapeAsXML, &GetInstance(), placeholders::_1 ),
        },

        //Export/Import script data as directories
        {
            "scrasdir",
            0,
            "If present, scripts will be exported as sub files within a directory, instead of a single huge xml file.",
            "-scrasdir",
            std::bind( &CStatsUtil::ParseOptionScriptAsDir, &GetInstance(), placeholders::_1 ),
        },
////////////////////////////////////////////////////////////////////////////////////////////

        //Specify the root of the extracted rom directory to work with
        {
            "romroot",
            1,
            "Specify the root of the extracted rom directory to work with! The directory must contain "
            "both a \"data\" and \"overlay\" directory, and at least a \"arm9.bin\" file! "
            "The \"data\" directory must contain the rom's files and directories!"
            "The \"overlay\" directory must contain the rom's many \"overlay_00xx.bin\" files!",
            "-romroot \"path/to/extracted/rom/root/directory\"",
            std::bind( &CStatsUtil::ParseOptionRomRoot, &GetInstance(), placeholders::_1 ),
        },


        //DumpALevelListFile
        {
            "dumplevellist",
            0,
            "Dump a level_list.bin file from the current configuration.",
            "-dumplevellist",
            std::bind( &CStatsUtil::ParseOptionDumpLvlList, &GetInstance(), placeholders::_1 ),
        },

        //dumpactorlist
        {
            "dumpactorlist",
            0, 
            "Dump a actor_list.bin file from the current configuration.",
            "-dumpactorlist",
            std::bind( &CStatsUtil::ParseOptionDumpActorList, &GetInstance(), placeholders::_1 ),
        },

////////////////////////////////////////////////////////////////////////////////////////////
        //Set nb threads to use
        {
            "th",
            1,
            "Used to set the maximum number of threads to use for various tasks during execution.",
            "-th 2",
            std::bind( &CStatsUtil::ParseOptionThreads, &GetInstance(), placeholders::_1 ),
        },
        //Turns on logging
        {
            "log",
            0,
            "Turn on logging to file.",
            "-log",
            std::bind( &CStatsUtil::ParseOptionLog, &GetInstance(), placeholders::_1 ),
        },
    }};



    inline void CreateDirIfDoesntExist( Poco::File dir )
    {
        if( !dir.exists() )
        {
            cout<<"Created directory : " <<dir.path() <<"\n";
            dir.createDirectory();
        }
    }


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
        m_force           = eOpForce::None;
        m_forcedLocale    = false;
        m_hndlStrings     = false;
        m_hndlItems       = false;
        m_hndlMoves       = false;
        m_hndlPkmn        = false;
        //m_langconf        = DefLangConfFile;
        m_pmd2cfg         = DefConfigFileName;
        m_flocalestr      = "";
        m_shouldlog       = false;
        m_hndlScripts     = false;
        m_escxml          = false;
        m_region          = eGameRegion::NorthAmerica;
        m_version         = eGameVersion::EoS;
        m_scriptdebug     = false;
        m_dumplvllist     = false;
        m_dumpactorlist   = false;
        m_scriptasdir     = false;
        utils::LibWide().StringValue(ScriptCompilerReportFname) = "compiler_report.txt"; //Set this keyvalue to our default report filename!
    }

    CStatsUtil::~CStatsUtil()
    {
        //Clear the logger before the destructor of the output stream redirect is ran in the base class!
        utils::LibWide().Logger(nullptr); 
        CommandLineUtility::~CommandLineUtility();
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
        Poco::Path inputfile(path);

        if( inputfile.isFile() || inputfile.isDirectory() )
        {
            m_firstparam = path;
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

    bool CStatsUtil::ShouldParseOutputPath( const std::vector<std::vector<std::string>> & optdata, 
                                            const std::deque<std::string>               & priorparams,
                                            size_t                                        nblefttoparse )
    {
        return ( priorparams.size() == 1 ) && ( nblefttoparse > 0 );
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

    bool CStatsUtil::ParseOptionScripts( const std::vector<std::string> & optdata )
    {
        return m_hndlScripts = true;
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
                if( utils::LibWide().isLogOn() )
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

    bool CStatsUtil::ParseOptionConfig( const std::vector<std::string> & optdata )
    {
        if( optdata.size() > 1 )
        {
            //assert(!m_applicationdir.empty());
            //Poco::Path cfgpath(m_applicationdir);
            //cfgpath.resolve(optdata[1]);  //If cfg path is relative, it gets appended. If absolute, it replaces the app dir path!

            //if( utils::isFile(cfgpath.toString()) )
            //{
                m_pmd2cfg = optdata[1];
                cout << "<!>- Set \"" <<m_pmd2cfg  <<"\" as path to pmd2data file!\n";
            //}
            //else
            //    throw runtime_error("New path to pmd2data file does not exists, or is inaccessible!");
            return true;
        }
        return false;
    }

    bool CStatsUtil::ParseOptionLog( const std::vector<std::string> & optdata )
    {
        m_shouldlog = true;
        m_redirectClog.Redirect( "log.txt" );
        utils::LibWide().isLogOn(true);
        cout <<"<!>- Enabled logging to current working directory!\n";
        return true;
    }

    bool CStatsUtil::ParseOptionRomRoot( const std::vector<std::string> & optdata )
    {
        if( optdata.size() > 1 )
        {
            if( utils::isFolder( optdata[1] ) )
            {
                m_romrootdir = optdata[1];
                cout << "<!>- Set \"" <<optdata[1]  <<"\" as ROM root directory!\n";
            }
            else
                throw runtime_error("Path to ROM root directory does not exists, or is inaccessible!");
        }
        return true;
    }

    bool CStatsUtil::ParseOptionThreads( const std::vector<std::string> & optdata )
    {
        stringstream sstr;
        unsigned int nbthreads = 1;

        sstr << optdata[1];
        sstr >> nbthreads;

        if( nbthreads < thread::hardware_concurrency() )
            utils::LibWide().setNbThreadsToUse(nbthreads);
        else 
            utils::LibWide().setNbThreadsToUse(thread::hardware_concurrency());

        cout << "<!>- Set to use " <<utils::LibWide().getNbThreadsToUse() <<" threads !\n";
        return true;
    }

    bool CStatsUtil::ParseOptionEscapeAsXML( const std::vector<std::string> & optdata )
    {
        return m_escxml = true;
    }

    bool CStatsUtil::ParseOptionScriptEnableDebugInstr(const std::vector<std::string>& optdata)
    {
        cout << "<!>- Outputing debug enabled scripts!\n";
        return m_scriptdebug = true;
    }

    bool CStatsUtil::ParseOptionDumpLvlList( const std::vector<std::string> & optdata )
    {
        cout << "<!>- Dumping level_list.bin from current configuration!\n";
        m_operationMode = eOpMode::DumpLevelList;
        return m_dumplvllist = true;
    }

    bool CStatsUtil::ParseOptionDumpActorList( const std::vector<std::string> & optdata )
    {
        cout << "<!>- Dumping actor_list.bin from current configuration!\n";
        m_operationMode = eOpMode::DumpActorList;
        return m_dumpactorlist = true;
    }

    bool CStatsUtil::ParseOptionScriptAsDir(const std::vector<std::string> & optdata )
    {
        cout << "<!>- Exporting/Importing Script XML as Directories!\n";
        return m_scriptasdir = true;
    }

    void CStatsUtil::SetupCFGPath(const std::string & cfgrelpath)
    {
        assert(!m_applicationdir.empty());
        Poco::Path cfgpath(m_applicationdir);
        cfgpath.makeDirectory();
        cfgpath.resolve(cfgrelpath);  //If cfg path is relative, it gets appended. If absolute, it replaces the app dir path!

        if( utils::isFile(cfgpath.toString()) )
            m_pmd2cfg = cfgpath.toString();
        else
            throw std::logic_error("CStatsUtil::SetupCFGPath(): Path to pmd2data xml file is non-existant! " + cfgpath.toString());
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
//! #TODO: Eventually put the parsing of the program's directory into the cmdlineutil baseclass!
            m_applicationdir = utils::GetPathExeDirectory();
            utils::LibWide().StringValue(utils::lwData::eBasicValues::ProgramExeDir) = m_applicationdir;
            utils::LibWide().StringValue(utils::lwData::eBasicValues::ProgramLogDir) = utils::getCWD();
            if( !SetArguments(argc,argv) ) //This parses the command line!!!!
                return -3;
            //Now that the command line is parsed, do stuff with it
            if(utils::LibWide().isLogOn())
            {
                utils::LibWide().Logger(new logging::SingleOutMTLogger);
                utils::LibWide().Logger() << "Logger initiated!\n";
            }
            SetupCFGPath(m_pmd2cfg);
            DetermineOperation();
        }
        catch( const Poco::Exception & pex )
        {
            stringstream sstr;
            sstr <<"\n<!>-POCO Exception - " <<pex.name() <<"(" <<pex.code() <<") : " << pex.message() <<"\n" <<endl;
            string strer = sstr.str(); 
            cerr << strer;

            if( utils::LibWide().isLogOn() )
                clog << strer;

            cout <<"=======================================================================\n"
                 <<"Readme\n"
                 <<"=======================================================================\n";
            PrintReadme();
            return pex.code();
        }
        catch( const exception & e )
        {
            stringstream sstr;
            sstr <<"\n<!>-Exception: " << e.what() <<"\n" <<endl;
            string strer = sstr.str(); 
            cerr << strer;

            if( utils::LibWide().isLogOn() )
                clog << strer;

            cout <<"=======================================================================\n"
                 <<"Readme\n"
                 <<"=======================================================================\n";
            PrintReadme();
            return -3;
        }
        return returnval;
    }



    void CStatsUtil::DetermineOperation()
#if 1
    {
        Poco::Path inpath( m_firstparam );
        Poco::File infile( inpath );

        if( m_operationMode != eOpMode::Invalid )
            return; //Skip if we have a forced mode
        if( !m_romrootdir.empty() && m_force == eOpForce::None )
            throw runtime_error("CStatsUtil::DetermineOperation() : We got a ROM root directory, but no import or export specifier!");
        else if( !m_romrootdir.empty() && m_force != eOpForce::None )
            return;

        //!#TODO: Handle drag and drop
        cerr <<"#TODO: Fix file handling and detection!\n";
        if( !infile.exists() )
            throw runtime_error("The input path does not exists!");
        
        if(infile.isFile())
        {
            const string pathext = inpath.getExtension();
            
            if( pathext == ::filetypes::SSB_FileExt )
                m_operationMode = eOpMode::ExportSingleScript;
            else if( pathext == ::filetypes::SSA_FileExt || pathext == ::filetypes::SSS_FileExt || pathext == ::filetypes::SSE_FileExt )
                m_operationMode = eOpMode::ExportSingleScriptData;
            else if( pathext == XML_FExt && DetermineXMLOps(m_firstparam) )
                return;
        }
        else if( infile.isDirectory() )
        {
            assert(false);
            throw std::runtime_error("Directory handling not yet implemented #TODO");
        }
        else
            throw runtime_error("Cannot determine the desired operation!");
            
    }

    bool CStatsUtil::DetermineXMLOps( const std::string & filepath )
    {
        RootNodeInfo rootnode = GetRootNodeFromXML(filepath);
        CheckGameVersionAndGameRegion(rootnode, m_version, m_region);

        if( IsXMLSingleScript(rootnode.name) )
            m_operationMode = eOpMode::ImportSingleScript;
        else if(IsXMLSingleScriptData(rootnode.name))
            m_operationMode = eOpMode::ImportSingleScriptData;
        else //! #TODO:Check for other kind of xml files we could be given!!
        {
            assert(false);
        }
        return false;
    }

#else
    {
        Poco::Path inpath( m_firstparam );
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
                        throw runtime_error("Can't import game strings from a directory : " + m_firstparam);
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
                else if( m_hndlScripts )
                {
                    if( m_force == eOpForce::Export )
                        m_operationMode = eOpMode::ExportGameScripts;
                    else
                        m_operationMode = eOpMode::ImportGameScripts;
                }
                else
                {
                    if( m_force == eOpForce::Import || isImportAllDir(m_firstparam) )
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
#endif

    void CStatsUtil::ValidateRomRoot()const
    {
        if( m_romrootdir.empty() )
            throw runtime_error("No extracted ROM root directory was specified using the \"-romroot\" option!");
        if( !utils::isFolder( m_romrootdir ) )
            throw runtime_error("Extracted ROM root directory path doesn't exist, or isn't a directory!");
    }

    int CStatsUtil::Execute()
    {
        int returnval = -1;
        utils::MrChronometer chronoexecuter("Total time elapsed");
        try
        {
            if( m_force != eOpForce::None )
            {
                ValidateRomRoot();
                GameDataLoader gloader( m_romrootdir, m_pmd2cfg );
                gloader.AnalyseGame();
                cout <<"\n";
                switch(m_force)
                {
                    case eOpForce::Import:
                    {
                        cout <<"================================================\n"
                             <<"                     Import                     \n"
                             <<"================================================\n\n"
                            ;
                        returnval = HandleImport(m_firstparam, gloader);
                        break;
                    }
                    case eOpForce::Export:
                    {
                        cout <<"================================================\n"
                             <<"                     Export                     \n"
                             <<"================================================\n\n"
                            ;
                        returnval = HandleExport(m_firstparam, gloader);
                        break;
                    }
                };
            }
            else if(!m_romrootdir.empty())
            {
                ValidateRomRoot();
                GameDataLoader gloader( m_romrootdir, m_pmd2cfg );
                gloader.AnalyseGame();

                switch(m_operationMode)
                {
                    case eOpMode::DumpLevelList: //! #TODO: This will move to its own utility, along with all ASM patching!!
                    {
                        ValidateRomRoot();
                        return DoDumpLevelList(m_firstparam, gloader );
                    }

                    case eOpMode::DumpActorList: //! #TODO: This will move to its own utility, along with all ASM patching!!
                    {
                        ValidateRomRoot();
                        return DoDumpActorList(m_firstparam, gloader );
                    }
                };
            }
            else //This is for mainly drag and drop stuff!!
            {
                //!Handle drag and drop!
                //assert(false);
                switch(m_operationMode)
                {
                    case eOpMode::ExportSingleScript:
                    {
                        cout<<"================================================\n"
                            <<"Exporting script...\n"
                            <<"================================================\n";
                        returnval = DoExportSingleScript();
                        break;
                    }
                    case eOpMode::ImportSingleScript:
                    {
                        cout <<"================================================\n"
                             <<"Importing script...\n"
                             <<"================================================\n";
                        returnval = DoImportSingleScript();
                        break;
                    }
                    case eOpMode::ExportSingleScriptData:
                    {
                        cout<<"================================================\n"
                            <<"Exporting script data...\n"
                            <<"================================================\n";
                        returnval = DoExportSingleScriptData();
                        break;
                    }
                    case eOpMode::ImportSingleScriptData:
                    {
                        cout <<"================================================\n"
                             <<"Importing script data...\n"
                             <<"================================================\n";
                        returnval = DoImportSingleScriptData();
                        break;
                    }
                };
            }

            ////OLD 
            //switch(m_operationMode)
            //{
            //    case eOpMode::ImportPokemonData:
            //    {
            //        cout << "=== Importing Pokemon data ===\n";
            //        returnval = DoImportPokemonData();
            //        break;
            //    }
            //    case eOpMode::ExportPokemonData:
            //    {
            //        cout << "=== Exporting Pokemon data ===\n";
            //        returnval = DoExportPokemonData();
            //        break;
            //    }
            //    case eOpMode::ImportItemsData:
            //    {
            //        cout << "=== Importing items data ===\n";
            //        returnval = DoImportItemsData();
            //        break;
            //    }
            //    case eOpMode::ExportItemsData:
            //    {
            //        cout << "=== Exporting items data ===\n";
            //        returnval = DoExportItemsData();
            //        break;
            //    }
            //    case eOpMode::ImportMovesData:
            //    {
            //        cout << "=== Importing moves data ===\n";
            //        returnval = DoImportMovesData();
            //        break;
            //    }
            //    case eOpMode::ExportMovesData:
            //    {
            //        cout << "=== Exporting moves data ===\n";
            //        returnval = DoExportMovesData();
            //        break;
            //    }
            //    case eOpMode::ImportGameStrings:
            //    {
            //        cout << "=== Importing game strings ===\n";
            //        returnval = DoImportGameStrings();
            //        break;
            //    }
            //    case eOpMode::ExportGameStrings:
            //    {
            //        cout << "=== Exporting game strings ===\n";
            //        returnval = DoExportGameStrings();
            //        break;
            //    }
            //    case eOpMode::ExportGameStringsFromFile:
            //    {
            //        cout << "=== Exporting game strings from file directly ===\n";
            //        returnval = DoExportGameStringsFromFile();
            //        break;
            //    }
            //    case eOpMode::ImportGameScripts:
            //    {
            //        cout << "=== Importing game scripts ===\n";
            //        returnval = DoImportGameScripts();
            //        break;
            //    }
            //    case eOpMode::ExportGameScripts:
            //    {
            //        cout << "=== Exporting game scripts ===\n";
            //        returnval = DoExportGameScripts();
            //        break;
            //    }

            //    case eOpMode::ImportAll:
            //    {
            //        cout << "=== Importing ALL ===\n";
            //        returnval = DoImportAll();
            //        break;
            //    }
            //    case eOpMode::ExportAll:
            //    {
            //        cout << "=== Exporting ALL ===\n";
            //        returnval = DoExportAll();
            //        break;
            //    }
            //    default:
            //    {
            //        throw runtime_error( "Invalid operation mode. Something is wrong with the arguments!" );
            //    }
            //};
        }
        catch( const Poco::Exception & e )
        {
            cerr <<"\n" << "<!>- POCO Exception - " <<e.name() <<"(" <<e.code() <<") : " << e.message() <<"\n" <<endl;
            if( utils::LibWide().isLogOn() )
                clog <<"\n" << "<!>- POCO Exception - " <<e.name() <<"(" <<e.code() <<") : " << e.message() <<"\n" <<endl;
        }
        catch( const exception &e )
        {
            cerr <<"\n<!>- Exception caught! :\n";
            stringstream strex;
            utils::PrintNestedExceptions( strex, e );
            const string exceptstr = strex.str();

            cerr <<exceptstr;
            if( utils::LibWide().isLogOn() )
                clog <<exceptstr;
        }
        return returnval;
    }

//--------------------------------------------
//  Operation
//--------------------------------------------

    int CStatsUtil::HandleImport( const std::string & frompath, pmd2::GameDataLoader & gloader )
    {
        bool        bhandleall = !m_hndlStrings && !m_hndlItems && !m_hndlMoves && !m_hndlPkmn && !m_hndlScripts;
        GameStats * pgamestats = nullptr; //Put this here, because several stat import uses it. 
        GameText  * pgametext  = nullptr;
        
        if(m_hndlStrings || bhandleall)
        {
            cout <<"\nGame Strings\n"
                 <<"---------------------------------\n"
                 <<"Reading..\n";

            pgametext = gloader.InitGameText();
            if( !pgametext )
                throw std::runtime_error("CStatsUtil::HandleImport(): Couldn't load game text!");

            Poco::Path textdir(frompath);
            textdir.append(DefExportStrDirName);
            if( utils::isFolder(textdir.toString()) )
            {
                pgametext->ImportText(textdir.toString());
                //We'll write the file at the end, after all modifications are done!
                cout <<"done!\n";
            }
            else
            {
                cout <<"Failed to import game strings.. Subdirectory " <<DefExportStrDirName
                     <<" is missing from specified input directory.\n Skipping over...\n";
            }
        }

        if(m_hndlScripts || bhandleall)
        {
            cout <<"\nScripts\n"
                 <<"---------------------------------\n";
            GameScripts * pgamescripts = gloader.InitScripts(pmd2::scriptprocoptions{true, true, false, m_scriptdebug, m_scriptasdir});
            if(!pgamescripts)
                throw std::runtime_error("CStatsUtil::HandleImport(): Couldn't load scripts!");

            Poco::Path scriptdir(frompath);
            scriptdir.append(DefExportScriptDirName);
            if( utils::isFolder(scriptdir.toString()) )
            {
                pgamescripts->ImportXML( scriptdir.toString() ); //Writing is done automatically
            }
            else
            {
                cout <<"Failed to import game scripts.. Subdirectory " <<DefExportScriptDirName
                     <<" is missing from specified input directory.\n Skipping over...\n";
            }
        }

        //Load game stats for the other elements as needed
        if( m_hndlPkmn || m_hndlMoves || m_hndlItems || bhandleall )
        {
            cout <<"\nLoading Game Data..\n"
                 <<"---------------------------------\n";
           pgamestats = gloader.InitStats();
            if(!pgamestats)
                throw std::runtime_error("CStatsUtil::HandleImport(): Couldn't load game stats!");
        }

        if(m_hndlPkmn || bhandleall)
        {
            cout <<"\nPokemon Data\n"
                 <<"---------------------------------\n";
            Poco::Path Pkmndatadir(frompath);
            Pkmndatadir.append(pmd2::GameStats::DefPkmnDir);
            if( utils::isFolder(Pkmndatadir.toString()) )
            {
                pgamestats->ImportPkmn(Pkmndatadir.toString());
                pgamestats->WritePkmn();
            }
            else
            {
                cout <<"Failed to import pokemon data.. Subdirectory " <<pmd2::GameStats::DefPkmnDir
                     <<" is missing from specified input directory.\n Skipping over...\n";
            }
        }

        if(m_hndlMoves || bhandleall)
        {
            cout <<"\nPokemon Move Data\n"
                 <<"-----------------------------------\n";
            Poco::Path mvdatadir(frompath);
            mvdatadir.append(pmd2::GameStats::DefMvDir);

            if( utils::isFolder(mvdatadir.toString()) )
            {
                pgamestats->ImportMoves(mvdatadir.toString());
                pgamestats->WriteMoves ();
            }
            else
            {
                cout <<"Failed to import pokemon moves.. Subdirectory " <<pmd2::GameStats::DefMvDir
                     <<" is missing from specified input directory.\n Skipping over...\n";
            }
        }

        if(m_hndlItems || bhandleall)
        {
            cout <<"\nItem Data\n"
                 <<"-----------------------------------\n";
            Poco::Path itemdatadir(frompath);
            itemdatadir.append(pmd2::GameStats::DefItemsDir);

            if( utils::isFolder(itemdatadir.toString()) )
            {
                pgamestats->ImportItems(itemdatadir.toString());
                pgamestats->WriteItems();
            }
            else
            {
                cout <<"Failed to import items data.. Subdirectory "  <<pmd2::GameStats::DefItemsDir
                     <<" is missing from specified input directory.\n Skipping over...\n";
            }
        }

        if(m_hndlStrings || bhandleall)
        {
            cout <<"\nWriting out GameStrings\n"
                 <<"---------------------------------\n"
                 <<"Writing...\n";
            pgametext->Write();
            cout <<"done!\n";
        }

        return 0;
    }
    
    int CStatsUtil::HandleExport( const std::string & topath, pmd2::GameDataLoader & gloader )
    {
        Poco::Path  outpath    = Poco::Path(topath).makeAbsolute();
        Poco::File  parentout  = outpath;
        GameStats * pgamestats = nullptr;

        bool bhandleall = !m_hndlStrings && !m_hndlItems && !m_hndlMoves && !m_hndlPkmn && !m_hndlScripts;

        if( ! parentout.exists() )
        {
            cout << "Created output directory \"" << parentout.path() <<"\"!\n";
            parentout.createDirectory();
        }

        if(m_hndlStrings || bhandleall)
        {
            cout <<"\nGame Strings\n"
                 <<"---------------------------------\n"
                 <<"Reading...";
            GameText * pgametext = gloader.InitGameText();
            if( !pgametext )
                throw std::runtime_error("CStatsUtil::HandleExport(): Couldn't load game text!");

            cout <<"Writing...\n";
            const string targetdir = Poco::Path(outpath).append(DefExportStrDirName).toString();
            CreateDirIfDoesntExist(targetdir);
            pgametext->ExportText(targetdir);
            cout <<"done!\n";
        }

        if(m_hndlScripts || bhandleall)
        {
            cout <<"\nScripts\n"
                 <<"---------------------------------\n";
            GameScripts * pgamescripts = gloader.InitScripts(pmd2::scriptprocoptions{true, true, false, m_scriptdebug, m_scriptasdir});
            if(!pgamescripts)
                throw std::runtime_error("CStatsUtil::HandleExport(): Couldn't load scripts!");

            const string targetdir = Poco::Path(outpath).append(DefExportScriptDirName).toString();
            CreateDirIfDoesntExist(targetdir);
            pgamescripts->ExportXML(targetdir);
        }

        //Load game stats for the other elements as needed
        if( m_hndlPkmn || m_hndlMoves || m_hndlItems || bhandleall )
        {
            cout <<"\nLoading Game Data..\n"
                 <<"---------------------------------\n";
           pgamestats = gloader.InitStats();
            if(!pgamestats)
                throw std::runtime_error("CStatsUtil::HandleExport(): Couldn't load game stats!");
        }

        if(m_hndlPkmn || bhandleall)
        {
            cout <<"\nPokemon Data\n"
                 <<"---------------------------------\n";

            const string targetdir = Poco::Path(outpath).append(GameStats::DefPkmnDir).toString();
            CreateDirIfDoesntExist(targetdir);
            pgamestats->ExportPkmn( targetdir );
        }

        if(m_hndlMoves || bhandleall)
        {
            cout <<"\nPokemon Move Data\n"
                 <<"-----------------------------------\n";

            const string targetdir = Poco::Path(outpath).append(GameStats::DefMvDir).toString();
            CreateDirIfDoesntExist(targetdir);
            pgamestats->ExportMoves(targetdir);
        }

        if(m_hndlItems || bhandleall)
        {
            cout <<"\nExporting Item Data\n"
                 <<"-----------------------------------\n";

            const string targetdir = Poco::Path(outpath).append(GameStats::DefItemsDir).toString();
            CreateDirIfDoesntExist(targetdir);
            pgamestats->ExportItems(targetdir);
        }

        return 0;
    }

    int CStatsUtil::DoExportSingleScript()
    {
        Poco::Path inpath(m_firstparam);
        Poco::Path outpath;
        
        if( m_outputPath.empty() )
            outpath = inpath.absolute().makeParent();
        else
            outpath = Poco::Path(m_outputPath).makeAbsolute().makeDirectory();

        //Test output path
        ConfigLoader cfgloader( m_version, m_region, m_pmd2cfg );
        CreateDirIfDoesntExist(outpath);

        eGameRegion  reg = m_region;
        eGameVersion ver = m_version;
        ScriptToXML( ::filetypes::ParseScript(inpath.toString(), 
                                              m_region, 
                                              m_version, 
                                              cfgloader.GetLanguageFilesDB(), 
                                              m_scriptdebug, 
                                              false ), 
                     cfgloader,
                     pmd2::scriptprocoptions{true, true, false, m_scriptdebug},
                     outpath.toString() );
        cout<<"\nDone!\n";
        return 0;
    }

    int CStatsUtil::DoImportSingleScript()
    {
        Poco::Path inpath(m_firstparam);
        Poco::Path outpath;
        
        if( m_outputPath.empty() )
        {
            outpath = inpath.absolute().makeParent().append(inpath.getBaseName()).setExtension(::filetypes::SSB_FileExt);
        }
        else
        {
            outpath = Poco::Path(m_outputPath).makeAbsolute();
        }

        //Test output path
        ConfigLoader cfgloader( m_version, m_region, m_pmd2cfg );

        eGameRegion  reg = m_region;
        eGameVersion ver = m_version;
        ::filetypes::WriteScript( outpath.toString(), XMLToScript( inpath.toString(), reg, ver, cfgloader),
                                 m_region, 
                                 m_version, 
                                 cfgloader.GetLanguageFilesDB() ); 
        cout<<"\nDone!\n";
        return 0;
    }


    int CStatsUtil::DoExportSingleScriptData()
    {
        Poco::Path inpath(m_firstparam);
        Poco::Path outpath;
        
        if( m_outputPath.empty() )
        {
            outpath = inpath.absolute().makeParent();
        }
        else
        {
            outpath = Poco::Path(m_outputPath).makeAbsolute().makeDirectory();
        }

        //Test output path
        ConfigLoader cfgloader( m_version, m_region, m_pmd2cfg );
        CreateDirIfDoesntExist(outpath);

        eGameRegion  reg = m_region;
        eGameVersion ver = m_version;
        ScriptDataToXML( ::filetypes::ParseScriptData(inpath.toString()), 
                         cfgloader,
                         pmd2::scriptprocoptions{true, true, false, m_scriptdebug},
                         outpath.toString() );
        cout<<"\nDone!\n";
        return 0;
    }

    int CStatsUtil::DoImportSingleScriptData()
    {
        Poco::Path inpath(m_firstparam);
        Poco::Path outpath;
        
        if( m_outputPath.empty() )
        {
            outpath = inpath.absolute().makeParent().append(inpath.getBaseName()).setExtension(::filetypes::SSB_FileExt);
        }
        else
        {
            outpath = Poco::Path(m_outputPath).makeAbsolute();
        }

        //Test output path
        ConfigLoader cfgloader( m_version, m_region, m_pmd2cfg );

        eGameRegion  reg = m_region;
        eGameVersion ver = m_version;
        ::filetypes::WriteScriptData( outpath.toString(), XMLToScriptData( inpath.toString(), reg, ver, cfgloader)); 
        cout<<"\nDone!\n";
        return 0;
    }


    int CStatsUtil::DoDumpActorList( std::string fpath, pmd2::GameDataLoader & gloader )
    {
        const pmd2::ConfigLoader & confload = MainPMD2ConfigWrapper::CfgInstance();
        //if( fpath.empty() )
        //    fpath = "actor_list.bin";

        gloader.InitAsm();
        cout<<"Dumping actor_list..\n";
        gloader.GetAsm()->WriteActorList( confload.GetGameScriptData().LivesEnt() );
        cout<<"Done with actor_list!\n";
        return 0;
    }

    int CStatsUtil::DoDumpLevelList( std::string fpath, pmd2::GameDataLoader & gloader )
#if 1
    {
        const pmd2::ConfigLoader & confload = MainPMD2ConfigWrapper::CfgInstance();
        //if( fpath.empty() )
        //    fpath = "level_list.bin";

        gloader.InitAsm();
        cout<<"Dumping level_list..\n";
        gloader.GetAsm()->WriteLevelList( confload.GetGameScriptData().LevelInfo() );
        cout<<"Done with level_list!\n";
        return 0;
    }
#else
    {
        static const size_t entrysz = 12;

        //1st pass pile up strings in order
        ::filetypes::FixedSIR0DataWrapper<std::vector<uint8_t>> sir0anddat;
        size_t                           strblksz  = 0; //Total size of all the strings one after the other, including padding!
        const size_t                     nbentries = MainPMD2ConfigWrapper::CfgInstance().GetGameScriptData().LevelInfo().size();
        const GameScriptData::lvlinf_t & lvlinf    = MainPMD2ConfigWrapper::CfgInstance().GetGameScriptData().LevelInfo();
        vector<uint8_t>                & outdata   = sir0anddat.Data();
        vector<uint32_t>                 stroffsets;
        stroffsets.reserve(nbentries);
        sir0anddat.Data().reserve((nbentries * 8) + (nbentries * entrysz));

        auto itbackins = std::back_inserter(sir0anddat.Data());

        for( size_t i = 0; i < nbentries; ++i )
        {
            const pmd2::level_info * pinf = lvlinf.FindByIndex(i);
            if(pinf)
            {
                stroffsets.push_back(outdata.size());
                std::copy( pinf->name.begin(), pinf->name.end(), itbackins );
                outdata.push_back(0);
                utils::AppendPaddingBytes( itbackins, outdata.size(), 4, 0 );
            }
        }
        utils::AppendPaddingBytes(itbackins, outdata.size(), 16, 0); //Pad the strings, because I'm a perfectionist

        //2nd pass, write the table entries
        sir0anddat.SetDataPointerOffset(outdata.size());

        for( size_t i = 0; i < nbentries; ++i )
        {
            const pmd2::level_info * pinf = lvlinf.FindByIndex(i);
            if(pinf)
            {
                //4 shorts, 1 dword
                utils::WriteIntToBytes( pinf->unk1,  itbackins );
                utils::WriteIntToBytes( pinf->unk2,  itbackins );
                utils::WriteIntToBytes( pinf->mapid, itbackins );
                utils::WriteIntToBytes( pinf->unk4,  itbackins );

                //Write pointer
                sir0anddat.pushpointer(stroffsets[i]);
            }
        }

        utils::AppendPaddingBytes(itbackins, outdata.size(), 16, 0xAA);

        //Then write out!
        ofstream out("level_list.bin",ios::out|ios::binary);
        sir0anddat.Write(std::ostreambuf_iterator<char>(out));
    }
#endif

#if 0
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
        GameStats gstats( m_outputPath, m_langconf );
        gstats.ImportPkmn( m_firstparam );
        gstats.WritePkmn( m_outputPath );
        return 0;
    }

    int CStatsUtil::DoExportPokemonData()
    {
        Poco::Path inpath(m_firstparam);
        Poco::Path outpath;
        
        if( m_outputPath.empty() )
        {
            outpath = inpath.absolute().makeParent().append(GameStats::DefPkmnDir);
        }
        else
        {
            outpath = Poco::Path(m_outputPath).makeAbsolute();
        }

        GameStats gstats( m_firstparam, m_langconf );
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

        GameStats gstats( m_outputPath, m_langconf );
        gstats.ImportItems( m_firstparam );
        gstats.WriteItems( m_outputPath );
        return 0;
    }

    int CStatsUtil::DoExportItemsData()
    {
        Poco::Path inpath(m_firstparam);
        Poco::Path outpath;
        
        if( m_outputPath.empty() )
        {
            outpath = inpath.absolute().makeParent().append(GameStats::DefItemsDir);
        }
        else
        {
            outpath = Poco::Path(m_outputPath).makeAbsolute();
        }

        GameStats gstats( m_firstparam, m_langconf );
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
        Poco::Path inpath(m_firstparam);
        Poco::Path outpath;
        
        if( m_outputPath.empty() )
            throw runtime_error("Output path is empty!");
        if( !utils::isFolder( m_outputPath ) )
            throw runtime_error("Output path doesn't exist, or isn't a directory!");

        outpath = Poco::Path(m_outputPath);

        GameStats gstats ( m_outputPath, m_langconf );
        gstats.ImportMoves( m_firstparam );
        gstats.WriteMoves ( m_outputPath );

        return 0;
    }

    int CStatsUtil::DoExportMovesData()
    {
        Poco::Path inpath(m_firstparam);
        Poco::Path outpath;
        
        if( m_outputPath.empty() )
        {
            outpath = inpath.absolute().makeParent().append(GameStats::DefMvDir);
        }
        else
        {
            outpath = Poco::Path(m_outputPath).makeAbsolute();
        }

        GameStats gstats( m_firstparam, m_langconf );
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
        Poco::Path inpath(m_firstparam);
        Poco::Path outpath;
        
        if( m_outputPath.empty() )
            throw runtime_error("Output path unspecified!");

        if( utils::isFolder( m_outputPath ) )
        {
            outpath = Poco::Path(m_outputPath).makeAbsolute().makeDirectory();
            GameStats gstats( outpath.toString(), m_langconf );
            gstats.AnalyzeGameDir();
            gstats.ImportStrings( m_firstparam );
            gstats.WriteStrings();
        }
        else
        {
            outpath = Poco::Path(m_outputPath).makeAbsolute();

            if( ! m_flocalestr.empty() )
            {
                auto myloc = std::locale( m_flocalestr );
                pmd2::filetypes::WriteTextStrFile( outpath.toString(), utils::io::ReadTextFileLineByLine( m_firstparam, myloc ), myloc );
            }
            else
            {
                pmd2::filetypes::WriteTextStrFile( outpath.toString(), utils::io::ReadTextFileLineByLine(m_firstparam) );
            }
        }
        return 0;
    }

    int CStatsUtil::DoExportGameStrings()
    {
        Poco::Path inpath(m_firstparam);
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
            GameStats mystats( m_firstparam, m_langconf );
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
        Poco::Path inpath(m_firstparam);
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
        Poco::Path inpath(m_firstparam);
        Poco::Path outpath;
        
        if( m_outputPath.empty() )
            throw runtime_error("Output path is empty!");
            
        if( !utils::isFolder( m_outputPath ) )
            throw runtime_error("Output path doesn't exist, or isn't a directory!");

        outpath = Poco::Path(m_outputPath);

        GameStats gstats ( m_outputPath, m_langconf );
        gstats.ImportAll( m_firstparam );
        gstats.Write();
        return 0;
    }

    int CStatsUtil::DoExportAll()
    {
        Poco::Path inpath(m_firstparam);
        Poco::Path outpath;
        
        if( m_outputPath.empty() )
        {
            outpath = inpath.absolute().makeParent().append(DefExportAllDir);
        }
        else
        {
            outpath = Poco::Path(m_outputPath).makeAbsolute();
        }

        GameStats gstats( m_firstparam, m_langconf );
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

#endif

    int CStatsUtil::DoExportGameScripts()
    {
        ////Validate output + input paths
        //Poco::Path inpath(m_firstparam);
        //Poco::Path outpath;
        //
        //if( m_outputPath.empty() )
        //    outpath = inpath.absolute().makeParent().append(DefExportScriptsDir);
        //else
        //    outpath = Poco::Path(m_outputPath).makeAbsolute();

        //Poco::File fTestOut = outpath;
        //if( ! fTestOut.exists() )
        //{
        //    cout << "Created output directory \"" << fTestOut.path() <<"\"!\n";
        //    fTestOut.createDirectory();
        //}
        //else if( ! fTestOut.isDirectory() )
        //    throw runtime_error("CStatsUtil::DoExportGameScripts(): Output path is not a directory!");


        ////Setup the script handler
        //auto locandvers = DetermineGameVersionAndLocale( inpath.absolute().toString() );

        //if( locandvers.first == eGameVersion::Invalid || locandvers.second == eGameRegion::Invalid  )
        //    throw std::runtime_error( "CStatsUtil::DoExportGameScripts(): Invalid game version or locale!" );

        //GameScripts scripts( inpath.absolute().toString(), locandvers.second, locandvers.first );
        //scripts.Load();

        //Convert to XML
        //scripts.ExportScriptsToXML( outpath.toString() );
        assert(false);
        return 0;
    }

    int CStatsUtil::DoImportGameScripts()
    {
        //Validate output + input paths
        Poco::Path inpath(m_firstparam);
        Poco::Path outpath;

        if( m_outputPath.empty() )
            throw runtime_error("CStatsUtil::DoImportGameScripts() : Output path is empty!");
            
        if( !utils::isFolder( m_outputPath ) )
            throw runtime_error("CStatsUtil::DoImportGameScripts() : Output path doesn't exist, or isn't a directory!");

        outpath = Poco::Path(m_outputPath);

        //Setup the script handler
        //GameScripts scripts( outpath.absolute().toString() );

        //Import from XML
        //scripts.ImportScriptsFromXML( inpath.absolute().toString() );
        assert(false);
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

        return returnval;
    }
};

//=================================================================================================
// Main Function
//=================================================================================================

//#include <ppmdu/fmts/integer_encoding.hpp>

//int Sir0Decoder( int argc, const char *argv[] )
//{
//    using namespace statsutil;
//    {
//        utils::MrChronometer chronoTester("Total time elapsed");
//        cout<<"Decoding..\n";
//        vector<uint8_t> encodedptrs = ReadFileToByteVector( argv[1] );
//        auto result = utils::DecodeIntegers<uint32_t>( encodedptrs.begin(), encodedptrs.end() );
//        cout<<"Done! Decoded " <<result.size() <<" values.\n";
//
//        uint32_t accumulator = 0;
//        for( const auto & val : result )
//        {
//            accumulator += val;
//            cout<<"0x"<<hex<<accumulator<<"\n";
//        }
//        cout<<dec<<"Done!\n";
//    }
//
//    return 0;
//}
//
//int IntegerDecoder( int argc, const char *argv[] )
//{
//    using namespace statsutil;
//    {
//        utils::MrChronometer chronoTester("Total time elapsed");
//        vector<uint8_t> encodedints = ReadFileToByteVector( argv[1] );
//        auto itRead = encodedints.begin();
//        auto itEnd  = encodedints.end();
//
//        auto itfindend = encodedints.begin();
//        //Exclude padding if neccessary
//        for( ; itfindend != itEnd; ++itfindend )
//        {
//            if( (*itfindend) == 0xAA && ( std::distance( itfindend, itEnd ) <= 15u ) && std::all_of( itfindend, itEnd, [](uint8_t val){return val == 0xAA;} ) )
//                break;
//        }
//        itEnd = itfindend;
//
//
//        unsigned int cnt = 0;
//        cout<<"Decoding integer lists..\n";
//        while( itRead != itEnd )
//        {
//            cout<<"List #" <<dec <<cnt <<"\n";
//        
//            vector<uint32_t> result; 
//            itRead = utils::DecodeIntegers( itRead, itEnd, back_inserter(result) );
//            cout <<"Has " <<result.size() <<" values.\n";
//
//            for( const auto & val : result )
//            {
//                cout <<"0x" <<uppercase <<hex <<val <<nouppercase <<"\n";
//            }
//            ++cnt;
//        }
//        cout<<dec<<"Done!\n";
//    }
//
//    return 0;
//}




//#TODO: Move the main function somewhere else !
int main( int argc, const char * argv[] )
{
    using namespace statsutil;
    try
    {
        CStatsUtil & application = CStatsUtil::GetInstance();
        return application.Main(argc,argv);
    }
    catch( const exception & e )
    {
        cout<< "<!>-ERROR:" <<e.what()<<"\n"
            << "If you get this particular error output, it means an exception got through, and the programmer should be notified!\n";
    }

#ifdef _DEBUG
        utils::PortablePause();
#endif

    return 0;
}
