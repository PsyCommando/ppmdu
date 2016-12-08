#ifndef PMD2_LEVELS_HPP
#define PMD2_LEVELS_HPP
/*
pmd2_levels.hpp
2016/12/05
psycommando@gmail.com
Description: Utilities for loading and saving levels related data to and from PMD2!
*/
#include <ppmdu/pmd2/pmd2.hpp>
#include <ppmdu/pmd2/pmd2_configloader.hpp>
#include <ppmdu/containers/level_tileset.hpp>
#include <ppmdu/fmts/bg_list_data.hpp>
#include <ppmdu/pmd2/pmd2_scripts.hpp>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <deque>
#include <memory>

namespace pmd2
{
//==========================================================================================================
//  Constants
//==========================================================================================================
    //use DirName_MAP_BG

//==========================================================================================================
//  Levels Manager
//==========================================================================================================


    /*************************************
        lvlprocopts
    *************************************/
    struct lvlprocopts
    {
        bool            bexportscripts = true;
        eHCDataLoadSrc  lvllistsrc     = eHCDataLoadSrc::ParseFromBin;
    };
    const lvlprocopts Default_Level_Options;

    /*************************************
        GameLevels
            Loads and process levels from the PMD2 games.
    *************************************/
    class GameLevels
    {
        friend class GameLevelsHandler;
    public:
        GameLevels(const std::string & fsrootdir, const ConfigLoader & conf, GameScripts & gs, const lvlprocopts & options = Default_Level_Options );
        ~GameLevels();

        //
        void ExportLevels( const std::string & destdir );

        void ImportLevels( const std::string & srcdir );

        void SetGameScriptData(const GameScriptData & pgsdata);
        const std::string    & GetMapBgDir()const;// {return m_mapbgdir;}
        const ConfigLoader   & GetConfig()const;  // {return m_gconf;}
        const lvlprocopts    & GetOptions()const; // {return m_options;}

    private:
        std::unique_ptr<GameLevelsHandler> m_pimpl;
    };

};

#endif
