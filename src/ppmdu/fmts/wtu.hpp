#ifndef WTU_HPP
#define WTU_HPP
/*
wtu.hpp
2015/01/28
psycommando@gmail.com
Description: Utilities for handling the WTU file format from the PMD2 games.
             WTU files are closely linked to a matching WTE file!
*/
#include <types/content_type_analyser.hpp>
#include <utils/utility.hpp>

namespace filetypes 
{
//===============================================================================
// Constants
//===============================================================================
    static const uint32_t WTU_MAGIC_NUMBER_INT = 0x57545500; //"WTU\0"

    extern const ContentTy CnTy_WTU; //Content ID handle

//===============================================================================
// Struct
//===============================================================================
    /*
    */
    struct WTU_header
    {
        uint32_t magic;
        uint32_t nbEntries;
        uint32_t unk0;
        uint32_t unk1;

        static const uint32_t HEADER_LENGTH = 16;


        /**************************************************************
        **************************************************************/
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            //Force the magic number instead of the "magic" variable's content
            itwriteto = utils::WriteIntToBytes( WTU_MAGIC_NUMBER_INT, itwriteto, false ); //Magic number is big endian
            itwriteto = utils::WriteIntToBytes( nbEntries,            itwriteto );
            itwriteto = utils::WriteIntToBytes( unk0,                 itwriteto );
            itwriteto = utils::WriteIntToBytes( unk1,                 itwriteto );
            return itwriteto;
        }

        /**************************************************************
        **************************************************************/
        template<class _init>
            _init ReadFromContainer( _init itread, _init itend )
        {
            magic     = utils::ReadIntFromBytes<decltype(magic)>    (itread, itend, false); //Magic number is big endian
            nbEntries = utils::ReadIntFromBytes<decltype(nbEntries)>(itread, itend);
            unk0      = utils::ReadIntFromBytes<decltype(unk0)>     (itread, itend);
            unk1      = utils::ReadIntFromBytes<decltype(unk1)>     (itread, itend);
            return itread;
        }
    };

};
#endif