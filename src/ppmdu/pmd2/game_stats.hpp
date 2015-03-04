#ifndef GAME_STATS_HPP
#define GAME_STATS_HPP 
/*
game_stats.hpp
2015/03/01
psycommando@gmail.com
Description: 
    This file contains generic resources for handling the PMD2 game data.
*/
#include <string>
#include <vector>

namespace pmd2{ namespace stats
{ 
    //Game data filenames list
    static const std::string StatsGrowthFilename;

    //Game data location list
    static const std::string GameStatsFolderPath;

    //Game data type list
    enum struct eGameDataTy 
    {
        Invalid,
        StatsGrowthData,
    };

    //Functions for identifying
    eGameDataTy GetGameDataTyForFile( const std::string & path );

    //Function for probing a directory to import
    eGameDataTy GetGameDataTyForDirectory( const std::string & path );
};};

#endif