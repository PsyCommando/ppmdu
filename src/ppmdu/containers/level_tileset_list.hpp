#ifndef LEVEL_TILESET_LIST_HPP
#define LEVEL_TILESET_LIST_HPP
/*
level_tileset_list.hpp
2017/03/10
psycommando@gmail.com
Description: Container for level list data!
*/
#include <ppmdu/pmd2/pmd2.hpp>
#include <string>
#include <vector>
#include <unordered_map>

namespace pmd2
{
    /*
        LvlResList
    */
    struct LvlResList
    {
        std::string                bplname;
        std::string                bpcname;
        std::string                bmaname;
        std::vector<std::string>   extranames;
    };

    /*
        
    */
    typedef std::unordered_map<std::string,LvlResList> LevelTilesetList; 

    /*
    */
    LevelTilesetList LoadLevelTilesetResourceList ( const std::string & bglistfile );
    void             WriteLevelTilesetResourceList( const std::string & bglistfile, const LevelTilesetList & src );
};

#endif