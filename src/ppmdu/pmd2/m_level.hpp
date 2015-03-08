#ifndef M_LEVEL_HPP
#define M_LEVEL_HPP
/*
m_level.hpp
2015/03/01
psycommando@gmail.com
Description:
    Resources for handling the data in the "/BALANCE/m_level.bin" file of PMD2!
*/
#include <ppmdu/containers/pokemon_stats.hpp>
#include <vector>
#include <string>
#include <cstdint>

namespace pmd2{ namespace filetypes 
{
    static const unsigned int MLevel_DefNBPokemon = 571; //Nb of pokemon stats growth entries in the m_level.bin file.

//
//
//
    /*
    */
    std::vector<stats::CPokemon> & ParseLevelGrowthData( const std::string            & inpath, 
                                                         std::vector<stats::CPokemon> & out_pkmdat );
    std::vector<stats::CPokemon> & ParseLevelGrowthData( const std::string                     & inpath,
                                                         std::vector<stats::CPokemon>::iterator  itbegpkmdat,
                                                         std::vector<stats::CPokemon>::iterator  itendpkmdat );

    /*
    */
    void WriteLevelGrowthData( const std::vector<stats::CPokemon> & pkmdat, 
                               const std::string                  & outpath );
    void WriteLevelGrowthData( const std::string                            & ouytpath,
                               std::vector<stats::CPokemon>::const_iterator  itbegpkmdat,
                               std::vector<stats::CPokemon>::const_iterator  itendpkmdat );

};};

#endif