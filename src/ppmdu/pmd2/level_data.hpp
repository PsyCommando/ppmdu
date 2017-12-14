#ifndef LEVEL_DATA_HPP
#define LEVEL_DATA_HPP
/*
level_data.hpp
2016/11/11
psycommando@gmail.com
Description: Utilities for exporting and importing level data to and from folders containing XML.
*/
#include <ppmdu/pmd2/pmd2.hpp>
#include <ppmdu/pmd2/pmd2_configloader.hpp>
#include <ppmdu/containers/level_tileset.hpp>
#include <ppmdu/containers/level_tileset_list.hpp>
#include <string>
#include <vector>
#include <unordered_map>

namespace pmd2
{
//
//
//
    /*
        LevelData
            Contain information on a level entry for the game. Mainly data used to refer to the content for this level!
            Done this way since, a level can share resources with another level!
            Any resources borrowed from another level is only referred to in name only! 
    */
    struct LevelData
    {
        level_info  lvlinf;         //Level list data for this level
        LvlResList  tilesetres;     //bg_list data for this level. (Filenames of the tileset resources tied to this level)
    };

    /*
        LevelsDB
            Contains all the data about several levels from the PMD2 game!
            Its a combination of the content of the level list in the  game, and the  bg_list.dat file!
    */
    class LevelsDB
    {
    public:

    private:
        std::unordered_map<std::string,LevelData> m_lvldata; //First is lvlname, second is level data

    };

//
//
//
    /*
    */
    void ExportXMLLevels( const std::string    & destdir, 
                          const LevelsDB       & lvldata );

    /*
    */
    LevelsDB ImportXMLLevels( const std::vector<std::string> & lvldirpaths );

};

#endif

