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
//  Game Loader
//======================================================================================
    
    /*
        GameData
            This identify what game the files being handled belongs to, and allows accessing all its data
            without having to access to data through the filesystem. It also indexes all the game data for
            easy retrieval and editing.

            Its made up of separate modules that each handles their respective specialized field of data.
    */
    class GameData
    {
    public:
        GameData ( const std::wstring & gamedir );
        ~GameData();

        //Set Game Dir
        void SetGameDir( const std::wstring & gamedir );
        const std::wstring & GetGameDir()const;

        //Handles Loading the Game Data
        void Load();

        //Handles Writing the Game Data
        void Write()const;

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
        GameAudio               * GetAudio();
        const GameAudio         * GetAudio()const;

    private:
        std::unique_ptr<GameText>            m_text;
        std::unique_ptr<GameScripts>         m_scripts;
        std::unique_ptr<GameGraphics>        m_graphics;
        std::unique_ptr<stats::GameStats>    m_stats;
        std::unique_ptr<GameAudio>           m_audio;

    };

};

#endif