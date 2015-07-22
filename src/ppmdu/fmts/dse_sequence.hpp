#ifndef DSE_SEQUENCE_HPP
#define DSE_SEQUENCE_HPP
/*
dse_sequence.hpp
2015/06/26
psycommando@gmail.com
Description: Contains utilities to deal with DSE event tracks. Or anything within a "trk\20" chunk.
*/
#include <ppmdu/fmts/dse_common.hpp>
#include <map>
#include <array>
#include <cstdint>

namespace DSE
{
//====================================================================================================
//  Typedefs / Enums / Constants
//====================================================================================================

    //Default tick rate of the Digital Sound Element Sound Driver for sequence playback.
    static const uint16_t     DefaultTickRte   = 48; 
    static const unsigned int NbTrkDelayValues = 16; //The nb of track delay prefixes in the DSE format


//  Track Events Specifics Constants

    /****************************************************************************
        eTrkDelays
            The tick values of each of the 16 possible delay prefix for events.
    ****************************************************************************/
    enum struct eTrkDelays : uint8_t
    {
        //whole         = 192, //Not referred in the system
        _half           = 96, // 1/2 note
        _dotqtr         = 72, // dotted 1/4 note
        _two3rdsofahalf = 64, // (1/2 note)/3 * 2
        _qtr            = 48, // 1/4 note
        _dot8th         = 36, // dotted 1/8 note
        _two3rdsofqtr   = 32, // (1/4 note)/3 * 2
        _8th            = 24, // 1/8 note
        _dot16th        = 18, // dotted 1/16 note
        _two3rdsof8th   = 16, // (1/8 note)/3 * 2
        _16th           = 12, // 1/16 note
        _dot32nd        =  9, // dotted 1/32
        _two3rdsof16th  =  8, // (1/16 note)/3 * 2
        _32nd           =  6, // 1/32 note
        _dot64th        =  4, // dotted 1/64 note
        _two3rdsof32th  =  3, // (1/32 note)/3 * 2  #NOTE: Its not truly 2/3 of a 32th note, because the game seems to round the duration, so its not identical to the last one.
        _64th           =  2, // 1/64 note          #NOTE: Its not truly a 64th note, because the game seems to round the duration, so its not identical to the last one.
    };

    /****************************************************************************
        TrkDelayCodeVals
            Lookup table for the value of the delay prefix codes used by events 
            in a track chunk.
    ****************************************************************************/
    static const std::map<uint8_t, eTrkDelays> TrkDelayCodeVals
    {{
        { 0x80, eTrkDelays::_half           },
        { 0x81, eTrkDelays::_dotqtr         },
        { 0x82, eTrkDelays::_two3rdsofahalf },
        { 0x83, eTrkDelays::_qtr            },
        { 0x84, eTrkDelays::_dot8th         },
        { 0x85, eTrkDelays::_two3rdsofqtr   },
        { 0x86, eTrkDelays::_8th            },
        { 0x87, eTrkDelays::_dot16th        },
        { 0x88, eTrkDelays::_two3rdsof8th   },
        { 0x89, eTrkDelays::_16th           },
        { 0x8A, eTrkDelays::_dot32nd        },
        { 0x8B, eTrkDelays::_two3rdsof16th  },
        { 0x8C, eTrkDelays::_32nd           },
        { 0x8D, eTrkDelays::_dot64th        },
        { 0x8E, eTrkDelays::_two3rdsof32th  },
        { 0x8F, eTrkDelays::_64th           },
    }};


    /****************************************************************************
        TickDelayToTrkDelayCodes
            The track delay values from eTrkDelays, but in an map to 
            facilitate getting the corred DSE track Delay code from 
            a delay in ticks!
    ****************************************************************************/
    static const std::array<uint8_t, NbTrkDelayValues> TickDelayToTrkDelayCodes
    {{
        static_cast<uint8_t>(eTrkDelays::_half),
        static_cast<uint8_t>(eTrkDelays::_dotqtr),
        static_cast<uint8_t>(eTrkDelays::_two3rdsofahalf),

        static_cast<uint8_t>(eTrkDelays::_qtr),
        static_cast<uint8_t>(eTrkDelays::_dot8th),
        static_cast<uint8_t>(eTrkDelays::_two3rdsofqtr),

        static_cast<uint8_t>(eTrkDelays::_8th),
        static_cast<uint8_t>(eTrkDelays::_dot16th),
        static_cast<uint8_t>(eTrkDelays::_two3rdsof8th),

        static_cast<uint8_t>(eTrkDelays::_16th),
        static_cast<uint8_t>(eTrkDelays::_dot32nd),
        static_cast<uint8_t>(eTrkDelays::_two3rdsof16th),

        static_cast<uint8_t>(eTrkDelays::_32nd),
        static_cast<uint8_t>(eTrkDelays::_dot64th),
        static_cast<uint8_t>(eTrkDelays::_two3rdsof32th),

        static_cast<uint8_t>(eTrkDelays::_64th),
    }};
    //static const std::map<uint8_t, uint8_t> TickDelayToTrkDelayCodes
    //{{
    //    { static_cast<uint8_t>(eTrkDelays::_half),            0x80 },
    //    { static_cast<uint8_t>(eTrkDelays::_dotqtr),          0x81 },
    //    { static_cast<uint8_t>(eTrkDelays::_two3rdsofahalf),  0x82 },

    //    { static_cast<uint8_t>(eTrkDelays::_qtr),             0x83 },
    //    { static_cast<uint8_t>(eTrkDelays::_dot8th),          0x84 },
    //    { static_cast<uint8_t>(eTrkDelays::_two3rdsofqtr),    0x85 },

    //    { static_cast<uint8_t>(eTrkDelays::_8th),             0x86 },
    //    { static_cast<uint8_t>(eTrkDelays::_dot16th),         0x87 },
    //    { static_cast<uint8_t>(eTrkDelays::_two3rdsof8th),    0x88 },

    //    { static_cast<uint8_t>(eTrkDelays::_16th),            0x89 },
    //    { static_cast<uint8_t>(eTrkDelays::_dot32nd),         0x8A },
    //    { static_cast<uint8_t>(eTrkDelays::_two3rdsof16th),   0x8B },

    //    { static_cast<uint8_t>(eTrkDelays::_32nd),            0x8C },
    //    { static_cast<uint8_t>(eTrkDelays::_dot64th),         0x8D },
    //    { static_cast<uint8_t>(eTrkDelays::_two3rdsof32th),   0x8E },

    //    { static_cast<uint8_t>(eTrkDelays::_64th),            0x8F },
    //}};

    static uint8_t FindClosestTrkDelayCode( uint8_t delayticks )
    {
        for( size_t i = 0; i < TickDelayToTrkDelayCodes.size(); ++i )
        {
            if( delayticks == TickDelayToTrkDelayCodes[i] ) //Exact match
            {
                return TickDelayToTrkDelayCodes[i];
            }
            else if( (i + 1) < (TickDelayToTrkDelayCodes.size()-1) )
            {
                //Check if the next value is smaller than the delay. If it is, we can't get a value any closer to "delayticks".
                if( delayticks > TickDelayToTrkDelayCodes[i+1] )
                {
                    //Compare this value and the next and see which one we're closest to
                    uint8_t diff = TickDelayToTrkDelayCodes[i] - TickDelayToTrkDelayCodes[i+1];

                    if( delayticks < (diff/2) )
                        return TickDelayToTrkDelayCodes[i+1]; //The closest value in this case is the next one
                    else
                        return TickDelayToTrkDelayCodes[i];   //The closest value in this case is the current one
                }
            }
        }

        //If all else fails, return the last!
        return TickDelayToTrkDelayCodes.back();
    }


    //Nb of track events in the event enum
    //static const uint32_t NB_Track_Events = 18;

    /************************************************************************
        eTrkEventCodes
            Codes for each different events stored in a smdl track chunk.
    ************************************************************************/
    enum struct eTrkEventCodes : uint8_t
    {
        Invalid         = 0x00,

        //Reserved range for NoteOn + Velocity
        NoteOnBeg       = 0x01, //The first event code reserved for play note events.
        NoteOnEnd       = 0x7F, //The last event code that is reserved for playing a note.

        //Reserved range for discriminating against delays prefixes
        DelayBeg        = 0x80, //The first value reserved for Delay prefixes
        DelayEnd        = 0x8F, //The last value reserved for Delay prefixes

        //Non-play note events
        RepeatLastPause = 0x90, //Repeat the last silence
        AddToLastPause  = 0x91, //Pause the track for the duration of the last pause + the duration specified
        Pause           = 0x92, //Pause the track for specified duration (uses a uint8)
        LongPause       = 0x93, //Pause the track for specified duration (uses a uint16)

        EndOfTrack      = 0x98, //Marks the end of the track. Also serve as padding.
        LoopPointSet    = 0x99, //Marks the location where the track should loop from.

        Unk_0x9C        = 0x9C, //Unknown

        SetOctave       = 0xA0, //Sets the octave notes are currently played at.

        SetTempo        = 0xA4, //Sets the tempo of the track in BPM.
        Unk_0xA8        = 0xA8, //Unknown purpose
        SetUnk1         = 0xA9, //Set that first unknown value from the track's header
        SetUnk2         = 0xAA, //Set that second unknown value from the track's header

        SetPreset       = 0xAC, //Sets the instrument preset to use

        Unk_0xB2        = 0xB2, //Unknown

        Unk_0xB4        = 0xB4, //Unknown
        Unk_0xB5        = 0xB5, //Unknown

        Unk_0xBE        = 0xBE, //Possibly set modulation ?
        Unk_0xBF        = 0xBF, //Unknown

        HoldNote        = 0xC0, //Holds the last note indefinitely until another note is played

        Unk_0xCB        = 0xCB, //Holds the last note indefinitely until another note is played

        Unk_0xD0        = 0xD0, //Unknown
        Unk_0xD1        = 0xD1, //Unknown
        Unk_0xD2        = 0xD2, //Unknown

        Unk_0xD4        = 0xD4, //Unknown

        Unk_0xD6        = 0xD6, //Unkown
        PitchBend       = 0xD7, //Pitch bending/modulation/LFO. Not 100% certain.
        Unk_0xDB        = 0xDB, //Unknown purpose. Used in bgmM0000.smd
        Unk_0xDC        = 0xDC, //Unknown

        SetTrkVol       = 0xE0, //Sets primary track volume.

        Unk_0xE2        = 0xE2, //Unknown
        SetExpress      = 0xE3, //Sets secondary volume control. AKA expression or GM CC#11.

        SetTrkPan       = 0xE8, //Sets the panning of the track.

        Unk_0xEA        = 0xEA, //Unknonw

        Unk_0xEC        = 0xEC, //Unknown

        Unk_0xF6        = 0xF6, //Unknown
    };

    /****************************************************************************
        eNote
            Values indicating each notes that can be represented in a NoteOn 
            event.
    ****************************************************************************/
    enum struct eNote : uint8_t
    {
        C  = 0x0,
        Cs = 0x1,
        D  = 0x2,
        Ds = 0x3,
        E  = 0x4,
        F  = 0x5,
        Fs = 0x6,
        G  = 0x7,
        Gs = 0x8,
        A  = 0x9,
        As = 0xA,
        B  = 0xB,
        nbNotes, //Must be last
    };
    
    /****************************************************************************
        NoteNames
            A textual representation of each note.
    ****************************************************************************/
    static const std::array<std::string, static_cast<uint8_t>(eNote::nbNotes)> NoteNames
    {{
        "C",
        "C#",
        "D",
        "D#",
        "E",
        "F",
        "F#",
        "G",
        "G#",
        "A",
        "A#",
        "B",
    }};

    /****************************************************************************
        eNotePitch
            Values indicating how the play note event deal with the track's current 
            pitch.
    ****************************************************************************/
    enum struct eNotePitch : uint8_t
    {
        reset     = 0x00,
        lower     = 0x10,
        current   = 0x20,
        higher    = 0x30,
    };



//Bitmasks
    static const uint8_t NoteEvParam1NoteMask     = 0x0F; //( 0000 1111 ) The id of the note "eDSENote" is stored in the lower nybble
    static const uint8_t NoteEvParam1PitchMask    = 0x30; //( 0011 0000 ) The value of those 2 bits in the "param1" of a NoteOn event indicate if/how to modify the track's current pitch.
    static const uint8_t NoteEvParam1NbParamsMask = 0xC0; //( 1100 0000 ) The value of those two bits indicates the amount of bytes to be parsed as parameters for the note on event

    static const uint8_t EventDelayMask           = 0xF0; // Apply this to get the part where the delay code is
    static const uint8_t EventDelayCode           = 0x80; // Compare the result after applying the above mask to this to know if its a prefixed delay

    /************************************************************************
        TrkEventsTable
            Contains details specifics on how to parse all event codes.
    ************************************************************************/
    struct TrkEventInfo;
    extern const std::vector<TrkEventInfo/*, NB_Track_Events*/> TrkEventsTable;

//====================================================================================================
// Structs
//====================================================================================================

    /************************************************************************
        TrkEventInfo
            Contains details specifics on how to parse an event codes.

            **Used only for the event lookup table!**
    ************************************************************************/
    struct TrkEventInfo
    {
        //Event code range
        eTrkEventCodes evcodebeg;   //Beginning of the range of event codes that can be used to represent this event.
        eTrkEventCodes evcodeend;   //Leave to invalid when is event with single code

        //nb params
        uint32_t       nbreqparams; //if has any required parameters
        bool           hasoptparam; //if has optional param 
        bool           isEoT;       //if is end of track marker
        bool           isLoopPoint; //if is loop point
        std::string    evlbl;       //text label for the event
    };

    /************************************************************************
        GetEventInfo
            If boolean in pair is true, the event code in the second member
            of the pair is the correct TrkEventInfo.
            Otherwise, if the boolean is false, the second member is invalid.
    ************************************************************************/
    std::pair<bool,TrkEventInfo> GetEventInfo( eTrkEventCodes ev );

    //What do we want to do with our events ?
    // 1. Get details to parse them
    // 2. Get details on how to interpret them
    // 3. Get details on how to write them
    //What do we have to determine the event type ?
    // The event code.

    //So we want something that can give us a polymorphic event object by passing event codes,
    // and something we can pass an event polymorphic object to get a raw event from. Or a midi event.

    /*
        BaseTrkEvent

    */
    //class TrkEvent
    //{
    //public:

    //    void toMIDI()const;
    //    void tostring()const;

    //    unsigned int getNbReqBytes()const;
    //    unsigned int getNbOptBytes( uint8_t evcode, const std::vector<uint8_t> & reqbytes )const;
    //};

//====================================================================================================
// Track Data
//====================================================================================================

    /************************************************************************
        TrkEvent
            Represent a raw track event used in the SEDL and SMDL format!
    ************************************************************************/
    struct TrkEvent
    {
        uint8_t dt     = 0;
        uint8_t evcode = 0;

        std::vector<uint8_t> params;

        std::string tostr()const;
    };

    /************************************************************************
        TrkPreamble
            First 4 bytes of data of a trk chunk. Contains track-specific
            info such as its trackID and its MIDI channel number.
    ************************************************************************/
    struct TrkPreamble
    {
        static const uint32_t Size = 4; //bytes

        static uint32_t size(){return Size;}

        uint8_t trkid  = 0;
        uint8_t chanid = 0;
        uint8_t unk1   = 0;
        uint8_t unk2   = 0;

        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToByteVector( trkid,  itwriteto );
            itwriteto = utils::WriteIntToByteVector( chanid, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk1,   itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk2,   itwriteto );
            return itwriteto;
        }

        template<class _init>
            _init ReadFromContainer(  _init itReadfrom )
        {
            trkid  = utils::ReadIntFromByteVector<decltype(trkid)> (itReadfrom);
            chanid = utils::ReadIntFromByteVector<decltype(chanid)>(itReadfrom);
            unk1   = utils::ReadIntFromByteVector<decltype(unk1)>  (itReadfrom);
            unk2   = utils::ReadIntFromByteVector<decltype(unk2)>  (itReadfrom);
            return itReadfrom;
        }
    };

//====================================================================================================
// Chunk Headers
//====================================================================================================

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
            itwriteto = utils::WriteIntToByteVector( static_cast<uint32_t>(eDSEChunks::song), itwriteto, false ); //Force this, to avoid bad surprises
            itwriteto = utils::WriteIntToByteVector( unk1,    itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk2,    itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk3,    itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk4,    itwriteto );
            itwriteto = utils::WriteIntToByteVector( tpqn,    itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk5,    itwriteto );
            itwriteto = utils::WriteIntToByteVector( nbtrks,  itwriteto );
            itwriteto = utils::WriteIntToByteVector( nbchans, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk6,    itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk7,    itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk8,    itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk9,    itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk10,   itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk11,   itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk12,   itwriteto );
            itwriteto = std::copy( unkpad.begin(), unkpad.end(), itwriteto );
            return itwriteto;
        }

        //
        template<class _init>
            _init ReadFromContainer(  _init itReadfrom )
        {
            label   = utils::ReadIntFromByteVector<decltype(label)>  (itReadfrom, false ); //iterator is incremented
            unk1    = utils::ReadIntFromByteVector<decltype(unk1)>   (itReadfrom); 
            unk2    = utils::ReadIntFromByteVector<decltype(unk2)>   (itReadfrom);
            unk3    = utils::ReadIntFromByteVector<decltype(unk3)>   (itReadfrom);
            unk4    = utils::ReadIntFromByteVector<decltype(unk4)>   (itReadfrom);
            tpqn    = utils::ReadIntFromByteVector<decltype(tpqn)>   (itReadfrom);
            unk5    = utils::ReadIntFromByteVector<decltype(unk5)>   (itReadfrom);
            nbtrks  = utils::ReadIntFromByteVector<decltype(nbtrks)> (itReadfrom);
            nbchans = utils::ReadIntFromByteVector<decltype(nbchans)>(itReadfrom);
            unk6    = utils::ReadIntFromByteVector<decltype(unk6)>   (itReadfrom);
            unk7    = utils::ReadIntFromByteVector<decltype(unk7)>   (itReadfrom);
            unk8    = utils::ReadIntFromByteVector<decltype(unk8)>   (itReadfrom);
            unk9    = utils::ReadIntFromByteVector<decltype(unk9)>   (itReadfrom);
            unk10   = utils::ReadIntFromByteVector<decltype(unk10)>  (itReadfrom);
            unk11   = utils::ReadIntFromByteVector<decltype(unk11)>  (itReadfrom);
            unk12   = utils::ReadIntFromByteVector<decltype(unk12)>  (itReadfrom);

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

//====================================================================================================
// Class
//====================================================================================================

    /************************************************************************
        EvTrack
            A track made out of raw events.
    ************************************************************************/
    //class EvTrack
    //{
    //public:
    //    typedef std::vector<TrkEvent>   track_t;
    //    typedef track_t::iterator       ittrk_t;
    //    typedef track_t::const_iterator cittrk_t;

    //    EvTrack() 
    //    {}

    //    // *** Events ops ***
    //    void             pushback( TrkEvent && ev )      { return m_events.push_back(ev); }
    //    TrkEvent      && popback ()
    //    {
    //        TrkEvent tmp;
    //        if( !m_events.empty() )
    //        {
    //            tmp = std::move( m_events[m_events.size()-1] );
    //            m_events.pop_back();
    //        }
    //        else
    //            throw std::runtime_error("EvTrack: Event track is empty! Cannot popback!");
    //        return std::move(tmp);
    //    }

    //    ittrk_t          begin()                         { return m_events.begin(); }
    //    cittrk_t         begin()const                    { return m_events.begin(); }
    //    ittrk_t          end()                           { return m_events.end();   }
    //    cittrk_t         end()const                      { return m_events.end();   }

    //    TrkEvent       & operator[]( size_t index )      { return m_events[index];  }
    //    const TrkEvent & operator[]( size_t index )const { return m_events[index];  }

    //    size_t size   ()const                            { return m_events.size();  }
    //    void   resize ( size_t newsz )                   { m_events.resize(newsz);  }
    //    void   reserve( size_t ressz )                   { m_events.reserve(ressz); }


    //private:


    //private:
    //    track_t m_events;
    //};


//====================================================================================================
// EventParser
//====================================================================================================
    /*
        EventParser
            Pass the bytes of an event track, after the preamble, and it will eat bytes and turn them 
            into events within the container refered to by the iterator !
    */
    template<class _outit>
        class EventParser
    {
    public:
        typedef _outit outit_t;

        EventParser( _outit itout )
            :m_itDest(itout), m_hasBegun(false), m_bytesToRead(0)
        {}

        //Feed bytes to this 
        void operator()( uint8_t by )
        {
            if( !m_hasBegun ) //If we haven't begun parsing
            {
                if( isDelayCode(by) )   //If we read a delay code, just put it in our upcoming event
                    m_curEvent.dt = by;
                else
                    beginNewEvent(by);
            }
            else
                fillEvent(by);
        }

    private:
        static inline bool isDelayCode( uint8_t by ) { return ( (by & EventDelayMask) == EventDelayCode ); }

        void beginNewEvent( uint8_t by )
        {
            auto einfo = GetEventInfo( static_cast<eTrkEventCodes>(by) );

            if( ! einfo.first )
            {
                std::stringstream sstr;
                sstr << "Unknown event type 0x" <<std::hex <<static_cast<uint16_t>(by) <<std::dec 
                     <<" encountered! Cannot continue due to unknown parameter length and possibly mis-alignment..";
                throw std::runtime_error( sstr.str() );
            }

            m_curEventInf     = einfo.second;
            m_curEvent.evcode = by;

            if( m_curEventInf.nbreqparams == 0 )
                endEvent();
            else
            {
                m_bytesToRead     = m_curEventInf.nbreqparams; //Parse the required params first
                m_hasBegun        = true;
            }
        }
        
        //This reads the parameter bytes for a given event. And reduce the nb of bytes to read counter
        void fillEvent( uint8_t by )
        {
            m_curEvent.params.push_back(by);

            if( m_curEvent.params.size() == 1 && (m_curEventInf.evcodebeg == eTrkEventCodes::NoteOnBeg) )
                m_bytesToRead += (m_curEvent.params.front() & NoteEvParam1NbParamsMask) >> 6; //For play notes events, the nb of extra bytes of data to read is contained in bits 7 and 8

            --m_bytesToRead;

            if( m_bytesToRead == 0 )
                endEvent();
        }

        void endEvent()
        {
            (*m_itDest) = std::move( m_curEvent );
            ++m_itDest;
            m_curEvent = TrkEvent(); //Re-init object state after move
            m_bytesToRead = 0;
            m_hasBegun = false;
        }

    private:
        outit_t             m_itDest;      // Output for assembled events
        bool                m_hasBegun;   // Whether we're working on an event right now
        uint32_t            m_bytesToRead; // this contains the amount of bytes to read before the event is fully parsed
        TrkEvent            m_curEvent;    // The event being assembled currently.
        TrkEventInfo        m_curEventInf; // Info on the current event type
    };

//====================================================================================================
// Functions
//====================================================================================================

    template<class _itin>
        std::pair<std::vector<TrkEvent>,TrkPreamble> ParseTrkChunk( _itin beg, _itin end )
    {
        ChunkHeader hdr;
        beg = hdr.ReadFromContainer(beg);

        if( hdr.label != static_cast<uint32_t>(eDSEChunks::trk) )
            throw std::runtime_error("ParseTrkChunk(): Unexpected chunk label !");

        if( std::distance(beg,end) < hdr.datlen )
            throw std::runtime_error("ParseTrkChunk(): Track chunk continues beyond the expected end !");

        //Set the actual end of the events track
        _itin itendevents = beg;
        std::advance( itendevents, hdr.datlen );

        std::vector<TrkEvent>   events;
        TrkPreamble             preamb;
        beg = preamb.ReadFromContainer(beg);

        //#TODO: Eventually do something with the preamble if its even of any use !

        
        events.reserve( hdr.datlen ); //Reserve worst case scenario

        std::for_each( beg, itendevents, EventParser<std::back_insert_iterator<std::vector<TrkEvent>>>(std::back_inserter(events)) );

        events.shrink_to_fit(); //Dealloc unused space

        return std::make_pair( std::move(events), std::move(preamb) );
    }

};

#endif