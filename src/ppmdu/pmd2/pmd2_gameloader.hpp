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
//  Game Config Loader
//======================================================================================

    /*
        GameConfigLoader
            This object identifies the version of the game being loaded
    */
    //class GameConfigLoader
    //{
    //public:
    //    GameConfigLoader( const std::wstring & pathConf ); //Path to master xml config file



    //private:
    //    std::wstring m_pathConf;
    //};

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
        GameDataLoader( const std::string & romroot );
        ~GameDataLoader();

        //Set ROM Root Dir (Directory conatining arm9.bin, data, and overlay directory)
        void SetRomRoot( const std::string & romroot );
        const std::string & GetRomRoot()const;

        //Handles Loading the Game Data
        void Load();

        //#TODO: implement those in their respective cpp file. To reduce dependencies.
        GameText         * LoadGameText();
        GameScripts      * LoadScripts();
        GameGraphics     * LoadGraphics();
        stats::GameStats * LoadStats();
        GameAudio        * LoadAudio();
        PMD2_ASM_Manip   * LoadAsm();

        //Handles Writing the Game Data
        void Write();

        //#TODO: implement those in their respective cpp file. To reduce dependencies.
        void WriteGameText();
        void WriteScripts();
        void WriteGraphics();
        void WriteStats();
        void WriteAudio();
        void WriteAsm();

        /*
            Access to the sub-sections of the game's data
        */
        GameText               * GetGameText();
        const GameText         * GetGameText()const;

        //
        GameScripts            * GetScripts();
        const GameScripts      * GetScripts()const;

        //
        GameGraphics           * GetGraphics();
        const GameGraphics     * GetGraphics()const;

        //
        stats::GameStats       * GetStats();
        const stats::GameStats * GetStats()const;

        //
        GameAudio              * GetAudio();
        const GameAudio        * GetAudio()const;

        //
        PMD2_ASM_Manip         * GetAsmManip();
        const PMD2_ASM_Manip   * GetAsmManip()const;

    private:
        void AnalyseGame();

    private:
        std::unique_ptr<GameText>            m_text;
        std::unique_ptr<GameScripts>         m_scripts;
        std::unique_ptr<GameGraphics>        m_graphics;
        std::unique_ptr<stats::GameStats>    m_stats;
        std::unique_ptr<GameAudio>           m_audio;
        std::unique_ptr<PMD2_ASM_Manip>      m_asmmanip;
        std::string                          m_romroot;
        std::string                          m_datadiroverride; //Contains the name of the data directory if name non-default

        //State
        eGameLocale                          m_gamelocale;
        eGameVersion                         m_gameversion;

        //No copies
        GameDataLoader(const GameDataLoader&)            = delete;
        GameDataLoader& operator=(const GameDataLoader&) = delete;
    };

};

#endif