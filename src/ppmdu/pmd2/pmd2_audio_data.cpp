#include "pmd2_audio_data.hpp"
#include <ppmdu/fmts/dse_common.hpp>
#include <ppmdu/fmts/dse_sequence.hpp>
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

        TrackPlaybackState()
            :curpitch(0), curbpm(0), lastsilence(0), curvol(0), curexp(0), curpreset(0), curpan(0),lastpitchev(0)
        {}

        std::string printevent( const TrkEvent & ev )
        {
            stringstream outstr;
        
            //Print Delta Time
            if( ev.dt != 0 )
            {
                outstr <<dec <<static_cast<uint16_t>( DSE::TrkDelayCodeVals.at(ev.dt) )  <<" ticks-";
            }

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
                    uint8_t pitchop   = (NoteEvParam1PitchMask & ev.params.front());

                    if( pitchop == static_cast<uint8_t>(eNotePitch::lower) ) 
                        curpitch -= 1;
                    else if( pitchop == static_cast<uint8_t>(eNotePitch::higher) ) 
                        curpitch += 1;
                    else if( pitchop == static_cast<uint8_t>(eNotePitch::undefined) ) 
                        curpitch = lastpitchev;

                    outstr <<dec <<static_cast<short>(prevpitch) <<"->" <<dec <<static_cast<short>(curpitch);
                    
                    outstr <<", note: " <<DSE::NoteNames.at( (ev.params.front() & NoteEvParam1NoteMask) );
                    
                    //Ugly but just for debug...
                    //switch( static_cast<eNote>(ev.param1 & NoteEvParam1NoteMask) )
                    //{

                    //    case eNote::C:  outstr <<"C";  break;
                    //    case eNote::Cs: outstr <<"C#"; break;
                    //    case eNote::D:  outstr <<"D";  break;
                    //    case eNote::Ds: outstr <<"D#"; break;
                    //    case eNote::E:  outstr <<"E";  break;
                    //    case eNote::F:  outstr <<"F";  break;
                    //    case eNote::Fs: outstr <<"F#"; break;
                    //    case eNote::G:  outstr <<"G";  break;
                    //    case eNote::Gs: outstr <<"G#";  break;
                    //    case eNote::A:  outstr <<"A";  break;
                    //    case eNote::As: outstr <<"A#"; break;
                    //    case eNote::B:  outstr <<"B";  break;
                    //};
                    outstr <<dec <<static_cast<short>(curpitch) <<" )";
                }
                else if( evinf.second.evcodebeg == eTrkEventCodes::SetOctave )
                {
                    outstr <<"( TrkPitch: ";
                    uint8_t prevpitch = curpitch;
                    lastpitchev = ev.params.front();
                    curpitch    = ev.params.front();
                    outstr <<dec <<static_cast<short>(prevpitch) <<"->" <<dec <<static_cast<short>(curpitch) <<" )";
                }
                else if( evinf.second.evcodebeg == eTrkEventCodes::SetExpress )
                {
                    outstr <<"( TrkExp: ";
                    int8_t prevexp = curexp;
                    curexp = ev.params.front();
                    outstr <<dec <<static_cast<short>(prevexp) <<"->" <<dec <<static_cast<short>(curexp) <<" )";
                }
                else if( evinf.second.evcodebeg == eTrkEventCodes::SetTrkVol )
                {
                    outstr <<"( Vol: ";
                    int8_t prevvol = curvol;
                    curvol = ev.params.front();
                    outstr <<dec <<static_cast<short>(prevvol) <<"->" <<dec <<static_cast<short>(curvol) <<" )";
                }
                else if( evinf.second.evcodebeg == eTrkEventCodes::SetTrkPan )
                {
                    outstr <<"( pan: ";
                    int8_t prevpan = curpan;
                    curpan = ev.params.front();
                    outstr <<dec <<static_cast<short>(prevpan) <<"->" <<dec <<static_cast<short>(curpan) <<" )";
                }
                else if( evinf.second.evcodebeg == eTrkEventCodes::SetPreset )
                {
                    outstr <<"( Prgm: ";
                    uint8_t prevpreset = curpreset;
                    curpreset = ev.params.front();
                    outstr <<dec <<static_cast<unsigned short>(prevpreset) <<"->" <<dec <<static_cast<unsigned short>(curpreset) <<" )";
                }
                else if( evinf.second.evcodebeg == eTrkEventCodes::SetTempo )
                {
                    outstr <<"( tempo: ";
                    int8_t prevbpm = curbpm;
                    curbpm = ev.params.front();
                    outstr <<dec <<static_cast<unsigned short>(prevbpm) <<"->" <<dec <<static_cast<unsigned short>(curbpm) <<" )";
                }
                else
                    outstr << "( param1: 0x" <<hex <<static_cast<unsigned short>(ev.params.front()) <<" )" <<dec;
            }

            //Print Event with 2 params or a int16 as param
            if( ( evinf.second.nbreqparams == 2 ) )
            {
                if( evinf.second.evcodebeg == eTrkEventCodes::LongPause )
                {
                    uint16_t duration = ( static_cast<uint16_t>(ev.params[1] << 8) | static_cast<uint16_t>(ev.params.front()) );
                    outstr << "( duration: 0x" <<hex <<duration <<" )" <<dec;
                }
                else
                    outstr << "( param1: 0x"  <<hex <<static_cast<unsigned short>(ev.params[0]) <<" , param2: 0x" <<static_cast<unsigned short>(ev.params[1]) <<" )" <<dec;
            }

            //if( evinf.second.evcodebeg == eTrkEventCodes::NoteOnBeg  )
            if( ev.params.size() > 2 )
            {
                //const uint8_t nbextraparams = (ev.params.front() & NoteEvParam1NbParamsMask) >> 6; // Get the two highest bits (1100 0000)

                outstr << "( ";

                for( unsigned int i = 0; i < ev.params.size(); ++i )
                {
                    outstr << "param" <<dec <<i <<": 0x" <<hex <<static_cast<unsigned short>(ev.params[i]) <<dec;

                    if( i != (ev.params.size()-1) )
                        outstr << ",";
                    else
                        outstr <<" )";
                }
                
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
        uint8_t  lastpitchev;
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