#include "pmd2_audio_data.hpp"
#include <ppmdu/fmts/dse_common.hpp>
#include <ppmdu/fmts/sedl.hpp>
#include <ppmdu/fmts/smdl.hpp>
#include <ppmdu/fmts/swdl.hpp>
#include <sstream>
#include <iomanip>

using namespace std;
using namespace DSE;

namespace pmd2 { namespace audio
{
    class TrackPlaybackState
    {
    public:

        /*typedef DSE::EvTrack::track_t::const_iterator ittrk_t;*/

        //A simplified event structure. Everything is fully interpreted.
        //struct plainevent
        //{
        //    uint8_t        dt;
        //    eTrkEventCodes ev;

        //    //Play note specifics
        //    uint8_t        vel;
        //    eNote          note;


        //};

        TrackPlaybackState( /*ittrk_t beg, ittrk_t end*/ )
            :/*m_beg(beg), m_end(end), m_loop(beg), m_cur(beg),*/ curpitch(0), curbpm(0), lastsilence(0), curvol(0), curexp(0), curpreset(0), curpan(0)
        {}

        std::string printevent( const TrkEvent & ev )
        {
            stringstream outstr;
        
            //Print Delta Time
            if( ev.dt != 0 )
                outstr <<dec << static_cast<unsigned short>(ev.dt & 0x0F) <<"tu-";

            auto evinf = GetEventInfo( static_cast<eTrkEventCodes>(ev.evcode) );

            //Print Event Label
            if( evinf.first )
                outstr << evinf.second.evlbl << "-";
            else
                outstr << "INVALID-";

            //Print Parameters and Event Specifics
            if( evinf.second.nbreqparams == 1 )
            {
                if( evinf.second.evcodebeg == eTrkEventCodes::NoteOnBeg )
                {
                    outstr << "( vel:" <<dec << static_cast<unsigned short>(ev.evcode) <<", TrkPitch:";
                    uint8_t prevpitch = curpitch;
                    uint8_t pitchop   = (NoteEvParam1PitchMask & ev.param1);

                    if( pitchop == static_cast<uint8_t>(eNotePitch::lower) ) 
                        curpitch -= 1;
                    else if( pitchop == static_cast<uint8_t>(eNotePitch::higher) ) 
                        curpitch += 1;

                    outstr <<dec <<static_cast<short>(prevpitch) <<"->" <<dec <<static_cast<short>(curpitch);
                    
                    outstr <<", note: ";
                    
                    //Ugly but just for debug...
                    switch( static_cast<eNote>(ev.param1 & NoteEvParam1NoteMask) )
                    {

                        case eNote::C:  outstr <<"C";  break;
                        case eNote::Cs: outstr <<"C#"; break;
                        case eNote::D:  outstr <<"D";  break;
                        case eNote::Ds: outstr <<"D#"; break;
                        case eNote::E:  outstr <<"E";  break;
                        case eNote::F:  outstr <<"F";  break;
                        case eNote::Fs: outstr <<"F#"; break;
                        case eNote::G:  outstr <<"G";  break;
                        case eNote::Gs: outstr <<"G#";  break;
                        case eNote::A:  outstr <<"A";  break;
                        case eNote::As: outstr <<"A#"; break;
                        case eNote::B:  outstr <<"B";  break;
                    };
                    outstr <<dec <<static_cast<short>(curpitch) <<" )";
                }
                else if( evinf.second.evcodebeg == eTrkEventCodes::SetOctave )
                {
                    outstr <<"( TrkPitch: ";
                    uint8_t prevpitch = curpitch;
                    curpitch = ev.param1;
                    outstr <<dec <<static_cast<short>(prevpitch) <<"->" <<dec <<static_cast<short>(curpitch) <<" )";
                }
                else if( evinf.second.evcodebeg == eTrkEventCodes::SetExpress )
                {
                    outstr <<"( TrkExp: ";
                    int8_t prevexp = curexp;
                    curexp = ev.param1;
                    outstr <<dec <<static_cast<short>(prevexp) <<"->" <<dec <<static_cast<short>(curexp) <<" )";
                }
                else if( evinf.second.evcodebeg == eTrkEventCodes::SetTrkVol )
                {
                    outstr <<"( Vol: ";
                    int8_t prevvol = curvol;
                    curvol = ev.param1;
                    outstr <<dec <<static_cast<short>(prevvol) <<"->" <<dec <<static_cast<short>(curvol) <<" )";
                }
                else if( evinf.second.evcodebeg == eTrkEventCodes::SetTrkPan )
                {
                    outstr <<"( pan: ";
                    int8_t prevpan = curpan;
                    curpan = ev.param1;
                    outstr <<dec <<static_cast<short>(prevpan) <<"->" <<dec <<static_cast<short>(curpan) <<" )";
                }
                else if( evinf.second.evcodebeg == eTrkEventCodes::SetPreset )
                {
                    outstr <<"( Prgm: ";
                    uint8_t prevpreset = curpreset;
                    curpreset = ev.param1;
                    outstr <<dec <<static_cast<unsigned short>(prevpreset) <<"->" <<dec <<static_cast<unsigned short>(curpreset) <<" )";
                }
                else if( evinf.second.evcodebeg == eTrkEventCodes::SetTempo )
                {
                    outstr <<"( tempo: ";
                    int8_t prevbpm = curbpm;
                    curbpm = ev.param1;
                    outstr <<dec <<static_cast<unsigned short>(prevbpm) <<"->" <<dec <<static_cast<unsigned short>(curbpm) <<" )";
                }
                else
                    outstr << "( param1: 0x" <<hex <<static_cast<unsigned short>(ev.param1) <<" )" <<dec;
            }

            //Print Event with 2 params or a int16 as param
            if( ( evinf.second.nbreqparams == 2 ) )
            {
                if( evinf.second.evcodebeg == eTrkEventCodes::LongPause )
                {
                    uint16_t duration = ( static_cast<uint16_t>(ev.param2 << 8) | static_cast<uint16_t>(ev.param1) );
                    outstr << "( duration: 0x" <<hex <<duration <<" )" <<dec;
                }
                else
                    outstr << "( param2: 0x" <<hex <<static_cast<unsigned short>(ev.param2) <<" )" <<dec;
            }

            if( ( evinf.second.evcodebeg == eTrkEventCodes::NoteOnBeg && (NoteEvParam1OptParamMask & ev.param1) > 0 ) )
            {
                outstr << "( param2: 0x" <<hex <<static_cast<unsigned short>(ev.param2) <<" )" <<dec;
            }

            return outstr.str();
        }

    private:
        //ittrk_t m_beg;
        //ittrk_t m_loop;
        //ittrk_t m_cur;
        //ittrk_t m_end;

        int8_t   curvol;
        int8_t   curpan;
        int8_t   curexp;
        uint8_t  curpitch;
        uint8_t  curbpm;
        uint8_t  curpreset;
        uint16_t lastsilence;
    };

    std::string MusicSequence::tostr()const
    {
        stringstream sstr;
        int cnt = 0;
        for( const auto & track : m_tracks )
        {
            sstr <<"==== Track" <<cnt << " ====\n\n";
            TrackPlaybackState st;
            for( const auto & ev : track )
            {
                sstr << st.printevent( ev ) << "\n";
            }
            ++cnt;
        }

        return move(sstr.str());
    }

    MusicSequence LoadSequence( const std::string & file )
    {
        return DSE::ParseSMDL( file );
    }




};};