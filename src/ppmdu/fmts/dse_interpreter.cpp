#include "dse_interpreter.hpp"

#include <jdksmidi/world.h>
#include <jdksmidi/track.h>
#include <jdksmidi/multitrack.h>
#include <jdksmidi/filereadmultitrack.h>
#include <jdksmidi/fileread.h>
#include <jdksmidi/fileshow.h>
#include <jdksmidi/filewritemultitrack.h>

using namespace std;

namespace DSE
{
    static const string UtilityID = "ExportedWith: ppmd_audioutil.exe ver0.1";

    //
    //
    //



    class DSESequenceToMidi
    {

        struct TrkState
        {
            uint32_t ticks_        = 0;
            uint32_t eventno_      = 0; //Event counter to identify a single problematic event
            uint32_t lastpause_    = 0;
            uint32_t lasthold_     = 0; //last duration a note was held
            int8_t   octave_       = 0;
            int8_t   lastoctaveev_ = 0;
            uint8_t  prgm_         = 0; //Keep track of the current program to apply pitch correction on specific instruments
            bool     sustainon     = false; //When a note is sustained, it must be let go of at the next play note event
            uint8_t  curbank_      = 0;
            uint32_t curloop_      = 0; //keep tracks of how many times the track has looped so far
        };

    public:
        DSESequenceToMidi( const std::string                & outmidiname, 
                           const pmd2::audio::MusicSequence & seq, 
                           const std::map<uint16_t,uint16_t>& presetconvtable,
                           eMIDIFormat                        midfmt,
                           eMIDIMode                          mode,
                           uint32_t                           nbloops = 0 )
            :m_fnameout(outmidiname), m_seq(seq), m_banktable(presetconvtable), m_midifmt(midfmt),m_midimode(mode),m_nbloops(nbloops)
        {}

        void operator()()
        {
            using namespace jdksmidi;

            if( m_midifmt == eMIDIFormat::SingleTrack )
                ExportAsSingleTrack();
            else if( m_midifmt == eMIDIFormat::MultiTrack )
                ExportAsMultiTrack();

            //Sort the tracks
            m_midiout.SortEventsOrder();

            //Then write the output!
            MIDIFileWriteStreamFileName out_stream( m_fnameout.c_str() );

            // then output the stream like my example does, except setting num_tracks to match your data

            if( out_stream.IsValid() )
            {
                // the object which takes the midi tracks and writes the midifile to the output stream
                MIDIFileWriteMultiTrack writer( &m_midiout, &out_stream );

                // write the output file
                if( m_midifmt == eMIDIFormat::SingleTrack )
                {
                    if ( !writer.Write( 1 ) )
                        throw std::runtime_error("DSESequenceToMidi::operator(): JDKSMidi failed while writing the MIDI file!");
                }
                else if( m_midifmt == eMIDIFormat::MultiTrack )
                {
                    if ( !writer.Write( m_midiout.GetNumTracks() ) )
                        throw std::runtime_error("DSESequenceToMidi::operator(): JDKSMidi failed while writing the MIDI file!");
                }
            }
            else
            {
                stringstream sstr;
                sstr << "DSESequenceToMidi::operator(): Couldn't open file " <<m_fnameout <<" for writing !";
                throw std::runtime_error(sstr.str());
            }
        }

    private:

        static uint32_t ConvertTempoToMicrosecPerQuarterNote( uint32_t bpm )
        {
            static const uint32_t NbMicrosecPerMinute = 60000000;
            return NbMicrosecPerMinute / bpm;
        }

        void HandlePlayNote( uint16_t trkno, uint16_t trkchan, TrkState & state, const DSE::TrkEvent & ev, jdksmidi::MIDITrack & outtrack )
        {
            using namespace jdksmidi;
            MIDITimedBigMessage mess;
            //Turn off sustain if neccessary
            if( state.sustainon )
            {
                MIDITimedBigMessage susoff;
                susoff.SetTime(state.ticks_);
                susoff.SetControlChange( trkchan, 66, 0 ); //sustainato
                m_midiout.GetTrack(trkno)->PutEvent( susoff );
                state.sustainon = false;
            }

            if( (ev.params.front() & DSE::NoteEvParam1PitchMask) == static_cast<uint8_t>(DSE::eNotePitch::lower) )
            {
                state.octave_ -= 1;
                if( state.octave_ < 1 || state.octave_ > 9 )
                    cout<<"Decremented octave too low! " <<state.octave_ <<" \n";
            }
            else if( (ev.params.front() & DSE::NoteEvParam1PitchMask) == static_cast<uint8_t>(DSE::eNotePitch::higher) )
            {
                state.octave_ += 1;
                if( state.octave_ < 1 || state.octave_ > 9 )
                    cout<<"Incremented octave too high! " <<state.octave_ <<" \n";
            }
            else if( (ev.params.front() & DSE::NoteEvParam1PitchMask) == static_cast<uint8_t>(DSE::eNotePitch::reset) )
            {
                state.octave_ = state.lastoctaveev_;
            }

            int8_t notenb  = (ev.params.front() & 0x0F);
            if( notenb > 0xB )
                cout<<"Warning: Got a note higher than 0xB !\n";

            int8_t mnoteid = notenb + ( (state.octave_) * 12 ); //Midi notes begins at -1 octave, while DSE ones at 0..
            if( static_cast<uint8_t>(mnoteid) > 127 )
                cout<<"Warning: Got a MIDI note ID higher than 127 ! (0x" <<hex <<uppercase <<static_cast<unsigned short>(mnoteid) <<nouppercase <<dec <<")\n";

            mess.SetTime(state.ticks_);
            mess.SetNoteOn( trkchan, mnoteid, static_cast<uint8_t>(ev.evcode & 0x7F) );
            m_midiout.GetTrack(trkno)->PutEvent( mess );
                     
            if( ev.params.size() >= 2 )
            {
                if( ev.params.size() == 2 )
                    state.lasthold_ = ev.params[1];
                else if( ev.params.size() == 3 )
                    state.lasthold_ = static_cast<uint16_t>( ev.params[1] << 8 ) | ev.params[2];
                else if( ev.params.size() == 4 )
                {
                    state.lasthold_ = static_cast<uint32_t>( ev.params[1] << 16 ) | ( ev.params[2] << 8 ) | ev.params[3];
                    cout<<"##Got Note Event with 3 bytes long hold! Parsed as " <<state.lasthold_ <<"!##" <<"( trk#" <<trkno <<", evt #" <<state.eventno_ << ")" <<"\n";
                }
            }
            MIDITimedBigMessage noteoff;
            noteoff.SetTime( state.ticks_ + state.lasthold_ );
            noteoff.SetNoteOff( trkchan, mnoteid, static_cast<uint8_t>(ev.evcode & 0x7F) ); //Set proper channel from original track eventually !!!!

            m_midiout.GetTrack(trkno)->PutEvent( noteoff );
        }

        void HandleEvent( uint16_t trkno, uint16_t trkchan, TrkState & state, const DSE::TrkEvent & ev, jdksmidi::MIDITrack & outtrack )
        {
            using namespace jdksmidi;
            MIDITimedBigMessage mess;
            const auto code = static_cast<DSE::eTrkEventCodes>(ev.evcode);

            //Handle delta-time
            if( ev.dt != 0 )
            {
                if( (ev.dt & 0xF0) != 0x80 )
                {
                    cerr << "Bad delta-time ! " <<"( trk#" <<trkno <<", evt #" <<state.eventno_ << ")" <<"\n";
                }
                else
                    state.ticks_ += static_cast<uint8_t>( DSE::TrkDelayCodeVals.at( ev.dt ) );
            }

            //Set the time properly now
            mess.SetTime(state.ticks_);

            switch( code )
            {
                case eTrkEventCodes::SetTempo:
                {
                    mess.SetTempo( ConvertTempoToMicrosecPerQuarterNote(ev.params.front()) );
                    m_midiout.GetTrack(trkno)->PutEvent( mess );
                    break;
                }
                //Pauses
                case eTrkEventCodes::LongPause:
                {
                    state.lastpause_ = (static_cast<uint16_t>(ev.params.back()) << 8) | ev.params.front();
                    state.ticks_     += state.lastpause_;
                    break;
                }
                case eTrkEventCodes::Pause:
                {
                    state.lastpause_ = ev.params.front();
                    state.ticks_     += state.lastpause_;
                    break;
                }
                case eTrkEventCodes::AddToLastPause:
                {
                    uint32_t prelastp = state.lastpause_;
                    state.lastpause_  = prelastp + ev.params.front();
                    state.ticks_     += state.lastpause_;
                    break;
                }
                case eTrkEventCodes::RepeatLastPause:
                {
                    state.ticks_ += state.lastpause_;
                    break;
                }

                //
                case eTrkEventCodes::SetOctave:
                {
                    state.lastoctaveev_ = ev.params.front();    
                    if( state.lastoctaveev_ > 9 )
                        cout<<"New octave value set is too high !" <<state.lastoctaveev_ <<"\n";

                    state.octave_       = state.lastoctaveev_;
                    break;
                }

                case eTrkEventCodes::SetExpress:
                {
                    mess.SetControlChange( trkchan, 0x0B, ev.params.front() );
                    m_midiout.GetTrack(trkno)->PutEvent( mess );
                    break;
                }
                case eTrkEventCodes::SetTrkVol:
                {
                    mess.SetControlChange( trkchan, 0x07, ev.params.front() );
                    m_midiout.GetTrack(trkno)->PutEvent( mess );
                    break;
                }
                case eTrkEventCodes::SetTrkPan:
                {
                    mess.SetControlChange( trkchan, 0x0A, ev.params.front() );
                    m_midiout.GetTrack(trkno)->PutEvent( mess );
                    break;
                }
                case eTrkEventCodes::SetPreset:
                {
                    //Select the correct bank
                    MIDITimedBigMessage banksel;
                    banksel.SetTime(state.ticks_);
                    state.curbank_ = static_cast<uint8_t>(m_banktable.at(ev.params.front()));
                    banksel.SetControlChange( m_seq[trkno].GetMidiChannel(), jdksmidi::C_GM_BANK, state.curbank_ );
                    m_midiout.GetTrack(trkno)->PutEvent(banksel);

                    //Then preset
                    //Keep track of the current program to apply pitch correction on instruments that need it..
                    state.prgm_ = ev.params.front();
                    mess.SetProgramChange( trkchan, state.prgm_ );
                    m_midiout.GetTrack(trkno)->PutEvent( mess );
                    break;
                }
                case eTrkEventCodes::PitchBend:
                {
                    mess.SetPitchBend( trkchan, ( (ev.params.front() << 8) | ev.params.back() ) );
                    m_midiout.GetTrack(trkno)->PutEvent( mess );
                    break;
                }
                case eTrkEventCodes::HoldNote:
                {
                    static const std::string TXT_HoldNote = "HoldNote";
                    m_midiout.GetTrack(trkno)->PutTextEvent( state.ticks_, META_GENERIC_TEXT, TXT_HoldNote.c_str(), TXT_HoldNote.size() );

                    //Put a sustenato
                    state.sustainon = true;
                    mess.SetControlChange( trkchan, 66, 127 ); //sustainato
                    m_midiout.GetTrack(trkno)->PutEvent( mess );
                    break;
                }
                case eTrkEventCodes::LoopPointSet:
                {
                    mess.SetMetaType(META_TRACK_LOOP);
                    static const std::string TXT_Loop = "LoopStart";
                    m_midiout.GetTrack(trkno)->PutTextEvent( state.ticks_, META_MARKER_TEXT, TXT_Loop.c_str(), TXT_Loop.size() );
                    m_midiout.GetTrack(trkno)->PutEvent( mess );
                    break;
                }

                //
                default:
                {
                    //Play note are handled here
                    if( code >= DSE::eTrkEventCodes::NoteOnBeg && code <= DSE::eTrkEventCodes::NoteOnEnd ) //Handle play note
                        HandlePlayNote( trkno, trkchan, state, ev, outtrack );
                    else
                    {
                    }

                }
            };
            //Event done increment event counter
            state.eventno_ += 1;
        }

        void PrepareMidiFile()
        {
            using namespace jdksmidi;
            //Setup Common Data
            //m_midiout = MIDIMultiTrack( m_trkstates.size() );
            m_midiout.SetClksPerBeat( m_seq.metadata().tpqn );

            //Put a XG or GS sysex message if specified
            if( m_midimode == eMIDIMode::GS )
            {
                {
                    MIDITimedBigMessage gsreset;

                    gsreset.SetTime(0);
                    gsreset.SetSysEx(jdksmidi::SYSEX_START_N);

                    MIDISystemExclusive mygssysex;
                    mygssysex.PutEXC();
                    mygssysex.PutByte(0x41); //Roland's ID
                    mygssysex.PutByte(0x10); //Device ID, 0x10 is default 
                    mygssysex.PutByte(0x42); //Model ID, 0x42 is universal for Roland
                    mygssysex.PutByte(0x12); //0x12 means we're sending data 

                    mygssysex.PutByte(0x40); //highest byte of address
                    mygssysex.PutByte(0x00); //mid byte of address
                    mygssysex.PutByte(0x7F); //lowest byte of address

                    mygssysex.PutByte(0x00); //data

                    mygssysex.PutByte(0x41); //checksum
                    mygssysex.PutEOX();

                    gsreset.CopySysEx( &mygssysex );
                    m_midiout.GetTrack(0)->PutEvent(gsreset);
                }
                {
                    //Now send the message to turn off the drum channel!
                    MIDITimedBigMessage gsoffdrums;
                    gsoffdrums.SetSysEx(jdksmidi::SYSEX_START_N);

                    MIDISystemExclusive drumsysex;
                    drumsysex.PutByte(0x41); //Roland's ID
                    drumsysex.PutByte(0x10); //Device ID, 0x10 is default 
                    drumsysex.PutByte(0x42); //Model ID, 0x42 is universal for Roland
                    drumsysex.PutByte(0x12); //0x12 means we're sending data 

                    drumsysex.PutByte(0x40); //highest byte of address
                    drumsysex.PutByte(0x10); //mid byte of address
                    drumsysex.PutByte(0x15); //lowest byte of address

                    drumsysex.PutByte(0x00); //data

                    drumsysex.PutByte(0x1B); //checksum
                    drumsysex.PutEOX();
                    gsoffdrums.CopySysEx( &drumsysex );
                    m_midiout.GetTrack(0)->PutEvent(gsoffdrums);
                }
            }
            else if( m_midimode == eMIDIMode::XG )
            {
                //Ugh.. I have no clue if that's how I should do this.. 
                // Stupid JDKSmidi has 0 documentation and some of the most 
                // incoherent and unintuitive layout I've seen.. 
                // Though I've actually seen worse..
                MIDITimedBigMessage xgreset;
                xgreset.SetTime(0);
                //xgreset.SetSysEx(jdksmidi::SYSEX_START_N);

                std::array<uint8_t,9> XG_SysEx{{0x43,0x10,0x4C,0x00,0x00,0x7E,0x00}};
                MIDISystemExclusive mysysex( XG_SysEx.data(), XG_SysEx.size(), XG_SysEx.size(), false );
                xgreset.SetDataLength(9);
                //mysysex.PutEXC();
                //mysysex.PutByte(0x43); //Yamaha's ID
                //mysysex.PutByte(0x10); //Device ID, 0x10 is default 
                //mysysex.PutByte(0x4c);
                //mysysex.PutByte(0x00);
                //mysysex.PutByte(0x00);
                //mysysex.PutByte(0x7E);
                //mysysex.PutByte(0x00);
                //mysysex.PutEOX();

                xgreset.CopySysEx( &mysysex );
                xgreset.SetSysEx(jdksmidi::SYSEX_START_N);
                
                m_midiout.GetTrack(0)->PutEvent(xgreset);
            }

            //Init track 0 with time signature
            MIDITimedBigMessage timesig;
            timesig.SetTime( 0 );
            timesig.SetTimeSig();
            m_midiout.GetTrack( 0 )->PutEvent( timesig );
            m_midiout.GetTrack( 0 )->PutTextEvent( 0, META_TRACK_NAME, m_seq.metadata().fname.c_str(), m_seq.metadata().fname.size() );
            m_midiout.GetTrack( 0 )->PutTextEvent( 0, META_GENERIC_TEXT, UtilityID.c_str(), UtilityID.size() );
        }

        void ExportAsMultiTrack()
        {
            using namespace jdksmidi;
            //Setup our track states
            m_trkstates.resize( m_seq.getNbTracks() );

            //Setup the time signature and etc..
            PrepareMidiFile();

            vector<size_t>   looppoints(m_seq.getNbTracks(), 0);
            vector<TrkState> savedstates(m_seq.getNbTracks()); //Save the channel's state right before the loop point

            //Play all tracks at least once
            for( unsigned int trkno = 0; trkno < m_seq.getNbTracks(); ++trkno )
            {
                for( size_t evno = 0; evno < m_seq.track(trkno).size(); ++evno )
                {
                    //Mark the loop position for each tracks
                    if( m_seq[trkno][evno].evcode == static_cast<uint8_t>( DSE::eTrkEventCodes::LoopPointSet ) )
                    {
                        looppoints[trkno]  = (evno + 1); //Add one to avoid duplicating the loop marker
                        savedstates[trkno] = m_trkstates[trkno]; //Save the track state
                    }

                    HandleEvent( trkno, m_seq[trkno].GetMidiChannel(), m_trkstates[trkno], m_seq[trkno][evno], *(m_midiout.GetTrack(trkno)) );
                }
            }

            //Then, if we're set to loop, then loop
            for( unsigned int nbloops = 0; nbloops < m_nbloops; ++nbloops )
            {
                for( unsigned int trkno = 0; trkno < m_seq.getNbTracks(); ++trkno )
                {
                    //Restore track state
                    uint32_t backticks        = m_trkstates[trkno].ticks_; //Save ticks
                    m_trkstates[trkno]        = savedstates[trkno];        //Overwrite state
                    m_trkstates[trkno].ticks_ = backticks;                 //Restore ticks

                    for( size_t evno = looppoints[trkno]; evno < m_seq.track(trkno).size(); ++evno ) //Begin at the loop point!
                    {
                        HandleEvent( trkno, m_seq[trkno].GetMidiChannel(), m_trkstates[trkno], m_seq[trkno][evno], *(m_midiout.GetTrack(trkno)) );
                    }
                }
            }
        }

        void ExportAsSingleTrack()
        {
            using namespace jdksmidi;
            //Setup our track states
            m_trkstates.resize(m_seq.getNbTracks());

            //Setup the time signature and etc..
            PrepareMidiFile();

            vector<size_t>   looppoints(m_seq.getNbTracks(), 0);
            vector<TrkState> savedstates(m_seq.getNbTracks()); //Save the channel's state right before the loop point

            //Play all tracks at least once
            for( unsigned int trkno = 0; trkno < m_seq.getNbTracks(); ++trkno )
            {
                for( size_t evno = 0; evno < m_seq.track(trkno).size(); ++evno )
                {
                    //Mark the loop position for each tracks
                    if( m_seq[trkno][evno].evcode == static_cast<uint8_t>( DSE::eTrkEventCodes::LoopPointSet ) )
                    {
                        looppoints[trkno]  = (evno + 1); //Add one to avoid duplicating the loop marker
                        savedstates[trkno] = m_trkstates[trkno]; //Save the track state
                    }

                    HandleEvent( trkno, m_seq[trkno].GetMidiChannel(), m_trkstates[trkno], m_seq[trkno][evno], *(m_midiout.GetTrack(0)) );
                }
            }

            //Then, if we're set to loop, then loop
            for( unsigned int nbloops = 0; nbloops < m_nbloops; ++nbloops )
            {
                for( unsigned int trkno = 0; trkno < m_seq.getNbTracks(); ++trkno )
                {
                    //Restore track state
                    uint32_t backticks        = m_trkstates[trkno].ticks_; //Save ticks
                    m_trkstates[trkno]        = savedstates[trkno];        //Overwrite state
                    m_trkstates[trkno].ticks_ = backticks;                 //Restore ticks

                    for( size_t evno = looppoints[trkno]; evno < m_seq.track(trkno).size(); ++evno ) //Begin at the loop point!
                    {
                        HandleEvent( trkno, m_seq[trkno].GetMidiChannel(), m_trkstates[trkno], m_seq[trkno][evno], *(m_midiout.GetTrack(0)) );
                    }
                }
            }
        }

    private:
        const std::string                & m_fnameout;
        const pmd2::audio::MusicSequence & m_seq;
        const std::map<uint16_t,uint16_t>& m_banktable;
        uint32_t                           m_nbloops;
        eMIDIFormat                        m_midifmt;
        eMIDIMode                          m_midimode;

        //State variables
        std::vector<TrkState>              m_trkstates;

        jdksmidi::MIDIMultiTrack           m_midiout;
    };



    //
    //
    //
    void SequenceToMidi( const std::string                 & outmidi, 
                         const pmd2::audio::MusicSequence  & seq, 
                         const std::map<uint16_t,uint16_t> & presetbanks,
                         eMIDIFormat                         midfmt,
                         eMIDIMode                           midmode )
    {
        DSESequenceToMidi( outmidi, seq, presetbanks, midfmt, midmode )();
    }
};