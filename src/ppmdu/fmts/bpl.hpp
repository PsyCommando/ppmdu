#ifndef BPL_HPP
#define BPL_HPP
/*
bpl.hpp
2016/09/28
psycommando@gmail.com
Description: Utilities for handling the BPL file format.
*/
#include <ppmdu/pmd2/pmd2.hpp>
#include <ppmdu/containers/level_tileset.hpp>
#include <types/content_type_analyser.hpp>
#include <utils/utility.hpp>
#include <cstdint>
#include <string>
#include <vector>
#include <array>

namespace filetypes
{
//============================================================================================
//  Constants
//============================================================================================
    const std::string       BPL_FileExt = "bpl";
    extern const ContentTy  CnTy_BPL; //Content ID handle

//============================================================================================
//  Struct
//============================================================================================
    
    /*
        bpl_header
    */
    struct bpl_header
    {
        static const size_t LEN = 4; //bytes
        uint16_t nbpalettes;
        uint16_t unk1;


        template<class _outit>
            _outit Write( _outit itw )const
        {
            itw = utils::WriteIntToBytes(nbpalettes,    itw );
            itw = utils::WriteIntToBytes(unk1,          itw );
            return itw;
        }


        template<class _fwdinit>
            _fwdinit Read( _fwdinit itr, _fwdinit itpend )
        {
            itr = utils::ReadIntFromBytes(nbpalettes,    itr, itpend );
            itr = utils::ReadIntFromBytes(unk1,          itr, itpend );
            return itr;
        }
    };

//============================================================================================
//  Functions
//============================================================================================
    pmd2::TilesetPalette ParseBPL( const std::string & fpath );
    void                 WriteBPL( const std::string & destfpath, const pmd2::TilesetPalette & srcpal );
};

#endif
