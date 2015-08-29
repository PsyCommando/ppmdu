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
#include <ppmdu/basetypes.hpp>
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
            itwriteto = utils::WriteIntToByteVector( WTE_MAGIC_NUMBER_INT, itwriteto, false ); //Magic number is big endian
            itwriteto = utils::WriteIntToByteVector( ptrImg,               itwriteto );
            itwriteto = utils::WriteIntToByteVector( imglen,               itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk0,                 itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk1,                 itwriteto );
            itwriteto = utils::WriteIntToByteVector( imgWidth,             itwriteto );
            itwriteto = utils::WriteIntToByteVector( imgHeight,            itwriteto );
            itwriteto = utils::WriteIntToByteVector( ptrPal,               itwriteto );
            itwriteto = utils::WriteIntToByteVector( nbColorsPal,          itwriteto );
            return itwriteto;
        }

        /**************************************************************
        **************************************************************/
        template<class _init>
            _init ReadFromContainer( _init itReadfrom )
        {
            magic       = utils::ReadIntFromByteVector<decltype(magic)>      (itReadfrom, false); //Magic number is big endian
            ptrImg      = utils::ReadIntFromByteVector<decltype(ptrImg)>     (itReadfrom);
            imglen      = utils::ReadIntFromByteVector<decltype(imglen)>     (itReadfrom);
            unk0        = utils::ReadIntFromByteVector<decltype(unk0)>       (itReadfrom);
            unk1        = utils::ReadIntFromByteVector<decltype(unk1)>       (itReadfrom);
            imgWidth    = utils::ReadIntFromByteVector<decltype(imgWidth)>   (itReadfrom);
            imgHeight   = utils::ReadIntFromByteVector<decltype(imgHeight)>  (itReadfrom);
            ptrPal      = utils::ReadIntFromByteVector<decltype(ptrPal)>     (itReadfrom);
            nbColorsPal = utils::ReadIntFromByteVector<decltype(nbColorsPal)>(itReadfrom);
            return itReadfrom;
        }
    };
};

#endif 