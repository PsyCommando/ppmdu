#include "dse_sequence.hpp"

using namespace std;

namespace DSE
{

    static const TrkEventInfo InvalidEventInfo {eTrkEventCodes::Invalid,eTrkEventCodes::Invalid,0,false,false,false, "INVALID"};

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
            false,                      //Is End Of Track Marker
            false,                      //Is Loop Point Marker
            "PNote",                    //Event label
        },

        //RepeatLastPause
        { eTrkEventCodes::RepeatLastPause, eTrkEventCodes::Invalid, 0, false, false, false, "RLP" },

        //AddToLastPause
        { eTrkEventCodes::AddToLastPause, eTrkEventCodes::Invalid, 1, false, false, false,  "ATLP" },

        //Pause
        { eTrkEventCodes::Pause,       eTrkEventCodes::Invalid, 1, false, false, false, "Pause" },

        //LongPause
        { eTrkEventCodes::LongPause,   eTrkEventCodes::Invalid, 2, false, false, false, "LongPause" },

        //EndOfTrack
        { eTrkEventCodes::EndOfTrack,    eTrkEventCodes::Invalid, 0, false, true, false,  "EoT" },

        //LoopPointSet
        { eTrkEventCodes::LoopPointSet, eTrkEventCodes::Invalid, 0, false, false, true,   "Loop" },

        //Unk_0x9C
        { eTrkEventCodes::Unk_0x9C,     eTrkEventCodes::Invalid, 1, false, false, true,   "Unk_0x9C" },

        //SetOctave
        { eTrkEventCodes::SetOctave,    eTrkEventCodes::Invalid, 1, false, false, false,  "Pitch" },

        //SetTempo
        { eTrkEventCodes::SetTempo,     eTrkEventCodes::Invalid, 1, false, false, false,  "BPM" },

        //Unk_0xA8
        { eTrkEventCodes::Unk_0xA8,     eTrkEventCodes::Invalid, 2, false, false, false,  "Unk_0xA8" },

        //SetUnk1
        { eTrkEventCodes::SetUnk1,      eTrkEventCodes::Invalid, 1, false, false, false,  "Unk1" },

        //SetUnk2
        { eTrkEventCodes::SetUnk2,      eTrkEventCodes::Invalid, 1, false, false, false,  "Unk2" },

        //SetPreset
        { eTrkEventCodes::SetPreset,    eTrkEventCodes::Invalid, 1, false, false, false,  "Prgm" },

        //Unk_0xB2
        { eTrkEventCodes::Unk_0xB2,     eTrkEventCodes::Invalid, 1, false, false, false,  "Unk_0xB2" },

        //Unk_0xB4
        { eTrkEventCodes::Unk_0xB4,     eTrkEventCodes::Invalid, 2, false, false, false,  "Unk_0xB4" },

        //Unk_0xB5
        { eTrkEventCodes::Unk_0xB5,     eTrkEventCodes::Invalid, 1, false, false, false,  "Unk_0xB5" },

        //Unk_0xBE
        { eTrkEventCodes::Unk_0xBE,      eTrkEventCodes::Invalid, 1, false, false, false,  "Unk_0xBE" },

        //Unk_0xBF
        { eTrkEventCodes::Unk_0xBF,      eTrkEventCodes::Invalid, 1, false, false, false,  "Unk_0xBF" },

        //HoldNote
        { eTrkEventCodes::HoldNote,      eTrkEventCodes::Invalid, 0, false, false, false,  "Hold" },

        //Unk_0xD0
        { eTrkEventCodes::Unk_0xD0,      eTrkEventCodes::Invalid, 1, false, false, false,  "Unk_0xD0" },

        //Unk_0xD1
        { eTrkEventCodes::Unk_0xD1,      eTrkEventCodes::Invalid, 2, false, false, false,  "Unk_0xD1" },

        //Unk_0xD2
        { eTrkEventCodes::Unk_0xD2,      eTrkEventCodes::Invalid, 1, false, false, false,  "Unk_0xD2" },

        //Unk_0xD4
        { eTrkEventCodes::Unk_0xD4,     eTrkEventCodes::Invalid, 3, false, false, false,  "Unk_0xD4" },

        //Unk_0xD6
        { eTrkEventCodes::Unk_0xD6,     eTrkEventCodes::Invalid, 2, false, false, false,  "Unk_0xD6" },

        //PitchBend
        { eTrkEventCodes::PitchBend,    eTrkEventCodes::Invalid, 2, false, false, false,  "PitchBend" },

        //Unk_0xDB
        { eTrkEventCodes::Unk_0xDB,      eTrkEventCodes::Invalid, 1, false, false, false,  "Unk_0xDB" },

        //Unk_0xDC
        { eTrkEventCodes::Unk_0xDC,      eTrkEventCodes::Invalid, 5, false, false, false,  "Unk_0xDC" },

        //SetTrkVol
        { eTrkEventCodes::SetTrkVol,    eTrkEventCodes::Invalid, 1, false, false, false,  "Vol" },

        //Unk_0xE2
        { eTrkEventCodes::Unk_0xE2,    eTrkEventCodes::Invalid, 3, false, false, false,  "Unk_0xE2" },

        //SetExpress
        { eTrkEventCodes::SetExpress,   eTrkEventCodes::Invalid, 1, false, false, false,  "Exp" },

        //SetTrkPan
        { eTrkEventCodes::SetTrkPan,    eTrkEventCodes::Invalid, 1, false, false, false,  "Pan" },

        //Unk_0xEA
        { eTrkEventCodes::Unk_0xEA,    eTrkEventCodes::Invalid, 3, false, false, false,  "Unk_0xEA" },

        //Unk_0xEC
        { eTrkEventCodes::Unk_0xEC,    eTrkEventCodes::Invalid, 5, false, false, false,  "Unk_0xEC" },

        //Unk_0xF6
        { eTrkEventCodes::Unk_0xF6,    eTrkEventCodes::Invalid, 1, false, false, false,  "Unk_0xF6" },
    }};


//====================================================================================================
// Utility
//====================================================================================================
    std::pair<bool,TrkEventInfo> GetEventInfo( eTrkEventCodes ev )
    {
        for( auto & entry : TrkEventsTable )
        {
            if( ( entry.evcodeend != eTrkEventCodes::Invalid     ) && 
                ( ev >= entry.evcodebeg && ev <= entry.evcodeend ) )
            {
                return make_pair(true, entry );
            }
            else if( entry.evcodebeg == ev )
            {
                return make_pair( true, entry );
            }
        }
        return make_pair(false, InvalidEventInfo );
    }



//====================================================================================================
//====================================================================================================

};