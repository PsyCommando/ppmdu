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
//======================================================================================
//  Constants
//======================================================================================
    static const uint32_t    MonsterMD_DefNBPokemons = 1155; //0x483
    static const uint32_t    MonsterMD_DefNBRegulars = 555;
    static const uint32_t    MonsterMD_DefNBSpecials = 45;
    static const uint32_t    MonsterMD_EntrySizeEoS  = 68; //bytes
    static const uint32_t    MonsterMD_EntrySizeEoTD = 76; //bytes
    static const std::string MonsterMD_FName         = "monster.md";

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
            itwriteto = utils::WriteIntToByteVector( MAGIC_NUMBER, itwriteto, false ); //Force this, to avoid bad surprises
            itwriteto = utils::WriteIntToByteVector( nbentries,    itwriteto );
            return itwriteto;
        }

        //Reading the magic number, and endzero value is solely for validating on read.
        template<class _init>
            _init ReadFromContainer( _init itReadfrom )
        {
            magicn    = utils::ReadIntFromByteVector<decltype(magicn)>   (itReadfrom, false ); //iterator is incremented
            nbentries = utils::ReadIntFromByteVector<decltype(nbentries)>(itReadfrom); //iterator is incremented
            return itReadfrom;
        }
    };


//======================================================================================
//  Function
//======================================================================================
    /*
        Returns a reference to the output vector passed as parameter!
    */
    std::vector<stats::PokeMonsterData> & ParsePokemonBaseData( const std::string                   & inpath, 
                                                                std::vector<stats::PokeMonsterData> & out_pkmdat );

    std::vector<stats::PokeMonsterData>   ParsePokemonBaseData( const std::string                   & inpath );

    /*
    */
    void WritePokemonBaseData( const std::vector<stats::PokeMonsterData> & pkmdat, 
                               const std::string                         & outpath );

};};

#endif