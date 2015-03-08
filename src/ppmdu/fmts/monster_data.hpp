#ifndef MONSTER_DATA_HPP
#define MONSTER_DATA_HPP
/*
monster_data.hpp
2015/03/06
psycommando@gmail.com
Description:
    Utilities for handling the monster.md file format from PMD2!
*/
#include <ppmdu/containers/pokemon_stats.hpp>
#include <string>
#include <cstdint>
#include <vector>

namespace pmd2 { namespace filetypes
{
    static const uint32_t MonsterMD_DefNBPokemons = 1155; //0x483
//
//
//
    /*
        Returns a reference to the output vector passed as parameter!
    */
    std::vector<stats::CPokemon> & ParsePokemonBaseData( const std::string            & inpath, 
                                                         std::vector<stats::CPokemon> & out_pkmdat );

    /*
    */
    void WritePokemonBaseData( const std::vector<stats::CPokemon> & pkmdat, 
                               const std::string                  & outpath );
    void WritePokemonBaseData( const std::string                            & ouytpath,
                               std::vector<stats::CPokemon>::const_iterator  itbegpkmdat,
                               std::vector<stats::CPokemon>::const_iterator  itendpkmdat );

};};

#endif