#include "dse_interpreter.hpp"
#include <utils/poco_wrapper.hpp>
#include <dse/dse_conversion.hpp>

#include <jdksmidi/world.h>
#include <jdksmidi/track.h>
#include <jdksmidi/multitrack.h>
#include <jdksmidi/filereadmultitrack.h>
#include <jdksmidi/fileread.h>
#include <jdksmidi/fileshow.h>
#include <jdksmidi/filewritemultitrack.h>

#include <list>
#include <iomanip>
#include <functional>
#include <numeric>
#include <string>
#include <cmath>
#include <deque>
#include <set>
#include <stack>


#ifndef AUDIOUTIL_VER
    #define AUDIOUTIL_VER "Poochyena"
#endif

using namespace std;

namespace DSE
{
    static const string UtilityID          = "ExportedWith: ppmd_audioutil.exe ver" AUDIOUTIL_VER;
    static const string TXT_LoopStart      = "LoopStart";
    static const string TXT_LoopEnd        = "LoopEnd";
    static const string TXT_HoldNote       = "HoldNote";
    static const string TXT_DSE_Event      = "DSE_Event"; //Marks DSE events that have no MIDI equivalents

    static const int8_t DSE_MaxOctave      = 9; //The maximum octave value possible to handle
    const int           NbMidiKeysInOctave = static_cast<uint8_t>(eNote::nbNotes);
    const uint8_t       MIDIDrumChannel    = 9;

    //const int8_t        NBMidiOctaves      = 128 / NbMidiKeysInOctave; //128 keys total, 12 keys per octave

    static const uint8_t MIDI_CC32_BankLSB = 32; 

    //
    //
    //



//======================================================================================
//  Utility
//======================================================================================
    void AddBankChangeMessage( jdksmidi::MIDITrack & outtrack, uint8_t channel, uint32_t time, bankid_t bank )
    {
        using namespace jdksmidi;
        MIDITimedBigMessage bankselMSB;
        MIDITimedBigMessage bankselLSB;

        bankselMSB.SetTime(time);
        bankselLSB.SetTime(time);

        bankselMSB.SetControlChange( channel, C_GM_BANK,         static_cast<unsigned char>(bank & 0x7F) );
        bankselLSB.SetControlChange( channel, MIDI_CC32_BankLSB, static_cast<unsigned char>( (bank >> 8) & 0x7F ) );
        outtrack.PutEvent(bankselMSB);
        outtrack.PutEvent(bankselLSB);
    }


//======================================================================================
//  DSESequenceToMidi_Improv
//======================================================================================
#if 0
    /*
        Improved version of the DSE MIDI Sequence converter.

        This version will work based on MIDI channels
        It will process all DSE tracks together at the same time. So it will be much easier
        to process the effect of an event, and pick an output MIDI channel for it.

        We should also keep preset/program states, so we can keep track of polyphony. 
        And then, we need to see what presets are playing at the same time on other channels, 
        and "prioritize" some based on their keygroup priority. I suspect it involves making
        one channel louder than the other by a certain amount of decibel!
    */
    class DSESequenceToMidi_Improv
    {
    public:
        typedef uint32_t ticks_t;

        /*
            Holds data about a note event. What time it began, what time it ends on. And all the required
            data to make a midi event.

            Those are used to keep track of the notes played at the same time.
        */
        class NoteEvent
        {
        public:
            NoteEvent( midinote_t noteid, int8_t velocity, uint8_t midichan, ticks_t begtime, ticks_t endtime );

            //This creates a MIDI event from the data contained in the note!
            operator jdksmidi::MIDITimedBigMessage()const;

            //This returns whether the note is finished playing from the time passed in parameters.
            bool IsNoteFinished( ticks_t time )const;

            //This changes the endtime value to the time in ticks specified!
            void CutNoteNow( ticks_t ticks );

            //The midi note to play
            midinote_t NoteID()const;

            //Get or set the target midi channel
            uint8_t MidiChan()const;
            void    MidiChan( uint8_t midichan );

        private:
        };

        /*
            Holds state data for a single dse program.
        */
        class PrgState
        {
        public:

        private:
            deque<NoteEvent> m_notes;
        };

        /*
            Holds state data for a single channel.
        */
        class ChannelState
        {
        public:

        private:
        };

        /*
            Holds the state of a DSE sequencer track
        */
        class TrackState
        {
        public:

            //This makes a DSE tracks determine if it needs to handle an event, and if it does handle it.
            void Process( ticks_t now );

            ticks_t NextEvent()const;

            size_t EventIndex()const;

        private:
            ticks_t m_nextev;   //The time in ticks when the next event will be processed
            size_t  m_evindex;  //The index of the DSE event being processed
        };

        /*
            Contains the sequencer-wide state variables.
        */
        struct SequencerState
        {
            ticks_t            m_globalTicks;  //The ticks the Sequencer is at.
            vector<TrackState> m_TrkState;     //State of all the DSE tracks
            deque<PrgState>    m_prgstates;    //State of all the DSE programs
        };


    private:
        //This calls the Process() method of all tracks
        void ProcessATick();


    private:
        SequencerState        m_CurState;     //Current state of the sequencer
        SequencerState        m_LoopBegState; //Saved state of the sequencer at the beginning of the loop
        const MusicSequence * m_seq;          //The Music sequence we're working on
        
    };
#endif


//======================================================================================
//  DSESequenceToMidi
//======================================================================================
    /*
        DSESequenceToMidi
            Convert a DSE event sequence to MIDI messages, and put them into the target 
            file.
    */ 
    class DSESequenceToMidi
    {
        struct NoteOnData
        {
            midinote_t noteid       = 0;
            uint32_t   noteonticks  = 0;
            uint32_t   noteoffticks = 0;
            size_t     noteonnum    = 0; //Event num of the note on event
            size_t     noteoffnum   = 0; //Event num of the eventual note off event
        };
        /***********************************************************************************
            TrkState
                Structure used for tracking the state of a track, to simulate events 
                having only an effect at runtime.
        ***********************************************************************************/
        struct TrkState
        {
            uint32_t               ticks_          = 0; //The current tick count for the track
            uint32_t               eventno_        = 0; //Event index counter to identify a single problematic event
            uint32_t               lastpause_      = 0; //Duration of the last pause event, including fixed duration pauses.
            uint32_t               lasthold_       = 0; //Last duration a note was held
            uint8_t                octave_         = 0; //The track's current octave
            dsepresetid_t          curprgm_        = 0; //Keep track of the current program to apply pitch correction on specific instruments
            bankid_t               curbank_        = 0;
            int8_t                 curmaxpoly_     = -1; //Maximum polyphony for current preset!

            size_t                 looppoint_      = 0; //The index of the envent after the loop pos
            std::deque<NoteOnData> noteson_; //The notes currently on
            
            bool                   hasinvalidbank  = false; //This is toggled when a bank couldn't be found. It stops all playnote events from playing. 
            
            int8_t                 transpose       = 0; //The nb of octaves to transpose the notes played by this channel!
            
            //Those allows to keep track of when to revert to and from the overriden presets
            bool                   presetoverriden = false;
            bankid_t               ovrbank_        = InvalidBankID;
            dsepresetid_t          ovrprgm_        = InvalidDSEPresetID;
            dsepresetid_t          origdseprgm_        = 0; //The original program ID, not the one that has been remaped
        
            //This is for presets that overrides the channel they're played on
            bool                   chanoverriden   = false;     //Whether the current preset overrides the channel
            uint8_t                ovrchan_        = UCHAR_MAX; //The channel we override to
            uint8_t                chantoreinit    = UCHAR_MAX; //If we did override a note from this track, and played it on another channel, this indicates the channel we need to refresh the current preset + bank on!
        };

    public:
        /***********************************************************************************
        ***********************************************************************************/
        DSESequenceToMidi( const std::string                & outmidiname, 
                           const MusicSequence              & seq, 
                           const SMDLPresetConversionInfo   & remapdata,
                           /*eMIDIFormat                        midfmt,*/
                           eMIDIMode                          mode,
                           uint32_t                           nbloops = 0 )
            :m_fnameout(outmidiname), m_seq(seq)/*, m_midifmt(midfmt)*/,m_midimode(mode),m_nbloops(nbloops),
             m_bLoopBegSet(false), m_bTrackLoopable(false), m_songlsttick(0), 
             m_convtable(&remapdata), 
             m_hasconvtbl(true) //Enable conversion data
        {}

        /***********************************************************************************
        ***********************************************************************************/
        DSESequenceToMidi( const std::string                & outmidiname, 
                           const MusicSequence              & seq, 
                           /*eMIDIFormat                        midfmt,*/
                           eMIDIMode                          mode,
                           uint32_t                           nbloops = 0 )
            :m_fnameout(outmidiname), m_seq(seq)/*, m_midifmt(midfmt)*/,m_midimode(mode),m_nbloops(nbloops),
             m_bLoopBegSet(false), m_bTrackLoopable(false), m_songlsttick(0), 
             m_convtable(nullptr), 
             m_hasconvtbl(false) //Disable conversion data
        {}

        /***********************************************************************************
            operator()
                Execute the conversion.
        ***********************************************************************************/
        void operator()()
        {
            using namespace jdksmidi;

            ExportAsSingleTrack();

            //Sort the tracks
            m_midiout.SortEventsOrder();

            //Then write the output!
            MIDIFileWriteStreamFileName out_stream( m_fnameout.c_str() );

            if( out_stream.IsValid() )
            {
                MIDIFileWriteMultiTrack writer( &m_midiout, &out_stream );

                // write the output file
                if ( !writer.Write( 1 ) )
                    throw std::runtime_error("DSESequenceToMidi::operator(): JDKSMidi failed while writing the MIDI file!");
            }
            else
            {
                stringstream sstr;
                sstr << "DSESequenceToMidi::operator(): Couldn't open file " <<m_fnameout <<" for writing !";
                throw std::runtime_error(sstr.str());
            }
        }

    private:

        /***********************************************************************************
            HandleFixedPauses
                Handle converting the DSE Delta-time and turning into a midi time stamp.
        ***********************************************************************************/
        inline void HandleFixedPauses( const DSE::TrkEvent & ev, 
                                       TrkState            & state )
        {
            state.lastpause_ = static_cast<uint8_t>( TrkDelayCodeVals.at(ev.evcode) );
            state.ticks_    += state.lastpause_;
        }

        /***********************************************************************************
            HandleSetPreset
                Converts DSE preset change events into MIDI bank select and MIDI patch 
                select.
        ***********************************************************************************/
        void HandleSetPreset( const DSE::TrkEvent           & ev, 
                              uint16_t                        trkno, 
                              uint8_t                         trkchan, 
                              TrkState                      & state, 
                              jdksmidi::MIDITimedBigMessage & mess, 
                              jdksmidi::MIDITrack           & outtrack )
        {
            using namespace jdksmidi;

            //The program id as read from the event!
            uint8_t originalprgm = ev.params.front();

            //Check if we have to translate preset/bank ids
            if( m_hasconvtbl )
            {
                // -- Select the correct bank --
                auto itfound = m_convtable->FindConversionInfo( originalprgm ); 

                /*
                    Some presets in the SMD might actually not even exist! Several tracks in PMD2 have this issue
                    So to avoid crashing, verify before looking up a bank.
                */
                if( itfound != m_convtable->end() ) //We found conversion info for the DSE preset
                {
                    state.hasinvalidbank = false;
                    state.curbank_       = itfound->second.midibank;
                    state.curprgm_       = itfound->second.midipres;
                    state.origdseprgm_   = originalprgm;
                    state.curmaxpoly_    = itfound->second.maxpoly;
                    state.transpose      = itfound->second.transpose;

                    if(itfound->second.idealchan != UCHAR_MAX)
                    {
                        state.chanoverriden = true;
                        state.ovrchan_      = itfound->second.idealchan;
                    }
                }
                else 
                {
                    //We didn't find any conversion info
                    state.hasinvalidbank = true;
                    state.curbank_       = 0x7F;           //Set to bank 127 to mark the error
                    state.curprgm_       = originalprgm; //Set preset as-is
                    state.origdseprgm_   = originalprgm;
                    state.curmaxpoly_    = -1;
                    state.transpose      =  0;
                    state.chanoverriden  = false;
                    state.ovrchan_       = UCHAR_MAX;

                    clog << "Couldn't find a matching bank for preset #" 
                         << static_cast<short>(ev.params.front()) <<" ! Setting to bank " <<state.curbank_ <<" !\n";
                }

            }
            else
            {
                //No need to translate anything
                state.hasinvalidbank = false;
                state.curbank_       =  0;
                state.curprgm_       = originalprgm; //Set preset as-is
                state.origdseprgm_   = originalprgm;
                state.curmaxpoly_    = -1;
                state.transpose      =  0;
                state.chanoverriden  = false;
                state.ovrchan_       = UCHAR_MAX;
            }

            //Change only if the preset/bank isn't overriden!
            if( !state.presetoverriden )
            {
                //Add the Bank select message
                AddBankChangeMessage( outtrack, m_seq[trkno].GetMidiChannel(), state.ticks_, state.curbank_ );

                //Add the Program change message
                mess.SetProgramChange( static_cast<uint8_t>(trkchan), static_cast<uint8_t>(state.curprgm_) );
                outtrack.PutEvent( mess );
            }

            //Clear the notes on buffer
            state.noteson_.clear();

            //Then disable/enable any effect controller 
            //#TODO: 
        }

        /***********************************************************************************
            HandlePauses
                Handle all pause events.
        ***********************************************************************************/
        inline void HandlePauses( eTrkEventCodes        code,
                                  const DSE::TrkEvent & ev,
                                  TrkState            & state )
        {
            if( code == eTrkEventCodes::Pause24Bits )
            {
                state.lastpause_ = (static_cast<uint32_t>(ev.params[2]) << 16) | (static_cast<uint32_t>(ev.params[1]) << 8) | ev.params[0];
                state.ticks_     += state.lastpause_;
            }
            else if( code == eTrkEventCodes::Pause16Bits )
            {
                state.lastpause_ = (static_cast<uint16_t>(ev.params.back()) << 8) | ev.params.front();
                state.ticks_     += state.lastpause_;
            }
            else if( code == eTrkEventCodes::Pause8Bits )
            {
                state.lastpause_ = ev.params.front();
                state.ticks_     += state.lastpause_;
            }
            else if( code == eTrkEventCodes::AddToLastPause )
            {
                int8_t value = static_cast<int8_t>(ev.params.front()); //The value is signed

                if( ev.params.front() >= state.lastpause_ )
                {
                    state.lastpause_  = 0;

                    if( utils::LibWide().isLogOn() )
                        clog << "Warning: AddToLastPause event addition resulted in a negative value! Clamping to 0!\n";
                }
                else
                    state.lastpause_  = state.lastpause_ + value;

                state.ticks_     += state.lastpause_;
            }
            else if( code == eTrkEventCodes::RepeatLastPause )
            {
                state.ticks_ += state.lastpause_;
            }
            else if( code == eTrkEventCodes::PauseUntilRel )
            {
#ifdef _DEBUG
                clog << "<!>- Error: Event 0x95 not yet implemented!\n";
                assert(false);
#else
                throw runtime_error("DSESequenceToMidi::HandlePauses() : Event 0x95 not yet implemented!\n");
#endif
            }
        }

        /***********************************************************************************
            HandleEvent
                Main conditional structure for converting events from the DSE format into 
                MIDI messages.
        ***********************************************************************************/
        void HandleEvent( uint16_t              trkno,
                          uint8_t               trkchan,
                          TrkState            & state, 
                          const DSE::TrkEvent & ev, 
                          jdksmidi::MIDITrack & outtrack )
        {
            using namespace jdksmidi;
            MIDITimedBigMessage       mess;
            const DSE::eTrkEventCodes code = static_cast<DSE::eTrkEventCodes>(ev.evcode);
            
            //Log events if neccessary
            if( utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn() )
                clog <<setfill(' ') <<setw(8) <<right <<state.ticks_ <<"t : " << ev <<setfill(' ') <<setw(16) <<right;
            
            //Handle Pauses then play notes, then anything else!
            if( code >= eTrkEventCodes::RepeatLastPause && code <= eTrkEventCodes::PauseUntilRel )
                HandlePauses( code, ev, state );
            else if( code >= eTrkEventCodes::Delay_HN   && code <= eTrkEventCodes::Delay_64N )
                HandleFixedPauses( ev, state );
            else if( code >= eTrkEventCodes::NoteOnBeg  && code <= eTrkEventCodes::NoteOnEnd )
                HandlePlayNote( trkno, trkchan, state, ev, outtrack );
            else
            {
                //Now that we've handled the pauses
                switch( code )
                {
                    //
                    case eTrkEventCodes::SetTempo:
                    case eTrkEventCodes::SetTempo2:
                    {
                        mess.SetTempo( ConvertTempoToMicrosecPerQuarterNote(ev.params.front()) );
                        mess.SetTime(state.ticks_);
                        outtrack.PutEvent( mess );
                        break;
                    }
                    case eTrkEventCodes::SetOctave:
                    {
                        int8_t newoctave = ev.params.front();
                        if( newoctave > DSE_MaxOctave )
                            clog << "New octave value set is too high !" <<static_cast<unsigned short>(newoctave) <<"\n";
                        state.octave_ = newoctave; 
                        break;
                    }
                    case eTrkEventCodes::AddOctave:
                    {
                        int8_t newoctave = ev.params.front();
                        newoctave += state.octave_;
                        if (newoctave > DSE_MaxOctave)
                            clog << "New octave value set is too high !" << static_cast<unsigned short>(newoctave) << "\n";
                        state.octave_ = newoctave;
                        break;
                    }
                    case eTrkEventCodes::SetExpress:
                    {
                        mess.SetControlChange( trkchan, jdksmidi::C_EXPRESSION, ev.params.front() );
                        mess.SetTime(state.ticks_);
                        outtrack.PutEvent( mess );
                        break;
                    }
                    case eTrkEventCodes::SetTrkVol:
                    {
                        mess.SetControlChange( trkchan, jdksmidi::C_MAIN_VOLUME, ev.params.front() );
                        mess.SetTime(state.ticks_);
                        outtrack.PutEvent( mess );
                        break;
                    }
                    case eTrkEventCodes::SetTrkPan:
                    {
                        assert(false); //#TODO: Need to track pan for the whole channel + track
                        mess.SetControlChange( trkchan, jdksmidi::C_PAN, ev.params.front() );
                        mess.SetTime(state.ticks_);
                        outtrack.PutEvent( mess );
                        break;
                    }
                    case eTrkEventCodes::SetChanPan:
                    {
                        assert(false); //#TODO: Need to track pan for the whole channel + track
                        //Set the channel pan
                        mess.SetControlChange(trkchan, jdksmidi::C_PAN, ev.params.front());
                        mess.SetTime(state.ticks_);
                        outtrack.PutEvent( mess );
                        break;
                    }
                    case eTrkEventCodes::SetPitchBend: //################### FIXME LATER ######################
                    {
                        //NOTE: Pitch bend's range is implementation specific in MIDI. Though PMD2's pitch bend range may vary per program split
                        mess.SetPitchBend( trkchan, ( static_cast<int16_t>(ev.params.front() << 8) | static_cast<int16_t>(ev.params.back() ) ) );
                        mess.SetTime(state.ticks_);
                        outtrack.PutEvent( mess );
                        break;
                    }
                    case eTrkEventCodes::SetPitchBendRng:
                    {
                        MIDITimedBigMessage rpn1msg;
                        MIDITimedBigMessage rpn2msg;
                        //0x0 0x0 is pitch bend range
                        rpn1msg.SetControlChange( trkchan, jdksmidi::C_RPN_LSB, 0 );
                        rpn1msg.SetTime(state.ticks_);
                        outtrack.PutEvent( rpn1msg );
                        rpn2msg.SetControlChange( trkchan, jdksmidi::C_RPN_MSB, 0 );
                        rpn2msg.SetTime(state.ticks_);
                        outtrack.PutEvent( rpn2msg );

                        //Possibly pitch bend range?
                        mess.SetControlChange( trkchan, jdksmidi::C_DATA_ENTRY, ev.params.front() );
                        mess.SetTime(state.ticks_);
                        outtrack.PutEvent( mess );
                        break;
                    }
                    case eTrkEventCodes::LoopPointSet:
                    {
                        if( m_nbloops == 0 )
                        {
                            if( !m_bLoopBegSet )
                            {
                                //Only place an event if we don't loop the track via code, and haven't placed it already to avoid playbak issues
                                mess.SetMetaType(META_TRACK_LOOP);
                                mess.SetTime(state.ticks_);
                                outtrack.PutTextEvent( state.ticks_, META_MARKER_TEXT, TXT_LoopStart.c_str(), TXT_LoopStart.size() );
                                outtrack.PutEvent( mess );
                            }
                        }

                        m_bTrackLoopable = true; //If we got a loop pos, then the track is loopable
                        m_bLoopBegSet    = true;

                        //Mark the loop position
                        state.looppoint_          = (state.eventno_ + 1);  //Add one to avoid re-processing the loop marker
                        m_beflooptrkstates[trkno] = state;                 //Save the track state
                        break;
                    }

                    //------------------ Repeat segment events ------------------
                    case eTrkEventCodes::RepeatFrom:
                    {
                        //#TODO: Will require some special implementation
                        assert(false);
                        break;
                    }
                    case eTrkEventCodes::RepeatSegment:
                    {
                        //#TODO: Will require some special implementation
                        assert(false);
                        break;
                    }
                    case eTrkEventCodes::AfterRepeat:
                    {
                        //#TODO: Will require some special implementation
                        assert(false);
                        break;
                    }

                    //------------------ Byte Skipping events! ------------------ 
                    case eTrkEventCodes::SkipNextByte:
                    case eTrkEventCodes::SkipNext2Bytes1:
                    case eTrkEventCodes::SkipNext2Bytes2:
                    {
                        //Don't do anything with those, we keep them in there just for reseach purpose
                        break;
                    }
                    //------------------ Unsupported Events ------------------ 
                    default:
                    {
                        //Put a cue point to mark unsupported events and their parameters
                        if( ShouldMarkUnsupported() )
                        {

                            stringstream evmark;
                            evmark << TXT_DSE_Event <<"_Chan:0x" <<hex <<uppercase <<trkno  
                                   <<"_ID:0x" <<static_cast<unsigned short>(ev.evcode) <<nouppercase;

                            //Then Write any parameters
                            for( const auto & param : ev.params )
                                evmark<< ", 0x"  <<hex <<uppercase  << static_cast<unsigned short>(param) <<nouppercase;

                            const string mark = evmark.str();
                            outtrack.PutTextEvent( state.ticks_, META_MARKER_TEXT, mark.c_str(), mark.size() );

                            //Count unknown events
                            auto itfound = m_unhandledEvList.find( ev.evcode );

                            if( itfound != m_unhandledEvList.end() )
                                itfound->second += 1;
                            else
                                m_unhandledEvList.insert( std::make_pair( ev.evcode, 1 ) );

                            //if( ev.evcode != static_cast<uint8_t>(eTrkEventCodes::SetUnk1) && ev.evcode != static_cast<uint8_t>(eTrkEventCodes::SetUnk2) )
                            //{
                            //    cout << "\tEvent ID: 0x" <<hex <<uppercase <<static_cast<unsigned short>(ev.evcode) << ", is unsupported ! Ignoring..\n" <<nouppercase <<dec;
                            //}
                        }
                    }
                };
            }

            if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
                clog << "\n";

            //Event handling done, increment event counter
            state.eventno_ += 1;
        }

        /***********************************************************************************
            HandleBankAndPresetOverrides
                Handle placing bank+preset change messages around a few specific notes.
        ***********************************************************************************/
        void HandleBankAndPresetOverrides(  uint16_t              trkno, 
                                            uint8_t               trkchan, 
                                            TrkState            & state, 
                                            jdksmidi::MIDITrack & outtrack,
                                            midinote_t          & mnoteid,
                                            const SMDLPresetConversionInfo::NoteRemapData & remapdata )
        {
            using namespace jdksmidi;

            //Remap the note
            //auto remapdata = m_convtable->RemapNote( state.origdseprgm_, (mnoteid & 0x7F) ); //Use the original program value for this
            //    
            //*** Remap the note ***
            mnoteid = remapdata.destnote;


            //First, check if we're playing a note past when the channel's preset + bank was changed by a previously processed track 
            if( m_midimode == eMIDIMode::GM && 
               !m_chanreqpresresets[trkchan].empty() && 
               m_chanreqpresresets[trkchan].front() <= state.ticks_ )
            {
                bankid_t      bnktoset = 0;
                dsepresetid_t prgtoset = 0;

                //Figure out what to reset the state to
                if( state.presetoverriden )
                {
                    bnktoset = state.ovrbank_;
                    prgtoset = state.ovrprgm_;
                }
                else
                {
                    bnktoset = state.curbank_;
                    prgtoset = state.curprgm_;
                }

                //Make the messages
                AddBankChangeMessage( outtrack, trkchan, state.ticks_, bnktoset );

                MIDITimedBigMessage msgpreset;
                msgpreset.SetTime(state.ticks_);
                msgpreset.SetProgramChange( static_cast<uint8_t>(trkchan), static_cast<uint8_t>(prgtoset) );
                outtrack.PutEvent( msgpreset );

                //Remove from the list!
                m_chanreqpresresets[trkchan].pop_front();
            }


            //Check if we should do an override, or restore the bank + preset
            if( state.presetoverriden && 
                remapdata.destpreset != state.ovrprgm_ && 
                remapdata.destbank   != state.ovrbank_ ) //Restore if the preset+bank was overriden, but the current preset doesn't define a bank+preset override!
            {
                //*** Restore override ***
                state.presetoverriden = false;
                state.ovrprgm_        = InvalidPresetID;
                state.ovrbank_        = InvalidBankID;

                //Put some bank+preset change messages
                AddBankChangeMessage( outtrack, trkchan, state.ticks_, state.curbank_ );

                //Restore original program
                MIDITimedBigMessage msgpreset;
                msgpreset.SetTime(state.ticks_);
                msgpreset.SetProgramChange( static_cast<uint8_t>(trkchan), static_cast<uint8_t>(state.curprgm_) );
                outtrack.PutEvent( msgpreset );
            }
            else if( remapdata.destpreset != InvalidPresetID || 
                     remapdata.destbank   != InvalidBankID )  //If the preset is not being overriden, but the current preset defines valid overrides of the bank+preset!
            {
                //*** Override Preset and Bank ***
                if( remapdata.destbank != InvalidBankID )
                {
                    //Check if its necessary to put new events
                    if( !state.presetoverriden || state.ovrbank_ != remapdata.destbank )
                    {
                        state.ovrbank_ = remapdata.destbank;
                        AddBankChangeMessage( outtrack, trkchan, state.ticks_, state.ovrbank_ );
                    }
                    //Do nothing otherwise
                }
                else if( state.presetoverriden )
                {
                    //Restore bank only
                    AddBankChangeMessage( outtrack, trkchan, state.ticks_, state.curbank_ );
                }

                if( remapdata.destpreset != InvalidPresetID )
                {
                    //Check if its necessary to put new events
                    if( !state.presetoverriden || state.ovrprgm_ != remapdata.destpreset )
                    {
                        state.ovrprgm_ = remapdata.destpreset;
                        MIDITimedBigMessage msgpreset;

                        msgpreset.SetTime(state.ticks_);

                        msgpreset.SetProgramChange( static_cast<uint8_t>(trkchan), static_cast<uint8_t>(state.ovrprgm_) );
                        outtrack.PutEvent( msgpreset );
                    }
                    //Do nothing otherwise
                }
                else if( state.presetoverriden )
                {
                    //Restore preset only
                    MIDITimedBigMessage msgpreset;
                    msgpreset.SetTime(state.ticks_);

                    //Restore original program
                    msgpreset.SetProgramChange( static_cast<uint8_t>(trkchan), static_cast<uint8_t>(state.curprgm_) );
                    outtrack.PutEvent( msgpreset );
                }

                //Mark the track state as overriden!
                state.presetoverriden = true;
            }
        }

        /***********************************************************************************
            HandleChannelOverride
                Overrides the channel properly as needed, depending on the note remap data, 
                or whether the preset has channel override data.
        ***********************************************************************************/
        inline uint8_t HandleChannelOverride( uint8_t                                         trkchan,
                                              TrkState                                      & state, 
                                              const SMDLPresetConversionInfo::NoteRemapData & remapdata )
        {
            //Only in GM
            if( m_midimode == eMIDIMode::GM )
            {
                //Handle per note remap
                if( remapdata.idealchan != UCHAR_MAX )
                {
                    //Handle per note remap
                    if( trkchan != MIDIDrumChannel ) //We don't care about the drum channel, since it ignores program changes!
                        m_chanreqpresresets[remapdata.idealchan].push_back( state.ticks_ ); //Mark the channel for being re-inited
                    //state.chantoreinit = remapdata.idealchan; 
                    return remapdata.idealchan;
                }
                else if( state.chanoverriden )
                {
                    //Handle per preset remap
                    return state.ovrchan_;
                }
            }
            return trkchan;
        }

        /***********************************************************************************
            HandlePlayNote
                Handle converting a Playnote event into a MIDI key on and key off message !
        ************************************************************************************/
        void HandlePlayNote( uint16_t              trkno, 
                             uint8_t               trkchan, 
                             TrkState            & state, 
                             const DSE::TrkEvent & ev, 
                             jdksmidi::MIDITrack & outtrack )
        {
            using namespace jdksmidi;
            MIDITimedBigMessage mess;

            //Check polyphony
            //if( state.curmaxpoly_ != -1 && state.curmaxpoly_ != 0 && state.noteson_.size() > static_cast<uint8_t>(state.curmaxpoly_) )
            //{
            //    //Kill another note and take its place
            //    uint8_t  notetokill   = state.noteson_.back().noteid;
            //    uint32_t noteoffnum   = state.noteson_.back().noteoffnum;
            //    uint32_t noteoffticks = state.noteson_.back().noteoffticks;
            //    state.noteson_.pop_back();

            //    //Check for the note off
            //    int evnum = 0;
            //    if( outtrack.FindEventNumber( state.noteson_.back().noteoffticks, &evnum ) )
            //    {
            //        MIDITimedBigMessage * ptrmess = outtrack.GetEvent( evnum );
            //        if( ptrmess != nullptr )
            //        {
            //            ptrmess->SetTime(state.ticks_); //Make it happen sooner
            //        }
            //        else
            //            assert(false);
            //    }
            //    else
            //        assert(false); 
            //}

            //Turn off sustain if neccessary
            //if( state.sustainon )
            //{
            //    MIDITimedBigMessage susoff;
            //    susoff.SetTime(state.ticks_);
            //    susoff.SetControlChange( trkchan, 66, 0 ); //sustainato
            //    outtrack.PutEvent( susoff );
            //    state.sustainon = false;
            //}

            //Interpret the first parameter byte of the play note event
            midinote_t mnoteid   = 0;
            uint8_t    param2len = 0; //length in bytes of param2
            int8_t     octmod    = 0;
            uint8_t    parsedkey = 0;
            //ParsePlayNoteParam1( ev.params.front(), state.octave_, param2len, mnoteid );

            ParsePlayNoteParam1( ev.params.front(), octmod, param2len, parsedkey );

            //Special case for when the play note even is 0xF
            if( parsedkey > static_cast<uint8_t>(eNote::nbNotes) )
            {
                clog <<"<!>- Event on track#" <<trkno << ", has key ID 0x" <<hex <<static_cast<short>(parsedkey) <<dec <<"! Unsupported!\n";
                return;
            }
            else
            {
                state.octave_ = static_cast<int8_t>(state.octave_) + octmod;                          //Apply octave modification
                mnoteid       = ( state.octave_ * static_cast<uint8_t>(eNote::nbNotes) ) + parsedkey; //Calculate MIDI key!
            }


            //Parse the note hold duration bytes
            uint32_t holdtime = 0;
            for( int cntby = 0; cntby < param2len; ++cntby )
                holdtime = (holdtime << 8) | ev.params[cntby+1];

            if( param2len != 0 )
                state.lasthold_ = holdtime;

            mess.SetTime(state.ticks_);

            //Check if we should change the note to another.
            if( m_hasconvtbl )
            {
                //Remap the note
                auto remapdata = m_convtable->RemapNote( state.origdseprgm_, (mnoteid & 0x7F) ); //Use the original program value for this
                
                //Handle channel overrides for the preset, and for each notes
                trkchan = HandleChannelOverride( trkchan, state, remapdata );


                //Handle preset overrides for each notes
                HandleBankAndPresetOverrides( trkno, trkchan, state, outtrack, mnoteid, remapdata );

                //Apply transposition
                if(state.transpose != 0 )
                {
                    int transposed = mnoteid + (state.transpose * NbMidiKeysInOctave);

                    if( transposed >= 0 && transposed < 127 )
                        mnoteid = transposed;
                    else
                        clog <<"<!>- Invalid transposition value was ignored! The transposed note " <<transposed <<" was out of the MIDI range!\n";
                }
            }

            //If we got an invalid bank, we just silence every notes, while leaving theme there!
            if( state.hasinvalidbank && !ShouldLeaveNoteWithInvalidPreset() )
                mess.SetNoteOn( trkchan, mnoteid, 0 ); //leave the note, but play no sound!
            else
                mess.SetNoteOn( trkchan, mnoteid, static_cast<uint8_t>(ev.evcode & 0x7F) );

            outtrack.PutEvent( mess );

            if( utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn() )
                clog <<"=> " << MidiNoteIdToText(mnoteid) << ", ";
            
            //Make the noteoff message
            MIDITimedBigMessage noteoff;
            uint32_t            noteofftime = state.ticks_ + state.lasthold_;


            //Check if we should cut the duration the key is held down.
            if( m_hasconvtbl )
            {
                auto itfound = m_convtable->FindConversionInfo(state.origdseprgm_);

                if( itfound != m_convtable->end() && itfound->second.maxkeydowndur != 0 )
                    noteofftime = state.ticks_ +  utils::Clamp( state.lasthold_, 0ui32, itfound->second.maxkeydowndur );
            }

            if( utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn() )
                clog <<"hold " <<state.lasthold_ <<" tick(s)";


            noteoff.SetTime( noteofftime );
            noteoff.SetNoteOff( trkchan, (mnoteid & 0x7F), static_cast<uint8_t>(ev.evcode & 0x7F) ); //Set proper channel from original track eventually !!!!
            outtrack.PutEvent( noteoff );

            //if( state.curmaxpoly_ != -1 && state.curmaxpoly_ != 0 )
            //{
            //    //Add the note to the noteon list !
            //    NoteOnData mynoteon;
            //    mynoteon.noteid = (mnoteid & 0x7F);
            //    mynoteon.noteonnum = outtrack.GetNumEvents()-2;
            //    mynoteon.noteonticks = state.ticks_;
            //    mynoteon.noteoffnum = outtrack.GetNumEvents()-1;
            //    mynoteon.noteoffticks = state.ticks_ + state.lasthold_;
            //    state.noteson_.push_front( mynoteon );
            //}
        }


        /****************************************************************************
            PrepareMidiFile
                Place common messages into the MIDI file.
        ****************************************************************************/
        void PrepareMidiFile()
        {
            using namespace jdksmidi;
            //Setup Common Data
            //m_midiout = MIDIMultiTrack( m_trkstates.size() );
            m_midiout.SetClksPerBeat( m_seq.metadata().tpqn );

            //Put a XG or GS sysex message if specified
            if( m_midimode == eMIDIMode::GS )
                WriteGSSysex();
            else if( m_midimode == eMIDIMode::XG )
                WriteXGSysex();

            //Init track 0 with time signature
            MIDITimedBigMessage timesig;
            timesig.SetTime( 0 );
            timesig.SetTimeSig();
            m_midiout.GetTrack( 0 )->PutEvent( timesig );
            m_midiout.GetTrack( 0 )->PutTextEvent( 0, META_TRACK_NAME, m_seq.metadata().fname.c_str(), m_seq.metadata().fname.size() );
            m_midiout.GetTrack( 0 )->PutTextEvent( 0, META_GENERIC_TEXT, UtilityID.c_str(), UtilityID.size() );
        }

        /****************************************************************************
        ****************************************************************************/
        void WriteGSSysex()
        {
            using namespace jdksmidi;
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

        /****************************************************************************
        ****************************************************************************/
        void WriteXGSysex()
        {
            using namespace jdksmidi;
            //Ugh.. I have no clue if that's how I should do this.. 
            // JDKSmidi has 0 documentation and some of the most 
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
                
            //Not sure if this all that I should do.. Documentation is sparse for XG

            m_midiout.GetTrack(0)->PutEvent(xgreset);
        }

        /****************************************************************************
            ExportAsSingleTrack
                Method handling export specifically for single track MIDI format 0
        ****************************************************************************/
        void ExportAsSingleTrack()
        {
            using namespace jdksmidi;

            //Setup our track states
            m_trkstates       .resize(m_seq.getNbTracks());
            m_beflooptrkstates.resize(m_seq.getNbTracks());
            m_songlsttick = 0;

            //Setup the time signature and etc..
            PrepareMidiFile();

            //Re-Organize channels if in GM mode, and build priority queue
            if( m_midimode == eMIDIMode::GM )
                RearrangeChannels();
            else
            {
                m_trackpriorityq.resize(m_seq.getNbTracks());
                std::iota( m_trackpriorityq.begin(), m_trackpriorityq.end(), 0 ); //Fill up the queue sequentially otherwise 0..15
            }

            //Play all tracks at least once
            for( unsigned int trkno = 0; trkno < m_trackpriorityq.size(); ++trkno )
            {
                unsigned int curtrk = m_trackpriorityq[trkno];
                ExportATrack( curtrk, 0 );

                //Keep track of the very last tick of the song as a whole.
                if( m_songlsttick < m_trkstates[curtrk].ticks_ )
                    m_songlsttick = m_trkstates[curtrk].ticks_;
            }

            //Just mark the end of the loop when we're not looping ourselves via code. This prevents there being a delay when looping in external players!
            if( m_bTrackLoopable && m_nbloops == 0 )
                m_midiout.GetTrack(0)->PutTextEvent( m_songlsttick, META_MARKER_TEXT, TXT_LoopEnd.c_str(), TXT_LoopEnd.size() );

            WriteUnhandledEventsReport();

            //Loop the track again if loopable and requested !
            if( m_bTrackLoopable )
            {
                //Then, if we're set to loop, then loop
                for( unsigned int nbloops = 0; nbloops < m_nbloops; ++nbloops )
                {
                    for( unsigned int trkno = 0; trkno < m_trackpriorityq.size(); ++trkno )
                    {
                        unsigned int curtrk = m_trackpriorityq[trkno];

                        //Restore track state
                        uint32_t backticks         = m_trkstates[curtrk].ticks_; //Save ticks
                        m_trkstates[curtrk]        = m_beflooptrkstates[curtrk]; //Overwrite state
                        m_trkstates[curtrk].ticks_ = backticks;                 //Restore ticks

                        ExportATrack( curtrk, 0, m_trkstates[curtrk].looppoint_ );
                    }
                }
            }

            
        }

        /*
            RearrangeChannels
                Try to free channel 10 if possible, and if not, set a track that has a 0x7F program change on chan 10.

                Only for GM conversion. Should probably get phased out eventually, because its pretty bad at its job.
                Not that swapping stuff from channel to channel is easy..
        */
        void RearrangeChannels()
        {
            //Init 
            m_chanreqpresresets.resize( NbMidiChannels );
            m_trkchanmap.resize(m_seq.getNbTracks(), 0); //Init the channel map!

            //Start by filling up the priority queue
            //#NOTE: This is really atrocious, 
            vector<size_t> presetswithchanremaps;       //The presets that have key remaps that play on another channel!

            //Make a list of the presets that remap notes to different channels
            for( const auto & remapentry : (*m_convtable) )
            {
                for( const auto & notermap : remapentry.second.remapnotes )
                {
                    //If we have a channel remap for a specific key, and we haven't added this preset to the list yet, add it!
                    if( notermap.second.idealchan != UCHAR_MAX &&
                        std::find( presetswithchanremaps.begin(), presetswithchanremaps.end(), remapentry.first ) != presetswithchanremaps.end() )
                    {
                        presetswithchanremaps.push_back( remapentry.first );
                    }
                }
            }

            //Find which tracks use those presets, and put them first in the priority queue!
            for( size_t cnttrk = 1; cnttrk < m_seq.getNbTracks(); ++cnttrk ) //Skip track 1 again
            {
                bool prioritize = false;
                for( size_t cntev = 0; cntev < m_seq[cnttrk].size(); ++cntev )
                {
                    if( m_seq[cnttrk][cntev].evcode == static_cast<uint8_t>(eTrkEventCodes::SetPreset) )
                    {
                        //When we get a SetPreset event, check if it matches one of the presets we're looking for.
                        for( const auto & apreset : presetswithchanremaps )
                        {
                            if( m_seq[cnttrk][cntev].params.front() == apreset )
                                prioritize = true;
                        }
                    }
                }

                //If we need to prioritize, push it in the front
                if( prioritize )
                    m_trackpriorityq.push_front( cnttrk );
                else
                    m_trackpriorityq.push_back( cnttrk );
            }

            //Push track 0
            m_trackpriorityq.push_back(0);

            //*** Next re-arrange the tracks ! ***
            
            array<bool,NbMidiChannels> usedchan;         //Keep tracks of channels in use

            //Check if something is on channel 10, and populate our channel map
            deque<size_t> chan10trks; //A list of tracks using chan 10

            //Always skip the first DSE track, as it can't have instruments on it
            for( size_t cnttrk = 1; cnttrk < m_seq.getNbTracks(); ++cnttrk )
            {
                if( m_seq[cnttrk].GetMidiChannel() < NbMidiChannels )
                    usedchan[m_seq[cnttrk].GetMidiChannel()] = true; 
                else
                    clog << "<!>- Warning: Encountered a track with an invalid MIDI channel in the current sequence!\n";

                if( m_seq[cnttrk].GetMidiChannel() == 9 )
                    chan10trks.push_back(cnttrk);

                m_trkchanmap[cnttrk] = m_seq[cnttrk].GetMidiChannel();
            }

            //If channel 10 not in use, nothing else to do here!
            if( chan10trks.empty() )
                return;

            // -------------------------------------
            // -- Mitigate the drum channel issue --
            // -------------------------------------

            // --- Step 1! ---
            size_t nbchaninuse = std::count_if( usedchan.begin(), usedchan.end(), [](bool entry){ return entry; } );

            //First, try to reassign to an empty channel!
            if( nbchaninuse != NbMidiChannels )
            {
                const size_t  NbToReloc   = chan10trks.size();
                deque<size_t> modch10trks;
                for( size_t cnttrks = 0; cnttrks < NbToReloc; ++cnttrks )
                {
                    size_t trktorelocate = chan10trks[cnttrks];

                    for( size_t cntchan = 0; cntchan < usedchan.size(); ++cntchan )
                    {
                        if( !usedchan[cntchan] && cntchan != 9 )
                        {
                            //Re-assign the unused channel
                            usedchan[m_trkchanmap[trktorelocate]] = false;   //Vaccate the old channel
                            m_trkchanmap[trktorelocate]           = static_cast<uint8_t>(cntchan);
                            usedchan[cntchan]                     = true;
                             //Remove a track from the list to relocate!
                        }
                        else
                            modch10trks.push_back(trktorelocate);
                    }
                }
                chan10trks = move(modch10trks);
                //If we re-assigned all tracks, nothing more to do here !
                if( chan10trks.empty() )
                    return;
                //If we still haven't reassigned all tracks, we go to step 2
            }
            

            // --- Step 2! ---
            //Then, we'll try to swap our place with a track that makes use of preset 0x7F, which is used for drums usually.
            deque<std::pair<size_t,size_t>> drumusingchans; //First is track index, second is channel it uses

            for( size_t cnttrk = 1; cnttrk < m_seq.getNbTracks(); ++cnttrk ) //Skip track 1 again
            {
                for( size_t cntev = 0; cntev < m_seq[cnttrk].size(); ++cntev )
                {
                    if( m_seq[cnttrk][cntev].evcode == static_cast<uint8_t>(eTrkEventCodes::SetPreset) && 
                        m_seq[cnttrk][cntev].params.front() == 0x7F )
                    {
                        drumusingchans.push_back( make_pair( cnttrk, m_seq[cnttrk].GetMidiChannel() ) );

                        //If one of the tracks we have is using channel 10 AND is using preset 0x7F, remove it from the list
                        // as it doesn't need to be relocated!
                        auto itfound = std::find( chan10trks.begin(), chan10trks.end(), cnttrk );
                        if( itfound != chan10trks.end() )
                            chan10trks.erase(itfound);
                    }
                }
            }

            //Now, do the actual swapping!
            if( !drumusingchans.empty() )
            {
                for( ; !chan10trks.empty() && !drumusingchans.empty(); )
                {
                    auto drumuserchan = drumusingchans.back();

                    uint8_t prevchan                 = m_trkchanmap[chan10trks.back()];
                    m_trkchanmap[chan10trks.back()]  = m_trkchanmap[drumuserchan.first];
                    m_trkchanmap[drumuserchan.first] = prevchan;

                    drumusingchans.pop_back();
                    chan10trks.pop_back();
                }

                //If we handled all the tracks on channel 10, we're done!
                if( chan10trks.empty() )
                    return;
            }


            // --- If we get here, we're screwed! ---
            clog << "<!>- Warning: Unable to reassign the channels for track(s) ";

            for( const auto & entry : chan10trks )
                clog << entry << " ";

            clog << " !\n";
        }


        /*
            ExportATrack
                Exports a single track, intrk, to the midi output, in the track slot specified by outtrk.
                
                - intrk  : The DSE track we're processing.
                - outtrk : The MIDI track in the output to place the events processed. 
                - evno   : The event to begin parsing the track at.
        */
        void ExportATrack( unsigned int intrk, unsigned int outtrk, size_t evno = 0 )
        {
            if( utils::LibWide().isLogOn()  )
            {
                clog <<"---- Exporting Track#" <<intrk <<" ----\n";
            }

            for( ; evno < m_seq.track(intrk).size(); ++evno )
            {
                if( m_seq[intrk][evno].evcode == static_cast<uint8_t>( DSE::eTrkEventCodes::EndOfTrack ) && 
                    !ShouldIgnorePrematureEoT() )
                    break; //Break on 0x98 as requested 

                //Obtain the channel the content of this track is played on
                uint8_t prefchan = m_seq[intrk].GetMidiChannel();

                if( m_midimode == eMIDIMode::GM )   //In GM mode, use our remapped channel table
                    prefchan = m_trkchanmap[intrk];

                HandleEvent( intrk, 
                             prefchan, 
                             m_trkstates[intrk], 
                             m_seq[intrk][evno], 
                             *(m_midiout.GetTrack(outtrk)) );
            }

            if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
                clog <<"---- End of Track ----\n\n";
        }

        /*
            ShouldMarkUnsupported
                Return whether unsupported events should be marked in the MIDI!
        */
        bool ShouldMarkUnsupported()const
        {
            return true; //#TODO: Make this configurable
        }

        /*
            ShouldIgnorePrematureEoT
                Returns whether we should keep converting events past any premature end of the track marker, if there are any.
        */
        bool ShouldIgnorePrematureEoT()const
        {
            return false; //#TODO: Make this configurable
        }

        /*
            ShouldLeaveNoteWithInvalidPreset
                Some tracks sometimes refers to a program in the SWD that is not loaded. It results in no sound being
                produced when the notes associated with that program number are played. However that's not the case with
                a MIDI.

                Setting this property to false will leave the silent note events in the resulting MIDI, but will set their
                velocity to 0, essentially making them silent.

                Setting it to true will leave the note events as-is.
        */
        bool ShouldLeaveNoteWithInvalidPreset()const
        {
            return false;
        }


        void WriteUnhandledEventsReport()
        {
            if( m_unhandledEvList.empty() )
                return;

            clog <<"<!>- Ingored the following unsupported events: \n";
            
            for( const auto & ev : m_unhandledEvList )
            {
                clog << "\tEventID: 0x" <<hex <<uppercase <<static_cast<unsigned short>(ev.first) <<dec <<nouppercase 
                     <<", ignored " <<ev.second << " times.\n";
            }

        }

    private:
        const std::string                & m_fnameout;
        const MusicSequence              & m_seq;
        const SMDLPresetConversionInfo   * m_convtable;
        bool                               m_hasconvtbl;    //Whether we should use the conv table at all
        uint32_t                           m_nbloops;
        /*eMIDIFormat                        m_midifmt;*/
        eMIDIMode                          m_midimode;

        //State variables
        //#TODO: This is horrible! I need to build something better!
        std::vector<TrkState>              m_trkstates;
        std::vector<TrkState>              m_beflooptrkstates; //Saved states of each tracks just before the loop point event! So we can restore each tracks to their intended initial states.
        std::vector<uint8_t>               m_trkchanmap;       //Contains the channel to use for each tracks. This overrides what's in the sequence!
        std::deque<size_t>                 m_trackpriorityq;   //tracks are ordered in this vector to be parsed in that order. Mainly to ensure tracks that have key remaps in which the channel is forced to something are handled first!
        std::vector<deque<uint32_t>>       m_chanreqpresresets; //The times for each tracks when we need to re-establish the current program + bank, after having other tracks play events on our MIDI channel!.  
        
        std::map<uint8_t, int>             m_unhandledEvList;   //A table to store all the unsupported events encountered and the number of times it was encountered!

        uint32_t                           m_songlsttick;      //The very last tick of the song

        bool                               m_bTrackLoopable;
        //Those two only apply to single track mode !
        bool                               m_bLoopBegSet;

        jdksmidi::MIDIMultiTrack           m_midiout;
    };

//======================================================================================
//  MIDIToDSE
//======================================================================================
    /*
        MIDIToDSE
            Convert a MIDI file into a DSE Sequence.

            **WIP**
    */
    class MIDIToDSE
    {
        typedef unsigned long long ticks_t;
        
        struct NoteOnEvInfo
        {
            size_t                        dseEvIndex = 0;       //The event slot that was reserved for this Note On/Off pair in the destination sequence!
            jdksmidi::MIDITimedBigMessage noteon;               // The note on event
        };

        struct TrkState
        {
            ticks_t             ticks       = 0; //The tick count at which the last event was placed in this track
            ticks_t             lastpause   = 0; //Duration of the last pause on this track
            stack<NoteOnEvInfo> notes;     //Contains "notes on" messages for which a "note off" was not yet encountered
            int8_t              lastoctave  = -1; //The octave the last note was played in. (Used for calculating the note event parameter!)
            uint32_t            lastnotedur = 0; //The last duration of a note
        };

    public:

        /*
            - bNoDeltaTimeRounding : If this is true, the program won't try to round delta-time between midi events to the 
                                     closest matches in the DSE system.
        */
        MIDIToDSE( const string & srcmidi, bool bNoDeltaTimeRounding = true )
            :m_srcpath(srcmidi), m_bIsAccurateTime(bNoDeltaTimeRounding)
        {}

        /****************************************************************************************
        ****************************************************************************************/
        MusicSequence operator()()
        {
            using namespace jdksmidi;

            if( utils::LibWide().isLogOn() )
            {
                clog << "------------------------\n"
                     << "Converting MIDI to DSE\n"
                     << "------------------------\n"
                    ;
            }

            //#1 - Load the MIDI
            MIDIFileReadStreamFile rs           ( m_srcpath.c_str() );
            MIDIMultiTrack         tracks;
            MIDIFileReadMultiTrack track_loader ( &tracks );
            MIDIFileRead           reader       ( &rs, &track_loader );

            if( !reader.Parse() ) //Apparently handling errors with exceptions is too much to ask to jdksmidi :P
                throw runtime_error( "JDKSMIDI: File parsing failed. Reason not specified.." );

            if( utils::LibWide().isLogOn() )
            {
                clog << "MIDI loaded! :\n"
                     << "\t"   << m_srcpath <<"\n"
                     << "\t\t" << "NbTracks : " <<tracks.GetNumTracksWithEvents() <<"\n"
                     << "\t\t" << "NbEvents : " <<tracks.GetNumEvents()           <<"\n"
                     << "\t\t" << "TPQN     : " <<tracks.GetClksPerBeat()         <<"\n"
                     << "\n";
                ;
            }

            //#2 - Convert the MIDI to a sequence
            DSE::DSE_MetaDataSMDL dseMeta;
            dseMeta.fname       = utils::GetBaseNameOnly(m_srcpath);
            dseMeta.tpqn        = tracks.GetClksPerBeat();
            dseMeta.createtime.SetTimeToNow();

            return std::move( ConvertMIDI(tracks,dseMeta) );
        }

    private:
        /****************************************************************************************
        ****************************************************************************************/
        MusicSequence ConvertMIDI( const jdksmidi::MIDIMultiTrack & midi, DSE::DSE_MetaDataSMDL & dseMeta )
        {
            using namespace jdksmidi;
            vector<MusicTrack> tracks;

            //Determine if multi-tracks
            const bool ismultitrk = midi.GetNumTracksWithEvents() > 1;

            if( ismultitrk )
            {
                //If multi tracks, use the track numbers as track numbers
                tracks.resize( midi.GetNumTracksWithEvents() ); //Pre-emptively resize to 16

                ConvertFromMultiTracksMidi( tracks, midi, dseMeta );
            }
            else
            {
                //If single track, use the MIDI channel for each events to place them onto a specific track
                tracks.resize( NB_DSETracks ); //Pre-emptively resize to max

                ConvertFromSingleTrackMidi( tracks, midi, dseMeta );
            }

            //!#TODO: Flush empty tracks?

            return std::move( MusicSequence( std::move(tracks), std::move(dseMeta) ) );
        }

        /****************************************************************************************
            ConvertFromMultiTracksMidi
        ****************************************************************************************/
        void ConvertFromMultiTracksMidi( vector<MusicTrack>              & tracks, 
                                         const jdksmidi::MIDIMultiTrack  & midi, 
                                         DSE::DSE_MetaDataSMDL           & dseMeta )
        {
            using namespace jdksmidi;
            cerr<<"Not implemented!\n";
            assert(false);
        }

        /****************************************************************************************
            ConvertFromSingleTrackMidi
        ****************************************************************************************/
        void ConvertFromSingleTrackMidi( vector<MusicTrack>              & tracks, 
                                         const jdksmidi::MIDIMultiTrack  & midi, 
                                         DSE::DSE_MetaDataSMDL           & dseMeta )
        {
            using namespace jdksmidi;
            if( midi.GetTrack(0) == nullptr )
                throw std::runtime_error( "ConvertFromSingleTrackMidi(): JDKSMIDI: jdksmidi returned a null track ! wtf.." );

            //Maintain a global tick count
            ticks_t              ticks  = 0;
            const MIDITrack    & mtrack = *( midi.GetTrack(0) );
            const int            nbev   = mtrack.GetNumEvents();
            vector<TrkState>     trkstates( NB_DSETracks ); //Pre-emptively alloc all midi channels

            //Init each tracks with its midi channel!
            {
                uint32_t cntmidchans = 0;
                for( auto & trk : tracks )
                {
                    trk.SetMidiChannel(cntmidchans);
                    ++cntmidchans;
                }
            }


            //cerr<<"Not implemented!\n";
            //assert(false);

            //Iterate through events
            for( int cntev = 0; cntev < nbev; ++cntev )
            {
                const MIDITimedBigMessage * ptrev = mtrack.GetEvent( cntev );
                

                if( ptrev != nullptr )
                {
                    if( ptrev->IsEndOfTrack() )
                        HandleEoT( *ptrev, trkstates, tracks, ticks );
                    else if( ptrev->IsMetaEvent() )
                        HandleUnsupportedEvents( *ptrev, trkstates, tracks, dseMeta, ticks );
                    else if( ptrev->IsTempo() )
                        HandleSetTempo( *ptrev, trkstates, tracks, ticks );
                    else 
                    {
                        //Get the correct channel to use as track state
                        //We skip track #0 because its reserved for setting the tempo!
                        HandleSingleTrackEvent( *ptrev, tracks[ptrev->GetChannel()+1], trkstates[ptrev->GetChannel()+1], ticks ); 
                    }
                }
            }

            //Then read all text events mentioning an MIDI unsupported DSE event
        }

        /****************************************************************************************
            The handling of single track events differs slightly
            Global ticks is the nb of ticks since the beginning of the single track. 
            Its used to properly pad events with silences if required, and properly 
            calculate the delta time of each events on each separate tracks.
        ****************************************************************************************/
        void HandleSingleTrackEvent( const jdksmidi::MIDITimedBigMessage & mev,
                                     MusicTrack                          & trk,
                                     TrkState                            & state,
                                     ticks_t                             & globalticks ) 
        {
            ticks_t trktickdiff   = (globalticks - state.ticks); //The difference in ticks between the track's last tick and the current global tick
            ticks_t evtglobaltick = mev.GetTime(); //The global absolute tick of the event

            //Insert a pause if needed
            ticks_t delta = (evtglobaltick - state.ticks);
            if( delta != 0 )
                HandlePauses(mev, state, trk, globalticks, delta);

            //After the delay was handled, deal with the event
            HandleEvent( mev, state, trk );
        }


        /****************************************************************************************
            Handles the pause event insertion logic, based on the delta time an event happens at!
        ****************************************************************************************/
        void HandlePauses( const jdksmidi::MIDITimedBigMessage & mev,
                           TrkState                            & state,
                           MusicTrack                          & trk, 
                           ticks_t                             & globalticks,
                           ticks_t                               delta )
        {
            //Check if we can handle the pause with a simple fixed duration pause event first!
            if( delta <= static_cast<ticks_t>(DSE::eTrkDelays::_half) )
            {
                DSE::TrkEvent pauseev;
                auto          itfound  = TicksToTrkDelayID.find( static_cast<uint8_t>(delta) );
                ticks_t       newdelta = delta;

                //If we have an exact match, go for that!
                if( itfound != TicksToTrkDelayID.end() )
                {
                    pauseev.evcode = DSE::TrkDelayToEvID.at(itfound->second);
                    newdelta       = itfound->first;
                }
                else if( isAccurateTime() ) //Otherwise, depending on our time conversion strategy, pick the closest time
                {
                    HandleLongPauseEvents( mev, state, trk, globalticks, delta );
                    return; //Just return if we handled it that way!
                }
                else
                {
                    auto closesttrkdelay = DSE::FindClosestTrkDelayID( static_cast<uint8_t>(delta) );
                    pauseev.evcode = DSE::TrkDelayToEvID.at( closesttrkdelay.first ); 
                    newdelta       = static_cast<uint8_t>(closesttrkdelay.first);
                }

                //Find out what delta we went for in the end!
                //Add the pause to the track state and global tick count
                globalticks     += newdelta;
                state.ticks     += newdelta;
                state.lastpause  = newdelta;
                trk.push_back( pauseev );
            }
            else
                HandleLongPauseEvents( mev, state, trk, globalticks, delta ); //If delta time is too long, use a longer pause event!
        }

        /****************************************************************************************
        ****************************************************************************************/
        void HandleLongPauseEvents( const jdksmidi::MIDITimedBigMessage & mev,
                                    TrkState                            & state,
                                    MusicTrack                          & trk, 
                                    ticks_t                             & globalticks,
                                    ticks_t                               delta )
        {

            if( delta == state.lastpause ) //Check if our last pause is the exact same duration, and use that if possible
            {
                DSE::TrkEvent pauseev;
                pauseev.evcode = static_cast<uint8_t>(DSE::eTrkEventCodes::RepeatLastPause);

                //Add the pause to the track state and global tick count
                globalticks += state.lastpause;
                state.ticks += state.lastpause;

                trk.push_back( pauseev );
            }
            else if( delta < (state.lastpause + std::numeric_limits<uint8_t>::max()) && delta > state.lastpause ) //Check if our last pause is shorter than the required delay, so we can just add to it
            {
                DSE::TrkEvent addpauseev;
                addpauseev.evcode = static_cast<uint8_t>(DSE::eTrkEventCodes::AddToLastPause);
                addpauseev.params.push_back( static_cast<uint8_t>( (delta - state.lastpause) & 0xFF ) );

                //Add the pause to the track state and global tick count
                globalticks    += state.lastpause + addpauseev.params.front();
                state.ticks    += state.lastpause + addpauseev.params.front();
                state.lastpause = state.lastpause + addpauseev.params.front();
                trk.push_back( addpauseev );
            }
            else if( delta < std::numeric_limits<uint8_t>::max() )
            {
                //Otherwise make a short pause event if the delay fits within a short pause
                DSE::TrkEvent shortpauseev;
                shortpauseev.evcode = static_cast<uint8_t>(DSE::eTrkEventCodes::Pause8Bits);
                shortpauseev.params.push_back( static_cast<uint8_t>( delta & 0xFF ) );

                //Add the pause to the track state and global tick count
                globalticks    += delta;
                state.ticks    += delta;
                state.lastpause = delta;
                trk.push_back( shortpauseev );
            }
            else if( delta < std::numeric_limits<uint16_t>::max() )
            {
                //Otherwise make a long pause event
                DSE::TrkEvent longpauseev;
                longpauseev.evcode = static_cast<uint8_t>(DSE::eTrkEventCodes::Pause16Bits);
                longpauseev.params.push_back( static_cast<uint8_t>( delta        & 0xFF ) );
                longpauseev.params.push_back( static_cast<uint8_t>( (delta >> 8) & 0xFF ) );

                //Add the pause to the track state and global tick count
                globalticks    += delta;
                state.ticks    += delta;
                state.lastpause = delta;
                trk.push_back( longpauseev );
            }
            else 
            {
                //Make several pauses in a row!
                unsigned long long nbpauses = delta / std::numeric_limits<uint16_t>::max();
                if( ( delta % std::numeric_limits<uint16_t>::max() ) != 0 )
                    ++nbpauses;

                unsigned long long pauseleft = delta; //The nb of ticks to pause for remaining

                for( unsigned long long i = 0; i < nbpauses; ++nbpauses )
                {
                    if( pauseleft < numeric_limits<uint8_t>::max() )
                    {
                        //Use short pause
                        DSE::TrkEvent shortpauseev;
                        shortpauseev.evcode = static_cast<uint8_t>(DSE::eTrkEventCodes::Pause8Bits);
                        shortpauseev.params.push_back( static_cast<uint8_t>( pauseleft & 0xFF ) );

                        //Add the pause to the track state and global tick count
                        globalticks    += pauseleft;
                        state.ticks    += pauseleft;
                        state.lastpause = pauseleft;
                        trk.push_back( shortpauseev );

                        pauseleft = 0;
                    }
                    else
                    {
                        //Pick the longest pause we can
                        uint16_t curpause = pauseleft & numeric_limits<uint16_t>::max();

                        DSE::TrkEvent longpauseev;
                        longpauseev.evcode = static_cast<uint8_t>(DSE::eTrkEventCodes::Pause16Bits);
                        longpauseev.params.push_back( static_cast<uint8_t>(  curpause       & 0xFF ) );
                        longpauseev.params.push_back( static_cast<uint8_t>( (curpause >> 8) & 0xFF ) );

                        //Add the pause to the track state and global tick count
                        globalticks    += curpause;
                        state.ticks    += curpause;
                        state.lastpause = curpause;
                        trk.push_back( longpauseev );

                        //Use long pause
                        pauseleft -= curpause;
                    }
                }
            }
        }

        /****************************************************************************************
        ****************************************************************************************/
        void HandleEvent( const jdksmidi::MIDITimedBigMessage & mev,
                          TrkState                            & state,
                          MusicTrack                          & trk )
        {
            using namespace jdksmidi;
            if( mev.IsKeySig() )
            {
            }
            else if( mev.IsControlChange() )
            {
                HandleControlChanges( mev, state, trk );
            }
            else if( mev.IsMetaEvent() )
            {
                HandleMetaEvents( mev, state, trk );
            }
            else if( mev.IsNoteOn() )
            {
                HandleNoteOn(mev, state, trk);
            }
            else if( mev.IsNoteOff() )
            {
                HandleNoteOff(mev, state, trk);
            }
            else if( mev.IsPanChange() )
            {
                HandlePanChanges( mev, state, trk );
            }
            else if( mev.IsPitchBend() )
            {
                HandlePitchBend( mev, state, trk );
            }
            else if( mev.IsProgramChange() )
            {
                HandleProgramChange( mev, state, trk );
            }
            else if( mev.IsEndOfTrack() )
            {
            }
            else
            {
                std::string txtbuff;
                txtbuff.resize(64); //The MsgToText function requires a buffer of 64 characters. Because its really dumb..
                clog << "<!>- Ignored midi event: \n" <<mev.MsgToText(&txtbuff.front()) <<"\n";
            }
        }

        /****************************************************************************************
        ****************************************************************************************/
        void HandleNoteOn( const jdksmidi::MIDITimedBigMessage & mev,
                           TrkState                            & state,
                           MusicTrack                          & trk )
        {
            NoteOnEvInfo noteoninf;
            uint8_t      vel          = mev.GetVelocity();
            uint8_t      note         = mev.GetNote();
            uint8_t      targetoctave = (note / NbMidiKeysInOctave);
            int8_t       octavediff   = state.lastoctave - targetoctave;
            noteoninf.dseEvIndex      = trk.size();
            noteoninf.noteon          = mev;
            state.notes.push(std::move(noteoninf));

            //See if we need to insert a setoctave event before this note!
            if( state.lastoctave  == -1 || 
                state.lastoctave  != -1 && (abs(octavediff) > 2 || octavediff < -1) ) //If we haven't specified the initial octave yet
                InsertSetOctaveEvent( trk, state, targetoctave );

            //We need to insert a dse event, to reserve the play note event's spot!
            //We also assemble the play note event partially (Because we need to hit the note off event for this note to know its duration)
            DSE::TrkEvent noteonev;
            uint8_t octmod   = (targetoctave + 2) - state.lastoctave;
            uint8_t key      = (note % NbMidiKeysInOctave);
            noteonev.evcode  = vel & 0x7F; 
            noteonev.params.push_back( ((octmod&3)<<4) | (key&0xF) ); //Put the param1 without the parameter lenght for now!
            trk.push_back(noteonev);
        }

        /****************************************************************************************
        ****************************************************************************************/
        void HandleNoteOff( const jdksmidi::MIDITimedBigMessage & mev,
                            TrkState                            & state,
                            MusicTrack                          & trk )
        {
            //Ignore orphanned notes off events
            if( !state.notes.empty() )
            {
                //Update DSE event that was reserved earlier!
                const NoteOnEvInfo & noteonev     = state.notes.top();
                ticks_t              noteduration = abs( static_cast<long>(mev.GetTime() - noteonev.noteon.GetTime()) );

                assert( noteonev.dseEvIndex < trk.size() );
                assert( !( trk[noteonev.dseEvIndex].params.empty() ) );

                uint8_t paramlenby = 0;

                if( state.lastnotedur != noteduration ) //If the note duration has changed since the last note, append it!
                {
                    if( (noteduration & 0x00FF0000) > 0 )
                        paramlenby = 3;
                    else if( (noteduration & 0x0000FF00) > 0 )
                        paramlenby = 2;
                    else if( (noteduration & 0x0000FF00) > 0 )
                        paramlenby = 1;

                    state.lastnotedur = static_cast<uint32_t>(noteduration); //Update last note duration
                }
                
                trk[noteonev.dseEvIndex].params.front() |= (paramlenby&3) << 6; //Add the nb of param bytes

                //Push the duration in ticks
                for( uint8_t cnt = 0; cnt < paramlenby; ++cnt )
                    trk[noteonev.dseEvIndex].params.push_back( paramlenby >> ( (paramlenby - cnt)  * 8) );

                state.notes.pop();
            }
            else
                clog <<mev.GetTime() << " - MIDI NoteOff event no preceeded by a NoteOn!";
            
        }


        /****************************************************************************************
            HandleUnsupportedEvents
                Handles parsing MIDI text events for obtaining the DSE event stored in them.
        *****************************************************************************************/
        void HandleUnsupportedEvents( const jdksmidi::MIDITimedBigMessage & mev,
                                      vector<TrkState>                    & states,
                                      vector<MusicTrack>                  & trks,
                                      DSE_MetaDataSMDL                    & dsemeta,
                                      ticks_t                             & globalticks )
        {
            using namespace jdksmidi;

            //If its a marker text meta-event, with a sysex, we try parsing what's inside for possible text DSE events
            if( mev.GetMetaType() == META_MARKER_TEXT && mev.GetSysEx() != nullptr &&  mev.GetSysEx()->GetLength() > 0 )
            {
                //First, copy the text data from the event's sysex
                const MIDISystemExclusive & txtdat = *(mev.GetSysEx());
                string                      evtxt;
                auto                        itins = back_inserter(evtxt);
                copy_n( txtdat.GetBuf(), txtdat.GetLength(), itins );

                //Try to see if its a DSE event in text form. If it is, parse it!
                auto found = evtxt.find( TXT_DSE_Event );
                if( found != string::npos )
                {
                    //Find all values in the text string
                    vector<uint8_t> values;
                    auto            itstrend = end(evtxt);
                    for( auto itstr = begin(evtxt); itstr != itstrend; ++itstr )
                    {
                        auto itnext = (itstr+1);
                        if( (*itstr) == '0' && itnext != itstrend && (*itnext) == 'x'  )
                        {
                            uint32_t     value = 0;
                            stringstream sstrparse( string(itstr,itstrend) );
                            sstrparse >> hex >> value;
                            values.push_back(value);
                        }
                    }

                    //If we found at least 2 values, interpret them
                    if( values.size() >= 2 )
                    {
                        static const unsigned int ParamBegPos = 2; //The index at which params begin
                        DSE::TrkEvent dsev;
                        uint8_t       evchan = values.at(0); //Event channel
                        dsev.evcode          = values.at(1); //Event id

                        //Validate channel id
                        if( evchan >= NB_DSETracks )
                        {
                            clog << "<!>- Ignored text DSE event because channel/track specified was invalid!! (" <<static_cast<uint16_t>(evchan) <<") :\n"
                                 << "\t" <<evtxt <<"\n";
                            return;
                        }

                        //Copy parameters
                        for( size_t i = ParamBegPos; i < values.size(); ++i )
                            dsev.params.push_back( values[i] );

                        //If we have a 0xAA or 0xA9 event, write their value to the meta-data, as those are track-wide 90% of the time
                        //!#TODO: Allow the user to input the value of those events/meta-data entries manually!
                        if( dsev.evcode == static_cast<uint8_t>(eTrkEventCodes::SetSwdl) && 
                            dsev.params.size() == 1 && 
                            dsemeta.unk1 == 0 )
                        {
                            dsemeta.unk1 = dsev.params.front();
                        }
                        else if( dsev.evcode == static_cast<uint8_t>(eTrkEventCodes::SetBank) && 
                                 dsev.params.size() == 1 && 
                                 dsemeta.unk2 == 0 )
                        {
                            dsemeta.unk2 = dsev.params.front();
                        }

                        //Insert parsed event
                        trks.at(evchan).push_back( dsev );
                    }
                    else
                    {
                        clog << "<!>- Ignored text DSE event because it was lacking a channel ID or/and even ID!! :\n"
                             << "\t" <<evtxt <<"\n";
                    }
                }
            }
        }

        /****************************************************************************************
            HandleMetaEvents
                Handle misc midi meta-events
        ****************************************************************************************/
        void HandleMetaEvents( const jdksmidi::MIDITimedBigMessage & mev,
                               TrkState                            & state,
                               MusicTrack                          & trk )
        {
            using namespace jdksmidi;
            //!#TODO
        }


        /****************************************************************************************
            HandleEoT
                Inserts the end of track marker.
        ****************************************************************************************/
        void HandleEoT( const jdksmidi::MIDITimedBigMessage & mev,
                        vector<TrkState>                    & states,
                        vector<MusicTrack>                  & trks,
                        ticks_t                             & globalticks)
        {

            for( size_t cnttrk = 0; cnttrk < trks.size(); ++cnttrk  )
            {
                ticks_t trktickdiff   = (globalticks - states[cnttrk].ticks); //The difference in ticks between the track's last tick and the current global tick
                ticks_t evtglobaltick = mev.GetTime(); //The global absolute tick of the event

                //Insert a pause if needed
                ticks_t delta = (evtglobaltick - states[cnttrk].ticks);
                if( delta != 0 )
                    HandlePauses(mev, states[cnttrk], trks[cnttrk], globalticks, delta);

                InsertDSEEvent( trks[cnttrk], eTrkEventCodes::EndOfTrack );
            }
        }


        void HandleControlChanges( const jdksmidi::MIDITimedBigMessage & mev,
                                   TrkState                            & state,
                                   MusicTrack                          & trk )
        {
        }

        /****************************************************************************************
        ****************************************************************************************/
        void HandleSetTempo( const jdksmidi::MIDITimedBigMessage & mev,
                             vector<TrkState>                    & states,
                             vector<MusicTrack>                  & trks,
                             ticks_t                             & globalticks )
        {
           // ticks_t trktickdiff   = (globalticks - states[0].ticks); //The difference in ticks between the track's last tick and the current global tick
            ticks_t evtglobaltick = mev.GetTime(); //The global absolute tick of the event

            //Insert a pause if needed
            ticks_t delta = (evtglobaltick - states[0].ticks);
            if( delta != 0 )
                HandlePauses(mev, states[0], trks[0], globalticks, delta);

            //
            uint8_t tempo = static_cast<uint8_t>(ConvertMicrosecPerQuarterNoteToBPM( mev.GetTempo() ));
            InsertDSEEvent( trks[0], eTrkEventCodes::SetTempo, {tempo} );
        }

        void HandlePanChanges( const jdksmidi::MIDITimedBigMessage & mev,
                               TrkState                            & state,
                               MusicTrack                          & trk )
        {
        }

        void HandlePitchBend( const jdksmidi::MIDITimedBigMessage & mev,
                              TrkState                            & state,
                              MusicTrack                          & trk )
        {
        }

        void HandleProgramChange( const jdksmidi::MIDITimedBigMessage & mev,
                                  TrkState                            & state,
                                  MusicTrack                          & trk )
        {
        }


    private:

        static void InsertDSEEvent( MusicTrack & trk, DSE::eTrkEventCodes evcode, std::vector<uint8_t> && params )
        {
            DSE::TrkEvent dsev;
            dsev.evcode = static_cast<uint8_t>(evcode);
            dsev.params = std::move(params);
            trk.push_back( dsev );
        }

        static void InsertDSEEvent( MusicTrack & trk, DSE::eTrkEventCodes evcode )
        {
            DSE::TrkEvent dsev;
            dsev.evcode = static_cast<uint8_t>(evcode);
            trk.push_back( dsev );
        }

        void InsertSetOctaveEvent( MusicTrack & trk, TrkState & state, uint8_t newoctave ) 
        {
            InsertDSEEvent( trk, DSE::eTrkEventCodes::SetOctave, {newoctave} );
            state.lastoctave = newoctave;
        }

    private:

        bool isAccurateTime()const
        {
            return m_bIsAccurateTime;
        }

    private:
        const string m_srcpath;
        bool         m_bIsAccurateTime;
    };

//======================================================================================
//  Functions
//======================================================================================

    void SequenceToMidi( const std::string              & outmidi, 
                         const MusicSequence            & seq, 
                         const SMDLPresetConversionInfo & remapdata,
                         int                              nbloop,
                         /*eMIDIFormat                      midfmt,*/
                         eMIDIMode                        midmode )
    {
        if( utils::LibWide().isLogOn() )
        {
            clog << "================================================================================\n"
                 << "Converting SMDL to MIDI " <<outmidi << "\n"
                 << "================================================================================\n";
        }
        DSESequenceToMidi( outmidi, seq, remapdata, /*midfmt,*/ midmode, nbloop )();
    }

    void SequenceToMidi( const std::string              & outmidi, 
                         const MusicSequence            & seq, 
                         int                              nbloop,
                         /*eMIDIFormat                      midfmt,*/
                         eMIDIMode                        midmode )
    {
        if( utils::LibWide().isLogOn() )
        {
            clog << "================================================================================\n"
                 << "Converting SMDL to MIDI " <<outmidi << "\n"
                 << "================================================================================\n";
        }
        DSESequenceToMidi( outmidi, seq, /*midfmt,*/ midmode, nbloop )();
    }



    /*************************************************************************************************
        MidiToSequence
            Converts a MIDI file into a DSE Sequence.
    *************************************************************************************************/
    MusicSequence MidiToSequence( const std::string & inmidi )
    {
        if( utils::LibWide().isLogOn() )
        {
            clog << "================================================================================\n"
                 << "Converting MIDI " <<inmidi << "to SMDL\n"
                 << "================================================================================\n";
        }
        return MIDIToDSE(inmidi)();
    }
};