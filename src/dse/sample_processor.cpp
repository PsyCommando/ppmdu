#include <dse/dse_conversion.hpp>
#include <vector>
#include <map>
#include <iostream>
#include <iomanip>
#include <deque>

using namespace std;

namespace DSE
{
    inline uint32_t MsecToNbSamples( uint32_t samplerate, uint32_t durationmsec )
    {
        return static_cast<uint32_t>( lround( 
                                                floor( (durationmsec / 1000.0) * static_cast<double>(samplerate) )
                                            ) );
    }



    /*
        SampleProcessor
            The sample processor upsamples DSE sound samples, and bakes the full envelope into it.

            The release phase is whatever is after the current sample playback position. 
            So essentially, samples that have 2 decay phases will not be looped in the soundfont, while those that have one, will loop, and then play their release phase right after the loop!
    */
    class SampleProcessor
    {
    public:
        SampleProcessor( const SampleBank &srcsmpl, 
                         int               desiredsmplrate  = -1, 
                         bool              bakeenv          = true, 
                         bool              applyfilters     = true, 
                         bool              applyfx          = true )
            :m_srcsmpl(srcsmpl), 
             m_desiredsmplrate(desiredsmplrate), 
             m_bshouldbakeenv(bakeenv), 
             m_bApplyFilters(applyfilters),
             m_bApplyFx(applyfx)
        {}

        /*
            Information on the the prg split to which this sample correspond to!
        */
        struct SrcSampleInf
        {
            int16_t prgmid;
            int16_t splitid;
        };

        /*
            Based on a list of presets using this sample, the same ammount of baked samples will be returned.
        */
        ProcessedPresets Process( const ProgramBank & prestoproc )
        {
            ProcessedPresets processed;
            for( const auto & inf : prestoproc.PrgmInfo() )
            {
                if( inf != nullptr )
                    ProcessAPrgm( *inf, processed );
            }
            return move( processed );
        }


    private:
        void ProcessAPrgm( const DSE::ProgramInfo & prgm, ProcessedPresets & processed )
        {
            ProcessedPresets::PresetEntry entry;
            entry.prginf = prgm;

            for( const auto & split : prgm.m_splitstbl )
            {
                auto psmpl    = m_srcsmpl.sample(split.smplid);
                auto psmplinf = m_srcsmpl.sampleInfo(split.smplid);

                if( psmpl != nullptr && psmplinf != nullptr )
                {
                    //ProcessASplit( entry, split, psmpl, psmplinf );
                    ProcessASplit2( entry, split, prgm.m_lfotbl, psmpl, psmplinf );
                }
            }
            processed.AddEntry( move(entry) );
        }

        inline uint32_t CalcTotalEnveloppeDuration( const DSE::ProgramInfo::SplitEntry  & split )const
        {
            uint32_t envtotaldur =  DSEEnveloppeDurationToMSec( split.attack, split.envmult ) +
                                    DSEEnveloppeDurationToMSec( split.hold,   split.envmult );

            if( split.sustain != 0x7F && split.decay != 0x7F )
                envtotaldur += DSEEnveloppeDurationToMSec( split.decay,  split.envmult ); 

            if( split.decay2 != 0x7F )
                envtotaldur += DSEEnveloppeDurationToMSec( split.decay2, split.envmult ); //Total duration of the envelope!!!
            return envtotaldur;
        }

        /*
        */
        void ProcessASplit2(ProcessedPresets::PresetEntry                       & entry, 
                            const DSE::ProgramInfo::SplitEntry                  & split, 
                            const std::vector<DSE::ProgramInfo::LFOTblEntry>    & lfos,
                            const std::vector<uint8_t>                          * psmpl, 
                            const DSE::WavInfo                                  * psmplinf)
        {
            const size_t    curindex               = entry.splitsamples.size();
            const bool      IsSampleLooped         = psmplinf->smplloop != 0;
            const bool      ShouldUnloop           = ( split.sustain == 0 ) || ( split.decay2 != 0x7F );
            const bool      ShouldRenderEnvAndLoop = !ShouldUnloop && IsSampleLooped;
            const uint32_t  envtotaldur            = CalcTotalEnveloppeDuration(split);
            const uint32_t  envtotaldursmpl        = MsecToNbSamples( psmplinf->smplrate, envtotaldur );

            // --- Convert Sample ----
            DSESampleConvertionInfo postconvloop; //The loop points after conversion
            entry.splitsamples.push_back( move( ConvertSample( *psmpl, psmplinf->smplfmt, psmplinf->loopbeg, postconvloop ) ) );
            entry.splitsmplinf.push_back( *psmplinf ); //Copy sample info #FIXME: maybe get a custom way to store the relevant data for loop points and sample rate instead ?
            const size_t    SampleLenPreLengthen = entry.splitsamples[curindex].size();

            //Update Loop info
            entry.splitsmplinf[curindex].loopbeg  = postconvloop.loopbeg_;
            entry.splitsmplinf[curindex].looplen  = postconvloop.loopend_ - postconvloop.loopbeg_;

            // ---- Handle Enveloppe ----
            if( split.envon != 0 )
            {
                if( IsSampleLooped )
                {
                    //Loop the sample a few times, so its as long as the envelope
                    if( envtotaldursmpl > entry.splitsamples[curindex].size() )
                        Lenghten( entry.splitsamples[curindex], envtotaldursmpl, postconvloop );

                    if( ShouldUnloop )
                    {
                        //We render the envelope and disable looping
                        ApplyEnveloppe( entry.splitsamples[curindex], DSEEnvelope(split), psmplinf->smplrate );
                        entry.splitsmplinf[curindex].smplloop = 0;
                    }
                    else
                    {
                        //We copy one loop to the end, render the envelope, Move the loop to the end past the decay phase, and keep looping on.
                        Lenghten( entry.splitsamples[curindex], 
                                  (entry.splitsamples[curindex].size() + (entry.splitsmplinf[curindex].looplen ) ), 
                                  postconvloop );
                        ApplyEnveloppe( entry.splitsamples[curindex], DSEEnvelope(split), psmplinf->smplrate );

                        //Move the loop to the end
                        entry.splitsmplinf[curindex].loopbeg = (envtotaldursmpl > SampleLenPreLengthen)? envtotaldursmpl : SampleLenPreLengthen;

                        //assert( (entry.splitsmplinf[curindex].loopbeg + entry.splitsmplinf[curindex].looplen) < entry.splitsamples[curindex].size() );
                    }
                }
                else
                {
                    //Render envelope only
                    ApplyEnveloppe( entry.splitsamples[curindex], DSEEnvelope(split), psmplinf->smplrate );
                }

                //Set envelope paramters to disabled, except the release
                entry.prginf.m_splitstbl[curindex].atkvol  = 0x00;
                entry.prginf.m_splitstbl[curindex].attack  = 0x00;
                entry.prginf.m_splitstbl[curindex].hold    = 0x00;
                entry.prginf.m_splitstbl[curindex].decay   = 0x00;
                entry.prginf.m_splitstbl[curindex].decay2  = 0x7F;
                entry.prginf.m_splitstbl[curindex].sustain = 0x7F;
            }

            // ---- Extra Processing ----
            if( ShouldResample() && m_desiredsmplrate != psmplinf->smplrate )
            {
                Resample( entry.splitsamples[curindex], psmplinf->smplrate, m_desiredsmplrate );
                //Update sample rate info
                entry.splitsmplinf[curindex].smplrate = m_desiredsmplrate;
            }
            if( ShouldApplyFilters() )
            {
                ApplyFilters( entry.splitsamples[curindex], entry.splitsmplinf[curindex].smplrate );
            }
            if( ShouldApplyFx() )
            {
                ApplyFx( entry.splitsamples[curindex], entry.splitsmplinf[curindex].smplrate, lfos );
            }
        }


        /*
        */
        //void ProcessASplit( ProcessedPresets::PresetEntry       & entry, 
        //                   const DSE::ProgramInfo::SplitEntry   & split, 
        //                   const std::vector<uint8_t>           * psmpl, 
        //                   const DSE::WavInfo                   * psmplinf )
        //{
        //    const size_t curindex = entry.splitsamples.size();

        //    //1. Convert sample to pcm16
        //    DSESampleConvertionInfo newloop; //The loop points after conversion
        //    entry.splitsamples.push_back( move( ConvertSample( *psmpl, psmplinf->smplfmt, psmplinf->loopbeg, newloop ) ) );
        //    entry.splitsmplinf.push_back( *psmplinf ); //Copy sample info #FIXME: maybe get a custom way to store the relevant data for loop points and sample rate instead ?

        //    const bool doesFadeOut  = ( split.decay != 0x7F && split.sustain  == 0 ) || ( split.decay2 != 0x7F );
        //    const bool ShouldUnloop = doesFadeOut;/*split.decay2 != 0 || split.decay2 != 0x7F;*/ //If we got a fade-out, we just loop the sample manually!
        //    const bool IsLooped     = entry.splitsmplinf[curindex].smplloop != 0;
        //    //const bool   loopedenv   = split.decay2 == 0 || split.decay2 == 0x7F; //If the sample doesn't fade out, it is looped by the sampler. Otherwise, we bake the full duration of the envelope into the sample.
        //    const size_t origsmpllen = entry.splitsamples[curindex].size(); //The length in sample points of the original sample data

        //    if( split.envon == 0 )
        //        return; //no envelope processing!

        //    uint32_t envtotaldur = 0;

        //    //Check if envelope looped or not
        //    //if( loopedenv ) 
        //    //{
        //    //    //In looped samples, we just want to bake the attack, hold, decay, and leave the sustain phase as loop
        //    //    envtotaldur =   DSEEnveloppeDurationToMSec( split.attack, split.envmult ) +
        //    //                    DSEEnveloppeDurationToMSec( split.hold,   split.envmult );//Total duration of the envelope!!!

        //    //    if( split.sustain != 0x7F && split.decay != 0x7F )
        //    //        envtotaldur += DSEEnveloppeDurationToMSec( split.decay,  split.envmult ); 
        //    //}
        //    //else
        //    //{
        //        //Non-looped samples have the extra decay2 param baked into them
        //        envtotaldur =   DSEEnveloppeDurationToMSec( split.attack, split.envmult ) +
        //                        DSEEnveloppeDurationToMSec( split.hold,   split.envmult );

        //        if( split.sustain != 0x7F && split.decay != 0x7F )
        //            envtotaldur += DSEEnveloppeDurationToMSec( split.decay,  split.envmult ); 

        //        if( split.decay2 != 0x7F )
        //            envtotaldur += DSEEnveloppeDurationToMSec( split.decay2, split.envmult ); //Total duration of the envelope!!!
        //    //}


        //    // ---- Process our enveloppe ---- 
        //    uint32_t envtotaldursmpl = MsecToNbSamples( psmplinf->smplrate, envtotaldur );//static_cast<uint32_t>( lround( ceil( ( (envtotaldur / 1000.0) * static_cast<double>(psmplinf->smplrate) ) ) ) ); //Result is a number of samples. The total duration in samples.

        //    if( /*loopedenv*/ !ShouldUnloop && IsLooped )
        //        envtotaldursmpl += (newloop.loopend_ - newloop.loopbeg_); //Add another extra loop so that the sample can be looped at a sustained volume!
        //    
        //    //We need to calculate the duration of the sample without looping
        //    // If its shorter than the enveloppe, we loop until the total duration is reached.
        //    const size_t newsmpllen = ( origsmpllen >= envtotaldursmpl )? origsmpllen : envtotaldursmpl; //in sample points

        //    //2. Lengthen the sound sample, or not, depending on the volume enveloppe
        //    if( envtotaldursmpl > origsmpllen && IsLooped )
        //    {
        //        Lenghten( entry.splitsamples[curindex], newsmpllen, newloop );
        //    }
        //    else if( envtotaldursmpl > origsmpllen && !IsLooped  )
        //    {
        //    }
        //    else if( origsmpllen >= envtotaldursmpl && (split.decay2 != 0x7F || split.sustain == 0) ) //If the envelope is shorter than the sample. 
        //    {
        //    }

        //    //3. Apply Volume enveloppe
        //    ApplyEnveloppe( entry.splitsamples[curindex], DSEEnvelope(split), psmplinf->smplrate );

        //    entry.splitsmplinf[curindex].smplloop = IsLooped && !ShouldUnloop;
        //    entry.splitsmplinf[curindex].loopbeg  = newloop.loopbeg_;
        //    entry.splitsmplinf[curindex].looplen  = (newloop.loopend_ - newloop.loopbeg_);

        //    // ---- Update our sample info ----
        //    //if( ShouldUnloop && IsLooped /*(entry.splitsmplinf[curindex].smplloop)*/ )
        //    //{
        //    //    //Disable looping on this sample! As the loop is already rendered into the sample
        //    //    entry.splitsmplinf[curindex].smplloop = 0;
        //    //    entry.splitsmplinf[curindex].loopbeg  = 0;
        //    //    entry.splitsmplinf[curindex].looplen  = newloop.loopend_;
        //    //}
        //    //else if( !ShouldUnloop )
        //    //{
        //    //    entry.splitsmplinf[curindex].loopbeg  = newloop.loopbeg_;
        //    //    entry.splitsmplinf[curindex].looplen  = (newloop.loopend_ - newloop.loopbeg_);
        //    //}
        //    ///else //if( /*!loopedenv*/ ShouldUnloop )

        //    //else
        //    //{
        //    //    //Update sample info
        //    //    entry.splitsmplinf[curindex].smplloop = 1;
        //    //    entry.splitsmplinf[curindex].loopbeg  = newloop.loopbeg_;
        //    //    entry.splitsmplinf[curindex].looplen  = (newloop.loopend_ - newloop.loopbeg_);
        //    //}
        //    entry.splitsmplinf[curindex].smplfmt = static_cast<uint16_t>(WavInfo::eSmplFmt::pcm16);

        //    //4. Resample
        //    if( m_desiredsmplrate != -1 && m_desiredsmplrate != psmplinf->smplrate )
        //    {
        //        Resample( entry.splitsamples[curindex], psmplinf->smplrate, m_desiredsmplrate );
        //        //Update sample rate info
        //        entry.splitsmplinf[curindex].smplrate = m_desiredsmplrate;
        //    }

        //    //Set envelope paramters to disabled, except the release
        //    entry.prginf.m_splitstbl[curindex].atkvol  = 0x00;
        //    entry.prginf.m_splitstbl[curindex].attack  = 0x00;
        //    entry.prginf.m_splitstbl[curindex].hold    = 0x00;
        //    entry.prginf.m_splitstbl[curindex].decay   = 0x00;
        //    entry.prginf.m_splitstbl[curindex].decay2  = 0x7F;
        //    entry.prginf.m_splitstbl[curindex].sustain = 0x7F;
        //}

        /*
        */
        vector<int16_t> ConvertSample( const vector<uint8_t> & srcsmpl, int16_t smplty, size_t origloopbeg, DSESampleConvertionInfo & newloop )
        {
            vector<int16_t> result;
            //Depending on sample format, convert to pcm16 !
            ConvertDSESample( smplty, origloopbeg, srcsmpl, newloop, result );
            return std::move(result);
        }

        /*
            Make the sample the length of the entire envelope.
        */
        void Lenghten( vector<int16_t> & smpl, size_t destlen, const DSESampleConvertionInfo & loopinf )
        {
            std::deque<size_t> loopjunctions;

            //Pre-alloc
            smpl.reserve( destlen );

            //Push the first loop junction point 
            loopjunctions.push_back( smpl.size() );

            //Copy samples from the loop, to the end of the entire sample, until we reach the desired amount of samples
            size_t cntsrcsmpl = loopinf.loopbeg_;
            for( size_t cntsmpltocpy = smpl.size(); cntsmpltocpy < destlen; ++cntsmpltocpy )
            {
                //Append sample to the end
                smpl.push_back( smpl[cntsrcsmpl] );

                //Loop our source when we get to the end of the loop 
                if( cntsrcsmpl < loopinf.loopend_ )
                    ++cntsrcsmpl;
                else
                {
                    //Mark loop pos, and to later smooth them
                    loopjunctions.push_back( smpl.size() );
                    cntsrcsmpl = loopinf.loopbeg_;
                }
            }

            //Smooth the loop points #TODO
            //for( const auto & loopjunc : loopjunctions )
            //{
            //    if( loopjunc > 8 && (loopjunc < smpl.size() - 8) ) //We need 8 samples before and after to properly smooth things out
            //    {
            //    }
            //}
        }

        /*
            Resample the sample.
        */
        void Resample( vector<int16_t> & smpl, int origsamplrte, int destsamplrte )
        {
            assert(false);
        }

        /*
            Apply the envelope phases over time on the sample
        */
        void ApplyEnveloppe( vector<int16_t> & smpl, const DSE::DSEEnvelope & env, int smplrate )
        {
            double lastvolumelvl = 1.0;

            //Attack
            const int atknbsmpls = MsecToNbSamples( smplrate, DSEEnveloppeDurationToMSec( env.attack, env.envmulti ) );
            const double atklvl   = ( ( env.atkvol * 100.0 ) / 128.0 ) / 100.0;
                //static_cast<int>( ceil( ( DSEEnveloppeDurationToMSec( env.attack, env.envmulti ) * (smplrate * 1000.0) ) / 1000.0 ) );
            if( env.attack != 0 )
                LerpVol( 0, atknbsmpls, atklvl, 1.0, smpl );

            //Hold
            const int holdbeg     = atknbsmpls;
            const int holdnbsmpls = MsecToNbSamples( smplrate, DSEEnveloppeDurationToMSec( env.hold, env.envmulti ) );
                //static_cast<int>( ceil( ( DSEEnveloppeDurationToMSec( env.hold, env.envmulti ) * (smplrate * 1000.0) ) / 1000.0 ) );
            const int holdend     = holdbeg + holdnbsmpls;

            //Decay
            const int    decaybeg     = holdend;
            int          decaynbsmpls = 0; 
            const double sustainlvl   = ( ( env.sustain * 100.0 ) / 128.0 ) / 100.0;
            if( env.decay != 0x7F )
            {
                decaynbsmpls += MsecToNbSamples( smplrate, DSEEnveloppeDurationToMSec( env.decay, env.envmulti ) );
                    //static_cast<int>( ceil( ( DSEEnveloppeDurationToMSec( env.decay, env.envmulti ) * (smplrate * 1000.0) ) / 1000.0 ) );
                LerpVol( decaybeg, decaybeg + decaynbsmpls, 1.0, sustainlvl, smpl );
                lastvolumelvl = sustainlvl;
            }
            
            if( /*env.decay2 != 0 &&*/ env.decay2 != 0x7F )
            {
                const int decay2beg     = decaybeg + decaynbsmpls;
                const int decay2nbsmpls = MsecToNbSamples( smplrate, DSEEnveloppeDurationToMSec( env.decay2, env.envmulti ) );
                    //static_cast<int>( ceil( ( DSEEnveloppeDurationToMSec( env.decay2, env.envmulti ) * (smplrate * 1000.0) ) / 1000.0 ) );
                LerpVol( decay2beg, decay2beg + decay2nbsmpls, lastvolumelvl, 0.0, smpl );
            }
            else
            {
                //Handle sustain volume for the loop
                LerpVol( decaybeg + decaynbsmpls, smpl.size(), lastvolumelvl, lastvolumelvl, smpl );
            }

            //Release is left to the sampler to process
        }

        void LerpVol( size_t begsmpl, size_t endsmpl, double initvol, double destvol, vector<int16_t> & smpl )
        {
            if( initvol != destvol )
            {
                //Lerp the volume between the two values
                size_t dist        = endsmpl - begsmpl;
                double rate        = (destvol - initvol) / dist;
                double curvolratio = initvol;

                for( size_t cntlerp = begsmpl; cntlerp < endsmpl && cntlerp < smpl.size(); ++cntlerp )
                {
                    double scaledsmpl = lround( smpl[cntlerp] * curvolratio );

                    //Clamp
                    if( scaledsmpl > SHRT_MAX )
                        scaledsmpl = SHRT_MAX;
                    else if( scaledsmpl < SHRT_MIN )
                        scaledsmpl = SHRT_MIN;

                    smpl[cntlerp] = static_cast<int16_t>( scaledsmpl );
                    curvolratio  += rate;
                }
            }
            else
            {
                //If there's no differences, just set everything to the same volume
                for( size_t cntlerp = begsmpl; cntlerp < endsmpl; ++cntlerp )
                {
                    double scaledsmpl = lround( smpl[cntlerp] * destvol );

                    //Clamp
                    if( scaledsmpl > SHRT_MAX )
                        scaledsmpl = SHRT_MAX;
                    else if( scaledsmpl < SHRT_MIN )
                        scaledsmpl = SHRT_MIN;

                    smpl[cntlerp] = static_cast<int16_t>( scaledsmpl );
                }
            }
        }

        /*
        */
        void ApplyFilters( vector<int16_t> & smpl, int smplrate )
        {
            //#TODO: Add smoothing filters
        }

        /*
        */
        void ApplyFx( vector<int16_t> & smpl, int smplrate, const std::vector<DSE::ProgramInfo::LFOTblEntry> & lfos )
        {
            //#TODO: Add LFO processing
        }


    private:

        inline bool ShouldResample()const
        {
            return ( m_desiredsmplrate != -1 );
        }

        inline bool ShouldApplyFilters()const
        {
            return m_bApplyFilters;
        }

        inline bool ShouldApplyFx()const
        {
            return m_bApplyFx;
        }

    private:
        const SampleBank & m_srcsmpl;
        int                m_desiredsmplrate;
        bool               m_bshouldbakeenv;
        bool               m_bApplyFilters;
        bool               m_bApplyFx;
    };



//=========================================================================================
//  Functions
//=========================================================================================

    DSE::ProcessedPresets ProcessDSESamples( const DSE::SampleBank &srcsmpl, const DSE::ProgramBank & prestoproc, int desiredsmplrate, bool bakeenv )
    {
        return move( SampleProcessor( srcsmpl, desiredsmplrate, bakeenv ).Process(prestoproc) );
    }

};