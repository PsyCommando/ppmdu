#include "dse_sequence.hpp"
#include "dse_common.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;

namespace DSE
{

    static const TrkEventInfo InvalidEventInfo {eTrkEventCodes::Invalid,eTrkEventCodes::Invalid, 0, "INVALID" };

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
            "PlayNote",                 //Event label
        },

        //TrackDelays
        {
            eTrkEventCodes::Delay_HN,
            eTrkEventCodes::Delay_64N,
            0,
            "FixedPause",
        },

        //RepeatLastPause
        { eTrkEventCodes::RepeatLastPause,  eTrkEventCodes::Invalid, 0, "RepeatLastPause" },

        //AddToLastPause
        { eTrkEventCodes::AddToLastPause,   eTrkEventCodes::Invalid, 1, "AddToLastPause" },

        //Pause8Bits
        { eTrkEventCodes::Pause8Bits,       eTrkEventCodes::Invalid, 1, "Pause8Bits" },

        //Pause16Bits
        { eTrkEventCodes::Pause16Bits,      eTrkEventCodes::Invalid, 2, "Pause16Bits" },

        //Pause24Bits
        { eTrkEventCodes::Pause24Bits,      eTrkEventCodes::Invalid, 3, "Pause24Bits" },

        //EndOfTrack
        { eTrkEventCodes::EndOfTrack,       eTrkEventCodes::Invalid, 0, "EndOfTrack" },

        //LoopPointSet
        { eTrkEventCodes::LoopPointSet,     eTrkEventCodes::Invalid, 0, "Loop" },

        //Unk_0x9C
        { eTrkEventCodes::Unk_0x9C,         eTrkEventCodes::Invalid, 1, "## Unk_0x9C ##" },

        //Unk_0x9D
        { eTrkEventCodes::Unk_0x9D,         eTrkEventCodes::Invalid, 0, "## Unk_0x9D ##" },

        //Unk_0x9E
        { eTrkEventCodes::Unk_0x9E,         eTrkEventCodes::Invalid, 0, "## Unk_0x9E ##" },

        //SetOctave
        { eTrkEventCodes::SetOctave,        eTrkEventCodes::Invalid, 1, "SetOctave" },

        //Unk_0xA1
        { eTrkEventCodes::Unk_0xA1,         eTrkEventCodes::Invalid, 1, "## Unk_0xA1 ##" },

        //SetTempo
        { eTrkEventCodes::SetTempo,         eTrkEventCodes::Invalid, 1, "SetTempo" },

        //Unk_0xA5
        { eTrkEventCodes::Unk_0xA5,         eTrkEventCodes::Invalid, 1, "## Unk_0xA5 ##" },

        //Unk_0xA8
        { eTrkEventCodes::Unk_0xA8,         eTrkEventCodes::Invalid, 2, "## Unk_0xA8 ##" },

        //SetUnk1
        { eTrkEventCodes::SetUnk1,          eTrkEventCodes::Invalid, 1, "SetUnk1" },

        //SetUnk2
        { eTrkEventCodes::SetUnk2,          eTrkEventCodes::Invalid, 1, "SetUnk" },

        //SkipNextByte
        { eTrkEventCodes::SkipNextByte,     eTrkEventCodes::Invalid, 1, "SkipNextByte" },

        //SetPreset
        { eTrkEventCodes::SetPreset,        eTrkEventCodes::Invalid, 1, "SetPreset" },

        //Unk_0xAF 
        { eTrkEventCodes::Unk_0xAF,         eTrkEventCodes::Invalid, 3, "## Unk_0xAF ##" },

        //Unk_0xB0
        { eTrkEventCodes::Unk_0xB0,         eTrkEventCodes::Invalid, 0, "## Unk_0xB0 ##" },

        //Unk_0xB1
        { eTrkEventCodes::Unk_0xB1,         eTrkEventCodes::Invalid, 1, "## Unk_0xB1 ##" },

        //Unk_0xB2
        { eTrkEventCodes::Unk_0xB2,         eTrkEventCodes::Invalid, 1, "## Unk_0xB2 ##" },

        //Unk_0xB3
        { eTrkEventCodes::Unk_0xB3,         eTrkEventCodes::Invalid, 1, "## Unk_0xB3 ##" },

        //Unk_0xB4
        { eTrkEventCodes::Unk_0xB4,         eTrkEventCodes::Invalid, 2, "## Unk_0xB4 ##" },

        //Unk_0xB5
        { eTrkEventCodes::Unk_0xB5,         eTrkEventCodes::Invalid, 1, "## Unk_0xB5 ##" },

        //Unk_0xB6
        { eTrkEventCodes::Unk_0xB6,         eTrkEventCodes::Invalid, 1, "## Unk_0xB6 ##" },

        //Unk_0xBC
        { eTrkEventCodes::Unk_0xBC,         eTrkEventCodes::Invalid, 1, "## Unk_0xBC ##" },

        //SetMod
        { eTrkEventCodes::SetMod,           eTrkEventCodes::Invalid, 1, "SetMod" },

        //Unk_0xBF
        { eTrkEventCodes::Unk_0xBF,         eTrkEventCodes::Invalid, 1, "## Unk_0xBF ##" },

        //Unk_0xC0
        { eTrkEventCodes::Unk_0xC0,         eTrkEventCodes::Invalid, 0, "## Unk_0xC0 ##" },

        //Unk_0xC3
        { eTrkEventCodes::Unk_0xC3,         eTrkEventCodes::Invalid, 1, "## Unk_0xC3 ##" },

        //SkipNext2Bytes1
        { eTrkEventCodes::SkipNext2Bytes1,  eTrkEventCodes::Invalid, 2, "SkipNext2Bytes" },

        //Unk_0xD0
        { eTrkEventCodes::Unk_0xD0,         eTrkEventCodes::Invalid, 1, "## Unk_0xD0 ##" },

        //Unk_0xD1
        { eTrkEventCodes::Unk_0xD1,         eTrkEventCodes::Invalid, 1, "## Unk_0xD1 ##" },

        //Unk_0xD2
        { eTrkEventCodes::Unk_0xD2,         eTrkEventCodes::Invalid, 1, "## Unk_0xD2 ##" },

        //Unk_0xD3
        { eTrkEventCodes::Unk_0xD3,         eTrkEventCodes::Invalid, 2, "## Unk_0xD3 ##" },

        //Unk_0xD4
        { eTrkEventCodes::Unk_0xD4,         eTrkEventCodes::Invalid, 3, "## Unk_0xD4 ##" },

        //Unk_0xD5
        { eTrkEventCodes::Unk_0xD5,         eTrkEventCodes::Invalid, 2, "## Unk_0xD5 ##" },

        //Unk_0xD6
        { eTrkEventCodes::Unk_0xD6,         eTrkEventCodes::Invalid, 2, "## Unk_0xD6 ##" },

        //PitchBend
        { eTrkEventCodes::PitchBend,        eTrkEventCodes::Invalid, 2, "PitchBend" },

        //Unk_0xD8
        { eTrkEventCodes::Unk_0xD8,         eTrkEventCodes::Invalid, 2, "## Unk_0xD8 ##" },

        //Unk_0xDB
        { eTrkEventCodes::Unk_0xDB,         eTrkEventCodes::Invalid, 1, "## Unk_0xDB ##" },

        //Unk_0xDC
        { eTrkEventCodes::Unk_0xDC,         eTrkEventCodes::Invalid, 5, "## Unk_0xDC ##" },

        //Unk_0xDD
        { eTrkEventCodes::Unk_0xDD,         eTrkEventCodes::Invalid, 4, "## Unk_0xDD ##" },

        //Unk_0xDF
        { eTrkEventCodes::Unk_0xDF,         eTrkEventCodes::Invalid, 1, "## Unk_0xDF ##" },

        //SetTrkVol
        { eTrkEventCodes::SetTrkVol,        eTrkEventCodes::Invalid, 1, "SetVolume" },

        //Unk_0xE1
        { eTrkEventCodes::Unk_0xE1,         eTrkEventCodes::Invalid, 1, "## Unk_0xE1 ##" },

        //Unk_0xE2
        { eTrkEventCodes::Unk_0xE2,         eTrkEventCodes::Invalid, 3, "## Unk_0xE2 ##" },

        //SetExpress
        { eTrkEventCodes::SetExpress,       eTrkEventCodes::Invalid, 1, "SetExpression" },

        //Unk_0xE4
        { eTrkEventCodes::Unk_0xE4,         eTrkEventCodes::Invalid, 5, "## Unk_0xE4 ##" },

        //Unk_0xE5
        { eTrkEventCodes::Unk_0xE5,         eTrkEventCodes::Invalid, 4, "## Unk_0xE5 ##" },

        //Unk_0xE7
        { eTrkEventCodes::Unk_0xE7,         eTrkEventCodes::Invalid, 1, "## Unk_0xE7 ##" },

        //SetTrkPan
        { eTrkEventCodes::SetTrkPan,        eTrkEventCodes::Invalid, 1, "SetPan" },

        //Unk_0xE9
        { eTrkEventCodes::Unk_0xE9,         eTrkEventCodes::Invalid, 1, "## Unk_0xE9 ##" },

        //Unk_0xEA
        { eTrkEventCodes::Unk_0xEA,         eTrkEventCodes::Invalid, 3, "## Unk_0xEA ##" },

        //Unk_0xEC
        { eTrkEventCodes::Unk_0xEC,         eTrkEventCodes::Invalid, 5, "## Unk_0xEC ##" },

        //Unk_0xED
        { eTrkEventCodes::Unk_0xED,         eTrkEventCodes::Invalid, 4, "## Unk_0xED ##" },

        //Unk_0xEF
        { eTrkEventCodes::Unk_0xEF,         eTrkEventCodes::Invalid, 1, "## Unk_0xEF ##" },

        //Unk_0xF0
        { eTrkEventCodes::Unk_0xF0,         eTrkEventCodes::Invalid, 5, "## Unk_0xF0 ##" },

        //Unk_0xF1
        { eTrkEventCodes::Unk_0xF1,         eTrkEventCodes::Invalid, 4, "## Unk_0xF1 ##" },

        //Unk_0xF2
        { eTrkEventCodes::Unk_0xF2,         eTrkEventCodes::Invalid, 2, "## Unk_0xF2 ##" },

        //Unk_0xF3
        { eTrkEventCodes::Unk_0xF3,         eTrkEventCodes::Invalid, 3, "## Unk_0xF3 ##" },

        //Unk_0xF6
        { eTrkEventCodes::Unk_0xF6,         eTrkEventCodes::Invalid, 1, "## Unk_0xF6 ##" },

        //SkipNext2Bytes2
        { eTrkEventCodes::SkipNext2Bytes2,   eTrkEventCodes::Invalid, 2, "SkipNext2Bytes2" },

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
    //void ProcPlayNoteParam1(  uint8_t   noteparam1, 
    //                           uint8_t & inout_curoctave, 
    //                           uint8_t & out_param2len, 
    //                           uint8_t & out_midinote )
    //{
    //    //1. Get param2's len
    //    out_param2len = ( ( noteparam1 & NoteEvParam1NbParamsMask ) >> 6 ) & 0x3; //(0011) just to be sure no sign bits slip through somehow

    //    //2. Get and apply the octave modifiere
    //    int8_t octavemod = ( ( (noteparam1 & NoteEvParam1PitchMask) >> 4 ) & 0x3 ) - NoteEvOctaveShiftRange;
    //    inout_curoctave  = static_cast<int8_t>(inout_curoctave) + octavemod; 

    //    //3. Get the midi note
    //    out_midinote = ( inout_curoctave * static_cast<uint8_t>(eNote::nbNotes) ) + (noteparam1 & 0xF);
    //}

    void ParsePlayNoteParam1( uint8_t  noteparam1,
                              int8_t   & out_octdiff,
                              uint8_t  & out_notedur,
                              uint8_t  & out_key )
    {
        //1. Get param2's len
        out_notedur = ( ( noteparam1 & NoteEvParam1NbParamsMask ) >> 6 ) & 0x3; //(0011) just to be sure no sign bits slip through somehow

        //2. Get and apply the octave modifiere
        out_octdiff = ( ( (noteparam1 & NoteEvParam1PitchMask) >> 4 ) & 0x3 ) - NoteEvOctaveShiftRange;

        //3. Get the key parameter 0x0 to 0xB, sometimes 0xF for special purpose notes!
        out_key = (noteparam1 & 0xF);
    }

    std::string MidiNoteIdToText( uint8_t midinote )
    {
        stringstream sstr;
        uint16_t     key    = midinote % static_cast<uint8_t>(eNote::nbNotes);
        uint16_t     octave = midinote / static_cast<uint8_t>(eNote::nbNotes);
        sstr << NoteNames.at(key) << octave;
        return std::move(sstr.str());
    }


    void LogEventToClog( const TrkEvent & ev )
    {
        clog << ev;
    }
//====================================================================================================
//====================================================================================================

};