#include "smdl.hpp"

using namespace std;

namespace DSE
{
//====================================================================================================
// Constants
//====================================================================================================

    /***************************************************************************************
        TrkEventsTable
            Definition of the TrkEventsTable.
            
            Contains important details on how to parse all individual events.
    ***************************************************************************************/
    const std::array<TrkEventInfo, NB_Track_Events> TrkEventsTable = 
    {{
        //Play Note event
        {
            eTrkEventCodes::NoteOnBeg,  //Event Codes Range Beginning
            eTrkEventCodes::NoteOnEnd,  //Event Codes Range End
            1,                          //Nb Required Parameters
            true,                       //Can Have Optional Parameter
            false,                      //Is End Of Track Marker
            false,                      //Is Loop Point Marker
        },

        //RepeatSilence
        { eTrkEventCodes::RepeatSilence, eTrkEventCodes::Invalid, 0, false, false, false },

        //Silence
        { eTrkEventCodes::Silence,       eTrkEventCodes::Invalid, 1, false, false, false },

        //LongSilence
        { eTrkEventCodes::LongSilence,   eTrkEventCodes::Invalid, 2, false, false, false },

        //EndOfTrack
        { eTrkEventCodes::EndOfTrack,    eTrkEventCodes::Invalid, 0, false, true, false  },

        //LoopPointSet
        { eTrkEventCodes::LoopPointSet, eTrkEventCodes::Invalid, 0, false, false, true   },

        //SetOctave
        { eTrkEventCodes::SetOctave,    eTrkEventCodes::Invalid, 1, false, false, false  },

        //SetTempo
        { eTrkEventCodes::SetTempo,     eTrkEventCodes::Invalid, 1, false, false, false  },

        //SetUnk1
        { eTrkEventCodes::SetUnk1,      eTrkEventCodes::Invalid, 1, false, false, false  },

        //SetUnk2
        { eTrkEventCodes::SetUnk2,      eTrkEventCodes::Invalid, 1, false, false, false  },

        //SetPreset
        { eTrkEventCodes::SetPreset,    eTrkEventCodes::Invalid, 1, false, false, false  },

        //Modulate
        { eTrkEventCodes::Modulate,     eTrkEventCodes::Invalid, 2, false, false, false  },

        //SetUnk3
        { eTrkEventCodes::SetUnk3,      eTrkEventCodes::Invalid, 1, false, false, false  },

        //SetTrkVol
        { eTrkEventCodes::SetTrkVol,    eTrkEventCodes::Invalid, 1, false, false, false  },

        //SetExpress
        { eTrkEventCodes::SetExpress,   eTrkEventCodes::Invalid, 1, false, false, false  },

        //SetTrkPan
        { eTrkEventCodes::SetTrkPan,    eTrkEventCodes::Invalid, 1, false, false, false  },
    }};


//====================================================================================================
// SMDL_Parser
//====================================================================================================

    class SMDL_Parser
    {
    public:

        SMDL_Parser( const vector<uint8_t> & filedata )
            :m_src(filedata)
        {}

        operator pmd2::audio::MusicSequence()
        {
            pmd2::audio::MusicSequence seq;

            ParseHeader();
            ParseSong();

            //Parse tracks

            return move(seq);
        }

    private:

        //Parse the SMDL header
        void ParseHeader()
        {
        }

        //Parse the song chunk
        void ParseSong()
        {
        }

        void ParseAllTracks()
        {
        }

        void ParseTrack()
        {
        }

    private:
        const vector<uint8_t> & m_src;
    };

//====================================================================================================
// SMDL_Writer
//====================================================================================================


//====================================================================================================
// Functions
//====================================================================================================

    pmd2::audio::MusicSequence ParseSMDL( const std::string & file )
    {
        return SMDL_Parser( utils::io::ReadFileToByteVector(file) );
    }

    void WriteSMDL( const std::string & file, const pmd2::audio::MusicSequence & seq )
    {

    }
};