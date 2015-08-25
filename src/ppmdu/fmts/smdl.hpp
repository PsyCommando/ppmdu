#ifndef SMDL_HPP
#define SMDL_HPP
/*
smdl.hpp
2015/05/20
psycommando@gmail.com
Description: Utilities for handling Pokemon Mystery Dungeon: Explorers of Sky/Time/Darkness's .smd files.

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P
*/
#include <dse/dse_common.hpp>
#include <dse/dse_containers.hpp>

//Forward declare #TODO: do something less ugly dependency-wise !
namespace pmd2 { namespace audio { class MusicSequence; }; };

namespace DSE
{

//====================================================================================================
//  Typedefs / Enums
//====================================================================================================
    static const uint32_t SMDL_MagicNumber = 0x736D646C; //"smdl"

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
            itwriteto = utils::WriteIntToByteVector   ( SMDL_MagicNumber, itwriteto, false ); //Write constant magic number, to avoid bad surprises
            itwriteto = utils::WriteIntToByteVector   ( unk7,             itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( flen,             itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( version,          itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( unk1,             itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( unk2,             itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( unk3,             itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( unk4,             itwriteto );

            itwriteto = utils::WriteIntToByteVector   ( year,             itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( month,            itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( day,              itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( hour,             itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( minute,           itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( second,           itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( centisec,         itwriteto );

            itwriteto = utils::WriteStrToByteContainer( itwriteto,        fname, fname.size() );

            itwriteto = utils::WriteIntToByteVector   ( unk5,             itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( unk6,             itwriteto );

            return itwriteto;
        }


        template<class _init>
            _init ReadFromContainer(  _init itReadfrom )
        {
            magicn      = utils::ReadIntFromByteVector<decltype(magicn)>     (itReadfrom, false ); //iterator is incremented
            unk7        = utils::ReadIntFromByteVector<decltype(unk7)>       (itReadfrom);
            flen        = utils::ReadIntFromByteVector<decltype(flen)>       (itReadfrom);
            version     = utils::ReadIntFromByteVector<decltype(version)>    (itReadfrom);
            unk1        = utils::ReadIntFromByteVector<decltype(unk1)>       (itReadfrom);
            unk2        = utils::ReadIntFromByteVector<decltype(unk2)>       (itReadfrom);
            unk3        = utils::ReadIntFromByteVector<decltype(unk3)>       (itReadfrom);
            unk4        = utils::ReadIntFromByteVector<decltype(unk4)>       (itReadfrom);

            year        = utils::ReadIntFromByteVector<decltype(year)>       (itReadfrom);
            month       = utils::ReadIntFromByteVector<decltype(month)>      (itReadfrom);
            day         = utils::ReadIntFromByteVector<decltype(day)>        (itReadfrom);
            hour        = utils::ReadIntFromByteVector<decltype(hour)>       (itReadfrom);
            minute      = utils::ReadIntFromByteVector<decltype(minute)>     (itReadfrom);
            second      = utils::ReadIntFromByteVector<decltype(second)>     (itReadfrom);
            centisec    = utils::ReadIntFromByteVector<decltype(centisec)>   (itReadfrom);

            itReadfrom  = utils::ReadStrFromByteContainer( itReadfrom, fname.data(), FNameLen );

            unk5        = utils::ReadIntFromByteVector<decltype(unk5)>       (itReadfrom);
            unk6        = utils::ReadIntFromByteVector<decltype(unk6)>       (itReadfrom);

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

};

#endif