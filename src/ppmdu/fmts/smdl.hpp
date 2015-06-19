#ifndef SMDL_HPP
#define SMDL_HPP
/*
smdl.hpp
2015/05/20
psycommando@gmail.com
Description: Utilities for handling Pokemon Mystery Dungeon: Explorers of Sky/Time/Darkness's .smd files.
*/
#include <ppmdu/fmts/dse_common.hpp>

namespace DSE
{

//====================================================================================================
//  Typedefs
//====================================================================================================

    /************************************************************************
        eTrkEventCodes
            Codes for each different events stored in a smdl track chunk.
    ************************************************************************/
    enum struct eTrkEventCodes : uint8_t
    {
        NoteOnBeg     = 0x01, //The first event code reserved for play note events.
        NoteOnEnd     = 0x7F, //The last event code that is reserved for playing a note.

        NoteDTBeg     = 0x80, //The first value reserved for note Delta-Time
        NoteDTEnd     = 0x8F, //The last value reserved for note Delta-Time

        RepeatSilence = 0x90, //Repeat the last silence
        Silence       = 0x92, //Silence the track for specified duration (uses a uint8)
        LongSilence   = 0x93, //Silence the track for specified duration (uses a uint16)
        EndOfTrack    = 0x98, //Marks the end of the track. Also serve as padding.
        LoopPointSet  = 0x99, //Marks the location where the track should loop from.
        SetOctave     = 0xA0, //Sets the octave notes are currently played at.
        SetTempo      = 0xA4, //Sets the tempo of the track in BPM.
        SetUnk1       = 0xA9, //Set that first unknown value from the track's header
        SetUnk2       = 0xAA, //Set that second unknown value from the track's header
        SetPreset     = 0xAC, //Sets the instrument preset to use
        Modulate      = 0xD7, //Pitch bending/modulation/LFO. Not 100% certain.
        SetUnk3       = 0xDB, //Unknown purpose. Used in bgmM0000.smd
        SetTrkVol     = 0xE0, //Sets primary track volume.
        SetExpress    = 0xE3, //Sets secondary volume control. AKA expression or GM CC#11.
        SetTrkPan     = 0xE8, //Sets the panning of the track.
    };

//====================================================================================================
// Structs
//====================================================================================================
    
    /************************************************************************
        TrkEvent
            Represent a raw track event.
    ************************************************************************/
    struct TrkEvent
    {
        uint8_t dt     = 0;
        uint8_t evcode = 0;
        uint8_t param1 = 0;
        uint8_t param2 = 0;
    };

    /************************************************************************
        SongChunk
            The raw song chunk.
    ************************************************************************/
    struct SongChunk
    {
    };

    /*
    */


//====================================================================================================
// Class
//====================================================================================================

    /************************************************************************
        EvTrack
            A track made out of raw events.
    ************************************************************************/
    class EvTrack
    {
    public:
        typedef std::vector<TrkEvent>   track_t;
        typedef track_t::iterator       ittrk_t;
        typedef track_t::const_iterator cittrk_t;

        EvTrack() 
        {}

        // *** Events ops ***
        void             pushback( TrkEvent && ev )      { return m_events.push_back(ev); }
        TrkEvent      && popback ()
        {
            TrkEvent tmp;
            if( !m_events.empty() )
            {
                tmp = std::move( m_events[m_events.size()-1] );
                m_events.pop_back();
            }
            else
                throw std::runtime_error("EvTrack: Event track is empty! Cannot popback!");
            return std::move(tmp);
        }

        ittrk_t          begin()                         { return m_events.begin(); }
        cittrk_t         begin()const                    { return m_events.begin(); }
        ittrk_t          end()                           { return m_events.end();   }
        cittrk_t         end()const                      { return m_events.end();   }

        TrkEvent       & operator[]( size_t index )      { return m_events[index];  }
        const TrkEvent & operator[]( size_t index )const { return m_events[index];  }

        size_t size   ()const                            { return m_events.size();  }
        void   resize ( size_t newsz )                   { m_events.resize(newsz);  }
        void   reserve( size_t ressz )                   { m_events.reserve(ressz); }

    private:
        track_t m_events;
    };

//====================================================================================================
// Functions
//====================================================================================================

};

#endif