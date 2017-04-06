#ifndef PMD2_GAME_LOADER_HPP
#define PMD2_GAME_LOADER_HPP
/*
pmd2_gameloader.hpp
2015/09/11
psycommando@gmail.com
Description:
    Utility for loading and editing game data from the PMD2 games.
*/
#include <cstdint>
#include <array>
#include <string>
#include <memory>
#include <ppmdu/pmd2/game_stats.hpp>
#include <ppmdu/pmd2/pmd2_text.hpp>
#include <ppmdu/pmd2/pmd2_scripts.hpp>
#include <ppmdu/pmd2/pmd2_graphics.hpp>
#include <ppmdu/pmd2/pmd2_audio.hpp>
#include <ppmdu/pmd2/pmd2_asm.hpp>
#include <ppmdu/pmd2/pmd2_levels.hpp>

//! #TODO: The gameloader header could be possibly more easily turned into an accessible
//!         interface for a possible shared library. If the dependencies and implementation can be
//!         pimpl-ifed.

namespace pmd2
{
//
//
//
    /*
        EX_NoRomDataAvailable
            Exception class sent out when there is no rom data found and a functionality requiring rom data was used!
    */
    class EX_NoRomDataAvailable : public std::runtime_error
    {
    public:
        EX_NoRomDataAvailable( const std::string & msg):std::runtime_error(msg){}
        EX_NoRomDataAvailable( const EX_NoRomDataAvailable & other ) = default;
        EX_NoRomDataAvailable( EX_NoRomDataAvailable && other ) = default;
        EX_NoRomDataAvailable & operator=(const EX_NoRomDataAvailable & other) = default;
        EX_NoRomDataAvailable & operator=(EX_NoRomDataAvailable && other) = default;
    };

//======================================================================================
//  GameDataLoader
//======================================================================================
    /*
        GameDataLoader
            This identify what game the files being handled belongs to, and allows accessing all its data
            without having to access to data through the filesystem. It also indexes all the game data for
            easy retrieval and editing.

            Its made up of separate modules that each handles their respective specialized field of data.

            **This instantiate the MainPMD2ConfigWrapper config instance**
    */
    class GameDataLoader
    {
    public:
        GameDataLoader( const std::string & romroot, const std::string & pmd2confxmlfile );
        ~GameDataLoader();

        //Load configuration
        void AnalyseGame();

        //Set ROM Root Dir (Directory conatining arm9.bin, data, and overlay directory)
        void                SetRomRoot( const std::string & romroot );
        const std::string & GetRomRoot()const;

        //Return the game version and locale
        inline eGameRegion   GetGameRegion ()const {return MainPMD2ConfigWrapper::CfgInstance().GetGameVersion().region; }
        inline eGameVersion  GetGameVersion()const {return MainPMD2ConfigWrapper::CfgInstance().GetGameVersion().version;}

        //Handles Loading the Game Data
        //void Init();

        GameText        * InitGameText();
        GameScripts     * InitScripts(const scriptprocoptions & options);
        GameLevels      * InitLevels(const lvlprocopts & options);
        GameGraphics    * InitGraphics();
        GameStats       * InitStats();
        GameAudio       * InitAudio();
        PMD2_ASM        * InitAsm();

        //Handles Writing the Game Data back to the
        //void DeInit();

        void DeInitGameText();
        void DeInitScripts();
        void DeInitLevels();
        void DeInitGraphics();
        void DeInitStats();
        void DeInitAudio();
        //void DeInitAsm();

        /*
            Access to the sub-sections of the game's data
        */
        GameText                * GetGameText();
        const GameText          * GetGameText()const;

        //
        GameScripts             * GetScripts();
        const GameScripts       * GetScripts()const;

        //
        GameLevels              * GetLevels();
        const GameLevels        * GetLevels()const;

        //
        GameGraphics            * GetGraphics();
        const GameGraphics      * GetGraphics()const;

        //
        GameStats               * GetStats();
        const GameStats         * GetStats()const;

        //
        GameAudio               * GetAudio();
        const GameAudio         * GetAudio()const;

        //
        PMD2_ASM                * GetAsm();
        const PMD2_ASM          * GetAsm()const;

    private:
        
        bool LoadConfigUsingARM9();
        //Code common to all init methods!
        void DoCommonInit(bool bcheckdata = true);

    private:
        std::shared_ptr<GameText>            m_text;
        std::shared_ptr<GameScripts>         m_scripts;
        std::unique_ptr<GameLevels>          m_levels;
        std::unique_ptr<GameGraphics>        m_graphics;
        std::unique_ptr<GameStats>           m_stats;
        std::unique_ptr<GameAudio>           m_audio;
        std::unique_ptr<PMD2_ASM>            m_asmmanip;

        std::string                          m_romroot;
        std::string                          m_datadiroverride; //Contains the name of the data directory if name non-default
        std::string                          m_configfile;

        //State
        bool                                 m_bAnalyzed;

        //Error conditions
        bool                                 m_nodata;      //This is true, if there is no data directory
        bool                                 m_noarm9;      //This is true if there is no arm9.bin file
        bool                                 m_nooverlays;  //This is true if there are no overlay directory or overlay files

        //No copies
        GameDataLoader(const GameDataLoader&)            = delete;
        GameDataLoader& operator=(const GameDataLoader&) = delete;
    };

};

#endif