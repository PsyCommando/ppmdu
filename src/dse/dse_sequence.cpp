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

        //0x90 - RepeatLastPause
        { eTrkEventCodes::RepeatLastPause,  eTrkEventCodes::Invalid, 0, "RepeatLastPause" },

        //0x91 - AddToLastPause
        { eTrkEventCodes::AddToLastPause,   eTrkEventCodes::Invalid, 1, "AddToLastPause" },

        //0x92 - Pause8Bits
        { eTrkEventCodes::Pause8Bits,       eTrkEventCodes::Invalid, 1, "Pause8Bits" },

        //0x93 - Pause16Bits
        { eTrkEventCodes::Pause16Bits,      eTrkEventCodes::Invalid, 2, "Pause16Bits" },

        //0x94 - Pause24Bits
        { eTrkEventCodes::Pause24Bits,      eTrkEventCodes::Invalid, 3, "Pause24Bits" },

        //0x95 - PauseUntilNoteOff

        //0x98 - EndOfTrack
        { eTrkEventCodes::EndOfTrack,       eTrkEventCodes::Invalid, 0, "EndOfTrack" },

        //0x99 - LoopPointSet
        { eTrkEventCodes::LoopPointSet,     eTrkEventCodes::Invalid, 0, "Loop" },

        //0x9C - RepeatFrom
        { eTrkEventCodes::RepeatFrom,       eTrkEventCodes::Invalid, 1, "RepeatFrom" },

        //0x9D - RepeatSegment
        { eTrkEventCodes::RepeatSegment,     eTrkEventCodes::Invalid, 0, "RepeatSegment" },

        //0x9E - AfterRepeat
        { eTrkEventCodes::AfterRepeat,      eTrkEventCodes::Invalid, 0, "AfterRepeat" },

        //0xA0 - SetOctave
        { eTrkEventCodes::SetOctave,        eTrkEventCodes::Invalid, 1, "SetOctave" },

        //0xA1 - AddOctave
        { eTrkEventCodes::AddOctave,        eTrkEventCodes::Invalid, 1, "AddOctave" },

        //0xA4 - SetTempo
        { eTrkEventCodes::SetTempo,         eTrkEventCodes::Invalid, 1, "SetTempo" },

        //0xA5 - SetTempo2
        { eTrkEventCodes::SetTempo2,         eTrkEventCodes::Invalid, 1, "SetTempo2" },

        //0xA8 - SetSwdlAndBank
        { eTrkEventCodes::SetSwdlAndBank,   eTrkEventCodes::Invalid, 2, "SetSwdlAndBank" },

        //0xA9 - SetSwdl
        { eTrkEventCodes::SetSwdl,          eTrkEventCodes::Invalid, 1, "SetSwdl" },

        //0xAA - SetBank
        { eTrkEventCodes::SetBank,          eTrkEventCodes::Invalid, 1, "SetBank" },

        //0xAB - SkipNextByte
        { eTrkEventCodes::SkipNextByte,     eTrkEventCodes::Invalid, 1, "SkipNextByte" },

        //0xAC - SetPreset
        { eTrkEventCodes::SetPreset,        eTrkEventCodes::Invalid, 1, "SetPreset" },

        //0xAF - FadeSongVolume 
        { eTrkEventCodes::FadeSongVolume,   eTrkEventCodes::Invalid, 3, "FadeSongVolume" },

        //0xB0 - DisableEnvelope
        { eTrkEventCodes::DisableEnvelope,  eTrkEventCodes::Invalid, 0, "DisableEnvelope" },

        //0xB1 - SetEnvAtkLvl
        { eTrkEventCodes::SetEnvAtkLvl,     eTrkEventCodes::Invalid, 1, "SetEnvAtkLvl" },

        //0xB2 - SetEnvAtkTime
        { eTrkEventCodes::SetEnvAtkTime,    eTrkEventCodes::Invalid, 1, "SetEnvAtkTime" },

        //0xB3 - SetEnvHold
        { eTrkEventCodes::SetEnvHold,       eTrkEventCodes::Invalid, 1, "SetEnvHold" },

        //0xB4 - SetEnvDecSus
        { eTrkEventCodes::SetEnvDecSus,     eTrkEventCodes::Invalid, 2, "SetEnvDecSus" },

        //0xB5 - SetEnvFade
        { eTrkEventCodes::SetEnvFade,       eTrkEventCodes::Invalid, 1, "SetEnvFade" },

        //0xB6 - SetEnvRelease
        { eTrkEventCodes::SetEnvRelease,    eTrkEventCodes::Invalid, 1, "SetEnvRelease" },

        //0xBC - SetNoteVol
        { eTrkEventCodes::SetNoteVol,       eTrkEventCodes::Invalid, 1, "SetNoteVol" },

        //0xBE - SetChanPan
        { eTrkEventCodes::SetChanPan,       eTrkEventCodes::Invalid, 1, "SetChanPan" },

        //Unk_0xBF
        { eTrkEventCodes::Unk_0xBF,         eTrkEventCodes::Invalid, 1, "## Unk_0xBF ##" },

        //Unk_0xC0
        { eTrkEventCodes::Unk_0xC0,         eTrkEventCodes::Invalid, 0, "## Unk_0xC0 ##" },

        //0xC3 - SetChanVol
        { eTrkEventCodes::SetChanVol,       eTrkEventCodes::Invalid, 1, "SetChanVol" },

        //0xCB - SkipNext2Bytes1
        { eTrkEventCodes::SkipNext2Bytes1,  eTrkEventCodes::Invalid, 2, "SkipNext2Bytes" },

        //0xD0 - SetFTune
        { eTrkEventCodes::SetFTune,         eTrkEventCodes::Invalid, 1, "SetFTune" },

        //0xD1 - AddFTune
        { eTrkEventCodes::AddFTune,         eTrkEventCodes::Invalid, 1, "AddFTune" },

        //0xD2 - SetCTune
        { eTrkEventCodes::SetCTune,         eTrkEventCodes::Invalid, 1, "SetCTune" },

        //0xD3 - AddCTune
        { eTrkEventCodes::AddCTune,         eTrkEventCodes::Invalid, 2, "AddCTune" },

        //0xD4 - SweepTune
        { eTrkEventCodes::SweepTune,        eTrkEventCodes::Invalid, 3, "SweepTune" },

        //0xD5 - SetRndNoteRng
        { eTrkEventCodes::SetRndNoteRng,    eTrkEventCodes::Invalid, 2, "SetRndNoteRng" },

        //0xD6 - SetDetuneRng
        { eTrkEventCodes::SetDetuneRng,     eTrkEventCodes::Invalid, 2, "SetDetuneRng" },

        //0xD7 - SetPitchBend
        { eTrkEventCodes::SetPitchBend,     eTrkEventCodes::Invalid, 2, "SetPitchBend" },

        //Unk_0xD8
        { eTrkEventCodes::Unk_0xD8,         eTrkEventCodes::Invalid, 2, "## Unk_0xD8 ##" },

        //0xDB - SetPitchBendRng
        { eTrkEventCodes::SetPitchBendRng,  eTrkEventCodes::Invalid, 1, "SetPitchBendRng" },

        //0xDC - SetLFO1
        { eTrkEventCodes::SetLFO1,          eTrkEventCodes::Invalid, 5, "SetLFO1" },

        //0xDD - SetLFO1DelayFade
        { eTrkEventCodes::SetLFO1DelayFade, eTrkEventCodes::Invalid, 4, "SetLFO1DelayFade" },

        //0xDF - RouteLFO1ToPitch
        { eTrkEventCodes::RouteLFO1ToPitch, eTrkEventCodes::Invalid, 1, "RouteLFO1ToPitch" },

        //0xE0 - SetTrkVol
        { eTrkEventCodes::SetTrkVol,        eTrkEventCodes::Invalid, 1, "SetVolume" },

        //0xE1 - AddTrkVol
        { eTrkEventCodes::AddTrkVol,        eTrkEventCodes::Invalid, 1, "AddTrkVol" },

        //0xE2 - SweepTrackVol
        { eTrkEventCodes::SweepTrackVol,    eTrkEventCodes::Invalid, 3, "SweepTrackVol" },

        //0xE3 - SetExpress
        { eTrkEventCodes::SetExpress,       eTrkEventCodes::Invalid, 1, "SetExpression" },

        //0xE4 - SetLFO2
        { eTrkEventCodes::SetLFO2,          eTrkEventCodes::Invalid, 5, "SetLFO2" },

        //0xE5 - SetLFO2DelFade
        { eTrkEventCodes::SetLFO2DelFade,   eTrkEventCodes::Invalid, 4, "SetLFO2DelFade" },

        //0xE7 - RouteLFO2ToVol
        { eTrkEventCodes::RouteLFO2ToVol,   eTrkEventCodes::Invalid, 1, "RouteLFO2ToVol" },

        //0xE8 - SetTrkPan
        { eTrkEventCodes::SetTrkPan,        eTrkEventCodes::Invalid, 1, "SetPan" },

        //0xE9 - AddTrkPan
        { eTrkEventCodes::AddTrkPan,        eTrkEventCodes::Invalid, 1, "AddTrkPan" },

        //0xEA - SweepTrkPan
        { eTrkEventCodes::SweepTrkPan,      eTrkEventCodes::Invalid, 3, "SweepTrkPan" },

        //0xEC - SetLFO3
        { eTrkEventCodes::SetLFO3,          eTrkEventCodes::Invalid, 5, "SetLFO3" },

        //0xED - SetLFO3DelFade
        { eTrkEventCodes::SetLFO3DelFade,   eTrkEventCodes::Invalid, 4, "SetLFO3DelFade" },

        //0xEF - RouteLFO3ToPan
        { eTrkEventCodes::RouteLFO3ToPan,   eTrkEventCodes::Invalid, 1, "RouteLFO3ToPan" },

        //0xF0 - SetLFO
        { eTrkEventCodes::SetLFO,           eTrkEventCodes::Invalid, 5, "SetLFO" },

        //0xF1 - SetLFODelFade
        { eTrkEventCodes::SetLFODelFade,    eTrkEventCodes::Invalid, 4, "SetLFODelFade" },

        //0xF2 - SetLFOParam
        { eTrkEventCodes::SetLFOParam,      eTrkEventCodes::Invalid, 2, "SetLFOParam" },

        //0xF3 - SetLFORoute
        { eTrkEventCodes::SetLFORoute,      eTrkEventCodes::Invalid, 3, "SetLFORoute" },

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