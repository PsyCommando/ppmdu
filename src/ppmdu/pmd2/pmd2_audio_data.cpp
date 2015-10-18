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
#include <ext_fmts/wav_io.hpp>

using namespace std;
using namespace DSE;

namespace pmd2 { namespace audio
{
    static const uint16_t DSE_InfiniteAttenuation_cB = 1440;//1440; //sf2::SF_GenLimitsInitAttenuation.max_;
    static const uint16_t DSE_LowestAttenuation_cB   =  200;//200; //20dB


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

    /*
        BankAndProgramIDAssigner
            A class that handles the various preset allocation strategies. When converting from SWD to SF2 and etc..

            This is the default strategy for SF2 files. Just assign presets with their original ids. 
            Each pair has its own bank. Past 127 pairs, the bankid resets to 0.
            Used for splitting presets into several SF2s.
    */
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

    /*        
        BankAndProgramIDAssigner
            A class that handles the various preset allocation strategies. When converting from SWD to SF2 and etc..

            This strategy is for using as efficiently as possible a single SF2, filling every single presets and banks.
    */
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

    //#FIXME: MOST LIKELY INNACURATE !
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

    //std::vector<pcm16s_t> DecodeADPCMAndAppendLoopBuff( const std::vector<uint8_t> & adpcm, size_t loopbufpos, size_t loopbuflen )
    //{
    //    std::vector<pcm16s_t> decompressed = std::move(::audio::DecodeADPCM_NDS(adpcm));
    //    decompressed.reserve( decompressed.size() + loopbuflen );
    //    std::copy_n( decompressed.begin() + loopbufpos, loopbuflen, back_inserter(decompressed) );
    //    return std::move(decompressed);
    //}

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
                                       int8_t envon,
                                       int8_t envmult )
    {
        sf2::Envelope volenv;
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

            volenv.attack = sf2::MSecsToTimecents( DSEEnveloppeDurationToMSec( attack, envmult ) );
        }
        else if( utils::LibWide().isLogOn() )
            clog<<"Skipping Attack..\n";

        //Handle Hold
        if( hold != 0 )
        {
            if( utils::LibWide().isLogOn() )
                clog<<"Handling Hold..\n";

            volenv.hold = sf2::MSecsToTimecents( DSEEnveloppeDurationToMSec( hold, envmult ) );
        }
        else if( utils::LibWide().isLogOn() )
            clog<<"Skipping Hold..\n";

        //Handle Sustain
        volenv.sustain = DseVolToSf2Attenuation( sustain ); 

        //Handle Decay
        if( decay != 0 && decay2 != 0 && decay2 != 0x7F ) 
        {
            if( utils::LibWide().isLogOn() )
                clog <<"We got combined decays! decay1-2 : " 
                     <<static_cast<unsigned short>(decay) <<" + " 
                     <<static_cast<unsigned short>(decay2) << " = " 
                     <<static_cast<unsigned short>(decay + decay2) <<" !\n";


            //If decay is set to infinite, we just ignore it!
            if( decay == 0x7F )
                volenv.decay   = sf2::MSecsToTimecents( DSEEnveloppeDurationToMSec( decay2, envmult) );
            else if( sustain == 0 ) //The sustain check is to avoid the case where the first decay phase already should have brought the volume to 0 before the decay2 phase would do anything. 
                volenv.decay   = sf2::MSecsToTimecents( DSEEnveloppeDurationToMSec( decay, envmult) );
            else
                volenv.decay   = sf2::MSecsToTimecents( DSEEnveloppeDurationToMSec( decay, envmult ) + DSEEnveloppeDurationToMSec( decay2, envmult) );
            
            volenv.sustain = DSE_InfiniteAttenuation_cB;
        }
        else if( decay != 0 )
        {
            if( utils::LibWide().isLogOn() )
                clog <<"Handling single decay..\n";

            //We use Decay
            //if( decay != 0x7F )
                volenv.decay = sf2::MSecsToTimecents( DSEEnveloppeDurationToMSec( decay, envmult ) );
            //else
            //    volenv.sustain = 0; //No decay at all
        }
        else 
        {
            if( utils::LibWide().isLogOn() )
                clog<<"Decay was 0, falling back to decay 2..\n";

            if( decay2 != 0x7F )
            {
                //We want to fake the volume going down until complete silence, while the key is still held down 
                volenv.decay   = sf2::MSecsToTimecents( DSEEnveloppeDurationToMSec( decay2, envmult ) );
                volenv.sustain = DSE_InfiniteAttenuation_cB;
            }
            else
                volenv.sustain = 0; //No decay at all
        }

        //Handle Release
        if( rel != 0 )
        {
            if( utils::LibWide().isLogOn() )
                clog <<"Handling Release..\n";

            //if( rel != 0x7F )
                volenv.release = sf2::MSecsToTimecents( DSEEnveloppeDurationToMSec( rel, envmult ) );
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

            //Diagnostic            
            if( attack > 0 && attack < 0x7F && 
               (volenv.attack == sf2::SF_GenLimitsVolEnvAttack.max_ || volenv.attack == sf2::SF_GenLimitsVolEnvAttack.min_) )
            {
                //Something fishy is going on !
                clog << "\tThe attack value " <<static_cast<short>(attack) <<" shouldn't result in the SF2 value " <<volenv.attack  <<" !\n";
                assert(false);
            }

            if( hold > 0 && hold < 0x7F && 
               (volenv.hold == sf2::SF_GenLimitsVolEnvHold.max_ || volenv.hold == sf2::SF_GenLimitsVolEnvHold.min_) )
            {
                //Something fishy is going on !
                clog << "\tThe hold value " <<static_cast<short>(hold) <<" shouldn't result in the SF2 value " <<volenv.hold  <<" !\n";
                assert(false);
            }

            if( decay > 0 && decay < 0x7F && 
               (volenv.decay == sf2::SF_GenLimitsVolEnvDecay.max_ || volenv.decay == sf2::SF_GenLimitsVolEnvDecay.min_) )
            {
                //Something fishy is going on !
                clog << "\tThe decay value " <<static_cast<short>(decay) <<" shouldn't result in the SF2 value " <<volenv.decay  <<" !\n";
                assert(false);
            }

            //if( sustain > 0 && sustain < 0x7F && 
            //   (volenv.sustain == sf2::SF_GenLimitsVolEnvSustain.max_ || volenv.sustain == sf2::SF_GenLimitsVolEnvSustain.min_) )
            //{
            //    //Something fishy is going on !
            //    clog << "\tThe sustain value " <<static_cast<short>(sustain) <<" shouldn't result in the SF2 value " <<volenv.sustain  <<" !\n";
            //    assert(false);
            //}

            if( rel > 0 && rel < 0x7F && 
               (volenv.release == sf2::SF_GenLimitsVolEnvRelease.max_ || volenv.release == sf2::SF_GenLimitsVolEnvRelease.min_) )
            {
                //Something fishy is going on !
                clog << "\tThe release value " <<static_cast<short>(rel) <<" shouldn't result in the SF2 value " <<volenv.release  <<" !\n";
                assert(false);
            }
        }

        return volenv;
    }

//========================================================================================
//  BatchAudioLoader
//========================================================================================

    BatchAudioLoader::BatchAudioLoader( /*const std::string & mbank,*/ bool singleSF2 )
        :/*m_mbankpath(mbank),*/ m_bSingleSF2(singleSF2)
    {}
    
    //void BatchAudioLoader::LoadMasterBank()
    //{
    //    m_master = move( LoadSwdBank( m_mbankpath ) );
    //}

    void BatchAudioLoader::LoadMasterBank( const std::string & mbank )
    {
        m_mbankpath = mbank;
        m_master = move( LoadSwdBank( m_mbankpath ) );
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

    template<class _Strategy>
        void MakeAPresetBankDBForAPair( size_t                           cntpair, 
                                        shared_ptr<ProgramBank>          ptrprginf, 
                                        SMDLPresetConversionInfo       & target,
                                        PresetAllocStrategy<_Strategy> & mystrat )
    {
        const auto & curinstlist = ptrprginf->instinfo();

        if( utils::LibWide().isLogOn() )
            clog << "=== SWDL #" << cntpair <<" ===\n";

        for( size_t cntinst = 0; cntinst < curinstlist.size();  ++cntinst ) //Test all the individual instruments and add them to their slot
        {
            if( curinstlist[cntinst] != nullptr )
            {
                int8_t bankid = 0;
                int8_t presid = 0;

                if( mystrat.ComputeBankAndInstID( cntpair, cntinst, bankid, presid ) )
                    target.AddPresetConvInfo( cntinst, SMDLPresetConversionInfo::PresetConvData( presid, bankid ) ); //set pair nb as bank id
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
    void DSESplitToSf2InstrumentZone( const ProgramInfo::SplitEntry          & dseinst, 
                                           const std::map<uint16_t,size_t>        & smplIdConvTbl, 
                                           sf2::SoundFont                         & sf,
                                           sf2::Instrument                        & inst,
                                           const vector<bool>                     & loopedsmpls,
                                           //uint16_t                                 dsepresetid,
                                           const vector<ProgramInfo::LFOTblEntry> & lfos,
                                           const vector<KeyGroup>                 & keygrps,
                                           const SampleBank                       & smplbnk,
                                           SMDLPresetConversionInfo::PresetConvData & cvdata)
    {
        using namespace sf2;

        if( utils::LibWide().isLogOn() )
            clog <<"\t--- Split#" <<static_cast<unsigned short>(dseinst.id) <<" ---\n";

        //Make a zone for this entry
        ZoneBag myzone;

        //  --- Set the generators ---

        //Key Range
        myzone.SetKeyRange( dseinst.lowkey, dseinst.hikey ); //Key range in first

        //Velocity Range
        myzone.SetVelRange( dseinst.unk14, dseinst.unk47 );

        //## Add new generators below ##

        //Fetch Loop Flag
        auto ptrinf = smplbnk.sampleInfo(dseinst.smplid);
        if( ptrinf != nullptr && ptrinf->smplloop != 0 )
        {
            myzone.SetSmplMode( eSmplMode::loop );
        }

        //Since the soundfont format has no notion of polyphony, we can only cut keygroup that only allow a single voice
        if( dseinst.kgrpid != 0 )
        {
            //if( keygrps.size() > dseinst.kgrpid && keygrps[dseinst.kgrpid].poly == 1 )
            //{
            //    myzone.SetExclusiveClass( dseinst.kgrpid );
            //}
            //else
            //{
                if( dseinst.kgrpid >= keygrps.size() )
                    cvdata.maxpoly = -1;
                else
                    cvdata.maxpoly = keygrps[dseinst.kgrpid].poly; //let the midi converter handle anything else
            //}

        }


        //int8_t ctuneadd = (dseinst.ftune) / 100;
        //int8_t ftunes   = (dseinst.ftune) % 100; // utils::Clamp( dseinst.ftune, sf2::SF_GenLimitsFineTune.min_, sf2::SF_GenLimitsFineTune.max_ ); 

        //#Test set pitch from scratch:
        myzone.SetRootKey(dseinst.rootkey);//( (dseinst.rootkey - (dseinst.ktps + dseinst.ctune) + ctuneadd) );

        //Pitch Correction
        //if( dseinst.ftune != 0 )
            //myzone.SetFineTune( ftunes/*dseinst.ftune*/ );

        //if( dseinst.ctune != DSE::DSEDefaultCoarseTune )
            //myzone.SetCoarseTune( /*( dseinst.ctune + 7 ) +*/ ctuneadd );



        //Volume
        if( dseinst.smplvol != DSE_LimitsVol.def_ )
            myzone.SetInitAtt( DseVolToSf2Attenuation(dseinst.smplvol) );

        //Pan
        if( dseinst.smplpan != DSE_LimitsPan.def_ )
            myzone.SetPan( DsePanToSf2Pan(dseinst.smplpan) );

        //Override rootkey
        //myzone.SetRootKey( dseinst.rootkey );

        //Volume Envelope
        Envelope myenv = RemapDSEVolEnvToSF2( dseinst.atkvol,
                                              dseinst.attack, 
                                              dseinst.hold, 
                                              dseinst.decay,
                                              dseinst.sustain,
                                              dseinst.decay2, 
                                              dseinst.release, 
                                              dseinst.rx,
                                              dseinst.envon,
                                              dseinst.envmult );

        /*
            ### In order to handle the atkvol param correctly, we'll make a copy of the Instrument, 
                with a very short envelope!
        */
        if( dseinst.atkvol != 0 && dseinst.attack != 0 )
        {
            ZoneBag atkvolzone( myzone );

            //Set attenuation to the atkvol value
            atkvolzone.SetInitAtt( DseVolToSf2Attenuation( dseinst.atkvol ) + 
                                   DseVolToSf2Attenuation(dseinst.smplvol) );

            //Set hold to the attack's duration
            Envelope atkvolenv;
            atkvolenv.hold    = myenv.attack;
            atkvolenv.sustain = SF_GenLimitsVolEnvSustain.max_;

            //Leave everything else to default
            //atkvolzone.SetVolEnvelope( atkvolenv );

            myzone.AddGenerator( eSFGen::holdVolEnv,    atkvolenv.hold );
            myzone.AddGenerator( eSFGen::sustainVolEnv, atkvolenv.sustain );
            
            //Sample ID in last
            atkvolzone.SetSampleId( smplIdConvTbl.at(dseinst.smplid) );

            inst.AddZone( std::move(atkvolzone) );
        }

        //Set the envelope
        //myzone.SetVolEnvelope( myenv );

        if( myenv.delay != SHRT_MIN )
            myzone.AddGenerator( eSFGen::delayVolEnv, myenv.delay );

        if( myenv.attack != SHRT_MIN )
            myzone.AddGenerator( eSFGen::attackVolEnv, myenv.attack );

        if( myenv.hold != SHRT_MIN )
            myzone.AddGenerator( eSFGen::holdVolEnv, myenv.hold );

        if( myenv.decay != SHRT_MIN )
            myzone.AddGenerator( eSFGen::decayVolEnv, myenv.decay );

        //if( myenv.sustain != 0 )
            myzone.AddGenerator( eSFGen::sustainVolEnv, myenv.sustain );

        if( myenv.release != SHRT_MIN )
            myzone.AddGenerator( eSFGen::releaseVolEnv, myenv.release );

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
                               const vector<KeyGroup>          & keygrps,
                               const shared_ptr<SampleBank>   && smplbnk,
                               SMDLPresetConversionInfo::PresetConvData  & convinf )
    {
        using namespace sf2;
        Preset pre(presname, convinf.midipres, bankno );

        if( utils::LibWide().isLogOn() )
            clog <<"======================\nHandling " <<presname <<"\n======================\n";

        //#0 - Add a global zone for global preset settings
        {
            ZoneBag global;

            //#1 - Setup Generators
            if( dsePres.m_hdr.insvol != DSE_LimitsVol.def_ )
                global.SetInitAtt( DseVolToSf2Attenuation(dsePres.m_hdr.insvol) );

            // Range of DSE Pan : 0x00 - 0x40 - 0x7F
            if( dsePres.m_hdr.inspan != DSE_LimitsPan.def_ )
                global.SetPan( DsePanToSf2Pan(dsePres.m_hdr.inspan) );
            
            //#2 - Setup LFOs
            unsigned int cntlfo = 0;
            for( const auto & lfo : dsePres.m_lfotbl )
            {
                if( lfo.unk52 != 0 ) //Is the LFO enabled ?
                {
                    if( utils::LibWide().isLogOn() )
                        clog << "\tLFO" <<cntlfo <<" : Target: ";
                    
                    if( lfo.unk26 == 1 ) //Pitch
                    {
                        //The effect on the pitch can be handled this way
                        global.AddGenerator( eSFGen::freqVibLFO,    DSE_LFOFrequencyToCents( lfo.unk28/50 ) ); //Frequency
                        global.AddGenerator( eSFGen::vibLfoToPitch, lfo.unk30/10 ); //Depth
                        global.AddGenerator( eSFGen::delayVibLFO,   MSecsToTimecents( lfo.unk31 ) ); //Delay
                        if( utils::LibWide().isLogOn() )
                            clog << "(1)pitch";
                    }
                    else if( lfo.unk26 == 2 ) //Volume
                    {
                        //The effect on the pitch can be handled this way
                        global.AddGenerator( eSFGen::freqModLFO,     DSE_LFOFrequencyToCents(lfo.unk28/50 ) ); //Frequency
                        global.AddGenerator( eSFGen::modLfoToVolume, (lfo.unk30/10) * -1 ); //Depth
                        global.AddGenerator( eSFGen::delayModLFO,    MSecsToTimecents( lfo.unk31 ) ); //Delay

                        if( utils::LibWide().isLogOn() )
                            clog << "(2)volume";
                    }
                    else if( lfo.unk26 == 3 ) //Pan
                    {
                        //Leave the data for the MIDI exporter, so maybe it can do something about it..
                        convinf.extrafx.push_back( 
                            SMDLPresetConversionInfo::ExtraEffects{ SMDLPresetConversionInfo::eEffectTy::Phaser, lfo.unk28, lfo.unk30, lfo.unk31 } 
                        );

                        //#TODO:
                        //We still need to figure a way to get the LFO involved, and set the oscilation frequency!
                        if( utils::LibWide().isLogOn() )
                            clog << "(3)pan";
                    }
                    else if( lfo.unk26 == 4 )
                    {
                        //Unknown LFO target
                        if( utils::LibWide().isLogOn() )
                            clog << "(4)unknown";
                    }
                    else
                    {
                        if( utils::LibWide().isLogOn() )
                            clog << "(" << static_cast<unsigned short>(lfo.unk26) <<")unknown";
                    }

                    if( utils::LibWide().isLogOn() )
                        clog <<", Frequency: " << static_cast<unsigned short>(lfo.unk28) << " Hz, Depth: " << static_cast<unsigned short>(lfo.unk30) << ", Delay: " <<static_cast<unsigned short>(lfo.unk31) <<" ms\n";
                }
                ++cntlfo;
            }

            //#3 - Add the global zone to the list!
            if( global.GetNbGenerators() > 0 && global.GetNbModulators() > 0 )
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

    void AddSampleToSoundfont( size_t cntsmslot, shared_ptr<SampleBank> & samples, map<uint16_t,size_t> & swdsmplofftosf,  sf2::SoundFont & sf )
    {
        using namespace sf2;

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


    vector<SMDLPresetConversionInfo> BatchAudioLoader::ExportSoundfont_New( const std::string & destf )
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
            const auto & curprginfos = curpair.second.prgmbank().lock()->instinfo();    //Get the current SWD's program list
            const auto & curkgrp     = curpair.second.prgmbank().lock()->keygrps();     //Get the current SWD's keygroup list
            auto         itcvtbl     = curcvinfo.begin();                      //Iterator on the conversion map!
            string       pairname    = "Trk#" + to_string(cntpairs);

            for( size_t prgcnt = 0; /*prgcnt < curcvinfo.size() && */itcvtbl != curcvinfo.end(); ++prgcnt, ++itcvtbl )
            {
                if( curprginfos[itcvtbl->first] != nullptr )
                {
                    stringstream sstrprgname;
                    sstrprgname << pairname << "_prg#" <<showbase <<hex <<curprginfos[itcvtbl->first]->m_hdr.id;
                    DSEPresetToSf2Preset( sstrprgname.str(), //pairname + "_prg#" + to_string(curprginfos[itcvtbl->first]->m_hdr.id), 
                                            itcvtbl->second.midibank,
                                            *(curprginfos[itcvtbl->first]),
                                            swdsmplofftosf,
                                            sf,
                                            instsf2cnt,
                                            loopedsmpls,
                                            curkgrp,
                                            m_master.smplbank().lock(),
                                            itcvtbl->second
                                            /*curcvinfo._convtbl.at(itcvtbl->first)*/ );

                    ++cntsf2presets;
                }
                else
                    assert(false); //This should never happen..
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

    void BatchAudioLoader::ExportSoundfontAndMIDIs( const std::string & destdir, int nbloops )
    {
        //Export the soundfont first

        Poco::Path outsoundfont(destdir);
        outsoundfont.append( outsoundfont.getBaseName() + ".sf2").makeFile();
        cerr<<"<*>- Currently exporting main bank to " <<outsoundfont.toString() <<"\n";

        vector<SMDLPresetConversionInfo> merged = std::move( ExportSoundfont_New( outsoundfont.toString() ) );

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
                                 //merged.filetopreset[i], 
                                 merged[i],
                                 nbloops,
                                 DSE::eMIDIMode::GS );  //This will disable the drum channel, since we don't need it at all!
        }
    }


    /*
        BuildMasterFromPairs
            If no main bank is loaded, and the loaded pairs contain their own samples, 
            build a main bank from those!
    */
    void BatchAudioLoader::BuildMasterFromPairs()
    {
        vector<SampleBank::smpldata_t> smpldata;

        //Iterate through all the pairs and fill up our sample data list !
        for( size_t cntpairs = 0; cntpairs < m_pairs.size(); cntpairs++ )
        {
            auto & presetbank = m_pairs[cntpairs].second;
            auto   ptrsmplbnk = presetbank.smplbank().lock();

            if( ptrsmplbnk != nullptr )
            {
                //Enlarge our vector when needed!
                if( ptrsmplbnk->NbSlots() > smpldata.size() )
                    smpldata.resize( ptrsmplbnk->NbSlots() );

                //Copy the samples over into their matching slot!
                for( size_t cntsmplslot = 0; cntsmplslot < ptrsmplbnk->NbSlots(); ++cntsmplslot )
                {
                    if( ptrsmplbnk->IsDataPresent(cntsmplslot) && 
                        ptrsmplbnk->IsInfoPresent(cntsmplslot) &&
                        !smpldata[cntsmplslot].pdata_ &&
                        !smpldata[cntsmplslot].pinfo_ )
                    {
                        smpldata[cntsmplslot].pinfo_.reset( new DSE::WavInfo(*ptrsmplbnk->sampleInfo(cntsmplslot) ) );
                        smpldata[cntsmplslot].pdata_.reset( new std::vector<uint8_t>(*ptrsmplbnk->sample(cntsmplslot)) );
                    }
                }
            }
        }

        DSE_MetaDataSWDL meta;
        meta.fname = "Main.SWD";
        meta.nbprgislots = 0;
        meta.nbwavislots = smpldata.size();

        m_master = move( PresetBank( move(meta), 
                                     move( unique_ptr<SampleBank>(new SampleBank(move( smpldata ))) ) 
                                   ) );
    }


    /*
        LoadMatchedSMDLSWDLPairs
            This function loads all matched smdl + swdl pairs in one or two different directories
    */
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

    //std::pair< DSE::PresetBank, std::vector<std::pair<DSE::MusicSequence,DSE::PresetBank>> > LoadBankAndSinglePair( const std::string & bank, const std::string & smd, const std::string & swd )
    //{
    //    DSE::PresetBank                                       mbank  = DSE::ParseSWDL( bank );
    //    vector<std::pair<DSE::MusicSequence,DSE::PresetBank>> seqpairs;
    //    Poco::File                                            pathsmd(smd);
    //    Poco::File                                            pathswd(swd);

    //    seqpairs.push_back( make_pair( DSE::ParseSMDL( pathsmd.path() ), DSE::ParseSWDL( pathswd.path() ) ) );

    //    return make_pair( std::move(mbank), std::move(seqpairs) );
    //}

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

    void ExportPresetBank( const std::string & directory, const DSE::PresetBank & bnk, bool samplesonly )
    {
        auto smplptr = bnk.smplbank().lock();
        
        if( smplptr != nullptr )
        {
            size_t       nbsamplesexport = 0;
            const size_t nbslots         = smplptr->NbSlots();

            for( size_t cntsmpl = 0; cntsmpl < nbslots; ++cntsmpl )
            {
                auto * ptrinfo = smplptr->sampleInfo( cntsmpl );
                auto * ptrdata = smplptr->sample    ( cntsmpl );

                if( ptrinfo != nullptr && ptrdata != nullptr )
                {
                    wave::PCM16sWaveFile                 outwave;
                    wave::smpl_chunk_content             smplchnk;
                    stringstream                         sstrname;
                    wave::smpl_chunk_content::SampleLoop loopinfo;

                    //We only have mono samples
                    outwave.GetSamples().resize(1);
                    outwave.SampleRate( ptrinfo->smplrate );

                    //Set the loop data
                    smplchnk.MIDIUnityNote_ = ptrinfo->rootkey;
                    smplchnk.samplePeriod_  = (1 / ptrinfo->smplrate);

                    sstrname << cntsmpl;

                    if( ptrinfo->smplfmt == static_cast<uint16_t>(WavInfo::eSmplFmt::ima_adpcm) )
                    {
                        outwave.GetSamples().front() = move(::audio::DecodeADPCM_NDS( *ptrdata ) );
                        sstrname <<"_adpcm";
                        loopinfo.start_ = (ptrinfo->loopbeg - SizeADPCMPreambleWords) * 8; //loopbeg is counted in int32, for APCM data, so multiply by 8 to get the loop beg as pcm16. Subtract one, because of the preamble.
                        loopinfo.end_   = ::audio::ADPCMSzToPCM16Sz(ptrdata->size() );
                    }
                    else if( ptrinfo->smplfmt == static_cast<uint16_t>(WavInfo::eSmplFmt::pcm8) )
                    {
                        outwave.GetSamples().front() = move( PCM8RawBytesToPCM16Vec(ptrdata) );
                        sstrname <<"_pcm8";
                        loopinfo.start_ = ptrinfo->loopbeg * 4; //loopbeg is counted in int32, for PCM8 data, so multiply by 4 to get the loop beg as pcm16
                        loopinfo.end_   = ptrdata->size() * 2; //PCM8 -> PCM16
                    }
                    else if(ptrinfo->smplfmt == static_cast<uint16_t>(WavInfo::eSmplFmt::pcm16) )
                    {
                        outwave.GetSamples().front() = move( RawBytesToPCM16Vec(ptrdata) );
                        sstrname <<"_pcm16";
                        loopinfo.start_ = ptrinfo->loopbeg * 2; //loopbeg is counted in int32, so multiply by 2 to get the loop beg as pcm16
                        loopinfo.end_   = ptrdata->size() / 2;
                    }
                    else if(ptrinfo->smplfmt == static_cast<uint16_t>(WavInfo::eSmplFmt::psg) )
                    {
                        clog <<"<!>- Sample# " <<cntsmpl <<" is an unsported PSG sample and was skipped!\n";
                    }

                    //Add loopinfo!
                    smplchnk.loops_.push_back( loopinfo );

                    sstrname<<".wav";
                    outwave.WriteWaveFile( Poco::Path(directory).append(sstrname.str()).makeFile().absolute().toString() );
                    ++nbsamplesexport;
                }
                else
                    clog<<"<!>- Sample + Sample info mismatch detected for index " <<cntsmpl <<"!\n";
            }

            cout <<"<*>- Exported " <<nbsamplesexport <<" samples !\n";
        }
        else
            cout << "<!>- The SWDL contains no samples to export!\n";

        if( !samplesonly )
        {
            auto prgptr  = bnk.prgmbank().lock();
            if( prgptr != nullptr )
            {
                cout << "Not implemented!!\n";
                assert(false);
            }
            else
            {
                cout << "<!>- The SWDL contains no preset data to export!\n";
            }
        }
    }


};};