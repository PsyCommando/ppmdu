#ifndef GFXCRUNCH_HPP
#define GFXCRUNCH_HPP
/*
gfxcrunch.hpp
2014/10/05
psycommando@gmail.com
Description: Main code for the Sprite utility !
*/
#include <ext_fmts/supported_io.hpp>
#include <utils/cmdline_util.hpp>
#include <atomic>
#include <future>

namespace gfx_util
{
    struct pathwrapper_t;

    /*
        CSpriteUtil
            The commandline application that handles pmd2 sprites.
    */
    class CGfxUtil : public utils::cmdl::CommandLineUtility
    {
    public:
        static CGfxUtil & GetInstance();

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
        //Constructor stuff
        CGfxUtil();
        void _Construct();
        //Disable copy and move
        CGfxUtil( const CGfxUtil & );
        CGfxUtil( CGfxUtil && );
        CGfxUtil& operator=(const CGfxUtil&);
        CGfxUtil& operator=(const CGfxUtil&&);

        //Parse Arguments
        bool ParseInputPath  ( const std::string & path );
        bool ParseOutputPath ( const std::string & path );
        bool ParseExtraPath  ( const std::string & path );

        bool ShouldParseOutputPath( const std::vector<std::vector<std::string>> & optdata, const std::deque<std::string> & priorparam, size_t nblefttoparse );

        //Parse Options
        bool ParseOptionQuiet           ( const std::vector<std::string> & optdata );
        bool ParseOptionExportFormat    ( const std::vector<std::string> & optdata );
        //bool ParseOptionForceInputFormat( const std::vector<std::string> & optdata );
        bool ParseOptionImportByIndex   ( const std::vector<std::string> & optdata );
        bool ParseOptionAnimResPath     ( const std::vector<std::string> & optdata );
        bool ParseOptionFaceNamesPath   ( const std::vector<std::string> & optdata );
        bool ParseOptionPokeNamesPath   ( const std::vector<std::string> & optdata );
        bool ParseOptionPokeSprNamesPath( const std::vector<std::string> & optdata );
        bool ParseOptionCompressPKDPX   ( const std::vector<std::string> & optdata );
        bool ParseOptionBuildPack       ( const std::vector<std::string> & optdata );
        bool ParseOptionNbThreads       ( const std::vector<std::string> & optdata );
        bool ParseOptionLog             ( const std::vector<std::string> & optdata );

        bool ParseOptionNoResFix        ( const std::vector<std::string> & optdata );

        bool ParseOptionForceExport     ( const std::vector<std::string> & optdata );
        bool ParseOptionForceImport     ( const std::vector<std::string> & optdata );
        bool ParseOptionBGP             ( const std::vector<std::string> & optdata );
        bool ParseOptionWAN             ( const std::vector<std::string> & optdata );
        bool ParseOptionPkPortraits     ( const std::vector<std::string> & optdata );
        bool ParseOptionPkSprites       ( const std::vector<std::string> & optdata );
        bool ParseOptionPropSprites     ( const std::vector<std::string> & optdata );
        bool ParseOptionConfig          ( const std::vector<std::string> & optdata );
        bool ParseOptionTSet            ( const std::vector<std::string> & optdata );


        //Execution
        int UnpackSprite();
        int BuildSprite();
        int UnpackAndExportPackedCharSprites();
        int PackAndImportCharSprites();
        int DecompressAndHandle();

        int ImportMainFontFile();
        int ExportMainFontFile();

        void DoImportPortraits();
        void DoExportPortraits();

        void DoImportPokeSprites();
        void DoExportPokeSprites();

        void DoImportMiscSprites();
        void DoExportMiscSprites();

        void HandleBGP();
        void HandleWAN();
        void HandleWTE();
        void DoImportTileset();
        void DoExportTileset();


        //Utility
        bool DetermineOperationMode();                                 //Figure out what to do based on our input and outputs args + options !
        int  GatherArgs            ( int argc, const char * argv[] );  //Handle argument parsing + exceptions
        int  Execute               ();                                 //Handle excution switch case + exceptions
        int  ExecOld               ();                                 //Exec the old version code trying to guess the input
        int  ExecNew               ();                                 //Exec the new command-line options only based method
        void PrintOperationMode    ();

        void ChkAndHndlUnsupportedRawOutput();

        bool DetermineOperationModeNew();

        //Constants
        static const std::string                                 Exe_Name;
        static const std::string                                 Title;
        static const std::string                                 Version;
        static const std::string                                 Short_Description;
        static const std::string                                 Long_Description;
        static const std::string                                 Misc_Text;
        static const std::vector<utils::cmdl::argumentparsing_t> Arguments_List;
        static const std::vector<utils::cmdl::optionparsing_t>   Options_List;

        static const std::string                                 DefPathAnimRes;
        static const std::string                                 DefPathPokeSprNames;
        static const std::string                                 DefPathFaceNames;
        static const std::string                                 DefPathPokeNames;

        static const utils::cmdl::argumentparsing_t              ExtraArg; 

        //The operations that can be done by the program
        enum struct eExecMode
        {
            INVALID_Mode,

            //Import/Build Modes:
            BUILD_WAN_Mode,                //Import a WAN sprite from a directory structure
            IMPORT_BGP_Mode,               //Import a BGP image
            IMPORT_WTE_Mode,               //Import a WTE image
            BUILD_POKE_SPRITES_PACK_Mode,  //Import a pack file, and export all its content
            BUILD_KAOMADO_Mode,            //Import a kaomado file
            IMPORT_MAINFONT,               //Import the main 1bpp font used in PMD2

            //Export/Unpack Modes:
            UNPACK_WAN_Mode,               //Export a WAN sprite to a directory structure
            EXPORT_BGP_Mode,               //Export a BGP image
            EXPORT_WTE_Mode,               //Export a WTE image
            UNPACK_POKE_SPRITES_PACK_Mode, //Export a pack file, and export all its content
            UNPACK_KAOMADO_Mode,           //Export a kaomado file from a directory
            EXPORT_MAINFONT,               //Export a folder to a 1bpp main font file!
            
            EXPORT_Tileset,                 //Export BPC + BMA + BPL and BPA

            //Special Modes:
            DECOMPRESS_AND_INDENTIFY_Mode, //Decompress the container and try to figure out what is inside
        };

        //Program Settings
        bool                           m_bQuiet;          //Whether we should output to console 
        bool                           m_ImportByIndex;   //Whether the images should be imported by their index number, and not just the order they're sorted as
        bool                           m_compressToPKDPX; //Whether the content should be compressed. Works with sprite files, and packed sprite files only this far!
        bool                           m_bRedirectClog;   //Whether we should redirect clog to a file
        bool                           m_bNoResAutoFix;   //Whether in case of resolution mismatch between the sprite XML data and the images, the utility will autofix
                                                          // the content of meta-frames with the resolution of the corresponding image!
        eExecMode                      m_execMode;        //This is set after reading the input path.

        std::string                    m_inputPath;      //This is the input path that was parsed 
        std::string                    m_outputPath;     //This is the output path that was parsed
        std::string                    m_pmd2cfg;        //PMD2 config data path

        utils::io::eSUPPORT_IMG_IO     m_PrefOutFormat;   //The image format to use when exporting

        //Other Misc Runtime Data<
        std::vector<std::string>       m_packPokemonNameList;   //List of all the pokemon names for each entry in a packfile!
        std::string                    m_pathToAnimNameResFile; //Path to the XML file containing the details on what files to use for what kind of sprites!
        std::string                    m_pathToPokeSprNamesFile;//Path to the file containing the name to give every entries in a Pack file containing pokemon sprites
        std::string                    m_pathToFaceNamesFile;   //Path to the file containing the name to give every face slots for each pokemon in a kaomado.kao file
        std::string                    m_pathToPokeNamesFile;   //Path to the file containing the name to give every entries in a "kaomado.kao" file

        //Temporary Execution Stuff
        //std::atomic<uint32_t> m_inputCompletion;//#REMOVEME
        //std::atomic<uint32_t> m_outputCompletion;//#REMOVEME
        //std::atomic<bool>     m_bStopProgressPrint;//#REMOVEME
        //std::future<void>     m_runThUpHpBar;//#REMOVEME

        utils::cmdl::RAIIClogRedirect m_redirectClog;


        //New System Stuff
        enum struct eFMT
        {
            INVALID,
            BGP,
            WAN,
            WTE,

        };

        eFMT m_GameFmt;
        bool m_Import;
        bool m_Export;

        bool m_doPkSpr;
        bool m_doPkKao;
        bool m_doPropSpr;
        //bool m_doTileset;
        std::vector<std::string> m_extraargs;
    };
};

#endif