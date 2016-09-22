#ifndef STATS_UTIL_HPP
#define STATS_UTIL_HPP
#include <utils/cmdline_util.hpp>
#include <ppmdu/pmd2/game_stats.hpp>
#include <ppmdu/pmd2/pmd2_gameloader.hpp>


namespace statsutil
{
    class CStatsUtil : public utils::cmdl::CommandLineUtility
    {
    public:
        static CStatsUtil & GetInstance();

        // -- Overrides --
        //Those return their implementation specific arguments, options, and extra parameter lists.
        const std::vector<utils::cmdl::argumentparsing_t> & getArgumentsList()const;
        const std::vector<utils::cmdl::optionparsing_t>   & getOptionsList()const;
        const utils::cmdl::argumentparsing_t              * getExtraArg()const; //Returns nullptr if there is no extra arg. Extra args are args preceeded by a "+" character, usually used for handling files in batch !
       
        //For writing the title and readme!
        const std::string & getTitle()const;            //Name/Title of the program to put in the title!
        const std::string & getExeName()const;          //Name of the executable file!
        const std::string & getVersionString()const;    //Version number
        const std::string & getShortDescription()const; //Short description of what the program does for the header+title
        const std::string & getLongDescription()const;  //Long description of how the program works
        const std::string & getMiscSectionText()const;  //Text for copyrights, credits, thanks, misc..

        //Main method
        int Main(int argc, const char * argv[]);

    private:
        CStatsUtil();
        ~CStatsUtil();

        //Parse Arguments
        bool ParseInputPath  ( const std::string              & path );
        bool ParseOutputPath ( const std::string              & path );

        bool ShouldParseOutputPath( const std::vector<std::vector<std::string>> & optdata, 
                                    const std::deque<std::string>               & priorparams,
                                    size_t                                        nblefttoparse );

        //Parse Options
        bool ParseOptionPk         ( const std::vector<std::string> & optdata );
        bool ParseOptionMvD        ( const std::vector<std::string> & optdata );
        bool ParseOptionItems      ( const std::vector<std::string> & optdata );
        bool ParseOptionStrings    ( const std::vector<std::string> & optdata );
        bool ParseOptionForceImport( const std::vector<std::string> & optdata );
        bool ParseOptionForceExport( const std::vector<std::string> & optdata );
        bool ParseOptionLocaleStr  ( const std::vector<std::string> & optdata );
        bool ParseOptionLog        ( const std::vector<std::string> & optdata );
        bool ParseOptionScripts    ( const std::vector<std::string> & optdata );
        bool ParseOptionConfig     ( const std::vector<std::string> & optdata );
        bool ParseOptionRomRoot    ( const std::vector<std::string> & optdata );
        bool ParseOptionThreads    ( const std::vector<std::string> & optdata );
        bool ParseOptionEscapeAsXML( const std::vector<std::string> & optdata );
        bool ParseOptionScriptEnableDebugInstr( const std::vector<std::string> & optdata );
        bool ParseOptionDumpLvlList( const std::vector<std::string> & optdata );
        bool ParseOptionDumpActorList( const std::vector<std::string> & optdata );
        bool ParseOptionScriptAsDir(const std::vector<std::string> & optdata ); 

        //Execution
        void DetermineOperation();
        bool DetermineXMLOps( const std::string & filepath );

        int  Execute           ();
        int  GatherArgs        ( int argc, const char * argv[] );


        void SetupCFGPath(const std::string & cfgrelpath); //Process the path to the CFG/data xml file. Should be run after the command line param were parsed!


        //Exec methods
        //int ExportPokeStatsGrowth();
        //int ImportPokeStatsGrowth();
        int DoImportGameData();
        int DoExportGameData();
        int DoImportPokemonData();
        int DoExportPokemonData();
        int DoImportItemsData();
        int DoExportItemsData();
        int DoImportMovesData();
        int DoExportMovesData();
        int DoImportGameStrings();
        int DoExportGameStrings();
        int DoExportGameStringsFromFile(); //For exporting the game strings from the text_*.str file directly
        int DoExportGameScripts();
        int DoImportGameScripts();

        int DoExportSingleScript();
        int DoImportSingleScript();
        int DoExportSingleScriptData();
        int DoImportSingleScriptData();

        int DoImportAll();
        int DoExportAll();

        int DoDumpLevelList( std::string fpath, pmd2::GameDataLoader & gloader );
        int DoDumpActorList( std::string fpath, pmd2::GameDataLoader & gloader );

        int HandleImport( const std::string & frompath, pmd2::GameDataLoader & gloader );
        int HandleExport( const std::string & topath,   pmd2::GameDataLoader & gloader );

        //Validation:
        void ValidateRomRoot()const;

        //Constants
        static const std::string                                 Exe_Name;
        static const std::string                                 Title;
        static const std::string                                 Version;
        static const std::string                                 Short_Description;
        static const std::string                                 Long_Description;
        static const std::string                                 Misc_Text;
        static const std::vector<utils::cmdl::argumentparsing_t> Arguments_List;
        static const std::vector<utils::cmdl::optionparsing_t>   Options_List;

        //Default filenames names
        static const std::string                                 DefExportStrName;
        static const std::string                                 DefExportStrDirName;
        static const std::string                                 DefExportScriptDirName;
        //static const std::string                                 DefExportPkmnOutDir;
        //static const std::string                                 DefExportMvDir;
        //static const std::string                                 DefExportItemsDir;
        static const std::string                                 DefExportAllDir;
//        static const std::string                                 DefExportScriptsDir;

        //static const std::string                                 DefLangConfFile;
        

        enum struct eOpForce
        {
            None,
            Import,
            Export,
        };

        enum struct eOpMode
        {
            Invalid,

            ImportPokemonData,
            ExportPokemonData,

            ImportItemsData,
            ExportItemsData,

            ImportMovesData,
            ExportMovesData,

            ImportGameStrings,
            ExportGameStrings,
            ExportGameStringsFromFile,

            ImportGameScripts,
            ExportGameScripts,
            
            //New
            ImportSingleScript,
            ExportSingleScript,
            ImportSingleScriptData,
            ExportSingleScriptData,

            DumpLevelList,
            DumpActorList,

            ImportAll,
            ExportAll,
        };

        //Variables
        std::string m_firstparam;      //This is the input path that was parsed 
        std::string m_outputPath;     //This is the output path that was parsed

        eOpMode     m_operationMode;  //This holds what the program should do
        //std::string m_langconf;       //The path to the language configuration file!
        std::string m_pmd2cfg;        //The path to the configuration file!
        std::string m_flocalestr;     //The forced locale string
        std::string m_romrootdir;     //The extracting rom's root directory
        std::string m_applicationdir; //Directory where the executable is


        bool        m_forcedLocale;   //Whether the -locale command line option was used at all.
        bool        m_hndlStrings;    //If we need to handle only a game string related OP, this is true!
        bool        m_hndlItems;      //If we handle only items
        bool        m_hndlMoves;      //If we handle only moves
        bool        m_hndlPkmn;       //If we handle only Pokemon
        bool        m_hndlScripts;    //If we handle only Scripts
        eOpForce    m_force;          // 
        bool        m_shouldlog;      
        bool        m_escxml;         //Force escape sequences to be standard XML instead of C
        bool        m_scriptdebug;
        bool        m_dumplvllist;
        bool        m_dumpactorlist;
        bool        m_scriptasdir;  //Whether scripts are exported/imported as directories
        
        pmd2::eGameRegion  m_region;
        pmd2::eGameVersion m_version;

        utils::cmdl::RAIIClogRedirect m_redirectClog;
        //eOutFormat  m_outputFormat;   //
        //bool        m_bExpSingleFile; //Whether the result will be Exported to a single file, when possible
    };
};

#endif