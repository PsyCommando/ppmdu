#include "smdl.hpp"
#include <ppmdu/pmd2/pmd2_audio_data.hpp>
#include <ppmdu/fmts/dse_sequence.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <mutex>
#include <atomic>

using namespace std;

namespace DSE
{
//====================================================================================================
// Constants
//====================================================================================================

    //Static values of the Parameters for the Trk Chunk
    static const uint32_t Trk_Chunk_Param1 = 0x1000000;
    static const uint32_t Trk_Chunk_Param2 = 0xFF04;



//====================================================================================================
// Utility
//====================================================================================================



//====================================================================================================
//  EvTrack
//====================================================================================================

    //template<class _EvTy, class _PlayIt, class _TickTy = unsigned long long>
    //    class BaseTrackPlaybackState
    //{
    //public:
    //    typedef _EvTy   event_t;
    //    typedef _PlayIt playit_t; //playback iterator
    //    typedef _TickTy tick_t;   //Type of the tick counter

    //    BaseTrackPlaybackState()
    //        :nextop(0),curtick(0),curpitch(0),curbpm(0),lastsilence(0)
    //    {}

    //    virtual ~BaseTrackPlaybackState(){}

    //    //Methods
    //    virtual void tick() 
    //    { 
    //        ++curtick; 
    //    }

    //    virtual void update()
    //    {
    //        if( m_isrealtime && m_curtick >= m_nextop )
    //        {
    //            if( m_tohandle != m_itend )
    //            {
    //            }
    //            else
    //            {
    //                ++m_itcur;
    //                m_tohandle = m_itcur
    //            }
    //            
    //        }
    //        else
    //        {
    //            ++m_itcur;
    //        }
    //    }

    //protected:

    //    virtual void HandleCurEvent()
    //    {
    //    }

    //protected:
    //    
    //    bool           m_isrealtime;    //whether is realtime or not
    //    atomic<tick_t> m_curtick;       //Cur tick counter value
    //    atomic<tick_t> m_nextop;        //Value of the tick counter when the next operation is executed

    //    uint8_t  m_curpitch;
    //    uint8_t  m_curbpm;
    //    uint16_t m_lastsilence;

    //    playit_t m_tohandle;      //The event to handle when curtick == nextop 

    //    playit_t m_itcur;
    //    playit_t m_itloop;
    //    playit_t m_itend;
    //};


    //class BaseEvent
    //{
    //public:
    //    BaseEvent(){}
    //    virtual ~BaseEvent(){}


    //};






    //std::string EvTrack::tostr()const
    //{
    //    //auto     itread = m_events.begin();
    //    //uint8_t      curpitch;
    //    //uint8_t      curbpm;
    //    //uint16_t     lastsilence;
    //    TrackPlaybackState pl;
    //    stringstream outstr;


    //    for( const auto & ev : m_events )
    //    {
    //        outstr << pl.printevent(ev) << " ";
    //    }

    //    return outstr.str();
    //}

    
    //std::string TrkEvent::tostr()const
    //{
    //    stringstream outstr;

    //    if( dt != 0 )
    //        outstr <<dec << static_cast<unsigned short>(dt) <<"tu-";

    //    auto evinf = GetEventInfo( static_cast<eTrkEventCodes>(evcode) );

    //    if( evinf.first )
    //        outstr << evinf.second.evlbl << "-";
    //    else
    //        outstr << "INVALID-";

    //    if( evinf.second.nbreqparams >= 1 )
    //    {
    //        if( evinf.second.evcodebeg == eTrkEventCodes::NoteOnBeg )
    //        {
    //            outstr << "vel:" <<dec << evcode <<"-TrkPitch:";
    //            
    //            if(  ) 
    //            << <<"";
    //        }
    //    }

    //    outstr <<dec << static_cast<unsigned short>(dt) <<"tu-";

    //    return outstr.str();
    //}

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
            //Skip padding
            std::advance( m_itread, 8 );
        }

        //Parse the song chunk
        inline void ParseSong()
        {
            m_itread = m_song.ReadFromContainer(m_itread);
        }

        std::vector<pmd2::audio::MusicTrack> ParseAllTracks()
        {
            vector<pmd2::audio::MusicTrack> tracks;
            tracks.reserve( m_song.nbtrks );

            try
            {
                while( m_itread != m_itEoC )
                {
                    //#1 - Find next track chunk
                    m_itread = DSE::FindNextChunk( m_itread, m_itEoC, DSE::eDSEChunks::trk );
                    if( m_itread != m_itEoC )
                    {
                        //Parse Track
                        tracks.push_back( ParseTrack() );
                    }
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

        pmd2::audio::MusicTrack ParseTrack()
        {
            //std::vector<TrkEvent> trk;
            DSE::ChunkHeader      hdr;
            hdr.ReadFromContainer(m_itread); //Don't increment itread
            auto itend     = m_itread + (hdr.datlen + DSE::ChunkHeader::size());
            auto itpreread = m_itread;
            m_itread = itend; //move it past the chunk already

            auto parsed = DSE::ParseTrkChunk(itpreread, itend);

            pmd2::audio::MusicTrack mtrk;
            mtrk.SetMidiChannel( parsed.second.chanid );
            mtrk.getEvents() = move(parsed.first);

            return move(mtrk);

            ////Sanity checks
            //if( hdr.label != static_cast<uint32_t>(DSE::eDSEChunks::trk) )
            //{
            //    throw std::runtime_error( "SMDL_Parser::ParseTrack() : Unexpected chunk ID ! " + to_string(hdr.label) );
            //}
            //if( hdr.param1 != Trk_Chunk_Param1 )
            //{
            //    cerr <<"SMDL_Parser::ParseTrack() : Track chunk's param1 value is non-default " <<hex <<hdr.param1 <<dec <<" !\n";
            //}
            //if( hdr.param2 != Trk_Chunk_Param2 )
            //{
            //    cerr <<"SMDL_Parser::ParseTrack() : Track chunk's param2 value is non-default " <<hex <<hdr.param2 <<dec <<" !\n";
            //}

            ////Set End of track
            //auto iteot = m_itread + hdr.datlen;

            ////Pre-allocate worst case scenario
            //trk.reserve( hdr.datlen );

            ////Read preamble
            //TrkPreamble pre;
            //m_itread = pre.ReadFromContainer(m_itread);

            ////Parse events
            //TrkEvent lastev;
            //while( (lastev.evcode != static_cast<uint8_t>(DSE::eTrkEventCodes::EndOfTrack)) && 
            //       (m_itread != m_itEoC) )
            //{
            //    lastev = std::move(ParseEvent());
            //    if( lastev.dt != 0 && (lastev.dt & 0xF0) != 0x80 )
            //        cout<<"DT isssue" <<endl;
            //    trk.push_back(lastev);
            //}

            ////Shrink
            //trk.shrink_to_fit();
        }

        //TrkEvent ParseEvent()
        //{
        //    TrkEvent ev;
        //    uint8_t  first = *m_itread;

        //    //Check if first val is DT
        //    if( (first & 0xF0) == 0x80 ) //Check if value begins with 0x8X (1000 XXXX)
        //    {
        //        ev.dt = first; //& 0xF; //DT is 0x0-0xF //NOTE: we don't know if the DT can't be 0x80.. So we need to leave the full value in there
        //        ++m_itread; //increment iter
        //    }
        //    else
        //        ev.dt = 0;

        //    //Get the event code
        //    ev.evcode = *m_itread;
        //    ++m_itread;

        //    //Check if evcode has a/some parameter(s)
        //    auto einfo = GetEventInfo( static_cast<DSE::eTrkEventCodes>(ev.evcode) );

        //    //If event doesn't exist in our database
        //    if( ! einfo.first )
        //    {
        //        stringstream sstr;
        //        sstr << "SMDL_Parser::ParseEvent() : Encountered reset event code 0x" <<hex <<static_cast<uint16_t>(ev.evcode) <<dec <<" !";
        //        throw std::runtime_error(sstr.str());
        //    }

        //    //Get up to 2 params
        //    if( einfo.second.nbreqparams >= 1 )
        //    {
        //        ev.param1 = *(m_itread);
        //        ++m_itread;
        //    }

        //    if( ( einfo.second.nbreqparams >= 2 ) || 
        //        ( einfo.second.hasoptparam && (einfo.second.optparammask & ev.param1) != 0 ) ) //Optional second param
        //    {
        //        ev.param2 = *(m_itread);
        //        ++m_itread;
        //    }

        //    //Done
        //    return std::move(ev);
        //}

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
        return std::move(SMDL_Parser( utils::io::ReadFileToByteVector(file) )); //Apparently it being an implicit move isn't enough for MSVC..
    }

    void WriteSMDL( const std::string & file, const pmd2::audio::MusicSequence & seq )
    {

    }
};