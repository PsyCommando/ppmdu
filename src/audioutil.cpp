#include "audioutil.hpp"
#include <ppmdu/utils/utility.hpp>
#include <ppmdu/utils/cmdline_util.hpp>
#include <ppmdu/pmd2/pmd2_audio_data.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>

#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/Exception.h>

#include <jdksmidi/world.h>
#include <jdksmidi/track.h>
#include <jdksmidi/multitrack.h>
#include <jdksmidi/filereadmultitrack.h>
#include <jdksmidi/fileread.h>
#include <jdksmidi/fileshow.h>
#include <jdksmidi/filewritemultitrack.h>

using namespace ::utils::cmdl;
using namespace ::utils::io;
using namespace ::std;

namespace audioutil
{
//=================================================================================================
//  CAudioUtil
//=================================================================================================

//------------------------------------------------
//  Constants
//------------------------------------------------
    const string CAudioUtil::Exe_Name            = "ppmd_audioutil.exe";
    const string CAudioUtil::Title               = "Music and sound import/export tool.";
    const string CAudioUtil::Version             = "0.1";
    const string CAudioUtil::Short_Description   = "A utility to export and import music and sounds from the PMD2 games.";
    const string CAudioUtil::Long_Description    = 
        "#TODO";
    const string CAudioUtil::Misc_Text           = 
        "Named in honour of Baz, the awesome Poochyena of doom ! :D\n"
        "My tools in binary form are basically Creative Commons 0.\n"
        "Free to re-use in any ways you may want to!\n"
        "No crappyrights, all wrongs reversed! :3";

//
//
//
    //void WriteMusicDump( const pmd2::audio::MusicSequence & seq, const std::string & fname );
    void WriteEventsToMidiFileTest( const std::string & file, const pmd2::audio::MusicSequence & seq );
    void WriteEventsToMidiFileTest_MF( const std::string & file, const pmd2::audio::MusicSequence & seq );

//------------------------------------------------
//  Arguments Info
//------------------------------------------------
    /*
        Data for the automatic argument parser to work with.
    */
    const vector<argumentparsing_t> CAudioUtil::Arguments_List =
    {{
        //Input Path argument
        { 
            0,      //first arg
            false,  //false == mandatory
            true,   //guaranteed to appear in order
            "input path", 
            "Path to the file/directory to export, or the directory to assemble.",
#ifdef WIN32
            "\"c:/pmd_romdata/data.bin\"",
#elif __linux__
            "\"/pmd_romdata/data.bin\"",
#endif
            std::bind( &CAudioUtil::ParseInputPath, &GetInstance(), placeholders::_1 ),
        },
        //Output Path argument
        { 
            1,      //second arg
            true,   //true == optional
            true,   //guaranteed to appear in order
            "output path", 
            "Output path. The result of the operation will be placed, and named according to this path!",
#ifdef WIN32
            "\"c:/pmd_romdata/data\"",
#elif __linux__
            "\"/pmd_romdata/data\"",
#endif
            std::bind( &CAudioUtil::ParseOutputPath, &GetInstance(), placeholders::_1 ),
        },
    }};




//------------------------------------------------
//  Options Info
//------------------------------------------------

    /*
        Information on all the switches / options to allow the automated parser 
        to parse them.
    */
    const vector<optionparsing_t> CAudioUtil::Options_List=
    {{
        //Force Import
        //{
        //    "i",
        //    0,
        //    "Specifying this will force import!",
        //    "-i",
        //    std::bind( &CAudioUtil::ParseOptionForceImport, &GetInstance(), placeholders::_1 ),
        //},
    }};


//------------------------------------------------
// Misc Methods
//------------------------------------------------

    CAudioUtil & CAudioUtil::GetInstance()
    {
        static CAudioUtil s_util;
        return s_util;
    }

    CAudioUtil::CAudioUtil()
        :CommandLineUtility()
    {
        m_operationMode = eOpMode::Invalid;
    }

    const vector<argumentparsing_t> & CAudioUtil::getArgumentsList   ()const { return Arguments_List;    }
    const vector<optionparsing_t>   & CAudioUtil::getOptionsList     ()const { return Options_List;      }
    const argumentparsing_t         * CAudioUtil::getExtraArg        ()const { return nullptr;           } //No extra args
    const string                    & CAudioUtil::getTitle           ()const { return Title;             }
    const string                    & CAudioUtil::getExeName         ()const { return Exe_Name;          }
    const string                    & CAudioUtil::getVersionString   ()const { return Version;           }
    const string                    & CAudioUtil::getShortDescription()const { return Short_Description; }
    const string                    & CAudioUtil::getLongDescription ()const { return Long_Description;  }
    const string                    & CAudioUtil::getMiscSectionText ()const { return Misc_Text;         }

//--------------------------------------------
//  Utility
//--------------------------------------------



//--------------------------------------------
//  Parse Args
//--------------------------------------------
    bool CAudioUtil::ParseInputPath( const string & path )
    {
        Poco::File inputfile(path);

        //check if path exists
        if( inputfile.exists() && ( inputfile.isFile() || inputfile.isDirectory() ) )
        {
            m_inputPath = path;
            return true;
        }
        return false;
    }
    
    bool CAudioUtil::ParseOutputPath( const string & path )
    {
        Poco::Path outpath(path);

        if( outpath.isFile() || outpath.isDirectory() )
        {
            m_outputPath = path;
            return true;
        }
        return false;
    }


//
//  Parse Options
//

    //bool CAudioUtil::ParseOptionPk( const std::vector<std::string> & optdata )
    //{
    //}


//
//  Program Setup and Execution
//
    int CAudioUtil::GatherArgs( int argc, const char * argv[] )
    {
        int returnval = 0;
        //Parse arguments and options
        try
        {
            if( !SetArguments(argc,argv) )
                return -3;

            DetermineOperation();
        }
        catch( Poco::Exception pex )
        {
            cerr <<"\n<!>-POCO Exception - " <<pex.name() <<"(" <<pex.code() <<") : " << pex.message() <<"\n" <<endl;
            cout <<"=======================================================================\n"
                 <<"Readme\n"
                 <<"=======================================================================\n";
            PrintReadme();
            return pex.code();
        }
        catch( exception e )
        {
            cerr <<"\n<!>-Exception: " << e.what() <<"\n" <<endl;
            cout <<"=======================================================================\n"
                 <<"Readme\n"
                 <<"=======================================================================\n";
            PrintReadme();
            return -3;
        }
        return returnval;
    }

    void CAudioUtil::DetermineOperation()
    {
        Poco::Path inpath( m_inputPath );
        Poco::File infile( inpath );

        if( m_operationMode != eOpMode::Invalid )
            return; //Skip if we have a forced mode         

        if( !m_outputPath.empty() && !Poco::File( Poco::Path( m_outputPath ).makeAbsolute().parent() ).exists() )
            throw runtime_error("Specified output path does not exists!");

        if( infile.exists() )
        {
            if( infile.isFile() )
            {
                stringstream sstr;
                string fext = inpath.getExtension();
                std::transform(fext.begin(), fext.end(), fext.begin(), ::tolower);

                if( fext == pmd2::audio::SMDL_FileExtension )
                    m_operationMode = eOpMode::ExportSMDL;
                else if( fext == pmd2::audio::SEDL_FileExtension )
                    m_operationMode = eOpMode::ExportSEDL;
                else if( fext == pmd2::audio::SWDL_FileExtension )
                    m_operationMode = eOpMode::ExportSWDL;
                else
                    throw runtime_error("Can't import this file format!");
            }
            else if( infile.isDirectory() )
            {

                //if( m_hndlStrings )
                //{
                //    if( m_force == eOpForce::Export )
                //        m_operationMode = eOpMode::ExportGameStrings;
                //    else
                //        throw runtime_error("Can't import game strings from a directory : " + m_inputPath);
                //}
                //else if( m_hndlItems )
                //{
                //    if( m_force == eOpForce::Export )
                //        m_operationMode = eOpMode::ExportItemsData;
                //    else
                //        m_operationMode = eOpMode::ImportItemsData;
                //}
                //else if( m_hndlMoves )
                //{
                //    if( m_force == eOpForce::Export )
                //        m_operationMode = eOpMode::ExportMovesData;
                //    else
                //        m_operationMode = eOpMode::ImportMovesData;

                //}
                //else if( m_hndlPkmn )
                //{
                //    if( m_force == eOpForce::Export )
                //        m_operationMode = eOpMode::ExportPokemonData;
                //    else
                //        m_operationMode = eOpMode::ImportPokemonData;
                //}
                //else
                //{
                //    if( m_force == eOpForce::Import || isImportAllDir(m_inputPath) )
                //        m_operationMode = eOpMode::ImportAll;
                //    else
                //        m_operationMode = eOpMode::ExportAll; //If all else fails, try an export all!
                //}
            }
            else
                throw runtime_error("Cannot determine the desired operation!");
        }
        else
            throw runtime_error("The input path does not exists!");

    }

    int CAudioUtil::Execute()
    {
        int returnval = -1;
        utils::MrChronometer chronoexecuter("Total time elapsed");
        try
        {
            switch(m_operationMode)
            {
                case eOpMode::ExportSWDLBank:
                {
                    cout << "=== Exporting SWD Bank ===\n";
                    returnval = ExportSWDLBank();
                    break;
                }
                case eOpMode::ExportSWDL:
                {
                    cout << "=== Exporting SWD ===\n";
                    returnval = ExportSWDL();
                    break;
                }
                case eOpMode::ExportSMDL:
                {
                    cout << "=== Exporting SMD ===\n";
                    returnval = ExportSMDL();
                    break;
                }
                case eOpMode::ExportSEDL:
                {
                    cout << "=== Exporting SED ===\n";
                    returnval = ExportSEDL();
                    break;
                }
                case eOpMode::BuildSWDL:
                {
                    cout << "=== Building SWD ===\n";
                    returnval = BuildSWDL();
                    break;
                }
                case eOpMode::BuildSMDL:
                {
                    cout << "=== Building SMD ===\n";
                    returnval = BuildSMDL();
                    break;
                }
                case eOpMode::BuildSEDL:
                {
                    cout << "=== Building SED ===\n";
                    returnval = BuildSEDL();
                    break;
                }
                default:
                {
                    throw runtime_error( "Invalid operation mode. Something is wrong with the arguments!" );
                }
            };
        }
        catch(Poco::Exception & e )
        {
            cerr <<"\n" << "<!>- POCO Exception - " <<e.name() <<"(" <<e.code() <<") : " << e.message() <<"\n" <<endl;
        }
        catch( exception &e )
        {
            cerr <<"\n" << "<!>- Exception - " <<e.what() <<"\n" <<"\n";
        }
        return returnval;
    }


//--------------------------------------------
//  Operation
//--------------------------------------------
    int CAudioUtil::ExportSWDLBank()
    {
        return 0;
    }
    
    int CAudioUtil::ExportSWDL()
    {
        return 0;
    }

    int CAudioUtil::ExportSMDL()
    {
        using namespace pmd2::audio;
        Poco::Path inputfile(m_inputPath);
        Poco::Path outputfile;
        string     outfname;

        if( ! m_outputPath.empty() )
            outputfile = Poco::Path(m_outputPath);
        else
            outputfile = inputfile.parent().append( inputfile.getBaseName() ).makeFile();

        outfname = outputfile.getBaseName();

        //Export
        MusicSequence smd = LoadSequence( inputfile.toString() );

        //Write output
        ofstream outstr( outputfile.toString() );
        outstr<<smd.tostr();
        outstr.flush();
        outstr.close();

        ofstream outfile( outputfile.toString() );
        outfile << smd.tostr();
        outfile.close();

        //WriteEventsToMidiFileTest_MF( outputfile.toString(), smd );
        outputfile.setExtension("mid");
        WriteEventsToMidiFileTest( outputfile.toString(), smd );

        //Write meta
        //Write the info that didn't fit in the midi here as XML.

        //Finish

        return 0;
    }

    void WriteEventsToMidiFileTest( const std::string & file, const pmd2::audio::MusicSequence & seq )
    {
        using namespace jdksmidi;
        MIDIMultiTrack mt( seq.getNbTracks() );
        mt.SetClksPerBeat(48);

        //Init track 0 with time signature
        MIDITimedBigMessage timesig;
        timesig.SetTime( 0 );
        timesig.SetTimeSig();
        mt.GetTrack( 0 )->PutEvent( timesig );
        //mt.GetTrack( 0 )->PutTextEvent(0, META_TRACK_NAME, "Tempo track");

        //Re-assign drumtrack #TODO: It will be important to work out something for tracks set to chan 10 that gets a program change midway !
        vector<uint8_t> midichan  ( seq.getNbTracks(), 0 );
        vector<int>    drumtracks;
        int             wrongdrumtrk = -1;
        for( int i = 0; i < seq.getNbTracks(); ++i )
        {
            midichan[i] = seq.track(i).GetMidiChannel();

            bool Iamdrumtrack = false;

            //Find if we're actually a drum track
            for( const auto & ev : seq.track(i) )
            {
                if( ev.evcode == static_cast<uint8_t>(DSE::eTrkEventCodes::SetPreset) && (ev.params.front() == 0x7F || ev.params.front() == 0x7B || ev.params.front() == 0x7E ) )
                {
                    Iamdrumtrack = true;
                    drumtracks.push_back(i);
                    break;
                }
            }

            if( midichan[i] == 9 && !Iamdrumtrack ) //Keep track of who got track 9
                wrongdrumtrk = i;
        }

        //If we can, swap the channel with one of the drumtracks
        if( wrongdrumtrk != -1 && ! drumtracks.empty() )
        {
            uint8_t swapchan = 0;
            swapchan = midichan[wrongdrumtrk];
            midichan[wrongdrumtrk] = midichan[drumtracks.front()];
            midichan[drumtracks.front()] = swapchan;
            cout <<"!!-- Re-assigned track #" <<wrongdrumtrk <<"'s MIDI channel from 9 to " << static_cast<short>(midichan[wrongdrumtrk]) <<"--!!\n";
        }

        //Set the remaining drumtracks to 10
        for( const auto & drumtrk : drumtracks )
        {
            cout <<"!!-- Re-assigned track #" <<drumtrk <<"'s MIDI channel from " << static_cast<short>(midichan[drumtrk]) <<" to  9 --!!\n";
            midichan[drumtrk] = 9;
        }

        uint32_t lastlasttick = 0; //The nb of ticks of the last track that was handled
        
        for( unsigned int trkno = 0; trkno < seq.getNbTracks(); ++trkno )
        {
            cout<<"Writing track #" <<trkno <<"\n";
            uint32_t ticks     = 0;
            uint32_t lastpause = 0;
            uint32_t lasthold  = 0; //last duration a note was held
            int8_t   curoctave = 0;
            int8_t   lastoctaveevent = 0;

            uint8_t curchannel = midichan[trkno];

            for( const auto & ev : seq.track(trkno) )
            {
                MIDITimedBigMessage mess;
                const auto code = static_cast<DSE::eTrkEventCodes>(ev.evcode);

                //Handle pauses if neccessary
                if( code == DSE::eTrkEventCodes::LongPause )
                {
                    lastpause = (static_cast<uint16_t>(ev.params.back()) << 8) | ev.params.front();
                    ticks += lastpause;
                }
                else if( code == DSE::eTrkEventCodes::Pause )
                {
                    lastpause = ev.params.front();
                    ticks += lastpause;
                }
                else if( code == DSE::eTrkEventCodes::AddToLastPause )
                {
                    uint32_t prelastp = lastpause;
                    lastpause = prelastp + ev.params.front();
                    ticks += lastpause;
                }
                else if( code == DSE::eTrkEventCodes::RepeatLastPause )
                {
                    ticks += lastpause;
                }

                //Handle delta-time
                if( ev.dt != 0 )
                    ticks += static_cast<uint8_t>( DSE::TrkDelayCodeVals.at( ev.dt ) );

                //Set the time properly now
                mess.SetTime(ticks);

                //Warn if we're writing notes past the ticks of the last track
                if( lastlasttick!= 0 && ticks > lastlasttick )
                    cout << "!!-- WARNING: Writing events past the last parsed track's end ! Prev track last ticks : " <<lastlasttick <<", current ticks : " <<ticks <<" --!!\n" ;

                if( trkno == 0 )
                {
                    if( code == DSE::eTrkEventCodes::SetTempo )
                    {
                        static const uint32_t NbMicrosecPerMinute = 60000000;
                        uint32_t microspquart= NbMicrosecPerMinute / ev.params.front();
                        //uint8_t asbytes[3] = { static_cast<uint8_t>(microspquart >> 16), static_cast<uint8_t>(microspquart >> 8), static_cast<uint8_t>(microspquart) };
                        //mf.addMetaEvent( trkno, ticks, 0x51, vector<uint8_t>{0x03, asbytes[0], asbytes[1], asbytes[2] } );
                        
                        mess.SetTempo( microspquart );
                        mt.GetTrack(trkno)->PutEvent( mess );
                    }
                    continue;
                }

                //Handle play note
                if( code >= DSE::eTrkEventCodes::NoteOnBeg && code <= DSE::eTrkEventCodes::NoteOnEnd )
                {
                    if( (ev.params.front() & DSE::NoteEvParam1PitchMask) == static_cast<uint8_t>(DSE::eNotePitch::lower) )
                        curoctave -= 1;
                    else if( (ev.params.front() & DSE::NoteEvParam1PitchMask) == static_cast<uint8_t>(DSE::eNotePitch::higher) )
                        curoctave += 1;
                    else if( (ev.params.front() & DSE::NoteEvParam1PitchMask) == static_cast<uint8_t>(DSE::eNotePitch::undefined) )
                    {
                        //cout <<"Undefined pitch ! Reseting\n";
                        curoctave = lastoctaveevent;
                    }
                    else if( (ev.params.front() & DSE::NoteEvParam1PitchMask) == static_cast<uint8_t>(DSE::eNotePitch::current) )
                    {
                        //cout <<"Trying to reset pitch\n";
                    }

                    //MidiEvent mev;
                    int8_t notenb  = (ev.params.front() & 0x0F);
                    int8_t mnoteid = notenb + ( (curoctave) * 12 ); //Midi notes begins at -1 octave, while DSE ones at 0..
                    //mev.makeNoteOn( mnoteid, ev.evcode, trkno );
                    //mf.addEvent( trkno, ticks, vector<uint8_t>{ static_cast<uint8_t>(eMidiMessCodes::NoteOn), static_cast<uint8_t>(mnoteid & 0x7F), static_cast<uint8_t>(ev.evcode & 0x7F) } );
                    //MidiEvent mevoff;
                    //mevoff.makeNoteOff( mnoteid, ev.evcode, trkno );
                    mess.SetTime(ticks);
                    mess.SetNoteOn( curchannel, mnoteid, static_cast<uint8_t>(ev.evcode & 0x7F) );
                    mt.GetTrack(trkno)->PutEvent( mess );
                     
                    //uint8_t notehold = 0; //By default hold for the shortest note duration
                    if( ev.params.size() >= 2 )
                    {
                        if( ev.params.size() == 2 )
                            lasthold = ev.params[1];
                        else if( ev.params.size() == 3 )
                        {
                            lasthold = static_cast<uint16_t>( ev.params[1] << 8 ) | ev.params[2];
                            //cout<<"##Got Note Event with 2 bytes long hold! Parsed as " <<lasthold <<"!##\n";
                        }
                        else if( ev.params.size() == 4 )
                        {
                            lasthold = static_cast<uint32_t>( ev.params[1] << 16 ) | ( ev.params[2] << 8 ) | ev.params[3];
                            cout<<"##Got Note Event with 3 bytes long hold! Parsed as " <<lasthold <<"!##\n";
                            
                        }
                    }
                    MIDITimedBigMessage noteoff;
                    noteoff.SetTime( ticks + lasthold );
                    noteoff.SetNoteOff( curchannel, mnoteid, static_cast<uint8_t>(ev.evcode & 0x7F) ); //Set proper channel from original track eventually !!!!

                    mt.GetTrack(trkno)->PutEvent( noteoff );

                    //mf.addEvent( trkno, ticks + notehold, vector<uint8_t>{ static_cast<uint8_t>(eMidiMessCodes::NoteOff), static_cast<uint8_t>(mnoteid & 0x7F), static_cast<uint8_t>(ev.evcode & 0x7F) } );
                }
                else if( code == DSE::eTrkEventCodes::SetOctave )
                {
                    lastoctaveevent = ev.params.front();
                    curoctave = ev.params.front();
                }
                else if( code == DSE::eTrkEventCodes::SetExpress )
                {
                    mess.SetControlChange( curchannel, 0x0B, ev.params.front() );
                    mt.GetTrack(trkno)->PutEvent( mess );
                    //mf.addEvent( trkno, ticks, vector<uint8_t>{ static_cast<uint8_t>(eMidiMessCodes::CtrlChange), 0x0B, ev.params.front() } );
                }
                else if( code == DSE::eTrkEventCodes::SetTrkVol )
                {
                    mess.SetControlChange( curchannel, 0x07, ev.params.front() );
                    mt.GetTrack(trkno)->PutEvent( mess );
                    //mf.addEvent( trkno, ticks, vector<uint8_t>{ static_cast<uint8_t>(eMidiMessCodes::CtrlChange), 0x07, ev.params.front() } );
                }
                else if( code == DSE::eTrkEventCodes::SetTrkPan )
                {
                    mess.SetControlChange( curchannel, 0x0A, ev.params.front() );
                    mt.GetTrack(trkno)->PutEvent( mess );
                    //mf.addEvent( trkno, ticks, vector<uint8_t>{ static_cast<uint8_t>(eMidiMessCodes::CtrlChange), 0x0A, ev.params.front() } );
                }
                else if( code == DSE::eTrkEventCodes::SetPreset )
                {
                    //mess.SetProgramChange( seq.track(trkno).GetMidiChannel(), ev.params.front() );
                    mess.SetProgramChange( curchannel, 1 ); //For now all pianos !
                    mt.GetTrack(trkno)->PutEvent( mess );
                   // mf.addEvent( trkno, ticks, vector<uint8_t>{ static_cast<uint8_t>(eMidiMessCodes::PrgmChange), ev.params.front() } );
                }
                else if( code == DSE::eTrkEventCodes::Modulate )
                {
                    uint16_t pitchw = ( static_cast<uint16_t>( (ev.params.front() & 0xF) << 8 ) | ev.params.back());
                    cout <<"Pitch Bend ! " <<pitchw <<"\n";

                    //MIDITimedBigMessage portatoggle;
                    //portatoggle.SetTime(ticks);

                    //if( ev.params.front() != 0 && ev.params.back() != 0 )
                    //    portatoggle.SetControlChange( curchannel, 65, 64 ); //Toggle portamento on (64 == on)
                    //else
                    //    portatoggle.SetControlChange( curchannel, 65, 63 ); //Toggle portamento off (63 == off)
                    //mt.GetTrack(trkno)->PutEvent( portatoggle );

                    //Fine Portatime 0x25
                    
                   // mess.SetControlChange( curchannel, 0x25,  ev.params.back() );
                    //mess.SetByte5( ev.params.front() );


                    //mess.SetPitchBend( seq.track(trkno).GetMidiChannel(), pitchw );

                    mess.SetControlChange( curchannel, 1, ev.params.back() );
                    mess.SetByte5( ev.params.front() );
                    mt.GetTrack(trkno)->PutEvent( mess );
                }

            }

            //Track is done, save last tick value
            lastlasttick = ticks;
        }

        //After all done
        mt.SortEventsOrder();

        MIDIFileWriteStreamFileName out_stream( file.c_str() );

        // then output the stream like my example does, except setting num_tracks to match your data

        if( out_stream.IsValid() )
        {
            // the object which takes the midi tracks and writes the midifile to the output stream
            MIDIFileWriteMultiTrack writer( &mt, &out_stream );

            // write the output file
            if ( writer.Write( mt.GetNumTracks() ) )
            {
                cout << "\nOK writing file " << file << endl;
            }
            else
            {
                cerr << "\nError writing file " << file << endl;
            }
        }
        else
        {
            cerr << "\nError opening file " << file << endl;
        }
    }

    enum struct eMidiMessCodes : uint8_t
    {
        NoteOff     = 0x80,
        NoteOn      = 0x90,
        Aftertouch  = 0xA0,
        CtrlChange  = 0xB0,
        PrgmChange  = 0xC0,
        ChanPress   = 0xD0,
        PitchWheel  = 0xE0,
        SySexcl     = 0xF0,
    };

    //midifile::MidiEvent ToMidiEvent( DSE::TrkEvent ev )
    //{
    //    const auto code = static_cast<DSE::eTrkEventCodes>( ev.evcode );
    //    if( code >= DSE::eTrkEventCodes::NoteOnBeg && code <= DSE::eTrkEventCodes::NoteOnEnd )
    //    {

    //    }
    //    else
    //    {
    //        switch( code )
    //        {
    //            case DSE::eTrkEventCodes::SetTempo:
    //            {
    //                break;
    //            }
    //            case DSE::eTrkEventCodes::SetExpress:
    //            {
    //                break;
    //            }
    //            case DSE::eTrkEventCodes::SetTrkVol:
    //            {
    //                break;
    //            }
    //            case DSE::eTrkEventCodes::SetTrkPan:
    //            {
    //                break;
    //            }
    //            case DSE::eTrkEventCodes::SetPreset:
    //            {
    //                break;
    //            }
    //            case DSE::eTrkEventCodes::Modulate:
    //            {
    //                break;
    //            }
    //        };
    //    }

    //    return ;
    //}

    //void WriteEventsToMidiFileTest_MF( const std::string & file, const pmd2::audio::MusicSequence & seq )
    //{
    //    //using namespace midifile;
    //    MidiFile mf;
    //    
    //    mf.absoluteTicks();
    //    mf.addTrack( (seq.getNbTracks()-1) );
    //    mf.setTicksPerQuarterNote( 48 );

    //    //Add time sig event on track 0
    //    mf.addEvent( 0, 0, vector<uint8_t>{ 0xFF, 0x58, 4, 4, 2, 24, 8 } );

    //    for( unsigned int trkno = 0; trkno < seq.getNbTracks(); ++trkno )
    //    {
    //        cout<<"Writing track #" <<trkno <<"\n";
    //        uint32_t ticks     = 0;
    //        uint32_t lastpause = 0;
    //        uint8_t  curoctave = 0;

    //        for( const auto & ev : seq.track(trkno) )
    //        {
    //            const auto code = static_cast<DSE::eTrkEventCodes>(ev.evcode);

    //            //Handle pauses if neccessary
    //            if( code == DSE::eTrkEventCodes::LongPause )
    //            {
    //                lastpause = (static_cast<uint16_t>(ev.params.back()) << 8) | ev.params.front();
    //                ticks += lastpause;
    //            }
    //            else if( code == DSE::eTrkEventCodes::Pause )
    //            {
    //                lastpause = ev.params.front();
    //                ticks += lastpause;
    //            }
    //            else if( code == DSE::eTrkEventCodes::AddToLastPause )
    //            {
    //                uint32_t prelastp = lastpause;
    //                lastpause = prelastp + ev.params.front();
    //                ticks += lastpause;
    //            }
    //            else if( code == DSE::eTrkEventCodes::RepeatLastPause )
    //            {
    //                ticks += lastpause;
    //            }

    //            //Handle delta-time
    //            if( ev.dt != 0 )
    //                ticks += static_cast<uint8_t>( DSE::TrkDelayCodeVals.at( ev.dt ) );


    //            if( trkno == 0 )
    //            {
    //                //if( code == DSE::eTrkEventCodes::SetTempo )
    //                //{
    //                //    static const uint32_t NbMicrosecPerMinute = 60000000;
    //                //    uint32_t microspquart= NbMicrosecPerMinute / ev.params.front();
    //                //    uint8_t asbytes[3] = { static_cast<uint8_t>(microspquart >> 16), static_cast<uint8_t>(microspquart >> 8), static_cast<uint8_t>(microspquart) };
    //                //    mf.addMetaEvent( trkno, ticks, 0x51, vector<uint8_t>{0x03, asbytes[0], asbytes[1], asbytes[2] } );
    //                //}
    //                continue;
    //            }

    //            //Handle play note
    //            if( code >= DSE::eTrkEventCodes::NoteOnBeg && code <= DSE::eTrkEventCodes::NoteOnEnd )
    //            {
    //                //MidiEvent mev;
    //                uint8_t   mnoteid = ev.params.front() & 0x0F + ( (curoctave+1) * 12 ); //Midi notes begins at -1 octave, while DSE ones at 0..
    //                //mev.makeNoteOn( mnoteid, ev.evcode, trkno );
    //                mf.addEvent( trkno, ticks, vector<uint8_t>{ static_cast<uint8_t>(eMidiMessCodes::NoteOn), static_cast<uint8_t>(mnoteid & 0x7F), static_cast<uint8_t>(ev.evcode & 0x7F) } );
    //                //MidiEvent mevoff;
    //                //mevoff.makeNoteOff( mnoteid, ev.evcode, trkno );
    //                 
    //                uint8_t notehold = 2; //By default hold for the shortest note duration
    //                if( ev.params.size() >= 2 )
    //                {
    //                    notehold = ev.params[1];
    //                }

    //                mf.addEvent( trkno, ticks + notehold, vector<uint8_t>{ static_cast<uint8_t>(eMidiMessCodes::NoteOff), static_cast<uint8_t>(mnoteid & 0x7F), static_cast<uint8_t>(ev.evcode & 0x7F) } );
    //            }
    //            else if( code == DSE::eTrkEventCodes::SetOctave )
    //            {
    //                curoctave = ev.params.front();
    //            }
    //            else if( code == DSE::eTrkEventCodes::SetExpress )
    //            {
    //                mf.addEvent( trkno, ticks, vector<uint8_t>{ static_cast<uint8_t>(eMidiMessCodes::CtrlChange), 0x0B, ev.params.front() } );
    //            }
    //            else if( code == DSE::eTrkEventCodes::SetTrkVol )
    //            {
    //                mf.addEvent( trkno, ticks, vector<uint8_t>{ static_cast<uint8_t>(eMidiMessCodes::CtrlChange), 0x07, ev.params.front() } );
    //            }
    //            else if( code == DSE::eTrkEventCodes::SetTrkPan )
    //            {
    //                mf.addEvent( trkno, ticks, vector<uint8_t>{ static_cast<uint8_t>(eMidiMessCodes::CtrlChange), 0x0A, ev.params.front() } );
    //            }
    //            else if( code == DSE::eTrkEventCodes::SetPreset )
    //            {
    //                mf.addEvent( trkno, ticks, vector<uint8_t>{ static_cast<uint8_t>(eMidiMessCodes::PrgmChange), ev.params.front() } );
    //            }
    //        }
    //    }

    //    //FF 2F 00  == End of track

    //    mf.sortTracks();
    //    mf.deltaTicks();

    //    stringstream sstr;
    //    sstr <<file <<".mid";
    //    mf.write( sstr.str() );
    //}

    //void WriteMusicDump( const pmd2::audio::MusicSequence & seq, const std::string & fname )
    //{

    //}

    int CAudioUtil::ExportSEDL()
    {
        return 0;
    }

    int CAudioUtil::BuildSWDL()
    {
        return 0;
    }

    int CAudioUtil::BuildSMDL()
    {
        return 0;
    }

    int CAudioUtil::BuildSEDL()
    {
        return 0;
    }

//--------------------------------------------
//  Main Methods
//--------------------------------------------
    int CAudioUtil::Main(int argc, const char * argv[])
    {
        int returnval = -1;
        PrintTitle();

        //Handle arguments
        returnval = GatherArgs( argc, argv );
        if( returnval != 0 )
            return returnval;
        
        //Execute the utility
        returnval = Execute();

#ifdef _DEBUG
        system("pause");
#endif

        return returnval;
    }
};

//=================================================================================================
// Main Function
//=================================================================================================

//#TODO: Move the main function somewhere else !
int main( int argc, const char * argv[] )
{
    using namespace audioutil;
    try
    {
        CAudioUtil & application = CAudioUtil::GetInstance();
        return application.Main(argc,argv);
    }
    catch( exception & e )
    {
        cout<< "<!>-ERROR:" <<e.what()<<"\n"
            << "If you get this particular error output, it means an exception got through, and the programmer should be notified!\n";
    }

    return 0;
}