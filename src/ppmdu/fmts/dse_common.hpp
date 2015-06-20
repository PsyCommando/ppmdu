#ifndef DSE_COMMON_HPP
#define DSE_COMMON_HPP
/*
dse_common.hpp
2015/06/06
psycommando@gmail.com
Description: Common data between several of the Procyon Studio Digital Sound Element sound driver.
*/
#include <ppmdu/pmd2/pmd2_audio_data.hpp>
#include <ppmdu/utils/utility.hpp>
#include <cstdint>
#include <ctime>
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

    //Enum containing the IDs of all chunks used by the Procyon DSE system for sequenced music and sound effects
    enum struct eDSEChunks : uint32_t
    {
        invalid,
        wavi = 0x77617669, //"wavi"
        prgi = 0x70726769, //"prgi"
        kgrp = 0x6B677270, //"kgrp"
        pcmd = 0x70636D64, //"pcmd"
        song = 0x736F6E67, //"song"
        trk  = 0x74726B20, //"trk\0x20"
        seq  = 0x73657120, //"seq\0x20"
        bnkl = 0x626E6B6C, //"bnkl"
        mcrl = 0x6D63726C, //"mcrl"
        eoc  = 0x656F6320, //"eoc\0x20"
        eod  = 0x656F6420, //"eod\0x20"
    };
    static const unsigned int NB_DSEChunks    = 11;
    static const uint32_t     SpecialChunkLen = 0xFFFFFFB0; //Value some special chunks have as their length

    //DSE Chunk ID stuff
    inline eDSEChunks IntToChunkID( uint32_t   value ); //Return eDSEChunks::invalid, if invalid ID !
    inline uint32_t   ChunkIDToInt( eDSEChunks id    );


//====================================================================================================
// Structs
//====================================================================================================

    /****************************************************************************************
        DateTime
            Format used to store a date + time stamp used by all DSE formats.
    ****************************************************************************************/
    struct DateTime
    {
        uint16_t year;
        uint8_t  month;
        uint8_t  day;
        uint8_t  hour;
        uint8_t  minute;
        uint8_t  second;
        uint8_t  centsec; //100th of a second ? We don't really know what this is for..

        inline DateTime( const std::tm & src )
        {
            //http://en.cppreference.com/w/cpp/chrono/c/tm
            year    = src.tm_year + 1900; //tm_year counts the nb of years since 1900
            month   = src.tm_mon;
            day     = src.tm_mday-1;      //tm_mday begins at 1, while the time in the DSE timestamp begins at 0!
            hour    = src.tm_hour;
            minute  = src.tm_min;
            second  = (src.tm_sec == 60)? 59 : src.tm_sec; //We're not dealing with leap seconds...
        }

        //Convert into the standard std::tm calendar time format 
        inline operator std::tm()
        {
            std::tm result;
            result.tm_year  = year - 1900;
            result.tm_mon   = month;
            result.tm_mday  = day + 1;
            result.tm_hour  = hour;
            result.tm_min   = minute;
            result.tm_sec   = second;
            result.tm_isdst = -1; //No info available
            return std::move(result);
        }
    };


    /****************************************************************************************
        ChunkHeader
            Format for chunks used in Procyon's Digital Sound Element SWDL, SMDL, SEDL format, 
            used in PMD2.
    ****************************************************************************************/
    struct ChunkHeader
    {
        static const uint32_t Size = 16; //Length of the header
        uint32_t label  = 0;
        uint32_t param1 = 0;
        uint32_t param2 = 0;
        uint32_t datlen = 0;

        static unsigned int size      ()      { return Size; } //Get the size of the structure in bytes
        bool                hasLength ()const { return (datlen != SpecialChunkLen); } //Returns whether this chunk has a valid data length
        eDSEChunks          GetChunkID()const { return IntToChunkID( label ); } //Returns the enum value representing this chunk's identity, judging from the label

        //Write the structure using an iterator to a byte container
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToByteVector( label,  itwriteto, false );
            itwriteto = utils::WriteIntToByteVector( param1, itwriteto );
            itwriteto = utils::WriteIntToByteVector( param2, itwriteto );
            itwriteto = utils::WriteIntToByteVector( datlen, itwriteto );
            return itwriteto;
        }

        //Read the structure from an iterator on a byte container
        template<class _init>
            _init ReadFromContainer(  _init itReadfrom )
        {
            label   = utils::ReadIntFromByteVector<decltype(label)> (itReadfrom, false ); //iterator is incremented
            param1  = utils::ReadIntFromByteVector<decltype(param1)>(itReadfrom);
            param2  = utils::ReadIntFromByteVector<decltype(param2)>(itReadfrom);
            datlen  = utils::ReadIntFromByteVector<decltype(datlen)>(itReadfrom);
            return itReadfrom;
        }
    };

    /****************************************************************************************
        WavInfo
            Entry from the "wavi" chunk in a swdl file.
    ****************************************************************************************/
    struct WavInfo
    {
        static const uint32_t Size = 64;

        uint16_t unk1       = 0;
        uint16_t id         = 0; //Index/ID of the sample
        uint16_t unk2       = 0;
        uint16_t unk3       = 0;
        uint16_t unk4       = 0;
        uint16_t unk5       = 0;
        uint16_t unk6       = 0;
        uint16_t unk7       = 0;
        uint16_t version    = 0; //
        uint16_t smplfmt    = 0; //Format of the sample 0x100 == PCM 16, 0x200 == IMA ADPCM
        uint8_t  unk9       = 0; 
        uint8_t  unk14      = 0;
        uint16_t unk10      = 0;
        uint16_t unk11      = 0;
        uint16_t unk12      = 0;
        uint32_t unk13      = 0;
        uint32_t smplrate   = 0; //Sampling rate of the sample
        uint32_t smplpos    = 0; //Offset within pcmd chunk of the sample
        uint32_t loopspos   = 0; //Position in sample, of the beginning of the loop 
        uint32_t looplen    = 0; //Length in number of samples of the loop
        uint8_t  unk17      = 0;
        uint8_t  unk18      = 0;
        uint8_t  unk19      = 0;
        uint8_t  unk20      = 0;
        uint16_t unk21      = 0;
        uint16_t unk22      = 0;
        uint16_t unk23      = 0;
        uint16_t unk24      = 0;
        uint16_t unk25      = 0;
        uint16_t unk26      = 0;
    };

    /************************************************************************
        TrkEvent
            Represent a raw track event used in the SEDL and SMDL format!
    ************************************************************************/
    struct TrkEvent
    {
        uint8_t dt     = 0;
        uint8_t evcode = 0;
        uint8_t param1 = 0;
        uint8_t param2 = 0;
    };

    /*
        DSE_MetaData
            Leftover game-specific data from parsing a DSE file format.
    */
    struct DSE_MetaData
    {
        uint8_t     unk1;       //Some kind of ID
        uint8_t     unk2;       //Some kind of volume value maybe
        std::string fname;      //Internal filename
        DateTime    createtime; //Time this was created on
    };

//====================================================================================================
// Class
//====================================================================================================

//====================================================================================================
// Functions
//====================================================================================================

};

#endif