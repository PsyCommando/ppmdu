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

    extern const std::string LevelData_LevelDefFname; //Filenaname of the level definition file generated inside an exported level folder!
    extern const std::string LevelDataXMLRoot;

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
    //!NOTE: Have to do this, so statsutil compiles
#ifdef PPMDU_STATSUTIL
    class GameLevels{};
#else
    class GameLevels
    {
    public:

        GameLevels(const std::string & fsrootdir, const ConfigLoader & conf, std::shared_ptr<GameScripts> && gs, const lvlprocopts & options );
        GameLevels(const GameLevels&) = default;
        GameLevels(GameLevels&&) = default;
        ~GameLevels();

        /***********************************************************************************************
            ExportAllLevels
                Exports all levels in the romroot to the "destdir" specified, as XML and tile data.
        ***********************************************************************************************/
        void ExportAllLevels( const std::string & destdir );

        /***********************************************************************************************
            ExportLevel
                Exports level "levelname" from the romroot to the "destdir" specified, as XML and tile data.
        ***********************************************************************************************/
        void ExportLevel( const std::string & destdir, const std::string & levelname);


        /***********************************************************************************************
            ImportAllLevels
                Import XML and tiledata for all levels within the "srcdir" directory into the romroot.
        ***********************************************************************************************/
        void ImportAllLevels( const std::string & srcdir );

        /***********************************************************************************************
            ImportLevel
                Import XML and tiledata for a single level within the "srclvldir" directory into the romroot.
        ***********************************************************************************************/
        void ImportLevel(const std::string & srclvldir);

        //utility

        /***********************************************************************************************
            DirContainsXMLLevelDef
                Returns whether the neccessary files for importing a level from  XML data are there!
        ***********************************************************************************************/
        bool DirContainsXMLLevelDef(const std::string & srclvldir);


        //
        void SetGameScriptData(const GameScriptData & pgsdata);
        const std::string    & GetMapBgDir()const;
        const ConfigLoader   & GetConfig()const;
        const lvlprocopts    & GetOptions()const;

    private:
        class GameLevelsHandler;
        std::unique_ptr<GameLevelsHandler> m_pimpl;
    };
#endif

};

#endif
