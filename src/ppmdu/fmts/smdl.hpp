#ifndef SMDL_HPP
#define SMDL_HPP
/*
smdl.hpp
2015/05/20
psycommando@gmail.com
Description: Utilities for handling Pokemon Mystery Dungeon: Explorers of Sky/Time/Darkness's .smd files.

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P

    #TODO: Move this to DSE folder + remove as many dependencies on PMD2 as possible !!!

*/
#include <dse/dse_common.hpp>
#include <dse/dse_containers.hpp>

#ifdef USE_PPMDU_CONTENT_TYPE_ANALYSER
    #include <types/content_type_analyser.hpp>
    namespace filetypes
    {
        extern const ContentTy CnTy_SMDL; //Content ID db handle
    };
#endif

namespace DSE
{

//====================================================================================================
//  Typedefs / Enums
//====================================================================================================
    static const uint32_t SMDL_MagicNumber = static_cast<uint32_t>(eDSEContainers::smdl); //0x736D646C; //"smdl"

//====================================================================================================
// Structs
//====================================================================================================

    /****************************************************************************************
        SMDL_Header
            The header of the SMDL file.
    ****************************************************************************************/
    struct SMDL_Header
    {
        static const uint32_t Size     = 52; //without padding
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
            itwriteto = utils::WriteIntToBytes   ( SMDL_MagicNumber, itwriteto, false ); //Write constant magic number, to avoid bad surprises
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
            _init ReadFromContainer(  _init itReadfrom )
        {
            magicn      = utils::ReadIntFromBytes<decltype(magicn)>     (itReadfrom, false ); //iterator is incremented
            unk7        = utils::ReadIntFromBytes<decltype(unk7)>       (itReadfrom);
            flen        = utils::ReadIntFromBytes<decltype(flen)>       (itReadfrom);
            version     = utils::ReadIntFromBytes<decltype(version)>    (itReadfrom);
            unk1        = utils::ReadIntFromBytes<decltype(unk1)>       (itReadfrom);
            unk2        = utils::ReadIntFromBytes<decltype(unk2)>       (itReadfrom);
            unk3        = utils::ReadIntFromBytes<decltype(unk3)>       (itReadfrom);
            unk4        = utils::ReadIntFromBytes<decltype(unk4)>       (itReadfrom);

            year        = utils::ReadIntFromBytes<decltype(year)>       (itReadfrom);
            month       = utils::ReadIntFromBytes<decltype(month)>      (itReadfrom);
            day         = utils::ReadIntFromBytes<decltype(day)>        (itReadfrom);
            hour        = utils::ReadIntFromBytes<decltype(hour)>       (itReadfrom);
            minute      = utils::ReadIntFromBytes<decltype(minute)>     (itReadfrom);
            second      = utils::ReadIntFromBytes<decltype(second)>     (itReadfrom);
            centisec    = utils::ReadIntFromBytes<decltype(centisec)>   (itReadfrom);

            itReadfrom  = utils::ReadStrFromByteContainer( itReadfrom, fname.data(), FNameLen );

            unk5        = utils::ReadIntFromBytes<decltype(unk5)>       (itReadfrom);
            unk6        = utils::ReadIntFromBytes<decltype(unk6)>       (itReadfrom);

            return itReadfrom;
        }
    };




//====================================================================================================
// Class
//====================================================================================================



//====================================================================================================
// Constants
//====================================================================================================



//====================================================================================================
// Functions
//====================================================================================================
    MusicSequence ParseSMDL( const std::string & file );
    void          WriteSMDL( const std::string & file, const MusicSequence & seq );

    MusicSequence ParseSMDL( std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend );

};

#endif