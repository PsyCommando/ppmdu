#include "smdl.hpp"
#include <ppmdu/pmd2/pmd2_audio_data.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;

namespace DSE
{
//====================================================================================================
// Constants
//====================================================================================================

    //Static values of the Parameters for the Trk Chunk
    static const uint32_t Trk_Chunk_Param1 = 0x1;
    static const uint32_t Trk_Chunk_Param2 = 0xFF04;

    static const TrkEventInfo InvalidEventInfo = {eTrkEventCodes::Invalid,eTrkEventCodes::Invalid,0,false,false,false,0};

    /***************************************************************************************
        TrkEventsTable
            Definition of the TrkEventsTable.
            
            Contains important details on how to parse all individual events.
    ***************************************************************************************/
    const std::array<TrkEventInfo, NB_Track_Events> TrkEventsTable
    {{
        //Play Note event
        {
            eTrkEventCodes::NoteOnBeg,  //Event Codes Range Beginning
            eTrkEventCodes::NoteOnEnd,  //Event Codes Range End
            1,                          //Nb Required Parameters
            true,                       //Can Have Optional Parameter
            false,                      //Is End Of Track Marker
            false,                      //Is Loop Point Marker
            0x40,                       //Bitmask for determining if optional parameter is there //0x40 is (0100 0000)
        },

        //RepeatSilence
        { eTrkEventCodes::RepeatSilence, eTrkEventCodes::Invalid, 0, false, false, false, 0 },

        //Silence
        { eTrkEventCodes::Silence,       eTrkEventCodes::Invalid, 1, false, false, false, 0 },

        //LongSilence
        { eTrkEventCodes::LongSilence,   eTrkEventCodes::Invalid, 2, false, false, false, 0 },

        //EndOfTrack
        { eTrkEventCodes::EndOfTrack,    eTrkEventCodes::Invalid, 0, false, true, false,  0 },

        //LoopPointSet
        { eTrkEventCodes::LoopPointSet, eTrkEventCodes::Invalid, 0, false, false, true,   0 },

        //SetOctave
        { eTrkEventCodes::SetOctave,    eTrkEventCodes::Invalid, 1, false, false, false,  0 },

        //SetTempo
        { eTrkEventCodes::SetTempo,     eTrkEventCodes::Invalid, 1, false, false, false,  0 },

        //SetUnk1
        { eTrkEventCodes::SetUnk1,      eTrkEventCodes::Invalid, 1, false, false, false,  0 },

        //SetUnk2
        { eTrkEventCodes::SetUnk2,      eTrkEventCodes::Invalid, 1, false, false, false,  0 },

        //SetPreset
        { eTrkEventCodes::SetPreset,    eTrkEventCodes::Invalid, 1, false, false, false,  0 },

        //Modulate
        { eTrkEventCodes::Modulate,     eTrkEventCodes::Invalid, 2, false, false, false,  0 },

        //SetUnk3
        { eTrkEventCodes::SetUnk3,      eTrkEventCodes::Invalid, 1, false, false, false,  0 },

        //SetTrkVol
        { eTrkEventCodes::SetTrkVol,    eTrkEventCodes::Invalid, 1, false, false, false,  0 },

        //SetExpress
        { eTrkEventCodes::SetExpress,   eTrkEventCodes::Invalid, 1, false, false, false,  0 },

        //SetTrkPan
        { eTrkEventCodes::SetTrkPan,    eTrkEventCodes::Invalid, 1, false, false, false,  0 },
    }};

//====================================================================================================
// Utility
//====================================================================================================
    std::pair<bool,TrkEventInfo> GetEventInfo( eTrkEventCodes ev )
    {
        for( auto & entry : TrkEventsTable )
        {
            if( entry.evcodebeg == ev )
            {
                return make_pair( true, entry );
            }
        }
        return make_pair(false, InvalidEventInfo );
    }

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
            //Set our iterator
            m_itread = m_src.begin();
            m_itEoC  = DSE::FindNextChunk( m_src.begin(), m_src.end(), DSE::eDSEChunks::eoc ); //Our end is either the eoc chunk, or the vector's end

            //Get the headers
            ParseHeader();
            ParseSong();

            //Build Meta-info
            DSE::DSE_MetaData meta;
            meta.createtime.year    = m_hdr.year;
            meta.createtime.month   = m_hdr.month;
            meta.createtime.day     = m_hdr.day;
            meta.createtime.hour    = m_hdr.hour;
            meta.createtime.minute  = m_hdr.minute;
            meta.createtime.second  = m_hdr.second;
            meta.createtime.centsec = m_hdr.centisec;
            meta.fname              = string( m_hdr.fname.data());
            meta.unk1               = m_hdr.unk1;
            meta.unk2               = m_hdr.unk2;

            //Parse tracks and return
            return pmd2::audio::MusicSequence( ParseAllTracks(), std::move(meta) );
        }

    private:

        //Parse the SMDL header
        inline void ParseHeader()
        {
            m_itread = m_hdr.ReadFromContainer( m_itread );
        }

        //Parse the song chunk
        inline void ParseSong()
        {
            m_itread = m_song.ReadFromContainer(m_itread);
        }

        std::vector< std::vector<TrkEvent> > ParseAllTracks()
        {
            vector< vector<TrkEvent> > tracks;
            tracks.reserve( m_song.nbtrks );

            try
            {
                while( m_itread != m_itEoC )
                {
                    //#1 - Find next track chunk
                    auto itfound = DSE::FindNextChunk( m_itread, m_src.end(), DSE::eDSEChunks::trk );

                    if( itfound != m_src.end() )
                    {
                        //Parse Track
                        tracks.push_back( ParseTrack() );

                        //Increment
                        m_itread = itfound + 1; //+1 so we don't find the same thing over and over again
                    }
                    else
                        m_itread = itfound;
                }
            }
            catch( std::runtime_error & e )
            {
                stringstream sstr;
                sstr << e.what() << " Caught exception while parsing track # " <<tracks.size() <<"! ";
                throw std::runtime_error( sstr.str() );
            }

            return std::move(tracks);
        }

        std::vector<TrkEvent> ParseTrack()
        {
            std::vector<TrkEvent> trk;
            DSE::ChunkHeader      hdr;
            m_itread = hdr.ReadFromContainer(m_itread);

            //Sanity checks
            if( hdr.label != static_cast<uint32_t>(DSE::eDSEChunks::trk) )
            {
                throw std::runtime_error( "SMDL_Parser::ParseTrack() : Unexpected chunk ID ! " + to_string(hdr.label) );
            }
            else if( hdr.param1 != Trk_Chunk_Param1 )
            {
                cerr <<"SMDL_Parser::ParseTrack() : Track chunk's param1 value is non-default " <<hex <<hdr.param1 <<dec <<" !\n";
            }
            else if( hdr.param2 != Trk_Chunk_Param2 )
            {
                cerr <<"SMDL_Parser::ParseTrack() : Track chunk's param2 value is non-default " <<hex <<hdr.param2 <<dec <<" !\n";
            }

            //Set End of track
            auto iteot = m_itread + hdr.datlen;

            //Pre-allocate worst case scenario
            trk.reserve( hdr.datlen );

            //Read preamble
            TrkPreamble pre;
            m_itread = pre.ReadFromContainer(m_itread);

            //Parse events
            TrkEvent lastev;
            while( (lastev.evcode != static_cast<uint8_t>(DSE::eTrkEventCodes::EndOfTrack)) && 
                   (m_itread != m_itEoC) )
            {
                trk.push_back(ParseEvent());
            }

            //Shrink
            trk.shrink_to_fit();
            return std::move(trk);
        }

        TrkEvent ParseEvent()
        {
            TrkEvent ev;
            uint8_t  first = *m_itread;

            //Check if first val is DT
            if( (first >> 4) == 4 ) //Check if value begins with 0x8X (1000 XXXX)
            {
                ev.dt = first & 0xF; //DT is 0x0-0xF
                ++m_itread; //increment iter
            }
            else
                ev.dt = 0;

            //Get the event code
            ev.evcode = *m_itread;
            ++m_itread;

            //Check if evcode has a/some parameter(s)
            auto einfo = GetEventInfo( static_cast<DSE::eTrkEventCodes>(ev.evcode) );

            //If event doesn't exist in our database
            if( ! einfo.first )
            {
                stringstream sstr;
                sstr << "SMDL_Parser::ParseEvent() : Encountered undefined event code 0x" <<hex <<static_cast<uint16_t>(ev.evcode) <<dec <<" !";
                throw std::runtime_error(sstr.str());
            }

            //Get up to 2 params
            if( einfo.second.nbreqparams >= 1 )
                ev.param1 = *(++m_itread);

            if( ( einfo.second.nbreqparams >= 2 ) || 
                ( einfo.second.hasoptparam && (einfo.second.optparammask & ev.param1) != 0 ) ) //Optional second param
                ev.param2 = *(++m_itread);

            //Done
            return std::move(ev);
        }

    private:
        const vector<uint8_t>         & m_src;
        vector<uint8_t>::const_iterator m_itread;
        vector<uint8_t>::const_iterator m_itEoC;    //Pos of the "end of chunk" chunk
        SMDL_Header                     m_hdr;
        SongChunk                       m_song;

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