#ifndef MAP_NYBBLER_HPP
#define MAP_NYBBLER_HPP
/*
mapnybbler.hpp
2016/11/11
psycommando@gmail.com
Description: Main code for the map editor tool !

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P
*/
#include <ppmdu/pmd2/pmd2.hpp>
#include <ppmdu/pmd2/pmd2_gameloader.hpp>
#include <utils/cmdline_util.hpp>
#include <string>
#include <vector>


    class CMapNybbler : public utils::cmdl::CommandLineUtility
    {
    private:
        //Constants
        static const std::string                                 Exe_Name;
        static const std::string                                 Title;
        static const std::string                                 Version;
        static const std::string                                 Short_Description;
        static const std::string                                 Long_Description;
        static const std::string                                 Misc_Text;
        static const std::vector<utils::cmdl::argumentparsing_t> Arguments_List;
        static const std::vector<utils::cmdl::optionparsing_t>   Options_List;

        enum struct eOpMode 
        {
            Invalid,
            Export,
            Import,
        };
        enum struct eTasks
        {
            Invalid,
            AsmLvlList,
            AsmActList,
            AsmPatchOps,
        };

        struct ExtraTasks
        {
            eTasks                   tskty;
            std::vector<std::string> args;
        };

    public:
        static CMapNybbler & GetInstance();

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
        CMapNybbler();

        //Parse Arguments
        bool ParseInputPath  ( const std::string & path );

        bool ShouldParseInputPath ( const std::vector<std::vector<std::string>> & options, 
                                    const std::deque<std::string>               & priorparams, 
                                    size_t                                        nblefttoparse );

        //Parse Options
        bool ParseOptionLog             ( const std::vector<std::string> & optdata ); //Redirects clog to the file specified
        bool ParseOptionVerbose         ( const std::vector<std::string> & optdata ); //Write more info to the log file!

        bool ParseOptionRomRoot         ( const std::vector<std::string> & optdata );
        bool ParseOptionConfig          ( const std::vector<std::string> & optdata );
        bool ParseOptionForceImport     ( const std::vector<std::string> & optdata );
        bool ParseOptionForceExport     ( const std::vector<std::string> & optdata );
        bool ParseOptionAssembleLvlList ( const std::vector<std::string> & optdata );
        bool ParseOptionAssembleBgList  ( const std::vector<std::string> & optdata );
        bool ParseOptionAssemblePatchOp ( const std::vector<std::string> & optdata );

        //Execution
        void DetermineOperation();
        int  Execute           ();
        int  GatherArgs        ( int argc, const char * argv[] );

        void SetupCFGPath(const std::string & cfgrelpath);
        void ValidateRomRoot()const;

        void HandleExtraTasks(pmd2::GameDataLoader & gloader);
        int HandleImport(const std::string & inpath, pmd2::GameDataLoader & gloader);
        int HandleExport(const std::string & inpath, pmd2::GameDataLoader & gloader);

        void RunLvlListAssembly     ( const ExtraTasks & task, pmd2::GameDataLoader & gloader );
        void RunActorListAssembly   ( const ExtraTasks & task, pmd2::GameDataLoader & gloader );
        void RunPatchOp             ( const ExtraTasks & task, pmd2::GameDataLoader & gloader );


    private:
        //Variables
        utils::cmdl::RAIIClogRedirect   m_redirectClog;
        std::string                     m_firstarg;
        std::string                     m_romroot;
        std::string                     m_cfgpath;
        std::string                     m_applicationdir;
        eOpMode                         m_opmode;
        
        std::deque<ExtraTasks>          m_extasks;
        std::deque<std::string>         m_processlist;      //List of map entries to load
    };


#endif