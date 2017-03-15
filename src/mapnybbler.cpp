#include "mapnybbler.hpp"
//ppmdu
#include <utils/utility.hpp>
#include <utils/cmdline_util.hpp>
#include <utils/cmdline_util_runner.hpp>
#include <types/content_type_analyser.hpp>
#include <utils/library_wide.hpp>
#include <ppmdu/pmd2/pmd2_gameloader.hpp>
#include <ppmdu/pmd2/pmd2_xml_sniffer.hpp>
#include <ppmdu/pmd2/pmd2_asm.hpp>
#include <utils/poco_wrapper.hpp>
#include <utils/whereami_wrapper.hpp>
//stdlib
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
//Poco
#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/Exception.h>
#include <Poco/Process.h>

using namespace ::utils::cmdl;
using namespace ::utils::io;
using namespace ::std;

/*
                --- STUFF TO DO!! ---
    - Make the script data file separately loaded from the configuration file!
    - Add check to verify and determine what mods are installed on a modded PMD2 game. Add safeguard so if 
      it encounters another tool's modded code that is prefixed by something else than PATCH, ignore, and error out!
    - Make the script data file editable, and generatable from several sources.
        * Unmodiffied games
        * Content of loaded scripts/levels
        * From the files added by the modification to the game.
    - Add support for dumping to XML, and importing for edititng the following hard-coded data to-and-from modded and unmodded games:
        * Actors
        * Objects
        * Common Routines
    - Add support for dumping and importing for editing hard-coded level data to-and-from modded and unmodded games. 
      + Integrate with level export!
    - Work out whether any files are shared inside the bg_list file, and whether any should be. And if they should be shared, this will complicate how everything works.
    







*/


//=================================================================================================
//  CMapNybbler
//=================================================================================================

//------------------------------------------------
//  Constants
//------------------------------------------------
    const string CMapNybbler::Exe_Name            = "ppmd_mapnybbler.exe";
    const string CMapNybbler::Title               = "PMD2 Map Utility";
#ifdef _DEBUG
    const string CMapNybbler::Version             = PPMD_MAPNYBBLER_VER" debug";
#else
    const string CMapNybbler::Version             = PPMD_MAPNYBBLER_VER;
#endif
    const string CMapNybbler::Short_Description   = "A utility to export and import map and tileset data from the PMD2 games.";
    const string CMapNybbler::Long_Description    = 
        "#TODO";
    const string CMapNybbler::Misc_Text           = 
        "Named in honour of Baz, the awesome Poochyena of doom ! :D\n"
        "My tools in binary form are basically Creative Commons 0.\n"
        "Free to re-use in any ways you may want to!\n"
        "No crappyrights, all wrongs reversed! :3";


//------------------------------------------------
//  Arguments Info
//------------------------------------------------
    /*
        Data for the automatic argument parser to work with.
    */
    const std::vector<argumentparsing_t> CMapNybbler::Arguments_List
    {{
        //Param argument
        { 
            0,      //first arg
            true,  //false == mandatory
            true,   //guaranteed to appear in order
            "path", 
            "Path to the file/directory to export, or the directory to assemble.",
#ifdef WIN32
            "\"c:/pmd_romdata/data.bin\"",
#elif __linux__
            "\"/pmd_romdata/data.bin\"",
#endif
            std::bind( &CMapNybbler::ParseInputPath,       &CMapNybbler::GetInstance(), placeholders::_1 ),
            std::bind( &CMapNybbler::ShouldParseInputPath, &CMapNybbler::GetInstance(), placeholders::_1, placeholders::_2, placeholders::_3 ),
        },
    }};


//------------------------------------------------
//  Options Info
//------------------------------------------------
    /*
        Information on all the switches / options to allow the automated parser 
        to parse them.
    */
    const vector<optionparsing_t> CMapNybbler::Options_List
    {{
    //=== Exec Parameters ===

        //Specify the root of the extracted rom directory to work with
        {
            "romroot",
            1,
            "Specify the root of the extracted rom directory to work with! The directory must contain "
            "both a \"data\" and \"overlay\" directory, and at least a \"arm9.bin\" file! "
            "The \"data\" directory must contain the rom's files and directories!"
            "The \"overlay\" directory must contain the rom's many \"overlay_00xx.bin\" files!",
            "-romroot \"path/to/extracted/rom/root/directory\"",
            std::bind( &CMapNybbler::ParseOptionRomRoot, &GetInstance(), placeholders::_1 ),
        },

        //Set path to PMD2 Config file
        {
            "cfg",
            1,
            "Set a non-default path to the pmd2data.xml file.",
            "-cfg \"path/to/pmd2/config/data/file\"",
            std::bind( &CMapNybbler::ParseOptionConfig, &GetInstance(), placeholders::_1 ),
        },

        //Force Import
        {
            "i",
            0,
            "Specifying this will force import!",
            "-i",
            std::bind( &CMapNybbler::ParseOptionForceImport, &GetInstance(), placeholders::_1 ),
        },

        //Force Export
        {
            "e",
            0,
            "Specifying this will force export!",
            "-e",
            std::bind( &CMapNybbler::ParseOptionForceExport, &GetInstance(), placeholders::_1 ),
        },

    //=== Exec Options ===

        //Assemble a level_list.bin
        {
            "asmll",
            0,
            "Assemble a level_list.bin file from the current configuration. After executing the main import operation, if there is one."
            "Will be placed into the target rom root!",
            "-asmll",
            std::bind( &CMapNybbler::ParseOptionAssembleLvlList, &GetInstance(), placeholders::_1 ),
        },

        //Assemble BG List
        {
            "asmbgl",
            0,
            "Assemble a bg_list.dat file from the current input data after executing the main import operation, if there is one."
            "Will be placed into the target rom root!",
            "-asmbgl",
            std::bind( &CMapNybbler::ParseOptionAssembleBgList, &GetInstance(), placeholders::_1 ),
        },

        //Apply patch from XML patch OP info
        {
            "asmpop",
            1,
            "Applies the asm patch operations listed in the xml file onto the available executable in the game files!",
            "-asmpop \"Path/ToXML/Patch/Info/File.xml\"",
            std::bind( &CMapNybbler::ParseOptionAssemblePatchOp, &GetInstance(), placeholders::_1 ),
        },

    //=== Misc Options ===

        //Redirect clog to file
        {
            "log",
            1,
            "This option writes a log to the file specified as parameter.",
            "-log \"logfilename.txt\"",
            std::bind( &CMapNybbler::ParseOptionLog, &GetInstance(), placeholders::_1 ),
        },

        //Verbose output
        {
            "v",
            0,
            "This enables the writing of a lot more info to the logfile!",
            "-v",
            std::bind( &CMapNybbler::ParseOptionVerbose, &GetInstance(), placeholders::_1 ),
        },
    }};

//------------------------------------------------
// Misc Methods
//------------------------------------------------

    CMapNybbler & CMapNybbler::GetInstance()
    {
        static CMapNybbler s_util;
        return s_util;
    }

    CMapNybbler::CMapNybbler()
        :CommandLineUtility()
    {
        //utils::cmdl::InstanceRunner(UtilTypeWrapper<CMapNybbler>()); //Set the instance to run
        m_opmode = eOpMode::Invalid;
        m_cfgpath = ::pmd2::DefConfigFileName;

        m_applicationdir = utils::GetPathExeDirectory();
        utils::LibWide().StringValue(utils::lwData::eBasicValues::ProgramExeDir) = m_applicationdir;
        utils::LibWide().StringValue(utils::lwData::eBasicValues::ProgramLogDir) = utils::getCWD();
    }

    const vector<argumentparsing_t> & CMapNybbler::getArgumentsList   ()const { return Arguments_List;    }
    const vector<optionparsing_t>   & CMapNybbler::getOptionsList     ()const { return Options_List;      }
    const argumentparsing_t         * CMapNybbler::getExtraArg        ()const { return nullptr;           } //No extra args
    const string                    & CMapNybbler::getTitle           ()const { return Title;             }
    const string                    & CMapNybbler::getExeName         ()const { return Exe_Name;          }
    const string                    & CMapNybbler::getVersionString   ()const { return Version;           }
    const string                    & CMapNybbler::getShortDescription()const { return Short_Description; }
    const string                    & CMapNybbler::getLongDescription ()const { return Long_Description;  }
    const string                    & CMapNybbler::getMiscSectionText ()const { return Misc_Text;         }


//------------------------------------------------
//  Parse Args
//------------------------------------------------
    bool CMapNybbler::ParseInputPath( const string & path )
    {
        Poco::Path inputfile(path);

        //check if path exists
        if(( inputfile.isFile() || inputfile.isDirectory() ) )
        {
            m_firstarg = path;
            return true;
        }

        return false;
    }

    /*
        When we have some specific options specified, we don't need the input path argument!
    */
    bool CMapNybbler::ShouldParseInputPath( const vector<vector<string>> & /*options*/,  
                                           const deque<string>          & /*priorparams*/, 
                                           size_t                         /*nblefttoparse*/ )
    {
        //auto itfoundinswitch = std::find_if( options.begin(), 
        //                                     options.end(), 
        //                                     [](const vector<string>& curopt)
        //                                     { 
        //                                        return ( curopt.front() == OPTION_SwdlPathSym ) || 
        //                                               ( curopt.front() == OPTION_SmdlPathSym ) ||
        //                                               ( curopt.front() == OPTION_BgmCntPath )  ||
        //                                               ( curopt.front() == OPTION_BgmBlobPath ) ||
        //                                               ( curopt.front() == OPTION_MkCvInfo ); 

        //                                     } );

        //If we have an input path option, we do not need the input path parameter!
        //return ( itfoundinswitch == options.end() );
        return true;
    }


//------------------------------------------------
//  Parse Options
//------------------------------------------------
    bool CMapNybbler::ParseOptionLog( const std::vector<std::string> & optdata )
    {
        Poco::Path outpath(optdata[1]);
        if( outpath.isFile() )
        {
            Poco::File OutputDir( outpath.parent().makeAbsolute() );
            if( !OutputDir.exists() )
            {
                if( !OutputDir.createDirectory() )
                    throw runtime_error( "Couldn't create output directory for log file!");
            }
                

            m_redirectClog.Redirect(optdata[1]);
            utils::LibWide().isLogOn(true);
            return true;
        }
        else
        {
            cerr << "<!>- ERROR: Invalid path to log file specified! Path is not a file!\n";
            return false;
        }
    }

    bool CMapNybbler::ParseOptionVerbose( const std::vector<std::string> & optdata )
    {
        utils::LibWide().setVerbose(true);
        return true;
    }

    bool CMapNybbler::ParseOptionRomRoot(const std::vector<std::string>& optdata)
    {
        if( utils::isFolder( optdata[1] ) )
        {
            m_romroot = optdata[1];
            cout << "<!>- Set \"" <<optdata[1]  <<"\" as ROM root directory!\n";
        }
        else
            throw runtime_error("Path to ROM root directory does not exists, or is inaccessible!");
        return true;
    }

    bool CMapNybbler::ParseOptionConfig(const std::vector<std::string>& optdata)
    {
        //Don't check if it exists because its compared to the app path and checked already inside setupcfg path!
        SetupCFGPath(optdata[1]);
        cout << "<!>- Set \"" <<m_cfgpath  <<"\" as path to pmd2data file!\n";
        return true;
    }

    bool CMapNybbler::ParseOptionForceImport(const std::vector<std::string>& optdata)
    {
        m_opmode = eOpMode::Import;
        return true;
    }

    bool CMapNybbler::ParseOptionForceExport(const std::vector<std::string>& optdata)
    {
        m_opmode = eOpMode::Export;
        return true;
    }

    bool CMapNybbler::ParseOptionAssembleLvlList(const std::vector<std::string>& optdata)
    {
        //Skip if we already added this task!
        for( const auto & task : m_extasks)
        {
            if( task.tskty == eTasks::AsmLvlList )
                return true;
        }
        m_extasks.push_back(ExtraTasks{eTasks::AsmLvlList});
        return true;
    }

    bool CMapNybbler::ParseOptionAssembleBgList(const std::vector<std::string>& optdata)
    {
        string ofpath = Poco::Path(optdata[1]).resolve("").toString();


        //Skip if we already added this task!
        for( auto & task : m_extasks)
        {
            if( task.tskty == eTasks::AsmActList )
            {
                task.args.front() = optdata[1];
                return true;
            }
        }
        m_extasks.push_back( ExtraTasks{eTasks::AsmActList,{optdata[1]}} );
        return true;
    }

    bool CMapNybbler::ParseOptionAssemblePatchOp ( const std::vector<std::string> & optdata )
    {
        string fpath = optdata[1];
        if( utils::isFile(fpath) )
        {
            m_extasks.push_back( ExtraTasks{eTasks::AsmPatchOps,{fpath}} );
            return true;
        }
        else 
            throw runtime_error("CMapNybbler::ParseOptionAssemblePatchOp(): Path to patch operation file invalid!");
        return false;
    }

    void CMapNybbler::SetupCFGPath(const std::string & cfgrelpath)
    {
        if(m_applicationdir.empty())
        {
            assert(false);
            throw std::runtime_error("CMapNybbler::SetupCFGPath(): Application dir was empty??");
        }

        Poco::Path cfgpath(m_applicationdir);
        cfgpath.makeDirectory();
        cfgpath.resolve(cfgrelpath);  //If cfg path is relative, it gets appended. If absolute, it replaces the app dir path!

        if( utils::isFile(cfgpath.toString()) )
            m_cfgpath = cfgpath.toString();
        else
            throw std::logic_error("CMapNybbler::SetupCFGPath(): Path to pmd2data xml file is non-existant! " + cfgpath.toString());
    }

    //
    void CMapNybbler::ValidateRomRoot()const
    {
        if( m_romroot.empty() )
            throw runtime_error("CMapNybbler::ValidateRomRoot(): No extracted ROM root directory was specified using the \"-romroot\" option!");
        if( !utils::isFolder( m_romroot ) )
            throw runtime_error("CMapNybbler::ValidateRomRoot(): Extracted ROM root directory path doesn't exist, or isn't a directory!");
    }


// ------------------------------------------------
//  Execution
// ------------------------------------------------

    /*
    */
    void CMapNybbler::DetermineOperation()
    {
        //Here we decide what the program will do, and we set the list of things to be processed!
        switch(m_opmode)
        {
            case eOpMode::Import:
            {
                if( utils::isFile(m_firstarg)  )
                {
                    m_processlist.push_back(m_firstarg);
                }
                else if(utils::isFolder(m_firstarg))
                {
                    //Check if we have the expected named subfolders, and they contain the correct XML files
                    assert(false);
                    //pmd2::GetRootNodeFromXML();
                }
                else
                    assert(false);
                break;
            }
            case eOpMode::Export:
            {
                //if( utils::isFile(m_firstarg)  )
                //{
                //    m_processlist.push_back(m_firstarg);
                //}
                //else if(utils::isFolder(m_firstarg))
                //{
                //    //!TODO: Check if MAP_BG dir contains a bg_list.dat and at least one set of bpc,bpl,bma.
                //    //First arg is export path!
                //}
                //else
                //    assert(false);
                break;
            }
        };
    }

    /*
    */
    int CMapNybbler::Execute()
    {
        int returnval = -1;
        utils::MrChronometer chronoexecuter("Total time elapsed");
        try
        {
            ValidateRomRoot();
            pmd2::GameDataLoader gloader( m_romroot, m_cfgpath );
            gloader.AnalyseGame(); //Load config, and etc..
            cout <<"\n";
            switch(m_opmode)
            {
                case eOpMode::Import:
                {
                    cout    <<"================================================\n"
                            <<"                     Import                     \n"
                            <<"================================================\n\n"
                        ;
                    returnval = HandleImport(m_firstarg, gloader);
                    break;
                }
                case eOpMode::Export:
                {
                    cout    <<"================================================\n"
                            <<"                     Export                     \n"
                            <<"================================================\n\n"
                        ;
                    returnval = HandleExport(m_firstarg, gloader);
                    break;
                }
            };

            //Handle Extra tasks
            HandleExtraTasks(gloader);
        }
        catch( const Poco::Exception & e )
        {
            cerr <<"\n" << "<!>- POCO Exception - " <<e.name() <<"(" <<e.code() <<") : " << e.message() <<"\n" <<endl;
            if( utils::LibWide().isLogOn() )
                clog <<"\n" << "<!>- POCO Exception - " <<e.name() <<"(" <<e.code() <<") : " << e.message() <<"\n" <<endl;
            assert(false);
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
            assert(false);
        }
        return returnval;
    }

    /*
    */
    int CMapNybbler::GatherArgs(int argc, const char * argv[])
    {
        int returnval = 0;
        //Parse arguments and options
        try
        {
            if( !SetArguments(argc,argv) ) //This parses the command line!!!!
                return -3;
            //Now that the command line is parsed, do stuff with it
            if(utils::LibWide().isLogOn())
            {
                utils::LibWide().Logger(new logging::SingleOutMTLogger);
                utils::LibWide().Logger() << "Logger initiated!\n";
            }
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


//
//
//

    void CMapNybbler::RunLvlListAssembly( const ExtraTasks & task, pmd2::GameDataLoader & gloader )
    {
        pmd2::ConfigLoader & cfg = pmd2::MainPMD2ConfigWrapper::CfgInstance();
        pmd2::PMD2_ASM     * pasm = gloader.GetAsm();

        //!#TODO: Add option to use default loaded entries
        if(true)
            cfg.GetGameScriptData().LevelInfo() = pasm->LoadLevelList();

        gloader.LoadAsm();
        cout<<"Dumping level_list..\n";
        pasm->WriteLevelList(cfg.GetGameScriptData().LevelInfo());
        cout<<"Done with level_list!\n";
    }

    void CMapNybbler::RunActorListAssembly( const ExtraTasks & task, pmd2::GameDataLoader & gloader )
    {
        pmd2::ConfigLoader & cfg  = pmd2::MainPMD2ConfigWrapper::CfgInstance();
        pmd2::PMD2_ASM     * pasm = gloader.GetAsm();


        //!#TODO: Add option to use default loaded entries
        if(true)
            cfg.GetGameScriptData().LivesEnt() = pasm->LoadActorList();

        gloader.LoadAsm();
        cout<<"Dumping actor_list..\n";
        pasm->WriteActorList(cfg.GetGameScriptData().LivesEnt());
        cout<<"Done with actor_list!\n";
    }

    void CMapNybbler::RunPatchOp(const ExtraTasks & task, pmd2::GameDataLoader & gloader)
    {
        assert(false);
        
        // == 0a. Parse arguments ==
        {
        }

        // == 0b. Read patch data XML, if needed ==
        {
        }

        // == 1a. Check if the binary is already patched. ==
        {
        }

        // == 1b. Check the patch version if the binary is already patched ==
        {
        }

        // == 2. Generate custom asm if needed ==
        {
        }

        // == 3. Patch the binaries ==
        {
        }
    }

    void CMapNybbler::HandleExtraTasks(pmd2::GameDataLoader & gloader)
    {
        for( const ExtraTasks & task : m_extasks )
        {
            switch( task.tskty )
            {
                case eTasks::AsmLvlList:
                {
                    RunLvlListAssembly(task, gloader);
                    break;
                }
                case eTasks::AsmActList:
                {
                    RunActorListAssembly(task, gloader);
                    break;
                }
                case eTasks::AsmPatchOps:
                {
                    RunPatchOp(task, gloader);
                    break;
                }
                case eTasks::Invalid:
                default:
                {
                    assert(false);
                    throw std::logic_error("CMapNybbler::HandleExtraTasks(): Got an invalid task! Report to programmer!");
                }
            };
        }
    }

    int CMapNybbler::HandleImport(const std::string & inpath, pmd2::GameDataLoader & gloader)
    {
        assert(false);
        return 0;
    }

    int CMapNybbler::HandleExport(const std::string & inpath, pmd2::GameDataLoader & gloader)
    {
        //Export all levelshttps://discordapp.com/channels/@me/289451511191175168

        //
        pmd2::GameLevels * plvl = gloader.LoadLevels(pmd2::Default_Level_Options);
        utils::DoCreateDirectory(inpath);
        plvl->ExportAllLevels(inpath);
        return 0;
    }

//--------------------------------------------
//  Main Methods
//--------------------------------------------
    int CMapNybbler::Main(int argc, const char * argv[])
    {
        int returnval = -1;
        try
        {
            PrintTitle();

            //Handle arguments
            returnval = GatherArgs( argc, argv );
            if( returnval != 0 )
                return returnval;
        
            //Execute the utility
            returnval = Execute();
        }
        catch( const exception & e )
        {
            cout<< "<!>-ERROR:" <<e.what()<<"\n"
                << "If you get this particular error output, it means an exception got through, and the programmer should be notified!\n";
        }
        return returnval;
    }



//int main( int argc, const char * argv[] ) {return CMapNybbler::GetInstance().Main(argc,argv);}
CMDLINE_UTILITY_MAIN(CMapNybbler);