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
#include <ppmdu/pmd2/pmd2_asm_manip.hpp>

namespace pmd2
{
//======================================================================================
//  GameDataLoader
//======================================================================================

    /*
        GameDataLoader
            This identify what game the files being handled belongs to, and allows accessing all its data
            without having to access to data through the filesystem. It also indexes all the game data for
            easy retrieval and editing.

            Its made up of separate modules that each handles their respective specialized field of data.
    */
    class GameDataLoader
    {
    public:
        //GameDataLoader( const std::string & romroot, const std::string & gamelangxmlfile = "gamelang.xml" );
        GameDataLoader( const std::string & romroot, const std::string & pmd2confxmlfile = DefConfigFileName );
        ~GameDataLoader();

        //Set ROM Root Dir (Directory conatining arm9.bin, data, and overlay directory)
        void                SetRomRoot( const std::string & romroot );
        const std::string & GetRomRoot()const;

        //Return the game version and locale
        inline eGameRegion   GetGameRegion ()const {return MainPMD2ConfigWrapper::CfgInstance().GetGameVersion().region; }
        inline eGameVersion  GetGameVersion()const {return MainPMD2ConfigWrapper::CfgInstance().GetGameVersion().version;}

        //Handles Loading the Game Data
        void Load();

        GameText         * LoadGameText();
        GameScripts      * LoadScripts(bool escapeasxml = false, bool bscriptdebug = false );
        GameGraphics     * LoadGraphics();
        GameStats        * LoadStats();
        GameAudio        * LoadAudio();
        PMD2_ASM_Manip   * LoadAsm();

        //Handles Writing the Game Data
        void Write();


        void WriteGameText();
        void WriteScripts();
        void WriteGraphics();
        void WriteStats();
        void WriteAudio();
        void WriteAsm();

        /*
            Access to the sub-sections of the game's data
        */
        GameText                * GetGameText();
        const GameText          * GetGameText()const;

        //
        GameScripts             * GetScripts();
        const GameScripts       * GetScripts()const;

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
        PMD2_ASM_Manip          * GetAsmManip();
        const PMD2_ASM_Manip    * GetAsmManip()const;

        //const ConfigLoader      * GetConfig()const;

    private:
        void AnalyseGame();
        bool LoadConfigUsingARM9();

    private:
        //std::shared_ptr<ConfigLoader>        m_conf;
        std::shared_ptr<GameText>            m_text;
        std::unique_ptr<GameScripts>         m_scripts;
        std::unique_ptr<GameGraphics>        m_graphics;
        std::unique_ptr<GameStats>           m_stats;
        std::unique_ptr<GameAudio>           m_audio;
        std::unique_ptr<PMD2_ASM_Manip>      m_asmmanip;

        std::string                          m_romroot;
        std::string                          m_datadiroverride; //Contains the name of the data directory if name non-default
        std::string                          m_configfile;

        //State
        //eGameRegion                          m_gameregion;
        //eGameVersion                         m_gameversion;
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