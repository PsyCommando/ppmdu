#include "pmd2_audio_data.hpp"
#include <dse/dse_common.hpp>
#include <dse/dse_sequence.hpp>
#include <dse/dse_interpreter.hpp>
#include <dse/dse_containers.hpp>
#include <utils/library_wide.hpp>

#include <ppmdu/fmts/sedl.hpp>
#include <ppmdu/fmts/smdl.hpp>
#include <ppmdu/fmts/swdl.hpp>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <unordered_map>

#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>

#include <ext_fmts/adpcm.hpp>
#include <ext_fmts/sf2.hpp>

using namespace std;
using namespace DSE;

namespace pmd2 { namespace audio
{
    static const uint16_t DSE_InfiniteAttenuation_cB = 1440; //sf2::SF_GenLimitsInitAttenuation.max_;
    static const uint16_t DSE_LowestAttenuation_cB   =  200; //20dB


    //A multiplier to use to convert from DSE Pan to Soundfont Pan
    static const double BytePanToSoundfontPanMulti = static_cast<double>(sf2::SF_GenLimitsPan.max_ + (-sf2::SF_GenLimitsPan.min_)) / static_cast<double>(DSE_LimitsPan.max_);

    //A multiplier to use to convert from DSE volume to soundfont attenuation.
    static const double ByteVolToSounfontAttnMulti = static_cast<double>(DSE_LowestAttenuation_cB) / static_cast<double>(DSE_LimitsVol.max_);

    /*
        The size of the ADPCM preamble in int32, the unit the NDS uses to store the loop positions
        Mainly here to make things more readable.
    */
    static const int SizeADPCMPreambleWords = ::audio::IMA_ADPCM_PreambleLen / sizeof(int32_t);

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





//===========================================================================================
//  Utility Functions
//===========================================================================================


    inline int16_t DsePanToSf2Pan( int8_t dsepan )
    {
        dsepan = abs(dsepan);

        if( dsepan == DSE_LimitsPan.max_ )
            return sf2::SF_GenLimitsPan.max_;
        else if( dsepan == DSE_LimitsPan.min_ )
            return sf2::SF_GenLimitsPan.min_;
        else
            return static_cast<int16_t>(lround( ( dsepan - DSE_LimitsPan.mid_ ) * BytePanToSoundfontPanMulti ));
    }

    //#FIXME: MOST LIKELY INNACURATE !
    inline uint16_t DseVolToSf2Attenuation( int8_t dsevol )
    {
        dsevol = abs(dsevol);

        //Because of the rounding, we need to make sure our limits match the SF limits
        if( dsevol == DSE_LimitsVol.max_ )
            return sf2::SF_GenLimitsInitAttenuation.min_;
        else if( dsevol ==  DSE_LimitsVol.min_ )
            return DSE_InfiniteAttenuation_cB;
        else
        {
            //Convert to NDS Register attenuation
            //int32_t diff = (0x7F << 0x17) - static_cast<int32_t>(dsevol << 0x17);
            //diff = diff >> 0x18;
            //diff = diff << 0x18;
            //return 20 * log10((double)(0x16980-diff)/(double)0x16980);

            //static const double attmax = 20.0;
            //double test = (dsevol * 100.0) / 127.0; //Get a percent
            //return round( (attmax * log10( test )) * 10 );
            return DSE_LowestAttenuation_cB - round( dsevol * ByteVolToSounfontAttnMulti );
        }
    }

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

    std::vector<pcm16s_t> PCM8RawBytesToPCM16Vec( std::vector<uint8_t> * praw )
    {
        std::vector<pcm16s_t> out;
        auto                  itread = praw->begin();
        auto                  itend  = praw->end();
        out.reserve(praw->size()*2);

        for(; itread != itend; ++itread )
            out.push_back( static_cast<pcm16s_t>(*itread) );

        return std::move(out);
    }

    std::vector<pcm16s_t> DecodeADPCMAndAppendLoopBuff( const std::vector<uint8_t> & adpcm, size_t loopbufpos, size_t loopbuflen )
    {
        std::vector<pcm16s_t> decompressed = std::move(::audio::DecodeADPCM_NDS(adpcm));
        decompressed.reserve( decompressed.size() + loopbuflen );
        std::copy_n( decompressed.begin() + loopbufpos, loopbuflen, back_inserter(decompressed) );
        return std::move(decompressed);
    }

    /*
        Convert the parameters of a DSE envelope to SF2
    */
    sf2::Envelope RemapDSEVolEnvToSF2( int8_t atkvol, 
                                       int8_t attack, 
                                       int8_t hold, 
                                       int8_t decay, 
                                       int8_t sustain, 
                                       int8_t decay2, 
                                       int8_t rel, 
                                       int8_t rx,
                                       int8_t mulatk,
                                       int8_t mul2 )
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

        static const int16_t BaseDuration = 0;//MinEnvTimeCent;


        if( utils::LibWide().isLogOn() )
        {
            clog<<"\tRemaping DSE (atkvol, atk, dec, sus, hold, dec2, rel, rx)( " 
                << static_cast<short>(atkvol)       <<", "
                << static_cast<short>(attack)       <<", "
                << static_cast<short>(decay)         <<", "
                << static_cast<short>(sustain)        <<", "
                << static_cast<short>(hold)      <<", "
                << static_cast<short>(decay2)       <<", "
                << static_cast<short>(rel)          <<", "
                << static_cast<short>(rx)           <<" )\n";
        }

        //Handle Attack
        if( attack != 0 )
        {
            if( utils::LibWide().isLogOn() )
                clog<<"Handling Attack..\n";
            //if( attack!= 0x7F )
                volenv.attack = sf2::MSecsToTimecents( DSEEnveloppeDurationToMSec( attack, mul2 ) );
            //else
            //    volenv.attack = SHRT_MAX; //Infinite
        }
        else if( utils::LibWide().isLogOn() )
            clog<<"Skipping Attack..\n";

        //Handle Hold
        if( hold != 0 )
        {
            if( utils::LibWide().isLogOn() )
                clog<<"Handling Hold..\n";

            //if( hold!= 0x7F )
                volenv.hold = sf2::MSecsToTimecents( DSEEnveloppeDurationToMSec( hold, mul2 ) );
            //else
            //    volenv.hold = SHRT_MAX; //Infinite
        }
        else if( utils::LibWide().isLogOn() )
            clog<<"Skipping Hold..\n";

        //Handle Decay
#if 0
        uint32_t decaytime = 0;
        if( /*decay != 0 &&*/ decay != 0x7F )
        {
            decaytime = DSEEnveloppeDurationToMSec( decay, mul2 );
        }

        if( decay2 != 0x7F )
        {
            decaytime += DSEEnveloppeDurationToMSec( decay2, mul2 );
        }

        if( decay != 0x7F || decay2 != 0x7F )
            volenv.decay = sf2::MSecsToTimecents( decaytime );
        else
            volenv.decay = SHRT_MIN;

        //DSE uses an AHDSDR envelope. There is a second decay after the sustain, and before the key is released. 
        // Its usually disabled (set to 0x7F), but when its enabled, we can simulate it easily by adding its duration to
        // the duration of the first decay, and set sustain to 0!
        if( decay2 == 0x7F )
        {
            //Second decay is disabled, handle decay + sustain as usual!
            //volenv.sustain = ( SustainMinVolume - ( ( sustain * SustainMinVolume ) / MaxSignedInt8 ) );
            //Test Logarithmic attenuation
            double attn = sustain;
            if( sustain == 0 )
                volenv.sustain = SustainMinVolume;
            else
            {
                attn = 144.00 * log( 127.00 / attn ); //128 possible values
                volenv.sustain = static_cast<uint16_t>(  lround( attn ) );
            }
        }
        else
        {
            //Second decay is enabled, add duration of first decay, and set sustain volume to 0 !
            //Set sustain to the maximum attenuation to simulate the second decay parameter ramping volume down to 0!
            volenv.sustain = SustainMinVolume; //144.0 dB of attenuation
        }
#else
        //if( sustain != 0x7F )
        //{
            volenv.sustain = DseVolToSf2Attenuation( sustain ); //DSE_MaxAttenuation_cB - ( sustain * DSE_MaxAttenuation_cB / MaxSignedInt8 );
            //double attn = sustain;
            //if( sustain == 0 )
            //    volenv.sustain = SustainMinVolume;
            //else
            //{
            //    attn = static_cast<double>(SustainMinVolume) * log( 127.00 / attn ); //128 possible values
            //    volenv.sustain = static_cast<uint16_t>(  lround( attn ) );
            //}
            //volenv.sustain = ( DSE_MaxAttenuation_cB - ( ( sustain * DSE_MaxAttenuation_cB ) / MaxSignedInt8 ) );
        ///}

        if( decay != 0 )
        {
            if( utils::LibWide().isLogOn() )
                clog <<"Handling Decay..\n";

            //We use Decay
            if( decay!= 0x7F )
                volenv.decay = sf2::MSecsToTimecents( DSEEnveloppeDurationToMSec( decay, mul2 ) );
            else
                volenv.sustain = 0; //No decay
            //else
            //    volenv.decay = SHRT_MAX; //Infinite
        }
        else 
        {
            if( utils::LibWide().isLogOn() )
                clog<<"Decay was 0, falling back to decay 2..\n";

            if( decay2 != 0x7F )
            {
                //We want to fake the volume going down until complete silence, while the key is still held down 
                volenv.decay   = sf2::MSecsToTimecents( DSEEnveloppeDurationToMSec( decay2, mul2 ) );
                volenv.sustain = DSE_InfiniteAttenuation_cB;
            }
            else
                volenv.sustain = 0; //No decay at all
        }


#endif

        //Handle Release
        if( rel != 0 )
        {
            if( utils::LibWide().isLogOn() )
                clog <<"Handling Release..\n";

            //if( rel != 0x7F )
                volenv.release = sf2::MSecsToTimecents( DSEEnveloppeDurationToMSec( rel, mul2 ) );
            //else
            //    volenv.release = SHRT_MAX; //Infinite
        }
        else if( utils::LibWide().isLogOn() )
            clog <<"Skipping Release..\n";

        if( utils::LibWide().isLogOn() )
        {
            clog<<"\tRemaped to (del, atk, hold, dec, sus, rel) ( " 
                << static_cast<short>(volenv.delay)   <<", "
                << static_cast<short>(volenv.attack)  <<", "
                << static_cast<short>(volenv.hold)    <<", "
                << static_cast<short>(volenv.decay)   <<", "
                << static_cast<short>(volenv.sustain) <<", "
                << static_cast<short>(volenv.release) <<" )" <<endl;
        }

        return volenv;
    }

//
//TrackPlaybackState
//

    //#REMOVEME!!!
    //class TrackPlaybackState
    //{
    //public:

    //    TrackPlaybackState()
    //        :curpitch(0), curbpm(0), lastsilence(0), curvol(0), curexp(0), curpreset(0), curpan(0),lastpitchev(0)
    //    {}

    //    std::string printevent( const TrkEvent & ev )
    //    {
    //        stringstream outstr;
    //    
    //        //Print Delta Time
    //        if( ev.dt != 0 )
    //        {
    //            outstr <<dec <<static_cast<uint16_t>( DSE::TrkDelayCodeVals.at(ev.dt) )  <<" ticks-";
    //        }

    //        auto evinf = GetEventInfo( static_cast<eTrkEventCodes>(ev.evcode) );

    //        //Print Event Label
    //        if( evinf.first )
    //            outstr << evinf.second.evlbl << "-";
    //        else
    //            outstr << "INVALID-";

    //        //Print Parameters and Event Specifics
    //        if( evinf.second.nbreqparams == 1 )
    //        {
    //            if( evinf.second.evcodebeg == eTrkEventCodes::NoteOnBeg )
    //            {
    //                outstr << "( vel:" <<dec << static_cast<unsigned short>(ev.evcode) <<", TrkPitch:";
    //                uint8_t prevpitch = curpitch;
    //                uint8_t pitchop   = (NoteEvParam1PitchMask & ev.params.front());

    //                if( pitchop == static_cast<uint8_t>(eNotePitch::lower) ) 
    //                    curpitch -= 1;
    //                else if( pitchop == static_cast<uint8_t>(eNotePitch::higher) ) 
    //                    curpitch += 1;
    //                else if( pitchop == static_cast<uint8_t>(eNotePitch::reset) ) 
    //                    curpitch = lastpitchev;

    //                outstr <<dec <<static_cast<short>(prevpitch) <<"->" <<dec <<static_cast<short>(curpitch);
    //                
    //                outstr <<", note: " <<DSE::NoteNames.at( (ev.params.front() & NoteEvParam1NoteMask) );
    //                
    //                outstr <<dec <<static_cast<short>(curpitch) <<" )";
    //            }
    //            else if( evinf.second.evcodebeg == eTrkEventCodes::SetOctave )
    //            {
    //                outstr <<"( TrkPitch: ";
    //                uint8_t prevpitch = curpitch;
    //                lastpitchev = ev.params.front();
    //                curpitch    = ev.params.front();
    //                outstr <<dec <<static_cast<short>(prevpitch) <<"->" <<dec <<static_cast<short>(curpitch) <<" )";
    //            }
    //            else if( evinf.second.evcodebeg == eTrkEventCodes::SetExpress )
    //            {
    //                outstr <<"( TrkExp: ";
    //                int8_t prevexp = curexp;
    //                curexp = ev.params.front();
    //                outstr <<dec <<static_cast<short>(prevexp) <<"->" <<dec <<static_cast<short>(curexp) <<" )";
    //            }
    //            else if( evinf.second.evcodebeg == eTrkEventCodes::SetTrkVol )
    //            {
    //                outstr <<"( Vol: ";
    //                int8_t prevvol = curvol;
    //                curvol = ev.params.front();
    //                outstr <<dec <<static_cast<short>(prevvol) <<"->" <<dec <<static_cast<short>(curvol) <<" )";
    //            }
    //            else if( evinf.second.evcodebeg == eTrkEventCodes::SetTrkPan )
    //            {
    //                outstr <<"( pan: ";
    //                int8_t prevpan = curpan;
    //                curpan = ev.params.front();
    //                outstr <<dec <<static_cast<short>(prevpan) <<"->" <<dec <<static_cast<short>(curpan) <<" )";
    //            }
    //            else if( evinf.second.evcodebeg == eTrkEventCodes::SetPreset )
    //            {
    //                outstr <<"( Prgm: ";
    //                uint8_t prevpreset = curpreset;
    //                curpreset = ev.params.front();
    //                outstr <<dec <<static_cast<unsigned short>(prevpreset) <<"->" <<dec <<static_cast<unsigned short>(curpreset) <<" )";
    //            }
    //            else if( evinf.second.evcodebeg == eTrkEventCodes::SetTempo )
    //            {
    //                outstr <<"( tempo: ";
    //                int8_t prevbpm = curbpm;
    //                curbpm = ev.params.front();
    //                outstr <<dec <<static_cast<short>(prevbpm) <<"->" <<dec <<static_cast<short>(curbpm) <<" )";
    //            }
    //            else
    //                outstr << "( param1: 0x" <<hex <<static_cast<unsigned short>(ev.params.front()) <<" )" <<dec;
    //        }

    //        //Print Event with 2 params or a int16 as param
    //        if( ( evinf.second.nbreqparams == 2 ) )
    //        {
    //            if( evinf.second.evcodebeg == eTrkEventCodes::LongPause )
    //            {
    //                uint16_t duration = ( static_cast<uint16_t>(ev.params[1] << 8) | static_cast<uint16_t>(ev.params.front()) );
    //                outstr << "( duration: 0x" <<hex <<duration <<" )" <<dec;
    //            }
    //            else
    //                outstr << "( param1: 0x"  <<hex <<static_cast<unsigned short>(ev.params[0]) <<" , param2: 0x" <<static_cast<unsigned short>(ev.params[1]) <<" )" <<dec;
    //        }

    //        //if( evinf.second.evcodebeg == eTrkEventCodes::NoteOnBeg  )
    //        if( ev.params.size() > 2 )
    //        {
    //            //const uint8_t nbextraparams = (ev.params.front() & NoteEvParam1NbParamsMask) >> 6; // Get the two highest bits (1100 0000)

    //            outstr << "( ";

    //            for( unsigned int i = 0; i < ev.params.size(); ++i )
    //            {
    //                outstr << "param" <<dec <<i <<": 0x" <<hex <<static_cast<unsigned short>(ev.params[i]) <<dec;

    //                if( i != (ev.params.size()-1) )
    //                    outstr << ",";
    //                else
    //                    outstr <<" )";
    //            }
    //            
    //        }

    //        return outstr.str();
    //    }

    //private:
    //    //ittrk_t m_beg;
    //    //ittrk_t m_loop;
    //    //ittrk_t m_cur;
    //    //ittrk_t m_end;

    //    int8_t   curvol;
    //    int8_t   curpan;
    //    int8_t   curexp;
    //    uint8_t  curpitch;
    //    uint8_t  lastpitchev;
    //    uint8_t  curbpm;
    //    uint8_t  curpreset;
    //    uint16_t lastsilence;
    //};



//========================================================================================
//  BatchAudioLoader
//========================================================================================

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
        #FIXME: This thing doesn't work at all. It keeps assigning wrong presets..
    */
    BatchAudioLoader::mergedProgData BatchAudioLoader::PrepareMergedInstrumentTable()const
    {
        //A list of all the shared presets between files merged together. Where non-duplicates are stacked into the second dimension of the table
        std::vector< std::vector<ProgramInfo*> > merged( GetSizeLargestPrgiChunk() );
        
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
//                        auto founddup = find_if( merged[cntinst].begin(), 
//                                                 merged[cntinst].end(), 
//                                                 [&]( const ProgramInfo * inf )->bool
//                        { 
//                            if( inf != nullptr )
//                                return (inf->isSimilar( *(curinstlist[cntinst].get()) ) != ProgramInfo::eCompareRes::different );
//                            else
//                            {
//#ifdef _DEBUG
//                                assert(false);
//#endif
//                                cerr <<"Null instrument pointer encountered !\n";
//                                return false;
//                            }
//                        });

                        //if( founddup != merged[cntinst].end() )
                        //{
                        //    //If its a duplicate, set the existing copy of this instrument as what this SWDL+SMDL pair should use
                        //    smdlPresLocation[cntpair].insert( make_pair( cntinst, distance( merged[cntinst].begin(), founddup) ) );
                        //}
                        //else
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

        return std::move(mergedProgData{ std::move(merged), std::move(smdlPresLocation) });
    }

    /*
        DSEInstrumentToSf2Instrument
            Turns a DSE Preset's "Instrument" into a SF2 Instrument zone. And add it to the Soundfont!
            - dseinst       : The instruemnt to convert.
            - smplIdConvTbl : A map mapping the Sample IDs from the DSE swd, to their new ID within the Soundfont file!
            - inst          : The SF2 Instruemnt this dse sample/instrument shall be added to.
    */
    void DSEInstrumentToSf2InstrumentZone( const ProgramInfo::SplitEntry       & dseinst, 
                                           const std::map<uint16_t,size_t>     & smplIdConvTbl, 
                                           sf2::SoundFont                      & sf,
                                           sf2::Instrument                     & inst,
                                           const vector<bool>                  & loopedsmpls,
                                           uint16_t                              dsepresetid )
    {
        using namespace sf2;

        if( utils::LibWide().isLogOn() )
            clog <<"\t--- Split#" <<static_cast<unsigned short>(dseinst.id) <<" ---\n";

        //Make a global instrument zone
        //ZoneBag globalzone;

        //#TODO: Add stuff here if needed.

        //inst.AddZone( std::move(globalzone) );

        //Make a zone for this entry
        ZoneBag myzone;

        //  --- Set the generators ---

        //Key Range
        myzone.SetKeyRange( dseinst.lowkey, dseinst.hikey ); //Key range in first

        //Velocity Range
        myzone.SetVelRange( dseinst.unk14, dseinst.unk47 );

        //## Add new generators below ##

        //Loop Flag
        if( loopedsmpls[dseinst.smplid] )
            myzone.SetSmplMode( eSmplMode::loop );

        //Volume Envelope
        Envelope myenv = RemapDSEVolEnvToSF2( dseinst.atkvol, 
                                              dseinst.attack, 
                                              dseinst.hold, 
                                              dseinst.decay,
                                              dseinst.sustain,
                                              dseinst.decay2, 
                                              dseinst.release, 
                                              dseinst.rx,
                                              dseinst.mulatk,
                                              dseinst.mul2 );

        myzone.SetVolEnvelope( myenv );

        //Pitch Correction 
        //myzone.SetFineTune( dseinst.ctune );

        //Volume
        uint16_t attenuation = DseVolToSf2Attenuation(dseinst.smplvol);
        //myzone.SetInitAtt( attenuation );

        //Pan
        myzone.SetPan( DsePanToSf2Pan(dseinst.smplpan) );

        //#TEST: Try overriding rootkey for setting the pitch !
        myzone.SetRootKey( dseinst.rootkey );

        //Sample ID in last
        myzone.SetSampleId( smplIdConvTbl.at(dseinst.smplid) );

        //### In order to handle atkvol correctly, we'll make a copy of the Instrument, with a very short envelope!
        if( dseinst.atkvol != 0 && dseinst.attack != 0 )
        {
            ZoneBag atkvolzone( myzone );

            //Set attenuation to the atkvol value
            //double attn = dseinst.atkvol;
            //attn = 100.00 * log( 128.00 / attn ); //128 possible values
            //uint16_t logatkvol = static_cast<uint16_t>( lround( attn ) );
            //uint16_t logatkvol = ( DSE_MaxAttenuation_cB - ( ( dseinst.atkvol * DSE_MaxAttenuation_cB ) / CHAR_MAX ) );
            //atkvolzone.SetInitAtt( /*attenuation +*/ logatkvol );
            atkvolzone.SetInitAtt( DseVolToSf2Attenuation( dseinst.atkvol ) );

            //Set hold to the attack's duration
            Envelope atkvolenv;
            atkvolenv.hold = myenv.attack;

            //Leave everything else to default
            atkvolzone.SetVolEnvelope( atkvolenv );
            inst.AddZone( std::move(atkvolzone) );
        }

        //if( dseinst.decay == 0 /*&& dseinst.decay2 != 0*/ && dseinst.decay2 != 0x7F /*&& dseinst.sustain != 0x7F*/ )
        //{
        //    ZoneBag decay2zone( myzone );

        //    //Set attenuation to the sustain value
        //    //double attn = dseinst.sustain;
        //    //attn = 100.00 * log( 128.00 / attn ); //128 possible values
        //    //uint16_t logatkvol = static_cast<uint16_t>( lround( attn ) );
        //    //uint16_t logatkvol = ( DSE_MaxAttenuation_cB - ( ( dseinst.atkvol * DSE_MaxAttenuation_cB ) / CHAR_MAX ) );
        //    decay2zone.SetInitAtt( myenv.sustain );///*attenuation +*/ logatkvol );

        //    //Delay the envelope to the duration at which Decay2 would take over
        //    Envelope decay2env;

        //    int16_t delaysum = 0;

        //    if( dseinst.attack != 0x7F )
        //        delaysum = DSEEnveloppeDurationToMSec(dseinst.attack, dseinst.mul2);

        //    if( dseinst.hold != 0x7F )
        //        delaysum += DSEEnveloppeDurationToMSec(dseinst.hold, dseinst.mul2);

        //    if( delaysum != 0 )
        //        decay2env.delay = sf2::MSecsToTimecents( delaysum );

        //    //Set decay to the decay2's duration
        //    decay2env.decay = sf2::MSecsToTimecents( DSEEnveloppeDurationToMSec(dseinst.decay2, dseinst.mul2) );

        //    decay2env.sustain = DSE_MaxAttenuation_cB; //Set to highest attenuation


        //    decay2env.release = sf2::MSecsToTimecents( DSEEnveloppeDurationToMSec(dseinst.release, dseinst.mul2) );

        //    //Leave everything else to default
        //    decay2zone.SetVolEnvelope( decay2env );
        //    inst.AddZone( std::move(decay2zone) );
        //}

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
                               const ProgramInfo            & dsePres, 
                               const std::map<uint16_t,size_t> & smplIdConvTbl, 
                               sf2::SoundFont                  & sf,
                               uint16_t                        & instidcnt,
                               const vector<bool>              & loopedsmpls )
    {
        using namespace sf2;
        Preset pre(presname, dsePres.m_hdr.id, bankno );

        if( utils::LibWide().isLogOn() )
            clog <<"======================\nHandling " <<presname <<"\n======================\n";

        //#0 - Add a global zone for global preset settings
        {
            ZoneBag global;

            //#1 - Setup Generators
            //global.SetInitAtt( (0x7F - dsePres.m_hdr.insvol) ); //Use the difference between full volume and the current volume to attenuate the preset's volume
            //global.SetInitAtt( DseVolToSf2Attenuation(dsePres.m_hdr.insvol) );

            // Range of DSE Pan : 0x00 - 0x40 - 0x7F
            // (curpresinf.m_hdr.inspan - 64) * 7.8125f (64 fits 7.8125 times within 500, and 500 is the maximum pan value for soundfont generators)
            //double convpan = ((dsePres.m_hdr.inspan - 0x40) * 7.8125); // Remove 64(0x40) to bring the middle to 0.  
            global.SetPan( DsePanToSf2Pan(dsePres.m_hdr.inspan) );//lround(convpan) ); 
            
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
        for( uint16_t cntsmpl = 0; cntsmpl < dsePres.m_splitstbl.size(); ++cntsmpl )
            DSEInstrumentToSf2InstrumentZone( dsePres.m_splitstbl[cntsmpl], smplIdConvTbl, sf, myinst, loopedsmpls, dsePres.m_hdr.id );

        sf.AddInstrument( std::move(myinst) );
        instzone.SetInstrumentId(instidcnt);
        pre.AddZone( std::move(instzone) ); //Add the instrument zone after the global zone!
        ++instidcnt; //Increase the instrument count

        sf.AddPreset( std::move(pre) );
    }


    BatchAudioLoader::mergedProgData BatchAudioLoader::ExportSoundfont( const std::string & destf )const
    {
        using namespace sf2;
        //First build a master instrument list from all our pairs, with multiple entries per instruments
        mergedProgData        merged = std::move( PrepareMergedInstrumentTable() ); //MSVC is too dumb to trust with implicit move constructor calls..
        SoundFont             sf( m_master.metadata().fname ); 
        map<uint16_t,size_t>  swdsmplofftosf; //Make a map with as key a the sample id in the Wavi table, and as value the sample id in the sounfont!
        
        //
        if( merged.mergedpresets.size() > numeric_limits<uint16_t>::max() )
        {
            stringstream sstr;
            sstr << "The merged preset list has grown larger than " << numeric_limits<uint16_t>::max()
                 << ", the soundfont format doesn't support that much presets!\n";
            throw std::overflow_error(sstr.str());
        }

        //Prepare samples list
        shared_ptr<SampleBank> samples = m_master.smplbank().lock();
        vector<bool>           loopedsmpls( samples->NbSlots(), false ); //Keep track of which samples are looped

        //Check all our sample slots and prepare loading them in the soundfont
        for( size_t cntsmslot = 0; cntsmslot < samples->NbSlots(); ++cntsmslot )
        {
            //#TODO: What's below should have its own method..
            if( samples->IsInfoPresent(cntsmslot) && samples->IsDataPresent(cntsmslot) ) 
            {
                const auto & cursminf = *(samples->sampleInfo(cntsmslot));

                Sample::loadfun_t   loadfun;
                Sample::smplcount_t smpllen = 0;
                Sample::smplcount_t loopbeg = 0;
                Sample::smplcount_t loopend = 0;

                if( cursminf.smplfmt == static_cast<uint16_t>( WavInfo::eSmplFmt::ima_adpcm ) )
                {
                    loadfun = std::move( std::bind( ::audio::DecodeADPCM_NDS, std::ref( *samples->sample(cntsmslot) ), 1 ) );
                    smpllen = ::audio::ADPCMSzToPCM16Sz(samples->sample(cntsmslot)->size() );
                    loopbeg = (cursminf.loopbeg - SizeADPCMPreambleWords) * 8; //loopbeg is counted in int32, for APCM data, so multiply by 8 to get the loop beg as pcm16. Subtract one, because of the preamble.
                    loopend = smpllen;
                }
                else if( cursminf.smplfmt == static_cast<uint16_t>( WavInfo::eSmplFmt::pcm16 ) )
                {
                    loadfun = std::move( std::bind( &RawBytesToPCM16Vec, samples->sample(cntsmslot) ) );
                    smpllen = samples->sample(cntsmslot)->size() / 2;
                    loopbeg = cursminf.loopbeg * 2; //loopbeg is counted in int32, so multiply by 2 to get the loop beg as pcm16
                    loopend = smpllen;
                }
                else if( cursminf.smplfmt == static_cast<uint16_t>( WavInfo::eSmplFmt::pcm8 ) )
                {
                    loadfun = std::move( std::bind( &PCM8RawBytesToPCM16Vec, samples->sample(cntsmslot) ) );
                    smpllen = samples->sample(cntsmslot)->size() * 2; //PCM8 -> PCM16
                    loopbeg = cursminf.loopbeg * 4; //loopbeg is counted in int32, for PCM8 data, so multiply by 4 to get the loop beg as pcm16
                    loopend = smpllen;
                }
                else if( cursminf.smplfmt == static_cast<uint16_t>( WavInfo::eSmplFmt::psg ) )
                {
                    stringstream sstrerr;
                    sstrerr << "PSG instruments unsuported!";
                    throw std::runtime_error( sstrerr.str() );
                }
                else
                {
                    stringstream sstrerr;
                    sstrerr << "Unknown sample format (0x" <<hex <<uppercase <<cursminf.smplfmt <<nouppercase <<dec  <<") encountered !";
                    throw std::runtime_error( sstrerr.str() );
                }

#ifdef _DEBUG
                assert( (loopbeg > smpllen || loopend > smpllen) );
#endif

                Sample sm( std::move( loadfun ), smpllen );
                sm.SetName( "smpl#" + to_string(cntsmslot) );
                sm.SetSampleRate ( cursminf.smplrate );

                //Add the pitch offset to the root key 
                //int16_t rootkey = 72;//67 + DSESamplePitchToSemitone(cursminf.pitchoffst); //When set to 72, "C5", its ~5 semitones off, with all the other pitch correction on.
                //sm.SetOriginalKey( cursminf.rootkey ); //Default to this for now, but it will be overriden in the instrument anyways
                sm.SetSampleType ( Sample::eSmplTy::monoSample ); //#TODO: Mono samples only for now !

                //#TODO:Come up with a better loop detection logic !!!
                //if( loopbeg != 0 ) 
                if( cursminf.smplloop != 0 )
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
        try
        {
            sf.Write( destf );
        }
        catch( const overflow_error & e )
        {
            stringstream sstr;
            sstr <<"There are too many different parameters throughout the music files in the folder to extract them to a single soundfont file!\n"
                 << e.what() 
                 << "\n";
            throw runtime_error( sstr.str() );
        }

        return std::move(merged);
    }

    void BatchAudioLoader::ExportSoundfontAndMIDIs( const std::string & destdir )const
    {
        //Export the soundfont first
        Poco::Path outsoundfont(destdir);
        outsoundfont.append( outsoundfont.getBaseName() + ".sf2").makeFile();
        cerr<<"<*>- Currently exporting main bank to " <<outsoundfont.toString() <<"\n";
        mergedProgData merged = std::move( ExportSoundfont( outsoundfont.toString() ) );

        //Then the MIDIs
        for( size_t i = 0; i < m_pairs.size(); ++i )
        {
            Poco::Path fpath(destdir);
            fpath.append( to_string(i) + "_" + m_pairs[i].first.metadata().fname);
            fpath.makeFile();
            fpath.setExtension("mid");

            cerr<<"<*>- Currently exporting smd to " <<fpath.toString() <<"\n";
            DSE::SequenceToMidi( fpath.toString(), 
                                 m_pairs[i].first, 
                                 merged.filetopreset[i], 
                                 DSE::eMIDIFormat::SingleTrack,
                                 DSE::eMIDIMode::GS );  //This will disable the drum channel, since we don't need it at all!
        }
    }

    //struct 
    //{
    //};

    //void BatchAudioLoader::ExportSoundfontAndMIDIs_new( const std::string & destdir )const
    //{
    //    Poco::Path outsoundfont(destdir);
    //    outsoundfont.append( outsoundfont.getBaseName() + ".sf2").makeFile();
    //    cerr<<"<*>- Currently exporting main bank to " <<outsoundfont.toString() <<"\n";
    //    vector<DSE::PresetBank&> swdls;


    //    //We want to export smdl and swdl at the same time
    //    for( size_t i = 0; i < m_pairs.size(); ++i )
    //    {
    //        uint32_t tpqn  =  m_pairs[i].first.metadata().tpqn;
    //        m_pairs[i].second;
    //       

    //        //We must get the tempo value from the SMDL in order to compute envelope durations and etc..

    //        //Assign the banks and preset numbers, and save them in a table for later use
    //    }

    //    //Using the table, build the midis.

    //}


    void BatchAudioLoader::ExportSoundfontAsGM( const std::string                               & destf, 
                                                const std::map< std::string, std::vector<int> > & dsetogm )const
    {
    }


//===========================================================================================
//  Functions
//===========================================================================================
    
    std::pair<DSE::PresetBank, std::vector<std::pair<DSE::MusicSequence,DSE::PresetBank>> > LoadBankAndPairs( const std::string & bank, const std::string & smdroot, const std::string & swdroot )
    {
        DSE::PresetBank                                       mbank  = DSE::ParseSWDL( bank );
        vector<std::pair<DSE::MusicSequence,DSE::PresetBank>> seqpairs;

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

    std::pair<DSE::MusicSequence,DSE::PresetBank> LoadSmdSwdPair( const std::string & smd, const std::string & swd )
    {
        DSE::PresetBank    bank = DSE::ParseSWDL( swd );
        DSE::MusicSequence seq  = DSE::ParseSMDL( smd );
        return std::make_pair( std::move(seq), std::move(bank) );
    }

    DSE::PresetBank LoadSwdBank( const std::string & file )
    {
        return DSE::ParseSWDL( file );
    }
        
    DSE::MusicSequence LoadSequence( const std::string & file )
    {
        return DSE::ParseSMDL( file );
    }

    void ExportPresetBank( const std::string & directory, const DSE::PresetBank & bnk )
    {
        auto smplptr = bnk.smplbank().lock();
        auto instptr = bnk.instbank().lock();

        if( smplptr != nullptr )
        {
        }
        else
        {
        }

        if( instptr != nullptr )
        {
        }
        else
        {
        }
    }


};};