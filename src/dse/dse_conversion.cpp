#include "dse_conversion.hpp"
#include <dse/dse_common.hpp>
#include <dse/dse_sequence.hpp>
#include <dse/dse_interpreter.hpp>
#include <dse/dse_containers.hpp>
#include <utils/library_wide.hpp>
#include <utils/audio_utilities.hpp>
#include <utils/poco_wrapper.hpp>

#include <ppmdu/fmts/sedl.hpp>
#include <ppmdu/fmts/smdl.hpp>
#include <ppmdu/fmts/swdl.hpp>
#include <dse/bgm_container.hpp>
#include <dse/bgm_blob.hpp>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <unordered_map>

#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>

#include <ext_fmts/adpcm.hpp>
#include <ext_fmts/sf2.hpp>
#include <ext_fmts/wav_io.hpp>

using namespace std;

#if 0
/*
    Prototype implementation of a more intuitive and less complicated method of loading and handling DSE audio.
*/
namespace DSE
{
    /*
    */
    class AudioDataSet
    {
    public:
        AudioDataSet();

        // 
        void LoadSMDL       ( const std::string & fpath );
        void LoadSWDL       ( const std::string & fpath );
        void LoadMatchedPair( const std::string & smdlpath, const std::string & smdlpath );

        //
        void BatchLoadSMDL( const std::string & dirpath );
        void BatchLoadSWDL( const std::string & dirpath );

        //
        void BatchSMDLFronBlob( const std::string & fpath, uint32_t startoffset = 0, uint32_t endoffset = 0 );
        void BatchSWDLFronBlob( const std::string & fpath, uint32_t startoffset = 0, uint32_t endoffset = 0 );

        //HasMainBank
        inline bool HasMainBank()const { return !m_mainbankname.empty(); }

    private:

        //These do some additional processing after parsing is done!
        void ProcessSMDL( std::istreambuf_iterator<char> itbeg, std::istreambuf_iterator<char> itpastend );
        void ProcessSWDL( std::istreambuf_iterator<char> itbeg, std::istreambuf_iterator<char> itpastend );

        //If a mainbank SWDL is located, 
        void InsertSequence( DSE::MusicSequence && seq, const std::string & name );
        void InsertBank    ( DSE::PresetBank    && bnk, const std::string & name );

        //Determine if a SWDL is a main bank (aka, contains sample data, but no presets!)
        void CheckAndHandleMainBank( DSE::PresetBank       & bnk );
        bool IsBankMainBank        ( const DSE::PresetBank & bnk )const;

    private:
        std::multimap<std::string, DSE::MusicSequence> m_sequences;    // Contains all the parsed DSE sequences(Content of SMDL)
        std::multimap<std::string, DSE::PresetBank>    m_banks;        // Contains all the parsed DSE PresetBanks(content of SWDL)
        std::string                                    m_mainbankname; // Used to access the main bank after it was loaded. Empty if no main bank.
    };

    /*
    */
    class AudioDataSetConverter
    {
    public:
        AudioDataSetConverter( const AudioDataSet & dsedata );



    };

};

#endif

namespace DSE
{
    static const uint16_t DSE_InfiniteAttenuation_cB = 1440;//1440; //sf2::SF_GenLimitsInitAttenuation.max_;
    static const uint16_t DSE_LowestAttenuation_cB   =  250;//200; //This essentially defines the dynamic range of the tracks. It sets how low the most quiet sound can go. Which is around -20 dB
    static const uint16_t DSE_SustainFactor_msec     = 3000; //An extra time value to add to the decay when decay1 + decay2 are combined, and the sustain is non-zero


    //A multiplier to use to convert from DSE Pan to Soundfont Pan
    static const double BytePanToSoundfontPanMulti = static_cast<double>(sf2::SF_GenLimitsPan.max_ + (-sf2::SF_GenLimitsPan.min_)) / static_cast<double>(DSE_LimitsPan.max_);

    //A multiplier to use to convert from DSE volume to soundfont attenuation.
    //static const double ByteVolToSounfontAttnMulti = static_cast<double>(DSE_LowestAttenuation_cB) / static_cast<double>(DSE_LimitsVol.max_);

    /*
        The size of the ADPCM preamble in int32, the unit the NDS uses to store the loop positions
        Mainly here to make things more readable.
    */
    static const int SizeADPCMPreambleWords = ::audio::IMA_ADPCM_PreambleLen / sizeof(int32_t);



//====================================================================================================
// Class
//====================================================================================================

    class PresetAllocStrategyTrait_DefaultSF2{};   //Each file's number is a bank, keeps presets the same value they were in the SWDL. Once the bank value reaches 127, it resets to 0.
    class PresetAllocStrategyTrait_SingleSF2 {};   //Reorganizes each presets in its own slot in a sf2 file, in one of the 127 possible presets slots, over all 127 banks.

    /*****************************************************************************************************************
        BankAndProgramIDAssigner
            A class that handles the various preset allocation strategies. When converting from SWD to SF2 and etc..

            This is the default strategy for SF2 files. Just assign presets with their original ids. 
            Each pair has its own bank. Past 127 pairs, the bankid resets to 0.
            Used for splitting presets into several SF2s.
    ******************************************************************************************************************/
    template<class _Strategy = PresetAllocStrategyTrait_DefaultSF2>
        class PresetAllocStrategy
    {
    public:

        PresetAllocStrategy( const BatchAudioLoader & loader )
            :m_loader(loader)
        {}

        bool ComputeBankAndInstID( size_t cntpair, size_t cntinst, int8_t & bankid, int8_t & instid )
        {
            //Default strategy, assign presets as-is one bank per file. And loop the bank id when we reach bank 127
            if( bankid < CHAR_MAX )
                bankid = static_cast<int8_t>(cntpair);
            else
                bankid = static_cast<int8_t>( cntpair % CHAR_MAX );

            instid = static_cast<int8_t>(cntinst);
            return true;
        }

    private:
        const BatchAudioLoader & m_loader;
    };

    /*****************************************************************************************************************
        BankAndProgramIDAssigner
            A class that handles the various preset allocation strategies. When converting from SWD to SF2 and etc..

            This strategy is for using as efficiently as possible a single SF2, filling every single presets and banks.
    *****************************************************************************************************************/
    template<>
        class PresetAllocStrategy< PresetAllocStrategyTrait_SingleSF2 >
    {
    public:
        PresetAllocStrategy( const BatchAudioLoader & loader )
            :m_loader(loader), m_curbank(0), m_curpreset(0)
        {}

        /*
            Will return false when no more room is available
            The 2 counters are ignored in this version.
        */
        bool ComputeBankAndInstID( size_t cntpair, size_t cntinst, int8_t & bankid, int8_t & presetid )
        {
            if( m_curbank >= CHAR_MAX )
                return false;

            if( m_curpreset < CHAR_MAX )
            {
                ++m_curpreset;
            }
            else
            {
                ++m_curbank;
                m_curpreset = 0;
            }

            bankid   = m_curbank;
            presetid = m_curpreset;

            clog << "Assinged for( " <<cntpair <<", " <<cntinst 
                 << " ) Preset " <<static_cast<unsigned short>(presetid) <<", Bank " 
                 << static_cast<unsigned short>(bankid) <<"\n";

            return true;
        }

    private:
        const BatchAudioLoader & m_loader;
        uint8_t                  m_curbank;
        uint8_t                  m_curpreset;
    };

//===========================================================================================
//  Utility Functions
//===========================================================================================

    /*
        DsePanToSf2Pan
            Convert a pan value from a DSE file(0 to 127) to a SF2 pan( -500 to 500 ).
    */
    inline int16_t DsePanToSf2Pan( int8_t dsepan )
    {
        dsepan = abs(dsepan);

        if( dsepan == DSE_LimitsPan.max_ )
            return sf2::SF_GenLimitsPan.max_;
        else if( dsepan == DSE_LimitsPan.min_ )
            return sf2::SF_GenLimitsPan.min_;
        else
        {
            return static_cast<int16_t>(lround( ( dsepan - DSE_LimitsPan.mid_ ) * BytePanToSoundfontPanMulti ));
        }
    }

    //!#FIXME: MOST LIKELY INNACURATE !
    inline uint16_t DseVolToSf2Attenuation( int8_t dsevol )
    {
        dsevol = abs(dsevol);

        //Because of the rounding, we need to make sure our limits match the SF limits
        if( dsevol ==  DSE_LimitsVol.min_ )
            return DSE_InfiniteAttenuation_cB;
        else
        {
            return DSE_LowestAttenuation_cB - (dsevol * DSE_LowestAttenuation_cB) / 127; //The NDS's volume curve is linear, but soundfonts use a logarithmic volume curve.. >_<
        }
    }

    inline int16_t DSE_LFOFrequencyToCents( int16_t freqhz )
    {
        static const double ReferenceFreq = 8.176; // 0 cents is 8.176 Hz
        return static_cast<int16_t>( lround( 1200.00 * log2( static_cast<double>(freqhz) / ReferenceFreq ) ) );
    }

    inline int16_t DSE_LFODepthToCents( int16_t depth )
    {
        return (depth/12) * -1; /*static_cast<int16_t>( lround(depth * 12000.0 / 10000.0 ) )*/;

        //return static_cast<int16_t>( lround( 1200.00 * log2( static_cast<double>(depth)  ) ) );
    }

    /*
        RawBytesToPCM16Vec
            Take a vector of 16 bits signed pcm samples as raw bytes, and put them into a vector of
            signed 16 bits integers!
    */
    std::vector<int16_t> RawBytesToPCM16Vec( const std::vector<uint8_t> * praw )
    {
        return std::move( utils::RawPCM16Parser<int16_t>( *praw ) );
    }

    /*
        PCM8RawBytesToPCM16Vec
            Take a vector of raw pcm8 samples, and put them into a pcm16 vector!
    */
    std::vector<int16_t> PCM8RawBytesToPCM16Vec( const std::vector<uint8_t> * praw )
    {
        std::vector<int16_t> out;
        auto                 itread = praw->begin();
        auto                 itend  = praw->end();
        out.reserve(praw->size()*2);

        for(; itread != itend; ++itread )
            out.push_back( ( ( static_cast<int16_t>(*itread)  ^ 0x80) * std::numeric_limits<int16_t>::max() ) / std::numeric_limits<uint8_t>::max() ); //Convert from excess-k coding to 2's complement. And scale up to pcm16.

        return std::move(out);//std::move( utils::RawPCM8Parser<int16_t>( *praw ) );
    }


    /*********************************************************************************
        ScaleEnvelopeDuration
            Obtain from the duration table the duration of each envelope params, 
            and scale them.
            Scale the various parts of the volume envelope by the specified factors.
    *********************************************************************************/
    inline DSEEnvelope IntepretAndScaleEnvelopeDuration( const DSEEnvelope   & srcenv, 
                                                         double                ovrllscale = 1.0, 
                                                         double                atkf       = 1.0, 
                                                         double                holdf      = 1.0,
                                                         double                decayf     = 1.0,
                                                         double                releasef   = 1.0 )
    {
        DSEEnvelope outenv(srcenv);
        outenv.attack  = static_cast<DSEEnvelope::timeprop_t>( lround( DSEEnveloppeDurationToMSec( static_cast<int8_t>(outenv.attack),  outenv.envmulti) *     atkf * ovrllscale ) );
        outenv.hold    = static_cast<DSEEnvelope::timeprop_t>( lround( DSEEnveloppeDurationToMSec( static_cast<int8_t>(outenv.hold),    outenv.envmulti) *    holdf * ovrllscale ) );
        outenv.decay   = static_cast<DSEEnvelope::timeprop_t>( lround( DSEEnveloppeDurationToMSec( static_cast<int8_t>(outenv.decay),   outenv.envmulti) *   decayf * ovrllscale ) );
        outenv.release = static_cast<DSEEnvelope::timeprop_t>( lround( DSEEnveloppeDurationToMSec( static_cast<int8_t>(outenv.release), outenv.envmulti) * releasef * ovrllscale ) );
        return outenv;
    }

    inline DSEEnvelope IntepretEnvelopeDuration( const DSEEnvelope & srcenv )
    {
        DSEEnvelope outenv(srcenv);
        outenv.attack  = static_cast<DSEEnvelope::timeprop_t>( DSEEnveloppeDurationToMSec( static_cast<int8_t>(outenv.attack),  outenv.envmulti) );
        outenv.hold    = static_cast<DSEEnvelope::timeprop_t>( DSEEnveloppeDurationToMSec( static_cast<int8_t>(outenv.hold),    outenv.envmulti) );
        outenv.decay   = static_cast<DSEEnvelope::timeprop_t>( DSEEnveloppeDurationToMSec( static_cast<int8_t>(outenv.decay),   outenv.envmulti) );
        outenv.release = static_cast<DSEEnvelope::timeprop_t>( DSEEnveloppeDurationToMSec( static_cast<int8_t>(outenv.release), outenv.envmulti) );
        return outenv;
    }

    /*
        Convert the parameters of a DSE envelope to SF2
    */
    sf2::Envelope RemapDSEVolEnvToSF2( DSEEnvelope origenv )
    {
        sf2::Envelope volenv;
       
        //#TODO: implement scaling by users !!!
        DSEEnvelope interpenv = IntepretEnvelopeDuration(origenv);//IntepretAndScaleEnvelopeDuration(origenv, 1.0, 1.0, 1.0, 1.0, 1.0 );

        if( utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn() )
        {
            clog<<"\tRemaping DSE( " 
                <<"atkvol : " << static_cast<short>(origenv.atkvol)       <<", "
                <<"atk    : " << static_cast<short>(origenv.attack)       <<", "
                <<"dec    : " << static_cast<short>(origenv.decay)        <<", "
                <<"sus    : " << static_cast<short>(origenv.sustain)      <<", "
                <<"hold   : " << static_cast<short>(origenv.hold)         <<", "
                <<"dec2   : " << static_cast<short>(origenv.decay2)       <<", "
                <<"rel    : " << static_cast<short>(origenv.release)      <<" )\n";
        }

        //Handle Attack
        if( origenv.attack != 0 )
        {
            if( utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn() )
                clog<<"Handling Attack..\n";

            volenv.attack = sf2::MSecsToTimecents( interpenv.attack );
        }
        else if( utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn() )
            clog<<"Skipping Attack..\n";

        //Handle Hold
        if( origenv.hold != 0 )
        {
            if( utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn() )
                clog<<"Handling Hold..\n";

            volenv.hold = sf2::MSecsToTimecents( interpenv.hold );
        }
        else if( utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn() )
            clog<<"Skipping Hold..\n";

        //Handle Sustain
        volenv.sustain = DseVolToSf2Attenuation( interpenv.sustain ); 

        //uint16_t sustainfactor = (interpenv.sustain * DSE_SustainFactor_msec) /127; //Add a bit more time, because soundfont handles volume exponentially, and thus, the volume sinks to 0 really quickly!


        //Handle Decay
        if( origenv.decay != 0 && origenv.decay2 != 0 && origenv.decay2 != 0x7F ) 
        {
            if( utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn() )
                clog <<"We got combined decays! decay1-2 : " 
                     <<static_cast<unsigned short>(interpenv.decay) <<" + " 
                     <<static_cast<unsigned short>(interpenv.decay2) << " = " 
                     <<static_cast<unsigned short>(interpenv.decay + interpenv.decay2) <<" !\n";

            //If decay is set to infinite, we just ignore it!
            if( origenv.decay == 0x7F )
                volenv.decay   = sf2::MSecsToTimecents( interpenv.decay2 ) /*+ sustainfactor*/;
            else if( origenv.sustain == 0 ) //The sustain check is to avoid the case where the first decay phase already should have brought the volume to 0 before the decay2 phase would do anything. 
                volenv.decay   = sf2::MSecsToTimecents( interpenv.decay ) /*+ sustainfactor*/;
            else
            {
                volenv.decay = sf2::MSecsToTimecents( interpenv.decay + interpenv.decay2 ) /*+ sustainfactor*/; //Add an extra factor based on the sustain value
            }
            volenv.sustain = DSE_InfiniteAttenuation_cB;
        }
        else if( origenv.decay != 0 )
        {
            if( utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn() )
                clog <<"Handling single decay..\n";

            //We use Decay
            //if( decay != 0x7F )
                volenv.decay = sf2::MSecsToTimecents( interpenv.decay );
            //else
            //    volenv.sustain = 0; //No decay at all
        }
        else 
        {
            if( utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn() )
                clog<<"Decay was 0, falling back to decay 2..\n";

            if( origenv.decay2 != 0x7F )
            {
                //We want to fake the volume going down until complete silence, while the key is still held down 
                volenv.decay   = sf2::MSecsToTimecents( interpenv.decay2 );
                volenv.sustain = DSE_InfiniteAttenuation_cB;
            }
            else
                volenv.sustain = 0; //No decay at all
        }

        //Handle Release
        if( origenv.release != 0 )
        {
            if( utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn() )
                clog <<"Handling Release..\n";

            //if( rel != 0x7F )
                volenv.release = sf2::MSecsToTimecents( interpenv.release );
            //else
            //    volenv.release = SHRT_MAX; //Infinite
        }
        else if( utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn() )
            clog <<"Skipping Release..\n";

        if( utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn() )
        {
            clog<<"\tRemaped to (del, atk, hold, dec, sus, rel) ( " 
                << static_cast<short>(volenv.delay)   <<", "
                << static_cast<short>(volenv.attack)  <<", "
                << static_cast<short>(volenv.hold)    <<", "
                << static_cast<short>(volenv.decay)   <<", "
                << static_cast<short>(volenv.sustain) <<", "
                << static_cast<short>(volenv.release) <<" )" <<endl;

            //Diagnostic            
            if( origenv.attack > 0 && origenv.attack < 0x7F && 
               (volenv.attack == sf2::SF_GenLimitsVolEnvAttack.max_ || volenv.attack == sf2::SF_GenLimitsVolEnvAttack.min_) )
            {
                //Something fishy is going on !
                clog << "\tThe attack value " <<static_cast<short>(origenv.attack) <<" shouldn't result in the SF2 value " <<volenv.attack  <<" !\n";
                assert(false);
            }

            if( origenv.hold > 0 && origenv.hold < 0x7F && 
               (volenv.hold == sf2::SF_GenLimitsVolEnvHold.max_ || volenv.hold == sf2::SF_GenLimitsVolEnvHold.min_) )
            {
                //Something fishy is going on !
                clog << "\tThe hold value " <<static_cast<short>(origenv.hold) <<" shouldn't result in the SF2 value " <<volenv.hold  <<" !\n";
                assert(false);
            }

            if( origenv.decay > 0 && origenv.decay < 0x7F && 
               (volenv.decay == sf2::SF_GenLimitsVolEnvDecay.max_ || volenv.decay == sf2::SF_GenLimitsVolEnvDecay.min_) )
            {
                //Something fishy is going on !
                clog << "\tThe decay value " <<static_cast<short>(origenv.decay) <<" shouldn't result in the SF2 value " <<volenv.decay  <<" !\n";
                assert(false);
            }

            //if( sustain > 0 && sustain < 0x7F && 
            //   (volenv.sustain == sf2::SF_GenLimitsVolEnvSustain.max_ || volenv.sustain == sf2::SF_GenLimitsVolEnvSustain.min_) )
            //{
            //    //Something fishy is going on !
            //    clog << "\tThe sustain value " <<static_cast<short>(sustain) <<" shouldn't result in the SF2 value " <<volenv.sustain  <<" !\n";
            //    assert(false);
            //}

            if( origenv.release > 0 && origenv.release < 0x7F && 
               (volenv.release == sf2::SF_GenLimitsVolEnvRelease.max_ || volenv.release == sf2::SF_GenLimitsVolEnvRelease.min_) )
            {
                //Something fishy is going on !
                clog << "\tThe release value " <<static_cast<short>(origenv.release) <<" shouldn't result in the SF2 value " <<volenv.release  <<" !\n";
                assert(false);
            }
        }

        return volenv;
    }

    eDSESmplFmt ConvertDSESample( int16_t                                smplfmt, 
                                  size_t                                 origloopbeg,
                                  const std::vector<uint8_t>           & in_smpl,
                                  DSESampleConvertionInfo              & out_cvinfo,
                                  std::vector<int16_t>                 & out_smpl )
    {
        if( smplfmt == static_cast<uint16_t>(eDSESmplFmt::ima_adpcm) )
        {
            out_smpl = move(::audio::DecodeADPCM_NDS( in_smpl ) );
            out_cvinfo.loopbeg_ = (origloopbeg - SizeADPCMPreambleWords) * 8; //loopbeg is counted in int32, for APCM data, so multiply by 8 to get the loop beg as pcm16. Subtract one, because of the preamble.
            out_cvinfo.loopend_ = out_smpl.size(); /*::audio::ADPCMSzToPCM16Sz(in_smpl.size() );*/
            return eDSESmplFmt::ima_adpcm;
        }
        else if( smplfmt == static_cast<uint16_t>(eDSESmplFmt::pcm8) )
        {
            out_smpl = move( PCM8RawBytesToPCM16Vec(&in_smpl) );
            out_cvinfo.loopbeg_ = origloopbeg    * 4; //loopbeg is counted in int32, for PCM8 data, so multiply by 4 to get the loop beg as pcm16
            out_cvinfo.loopend_ = out_smpl.size(); //PCM8 -> PCM16
            return eDSESmplFmt::pcm8;
        }
        else if( smplfmt == static_cast<uint16_t>(eDSESmplFmt::pcm16) )
        {
            out_smpl = move( RawBytesToPCM16Vec(&in_smpl) );
            //out_smpl = vector<int16_t>( (in_smpl.size() / 2), 0 );
            out_cvinfo.loopbeg_ = (origloopbeg * 2); //loopbeg is counted in int32, so multiply by 2 to get the loop beg as pcm16
            out_cvinfo.loopend_ = out_smpl.size();

            //if( (out_cvinfo.loopbeg_ - 2) > 0 )
            //    out_cvinfo.loopbeg_ -= 2;

            //if( (out_cvinfo.loopbeg_ - 1) > 0 && out_cvinfo.loopbeg_ % 2 != 0 )
            //    out_cvinfo.loopbeg_ -= 1;
            //else if( (out_cvinfo.loopbeg_ - 3) > 0 && out_cvinfo.loopbeg_ % 2 == 0 )
            //    out_cvinfo.loopbeg_ -= 3;

            return eDSESmplFmt::pcm16;
        }
        else if( smplfmt == static_cast<uint16_t>(eDSESmplFmt::psg) )
        {
            clog << "<!>- Warning: ConvertDSESample(): Samplefmt PSG ???\n";
#ifdef _DEBUG
            assert(false);
#endif
            return eDSESmplFmt::psg;
        }
        else
            return eDSESmplFmt::invalid;
    }


    //eDSESmplFmt ConvertAndLoopDSESample( int16_t                                smplfmt, 
    //                                     size_t                                 origloopbeg,
    //                                     size_t                                 origlooplen,
    //                                     size_t                                 nbloops,
    //                                     const std::vector<uint8_t>           & in_smpl,
    //                                     DSESampleConvertionInfo              & out_cvinfo,
    //                                     std::vector<int16_t>                 & out_smpl )
    //{
    //    if( smplfmt == static_cast<uint16_t>(eDSESmplFmt::ima_adpcm) )
    //    {
    //        auto postconv = move( ::audio::LoopAndConvertADPCM_NDS( in_smpl, origloopbeg, origlooplen, nbloops, 1 ) );
    //        
    //        out_cvinfo.loopbeg_ = postconv.second;
    //        out_cvinfo.loopend_ = postconv.first.front().size();
    //        out_smpl = move(postconv.first.front());
    //        //out_cvinfo.loopbeg_ = (origloopbeg - SizeADPCMPreambleWords) * 8; //loopbeg is counted in int32, for APCM data, so multiply by 8 to get the loop beg as pcm16. Subtract one, because of the preamble.
    //        //out_cvinfo.loopend_ = ::audio::ADPCMSzToPCM16Sz(in_smpl.size() );
    //        return eDSESmplFmt::ima_adpcm;
    //    }
    //    else if( smplfmt == static_cast<uint16_t>(eDSESmplFmt::pcm8) )
    //    {
    //        out_smpl = move( PCM8RawBytesToPCM16Vec(&in_smpl) );
    //        out_cvinfo.loopbeg_ = origloopbeg    * 4; //loopbeg is counted in int32, for PCM8 data, so multiply by 4 to get the loop beg as pcm16
    //        out_cvinfo.loopend_ = out_smpl.size(); //in_smpl.size() * 2; //PCM8 -> PCM16
    //        return eDSESmplFmt::pcm8;
    //    }
    //    else if( smplfmt == static_cast<uint16_t>(eDSESmplFmt::pcm16) )
    //    {
    //        out_smpl = move( RawBytesToPCM16Vec(&in_smpl) );
    //        cerr<< "\n#FIXME ConvertAndLoopDSESample(): PCM16 sample loop beginning kept as-is: " <<origloopbeg <<" \n";
    //        //#FIXME: changed this just to test if it improves pcm16 looping.
    //        out_cvinfo.loopbeg_ = origloopbeg;//    * 2; //loopbeg is counted in int32, so multiply by 2 to get the loop beg as pcm16
    //        out_cvinfo.loopend_ = out_smpl.size();//in_smpl.size() / 2;
    //        return eDSESmplFmt::pcm16;
    //    }
    //    else if( smplfmt == static_cast<uint16_t>(eDSESmplFmt::psg) )
    //    {
    //        return eDSESmplFmt::psg;
    //    }
    //    else
    //        return eDSESmplFmt::invalid;
    //}

//========================================================================================
//  BatchAudioLoader
//========================================================================================

    BatchAudioLoader::BatchAudioLoader( bool singleSF2, bool lfofxenabled )
        : m_bSingleSF2(singleSF2),m_lfoeffects(lfofxenabled)
    {}


    void BatchAudioLoader::LoadMasterBank( const std::string & mbank )
    {
        m_mbankpath = mbank;
        m_master = move( DSE::ParseSWDL( m_mbankpath ) );
    }

    void BatchAudioLoader::LoadSmdSwdPair( const std::string & smd, const std::string & swd )
    {
        DSE::PresetBank    bank( move( DSE::ParseSWDL( swd ) ) );
        DSE::MusicSequence seq( move( DSE::ParseSMDL( smd ) ) );

        //Tag our files with their original file name, for cvinfo lookups to work!
        seq.metadata().origfname  = Poco::Path(smd).getBaseName();
        bank.metadata().origfname = Poco::Path(swd).getBaseName();

        m_pairs.push_back( move( std::make_pair( std::move(seq), std::move(bank) ) ) );
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

    template<class _Strategy>
        void MakeAPresetBankDBForAPair( size_t                           cntpair, 
                                        shared_ptr<ProgramBank>          ptrprginf, 
                                        SMDLPresetConversionInfo       & target,
                                        PresetAllocStrategy<_Strategy> & mystrat )
    {
        const auto & curinstlist = ptrprginf->PrgmInfo();

        if( utils::LibWide().isLogOn() )
            clog << "=== SWDL #" << cntpair <<" ===\n";

        for( size_t cntinst = 0; cntinst < curinstlist.size();  ++cntinst ) //Test all the individual instruments and add them to their slot
        {
            if( curinstlist[cntinst] != nullptr )
            {
                int8_t bankid = 0;
                int8_t presid = 0;

                if( mystrat.ComputeBankAndInstID( cntpair, cntinst, bankid, presid ) )
                    target.AddPresetConvInfo( static_cast<dsepresetid_t>(cntinst), SMDLPresetConversionInfo::PresetConvData( presid, bankid ) ); //set pair nb as bank id
                else
                    throw runtime_error("MakeAPresetBankDBForAPair() : SF2 file is full!!");
            }
        }
    }

    bool BatchAudioLoader::IsMasterBankLoaded()const
    {
        return (m_master.smplbank().lock() != nullptr );
    }

    /*
        AllocPresetSingleSF2
            Assign preset+bank slots sequentially (fill the presets 1-127 of bank 0 first, then 1-127 of bank 1, and so on! )
    */
    void BatchAudioLoader::AllocPresetSingleSF2( std::vector<DSE::SMDLPresetConversionInfo> & toalloc )const
    {
        PresetAllocStrategy<PresetAllocStrategyTrait_SingleSF2> allocator(*this);

        for( size_t cntpair = 0; cntpair < m_pairs.size(); ++cntpair ) //Iterate through all SWDs
        {
            auto ptrinstinf = m_pairs[cntpair].second.prgmbank().lock(); //Get the program list

            if( ptrinstinf != nullptr )
            {
                 MakeAPresetBankDBForAPair( cntpair, move(ptrinstinf), toalloc[cntpair], allocator );
            }
        }
    }

    /*
        AllocPresetDefault
            Keep the same preset ID as in the SWDL files, but each SWDL file has its own bank. This results in a lot of
            unused slots in each banks.
    */
    void BatchAudioLoader::AllocPresetDefault  ( std::vector<DSE::SMDLPresetConversionInfo> & toalloc )const
    {
        PresetAllocStrategy<> allocator(*this);

        for( size_t cntpair = 0; cntpair < m_pairs.size(); ++cntpair ) //Iterate through all SWDs
        {
            auto ptrinstinf = m_pairs[cntpair].second.prgmbank().lock(); //Get the program list

            if( ptrinstinf != nullptr )
            {
                 MakeAPresetBankDBForAPair( cntpair, move(ptrinstinf), toalloc[cntpair], allocator );
            }
        }
    }

    vector<SMDLPresetConversionInfo> BatchAudioLoader::BuildPresetConversionDB()const
    {
        vector<SMDLPresetConversionInfo> result(m_pairs.size());

        if( m_bSingleSF2 )
            AllocPresetSingleSF2(result);
        else
            AllocPresetDefault(result);

        return move(result);
    }

    /*
        DSEInstrumentToSf2Instrument
            Turns a DSE Preset's "Instrument" into a SF2 Instrument zone. And add it to the Soundfont!
            - dseinst       : The instruemnt to convert.
            - smplIdConvTbl : A map mapping the Sample IDs from the DSE swd, to their new ID within the Soundfont file!
            - inst          : The SF2 Instruemnt this dse sample/instrument shall be added to.
    */
    void DSESplitToSf2InstrumentZone( const SplitEntry                            & dseinst, 
                                           const std::map<uint16_t,size_t>        & smplIdConvTbl, 
                                           sf2::SoundFont                         & sf,
                                           sf2::Instrument                        & inst,
                                           const vector<bool>                     & loopedsmpls,
                                           const vector<LFOTblEntry>              & lfos,
                                           const KeyGroupList                     & keygrps,
                                           const SampleBank                       & smplbnk,
                                           SMDLPresetConversionInfo::PresetConvData & cvdata)
    {
        using namespace sf2;

        if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
            clog <<"\t--- Split#" <<static_cast<unsigned short>(dseinst.id) <<" ---\n";

        //Make a zone for this entry
        ZoneBag myzone;

        //  --- Set the generators ---

        //## Key range and vel range MUST be first, or else the soundfont parser will ignore them! ##
        myzone.SetKeyRange( dseinst.lowkey, dseinst.hikey ); //Key range always in first
        myzone.SetVelRange( dseinst.lovel,  dseinst.hivel );
        //## Add other generators below ##

        //Fetch Loop Flag
        auto ptrinf = smplbnk.sampleInfo(dseinst.smplid);
        if( ptrinf != nullptr && ptrinf->smplloop != 0 )
        {
            myzone.SetSmplMode( eSmplMode::loop );
        }

        //Since the soundfont format has no control on polyphony, we can only cut keygroup that only allow a single voice
        if( dseinst.kgrpid != 0 )
        {
            //if( keygrps.size() > dseinst.kgrpid && keygrps[dseinst.kgrpid].poly == 1 )
            //{
            //    myzone.SetExclusiveClass( dseinst.kgrpid );
            //}
            //else
            //{
                //if( dseinst.kgrpid >= keygrps.size() )
                //    cvdata.maxpoly = -1;
                //else
                    cvdata.maxpoly = keygrps[dseinst.kgrpid].poly; //let the midi converter handle anything else
            //}

        }


        //int8_t ctuneadd = (dseinst.ftune) / 100;
        //int8_t ftunes   = (dseinst.ftune) % 100; // utils::Clamp( dseinst.ftune, sf2::SF_GenLimitsFineTune.min_, sf2::SF_GenLimitsFineTune.max_ ); 

        ////#Test set pitch from scratch:
        //myzone.SetRootKey(dseinst.rootkey);//( (dseinst.rootkey - (dseinst.ktps + dseinst.ctune) + ctuneadd) );

        ////Pitch Correction
        //if( dseinst.ftune != 0 )
        //    myzone.SetFineTune( ftunes/*dseinst.ftune*/ );

        //if( dseinst.ctune != DSE::DSEDefaultCoarseTune )
        //    myzone.SetCoarseTune( /*( dseinst.ctune + 7 ) +*/ ctuneadd );



        //Volume
        if( dseinst.smplvol != DSE_LimitsVol.def_ )
            myzone.SetInitAtt( DseVolToSf2Attenuation(dseinst.smplvol) );

        //Pan
        if( dseinst.smplpan != DSE_LimitsPan.def_ )
            myzone.SetPan( DsePanToSf2Pan(dseinst.smplpan) );

        //Volume Envelope
        Envelope myenv = RemapDSEVolEnvToSF2(dseinst.env);

        /*
            ### In order to handle the atkvol param correctly, we'll make a copy of the Instrument, 
                with a very short envelope!

            #TODO : Make this part of the envelope parsing functions !!!
        */
        if( dseinst.env.atkvol != 0 && dseinst.env.attack != 0 )
        {
            ZoneBag atkvolzone( myzone );

            //Set attenuation to the atkvol value
            atkvolzone.SetInitAtt( DseVolToSf2Attenuation( dseinst.env.atkvol ) + 
                                   DseVolToSf2Attenuation(dseinst.smplvol) );

            //Set hold to the attack's duration
            Envelope atkvolenv;
            //if( dseinst.attack >= 10 )
            //    atkvolenv.hold = MSecsToTimecents( dseinst.attack - 10 );
            //else
                atkvolenv.hold = myenv.attack;
            atkvolenv.decay   = (dseinst.env.hold > 0)? myenv.hold : (dseinst.env.decay > 0 || dseinst.env.decay2 > 0)? myenv.decay : SHRT_MIN; 
            atkvolenv.sustain = SF_GenLimitsVolEnvSustain.max_;

            //Leave everything else to default
            //atkvolzone.SetVolEnvelope( atkvolenv );

            atkvolzone.AddGenerator( eSFGen::holdVolEnv,    atkvolenv.hold );
            atkvolzone.AddGenerator( eSFGen::sustainVolEnv, atkvolenv.sustain );
            
            //Sample ID in last
            atkvolzone.SetSampleId( smplIdConvTbl.at(dseinst.smplid) );

            inst.AddZone( std::move(atkvolzone) );
        }

        //Set the envelope
        myzone.SetVolEnvelope( myenv );

        //if( myenv.delay != SHRT_MIN )
        //    myzone.AddGenerator( eSFGen::delayVolEnv, myenv.delay );

        //if( myenv.attack != SHRT_MIN )
        //    myzone.AddGenerator( eSFGen::attackVolEnv, myenv.attack );

        //if( myenv.hold != SHRT_MIN )
        //    myzone.AddGenerator( eSFGen::holdVolEnv, myenv.hold );

        //if( myenv.decay != SHRT_MIN )
        //    myzone.AddGenerator( eSFGen::decayVolEnv, myenv.decay );

        ////if( myenv.sustain != 0 )
        //    myzone.AddGenerator( eSFGen::sustainVolEnv, myenv.sustain );

        //if( myenv.release != SHRT_MIN )
        //    myzone.AddGenerator( eSFGen::releaseVolEnv, myenv.release );

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
                               const ProgramInfo               & dsePres, 
                               const std::map<uint16_t,size_t> & smplIdConvTbl, 
                               sf2::SoundFont                  & sf,
                               uint16_t                        & instidcnt,
                               const vector<bool>              & loopedsmpls,
                               const KeyGroupList              & keygrps,
                               const shared_ptr<SampleBank>   && smplbnk,
                               SMDLPresetConversionInfo::PresetConvData  & convinf,
                               bool                              lfoeffectson )
    {
        using namespace sf2;
        Preset pre(presname, convinf.midipres, bankno );

        if( utils::LibWide().isLogOn() )
            clog <<"======================\nHandling " <<presname <<"\n======================\n";

        //#0 - Add a global zone for global preset settings
        {
            ZoneBag global;

            //#1 - Setup Generators
            if( dsePres.prgvol != DSE_LimitsVol.def_ )
                global.SetInitAtt( DseVolToSf2Attenuation(dsePres.prgvol) );

            // Range of DSE Pan : 0x00 - 0x40 - 0x7F
            if( dsePres.prgpan != DSE_LimitsPan.def_ )
                global.SetPan( DsePanToSf2Pan(dsePres.prgpan) );
            
            //#2 - Setup LFOs
            unsigned int cntlfo = 0;
            for( const auto & lfo : dsePres.m_lfotbl )
            {
                //!#TODO: Put the content of the loop in its own function. This also seem like duplicate code!! (HandleBakedPrgInst)
                if( lfo.unk52 != 0 && lfoeffectson ) //Is the LFO enabled ?
                {
                    if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
                        clog << "\tLFO" <<cntlfo <<" : Target: ";
                    
                    if( lfo.dest == static_cast<uint8_t>(LFOTblEntry::eLFODest::Pitch) ) //Pitch
                    {
                        //The effect on the pitch can be handled this way
                        global.AddGenerator( eSFGen::freqVibLFO,    DSE_LFOFrequencyToCents( lfo.depth/*lfo.rate*//*/50*/ ) ); //Frequency
                        global.AddGenerator( eSFGen::vibLfoToPitch, lfo.rate ); //Depth /*static_cast<uint16_t>( lround( static_cast<double>(lfo.rate) / 2.5 )*/
                        global.AddGenerator( eSFGen::delayVibLFO,   MSecsToTimecents( lfo.delay ) ); //Delay
                        
                        if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
                            clog << "(1)pitch";
                    }
                    else if( lfo.dest == static_cast<uint8_t>(LFOTblEntry::eLFODest::Volume) ) //Volume
                    {
                        //The effect on the pitch can be handled this way
                        global.AddGenerator( eSFGen::freqModLFO,     DSE_LFOFrequencyToCents(lfo.rate/*lfo.rate*//*/50*/ ) ); //Frequency
                        global.AddGenerator( eSFGen::modLfoToVolume, -(lfo.depth * (20) / 127) /*//*(lfo.depth/10) * -1*/ ); //Depth
                        global.AddGenerator( eSFGen::delayModLFO,    MSecsToTimecents( lfo.delay ) ); //Delay

                        if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
                            clog << "(2)volume";
                    }
                    else if( lfo.dest == static_cast<uint8_t>(LFOTblEntry::eLFODest::Pan) ) //Pan
                    {
                        //Leave the data for the MIDI exporter, so maybe it can do something about it..
                        convinf.extrafx.push_back( 
                            SMDLPresetConversionInfo::ExtraEffects{ SMDLPresetConversionInfo::eEffectTy::Phaser, lfo.rate, lfo.depth, lfo.delay } 
                        );

                        

                        //#TODO:
                        //We still need to figure a way to get the LFO involved, and set the oscilation frequency!
                        if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
                            clog << "(3)pan";
                    }
                    else if( lfo.dest == static_cast<uint8_t>(LFOTblEntry::eLFODest::UNK_4) )
                    {
                        //Unknown LFO target
                        if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
                            clog << "(4)unknown";
                    }
                    else
                    {
                        if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
                            clog << "(" << static_cast<unsigned short>(lfo.dest) <<")unknown";
                    }

                    if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
                    {
                        clog <<", Frequency: " << static_cast<unsigned short>(lfo.rate) 
                             << " Hz, Depth: " << static_cast<unsigned short>(lfo.depth) 
                             << ", Delay: " <<static_cast<unsigned short>(lfo.delay) <<" ms\n";
                    }
                }
                ++cntlfo;
            }

            //#3 - Add the global zone to the list!
            if( global.GetNbGenerators() > 0 || global.GetNbModulators() > 0 )
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
        {
            DSESplitToSf2InstrumentZone( dsePres.m_splitstbl[cntsmpl], 
                                         smplIdConvTbl, 
                                         sf, 
                                         myinst, 
                                         loopedsmpls, 
                                         dsePres.m_lfotbl, 
                                         keygrps, 
                                         *(smplbnk.get()),
                                         convinf );
        }
        sf.AddInstrument( std::move(myinst) );
        instzone.SetInstrumentId(instidcnt);
        pre.AddZone( std::move(instzone) ); //Add the instrument zone after the global zone!
        ++instidcnt; //Increase the instrument count

        sf.AddPreset( std::move(pre) );
    }

    /***************************************************************************************
        AddSampleToSoundfont
    ***************************************************************************************/
    void AddSampleToSoundfont( size_t                           cntsmslot, 
                               shared_ptr<SampleBank>           & samples, 
                               map<uint16_t,size_t>             & swdsmplofftosf,  
                               sf2::SoundFont                   & sf )
    {
        using namespace sf2;

        if( samples->IsInfoPresent(cntsmslot) && samples->IsDataPresent(cntsmslot) ) 
        {
            const auto        & cursminf = *(samples->sampleInfo(cntsmslot));
            Sample::loadfun_t   loadfun;
            Sample::smplcount_t smpllen = 0;
            Sample::smplcount_t loopbeg = 0;
            Sample::smplcount_t loopend = 0;

            if( cursminf.smplfmt == eDSESmplFmt::ima_adpcm )
            {
                loadfun = std::move( std::bind( ::audio::DecodeADPCM_NDS, std::ref( *samples->sample(cntsmslot) ), 1 ) );
                smpllen = ::audio::ADPCMSzToPCM16Sz(samples->sample(cntsmslot)->size() );
                loopbeg = (cursminf.loopbeg - SizeADPCMPreambleWords) * 8; //loopbeg is counted in int32, for APCM data, so multiply by 8 to get the loop beg as pcm16. Subtract one, because of the preamble.
                loopend = smpllen;
            }
            else if( cursminf.smplfmt == eDSESmplFmt::pcm16 )
            {
                loadfun = std::move( std::bind( &RawBytesToPCM16Vec, samples->sample(cntsmslot) ) );
                smpllen = samples->sample(cntsmslot)->size() / 2;
                loopbeg = cursminf.loopbeg * 2; //loopbeg is counted in int32, so multiply by 2 to get the loop beg as pcm16
                loopend = smpllen;
            }
            else if( cursminf.smplfmt == eDSESmplFmt::pcm8 )
            {
                loadfun = std::move( std::bind( &PCM8RawBytesToPCM16Vec, samples->sample(cntsmslot) ) );
                smpllen = samples->sample(cntsmslot)->size() * 2; //PCM8 -> PCM16
                loopbeg = cursminf.loopbeg * 4; //loopbeg is counted in int32, for PCM8 data, so multiply by 4 to get the loop beg as pcm16
                loopend = smpllen;
            }
            else if( cursminf.smplfmt == eDSESmplFmt::psg )
            {
                //stringstream sstrerr;
                //sstrerr << "PSG instruments unsuported!";
                //throw std::runtime_error( sstrerr.str() );
                clog << "<!>- Warning: AddSampleToSoundfont(): Unsuported type 3 sample added!\n";
            }
            else
            {
                stringstream sstrerr;
                sstrerr << "Unknown sample format (0x" <<hex <<uppercase <<static_cast<uint16_t>(cursminf.smplfmt) <<nouppercase <<dec  <<") encountered !";
                throw std::runtime_error( sstrerr.str() );
            }

#ifdef _DEBUG
            assert( (loopbeg < smpllen || loopend >= smpllen) );
#endif

            Sample sm( std::move( loadfun ), smpllen );
            stringstream sstrname;
            sstrname << "smpl_0x" <<uppercase <<hex <<cntsmslot;
            //sm.SetName( "smpl#" + to_string(cntsmslot) );
            sm.SetName( sstrname.str() );
            sm.SetSampleRate ( cursminf.smplrate );

//#ifdef _DEBUG
            //double pcorrect  = 1.0 / 100.0 / 2.5 / lround( static_cast<double>(cursminf.pitchoffst) );
            //double remainder = abs(pcorrect) - 127;
            //sm.SetPitchCorrection( static_cast<int8_t>(lround(pcorrect)) ); //Pitch correct is 1/250th of a semitone, while the SF2 pitch correction is 1/100th of a semitone

            //if( remainder > 0)
            //    cout <<"########## Sample pitch correct remainder !!!! ####### " <<showbase <<showpoint << remainder <<noshowbase <<noshowpoint <<"\n";
//#endif

            sm.SetSampleType ( Sample::eSmplTy::monoSample ); //#TODO: Mono samples only for now !

            if( cursminf.smplloop != 0 )
                sm.SetLoopBounds ( loopbeg, loopend );
                
            swdsmplofftosf.emplace( cntsmslot, sf.AddSample( std::move(sm) ) );
        }
    }

    /***************************************************************************************
        HandlePrgSplitBaked
    ***************************************************************************************/
    void BatchAudioLoader::HandlePrgSplitBaked( sf2::SoundFont                     * destsf2, 
                                                const DSE::SplitEntry              & split,
                                                size_t                               sf2sampleid,
                                                const DSE::WavInfo                 & smplinf,
                                                const DSE::KeyGroup                & keygroup,
                                                SMDLPresetConversionInfo::PresetConvData  & presetcvinf,
                                                sf2::Instrument                    * destinstsf2 )
    {
        using namespace sf2;

        if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
            clog <<"\t--- Split#" <<static_cast<unsigned short>(split.id) <<" ---\n";

        //Make a zone for this entry
        ZoneBag myzone;

        //  --- Set the generators ---
        //## Key range and vel range MUST be first, or else the soundfont parser will ignore them! ##
        myzone.SetKeyRange( split.lowkey, split.hikey ); //Key range always in first
        myzone.SetVelRange( split.lovel,  split.hivel );

        //## Add other generators below ##

        //Fetch Loop Flag
        if( smplinf.smplloop != 0 )
            myzone.SetSmplMode( eSmplMode::loop );

        //Since the soundfont format has no control on polyphony, we can only cut keygroup that only allow a single voice
        if( split.kgrpid != 0 )
            presetcvinf.maxpoly = keygroup.poly; //let the midi converter handle anything else

        int8_t ctuneadd = (split.ftune) / 100;/*100;*/
        int8_t ftunes   = (split.ftune) % 100;/*100;*/ // utils::Clamp( dseinst.ftune, sf2::SF_GenLimitsFineTune.min_, sf2::SF_GenLimitsFineTune.max_ ); 


        //Root Pitch
        myzone.SetRootKey( split.rootkey ); // + (split.ktps + split.ctune) + ctuneadd ); //split.rootkey
        //myzone.SetRootKey( split.rootkey + (/*split.ktps +*/ split.ctune) + ctuneadd );
        //cout << "\trootkey :" << static_cast<short>(split.rootkey + (split.ktps + split.ctune) + ctuneadd) <<"\n"; 

        //Pitch Correction
        //if( split.ftune != 0 )
        //    myzone.SetFineTune( ftunes );

        //if( split.ctune != DSE::DSEDefaultCoarseTune )
        //    myzone.SetCoarseTune( split.ctune );

        //Pitch Correction
        //if( split.ftune != 0 )
        //    myzone.SetFineTune( ftunes/*split.ftune*/ );

        //if( split.ctune != DSE::DSEDefaultCoarseTune )
        //    myzone.SetCoarseTune( /*( split.ctune + 7 ) +*/ split.ctune + ctuneadd );

        //Volume
        if( split.smplvol != DSE_LimitsVol.def_ )
            myzone.SetInitAtt( DseVolToSf2Attenuation(split.smplvol) );

        //Pan
        if( split.smplpan != DSE_LimitsPan.def_ )
            myzone.SetPan( DsePanToSf2Pan(split.smplpan) );

        //Volume Envelope
        //DSEEnvelope dseenv(split);
        Envelope myenv = RemapDSEVolEnvToSF2(split.env);

        //Set the envelope
        myzone.SetVolEnvelope( myenv );
        
        if( split.env.release != 0 )
            myzone.AddGenerator( eSFGen::releaseVolEnv, static_cast<sf2::genparam_t>( lround(myenv.release * 1.1) ) );

        //Sample ID in last
        myzone.SetSampleId( sf2sampleid );

        destinstsf2->AddZone( std::move(myzone) );
    }


    /***************************************************************************************
        HandleBakedPrgInst
    ***************************************************************************************/
    void BatchAudioLoader::HandleBakedPrgInst( const ProcessedPresets::PresetEntry   & entry, 
                                               sf2::SoundFont                        * destsf2, 
                                               const std::string                     & presname, 
                                               int                                     cntpair, 
                                               SMDLPresetConversionInfo::PresetConvData  & presetcvinf,
                                               int                                   & instidcnt,
                                               int                                   & presetidcnt,
                                               const DSE::KeyGroupList     & keygroups )
    {
        using namespace sf2;
        Preset sf2preset(presname, presetcvinf.midipres, presetcvinf.midibank );

        const auto & curprg      = entry.prginf;
        const auto & cursmpls    = entry.splitsamples;  //PCM16 samples for each split slots
        const auto & cursmplsinf = entry.splitsmplinf;  //New sample info for each samples in entry.splitsamples

        if( utils::LibWide().isLogOn() )
            clog <<"======================\nHandling " <<presname <<"\n======================\n";


        //#0 - Add a global zone for global preset settings
        {
            ZoneBag global;

            //#1 - Setup Generators
            //if( curprg.m_hdr.insvol != DSE_LimitsVol.def_ )
            //    global.SetInitAtt( DseVolToSf2Attenuation(curprg.m_hdr.insvol) );

            // Range of DSE Pan : 0x00 - 0x40 - 0x7F
            if( curprg.prgpan != DSE_LimitsPan.def_ )
                global.SetPan( DsePanToSf2Pan(curprg.prgpan) );
            
            //#2 - Setup LFOs
            unsigned int cntlfo = 0;
            for( const auto & lfo : curprg.m_lfotbl )
            {
                //!#TODO: Put the content of this loop in a function!!!!
                if( lfo.unk52 != 0 && m_lfoeffects ) //Is the LFO enabled ?
                {
                    if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
                        clog << "\tLFO" <<cntlfo <<" : Target: ";

                    //Gather statistics
                    m_stats.lfodepth.Process( lfo.depth );
                    m_stats.lforate.Process( lfo.rate );
                    
                    if( lfo.dest == static_cast<uint8_t>(LFOTblEntry::eLFODest::Pitch) ) //Pitch
                    {
                        if( lfo.rate <= 100 )
                        {
                            //The effect on the pitch can be handled this way
                            global.AddGenerator( eSFGen::freqVibLFO,    DSE_LFOFrequencyToCents( lfo.rate ) ); //Frequency
                            global.AddGenerator( eSFGen::vibLfoToPitch, DSE_LFODepthToCents( lfo.depth ) ); //Depth 
                            global.AddGenerator( eSFGen::delayVibLFO,   MSecsToTimecents( lfo.delay ) ); //Delay
                        
                            if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
                                clog << "(1)pitch";
                        }
                        else if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
                        {
                            clog << "<!>- LFO Vibrato effect was ignored, because the rate(" <<lfo.rate <<") is higher than what Soundfont LFOs supports!\n";
                        }
                    }
                    else if( lfo.dest == static_cast<uint8_t>(LFOTblEntry::eLFODest::Volume) ) //Volume
                    {
                        if( lfo.rate <= 100 )
                        {
                            //The effect on the pitch can be handled this way
                            global.AddGenerator( eSFGen::freqModLFO,     DSE_LFOFrequencyToCents(lfo.rate ) ); //Frequency
                            global.AddGenerator( eSFGen::modLfoToVolume, DSE_LFODepthToCents( lfo.depth ) ); //Depth
                            global.AddGenerator( eSFGen::delayModLFO,    MSecsToTimecents( lfo.delay ) ); //Delay

                            if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
                                clog << "(2)volume";
                        }
                        else if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
                        {
                            clog << "<!>- LFO volume level effect was ignored, because the rate(" <<lfo.rate <<") is higher than what Soundfont LFOs supports!\n";
                        }
                    }
                    else if( lfo.dest == static_cast<uint8_t>(LFOTblEntry::eLFODest::Pan) ) //Pan
                    {
                        //Leave the data for the MIDI exporter, so maybe it can do something about it..
                        presetcvinf.extrafx.push_back( 
                            SMDLPresetConversionInfo::ExtraEffects{ SMDLPresetConversionInfo::eEffectTy::Phaser, lfo.rate, lfo.depth, lfo.delay } 
                        );

                        //#TODO:
                        //We still need to figure a way to get the LFO involved, and set the oscilation frequency!
                        if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
                            clog << "(3)pan";
                    }
                    else if( lfo.dest == static_cast<uint8_t>(LFOTblEntry::eLFODest::UNK_4) )
                    {
                        //Unknown LFO target
                        if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
                            clog << "(4)unknown";
                    }
                    else
                    {
                        if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
                            clog << "(" << static_cast<unsigned short>(lfo.dest) <<")unknown";
                    }

                    if( utils::LibWide().isLogOn()  && utils::LibWide().isVerboseOn() )
                    {
                        clog <<", Frequency: " << static_cast<unsigned short>(lfo.rate) 
                                << " Hz, Depth: " << static_cast<unsigned short>(lfo.depth) 
                                << ", Delay: " <<static_cast<unsigned short>(lfo.delay) <<" ms\n";
                    }
                }
                ++cntlfo;
            }

            //#3 - Add the global zone to the list!
            if( global.GetNbGenerators() > 0 || global.GetNbModulators() > 0 )
                sf2preset.AddZone( std::move(global) );
        }     

        //Process
        stringstream sstrinstnames;
        sstrinstnames << "Prg" <<presetidcnt << "->Inst" <<instidcnt;
        Instrument myinst( sstrinstnames.str() );
        ZoneBag    instzone;


        if( cursmpls.empty() && cursmplsinf.empty() )
        {
            clog << "<!>- No samples!!!\n";
#ifdef _DEBUG
            assert(false);
#endif
            return;
        }
        else if( cursmpls.empty() || cursmplsinf.empty() )
        {
            clog << "<!>- Mismatch between the nb of sample info and converted samples for this program!!! ( " <<cursmpls.size() <<"," <<cursmplsinf.size() <<" )\n";
#ifdef _DEBUG
            assert(false);
#endif
            return;
        }

        for( size_t cntsplit = 0; cntsplit < curprg.m_splitstbl.size(); ++cntsplit )
        {
            const auto & cursplit = curprg.m_splitstbl[cntsplit]; 
            stringstream sstrnames;
            sstrnames <<"Prg" <<presetidcnt << "->Smpl" <<cntsplit;

            //Place the sample used by the current split in the soundfont
            sf2::Sample sampl(  cursmpls[cntsplit].begin(), 
                                cursmpls[cntsplit].end(), 
                                sstrnames.str(),
                                0,
                                0,
                                cursmplsinf[cntsplit].smplrate,
                                cursplit.rootkey );

            if( cursmplsinf[cntsplit].smplloop != 0 )
                sampl.SetLoopBounds( cursmplsinf[cntsplit].loopbeg, cursmplsinf[cntsplit].loopbeg + cursmplsinf[cntsplit].looplen );

            size_t sf2smplindex = destsf2->AddSample( move(sampl) );

            //Make sure the KGrp exists, because prof layton is sloppy..
            const auto * ptrkgrp = &(keygroups.front());
            if( cursplit.kgrpid < keygroups.size() )
                ptrkgrp = &(keygroups[cursplit.kgrpid]);

            //Add our split
            HandlePrgSplitBaked( destsf2, 
                                 cursplit, 
                                 sf2smplindex, 
                                 cursmplsinf[cntsplit], 
                                 *ptrkgrp, 
                                 presetcvinf, 
                                 &myinst );
        }


        destsf2->AddInstrument( std::move(myinst) );
        instzone.SetInstrumentId(instidcnt);
        sf2preset.AddZone( std::move(instzone) ); //Add the instrument zone after the global zone!
        ++instidcnt; //Increase the instrument count
        destsf2->AddPreset( std::move(sf2preset) );
        ++presetidcnt;

    }


    /***************************************************************************************
        HandleBakedPrg
            Handles all presets for a file
    ***************************************************************************************/
    void BatchAudioLoader::HandleBakedPrg( const ProcessedPresets                & entry, 
                                           sf2::SoundFont                        * destsf2, 
                                           const std::string                     & curtrkname, 
                                           int                                     cntpair,
                                           std::vector<SMDLPresetConversionInfo> & presetcvinf,
                                           int                                   & instidcnt,
                                           int                                   & presetidcnt,
                                           const DSE::KeyGroupList      & keygroups
                                           )
    {
        using namespace sf2;

        auto & cvinfo = presetcvinf[cntpair];

        for( const auto & dseprg : entry )
        {
            const auto & curprg      = dseprg.second.prginf;
            const auto & cursmpls    = dseprg.second.splitsamples;
            const auto & cursmplsinf = dseprg.second.splitsmplinf;

            auto itcvinf = cvinfo.FindConversionInfo( curprg.id );

            if( itcvinf == cvinfo.end() )
            {
                clog << "<!>- Warning: The SWDL + SMDL pair #" <<cntpair <<", for preset " <<presetidcnt <<" is missing a conversion info entry.\n";
#ifdef _DEBUG
                assert(false);
#endif
                ++presetidcnt;
                continue;   //Skip this preset
            }

            if( cursmplsinf.size() != cursmpls.size() )
            {
                clog << "<!>- Warning: The SWDL + SMDL pair #" <<cntpair <<", for preset " <<presetidcnt <<" has mismatched lists of splits and samples.\n";
#ifdef _DEBUG
                assert(false);
#endif
                ++presetidcnt;
                continue;   //Skip this preset
            }
            
            if( cursmplsinf.empty() )
            {
                clog << "<!>- Warning: The SWDL + SMDL pair #" <<cntpair <<", for preset " <<presetidcnt <<" has " <<cursmplsinf.size() <<" Sample info slots, and " <<cursmpls.size() <<" samples .\n";
                ++presetidcnt;
                continue;   //Skip this preset
            }

            stringstream sstrprgname;
            sstrprgname << curtrkname << "_prg#" <<showbase <<hex <<curprg.id;
            const string presname = move( sstrprgname.str() );


            HandleBakedPrgInst( dseprg.second, destsf2, presname, cntpair, itcvinf->second, instidcnt, presetidcnt, keygroups );
        }
    }

    /***************************************************************************************
        ExportSoundfontBakedSamples
            
    ***************************************************************************************/
    vector<SMDLPresetConversionInfo> BatchAudioLoader::ExportSoundfontBakedSamples( const std::string & destf )
    {
        using namespace sf2;

        if( m_pairs.size() > CHAR_MAX && !m_bSingleSF2 )
        {
            cout << "<!>- Error: Got over 127 different SMDL+SWDL pairs and set to preserve program numbers. Splitting into multiple soundfonts is not implemented yet!\n";
            //We need to build several smaller sf2 files, each in their own sub-dir with the midis they're tied to!
            assert(false); //#TODO
        }

        //If the main bank is not loaded, try to make one out of the loaded pairs!
        if( !IsMasterBankLoaded() )
            BuildMasterFromPairs();

        m_stats = audiostats(); //reset stats

        vector<SMDLPresetConversionInfo> trackprgconvlist = move( BuildPresetConversionDB() );
        SoundFont                        sf( m_master.metadata().fname ); 
        
        //Prepare
        shared_ptr<SampleBank>  samples = m_master.smplbank().lock();
        deque<ProcessedPresets> procpres; //We need to put all processed stuff in there, because the samples need to exist when the soundfont is written.

        //Counters for the unique preset and instruments IDs
        int cntpres = 0;
        int cntinst = 0;

        for( size_t cntpair = 0; cntpair < m_pairs.size();  )
        {
            const auto & curpair     = m_pairs[cntpair];
            const auto   prgptr      = curpair.second.prgmbank().lock();
            string       pairname    = "Trk#" + to_string(cntpair);

            if( prgptr != nullptr )
            {
                procpres.push_back( move( ProcessDSESamples( *samples, *prgptr ) ) );
                HandleBakedPrg( procpres.back(), &sf, pairname, cntpair, trackprgconvlist, cntinst, cntpres, prgptr->Keygrps() );
            }

            ++cntpair;
            if( prgptr != nullptr )
                cout <<"\r\tProcessing samples.." <<right <<setw(3) <<setfill(' ') <<((cntpair * 100) / m_pairs.size())  <<"%";
        }
        cout <<"\n";

        //Print Stats
        if( utils::LibWide().isLogOn() )
            clog << m_stats.Print();

        //Write the soundfont
        try
        {
            if( utils::LibWide().isLogOn() )
            {
                clog <<"\nWriting Soundfont...\n"
                     <<"================================================================================\n";
            }
            cout<<"\tWriting soundfont..";
            sf.Write( destf );
            cout<<"\n";
        }
        catch( const overflow_error & e )
        {
            stringstream sstr;
            sstr <<"There are too many different parameters throughout the music files in the folder to extract them to a single soundfont file!\n"
                 << e.what() 
                 << "\n";
            throw runtime_error( sstr.str() );
        }

        return move( trackprgconvlist );
    }



    /***************************************************************************************
        ExportSoundfont
            
    ***************************************************************************************/
    vector<SMDLPresetConversionInfo> BatchAudioLoader::ExportSoundfont( const std::string & destf )
    {
        using namespace sf2;

        if( m_pairs.size() > CHAR_MAX && !m_bSingleSF2 )
        {
            cout << "<!>- Error: Got over 127 different SMDL+SWDL pairs and set to preserve program numbers. Splitting into multiple soundfonts is not implemented yet!\n";
            //We need to build several smaller sf2 files, each in their own sub-dir with the midis they're tied to!
            assert(false); //#TODO
        }

        //If the main bank is not loaded, try to make one out of the loaded pairs!
        if( !IsMasterBankLoaded() )
            BuildMasterFromPairs();



        vector<SMDLPresetConversionInfo> merged = move( BuildPresetConversionDB() );
        SoundFont                        sf( m_master.metadata().fname ); 
        map<uint16_t,size_t>             swdsmplofftosf; //Make a map with as key a the sample id in the Wavi table, and as value the sample id in the sounfont!
        
        //Prepare samples list
        shared_ptr<SampleBank> samples = m_master.smplbank().lock();
        vector<bool>           loopedsmpls( samples->NbSlots(), false ); //Keep track of which samples are looped

        //Check all our sample slots and prepare loading them in the soundfont
        for( size_t cntsmslot = 0; cntsmslot < samples->NbSlots(); ++cntsmslot )
        {
            AddSampleToSoundfont( cntsmslot, samples, swdsmplofftosf, sf );
        }

        //Now build the Preset and instrument list !
        size_t   cntsf2presets = 0; //Count presets total
        uint16_t instsf2cnt    = 0;

        for( size_t cntpairs = 0; cntpairs < m_pairs.size(); ++cntpairs )// const auto & aswd : merged )
        {
            SMDLPresetConversionInfo & curcvinfo   = merged[cntpairs];                                //Get the conversion info for all presets in the current swd
            const auto & curpair     = m_pairs[cntpairs];                               //Get the current swd + smd pair
            const auto & curprginfos = curpair.second.prgmbank().lock()->PrgmInfo();    //Get the current SWD's program list
            const auto & curkgrp     = curpair.second.prgmbank().lock()->Keygrps();     //Get the current SWD's keygroup list
            auto         itcvtbl     = curcvinfo.begin();                      //Iterator on the conversion map!
            string       pairname    = "Trk#" + to_string(cntpairs);

            for( size_t prgcnt = 0; /*prgcnt < curcvinfo.size() && */itcvtbl != curcvinfo.end(); ++prgcnt, ++itcvtbl )
            {
                if( curprginfos[itcvtbl->first] != nullptr )
                {
                    stringstream sstrprgname;
                    sstrprgname << pairname << "_prg#" <<showbase <<hex <<curprginfos[itcvtbl->first]->id;
                    DSEPresetToSf2Preset( sstrprgname.str(), //pairname + "_prg#" + to_string(curprginfos[itcvtbl->first]->m_hdr.id), 
                                            itcvtbl->second.midibank,
                                            *(curprginfos[itcvtbl->first]),
                                            swdsmplofftosf,
                                            sf,
                                            instsf2cnt,
                                            loopedsmpls,
                                            curkgrp,
                                            m_master.smplbank().lock(),
                                            itcvtbl->second,
                                            m_lfoeffects );

                    ++cntsf2presets;
                }
                else
                {
                    assert(false); //This should never happen..
                }
            }
        }

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

        return move( merged );
    }

    /***************************************************************************************
        ExportXMLPrograms
            Export all the presets for each loaded swdl pairs! And if the sample data is 
            present, it will also export it!
    ***************************************************************************************/
    void BatchAudioLoader::ExportXMLPrograms( const std::string & destf )
    {
        //static const string _DefaultMainSampleDirName = "mainbank";
        //static const string _DefaultSWDLSmplDirName   = "samples";

        ////If the main bank is not loaded, try to make one out of the loaded pairs!
        //if( IsMasterBankLoaded() )
        //{
        //    //Make the main sample bank sub-directory if we have a master bank
        //    Poco::File outmbankdir( Poco::Path( destf ).append(_DefaultMainSampleDirName).makeDirectory() );
        //    if(!outmbankdir.createDirectory())
        //        throw runtime_error("BatchAudioLoader::ExportXMLPrograms(): Error, couldn't create output main bank samples directory!");

        //    //Export Main Bank samples :
        //    ExportPresetBank( outmbankdir.path(), m_master, true, false );
        //}

        ////Iterate through all the tracks 

        //for( size_t i = 0; i < m_pairs.size(); ++i )
        //{
        //    Poco::Path fpath(destf);
        //    fpath.append( to_string(i) + "_" + m_pairs[i].first.metadata().fname);
        //    
        //    if( ! Poco::File(fpath).createDirectory() )
        //    {
        //        clog <<"Couldn't create directory for track " <<fpath.toString() << "!\n";
        //        continue;
        //    }

        //    const auto & curpair     = m_pairs[cntpairs];                               //Get the current swd + smd pair
        //    const auto & curprginfos = curpair.second.prgmbank().lock()->PrgmInfo();    //Get the current SWD's program list
        //    const auto & curkgrp     = curpair.second.prgmbank().lock()->Keygrps();     //Get the current SWD's keygroup list
        //    auto         itcvtbl     = curcvinfo.begin();                      //Iterator on the conversion map!
        //    string       pairname    = "Trk#" + to_string(cntpairs);

        //}

    }

    /***************************************************************************************
        ExportSoundfontAndMIDIs
    ***************************************************************************************/
    void BatchAudioLoader::ExportSoundfontAndMIDIs( const std::string & destdir, int nbloops, bool bbakesamples )
    {
        //Export the soundfont first

        Poco::Path outsoundfont(destdir);
        outsoundfont.append( outsoundfont.getBaseName() + ".sf2").makeFile();
        cerr<<"<*>- Currently exporting main bank to " <<outsoundfont.toString() <<"\n";

        vector<SMDLPresetConversionInfo> merged;
        if( bbakesamples )
            merged = std::move( ExportSoundfontBakedSamples( outsoundfont.toString() ) );
        else
            merged = std::move( ExportSoundfont( outsoundfont.toString() ) );

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
                                 merged[i],
                                 nbloops,
                                 DSE::eMIDIMode::GS );  //This will disable the drum channel, since we don't need it at all!
        }
    }

    /***************************************************************************************
        ExportXMLAndMIDIs
            Export all music sequences to midis and export all preset data to 
            xml + pcm16 samples!
    ***************************************************************************************/
    void BatchAudioLoader::ExportXMLAndMIDIs( const std::string & destdir, int nbloops )
    {
        static const string _DefaultMainSampleDirName = "mainbank";

        if( IsMasterBankLoaded() )
        {
            //Make the main sample bank sub-directory if we have a master bank
            //Poco::File outmbankdir( Poco::Path( destdir ).append(_DefaultMainSampleDirName).makeDirectory() );
            Poco::Path outmbankpath( destdir );
            outmbankpath.append(_DefaultMainSampleDirName).makeDirectory().toString();

            
            if(! utils::DoCreateDirectory(outmbankpath.toString())  )
                throw runtime_error("BatchAudioLoader::ExportXMLPrograms(): Error, couldn't create output main bank samples directory!");

            //Export Main Bank samples :
            cerr<<"<*>- Currently exporting main bank to " <<outmbankpath.toString() <<"\n";
            ExportPresetBank( outmbankpath.toString(), m_master, false, false );
        }

        //Then the MIDIs + presets + optionally samples contained in the swd of the pair
        for( size_t i = 0; i < m_pairs.size(); ++i )
        {
            Poco::Path fpath(destdir);
            fpath.append( to_string(i) + "_" + m_pairs[i].first.metadata().fname);
            
            if( ! utils::DoCreateDirectory(fpath.toString()) )
            {
                clog <<"Couldn't create directory for track " <<fpath.toString() << "!\n";
                continue;
            }

            Poco::Path midpath(fpath);
            midpath.append( to_string(i) + "_" + m_pairs[i].first.metadata().fname);
            midpath.makeFile();
            midpath.setExtension("mid");

            cerr<<"<*>- Currently exporting smd + swd to " <<fpath.toString() <<"\n";
            DSE::SequenceToMidi( midpath.toString(), 
                                 m_pairs[i].first, 
                                 nbloops,
                                 DSE::eMIDIMode::GS );  //This will disable the drum channel, since we don't need it at all!

            ExportPresetBank( fpath.toString(), m_pairs[i].second, false, false );
        }
    }


    /***************************************************************************************
        BuildMasterFromPairs
            If no main bank is loaded, and the loaded pairs contain their own samples, 
            build a main bank from those!
    ***************************************************************************************/
    void BatchAudioLoader::BuildMasterFromPairs()
    {
        vector<SampleBank::smpldata_t> smpldata;
        bool                           bnosmpldata = true;

        //Iterate through all the pairs and fill up our sample data list !
        for( size_t cntpairs = 0; cntpairs < m_pairs.size(); cntpairs++ )
        {
            auto & presetbank = m_pairs[cntpairs].second;
            auto   ptrprgs    = presetbank.prgmbank().lock();
            auto   ptrsmplbnk = presetbank.smplbank().lock();

            if( ptrsmplbnk != nullptr  )
            {
                //We have sample data
                bnosmpldata = false;

                if( ptrprgs != nullptr )
                {
                    //Enlarge our vector when needed!
                    //if( ptrsmplbnk->NbSlots() > smpldata.size() )
                    //    smpldata.resize( ptrsmplbnk->NbSlots() );

                    //Copy the samples over into their matching slot!
                    for( size_t cntsmplslot = 0; cntsmplslot < ptrsmplbnk->NbSlots(); ++cntsmplslot )
                    {
                        if( ptrsmplbnk->IsDataPresent(cntsmplslot) && 
                            ptrsmplbnk->IsInfoPresent(cntsmplslot) /*&&
                            !smpldata[cntsmplslot].pdata_ &&
                            !smpldata[cntsmplslot].pinfo_*/ )
                        {
                            for( auto & prgm : ptrprgs->PrgmInfo() )
                            {
                                if( prgm != nullptr )
                                {
                                    for( auto & split : prgm->m_splitstbl )
                                    {
                                        if( split.smplid == cntsmplslot )
                                        {
                                            split.smplid = smpldata.size(); //Reassign sample IDs to our unified table!
                                        }
                                    }
                                }
                            }

                            //Insert a new sample entry in the table!
                            SampleBank::smpldata_t blk;
                            blk.pinfo_.reset( new DSE::WavInfo        (*ptrsmplbnk->sampleInfo(cntsmplslot) ) );
                            blk.pdata_.reset( new std::vector<uint8_t>(*ptrsmplbnk->sample(cntsmplslot)) );
                            smpldata.push_back( std::move(blk) );
                        }
                    }
                }
            }
        }

        if( bnosmpldata )
            throw runtime_error("BatchAudioLoader::BuildMasterFromPairs(): No sample data found in the SWDL containers that were loaded! Its possible the main bank was not loaded, or that no SWDL were loaded.");

        DSE_MetaDataSWDL meta;
        meta.fname       = "Main.SWD";
        meta.nbprgislots = 0;
        meta.nbwavislots = static_cast<uint16_t>( smpldata.size() );

        m_master = move( PresetBank( move(meta), 
                                     move( unique_ptr<SampleBank>(new SampleBank(move( smpldata ))) ) 
                                   ) );
    }
    //{
    //    vector<SampleBank::smpldata_t> smpldata;
    //    bool                           bnosmpldata = true;

    //    //Iterate through all the pairs and fill up our sample data list !
    //    for( size_t cntpairs = 0; cntpairs < m_pairs.size(); cntpairs++ )
    //    {
    //        auto & presetbank = m_pairs[cntpairs].second;
    //        auto   ptrsmplbnk = presetbank.smplbank().lock();

    //        if( ptrsmplbnk != nullptr )
    //        {
    //            //We have sample data
    //            bnosmpldata = false;


    //            //!#FIXME: This is really stupid. Not all games alloc the same ammount of samples as eachothers. 

    //            //Enlarge our vector when needed!
    //            if( ptrsmplbnk->NbSlots() > smpldata.size() )
    //                smpldata.resize( ptrsmplbnk->NbSlots() );

    //            //Copy the samples over into their matching slot!
    //            for( size_t cntsmplslot = 0; cntsmplslot < ptrsmplbnk->NbSlots(); ++cntsmplslot )
    //            {
    //                if( ptrsmplbnk->IsDataPresent(cntsmplslot) && 
    //                    ptrsmplbnk->IsInfoPresent(cntsmplslot) &&
    //                    !smpldata[cntsmplslot].pdata_ &&
    //                    !smpldata[cntsmplslot].pinfo_ )
    //                {
    //                    smpldata[cntsmplslot].pinfo_.reset( new DSE::WavInfo(*ptrsmplbnk->sampleInfo(cntsmplslot) ) );
    //                    smpldata[cntsmplslot].pdata_.reset( new std::vector<uint8_t>(*ptrsmplbnk->sample(cntsmplslot)) );
    //                }
    //            }
    //        }
    //    }

    //    if( bnosmpldata )
    //        throw runtime_error("BatchAudioLoader::BuildMasterFromPairs(): No sample data found in the SWDL containers that were loaded! Its possible the main bank was not loaded, or that no SWDL were loaded.");

    //    DSE_MetaDataSWDL meta;
    //    meta.fname       = "Main.SWD";
    //    meta.nbprgislots = 0;
    //    meta.nbwavislots = static_cast<uint16_t>( smpldata.size() );

    //    m_master = move( PresetBank( move(meta), 
    //                                 move( unique_ptr<SampleBank>(new SampleBank(move( smpldata ))) ) 
    //                               ) );
    //}


    /***************************************************************************************
        LoadMatchedSMDLSWDLPairs
            This function loads all matched smdl + swdl pairs in one or two different 
            directories.
    ***************************************************************************************/
    void BatchAudioLoader::LoadMatchedSMDLSWDLPairs( const std::string & swdldir, const std::string & smdldir )
    {
        //Grab all the swd and smd pairs in the folder
        Poco::DirectoryIterator dirit(smdldir);
        Poco::DirectoryIterator diritend;
        cout << "<*>- Loading matched smd in the " << smdldir <<" directory..\n";

        unsigned int cntparsed = 0;
        while( dirit != diritend )
        {
            string fext = dirit.path().getExtension();
            std::transform(fext.begin(), fext.end(), fext.begin(), ::tolower);

            //Check all smd/swd file pairs
            if( fext == SMDL_FileExtension )
            {
                Poco::File matchingswd( Poco::Path(swdldir).append(dirit.path().getBaseName()).makeFile().setExtension(SWDL_FileExtension) );
                    
                if( matchingswd.exists() && matchingswd.isFile() )
                {
                    cout <<"\r[" <<setfill(' ') <<setw(4) <<right <<cntparsed <<" pairs loaded] - Currently loading : " <<dirit.path().getFileName() <<"..";
                    LoadSmdSwdPair( dirit.path().toString(), matchingswd.path() );
                    ++cntparsed;
                }
                else
                    cout<<"<!>- File " << dirit.path().toString() <<" is missing a matching .swd file! Skipping !\n";
            }
            ++dirit;
        }
        cout <<"\n..done\n\n";
    }



    /*
        LoadBgmContainer
            Load a single bgm container file.
            Bgm containers are SWDL and SMDL pairs packed into a single file using a SIR0 container.
    */
    void BatchAudioLoader::LoadBgmContainer( const std::string & file )
    {
        if( utils::LibWide().isLogOn() )
            clog << "--------------------------------------------------------------------------\n"
                 << "Parsing BGM container \"" <<Poco::Path(file).getFileName() <<"\"\n"
                 << "--------------------------------------------------------------------------\n";

        auto pairdata( move( ReadBgmContainer( file ) ) );
        //Tag our files with their original file name, for cvinfo lookups to work!
        pairdata.first.metadata().origfname  = Poco::Path(file).getBaseName();
        pairdata.second.metadata().origfname = Poco::Path(file).getBaseName();

        m_pairs.push_back( move( std::make_pair( std::move(pairdata.second), std::move(pairdata.first) ) ) );
    }

    /*
        LoadBgmContainers
            Load all pairs in the folder. 
            Bgm containers are SWDL and SMDL pairs packed into a single file using a SIR0 container.

            - bgmdir : The directory where the bgm containers are located at.
            - ext    : The file extension the bgm container files have.
    */
    void BatchAudioLoader::LoadBgmContainers( const std::string & bgmdir, const std::string & ext )
    {
        //Grab all the bgm containers in here
        Poco::DirectoryIterator dirit(bgmdir);
        Poco::DirectoryIterator diritend;
        cout << "<*>- Loading bgm containers *." <<ext <<" in the " << bgmdir <<" directory..\n";

        stringstream sstrloadingmesage;
        sstrloadingmesage << " *." <<ext <<" loaded] - Currently loading : ";
        const string loadingmsg = sstrloadingmesage.str();

        unsigned int cntparsed = 0;
        while( dirit != diritend )
        {
            if( dirit->isFile() )
            {
                string fext = dirit.path().getExtension();
                std::transform(fext.begin(), fext.end(), fext.begin(), ::tolower);

                //Check all smd/swd file pairs
                try
                {
                    if( fext == ext )
                    {
                        cout <<"\r[" <<setfill(' ') <<setw(4) <<right <<cntparsed <<loadingmsg <<dirit.path().getFileName() <<"..";
                        LoadBgmContainer( dirit.path().absolute().toString() );
                        ++cntparsed;
                    }
                }
                catch( std::exception & e )
                {
                    cerr <<"\nBatchAudioLoader::LoadBgmContainers(): Exception : " <<e.what() <<"\n"
                         <<"Skipping file and attemnpting to recover!\n\n";
                    if( utils::LibWide().isLogOn() )
                    {
                        clog <<"\nBatchAudioLoader::LoadBgmContainers(): Exception : " <<e.what() <<"\n"
                         <<"Skipping file and attemnpting to recover!\n\n";
                    }
                }
            }
            ++dirit;
        }
        cout <<"\n..done\n\n";
    }

    /*
        LoadSingleSMDLs
            Loads only SMDL in the folder.
    */
    void BatchAudioLoader::LoadSingleSMDLs( const std::string & smdldir )
    {
        //Grab all the swd and smd pairs in the folder
        Poco::DirectoryIterator dirit(smdldir);
        Poco::DirectoryIterator diritend;
        cout << "<*>- Loading smd files in directory \"" << smdldir <<"\"..\n";

        unsigned int cntparsed = 0;
        while( dirit != diritend )
        {
            if( dirit->isFile() )
            {
                string fext = dirit.path().getExtension();
                std::transform(fext.begin(), fext.end(), fext.begin(), ::tolower);

                if( fext == SMDL_FileExtension )
                {
                    cout <<"\r[" <<setfill(' ') <<setw(4) <<right <<cntparsed <<" smd loaded] - Currently loading : " <<dirit.path().getFileName() <<"..";
                    LoadSMDL( dirit.path().toString() );
                    ++cntparsed;
                }
            }
            ++dirit;
        }
        cout <<"\n..done\n\n";
    }

    /*
    */
    void BatchAudioLoader::LoadSMDL( const std::string & smdl )
    {
        m_pairs.push_back( move( make_pair( move(ParseSMDL(smdl)), move( PresetBank() ) ) ) );
        //Tag our files with their original file name, for cvinfo lookups to work!
        m_pairs.back().first.metadata().origfname = Poco::Path(smdl).getBaseName();
    }

    /*
    */
    void BatchAudioLoader::ExportMIDIs( const std::string & destdir, const std::string & cvinfopath, int nbloops )
    {
        DSE::SMDLConvInfoDB cvinf;

        if( ! cvinfopath.empty() )
            cvinf.Parse( cvinfopath );

        //Then the MIDIs
        for( size_t i = 0; i < m_pairs.size(); ++i )
        {
            //Lookup cvinfo with the original filename from the game filesystem!
            auto itfound = cvinf.end();

            if(! cvinf.empty() )
                itfound = cvinf.FindConversionInfo( m_pairs[i].first.metadata().origfname );

            Poco::Path fpath(destdir);
            fpath.append( to_string(i) + "_" + m_pairs[i].first.metadata().fname).makeFile().setExtension("mid");

            cout <<"<*>- Currently exporting smd to " <<fpath.toString() <<"\n";
            if( utils::LibWide().isLogOn() )
                clog <<"<*>- Currently exporting smd to " <<fpath.toString() <<"\n";

            if( itfound != cvinf.end() )
            {
                if( utils::LibWide().isLogOn() )
                    clog << "<*>- Got conversion info for this track! MIDI will be remapped accordingly!\n";
                DSE::SequenceToMidi( fpath.toString(), 
                                     m_pairs[i].first, 
                                     itfound->second,
                                     nbloops,
                                     DSE::eMIDIMode::GS );  //This will disable the drum channel, since we don't need it at all!
            }
            else
            {
                if( utils::LibWide().isLogOn() )
                    clog <<"<!>- Couldn't find a conversion info entry for this SMDL! Falling back to converting as-is..\n";
                DSE::SequenceToMidi( fpath.toString(), 
                                     m_pairs[i].first, 
                                     nbloops,
                                     DSE::eMIDIMode::GS );  //This will disable the drum channel, since we don't need it at all!
            }
        }
    }


    /*
    */
    void BatchAudioLoader::LoadSMDLSWDLSPairsFromBlob( const std::string & blob )
    {
        vector<uint8_t> filedata = utils::io::ReadFileToByteVector( blob );

        BlobScanner<vector<uint8_t>::const_iterator> blobscan( filedata.begin(), filedata.end() );
        blobscan.Scan();
        auto foundpairs = blobscan.ListAllMatchingSMDLPairs();

        size_t cntpairs = 0;
        cout << "------------------------\n"
             << "Loading pairs..\n"
             << "------------------------\n";
        for( const auto apair : foundpairs )
        {
            if( utils::LibWide().isLogOn() )
            {
                clog <<"====================================================================\n"
                     <<"Parsing SWDL " <<apair.first._name <<"\n"
                     <<"====================================================================\n";
            }
            DSE::PresetBank    bank( move( DSE::ParseSWDL( apair.first._beg,  apair.first._end  ) ) );

            if( utils::LibWide().isLogOn() )
            {
                clog <<"====================================================================\n"
                     <<"Parsing SMDL " <<apair.second._name <<"\n"
                     <<"====================================================================\n";
            }
            DSE::MusicSequence seq ( move( DSE::ParseSMDL( apair.second._beg, apair.second._end ) ) );

            //Tag our files with their original file name, for cvinfo lookups to work!
            seq.metadata().origfname  = apair.second._name;
            bank.metadata().origfname = apair.first._name;

            m_pairs.push_back( move( std::make_pair( std::move(seq), std::move(bank) ) ) );

            ++cntpairs;
            cout <<"\r[" <<setfill(' ') <<setw(4) <<right <<cntpairs <<" of " <<foundpairs.size() <<" loaded]" <<"..";
        }

        cout<<"\n..done!\n";
    }

    /*
    */
    void BatchAudioLoader::LoadSMDLSWDLPairsAndBankFromBlob( const std::string & blob, const std::string & bankname )
    {
        vector<uint8_t> filedata = utils::io::ReadFileToByteVector( blob );

        BlobScanner<vector<uint8_t>::const_iterator> blobscan( filedata.begin(), filedata.end() );
        blobscan.Scan();
        auto foundpairs = blobscan.ListAllMatchingSMDLPairs();

        for( const auto apair : foundpairs )
        {
            if( utils::LibWide().isLogOn() )
            {
                clog <<"====================================================================\n"
                     <<"Parsing SWDL " <<apair.first._name <<"\n"
                     <<"====================================================================\n";
            }
            DSE::PresetBank    bank( move( DSE::ParseSWDL( apair.first._beg,  apair.first._end  ) ) );

            if( utils::LibWide().isLogOn() )
            {
                clog <<"====================================================================\n"
                     <<"Parsing SMDL " <<apair.second._name <<"\n"
                     <<"====================================================================\n";
            }
            DSE::MusicSequence seq ( move( DSE::ParseSMDL( apair.second._beg, apair.second._end ) ) );

            //Tag our files with their original file name, for cvinfo lookups to work!
            seq.metadata().origfname  = apair.second._name;
            bank.metadata().origfname = apair.first._name;

            m_pairs.push_back( move( std::make_pair( std::move(seq), std::move(bank) ) ) );
        }

        string fixedbankname;
        if( bankname.size() > BlobScanner<vector<uint8_t>::const_iterator>::FilenameLength )
            fixedbankname = bankname.substr( 0, BlobScanner<vector<uint8_t>::const_iterator>::FilenameLength );

        auto foundBank = blobscan.FindSWDL( fixedbankname );

        if( !foundBank.empty() && foundBank.size() == 1 )
        {
            const auto & swdlfound = foundBank.front();
            
            if( utils::LibWide().isLogOn() )
            {
                clog <<"====================================================================\n"
                     <<"Parsing SWDL " <<swdlfound._name <<"\n"
                     <<"====================================================================\n";
            }
            DSE::PresetBank bank( move( DSE::ParseSWDL( swdlfound._beg, swdlfound._end ) ) );
            bank.metadata().origfname = swdlfound._name;
            m_master = move(bank);
        }
        else if( foundBank.empty() )
        {
            stringstream sstrer;
            sstrer << "BatchAudioLoader::LoadSMDLSWDLPairsAndBankFromBlob() : Couldn't find a SWDL bank named \""
                   << fixedbankname
                   << "\" ! (Name was trimed to 16 characters)";
            throw runtime_error( sstrer.str() );
        }
        else
        {
            stringstream sstrer;
            sstrer << "BatchAudioLoader::LoadSMDLSWDLPairsAndBankFromBlob() : Found more than a single SWDL bank named \""
                   << fixedbankname
                   << "\" ! (Name was trimed to 16 characters)";
            throw runtime_error( sstrer.str() );
        }

    }

//===========================================================================================
//  Functions
//===========================================================================================
    
    void ExportPresetBank( const std::string & directory, const DSE::PresetBank & bnk, bool samplesonly, bool hexanumbers, bool noconvert )
    {
        static const string _DeafaultSamplesSubDir = "samples";
        auto smplptr = bnk.smplbank().lock();
        
        if( smplptr != nullptr )
        {
            size_t       nbsamplesexport = 0;
            const size_t nbslots         = smplptr->NbSlots();
            string       smpldir;

            if( !samplesonly )
            {
                Poco::Path smpldirpath(directory);
                smpldirpath.append(_DeafaultSamplesSubDir).makeDirectory();
                if( ! utils::DoCreateDirectory( smpldirpath.toString() ) )
                    throw runtime_error( "ExportPresetBank(): Couldn't create sample directory ! " + smpldirpath.toString() );
                smpldir = smpldirpath.toString();
            }
            else
                smpldir = directory;

            for( size_t cntsmpl = 0; cntsmpl < nbslots; ++cntsmpl )
            {
                auto * ptrinfo = smplptr->sampleInfo( cntsmpl );
                auto * ptrdata = smplptr->sample    ( cntsmpl );

                if( ptrinfo != nullptr && ptrdata != nullptr )
                {
                    wave::smpl_chunk_content             smplchnk;
                    stringstream                         sstrname;
                    wave::smpl_chunk_content::SampleLoop loopinfo;
                    DSESampleConvertionInfo              cvinf;

                    //Set the loop data
                    smplchnk.MIDIUnityNote_ = ptrinfo->rootkey;        //#FIXME: Most of the time, the correct root note is only stored in the preset, not the sample data!
                    smplchnk.samplePeriod_  = (1 / ptrinfo->smplrate);

                    if( hexanumbers )
                        sstrname <<"0x" <<hex <<uppercase << cntsmpl <<nouppercase;
                    else
                        sstrname <<right <<setw(4) <<setfill('0') << cntsmpl;

                    if( ptrinfo->smplfmt == eDSESmplFmt::ima_adpcm && noconvert )
                    {
                        //wave::IMAADPCMWaveFile outwave;

                        ////We only have mono samples
                        //outwave.GetSamples().resize(1);
                        //outwave.SampleRate( ptrinfo->smplrate );

                        //std::copy( std::begin(*ptrdata), std::end(*ptrdata), std::back_inserter(outwave.GetSamples().front()) );
                        cvinf.loopbeg_ = (cvinf.loopbeg_ - SizeADPCMPreambleWords) * 8; //loopbeg is counted in int32, for APCM data, so multiply by 8 to get the loop beg as pcm16. Subtract one, because of the preamble.
                        cvinf.loopend_ = ::audio::ADPCMSzToPCM16Sz( ptrdata->size() );

                        sstrname <<"_adpcm";

                        loopinfo.start_ = cvinf.loopbeg_;
                        loopinfo.end_   = cvinf.loopend_;

                        ////Add loopinfo!
                        //smplchnk.loops_.push_back( loopinfo );
                        //outwave.AddRIFFChunk( smplchnk );
                        sstrname<<".adpcm";
                        //outwave.WriteWaveFile( Poco::Path(smpldir).append(sstrname.str()).makeFile().absolute().toString() );
                        audio::DumpADPCM( Poco::Path(smpldir).append(sstrname.str()).makeFile().absolute().toString(), *ptrdata );
                    }
                    else if( ptrinfo->smplfmt == eDSESmplFmt::pcm8 && noconvert )
                    {
                        wave::PCM8WaveFile outwave;

                        //We only have mono samples
                        outwave.GetSamples().resize(1);
                        outwave.SampleRate( ptrinfo->smplrate );

                        auto backins = std::back_inserter(outwave.GetSamples().front());
                        for( const auto asample : *ptrdata )
                            (*backins) = asample ^ 0x80; //Flip the first bit, to turn from 2's complement to offset binary(excess-K)

                        sstrname <<"_pcm8";
                        loopinfo.start_ = cvinf.loopbeg_;
                        loopinfo.end_   = cvinf.loopend_;

                        //Add loopinfo!
                        smplchnk.loops_.push_back( loopinfo );
                        outwave.AddRIFFChunk( smplchnk );
                        sstrname<<".wav";
                        outwave.WriteWaveFile( Poco::Path(smpldir).append(sstrname.str()).makeFile().absolute().toString() );
                    }
                    else
                    {
                        wave::PCM16sWaveFile outwave;

                        //We only have mono samples
                        outwave.GetSamples().resize(1);
                        outwave.SampleRate( ptrinfo->smplrate );

                        switch( ConvertDSESample( static_cast<uint16_t>(ptrinfo->smplfmt), ptrinfo->loopbeg, *ptrdata, cvinf, outwave.GetSamples().front() ) )
                        {
                            case eDSESmplFmt::ima_adpcm:
                            {
                                sstrname <<"_adpcm";
                                break;
                            }
                            case eDSESmplFmt::pcm8:
                            {
                                sstrname <<"_pcm8";
                                break;
                            }
                            case eDSESmplFmt::pcm16:
                            {
                                sstrname <<"_pcm16";
                                break;
                            }
                            case eDSESmplFmt::psg:
                            {
                                clog <<"<!>- Sample# " <<cntsmpl <<" is an unsported PSG sample and was skipped!\n";
                                break;
                            }
                            case eDSESmplFmt::invalid:
                            {
                                clog <<"<!>- Sample# " <<cntsmpl <<" is in an unknown unsported format and was skipped!\n";
                            }
                        }

                        loopinfo.start_ = cvinf.loopbeg_;
                        loopinfo.end_   = cvinf.loopend_;

                        //Add loopinfo!
                        smplchnk.loops_.push_back( loopinfo );
                        outwave.AddRIFFChunk( smplchnk );
                        sstrname<<".wav";
                        outwave.WriteWaveFile( Poco::Path(smpldir).append(sstrname.str()).makeFile().absolute().toString() );
                    }
                    ++nbsamplesexport;
                    cout <<"\r\tExported " <<nbsamplesexport <<" samples!";
                }
                else if( ptrinfo == nullptr || ptrdata == nullptr )
                    clog<<"\n<!>- Sample + Sample info mismatch detected for index " <<cntsmpl <<"!\n";
            }
            cout <<"\n";
        }
        else
            cout << "\tNo samples to export!\n";

        if( !samplesonly )
        {
            //auto prgptr  = bnk.prgmbank().lock();
            //if( prgptr != nullptr )
            //{
                PresetBankToXML( bnk, directory );
            //}
            //else
            //    cout << "<!>- The SWDL contains no preset data to export!\n";
        }
    }


};
