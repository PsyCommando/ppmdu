#include "dse_sequence.hpp"
#include "dse_common.hpp"
#include <iostream>
#include <iomanip>

using namespace std;

namespace DSE
{

    static const TrkEventInfo InvalidEventInfo {eTrkEventCodes::Invalid,eTrkEventCodes::Invalid, 0, false, "INVALID" };

    /***************************************************************************************
        TrkEventsTable
            Definition of the TrkEventsTable.
            
            Contains important details on how to parse all individual events.
    ***************************************************************************************/
    const std::vector<TrkEventInfo/*, NB_Track_Events*/> TrkEventsTable
    {{
        //Play Note event
        {
            eTrkEventCodes::NoteOnBeg,  //Event Codes Range Beginning
            eTrkEventCodes::NoteOnEnd,  //Event Codes Range End
            1,                          //Nb Required Parameters
            true,                       //Can Have Optional Parameter
            //false,                      //Is End Of Track Marker
            //false,                      //Is Loop Point Marker
            "PlayNote",                 //Event label
        },

        //TrackDelays
        {
            eTrkEventCodes::Delay_HN,
            eTrkEventCodes::Delay_64N,
            0,
            false,
            "FixedPause",
        },

        //RepeatLastPause
        { eTrkEventCodes::RepeatLastPause,  eTrkEventCodes::Invalid, 0, false, "RepeatLastPause" },

        //AddToLastPause
        { eTrkEventCodes::AddToLastPause,   eTrkEventCodes::Invalid, 1, false, "AddToLastPause" },

        //Pause8Bits
        { eTrkEventCodes::Pause8Bits,       eTrkEventCodes::Invalid, 1, false, "Pause8Bits" },

        //Pause16Bits
        { eTrkEventCodes::Pause16Bits,      eTrkEventCodes::Invalid, 2, false, "Pause16Bits" },

        //Pause24Bits
        { eTrkEventCodes::Pause24Bits,      eTrkEventCodes::Invalid, 3, false, "Pause24Bits" },

        //EndOfTrack
        { eTrkEventCodes::EndOfTrack,       eTrkEventCodes::Invalid, 0, false, "EndOfTrack" },

        //LoopPointSet
        { eTrkEventCodes::LoopPointSet,     eTrkEventCodes::Invalid, 0, false, "Loop" },

        //Unk_0x9C
        { eTrkEventCodes::Unk_0x9C,         eTrkEventCodes::Invalid, 1, false, "Unk_0x9C" },

        //Unk_0x9D
        { eTrkEventCodes::Unk_0x9D,         eTrkEventCodes::Invalid, 0, false, "Unk_0x9D" },

        //Unk_0x9E
        { eTrkEventCodes::Unk_0x9E,         eTrkEventCodes::Invalid, 0, false, "Unk_0x9E" },

        //SetOctave
        { eTrkEventCodes::SetOctave,        eTrkEventCodes::Invalid, 1, false, "SetOctave" },

        //Unk_0xA1
        { eTrkEventCodes::Unk_0xA1,         eTrkEventCodes::Invalid, 1, false, "Unk_0xA1" },

        //SetTempo
        { eTrkEventCodes::SetTempo,         eTrkEventCodes::Invalid, 1, false, "SetTempo" },

        //Unk_0xA5
        { eTrkEventCodes::Unk_0xA5,         eTrkEventCodes::Invalid, 1, false, "Unk_0xA5" },

        //Unk_0xA8
        { eTrkEventCodes::Unk_0xA8,         eTrkEventCodes::Invalid, 2, false, "Unk_0xA8" },

        //SetUnk1
        { eTrkEventCodes::SetUnk1,          eTrkEventCodes::Invalid, 1, false, "SetUnk1" },

        //SetUnk2
        { eTrkEventCodes::SetUnk2,          eTrkEventCodes::Invalid, 1, false, "SetUnk2" },

        //SetPreset
        { eTrkEventCodes::SetPreset,        eTrkEventCodes::Invalid, 1, false, "SetPreset" },

        //Unk_0xB2
        { eTrkEventCodes::Unk_0xB2,         eTrkEventCodes::Invalid, 1, false, "Unk_0xB2" },

        //Unk_0xB4
        { eTrkEventCodes::Unk_0xB4,         eTrkEventCodes::Invalid, 2, false, "Unk_0xB4" },

        //Unk_0xB5
        { eTrkEventCodes::Unk_0xB5,         eTrkEventCodes::Invalid, 1, false, "Unk_0xB5" },

        //Unk_0xBE
        { eTrkEventCodes::Unk_0xBE,         eTrkEventCodes::Invalid, 1, false, "Unk_0xBE" },

        //Unk_0xBF
        { eTrkEventCodes::Unk_0xBF,         eTrkEventCodes::Invalid, 1, false, "Unk_0xBF" },

        //Unk_0xC0
        { eTrkEventCodes::Unk_0xC0,         eTrkEventCodes::Invalid, 0, false, "Unk_0xC0" },

        //Unk_0xCB
        { eTrkEventCodes::Unk_0xCB,         eTrkEventCodes::Invalid, 2, false, "Unk_0xCB" },

        //Unk_0xD0
        { eTrkEventCodes::Unk_0xD0,         eTrkEventCodes::Invalid, 1, false, "Unk_0xD0" },

        //Unk_0xD1
        { eTrkEventCodes::Unk_0xD1,         eTrkEventCodes::Invalid, 1, false, "Unk_0xD1" },

        //Unk_0xD2
        { eTrkEventCodes::Unk_0xD2,         eTrkEventCodes::Invalid, 1, false, "Unk_0xD2" },

        //Unk_0xD4
        { eTrkEventCodes::Unk_0xD4,         eTrkEventCodes::Invalid, 3, false, "Unk_0xD4" },

        //Unk_0xD6
        { eTrkEventCodes::Unk_0xD6,         eTrkEventCodes::Invalid, 2, false, "Unk_0xD6" },

        //PitchBend
        { eTrkEventCodes::PitchBend,        eTrkEventCodes::Invalid, 2, false, "PitchBend" },

        //Unk_0xDB
        { eTrkEventCodes::Unk_0xDB,         eTrkEventCodes::Invalid, 1, false, "Unk_0xDB" },

        //Unk_0xDC
        { eTrkEventCodes::Unk_0xDC,         eTrkEventCodes::Invalid, 5, false, "Unk_0xDC" },

        //SetTrkVol
        { eTrkEventCodes::SetTrkVol,        eTrkEventCodes::Invalid, 1, false, "SetVolume" },

        //Unk_0xE2
        { eTrkEventCodes::Unk_0xE2,         eTrkEventCodes::Invalid, 3, false, "Unk_0xE2" },

        //SetExpress
        { eTrkEventCodes::SetExpress,       eTrkEventCodes::Invalid, 1, false, "SetExpression" },

        //SetTrkPan
        { eTrkEventCodes::SetTrkPan,        eTrkEventCodes::Invalid, 1, false, "SetPan" },

        //Unk_0xEA
        { eTrkEventCodes::Unk_0xEA,         eTrkEventCodes::Invalid, 3, false, "Unk_0xEA" },

        //Unk_0xEC
        { eTrkEventCodes::Unk_0xEC,         eTrkEventCodes::Invalid, 5, false, "Unk_0xEC" },

        //Unk_0xF6
        { eTrkEventCodes::Unk_0xF6,         eTrkEventCodes::Invalid, 1, false, "Unk_0xF6" },
    }};

//====================================================================================================
// Utility
//====================================================================================================
    std::pair<bool,TrkEventInfo> GetEventInfo( eTrkEventCodes ev )
    {
        for( const auto & entry : TrkEventsTable )
        {
            if( ( entry.evcodeend != eTrkEventCodes::Invalid     ) && 
                ( ev >= entry.evcodebeg && ev <= entry.evcodeend ) )
            {
                return move( make_pair(true, entry ) );
            }
            else if( entry.evcodebeg == ev )
            {
                return move( make_pair( true, entry ) );
            }
        }
        return move( make_pair(false, InvalidEventInfo ) );
    }


    /*****************************************************************
        ParsePlayNoteParam1
            This interpret and returns the 3 values that are 
            stored in the playnote event's first parameter.
    *****************************************************************/
    void ParsePlayNoteParam1(  uint8_t   noteparam1, 
                               uint8_t & inout_curoctave, 
                               uint8_t & out_param2len, 
                               uint8_t & out_midinote )
    {
        //1. Get param2's len
        out_param2len = ( ( noteparam1 & NoteEvParam1NbParamsMask ) >> 6 ) & 0x3; //(0011) just to be sure no sign bits slip through somehow

        //2. Get and apply the octave modifiere
        int8_t octavemod = ( ( (noteparam1 & NoteEvParam1PitchMask) >> 4 ) & 0x3 ) - NoteEvOctaveShiftRange;
        inout_curoctave  = static_cast<int8_t>(inout_curoctave) + octavemod; 

        //3. Get the midi note
        out_midinote = ( inout_curoctave * static_cast<uint8_t>(eNote::nbNotes) ) + (noteparam1 & 0xF);
    }


    void LogEventToClog( const TrkEvent & ev )
    {
        clog << ev;
    }
//====================================================================================================
//====================================================================================================

};