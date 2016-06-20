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
#include <iostream>

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
    const uint8_t       InvalidMIDIKey     = UCHAR_MAX; //The default value for the MIDI key

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

    enum struct eDSEContainers : uint32_t
    {
        invalid,
        smdl = 0x736D646C,  //"smdl"
        swdl = 0x7377646C,  //"swdl"
        sedl = 0x7365646C,  //"sedl"
        sadl = 0x7361646C,  //"sadl"
    };
    std::ostream & operator<<( std::ostream &  strm, const DSE::eDSEContainers cnty );

    const int                 NbMidiChannels        = 16;
    const unsigned int        NB_DSEChannels        = 17;
    const unsigned int        NB_DSETracks          = 17;
    static const unsigned int NB_DSEChunks          = 11;
    static const unsigned int NB_DSEContainers      = 4;
    static const uint32_t     SpecialChunkLen       = 0xFFFFFFB0;   //Value some special chunks have as their length
    static const int16_t      DSERootKey            = 60;           //By default the root key for dse sequences is assumed to be 60 the MIDI standard's middle C, AKA C4
    const int8_t              DSEDefaultCoarseTune  = -7;           //The default coarse tune value for splits and samples.

    extern const std::array<eDSEChunks,     NB_DSEChunks>      DSEChunksList;    //Array containing all chunks labels
    extern const std::array<eDSEContainers, NB_DSEContainers>  DSEContainerList; //Array containing all the DSE container's magic number.

    //
    //
    //
    enum struct eDSESmplFmt : uint16_t 
    {
        invalid   = SHRT_MAX,
        pcm8      = 0x000,
        pcm16     = 0x100,
        ima_adpcm = 0x200,
        psg       = 0x300, //Probably not PSG!
    };

    inline eDSESmplFmt IntToDSESmplFmt( uint16_t val )
    {
        if( val == static_cast<uint16_t>(eDSESmplFmt::pcm8) )
            return eDSESmplFmt::pcm8;
        else if( val == static_cast<uint16_t>(eDSESmplFmt::pcm16) )
            return eDSESmplFmt::pcm16;
        else if( val == static_cast<uint16_t>(eDSESmplFmt::ima_adpcm) )
            return eDSESmplFmt::ima_adpcm;
        else if( val == static_cast<uint16_t>(eDSESmplFmt::psg) )
            return eDSESmplFmt::psg;
        else
            return eDSESmplFmt::invalid;
    }


    // -------------------------------
    // ------- DSE Version IDs -------
    // -------------------------------
    enum struct eDSEVersion : uint16_t
    {
        V402 = 0x402,   //Used in Luminous Arc
        V415 = 0x415,   //Used in Pokemon Mystery Dungeon2, Zombi Daisuki, Inayusha Eleven, Professor Layton, etc..
        VDef = V415,
        VInvalid = 0,
    };

    inline eDSEVersion intToDseVer( uint16_t val )
    {
        if( val == static_cast<uint16_t>(eDSEVersion::V402) )
            return eDSEVersion::V402;
        if( val == static_cast<uint16_t>(eDSEVersion::V415) )
            return eDSEVersion::V415;
        else
            return eDSEVersion::VInvalid;
    }

    inline uint16_t DseVerToInt( eDSEVersion ver )
    {
        return static_cast<uint16_t>(ver);
    }
    
    // -------------------------------
    // ----- DSE Chunk ID stuff ------
    // -------------------------------
    //inline eDSEChunks IntToChunkID( uint32_t   value ); //Return eDSEChunks::invalid, if invalid ID !
    //inline uint32_t   ChunkIDToInt( eDSEChunks id    );
    inline eDSEChunks IntToChunkID( uint32_t value )
    {
        for( auto cid : DSEChunksList )
        {
            if( value == static_cast<uint32_t>(cid) )
                return cid;
        }
        return eDSEChunks::invalid;
    }
    
    inline uint32_t ChunkIDToInt( eDSEChunks id )
    {
        return static_cast<uint32_t>(id);
    }

    //DSE Magic Number
    inline eDSEContainers IntToContainerMagicNum( uint32_t value )  //Return eDSEContainers::invalid, if invalid ID !
    {
        for( auto cid : DSEContainerList )
        {
            if( value == static_cast<uint32_t>(cid) )
                return cid;
        }
        return eDSEContainers::invalid;
    }

    inline uint32_t ContainerMagicNumToInt( eDSEContainers magicn )
    {
        return static_cast<uint32_t>(magicn);
    }

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

        inline void SetTimeToNow()
        {
            std::time_t t  = std::time(nullptr);
            std::tm     ti;// = *std::localtime(&t);
            if( ! localtime_s(&ti, &t) )
                std::clog << "<!>- DSE::DateTime::SetTimeToNow(): localtime_s returned an error while getting the current date!\n";
            //http://en.cppreference.com/w/cpp/chrono/c/tm
            year    = ti.tm_year + 1900; //tm_year counts the nb of years since 1900
            month   = ti.tm_mon;
            day     = ti.tm_mday-1;      //tm_mday begins at 1, while the time in the DSE timestamp begins at 0!
            hour    = ti.tm_hour;
            minute  = ti.tm_min;
            second  = (ti.tm_sec == 60)? 59 : ti.tm_sec; //We're not dealing with leap seconds...
        }

        friend std::ostream & operator<<(std::ostream &os, const DateTime &obj );
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

        int8_t     envmulti;        //The envelope multiplier
        
        volprop_t  atkvol;
        timeprop_t attack;
        timeprop_t hold;
        timeprop_t decay;
        volprop_t  sustain;
        timeprop_t decay2;
        timeprop_t release;
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
            _init ReadFromContainer(  _init itReadfrom, _init itpastend )
        {
            label   = utils::ReadIntFromBytes<decltype(label)> (itReadfrom, itpastend, false ); //iterator is incremented
            param1  = utils::ReadIntFromBytes<decltype(param1)>(itReadfrom, itpastend);
            param2  = utils::ReadIntFromBytes<decltype(param2)>(itReadfrom, itpastend);
            datlen  = utils::ReadIntFromBytes<decltype(datlen)>(itReadfrom, itpastend);
            return itReadfrom;
        }
    };

    /************************************************************************
        SongData
            A generic version of the DSE Song Chunk meant to contain only 
            the relevant data and work with any version of DSE!
    ************************************************************************/
    struct SongData
    {
        uint16_t tpqn;
        uint8_t  nbtrks;
        uint8_t  nbchans;

        // v0x402 specifics
        int8_t   mainvol;
        int8_t   mainpan;
    };

    /****************************************************************************************
        WavInfo
            Entry from the "wavi" chunk in a swdl file.
    ****************************************************************************************/
    
    /*
        This holds DSE version independent information on samples.
    */
    struct WavInfo
    {
        uint16_t    id         = 0;
        int8_t      ftune      = 0; 
        int8_t      ctune      = 0;
        uint8_t     rootkey    = 0;
        int8_t      ktps       = 0;
        int8_t      vol        = 0;
        int8_t      pan        = 0;
        eDSESmplFmt smplfmt    = eDSESmplFmt::invalid;
        bool        smplloop   = false;
        uint32_t    smplrate   = 0; //Sampling rate of the sample
        uint32_t    smplpos    = 0;
        uint32_t    loopbeg    = 0; //Loop start in int32 (based on the resulting PCM16)
        uint32_t    looplen    = 0; //Length of the sample in int32
        //Envelope
        uint8_t     envon      = 0;
        uint8_t     envmult    = 0;
        int8_t      atkvol     = 0;
        int8_t      attack     = 0;
        int8_t      decay      = 0;
        int8_t      sustain    = 0;
        int8_t      hold       = 0;
        int8_t      decay2     = 0;
        int8_t      release    = 0;
    };



    /*****************************************************************************************
        KeyGroup
            Contains info on note-stealing priority and voice range, polyphony, and maybe more.
    *****************************************************************************************/
    struct KeyGroup
    {
        friend std::ostream & operator<<( std::ostream &  strm, const DSE::KeyGroup & other );

        static const uint32_t SIZE     = 8; //bytes
        static const uint8_t  DefPoly  = 0xFF;
        static const uint8_t  DefPrio  = 0x08;
        static const uint8_t  DefVcHi  = 0xF;
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
            _init ReadFromContainer( _init itReadfrom, _init itpastend )
        {
            itReadfrom = utils::ReadIntFromBytes( id,       itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( poly,     itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( priority, itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( vclow,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( vchigh,   itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk50,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk51,    itReadfrom, itpastend );
            return itReadfrom;
        }
    };


    /*---------------------------------------------------------------------
        LFOTblEntry
            Contains details on to pass the LFOs during the playback of 
            a program's samples.
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

        /*
            Return true if the LFO's fields have non-default values.
        */
        inline bool isLFONonDefault()const
        {
            return ( unk52 != 0 && dest != 0 && wshape != 1 && rate != 0 && 
                        unk29 != 0 && depth != 0 && delay != 0 && unk32 != 0 && 
                        unk33 != 0 );
        }

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
            _init ReadFromContainer( _init itReadfrom, _init itpastend )
        {
            itReadfrom = utils::ReadIntFromBytes( unk34,  itReadfrom, itpastend  );
            itReadfrom = utils::ReadIntFromBytes( unk52,  itReadfrom, itpastend  );
            itReadfrom = utils::ReadIntFromBytes( dest,   itReadfrom, itpastend  );
            itReadfrom = utils::ReadIntFromBytes( wshape, itReadfrom, itpastend  );
            itReadfrom = utils::ReadIntFromBytes( rate,   itReadfrom, itpastend  );
            itReadfrom = utils::ReadIntFromBytes( unk29,  itReadfrom, itpastend  );
            itReadfrom = utils::ReadIntFromBytes( depth,  itReadfrom, itpastend  );
            itReadfrom = utils::ReadIntFromBytes( delay,  itReadfrom, itpastend  );
            itReadfrom = utils::ReadIntFromBytes( unk32,  itReadfrom, itpastend  );
            itReadfrom = utils::ReadIntFromBytes( unk33,  itReadfrom, itpastend  );
            return itReadfrom;
        }
    };

    /*---------------------------------------------------------------------
        SplitEntry
            Data on a particular sample mapped to this instrument
    ---------------------------------------------------------------------*/
    struct SplitEntry
    {
        uint8_t     id        = 0; //0x1
        uint8_t     unk11     = 0; //0x2
        uint8_t     unk25     = 0; //0x3
        int8_t      lowkey    = 0; //0x4
        int8_t      hikey     = 0; //0x5
        int8_t      lowkey2   = 0; //0x6
        int8_t      hikey2    = 0; //0x7
        int8_t      lovel     = 0; //0x8
        int8_t      hivel     = 0; //0x9
        int8_t      lovel2    = 0; //0xA
        int8_t      hivel2    = 0; //0xB
        uint16_t    smplid    = 0; //0x12
        int8_t      ftune     = 0; //0x14
        int8_t      ctune     = 0; //0x15
        int8_t      rootkey   = 0; //0x16
        int8_t      ktps      = 0; //0x17
        uint8_t     smplvol   = 0; //0x18
        uint8_t     smplpan   = 0; //0x19
        uint8_t     kgrpid    = 0; //0x1A
        uint8_t     envon     = 0; //0x20
        DSEEnvelope env;
    };

    /*****************************************************************************************
        ProgramInfo
            Contains data for a single instrument.
            This is a generic version independent version of the program info structure.
    *****************************************************************************************/
    class ProgramInfo
    {
    public:
        friend std::ostream & operator<<( std::ostream &  strm, const DSE::ProgramInfo & other );

        /*---------------------------------------------------------------------
            Program info header stuff
        ---------------------------------------------------------------------*/
        uint16_t id        = 0;
        uint8_t  prgvol    = 0;
        uint8_t  prgpan    = 0;
        uint8_t  unkpoly   = 0;
        uint8_t  unk4      = 0;
        uint8_t  padbyte   = 0;

        std::vector<LFOTblEntry> m_lfotbl;
        std::vector<SplitEntry>  m_splitstbl;
    };








//  DSEMetaData
    /************************************************************************
        DSE_MetaData
            Header data contained in all DSE files.
    ************************************************************************/
    struct DSE_MetaData
    {
        DSE_MetaData()
            :unk1(0),unk2(0),createtime(), origversion(eDSEVersion::VDef)
        {}

        uint8_t     unk1;       //Some kind of ID
        uint8_t     unk2;       //Some kind of volume value maybe
        std::string fname;      //Internal filename
        DateTime    createtime; //Time this was created on
        std::string origfname;  //The original filename, in the game's filesystem if applicable
        eDSEVersion origversion;
    };

    /************************************************************************
        DSE_MetaDataSMDL
            Header data specific to the DSE SMD format.
    ************************************************************************/
    struct DSE_MetaDataSMDL : public DSE_MetaData
    {
        DSE_MetaDataSMDL()
            :DSE_MetaData(), tpqn(0), mainvol(127), mainpan(64) //#TODO: Replace with constants!
        {}

        uint16_t tpqn; //ticks per quarter note
        //More to be added
        //v0x402 specifics
        int8_t      mainvol;
        int8_t      mainpan;
    };

    /************************************************************************
        DSE_MetaDataSWDL
            
    ************************************************************************/
    struct DSE_MetaDataSWDL : public DSE_MetaData
    {
        DSE_MetaDataSWDL()
            :DSE_MetaData(), nbwavislots(0), nbprgislots(0),unk17(0)
        {}

        uint16_t nbwavislots;
        uint16_t nbprgislots;
        uint16_t unk17;
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
                uint32_t actualid = utils::ReadIntFromBytes<uint32_t>( _init(beg), end, false ); //Make a copy of beg, to avoid it being incremented

                if( actualid == static_cast<uint32_t>(chnkid) ) //Check if we match the chunk we're looking for
                    return beg;
                else if( actualid == static_cast<uint32_t>(potential) ) //If it actually matches another chunk's id
                {
                    _init backup = beg;

                    //Read the chunk's size and skip if possible
                    std::advance( beg, ChunkHeader::OffsetDataLen );
                    uint32_t chnksz = utils::ReadIntFromBytes<uint32_t>(beg, end);

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


    /*
        FindEndChunk
        

            Use this to locate the proper end chunk depending on the DSE container type!
    */
    template<class _init>
        _init FindEndChunk( _init beg, _init end, eDSEContainers cnty )
    {
        switch(cnty)
        {
            case eDSEContainers::smdl:
            {
                beg = FindNextChunk( beg, end, eDSEChunks::eoc );
                break;
            }
            case eDSEContainers::swdl:
            case eDSEContainers::sedl:
            {
                beg = FindNextChunk( beg, end, eDSEChunks::eod );
                break;
            }
            case eDSEContainers::sadl:
            {
                std::cerr << "<!>- Error : FindEndChunk() : SADL doesn't have a end chunk!\n";
                assert(false);
            }
            default:
            {
                std::cerr << "<!>- Error : FindEndChunk() : Invalid DSE container type!\n";
                assert(false);
            }
        };

        return beg;
    }

};


#endif