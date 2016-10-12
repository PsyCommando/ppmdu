#ifndef BMA_HPP
#define BMA_HPP
/*
bma.hpp
2016/09/28
psycommando@gmail.com
Description: Utilities for handling the BMA file format.
*/
#include <ppmdu/pmd2/pmd2.hpp>
#include <ppmdu/containers/level_tileset.hpp>
#include <types/content_type_analyser.hpp>
#include <cstdint>
#include <string>
#include <vector>
#include <array>

namespace filetypes
{
//============================================================================================
//  Constants
//============================================================================================
    const std::string       BMA_FileExt = "bma";
    extern const ContentTy  CnTy_BMA; //Content ID handle



    struct bma_header
    {
        static const size_t LEN = 12; //bytes

        uint8_t  width;
        uint8_t  height;
        uint8_t  unk1;
        uint8_t  unk2;
        uint8_t  unk3;
        uint8_t  unk4;
        uint16_t unk5;
        uint16_t unk6;
        uint16_t unk7;

        //
        template<class _outit>
            _outit Write( _outit itw )const
        {
            itw = utils::WriteIntToBytes( width,  itw );
            itw = utils::WriteIntToBytes( height, itw );
            itw = utils::WriteIntToBytes( unk1,   itw );
            itw = utils::WriteIntToBytes( unk2,   itw );
            itw = utils::WriteIntToBytes( unk3,   itw );
            itw = utils::WriteIntToBytes( unk4,   itw );
            itw = utils::WriteIntToBytes( unk5,   itw );
            itw = utils::WriteIntToBytes( unk6,   itw );
            itw = utils::WriteIntToBytes( unk7,   itw );
            return itw;
        }

        //
        template<class _init>
            _init Read( _init itr, _init itpend )
        {
            itr = utils::ReadIntFromBytes( width,  itr, itpend );
            itr = utils::ReadIntFromBytes( height, itr, itpend );
            itr = utils::ReadIntFromBytes( unk1,   itr, itpend );
            itr = utils::ReadIntFromBytes( unk2,   itr, itpend );
            itr = utils::ReadIntFromBytes( unk3,   itr, itpend );
            itr = utils::ReadIntFromBytes( unk4,   itr, itpend );
            itr = utils::ReadIntFromBytes( unk5,   itr, itpend );
            itr = utils::ReadIntFromBytes( unk6,   itr, itpend );
            itr = utils::ReadIntFromBytes( unk7,   itr, itpend );
            return itr;
        }
    };

//============================================================================================
//  Functions
//============================================================================================
    pmd2::TilesetBMAData ParseBMA( const std::string & fpath );
    void                 WriteBMA( const std::string & destfpath, const pmd2::TilesetBMAData & bmadat );
};

#endif