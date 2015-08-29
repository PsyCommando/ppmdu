#ifndef M_LEVEL_HPP
#define M_LEVEL_HPP
/*
m_level.hpp
2015/03/01
psycommando@gmail.com
Description:
    Resources for handling the data in the "/BALANCE/m_level.bin" file of PMD2!

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P
*/
#include <ppmdu/containers/pokemon_stats.hpp>
#include <vector>
#include <string>
#include <cstdint>

namespace filetypes 
{

//=====================================================================================
//  Constants
//=====================================================================================
    static const unsigned int MLevel_EoTD_DefNBPokemon = 1192; //Nb of pokemon stats growth entries in the m_level.bin file for Explores of Time/Darkness. 
    static const unsigned int MLevel_EoS_DefNBPokemon  = 571;  //Nb of pokemon stats growth entries in the m_level.bin file for Explorers of Sky. 

    static const std::string  MLevel_FName             = "m_level.bin";

//=====================================================================================
//  Functions:
//=====================================================================================
    /*
    */
    std::vector<pmd2::stats::PokeStatsGrowth> & ParseLevelGrowthData( const std::string                   & inpath, 
                                                                std::vector<pmd2::stats::PokeStatsGrowth> & out_pkmdat );
    std::vector<pmd2::stats::PokeStatsGrowth>   ParseLevelGrowthData( const std::string                   & inpath );

    /*
    */
    void WriteLevelGrowthData( const std::vector<pmd2::stats::PokeStatsGrowth> & pkmdat, 
                               const std::string                         & outpath );

};

#endif