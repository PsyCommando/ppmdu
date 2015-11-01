#ifndef DSE_COMMON_HPP
#define DSE_COMMON_HPP
/*
dse_common.hpp
2015/06/06
psycommando@gmail.com
Description: Common data between several of the Procyon Studio Digital Sound Element sound driver.
*/
//#include <ppmdu/pmd2/pmd2_audio_data.hpp>
#include <utils/utility.hpp>
#include <cstdint>
#include <ctime>
#include <vector>
#include <array>
#include <string>
#include <map>
#include <cassert>
#include <limits>

namespace DSE
{
//====================================================================================================
//  Typedefs
//====================================================================================================
    typedef uint16_t dsepresetid_t;
    typedef uint16_t bankid_t;
    typedef uint8_t  presetid_t;
    typedef uint8_t  midinote_t;

    const bankid_t      InvalidBankID      = USHRT_MAX;
    const presetid_t    InvalidPresetID    = UCHAR_MAX;
    const dsepresetid_t InvalidDSEPresetID = USHRT_MAX;

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
    const int                 NbMidiChannels  = 16;
    static const unsigned int NB_DSEChunks    = 11;
    static const uint32_t     SpecialChunkLen = 0xFFFFFFB0; //Value some special chunks have as their length

    extern const std::array<eDSEChunks, NB_DSEChunks> DSEChunksList; //Array containing all chunks labels

    //DSE Chunk ID stuff
    inline eDSEChunks IntToChunkID( uint32_t   value ); //Return eDSEChunks::invalid, if invalid ID !
    inline uint32_t   ChunkIDToInt( eDSEChunks id    );


    static const int16_t DSERootKey           = 60; //By default the root key for dse sequences is assumed to be 60 the MIDI standard's middle C, AKA C4
    const int8_t         DSEDefaultCoarseTune = -7; //The default coarse tune value for splits and samples.

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

        inline DateTime()
            :year(0), month(0), day(0), hour(0), minute(0), second(0), centsec(0)
        {}

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
        inline operator std::tm()const
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

        friend std::ostream & operator<<(std::ostream &os, const DateTime &obj );
    };


    /****************************************************************************************
        ChunkHeader
            Format for chunks used in Procyon's Digital Sound Element SWDL, SMDL, SEDL format, 
            used in PMD2.
    ****************************************************************************************/
    struct ChunkHeader
    {
        static const uint32_t Size          = 16; //Length of the header
        static const uint32_t OffsetDataLen = 12; //Offset from the start of the header where the length of the chunk is stored
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
            itwriteto = utils::WriteIntToBytes( label,  itwriteto, false );
            itwriteto = utils::WriteIntToBytes( param1, itwriteto );
            itwriteto = utils::WriteIntToBytes( param2, itwriteto );
            itwriteto = utils::WriteIntToBytes( datlen, itwriteto );
            return itwriteto;
        }

        //Read the structure from an iterator on a byte container
        template<class _init>
            _init ReadFromContainer(  _init itReadfrom )
        {
            label   = utils::ReadIntFromBytes<decltype(label)> (itReadfrom, false ); //iterator is incremented
            param1  = utils::ReadIntFromBytes<decltype(param1)>(itReadfrom);
            param2  = utils::ReadIntFromBytes<decltype(param2)>(itReadfrom);
            datlen  = utils::ReadIntFromBytes<decltype(datlen)>(itReadfrom);
            return itReadfrom;
        }
    };

    /************************************************************************
        SongChunk
            The raw song chunk.
            For some reasons, most of the data in this chunk rarely ever 
            changes in-between games or files.. Only the nb of channels and
            tracks does..
    ************************************************************************/
    struct SongChunk
    {
        static const uint32_t SizeNoPadd    = 48; //bytes
        static const uint32_t LenMaxPadding = 16; //bytes

        unsigned int size()const { return SizeNoPadd + unkpad.size(); }

        uint32_t label   = 0;
        uint32_t unk1    = 0;
        uint32_t unk2    = 0;
        uint32_t unk3    = 0;
        uint16_t unk4    = 0;
        uint16_t tpqn    = 0;
        uint16_t unk5    = 0;
        uint8_t  nbtrks  = 0;
        uint8_t  nbchans = 0;
        uint32_t unk6    = 0;
        uint32_t unk7    = 0;
        uint32_t unk8    = 0;
        uint32_t unk9    = 0;
        uint16_t unk10   = 0;
        uint16_t unk11   = 0;
        uint32_t unk12   = 0;
        std::vector<uint8_t> unkpad;

        //
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( static_cast<uint32_t>(eDSEChunks::song), itwriteto, false ); //Force this, to avoid bad surprises
            itwriteto = utils::WriteIntToBytes( unk1,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk2,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk3,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk4,    itwriteto );
            itwriteto = utils::WriteIntToBytes( tpqn,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk5,    itwriteto );
            itwriteto = utils::WriteIntToBytes( nbtrks,  itwriteto );
            itwriteto = utils::WriteIntToBytes( nbchans, itwriteto );
            itwriteto = utils::WriteIntToBytes( unk6,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk7,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk8,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk9,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk10,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unk11,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unk12,   itwriteto );
            itwriteto = std::copy( unkpad.begin(), unkpad.end(), itwriteto );
            return itwriteto;
        }

        //
        template<class _init>
            _init ReadFromContainer(  _init itReadfrom )
        {
            itReadfrom = utils::ReadIntFromBytes( label,    itReadfrom, false ); //iterator is incremented
            itReadfrom = utils::ReadIntFromBytes( unk1,     itReadfrom); 
            itReadfrom = utils::ReadIntFromBytes( unk2,     itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( unk3,     itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( unk4,     itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( tpqn,     itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( unk5,     itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( nbtrks,   itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( nbchans,  itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( unk6,     itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( unk7,     itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( unk8,     itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( unk9,     itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( unk10,    itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( unk11,    itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( unk12,    itReadfrom);

            for( uint32_t i = 0; i < LenMaxPadding; ++i, ++itReadfrom )
            {
                if( *itReadfrom == 0xFF )
                    unkpad.push_back( 0xFF ); //save on dereferencing the iterator when we already know its value..
                else
                    break;
            }

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
        enum struct eSmplFmt : uint16_t 
        {
            invalid,
            pcm8      = 0x000,
            pcm16     = 0x100,
            ima_adpcm = 0x200,
            psg       = 0x300,
        };

        uint16_t unk1       = 0;
        uint16_t id         = 0; //Index/ID of the sample
        int8_t   ftune      = 0; 
        int8_t   ctune      = 0;
        uint8_t  rootkey    = 0; //Possibly the MIDI key matching the pitch the sample was sampled at!
        int8_t   ktps       = 0; //Transpose
        int8_t   vol        = 0;
        int8_t   pan        = 0;
        uint8_t  unk5       = 0;
        uint8_t  unk58      = 0;
        uint16_t unk6       = 0;
        uint16_t unk7       = 0;
        uint16_t unk59      = 0; //
        uint16_t smplfmt    = 0; //Format of the sample 0x100 == PCM 16, 0x200 == IMA ADPCM
        uint8_t  unk9       = 0; 
        uint8_t  smplloop   = 0; //loop flag, 1 = loop, 0 = no loop
        uint8_t  unk10      = 0;
        uint8_t  unk60      = 0;
        uint8_t  unk11      = 0;
        uint8_t  unk61      = 0;
        uint8_t  unk12      = 0;
        uint8_t  unk62      = 0;
        uint32_t unk13      = 0;
        uint32_t smplrate   = 0; //Sampling rate of the sample
        uint32_t smplpos    = 0; //Offset within pcmd chunk of the sample
        uint32_t loopbeg    = 0; //Loop start in int32 (based on the resulting PCM16)
        uint32_t looplen    = 0; //Length of the sample in int32
        uint8_t  envon      = 0;
        uint8_t  envmult    = 0;
        uint8_t  unk19      = 0;
        uint8_t  unk20      = 0;
        uint16_t unk21      = 0;
        uint16_t unk22      = 0;
        int8_t   atkvol     = 0;
        int8_t   attack     = 0;
        int8_t   decay      = 0;
        int8_t   sustain    = 0;
        int8_t   hold       = 0;
        int8_t   decay2     = 0;
        int8_t   release    = 0;
        int8_t   unk57      = 0;

        //Write the structure using an iterator to a byte container
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( unk1,      itwriteto );
            itwriteto = utils::WriteIntToBytes( id,        itwriteto );
            itwriteto = utils::WriteIntToBytes( ftune,     itwriteto );
            itwriteto = utils::WriteIntToBytes( ctune,     itwriteto );
            itwriteto = utils::WriteIntToBytes( rootkey,   itwriteto );
            itwriteto = utils::WriteIntToBytes( ktps,      itwriteto );
            itwriteto = utils::WriteIntToBytes( vol,       itwriteto );
            itwriteto = utils::WriteIntToBytes( pan,       itwriteto );
            itwriteto = utils::WriteIntToBytes( unk5,      itwriteto );
            itwriteto = utils::WriteIntToBytes( unk58,     itwriteto );
            itwriteto = utils::WriteIntToBytes( unk6,      itwriteto );
            itwriteto = utils::WriteIntToBytes( unk7,      itwriteto );
            itwriteto = utils::WriteIntToBytes( unk59,     itwriteto );
            itwriteto = utils::WriteIntToBytes( smplfmt,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unk9,      itwriteto );
            itwriteto = utils::WriteIntToBytes( smplloop,  itwriteto );
            itwriteto = utils::WriteIntToBytes( unk10,     itwriteto );
            itwriteto = utils::WriteIntToBytes( unk60,     itwriteto );
            itwriteto = utils::WriteIntToBytes( unk11,     itwriteto );
            itwriteto = utils::WriteIntToBytes( unk61,     itwriteto );
            itwriteto = utils::WriteIntToBytes( unk12,     itwriteto );
            itwriteto = utils::WriteIntToBytes( unk62,     itwriteto );
            itwriteto = utils::WriteIntToBytes( unk13,     itwriteto );
            itwriteto = utils::WriteIntToBytes( smplrate,  itwriteto );
            itwriteto = utils::WriteIntToBytes( smplpos,   itwriteto );
            itwriteto = utils::WriteIntToBytes( loopbeg,   itwriteto );
            itwriteto = utils::WriteIntToBytes( looplen,   itwriteto );
            itwriteto = utils::WriteIntToBytes( envon,     itwriteto );
            itwriteto = utils::WriteIntToBytes( envmult,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unk19,     itwriteto );
            itwriteto = utils::WriteIntToBytes( unk20,     itwriteto );
            itwriteto = utils::WriteIntToBytes( unk21,     itwriteto );
            itwriteto = utils::WriteIntToBytes( unk22,     itwriteto );
            itwriteto = utils::WriteIntToBytes( atkvol,    itwriteto );
            itwriteto = utils::WriteIntToBytes( attack,    itwriteto );
            itwriteto = utils::WriteIntToBytes( decay,     itwriteto );
            itwriteto = utils::WriteIntToBytes( sustain,   itwriteto );
            itwriteto = utils::WriteIntToBytes( hold,      itwriteto );
            itwriteto = utils::WriteIntToBytes( decay2,    itwriteto );
            itwriteto = utils::WriteIntToBytes( release,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unk57,     itwriteto );
            return itwriteto;
        }

        //Read the structure from an iterator on a byte container
        template<class _init>
            _init ReadFromContainer( _init itReadfrom )
        {
            itReadfrom = utils::ReadIntFromBytes( unk1,     itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( id,       itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( ftune,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( ctune,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( rootkey,  itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( ktps,     itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( vol,      itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( pan,      itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk5,     itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk58,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk6,     itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk7,     itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk59,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( smplfmt,  itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk9,     itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( smplloop, itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk10,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk60,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk11,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk61,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk12,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk62,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk13,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( smplrate, itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( smplpos,  itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( loopbeg,  itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( looplen,  itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( envon,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( envmult,  itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk19,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk20,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk21,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk22,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( atkvol,   itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( attack,   itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( decay,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( sustain,  itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( hold,     itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( decay2,   itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( release,  itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk57,    itReadfrom );
            return itReadfrom;
        }
    };

    /*****************************************************************************************
        KeyGroups
            Contains several groups of notes each with an ID.
    *****************************************************************************************/
    struct KeyGroup
    {
        friend std::ostream & operator<<( std::ostream &  strm, const DSE::KeyGroup & other );

        static const uint32_t SIZE = 8; //bytes
        static uint32_t size() {return SIZE;}

        uint16_t id       = 0;
        uint8_t  poly     = 0;
        uint8_t  priority = 0;
        uint8_t  vclow    = 0;
        uint8_t  vchigh   = 0;
        uint8_t  unk50    = 0;
        uint8_t  unk51    = 0;

        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( id,       itwriteto );
            itwriteto = utils::WriteIntToBytes( poly,     itwriteto );
            itwriteto = utils::WriteIntToBytes( priority, itwriteto );
            itwriteto = utils::WriteIntToBytes( vclow,    itwriteto );
            itwriteto = utils::WriteIntToBytes( vchigh,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unk50,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk51,    itwriteto );
            return itwriteto;
        }


        template<class _init>
            _init ReadFromContainer( _init itReadfrom )
        {
            itReadfrom = utils::ReadIntFromBytes( id,       itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( poly,     itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( priority, itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( vclow,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( vchigh,   itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk50,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk51,    itReadfrom );
            return itReadfrom;
        }
    };

    /*****************************************************************************************
        ProgramInfo
            Contains data for a single instrument.
    *****************************************************************************************/
    class ProgramInfo
    {
    public:

        friend std::ostream & operator<<( std::ostream &  strm, const DSE::ProgramInfo & other );

        /*---------------------------------------------------------------------
            InstInfoHeader
                First 16 bytes of an instrument info block
        ---------------------------------------------------------------------*/
        struct InstInfoHeader
        {
            static const uint32_t SIZE = 16; //bytes
            static uint32_t size() { return SIZE; }

            uint16_t id        = 0;
            uint16_t nbsplits  = 0;
            uint8_t  insvol    = 0;
            uint8_t  inspan    = 0;
            uint16_t unk3      = 0;
            uint16_t unk4      = 0;
            uint8_t  unk5      = 0;
            uint8_t  nblfos    = 0; //Nb entries in the first table 
            uint8_t  padbyte   = 0; //character used for padding
            uint8_t  unk7      = 0;
            uint8_t  unk8      = 0;
            uint8_t  unk9      = 0;

            inline bool operator==( const InstInfoHeader & other )const
            {
                return ( id       == other.id       && 
                         nbsplits == other.nbsplits && 
                         insvol   == other.insvol   && 
                         inspan   == other.inspan   && 
                         unk3     == other.unk3     && 
                         unk4     == other.unk4     && 
                         unk5     == other.unk5     &&  
                         nblfos   == other.nblfos   &&
                         padbyte  == other.padbyte  &&
                         unk7     == other.unk7     &&
                         unk8     == other.unk8     &&
                         unk9     == other.unk9     );
            }

            inline bool operator!=( const InstInfoHeader & other )const
            {
                return !( operator==(other));
            }

            template<class _outit>
                _outit WriteToContainer( _outit itwriteto )const
            {
                itwriteto = utils::WriteIntToBytes( id,        itwriteto );
                itwriteto = utils::WriteIntToBytes( nbsplits,  itwriteto );
                itwriteto = utils::WriteIntToBytes( insvol,    itwriteto );
                itwriteto = utils::WriteIntToBytes( inspan,    itwriteto );
                itwriteto = utils::WriteIntToBytes( unk3,      itwriteto );
                itwriteto = utils::WriteIntToBytes( unk4,      itwriteto );
                itwriteto = utils::WriteIntToBytes( unk5,      itwriteto );
                itwriteto = utils::WriteIntToBytes( nblfos,    itwriteto );
                itwriteto = utils::WriteIntToBytes( padbyte,   itwriteto );
                itwriteto = utils::WriteIntToBytes( unk7,      itwriteto );
                itwriteto = utils::WriteIntToBytes( unk8,      itwriteto );
                itwriteto = utils::WriteIntToBytes( unk9,      itwriteto );
                return itwriteto;
            }


            template<class _init>
                _init ReadFromContainer( _init itReadfrom )
            {
                itReadfrom = utils::ReadIntFromBytes( id,        itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( nbsplits,  itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( insvol,    itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( inspan,    itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( unk3,      itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( unk4,      itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( unk5,      itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( nblfos,    itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( padbyte,   itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( unk7,      itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( unk8,      itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( unk9,      itReadfrom );
                return itReadfrom;
            }
        };

        /*---------------------------------------------------------------------
            LFOTblEntry
                First table after the header
        ---------------------------------------------------------------------*/
        struct LFOTblEntry
        {
            static const uint32_t SIZE = 16; //bytes
            static uint32_t size() { return SIZE; }

            enum struct eLFODest : uint8_t
            {
                None   = 0,
                Pitch  = 1,
                Volume = 2,
                Pan    = 3,
                UNK_4  = 4,
            };

            uint8_t  unk34  = 0;
            uint8_t  unk52  = 0;
            uint8_t  dest   = 0;
            uint8_t  wshape = 0;
            uint16_t rate   = 0;
            uint16_t unk29  = 0;
            uint16_t depth  = 0;
            uint16_t delay  = 0;
            uint16_t unk32  = 0;
            uint16_t unk33  = 0;

            template<class _outit>
                _outit WriteToContainer( _outit itwriteto )const
            {
                itwriteto = utils::WriteIntToBytes( unk34,  itwriteto );
                itwriteto = utils::WriteIntToBytes( unk52,  itwriteto );
                itwriteto = utils::WriteIntToBytes( dest,   itwriteto );
                itwriteto = utils::WriteIntToBytes( wshape, itwriteto );
                itwriteto = utils::WriteIntToBytes( rate,   itwriteto );
                itwriteto = utils::WriteIntToBytes( unk29,  itwriteto );
                itwriteto = utils::WriteIntToBytes( depth,  itwriteto );
                itwriteto = utils::WriteIntToBytes( delay,  itwriteto );
                itwriteto = utils::WriteIntToBytes( unk32,  itwriteto );
                itwriteto = utils::WriteIntToBytes( unk33,  itwriteto );
                return itwriteto;
            }


            template<class _init>
                _init ReadFromContainer( _init itReadfrom )
            {
                itReadfrom = utils::ReadIntFromBytes( unk34,  itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( unk52,  itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( dest,   itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( wshape, itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( rate,   itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( unk29,  itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( depth,  itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( delay,  itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( unk32,  itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( unk33,  itReadfrom );
                return itReadfrom;
            }
        };

        /*---------------------------------------------------------------------
            SplitEntry
                Data on a particular sample mapped to this instrument
        ---------------------------------------------------------------------*/
        struct SplitEntry
        {
            static const uint32_t SIZE = 48; //bytes
            static uint32_t size() { return SIZE; }

            uint8_t  unk10    = 0; //0x0
            uint8_t  id       = 0; //0x1
            uint8_t  unk11    = 0; //0x2
            uint8_t  unk25    = 0; //0x3

            int8_t   lowkey   = 0; //0x4
            int8_t   hikey    = 0; //0x5

            int8_t   lowkey2  = 0; //0x6
            int8_t   hikey2   = 0; //0x7
            int8_t   lovel    = 0; //0x8
            int8_t   hivel    = 0; //0x9
            int8_t   lovel2   = 0; //0xA
            int8_t   hivel2   = 0; //0xB

            uint32_t unk16    = 0; //0xC
            uint16_t unk17    = 0; //0x10
            uint16_t smplid   = 0; //0x12

            int8_t  ftune     = 0; //0x14
            int8_t  ctune     = 0; //0x15

            int8_t  rootkey   = 0; //0x16
            int8_t  ktps      = 0; //0x17

            uint8_t smplvol   = 0; //0x18
            uint8_t smplpan   = 0; //0x19

            uint8_t  kgrpid   = 0; //0x1A
            uint8_t  unk22    = 0; //0x1B
            uint16_t unk23    = 0; //0x1C
            uint16_t unk24    = 0; //0x1E
            //The last 16 bytes are a perfect copy of the last 16 bytes of a wavi info block
            uint8_t  envon    = 0; //0x20 
            uint8_t  envmult  = 0; //0x21 //Multiplier for other envelope params
            uint8_t  unk37    = 0; //0x22
            uint8_t  unk38    = 0; //0x23
            uint16_t unk39    = 0; //0x24
            int16_t  unk40    = 0; //0x26

            int8_t   atkvol   = 0; //0x28
            int8_t   attack   = 0; //0x29

            int8_t   decay    = 0; //0x2A
            int8_t   sustain  = 0; //0x2B

            int8_t   hold     = 0; //0x2C
            int8_t   decay2   = 0; //0x2D

            int8_t   release  = 0; //0x2E
            int8_t   rx       = 0; //0x2F

            template<class _outit>
                _outit WriteToContainer( _outit itwriteto )const
            {
                itwriteto = utils::WriteIntToBytes( unk10,    itwriteto );
                itwriteto = utils::WriteIntToBytes( id,       itwriteto );
                itwriteto = utils::WriteIntToBytes( unk11,    itwriteto );
                itwriteto = utils::WriteIntToBytes( unk25,    itwriteto );

                itwriteto = utils::WriteIntToBytes( lowkey,   itwriteto );
                itwriteto = utils::WriteIntToBytes( hikey,    itwriteto );
                itwriteto = utils::WriteIntToBytes( lowkey2,  itwriteto );
                itwriteto = utils::WriteIntToBytes( hikey2,   itwriteto );
                itwriteto = utils::WriteIntToBytes( lovel,    itwriteto );
                itwriteto = utils::WriteIntToBytes( hivel,    itwriteto );
                itwriteto = utils::WriteIntToBytes( lovel2,   itwriteto );
                itwriteto = utils::WriteIntToBytes( hivel2,   itwriteto );

                itwriteto = utils::WriteIntToBytes( unk16,    itwriteto );
                itwriteto = utils::WriteIntToBytes( unk17,    itwriteto );
                itwriteto = utils::WriteIntToBytes( smplid,   itwriteto );

                itwriteto = utils::WriteIntToBytes( ftune,     itwriteto );
                itwriteto = utils::WriteIntToBytes( ctune,   itwriteto );

                itwriteto = utils::WriteIntToBytes( rootkey,  itwriteto );
                itwriteto = utils::WriteIntToBytes( ktps,    itwriteto );

                itwriteto = utils::WriteIntToBytes( smplvol,  itwriteto );
                itwriteto = utils::WriteIntToBytes( smplpan,  itwriteto );

                itwriteto = utils::WriteIntToBytes( kgrpid,   itwriteto );
                itwriteto = utils::WriteIntToBytes( unk22,    itwriteto );
                itwriteto = utils::WriteIntToBytes( unk23,    itwriteto );
                itwriteto = utils::WriteIntToBytes( unk24,    itwriteto );

                itwriteto = utils::WriteIntToBytes( envon,    itwriteto );
                itwriteto = utils::WriteIntToBytes( envmult,  itwriteto );
                itwriteto = utils::WriteIntToBytes( unk37,    itwriteto );
                itwriteto = utils::WriteIntToBytes( unk38,    itwriteto );
                itwriteto = utils::WriteIntToBytes( unk39,    itwriteto );
                itwriteto = utils::WriteIntToBytes( unk40,    itwriteto );

                itwriteto = utils::WriteIntToBytes( atkvol,   itwriteto );
                itwriteto = utils::WriteIntToBytes( attack,   itwriteto );

                itwriteto = utils::WriteIntToBytes( decay,    itwriteto );
                itwriteto = utils::WriteIntToBytes( sustain,  itwriteto );

                itwriteto = utils::WriteIntToBytes( hold,     itwriteto );
                itwriteto = utils::WriteIntToBytes( decay2,   itwriteto );

                itwriteto = utils::WriteIntToBytes( release,  itwriteto );
                itwriteto = utils::WriteIntToBytes( rx,       itwriteto );
                return itwriteto;
            }


            template<class _init>
                _init ReadFromContainer( _init itReadfrom )
            {
                itReadfrom = utils::ReadIntFromBytes( unk10,    itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( id,       itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( unk11,    itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( unk25,    itReadfrom );

                itReadfrom = utils::ReadIntFromBytes( lowkey,   itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( hikey,    itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( lowkey2,  itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( hikey2,   itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( lovel,    itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( hivel,    itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( lovel2,   itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( hivel2,   itReadfrom );

                itReadfrom = utils::ReadIntFromBytes( unk16,    itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( unk17,    itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( smplid,   itReadfrom );

                itReadfrom = utils::ReadIntFromBytes( ftune,     itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( ctune,   itReadfrom );

                itReadfrom = utils::ReadIntFromBytes( rootkey,  itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( ktps,    itReadfrom );

                itReadfrom = utils::ReadIntFromBytes( smplvol,  itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( smplpan,  itReadfrom );

                itReadfrom = utils::ReadIntFromBytes( kgrpid,   itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( unk22,    itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( unk23,    itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( unk24,    itReadfrom );

                itReadfrom = utils::ReadIntFromBytes( envon,    itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( envmult,  itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( unk37,    itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( unk38,    itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( unk39,    itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( unk40,    itReadfrom );

                itReadfrom = utils::ReadIntFromBytes( atkvol,   itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( attack,   itReadfrom );

                itReadfrom = utils::ReadIntFromBytes( decay,    itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( sustain,  itReadfrom );

                itReadfrom = utils::ReadIntFromBytes( hold,     itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( decay2,   itReadfrom );

                itReadfrom = utils::ReadIntFromBytes( release,  itReadfrom );
                itReadfrom = utils::ReadIntFromBytes( rx,       itReadfrom );

                return itReadfrom;
            }
        };

        //----------------------------
        //  ProgramInfo
        //----------------------------
        ProgramInfo()
        {}

        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = m_hdr.WriteToContainer(itwriteto);

            for( const auto & entry : m_lfotbl )
                itwriteto = entry.WriteToContainer(itwriteto);

            //16 bytes of padding
            itwriteto = std::fill_n( itwriteto, 16, m_hdr.padbyte );

            for( const auto & smpl : m_splitstbl )
                itwriteto = smpl.WriteToContainer(itwriteto);

            return itwriteto;
        }


        template<class _init>
            _init ReadFromContainer( _init itReadfrom )
        {
            itReadfrom = m_hdr.ReadFromContainer(itReadfrom);

            m_lfotbl   .resize(m_hdr.nblfos);
            m_splitstbl.resize(m_hdr.nbsplits);

            for( auto & entry : m_lfotbl )
                itReadfrom = entry.ReadFromContainer(itReadfrom);

            //16 bytes of padding
            std::advance( itReadfrom, 16 );

            for( auto & smpl : m_splitstbl )
                itReadfrom = smpl.ReadFromContainer(itReadfrom);

            return itReadfrom;
        }

        enum struct eCompareRes
        {
            different,
            sharesamples,
            identical,
        };

        eCompareRes isSimilar( const ProgramInfo & other )const
        {
            //
            size_t  nbmatchsmpls   = 0;

            //Test shared samples first (We don't care about slight variations in the samples parameters)
            for( const auto & asmpl : m_splitstbl )
            {
                auto found = std::find_if( other.m_splitstbl.begin(), 
                                           other.m_splitstbl.end(), 
                                           [&asmpl](const SplitEntry& entry){ return (entry.smplid == asmpl.smplid); } );
                
                if( found != other.m_splitstbl.end() )
                    ++nbmatchsmpls;
            }

            if( nbmatchsmpls != std::max( m_splitstbl.size(), m_splitstbl.size() ) )
                return eCompareRes::sharesamples;
            else if( nbmatchsmpls == 0 )
                return eCompareRes::different;

            //Test header and prestbl for saying the whole thing is identical
            if( m_hdr == other.m_hdr )
                return eCompareRes::identical;
            else
                return eCompareRes::different;
        }

    /*private:*/
        InstInfoHeader           m_hdr;
        std::vector<LFOTblEntry> m_lfotbl;
        std::vector<SplitEntry>  m_splitstbl;
    };


    /****************************************************************************************
        DSEEnvelope
            Represents a DSE envelope used in the SWDL file format!
    ****************************************************************************************/
    struct DSEEnvelope
    {
        typedef int16_t timeprop_t;
        typedef int8_t  volprop_t;

        DSEEnvelope();

        //Build from a split entry
        DSEEnvelope            ( const ProgramInfo::SplitEntry & splitentry );
        DSEEnvelope & operator=( const ProgramInfo::SplitEntry & splitentry );

        int8_t     envmulti;        //The envelope multiplier
        
        volprop_t  atkvol;
        timeprop_t attack;
        timeprop_t hold;
        timeprop_t decay;
        volprop_t  sustain;
        timeprop_t decay2;
        timeprop_t release;
    };


//  DSEMetaData
    /************************************************************************
        DSE_MetaData
            Header data contained in all DSE files.
    ************************************************************************/
    struct DSE_MetaData
    {
        DSE_MetaData()
            :unk1(0),unk2(0),createtime()
        {}

        uint8_t     unk1;       //Some kind of ID
        uint8_t     unk2;       //Some kind of volume value maybe
        std::string fname;      //Internal filename
        DateTime    createtime; //Time this was created on
    };

    /************************************************************************
        DSE_MetaDataSMDL
            Header data specific to the DSE SMD format.
    ************************************************************************/
    struct DSE_MetaDataSMDL : public DSE_MetaData
    {
        DSE_MetaDataSMDL()
            :DSE_MetaData(), tpqn(0)
        {}

        uint16_t tpqn; //ticks per quarter note
        //More to be added
    };

    /************************************************************************
        DSE_MetaDataSWDL
            
    ************************************************************************/
    struct DSE_MetaDataSWDL : public DSE_MetaData
    {
        DSE_MetaDataSWDL()
            :DSE_MetaData(), nbwavislots(0), nbprgislots(0)
        {}

        uint16_t nbwavislots;
        uint16_t nbprgislots;
    };

//====================================================================================================
// Class
//====================================================================================================

//====================================================================================================
// Conversion Functions
//====================================================================================================

    /*
        This converts the pitch value used for samples pitch correction in SWDL files, into semitones.
        #TODO: Confirm this is correct.
    */
    //inline int16_t DSESamplePitchToSemitone( int16_t dsesmplpitch )
    //{
    //    static const int16_t NbUnitPerSemitone = 250;
    //    return ( dsesmplpitch / NbUnitPerSemitone );
    //}

    /*
        This converts the pitch value used for samples pitch correction in SWDL files, into cents(1/100th of a semitone).
        #TODO: Confirm this is correct.
    */
    //inline int16_t DSESamplePitchToCents( int16_t dsesmplpitch )
    //{
    //    //static const double NbUnitPerSemitone = 250.0;
    //    //double result = ( static_cast<double>(dsesmplpitch) / NbUnitPerSemitone ) * 100.0;
    //    //return static_cast<int16_t>( lround(result) );
    //    return dsesmplpitch;
    //    //return static_cast<int16_t>( lround( (static_cast<double>(dsesmplpitch) / 2.5) ) );
    //}

    /*
        This converts the pitch value used for pitch bend events (0xD7) into semitones.
        #TODO: Fix
    */
    //static int16_t DSEPitchBendToSemitone( int16_t dsepitchbend )
    //{
    //    assert(false); //pitch bend range can be changed in the swd !!! So this doesn't work!
    //    static const int16_t NbUnitPerSemitone = 500;
    //    return ( dsepitchbend / NbUnitPerSemitone );
    //}

    /*
        This converts the pitch value used for pitch bend events (0xD7) into cents(1/100th of a semitone).
    */
    //static int16_t DSEPitchBendToCents( int16_t dsepitchbend )
    //{
    //    
    //    static const double NbUnitPerSemitone = 500.0;
    //    double result = ( static_cast<double>(dsepitchbend) / NbUnitPerSemitone ) * 100.0;
    //    return static_cast<int16_t>( lround(result) );
    //}

    /*
    */
    //inline int8_t DseCtuneToSemitones( int8_t ctune )
    //{
    //    return ctune;

    //    //Since -7 is basically no pitch shift, we need to work around that..
    //    //if( ctune != DSEDefaultCoarseTune )
    //    //{
    //    //    if( ctune < 0 )
    //    //    {
    //    //        abs(abs(ctune) - abs(DSEDefaultCoarseTune));
    //    //        return ctune + (DSEDefaultCoarseTune * -1);
    //    //    }
    //    //    else
    //    //        return ctune + (DSEDefaultCoarseTune * -1);
    //    //}
    //    //else
    //    //    return 0;
    //}

    /*
        DSEEnveloppeDurationToMSec
            This turns an envelope's duration parameter(0-127) into miliseconds.

            The multiplier is usually 1
    */
    int32_t DSEEnveloppeDurationToMSec( int8_t param, int8_t multiplier );

    /*
        DSEEnveloppeVolumeTocB
            Turns an envelope's volume param(0-127) into centibels.
    */
    int32_t DSEEnveloppeVolumeTocB( int8_t param );

    //--------------------
    //  Lengths
    //--------------------

    /*
        DSESampleOffsetToBytes
            Sample loop position and loop lenght are stored in nb of 32 bits integers. Not in bytes or samples.
            This functions handles the conversion.
    */
    inline uint32_t DSESampleLoopOffsetToBytes( uint32_t loopoffset )
    {
        return (loopoffset * sizeof(uint32_t));
    }

//====================================================================================================
// Utility Functions
//====================================================================================================

    /************************************************************************
        DSE_ChunkIDLookup
            This singleton's "Find" static method returns the first chunk id 
            whose's highest byte matches the specified byte.
    ************************************************************************/
    class DSE_ChunkIDLookup
    {
    public:
        static std::vector<eDSEChunks> Find( uint8_t highbyte )
        {
            static DSE_ChunkIDLookup s_instance; //creates it when first called
            auto lambdaFId = [&highbyte]( const std::pair<uint8_t,eDSEChunks>& val )->bool
            { 
                return (val.first == highbyte); 
            };

            std::vector<eDSEChunks> possiblematches;

            for( auto itfound = s_instance.m_lutbl.find(highbyte);  //Search once, if no match will not loop once.
                 itfound != s_instance.m_lutbl.end(); 
                 itfound = std::find_if( ++itfound, s_instance.m_lutbl.end(), lambdaFId ) ) //If we had a match search again, one slot after our last match
            {
                if( itfound->second != eDSEChunks::invalid )
                    possiblematches.push_back(itfound->second);
            }

            return std::move( possiblematches );
        }

    private:
        //Build the quick lookup table
        DSE_ChunkIDLookup()
        {
            for( eDSEChunks id : DSEChunksList )
                m_lutbl.insert( std::make_pair( static_cast<uint8_t>((static_cast<uint32_t>(id) >> 24) & 0xFF) , id ) ); //Isolate highest byte
        }

        //No copy, no move
        DSE_ChunkIDLookup( const DSE_ChunkIDLookup & );
        DSE_ChunkIDLookup( DSE_ChunkIDLookup && );
        DSE_ChunkIDLookup & operator=( const DSE_ChunkIDLookup & );
        DSE_ChunkIDLookup & operator=( DSE_ChunkIDLookup && );

        std::map<uint8_t, eDSEChunks> m_lutbl;
    };

    /************************************************************************
        FindNextChunk
            Find the start of the next chunk that has the specified chunk id.

            If the chunk is not found, the function returns "end".

            NOTE: "beg" must be aligned on 4 bytes!
    ************************************************************************/
    template<class _init>
        _init FindNextChunk( _init beg, _init end, eDSEChunks chnkid )
    {
        //search
        while( beg != end ) 
        {
            //check if we possibly are at the beginning of a chunk, looking for its highest byte.
            vector<eDSEChunks> possibleid = std::move( DSE_ChunkIDLookup::Find( *beg ) ); 
            size_t             skipsize = 4; //Default byte skip size on each loop (The NDS makes 4 bytes aligned reads)

            //Check all found results
            for( auto & potential : possibleid )
            {
                //Check if its really the chunk's header start, or just a coincidence
                uint32_t actualid = utils::ReadIntFromBytes<uint32_t>( _init(beg), false ); //Make a copy of beg, to avoid it being incremented

                if( actualid == static_cast<uint32_t>(chnkid) ) //Check if we match the chunk we're looking for
                    return beg;
                else if( actualid == static_cast<uint32_t>(potential) ) //If it actually matches another chunk's id
                {
                    _init backup = beg;

                    //Read the chunk's size and skip if possible
                    std::advance( beg, ChunkHeader::OffsetDataLen );
                    uint32_t chnksz = utils::ReadIntFromBytes<uint32_t>(beg);

                    if( chnksz != DSE::SpecialChunkLen ) //Some chunks have an invalid length that is equal to this value.
                    {
                        //Then attempt to skip
                        //try
                        //{
                            skipsize = chnksz;  //We have to do this like so, because some chunks may use bogus sizes 
                                                // for some mindblowingly odd reasons.. It shouldn't happen too often though..
                        //}
                        //catch(std::exception &)
                        //{
                        //    beg = backup; //Restore iterator to last valid state if fails
                        //}
                    }
                    else
                        skipsize = 0; //otherwise, just continue on, without further incrementing the iterator

                    break; //After all this we know its an actual chunk, just kick us out of the loop, as other matches don't matter anymore!
                }
            }

            //Skip the required ammount of bytes
            if( skipsize != 4 )
                std::advance( beg, skipsize ); //Advance is much faster, and here we're certain we're not supposed to go out of bounds.
            else
                for( int cnt = 0; cnt < 4 && beg != end; ++cnt, ++beg ); //SMDL files chunk headers are always 4 bytes aligned
        }

        //Return Result
        return beg;
    }

};

#endif