#ifndef SEDL_HPP
#define SEDL_HPP
/*
sedl.hpp
2015/05/20
psycommando@gmail.com
Description: Utilities for handling Pokemon Mystery Dungeon: Explorers of Sky/Time/Darkness's .sed files.

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P
*/
#include <dse/dse_common.hpp>

#ifdef USE_PPMDU_CONTENT_TYPE_ANALYSER
    #include <types/content_type_analyser.hpp>
    namespace filetypes
    {
        extern const ContentTy CnTy_SEDL; //Content ID db handle
    };
#endif

namespace DSE
{

    static const uint32_t SEDL_MagicNumber = static_cast<uint32_t>(eDSEContainers::sedl);//0x7365646C; //"sedl"


    /****************************************************************************************
        SEDL_Header
            The header of the SEDL file.
    ****************************************************************************************/
    struct SEDL_Header
    {
        static const uint32_t Size     = 56; //without padding
        static const uint32_t FNameLen = 16;

        static unsigned int size() { return Size; }

        uint32_t magicn          = 0;
        uint32_t unk7            = 0;
        uint32_t flen            = 0;
        uint16_t version         = 0;
        uint8_t  unk1            = 0;
        uint8_t  unk2            = 0;
        uint32_t unk3            = 0;
        uint32_t unk4            = 0;

        uint16_t year            = 0;
        uint8_t  month           = 0;
        uint8_t  day             = 0;
        uint8_t  hour            = 0;
        uint8_t  minute          = 0;
        uint8_t  second          = 0;
        uint8_t  centisec        = 0;

        std::array<char,FNameLen> fname;

        uint32_t unk5            = 0;
        uint32_t unk6            = 0;


        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes   ( SEDL_MagicNumber, itwriteto, false ); //Write constant magic number, to avoid bad surprises
            itwriteto = utils::WriteIntToBytes   ( unk7,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( flen,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( version,          itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk1,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk2,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk3,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk4,             itwriteto );

            itwriteto = utils::WriteIntToBytes   ( year,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( month,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( day,              itwriteto );
            itwriteto = utils::WriteIntToBytes   ( hour,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( minute,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( second,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( centisec,         itwriteto );

            itwriteto = utils::WriteStrToByteContainer( itwriteto,        fname, fname.size() );

            itwriteto = utils::WriteIntToBytes   ( unk5,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk6,             itwriteto );

            return itwriteto;
        }


        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itPastEnd )
        {
            magicn      = utils::ReadIntFromBytes<decltype(magicn)>     ( itReadfrom, itPastEnd, false ); //iterator is incremented
            unk7        = utils::ReadIntFromBytes<decltype(unk7)>       ( itReadfrom, itPastEnd );
            flen        = utils::ReadIntFromBytes<decltype(flen)>       ( itReadfrom, itPastEnd );
            version     = utils::ReadIntFromBytes<decltype(version)>    ( itReadfrom, itPastEnd );
            unk1        = utils::ReadIntFromBytes<decltype(unk1)>       ( itReadfrom, itPastEnd );
            unk2        = utils::ReadIntFromBytes<decltype(unk2)>       ( itReadfrom, itPastEnd );
            unk3        = utils::ReadIntFromBytes<decltype(unk3)>       ( itReadfrom, itPastEnd );
            unk4        = utils::ReadIntFromBytes<decltype(unk4)>       ( itReadfrom, itPastEnd );
            year        = utils::ReadIntFromBytes<decltype(year)>       ( itReadfrom, itPastEnd );
            month       = utils::ReadIntFromBytes<decltype(month)>      ( itReadfrom, itPastEnd );
            day         = utils::ReadIntFromBytes<decltype(day)>        ( itReadfrom, itPastEnd );
            hour        = utils::ReadIntFromBytes<decltype(hour)>       ( itReadfrom, itPastEnd );
            minute      = utils::ReadIntFromBytes<decltype(minute)>     ( itReadfrom, itPastEnd );
            second      = utils::ReadIntFromBytes<decltype(second)>     ( itReadfrom, itPastEnd );
            centisec    = utils::ReadIntFromBytes<decltype(centisec)>   ( itReadfrom, itPastEnd );

            itReadfrom  = utils::ReadStrFromByteContainer( itReadfrom, fname.data(), FNameLen );

            unk5        = utils::ReadIntFromBytes<decltype(unk5)>       ( itReadfrom, itPastEnd );
            unk6        = utils::ReadIntFromBytes<decltype(unk6)>       ( itReadfrom, itPastEnd );

            return itReadfrom;
        }
    };
};

#endif