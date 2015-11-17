#ifndef SWDL_HPP
#define SWDL_HPP
/*
swdl.hpp
2015/05/20
psycommando@gmail.com
Description: Utilities for handling Pokemon Mystery Dungeon: Explorers of Sky/Time/Darkness's .swd files.

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P
*/
//#include <ppmdu/pmd2/pmd2_audio_data.hpp>
#include <dse/dse_common.hpp>
#include <dse/dse_containers.hpp>
#include <utils/utility.hpp>
#include <cstdint>
#include <vector>
#include <array>
#include <string>
#include <types/content_type_analyser.hpp>

namespace filetypes
{
    extern const ContentTy CnTy_SWDL; //Content ID db handle 
};


namespace DSE
{
//====================================================================================================
//  Typedefs
//====================================================================================================

//====================================================================================================
//  Constants
//====================================================================================================
    static const uint32_t SWDL_MagicNumber = 0x7377646C; //"swdl"

//====================================================================================================
// Structs
//====================================================================================================
    
    /****************************************************************************************
        SWDL_Header
            The header of the SWDL file.
    ****************************************************************************************/
    struct SWDL_Header
    {
        static const uint32_t Size     = 80;
        static const uint32_t FNameLen = 16;

        static unsigned int size() { return Size; }

        uint32_t magicn          = 0;
        uint32_t unk18           = 0;
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
        uint32_t unk10           = 0;
        uint32_t unk11           = 0;

        uint32_t unk12           = 0;
        uint32_t unk13           = 0;
        uint32_t pcmdlen         = 0;
        uint16_t unk14           = 0;
        uint16_t nbwavislots     = 0;
        uint16_t nbprgislots     = 0;
        uint16_t unk17           = 0;
        uint16_t wavilen         = 0;


        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes   ( SWDL_MagicNumber, itwriteto, false ); //Write constant magic number, to avoid bad surprises
            itwriteto = utils::WriteIntToBytes   ( unk18,            itwriteto );
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

            itwriteto = utils::WriteIntToBytes   ( unk10,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk11,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk12,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk13,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( pcmdlen,          itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk14,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( nbwavislots,      itwriteto );
            itwriteto = utils::WriteIntToBytes   ( nbprgislots,      itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk17,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( wavilen,          itwriteto );
            return itwriteto;
        }


        template<class _init>
            _init ReadFromContainer(  _init itReadfrom )
        {
            magicn      = utils::ReadIntFromBytes<decltype(magicn)>     (itReadfrom, false ); //iterator is incremented
            unk18       = utils::ReadIntFromBytes<decltype(unk18)>      (itReadfrom);
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

            unk10       = utils::ReadIntFromBytes<decltype(unk10)>      (itReadfrom);
            unk11       = utils::ReadIntFromBytes<decltype(unk11)>      (itReadfrom);
            unk12       = utils::ReadIntFromBytes<decltype(unk12)>      (itReadfrom);
            unk13       = utils::ReadIntFromBytes<decltype(unk13)>      (itReadfrom);
            pcmdlen     = utils::ReadIntFromBytes<decltype(pcmdlen)>    (itReadfrom);
            unk14       = utils::ReadIntFromBytes<decltype(unk14)>      (itReadfrom);
            nbwavislots = utils::ReadIntFromBytes<decltype(nbwavislots)>(itReadfrom);
            nbprgislots = utils::ReadIntFromBytes<decltype(nbprgislots)>(itReadfrom);
            unk17       = utils::ReadIntFromBytes<decltype(unk17)>      (itReadfrom);
            wavilen     = utils::ReadIntFromBytes<decltype(wavilen)>    (itReadfrom);
            return itReadfrom;
        }


        /*
            DoesContainsSamples
                Returns true if the swdl contains sample data.
        */
        bool DoesContainsSamples()const;

        /*
            IsSampleBankOnly
                Returns true if the swdl is only a sample bank, without program info.
        */
        bool IsSampleBankOnly()const;
    };





//====================================================================================================
// Class
//====================================================================================================



    /*
        Represents the possible content of a swdl file.
        Just an additional layer over the swdl loaded to determine if 
    */
    //class SWDL_Content
    //{
    //public:

    //    SWDL_Content()
    //    {
    //    }

    //    /*
    //        Details on the SWDL file
    //    */
    //    bool ContainsSamples()const;

    //    /*
    //        Convert to generic format
    //    */
    //    operator pmd2::audio::PresetBank();


    //private:
    //    pmd2::audio::SampleBank     m_audiodata;
    //    pmd2::audio::ProgramBank m_instdata;


    //};

//====================================================================================================
// Functions
//====================================================================================================

    PresetBank ParseSWDL( const std::string & filename );
    void       WriteSWDL( const std::string & filename, const PresetBank & audiodata );

    /*
        ReadSwdlHeader
            Reads only the SWDL header from a file.
    */
    SWDL_Header ReadSwdlHeader( const std::string & filename );

};

#endif