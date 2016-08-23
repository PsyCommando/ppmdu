#ifndef WTE_HPP
#define WTE_HPP
/*
wte.hpp
2015/01/28
psycommando@gmail.com
Description: Utilities for handling the WTE file format from the PMD2 games.
             The WTE format is SIR0-wrapped !
*/
#include <types/content_type_analyser.hpp>
#include <ppmdu/fmts/sir0.hpp>

namespace filetypes 
{
//===============================================================================
// Constants
//===============================================================================
    static const uint32_t  WTE_MAGIC_NUMBER_INT = 0x57544500; //"WTE\0"
    extern const ContentTy CnTy_WTE;    //Content ID db handle, and contains file extension too.

//===============================================================================
// Struct
//===============================================================================
    struct WTE_header
    {
        uint32_t magic;
        uint32_t ptrImg;
        uint32_t imglen;
        uint32_t unk0;
        uint32_t unk1;
        uint16_t imgWidth;
        uint16_t imgHeight;
        uint32_t ptrPal;
        uint32_t nbColorsPal;

        static const uint32_t HEADER_LENGTH = 32; //bytes

        /**************************************************************
        **************************************************************/
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            //Force the magic number instead of the "magic" variable's content
            itwriteto = utils::WriteIntToBytes( WTE_MAGIC_NUMBER_INT, itwriteto, false ); //Magic number is big endian
            itwriteto = utils::WriteIntToBytes( ptrImg,               itwriteto );
            itwriteto = utils::WriteIntToBytes( imglen,               itwriteto );
            itwriteto = utils::WriteIntToBytes( unk0,                 itwriteto );
            itwriteto = utils::WriteIntToBytes( unk1,                 itwriteto );
            itwriteto = utils::WriteIntToBytes( imgWidth,             itwriteto );
            itwriteto = utils::WriteIntToBytes( imgHeight,            itwriteto );
            itwriteto = utils::WriteIntToBytes( ptrPal,               itwriteto );
            itwriteto = utils::WriteIntToBytes( nbColorsPal,          itwriteto );
            return itwriteto;
        }

        /**************************************************************
        **************************************************************/
        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itpastend )
        {
            magic       = utils::ReadIntFromBytes<decltype(magic)>      (itReadfrom, itpastend, false); //Magic number is big endian
            ptrImg      = utils::ReadIntFromBytes<decltype(ptrImg)>     (itReadfrom, itpastend);
            imglen      = utils::ReadIntFromBytes<decltype(imglen)>     (itReadfrom, itpastend);
            unk0        = utils::ReadIntFromBytes<decltype(unk0)>       (itReadfrom, itpastend);
            unk1        = utils::ReadIntFromBytes<decltype(unk1)>       (itReadfrom, itpastend);
            imgWidth    = utils::ReadIntFromBytes<decltype(imgWidth)>   (itReadfrom, itpastend);
            imgHeight   = utils::ReadIntFromBytes<decltype(imgHeight)>  (itReadfrom, itpastend);
            ptrPal      = utils::ReadIntFromBytes<decltype(ptrPal)>     (itReadfrom, itpastend);
            nbColorsPal = utils::ReadIntFromBytes<decltype(nbColorsPal)>(itReadfrom, itpastend);
            return itReadfrom;
        }
    };
};

#endif 