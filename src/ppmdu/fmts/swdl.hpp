#ifndef SWDL_HPP
#define SWDL_HPP
/*
swdl.hpp
2015/05/20
psycommando@gmail.com
Description: Utilities for handling Pokemon Mystery Dungeon: Explorers of Sky/Time/Darkness's .swd files.
*/
#include <ppmdu/pmd2/pmd2_audio_data.hpp>
#include <ppmdu/fmts/dse_common.hpp>
#include <ppmdu/utils/utility.hpp>
#include <cstdint>
#include <vector>
#include <array>
#include <string>


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
        uint16_t unk5            = 0;
        uint16_t unk6            = 0;
        uint8_t  unk7            = 0;
        uint8_t  unk8            = 0;
        uint16_t unk9            = 0;
        std::array<char,FNameLen> fname;
        uint32_t unk10           = 0;
        uint32_t unk11           = 0;
        uint32_t unk12           = 0;
        uint32_t unk13           = 0;
        uint32_t pcmdlen         = 0;
        uint16_t unk14           = 0;
        uint16_t nbwavislots     = 0;
        uint16_t unk16           = 0;
        uint16_t unk17           = 0;
        uint16_t wavilen         = 0;


        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToByteVector   ( SWDL_MagicNumber, itwriteto, false ); //Write constant magic number, to avoid bad surprises
            itwriteto = utils::WriteIntToByteVector   ( unk18,            itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( flen,             itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( version,          itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( unk1,             itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( unk2,             itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( unk3,             itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( unk4,             itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( unk5,             itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( unk6,             itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( unk7,             itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( unk8,             itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( unk9,             itwriteto );
            itwriteto = utils::WriteStrToByteContainer( itwriteto,        fname, fname.size() );
            itwriteto = utils::WriteIntToByteVector   ( unk10,            itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( unk11,            itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( unk12,            itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( unk13,            itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( pcmdlen,          itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( unk14,            itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( nbwavislots,      itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( unk16,            itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( unk17,            itwriteto );
            itwriteto = utils::WriteIntToByteVector   ( wavilen,          itwriteto );
            return itwriteto;
        }


        template<class _init>
            _init ReadFromContainer(  _init itReadfrom )
        {
            magicn      = utils::ReadIntFromByteVector<decltype(magicn)>     (itReadfrom, false ); //iterator is incremented
            unk18       = utils::ReadIntFromByteVector<decltype(unk18)>      (itReadfrom);
            flen        = utils::ReadIntFromByteVector<decltype(flen)>       (itReadfrom);
            version     = utils::ReadIntFromByteVector<decltype(version)>    (itReadfrom);
            unk1        = utils::ReadIntFromByteVector<decltype(unk1)>       (itReadfrom);
            unk2        = utils::ReadIntFromByteVector<decltype(unk2)>       (itReadfrom);
            unk3        = utils::ReadIntFromByteVector<decltype(unk3)>       (itReadfrom);
            unk4        = utils::ReadIntFromByteVector<decltype(unk4)>       (itReadfrom);
            unk5        = utils::ReadIntFromByteVector<decltype(unk5)>       (itReadfrom);
            unk6        = utils::ReadIntFromByteVector<decltype(unk6)>       (itReadfrom);
            unk7        = utils::ReadIntFromByteVector<decltype(unk7)>       (itReadfrom);
            unk8        = utils::ReadIntFromByteVector<decltype(unk8)>       (itReadfrom);
            unk9        = utils::ReadIntFromByteVector<decltype(unk9)>       (itReadfrom);
            itReadfrom  = utils::ReadStrFromByteContainer( itReadfrom, fname, FNameLen );
            unk10       = utils::ReadIntFromByteVector<decltype(unk10)>      (itReadfrom);
            unk11       = utils::ReadIntFromByteVector<decltype(unk11)>      (itReadfrom);
            unk12       = utils::ReadIntFromByteVector<decltype(unk12)>      (itReadfrom);
            unk13       = utils::ReadIntFromByteVector<decltype(unk13)>      (itReadfrom);
            pcmdlen     = utils::ReadIntFromByteVector<decltype(pcmdlen)>    (itReadfrom);
            unk14       = utils::ReadIntFromByteVector<decltype(unk14)>      (itReadfrom);
            nbwavislots = utils::ReadIntFromByteVector<decltype(nbwavislots)>(itReadfrom);
            unk16       = utils::ReadIntFromByteVector<decltype(unk16)>      (itReadfrom);
            unk17       = utils::ReadIntFromByteVector<decltype(unk17)>      (itReadfrom);
            wavilen     = utils::ReadIntFromByteVector<decltype(wavilen)>    (itReadfrom);
            return itReadfrom;
        }
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
    //    pmd2::audio::InstrumentBank m_instdata;


    //};

//====================================================================================================
// Functions
//====================================================================================================

    pmd2::audio::PresetBank ParseSWDL( const std::string & filename );
    void                    WriteSWDL( const std::string & filename, const pmd2::audio::PresetBank & audiodata );


};

#endif