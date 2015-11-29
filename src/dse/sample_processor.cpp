#include <dse/dse_conversion.hpp>
#include <vector>
#include <map>
#include <iostream>
#include <iomanip>
#include <deque>
//#include <CDSPResampler.h>

using namespace std;

namespace DSE
{
    inline uint32_t MsecToNbSamples( uint32_t samplerate, uint32_t durationmsec )
    {
        return static_cast<uint32_t>( lround( 
                                                floor( (durationmsec / 1000.0) * static_cast<double>(samplerate) )
                                            ) );
    }

    inline uint32_t NbSamplesToMsec( uint32_t samplerate, uint32_t nbsamples )
    {
        return static_cast<uint32_t>( lround( 
                                                floor( ( nbsamples / static_cast<double>(samplerate) ) * 1000.0 )
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
                {
                    ProcessAPrgm( *inf, processed );
                }
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
                    ProcessASplit2( entry, split, prgm.m_lfotbl, psmpl, psmplinf, prgm.m_hdr );
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
                            const DSE::WavInfo                                  * psmplinf,
                            const DSE::ProgramInfo::InstInfoHeader              & prgminf )
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

            entry.splitsmplinf[curindex].smplfmt = static_cast<uint16_t>(WavInfo::eSmplFmt::pcm16);

            //Update Loop info
            entry.splitsmplinf[curindex].loopbeg  = postconvloop.loopbeg_;
            entry.splitsmplinf[curindex].looplen  = postconvloop.loopend_ - postconvloop.loopbeg_;

            // ---- Handle Enveloppe ----
            if( split.envon != 0 )
            {
                double volumeFactor = ((prgminf.insvol * 100 / 127) / 100.0) * ((split.smplvol * 100 / 127) / 100.0);

                if( IsSampleLooped )
                {
                    //Loop the sample a few times, so its as long as the envelope
                    if( envtotaldursmpl > entry.splitsamples[curindex].size() )
                        Lenghten( entry.splitsamples[curindex], envtotaldursmpl, postconvloop );

                    if( ShouldUnloop )
                    {
                        //We render the envelope and disable looping
                        ApplyEnveloppe( entry.splitsamples[curindex], DSEEnvelope(split), psmplinf->smplrate, volumeFactor );
                        entry.splitsmplinf[curindex].smplloop = 0;
                    }
                    else
                    {
                        //We copy one loop to the end, render the envelope, Move the loop to the end past the decay phase, and keep looping on.
                        Lenghten( entry.splitsamples[curindex], 
                                  (entry.splitsamples[curindex].size() + (entry.splitsmplinf[curindex].looplen ) ), 
                                  postconvloop );
                        ApplyEnveloppe( entry.splitsamples[curindex], DSEEnvelope(split), psmplinf->smplrate, volumeFactor );

                        //Move the loop to the end
                        entry.splitsmplinf[curindex].loopbeg = (envtotaldursmpl > SampleLenPreLengthen)? envtotaldursmpl : SampleLenPreLengthen;
                    }
                }
                else
                {
                    //Render envelope only
                    ApplyEnveloppe( entry.splitsamples[curindex], DSEEnvelope(split), psmplinf->smplrate, volumeFactor );
                }

                //Set envelope paramters to disabled, except the release
                entry.prginf.m_splitstbl[curindex].atkvol  = 0x00;
                entry.prginf.m_splitstbl[curindex].attack  = 0x00;
                entry.prginf.m_splitstbl[curindex].hold    = 0x00;
                entry.prginf.m_splitstbl[curindex].decay   = 0x00;
                entry.prginf.m_splitstbl[curindex].decay2  = 0x7F;
                entry.prginf.m_splitstbl[curindex].sustain = 0x7F;
            }
            else
            {
                clog << "\nSampleID : " <<psmplinf->id <<" has its envelope disabled!\n";
            }

            // ---- Extra Processing ----
            if( ShouldResample() && m_desiredsmplrate != psmplinf->smplrate )
            {
                const size_t smplszbefresample = entry.splitsamples[curindex].size();
                DSESampleConvertionInfo postresampleloop;
                postresampleloop.loopbeg_ = entry.splitsmplinf[curindex].loopbeg;
                postresampleloop.loopend_ = (entry.splitsmplinf[curindex].loopbeg + entry.splitsmplinf[curindex].looplen);

                if( Resample( entry.splitsamples[curindex], psmplinf->smplrate, m_desiredsmplrate, postresampleloop ) )
                {
                    //Update loop points
                    entry.splitsmplinf[curindex].loopbeg = postresampleloop.loopbeg_;

                    if( postresampleloop.loopend_ > entry.splitsamples[curindex].size() )
                        postresampleloop.loopend_ = entry.splitsamples[curindex].size();

                    entry.splitsmplinf[curindex].looplen = (postresampleloop.loopend_ - postresampleloop.loopbeg_);

                    const size_t szafterfix = ( entry.splitsmplinf[curindex].looplen + entry.splitsmplinf[curindex].loopbeg );

#ifdef DEBUG
                    assert( ( szafterfix < entry.splitsamples[curindex].size() ) );
#endif

                    //Update sample rate info
                    entry.splitsmplinf[curindex].smplrate = m_desiredsmplrate;
                }
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
            //Pre-alloc
            smpl.reserve( destlen );

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
                    cntsrcsmpl = loopinf.loopbeg_;
            }

            //Smooth the loop points #TODO
        }

        /*
            Apply the envelope phases over time on the sample
        */
        void ApplyEnveloppe( vector<int16_t> & smpl, const DSE::DSEEnvelope & env, int smplrate, double volmul )
        {
            const double MaxVol        = volmul * 1.0;
            double       lastvolumelvl = MaxVol;

            //Attack
            const int atknbsmpls = MsecToNbSamples( smplrate, DSEEnveloppeDurationToMSec( env.attack, env.envmulti ) );
            const double atklvl   = (( ( env.atkvol * 100.0 ) / 128.0 ) / 100.0) * volmul;
            if( env.attack != 0 )
                LerpVol( 0, atknbsmpls, atklvl, MaxVol, smpl );

            //Hold
            const int holdbeg     = atknbsmpls;
            const int holdnbsmpls = MsecToNbSamples( smplrate, DSEEnveloppeDurationToMSec( env.hold, env.envmulti ) );
            const int holdend     = holdbeg + holdnbsmpls;

            //Decay
            const int    decaybeg     = holdend;
            int          decaynbsmpls = 0; 
            const double sustainlvl   = (( ( env.sustain * 100.0 ) / 128.0 ) / 100.0) * volmul;
            if( env.decay != 0x7F && !(env.decay == 0 && sustainlvl == 0) )
            {
                decaynbsmpls += MsecToNbSamples( smplrate, DSEEnveloppeDurationToMSec( env.decay, env.envmulti ) );
                LerpVol( decaybeg, decaybeg + decaynbsmpls, MaxVol, sustainlvl, smpl );
                lastvolumelvl = sustainlvl;
            }
            
            if( env.decay2 != 0x7F )
            {
                const int decay2beg     = decaybeg + decaynbsmpls;
                const int decay2nbsmpls = MsecToNbSamples( smplrate, DSEEnveloppeDurationToMSec( env.decay2, env.envmulti ) );
                LerpVol( decay2beg, smpl.size(), lastvolumelvl, 0.0, smpl );
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
            Resample the sample.
        */
        bool Resample( vector<int16_t> & smpl, int origsamplrte, int destsamplrte, DSESampleConvertionInfo & inout_newloop )
        { return false; }
    //        r8b::CDSPResampler16 resampler( origsamplrte, destsamplrte, smpl.size() );
    //        vector<double>       tempbuf;
    //        tempbuf.reserve( smpl.size() );

    //        for( const auto sp : smpl )
    //            tempbuf.push_back( sp );

    //        double *ptrout = nullptr;
    //        int     szresampled = 0;
    //        
    //        //#FIXME : Ugh, so I can't find any info on how to preserve loop points after resampling for some reasons(thanks google) 
    //        //          so, I'll do this by doing some extra useless work here.
    //        if( inout_newloop.loopbeg_ != 0 )
    //        {
    //            const uint32_t oldloopbeg = inout_newloop.loopbeg_;
    //            const uint32_t oldlooplen = (inout_newloop.loopend_ - inout_newloop.loopbeg_);
    //            vector<int16_t> asmsmplbuff;
    //            auto            backins = std::back_inserter(asmsmplbuff);

    //            //Loop beg
    //            szresampled = resampler.process( tempbuf.data(), inout_newloop.loopbeg_, ptrout );
    //            if( szresampled > 0 && ptrout != nullptr )
    //            {
    //                inout_newloop.loopbeg_ = szresampled;
    //                std::copy_n( ptrout, szresampled, backins );
    //            }
    //            else
    //                return false;

    //            //Loop length
    //            szresampled = resampler.process( (tempbuf.data() + oldloopbeg), 
    //                                             oldlooplen, 
    //                                             ptrout );
    //            if( szresampled > 0 && ptrout != nullptr )
    //            {
    //                inout_newloop.loopend_ = inout_newloop.loopbeg_ + szresampled;
    //                std::copy_n( ptrout, szresampled, backins );
    //            }
    //            else
    //                return false;

    //            //Reset everything 
    //            //ptrout      = nullptr; //The resampler class owns the content pointed to, so we can safely set this to null
    //            //szresampled = 0;

    //            smpl = std::move(asmsmplbuff);
    //            return true;
    //        }
    //        else
    //        {
    //            szresampled = resampler.process( tempbuf.data(), tempbuf.size(), ptrout );

    //            if( szresampled > 0 && ptrout != nullptr )
    //            {
    //                smpl.resize(szresampled);

    //                for( size_t i = 0; i < smpl.size(); ++i )
    //                    smpl[i] = ptrout[i];

    //                return true;
    //            }
    //            else
    //            {
    //                clog << "DSE::SampleProcessor::Resample(): Error, resampling failed! (" <<origsamplrte <<", " <<destsamplrte <<")\n";
    //#ifdef DEBUG
    //                assert(false);
    //#endif
    //                return false;
    //            }
    //        }
    //    }


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