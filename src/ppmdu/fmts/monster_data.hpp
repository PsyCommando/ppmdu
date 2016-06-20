#ifndef MONSTER_DATA_HPP
#define MONSTER_DATA_HPP
/*
monster_data.hpp
2015/03/06
psycommando@gmail.com
Description:
    Utilities for handling the monster.md file format from PMD2!

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P
*/
#include <types/content_type_analyser.hpp>
#include <ppmdu/containers/pokemon_stats.hpp>
#include <string>
#include <cstdint>
#include <vector>

namespace filetypes
{
//======================================================================================
//  Constants
//======================================================================================
    static const uint32_t    MonsterMD_DefNBPokemons = 1155; //0x483
    static const uint32_t    MonsterMD_DefNBRegulars = 555;
    static const uint32_t    MonsterMD_DefNBSpecials = 45;
    static const uint32_t    MonsterMD_EntrySizeEoS  = 68; //bytes
    static const uint32_t    MonsterMD_EntrySizeEoTD = 76; //bytes
    static const std::string MonsterMD_FName         = "monster.md";

    extern const ContentTy CnTy_MonsterMD; //Content ID db Handle.

//==========================================================================================
//  monster.md header
//==========================================================================================
    struct monstermd_header
    {
        static const unsigned int HEADER_LEN       = 8; //bytes
        static const uint32_t     MAGIC_NUMBER     = 0x4D440000; // {0x4D,0x44,0x00,0x00} "MD\0\0"

        uint32_t magicn    = 0;
        uint32_t nbentries = 0;

        //
        inline bool isValid()const { return ( (magicn == MAGIC_NUMBER) && ( nbentries > 0 ) ); }

        //
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( MAGIC_NUMBER, itwriteto, false ); //Force this, to avoid bad surprises
            itwriteto = utils::WriteIntToBytes( nbentries,    itwriteto );
            return itwriteto;
        }

        //Reading the magic number, and endzero value is solely for validating on read.
        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itpastend )
        {
            magicn    = utils::ReadIntFromBytes<decltype(magicn)>   (itReadfrom, itpastend, false ); //iterator is incremented
            nbentries = utils::ReadIntFromBytes<decltype(nbentries)>(itReadfrom, itpastend); //iterator is incremented
            return itReadfrom;
        }
    };


//======================================================================================
//  Function
//======================================================================================
    /*
        Returns a reference to the output vector passed as parameter!
    */
    std::vector<pmd2::stats::PokeMonsterData> & ParsePokemonBaseData( const std::string                         & inpath, 
                                                                      std::vector<pmd2::stats::PokeMonsterData> & out_pkmdat );

    std::vector<pmd2::stats::PokeMonsterData>   ParsePokemonBaseData( const std::string                         & inpath );

    /*
    */
    void WritePokemonBaseData( const std::vector<pmd2::stats::PokeMonsterData> & pkmdat, 
                               const std::string                               & outpath );

};

#endif