#include "pmd2_audio_data.hpp"
#include <ppmdu/fmts/dse_common.hpp>
#include <ppmdu/fmts/dse_sequence.hpp>
#include <ppmdu/fmts/dse_interpreter.hpp>

#include <ppmdu/fmts/sedl.hpp>
#include <ppmdu/fmts/smdl.hpp>
#include <ppmdu/fmts/swdl.hpp>
#include <sstream>
#include <iomanip>
#include <iostream>

#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>

#include <ppmdu/ext_fmts/adpcm.hpp>
#include <ppmdu/ext_fmts/sf2.hpp>

using namespace std;
using namespace DSE;

namespace pmd2 { namespace audio
{

    static const std::vector<uint8_t> PMD2PresetsToGM 
    {
        0,  // 0
        88, // 1
        5,  // 2
        70, // 3
        70, // 4
        13, // 5
        11, // 6
        9,  // 7
        9,  // 8
        9,  // 9
        46, // 10
        46, // 11
        14, // 12
        116,// 13
        0,
        0,
        0,
        0,
        0,
        0,
        24, // 20
        24, // 21
        15, // 22
        15, // 23 0x17
        38, // 25 0x19
        39, // 26 0x1A
        0,  // 27 0x1B
        0,  // 28 0x1C
        36, // 29 0x1D
        0,  // 30 0x1E
        50, // 31 0x1F
        50, // 32 0x20
        0,
        0,
        52, // 35 0x23
        0,
        0,
        0,
        49, // 39 0x27
        49, // 40 0x28
        0,
        69, // 42 0x2A
        17, // 43 0x2B
        17, // 44 0x2C
        0,
        48, // 46 0x2E
        0,
        109, //0x30
        74,  //0x31
        74,  //0x32
        73,  //0x33
        73,  //0x34
        71,  //0x35
        69,  //0x36
        69,  //0x37
        0,
        0,
        0,
        56,  //0x3B
        0,
        56,  //0x3D
        57,  //0x3E
        58,  //0x3F
        60,  //0x40
        60,  //0x41
        60,  //0x42
        0,
        61,  //0x44
        0,
        0,
        40,  //0x47
        40,  //0x48
        0,
        42,  //0x4A
        45,  //0x4B
        75,  //0x51
        75,  //0x52
        114, //0x53
        104, //0x54
        0,
        0,
        0,
        0,
        0,
        0,
        90,  //0x5B
        0,
        63,  //0x5D
        78,  //0x5E
        78,  //0x5F
        80,  //0x60
        62,  //0x61
        62,  //0x62
        112, //0x63
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        47,  //0x79
        0,
        116,  //0x7B
        0,
        0,
        0,
        0, //0x7F
    };

//
//
//
    std::vector<pcm16s_t> RawBytesToPCM16Vec( std::vector<uint8_t> * praw )
    {
        std::vector<pcm16s_t> out;
        auto                  itread = praw->begin();
        auto                  itend  = praw->end();
        out.reserve(praw->size()/2);

        while( itread != itend )
            out.push_back( utils::ReadIntFromByteVector<pcm16s_t>( itread ) ); //Iterator is incremented

        return std::move(out);
    }

    std::vector<pcm16s_t> DecodeADPCMAndAppendLoopBuff( const std::vector<uint8_t> & adpcm, size_t loopbufpos, size_t loopbuflen )
    {
        std::vector<pcm16s_t> decompressed = std::move(::audio::DecodeADPCM_IMA(adpcm));
        decompressed.reserve( decompressed.size() + loopbuflen );
        std::copy_n( decompressed.begin() + loopbufpos, loopbuflen, back_inserter(decompressed) );
        return std::move(decompressed);
    }

    /*
        Convert the parameters of a DSE envelope to SF2
    */
    sf2::Envelope RemapDSEVolEnvToSF2( int8_t inrate, 
                                       int8_t inlvl, 
                                       int8_t hold, 
                                       int8_t sustain, 
                                       int8_t susrte, 
                                       int8_t decrte, 
                                       int8_t rel, 
                                       int8_t reldecay )
    {
        sf2::Envelope volenv;

        // 0-127
        //-12,000 to 5,000
        static const int16_t MinEnvTimeCent  = -12000; //Minimum generator value for all that use timecent as unit
        static const int16_t Max20sec        =   5000; //Maximum generator value for env param that go up to 20 seconds
        static const int16_t Max100sec       =   8000; //Maximum generator value for env param that go up to 100 seconds
        static const int16_t Timecent5Sec    =   2786; //The value of the envelope parameter to represent 5 seconds. The formula is 1200 * log2(DurationInSec)
        static const int16_t SustainMinVolume=   1440; //The higher the value, the more attenuated the sound is. 0 is max volume
        static const int16_t MaxSignedInt8   =    127;

        //Basically : ( (indel * 17000) / 127 ) - 12000
        //          Since the envelope's values range from -12000 to 5000, and the range of our DSE value is 0 to 127, 
        //          we want to shift all SF2 related values into positive ranges only, and after the calculation is done,
        //          shift it back into the SF2 envelope value range. The actual calculation is a simple rule of three!
        //          That way we still have the full 17000 possible values, but all in the positive, which makes it all much easier
        //          to work with !


        //#TODO: Find a way to include the attack level, sustain rate, and RX parameter!!

//        if( inlvl != 0 )
//            volenv.delay   = static_cast<int16_t>( ((inlvl  * (labs(MinEnvTimeCent) + Max100sec) ) / MaxSignedInt8) + MinEnvTimeCent );
        if( inrate != 0 )
            volenv.attack  = static_cast<int16_t>( (( inrate * (labs(MinEnvTimeCent) + Max100sec) ) / MaxSignedInt8 ) + MinEnvTimeCent );
        if( hold != 0 )
        {
            volenv.hold   = static_cast<int16_t>( (( hold *  (labs(MinEnvTimeCent) + Max20sec ) ) / MaxSignedInt8) + MinEnvTimeCent );
            //volenv.hold  += static_cast<int16_t>( (( susrte * (labs(MinEnvTimeCent) + (Max20sec/2) ) ) / MaxSignedInt8 ) + MinEnvTimeCent );
        }
        if( decrte != 0 )
        {
            volenv.decay   = static_cast<int16_t>( (( decrte * (labs(MinEnvTimeCent) + Max100sec) ) / MaxSignedInt8 ) + MinEnvTimeCent );
        }
        if( sustain != 0 )
            volenv.sustain = static_cast<int16_t>( (MaxSignedInt8 - sustain) * 10  );
        if( rel != 0 )
            volenv.release = static_cast<int16_t>( ((rel * (labs(MinEnvTimeCent) + Max100sec) ) / MaxSignedInt8) + MinEnvTimeCent );

        return volenv;
    }

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
                    else if( pitchop == static_cast<uint8_t>(eNotePitch::reset) ) 
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
                    outstr <<dec <<static_cast<short>(prevbpm) <<"->" <<dec <<static_cast<short>(curbpm) <<" )";
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

    std::string MusicSequence::printinfo()const
    {
        stringstream sstr;
        sstr << " ==== " <<m_meta.fname <<" ==== \n"
             << "CREATE ITME : " <<m_meta.createtime <<"\n"
             << "NB TRACKS   : " <<m_tracks.size()   <<"\n"
             << "TPQN        : " <<m_meta.tpqn       <<"\n";

        return sstr.str();
    }

//
//  BatchAudioLoader
//

    BatchAudioLoader::BatchAudioLoader( const std::string & mbank )
        :m_mbankpath(mbank)
    {
    }
    
    void BatchAudioLoader::LoadMasterBank()
    {
        m_master = move( LoadSwdBank( m_mbankpath ) );
    }

    void BatchAudioLoader::LoadMasterBank( const std::string & mbank )
    {
        m_mbankpath = mbank;
        LoadMasterBank();
    }

    void BatchAudioLoader::LoadSmdSwdPair( const std::string & smd, const std::string & swd )
    {
        m_pairs.push_back( move( pmd2::audio::LoadSmdSwdPair( smd, swd ) ) );
    }

    /*
    */
    uint16_t BatchAudioLoader::GetSizeLargestPrgiChunk()const
    {
        uint16_t largestprgi = m_master.metadata().nbprgislots; //Start with the master bank

        //Then look at the loaded pairs
        for( size_t cntpair = 0; cntpair < m_pairs.size(); ++cntpair ) //Iterate through all SWDs
        {
            uint16_t curprgisz = m_pairs[cntpair].second.metadata().nbprgislots;
            if( curprgisz > largestprgi )
                largestprgi = curprgisz;
        }

        return largestprgi;
    }

    /*

    */
    BatchAudioLoader::mergedInstData BatchAudioLoader::PrepareMergedInstrumentTable()const
    {
        //A list of all the shared presets between files merged together. Where non-duplicates are stacked into the second dimension of the table
        std::vector< std::vector<InstrumentInfo*> > merged( GetSizeLargestPrgiChunk() );
        
        //List of what slot the instruments were put into for each SMD+SWD pair
        std::vector< std::map<uint16_t,uint16_t> > smdlPresLocation (m_pairs.size()); 

        for( size_t cntpair = 0; cntpair < m_pairs.size(); ++cntpair ) //Iterate through all SWDs
        {
            const auto & apair      = m_pairs[cntpair];
            auto         ptrinstinf = apair.second.instbank().lock();
            
            if( ptrinstinf != nullptr )
            {
                const auto & curinstlist = ptrinstinf->instinfo();

                for( size_t cntinst = 0; cntinst < curinstlist.size();  ++cntinst ) //Test all the individual instruments and add them to their slot
                {
                    //Compare against all presets for the same prgi slot, to find out whether this is a duplicate or not.
                    if( curinstlist[cntinst] != nullptr )
                    {
                        auto founddup = find_if( merged[cntinst].begin(), 
                                                 merged[cntinst].end(), 
                                                 [&]( const InstrumentInfo * inf )->bool
                        { 
                            if( inf != nullptr )
                                return (inf->isSimilar( *(curinstlist[cntinst].get()) ) != InstrumentInfo::eCompareRes::different );
                            else
                            {
                                throw std::exception("BatchAudioLoader::PrepareMergedInstrumentTable(): Null instrument pointer?!");
                                return false;
                            }
                        });

                        if( founddup != merged[cntinst].end() )
                        {
                            //If its a duplicate, set the existing copy of this instrument as what this SWDL+SMDL pair should use
                            smdlPresLocation[cntpair].insert( make_pair( cntinst, distance( merged[cntinst].begin(), founddup) ) );
                        }
                        else
                        {
                            //Push into merged list for this prgi slot number, if nothing similar found
                            merged[cntinst].push_back( curinstlist[cntinst].get() );
                            smdlPresLocation[cntpair].insert( make_pair( cntinst, merged[cntinst].size()-1 ) );
                        }
                    }
                    //Don't mark the position of null entries
                }
            }
        }

        return std::move(mergedInstData{ std::move(merged), std::move(smdlPresLocation) });
    }

    /*
        DSEInstrumentToSf2Instrument
            Turns a DSE Preset's "Instrument" into a SF2 Instrument zone. And add it to the Soundfont!
            - dseinst       : The instruemnt to convert.
            - smplIdConvTbl : A map mapping the Sample IDs from the DSE swd, to their new ID within the Soundfont file!
            - inst          : The SF2 Instruemnt this dse sample/instrument shall be added to.
    */
    void DSEInstrumentToSf2InstrumentZone( const InstrumentInfo::PrgiSmplEntry & dseinst, 
                                           const std::map<uint16_t,size_t>     & smplIdConvTbl, 
                                           sf2::SoundFont                      & sf,
                                           sf2::Instrument                     & inst,
                                           const vector<bool>                  & loopedsmpls,
                                           uint16_t                              dsepresetid )
    {
        using namespace sf2;

        //Make a global instrument zone
        //ZoneBag globalzone;

        //#TODO: Add stuff here if needed.

        //inst.AddZone( std::move(globalzone) );

        //Make a zone for this entry
        ZoneBag myzone;

        //Set the generators
        myzone.SetKeyRange( dseinst.lowkey, dseinst.hikey ); //Key range in first

        //#TODO: Add more generators here.

        if( loopedsmpls[dseinst.smplid] )
            myzone.SetSmplMode( eSmplMode::loop );

        Envelope myenv = RemapDSEVolEnvToSF2( dseinst.atkrte, 
                                              dseinst.atklvl, 
                                              dseinst.hold, 
                                              dseinst.suslvl, 
                                              dseinst.susrte,
                                              dseinst.decrte, 
                                              dseinst.release, 
                                              dseinst.reldecay );

        myzone.SetVolEnvelope( myenv );

        //int16_t coarse = (dseinst.pitch1 / 250); /*(dseinst.pitch1 / 1000) + *///(dseinst.pitch2 / 1000);
        //int16_t fine   = (/*dseinst.pitch1 +*/ dseinst.pitch2) - (coarse * 1000); //Remove the highest part of the coarse tuning, and keep only the cent
        
        //int16_t pitchshift = DSESamplePitchToCents(dseinst.pitch2); /*+ DSESamplePitchToCents(dseinst.pitch2)*/
        //int16_t finetune   = (pitchshift - (pitchshift/100) *100 );
        //myzone.SetFineTune( finetune ); //cents -99 to 99
        //myzone.SetCoarseTune( (pitchshift / 100) );                  //semitones -120 to 120
        //myzone.SetCoarseTune( dseinst.tune /*DSESamplePitchToSemitone(dseinst.pitch2)*/ );
        myzone.SetCoarseTune( dseinst.ctune );

        //#TEST: Try overriding rootkey for setting the pitch !
        myzone.SetRootKey( dseinst.rootkey);

        //if( dsepresetid == 0x7F ) //Non-chromatic percussion
            //myzone.SetScaleTuning( 0 ); //Never change pitch on midi key

        //Sample ID in last
        myzone.SetSampleId( smplIdConvTbl.at(dseinst.smplid) );
        
        inst.AddZone( std::move(myzone) );
    }

    /*
        DSEPresetToSf2Preset
            Turns a DSE Preset into a SF2 Preset. And add it to the Soundfont!
            - dsePres       : The Preset to convert.
            - smplIdConvTbl : A map mapping the Sample IDs from the DSE swd, to their new ID within the Soundfont file!
    */
    void DSEPresetToSf2Preset( const std::string               & presname, 
                               uint16_t                          bankno, 
                               const InstrumentInfo            & dsePres, 
                               const std::map<uint16_t,size_t> & smplIdConvTbl, 
                               sf2::SoundFont                  & sf,
                               uint16_t                        & instidcnt,
                               const vector<bool>              & loopedsmpls )
    {
        using namespace sf2;
        Preset pre(presname, dsePres.m_hdr.id, bankno );

        //#0 - Add a global zone for global preset settings
        {
            ZoneBag global;

            //#1 - Setup Generators
            global.SetInitAtt( (0x7F - dsePres.m_hdr.insvol) ); //Use the difference between full volume and the current volume to attenuate the preset's volume
        
            // Range of DSE Pan : 0x00 - 0x40 - 0x7F
            // (curpresinf.m_hdr.inspan - 64) * 7.8125f (64 fits 7.8125 times within 500, and 500 is the maximum pan value for soundfont generators)
            double convpan = ((dsePres.m_hdr.inspan - 0x40) * 7.8125); // Remove 64(0x40) to bring the middle to 0.  
            global.SetPan( lround(convpan) ); 
            
            //#2 - Setup Modulators

            pre.AddZone( std::move(global) );
        }       

        //#3 - Handle Table 1
        //  #TODO: Handle the data that's in there!!

        //#4 - Convert instruments
        // A DSE Preset is an SF2 instrument

        std::array<char,20> insame;
        sprintf_s( insame.data(), insame.size(), "Inst%i", instidcnt ); //Had to use this, as stringstreams are just too slow for this..
        Instrument myinst( string( insame.begin(), insame.end() ) );
        ZoneBag    instzone;

        //Iterate through each DSE Preset's associated samples
        for( uint16_t cntsmpl = 0; cntsmpl < dsePres.m_mappedsmpls.size(); ++cntsmpl )
            DSEInstrumentToSf2InstrumentZone( dsePres.m_mappedsmpls[cntsmpl], smplIdConvTbl, sf, myinst, loopedsmpls, dsePres.m_hdr.id );

        sf.AddInstrument( std::move(myinst) );
        instzone.SetInstrumentId(instidcnt);
        pre.AddZone( std::move(instzone) ); //Add the instrument zone after the global zone!
        ++instidcnt; //Increase the instrument count

        sf.AddPreset( std::move(pre) );
    }


    BatchAudioLoader::mergedInstData BatchAudioLoader::ExportSoundfont( const std::string & destf )const
    {
        using namespace sf2;
        //First build a master instrument list from all our pairs, with multiple entries per instruments
        mergedInstData        merged = std::move( PrepareMergedInstrumentTable() ); //MSVC is too dumb to trust with implicit move constructor calls..
        SoundFont             sf( m_master.metadata().fname ); 
        map<uint16_t,size_t>  swdsmplofftosf; //Make a map with as key a the sample id in the Wavi table, and as value the sample id in the sounfont!
        

        //Prepare samples list
        shared_ptr<SampleBank>  samples = m_master.smplbank().lock();
        vector<bool>            loopedsmpls( samples->NbSlots(), false ); //Keep track of which samples are looped

        for( size_t cntsmslot = 0; cntsmslot < samples->NbSlots(); ++cntsmslot )
        {
            if( samples->IsInfoPresent(cntsmslot) && samples->IsDataPresent(cntsmslot) ) 
            {
                const auto & cursminf = *(samples->sampleInfo(cntsmslot));

                Sample::loadfun_t   loadfun;
                Sample::smplcount_t smpllen = 0;
                Sample::smplcount_t loopbeg = 0;
                Sample::smplcount_t loopend = 0;

                if( cursminf.smplfmt == static_cast<uint16_t>( WavInfo::eSmplFmt::ima_adpcm ) )
                {
                    loadfun = std::move( std::bind( ::audio::DecodeADPCM_IMA, std::ref( *samples->sample(cntsmslot) ), 1 ) );
                    smpllen = ::audio::ADPCMSzToPCM16Sz(samples->sample(cntsmslot)->size() );
                    loopbeg = cursminf.loopspos * 4; /** 2 + cursminf.looplen * 2;*/ //ADPCM samples turn to pcm16, 
                    loopend = smpllen;
                }
                else if( cursminf.smplfmt == static_cast<uint16_t>( WavInfo::eSmplFmt::pcm ) )
                {
                    loadfun = std::move( std::bind( &RawBytesToPCM16Vec, samples->sample(cntsmslot) ) );
                    smpllen = samples->sample(cntsmslot)->size()/2;
                    loopbeg = cursminf.loopspos/2;
                    loopend = smpllen;//(cursminf.looplen + cursminf.loopspos) /2;
                }
                else
                {
                    stringstream sstrerr;
                    sstrerr << "Unknown sample format (0x" <<hex <<uppercase <<cursminf.smplfmt <<nouppercase <<dec  <<") encountered !";
                    throw std::runtime_error( sstrerr.str() );
                }

#ifdef _DEBUG
                if( loopbeg > smpllen || loopend > smpllen )
                    assert(false);
#endif

                Sample sm( std::move( loadfun ), smpllen );
                sm.SetName( "smpl#" + to_string(cntsmslot) );
                sm.SetSampleRate ( cursminf.smplrate );

                //Add the pitch offset to the root key 
                //int16_t rootkey = 72;//67 + DSESamplePitchToSemitone(cursminf.pitchoffst); //When set to 72, "C5", its ~5 semitones off, with all the other pitch correction on.
                sm.SetOriginalKey( cursminf.rootkey ); //Default to this for now, but it will be overriden in the instrument anyways
                sm.SetSampleType ( Sample::eSmplTy::monoSample ); //#TODO: Mono samples only for now !

                //#TODO:Come up with a better loop detection logic !!!


                if( loopend > 0 /*(loopend - loopbeg) > 32 && loopbeg >= 8*/ ) //SF2 min loop len
                {
                    sm.SetLoopBounds ( loopbeg, loopend );
                    loopedsmpls[cntsmslot].flip();
                }
                
                swdsmplofftosf.emplace( cntsmslot, sf.AddSample( std::move(sm) ) );
            }
        }

        //Now build the Preset and instrument list !
        auto &   mergedpresets = merged.mergedpresets;
        uint16_t instsf2cnt    = 0; //Used to assign unique instrument IDs

        for( size_t cntpres = 0; cntpres < mergedpresets.size(); ++cntpres )
        {
            for( size_t cntbank = 0; cntbank < mergedpresets[cntpres].size(); ++cntbank )
            {
                if( mergedpresets[cntpres][cntbank] != nullptr )
                {
                    DSEPresetToSf2Preset( "Preset#" + to_string(cntpres), 
                                          cntbank, 
                                          *(mergedpresets[cntpres][cntbank]), 
                                          swdsmplofftosf, 
                                          sf, 
                                          instsf2cnt,
                                          loopedsmpls );
                }
            }

        }

        //Then send that to the Soundfont writing function.

        //Write the soundfont
        sf.Write( destf );

        return std::move(merged);
    }

    void BatchAudioLoader::ExportSoundfontAndMIDIs( const std::string & destdir )const
    {
        //Export the soundfont first
        Poco::Path outsoundfont(destdir);
        outsoundfont.append( outsoundfont.getBaseName() + ".sf2").makeFile();
        cerr<<"<*>- Currently exporting main bank to " <<outsoundfont.toString() <<"\n";
        mergedInstData merged = std::move( ExportSoundfont( outsoundfont.toString() ) );

        //Then the MIDIs
        for( size_t i = 0; i < m_pairs.size(); ++i )
        {
            Poco::Path fpath(destdir);
            fpath.append( to_string(i) + "_" + m_pairs[i].first.metadata().fname);
            fpath.makeFile();
            fpath.setExtension("mid");

            cerr<<"<*>- Currently exporting smd to " <<fpath.toString() <<"\n";
            DSE::SequenceToMidi( fpath.toString(), m_pairs[i].first, merged.filetopreset[i] );
        }
    }

    void BatchAudioLoader::ExportSoundfontAsGM( const std::string                               & destf, 
                                                const std::map< std::string, std::vector<int> > & dsetogm )const
    {
    }


//===========================================================================================
//  Functions
//===========================================================================================
    
    std::pair< PresetBank, std::vector<std::pair<MusicSequence,PresetBank>> > LoadBankAndPairs( const std::string & bank, const std::string & smdroot, const std::string & swdroot )
    {
        PresetBank                                  mbank  = DSE::ParseSWDL( bank );
        vector<std::pair<MusicSequence,PresetBank>> seqpairs;

        //We can't know for sure if they're always in the same directory!
        //vector<string> smdfnames;
        //vector<string> swdfnames;
        Poco::DirectoryIterator dirend;
        Poco::File              dirswd( swdroot );
        Poco::File              dirsmd( smdroot );
        vector<Poco::File>      cntdirswd;
        vector<Poco::File>      cntdirsmd;

        //Fill up file lists
        dirswd.list(cntdirswd);
        dirsmd.list(cntdirsmd);
        
        //Find and load all smd/swd pairs!
        for( const auto & smd : cntdirsmd )
        {
            const string smdbasename = Poco::Path(smd.path()).getBaseName();

            //Find matching swd
            vector<Poco::File>::const_iterator itfound = cntdirswd.begin();
            for( ; itfound != cntdirswd.end(); ++itfound )
            {
                if( smdbasename == Poco::Path(itfound->path()).getBaseName() )
                    break;
            }

            if( itfound == cntdirswd.end() )
            {
                cerr<<"Skipping " <<smdbasename <<".smd, because its corresponding " <<smdbasename <<".swd can't be found!\n";
                continue;
            }

            //Parse the files, and push_back the pair
            seqpairs.push_back( make_pair( DSE::ParseSMDL( smd.path() ), DSE::ParseSWDL( itfound->path() ) ) );
        }

        return make_pair( std::move(mbank), std::move(seqpairs) );
    }

    std::pair<MusicSequence,PresetBank> LoadSmdSwdPair( const std::string & smd, const std::string & swd )
    {
        PresetBank    bank = DSE::ParseSWDL( swd );
        MusicSequence seq  = DSE::ParseSMDL( smd );
        return std::make_pair( std::move(seq), std::move(bank) );
    }

    PresetBank LoadSwdBank( const std::string & file )
    {
        return DSE::ParseSWDL( file );
    }
        
    MusicSequence LoadSequence( const std::string & file )
    {
        return DSE::ParseSMDL( file );
    }


};};