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

            int cntsplit = 0;
            for( const auto & split : prgm.m_splitstbl )
            {
                auto psmpl    = m_srcsmpl.sample(split.smplid);
                auto psmplinf = m_srcsmpl.sampleInfo(split.smplid);

                if (psmpl == nullptr || psmplinf == nullptr)
                    clog << "<!>-DSE::SampleProcessor::ProcessAPrgm(): Warning! The non-existant sample ID " <<split.smplid <<" was referred to in Program#" <<prgm.id <<", split#" <<cntsplit <<". Skipping!\n";

                if( psmpl != nullptr && psmplinf != nullptr )
                {
                    ProcessASplit2( entry, split, prgm.m_lfotbl, psmpl, psmplinf, prgm );
                }
                ++cntsplit;
            }
            processed.AddEntry( move(entry) );
        }

        inline uint32_t CalcTotalEnveloppeDuration( const DSE::SplitEntry  & split )const
        {
            uint32_t envtotaldur =  DSEEnveloppeDurationToMSec( static_cast<int8_t>(split.env.attack), static_cast<int8_t>(split.env.envmulti) ) +
                                    DSEEnveloppeDurationToMSec( static_cast<int8_t>(split.env.hold),   static_cast<int8_t>(split.env.envmulti) );

            if( split.env.sustain != 0x7F && split.env.decay != 0x7F )
                envtotaldur += DSEEnveloppeDurationToMSec( static_cast<int8_t>(split.env.decay),  static_cast<int8_t>(split.env.envmulti) ); 

            if( split.env.decay2 != 0x7F )
                envtotaldur += DSEEnveloppeDurationToMSec( static_cast<int8_t>(split.env.decay2), static_cast<int8_t>(split.env.envmulti) ); //Total duration of the envelope!!!
            return envtotaldur;
        }

        /*
        */
        void ProcessASplit2(ProcessedPresets::PresetEntry           & entry, 
                            const DSE::SplitEntry                   & split, 
                            const std::vector<DSE::LFOTblEntry>     & lfos,
                            const std::vector<uint8_t>              * psmpl, 
                            const DSE::WavInfo                      * psmplinf,
                            const DSE::ProgramInfo                  & prgminf )
        {
            const size_t    curindex               = entry.splitsamples.size(); //!#FIXME: Past me, what the fuck?
            const bool      IsSampleLooped         = psmplinf->smplloop != 0;
            const bool      ShouldUnloop           = ( split.env.sustain == 0 ) || ( split.env.decay2 != 0x7F );
            const bool      ShouldRenderEnvAndLoop = !ShouldUnloop && IsSampleLooped;
            const uint32_t  envtotaldur            = CalcTotalEnveloppeDuration(split);
            const uint32_t  envtotaldursmpl        = MsecToNbSamples( psmplinf->smplrate, envtotaldur );

            // --- Convert Sample ----
            DSESampleConvertionInfo postconvloop; //The loop points after conversion
            
            //entry.splitsamples.push_back( move( ConvertAndLoopSample( *psmpl, static_cast<uint16_t>(psmplinf->smplfmt), psmplinf->loopbeg, psmplinf->looplen, 1, postconvloop ) ) );
            entry.splitsamples.push_back( move( ConvertSample( *psmpl, static_cast<uint16_t>(psmplinf->smplfmt), psmplinf->loopbeg, postconvloop ) ) );
            
            entry.splitsmplinf.push_back( *psmplinf ); //Copy sample info #FIXME: maybe get a custom way to store the relevant data for loop points and sample rate instead ?
            const size_t    SampleLenPreLengthen = entry.splitsamples[curindex].size();

            entry.splitsmplinf[curindex].smplfmt = eDSESmplFmt::pcm16;

            //Update Loop info
            entry.splitsmplinf[curindex].loopbeg  = postconvloop.loopbeg_;
            entry.splitsmplinf[curindex].looplen  = postconvloop.loopend_ - postconvloop.loopbeg_;

            // ---- Handle Enveloppe ----
            if( split.envon != 0 )
            {
                double volumeFactor = ( static_cast<int>(prgminf.prgvol * 100 / 127) / 100.0) * ( static_cast<int>(split.smplvol * 100 / 127) / 100.0);

                if( IsSampleLooped )
                {
                    if( psmplinf->smplfmt != eDSESmplFmt::pcm16 ) //!#FIXME: This is a temporary fix for the pitch issue with pcm16 samples. It makes some samples not loop, and it doesn't fix all samples however..
                    {
                    if( ShouldUnloop )
                    {
                        //Loop the sample a few times, so its as long as the envelope
                        if( envtotaldursmpl > entry.splitsamples[curindex].size() )
                            Lenghten( entry.splitsamples[curindex], envtotaldursmpl, postconvloop );

            ////!###TEST####
            //const bool bispcm16                   = (psmplinf->smplfmt == eDSESmplFmt::pcm16);
            //uint32_t   presmplrate                = psmplinf->smplrate;
            //uint32_t   Newsmplrate                = std::lround((static_cast<double>(entry.splitsamples[curindex].size()) / static_cast<double>(SampleLenPreLengthen)) / 100.0) * presmplrate;
            //if( bispcm16 )
            //    entry.splitsmplinf[curindex].smplrate = Newsmplrate;
            ////!###TEST####

                        //We render the envelope and disable looping
                        ApplyEnveloppe( entry.splitsamples[curindex], split.env, psmplinf->smplrate, volumeFactor );
                        entry.splitsmplinf[curindex].smplloop = 0;
                    }
                    else
                    {
                        //Loop the sample a few times, so its as long as the envelope
                        if( envtotaldursmpl > entry.splitsamples[curindex].size() )
                        {
                            int          nbextraloops = 0;
                            const size_t durtoloop    = envtotaldursmpl - SampleLenPreLengthen; //entry.splitsmplinf[curindex].loopbeg;
                            //const size_t resultinglength  = envtotaldursmpl + ( durofunloopedenv % entry.splitsmplinf[curindex].looplen );
                            
                            if( ( durtoloop % entry.splitsmplinf[curindex].looplen ) != 0 )
                                nbextraloops = (durtoloop / entry.splitsmplinf[curindex].looplen) + 1;
                            else
                                nbextraloops = (durtoloop / entry.splitsmplinf[curindex].looplen);

                            LenghtenByNbLoops( entry.splitsamples[curindex], nbextraloops, postconvloop );

                            //Make sure the sample ends only after fully completing its last loop, this will keep 
                            // the sample from clicking/abruptly cutting to the loop.
                        }

                        //Save the length of the sample after making it longer, since it differ from "envtotaldursmpl"
                        const size_t actualnewloopbeg = entry.splitsamples[curindex].size();

                        //We copy one loop to the end, render the envelope, Move the loop to the end past the decay phase, and keep looping on.
                        LenghtenByNbLoops( entry.splitsamples[curindex], 1, postconvloop );
                        ApplyEnveloppe( entry.splitsamples[curindex], split.env, psmplinf->smplrate, volumeFactor );


                        ////!###Test release###
                        //size_t    releasebeg     = entry.splitsamples[curindex].size();
                        //size_t    nbreleaselp    = 0;
                        //const int releasenbsmpls = MsecToNbSamples( psmplinf->smplrate,
                        //                                            DSEEnveloppeDurationToMSec( static_cast<int8_t>(split.env.release), 
                        //                                                                        static_cast<int8_t>(split.env.envmulti)));
                        //if( ( releasenbsmpls % entry.splitsmplinf[curindex].looplen ) != 0 )
                        //    nbreleaselp = (releasenbsmpls / entry.splitsmplinf[curindex].looplen) + 1;
                        //else
                        //    nbreleaselp = (releasenbsmpls / entry.splitsmplinf[curindex].looplen);
                        //
                        //LenghtenByNbLoops( entry.splitsamples[curindex], nbreleaselp, postconvloop );

                        //const double sustainlvl   = ( ( (static_cast<double>(split.env.sustain) * 100.0 ) / 128.0 ) / 100.0) * split.env.envmulti;
                        //if( split.env.decay2 == 0x7F )
                        //    LerpVol( releasebeg, releasenbsmpls, sustainlvl, 0.0, entry.splitsamples[curindex] ); 
                        ////!###Test release###


            ////!###TEST####
            //const bool bispcm16                   = (psmplinf->smplfmt == eDSESmplFmt::pcm16);
            //uint32_t   presmplrate                = psmplinf->smplrate;
            //double     oldratio                   = static_cast<double>(SampleLenPreLengthen)                / static_cast<double>(presmplrate);
            //double     newratio                   = static_cast<double>(entry.splitsamples[curindex].size()) / static_cast<double>(presmplrate);
            //uint32_t   Newsmplrate                = std::lround(( static_cast<double>(entry.splitsamples[curindex].size())) *  fabs( oldratio ) );
            //if( bispcm16 )
            //    entry.splitsmplinf[curindex].smplrate = Newsmplrate;
            ////!###TEST####

                        //Move the loop to the end
                        entry.splitsmplinf[curindex].loopbeg = (actualnewloopbeg > SampleLenPreLengthen)? actualnewloopbeg : SampleLenPreLengthen;
                    }
                    }
                }
                else
                {
                    //Render envelope only
                    ApplyEnveloppe( entry.splitsamples[curindex], split.env, psmplinf->smplrate, volumeFactor );
                }

                //Set envelope paramters to disabled, except the release, so we don't end up applying the SF2 envelope over the sample!
                entry.prginf.m_splitstbl[curindex].env.atkvol  = 0x00;
                entry.prginf.m_splitstbl[curindex].env.attack  = 0x00;
                entry.prginf.m_splitstbl[curindex].env.hold    = 0x00;
                entry.prginf.m_splitstbl[curindex].env.decay   = 0x00;
                entry.prginf.m_splitstbl[curindex].env.decay2  = 0x7F;
                entry.prginf.m_splitstbl[curindex].env.sustain = 0x7F;
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
        */
        vector<int16_t> ConvertAndLoopSample( const vector<uint8_t> & srcsmpl, int16_t smplty, size_t origloopbeg, size_t origlooplen, size_t nbloops, DSESampleConvertionInfo & newloop )
        {
            vector<int16_t> result;

            //Depending on sample format, convert to pcm16 !
            ConvertAndLoopDSESample( smplty, origloopbeg, origlooplen, nbloops, srcsmpl, newloop, result );

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
            Add the loop to the end of the sample the specified amount of times!
        */
        void LenghtenByNbLoops( vector<int16_t> & smpl, int nbloops, const DSESampleConvertionInfo & loopinf )
        {
            const size_t looplen     = (loopinf.loopend_ - loopinf.loopbeg_);
            const size_t targetlen   = loopinf.loopbeg_ + ( (nbloops+1) * looplen );
            const size_t origsmpllen = smpl.size();

            //Pre-alloc
            smpl.reserve( targetlen );
            
            for( int cntloop = 0; cntloop < nbloops; ++cntloop )
            {
                for( size_t cntsmpls = loopinf.loopbeg_; cntsmpls < origsmpllen; ++cntsmpls )
                    smpl.push_back( smpl[cntsmpls] );
            }
        }
        //{
        //    const size_t looplen   = (loopinf.loopend_ - loopinf.loopbeg_);
        //    //Pre-alloc
        //    vector<int16_t> destsmpl(smpl);
        //    auto            itbackins = back_inserter(destsmpl);
        //    destsmpl.reserve( loopinf.loopbeg_ + ( nbloops * looplen ) );

        //    //Copy samples from the loop, to the end of the entire sample, until we reach the desired amount of samples
        //    size_t cntsrcsmpl = loopinf.loopbeg_;
        //    auto   itloopbeg  = smpl.begin() + cntsrcsmpl;

        //    for( int cntloop = 0; cntloop < nbloops; ++cntloop )
        //    {
        //        std::copy_n( itloopbeg, looplen, itbackins );
        //    }

        //    smpl = move( destsmpl );
        //}

        /*
            Apply the envelope phases over time on the sample
        */
        void ApplyEnveloppe( vector<int16_t> & smpl, const DSE::DSEEnvelope & env, int smplrate, double volmul )
        {
            const double MaxVol        = volmul * 1.0;
            double       lastvolumelvl = MaxVol;

            //Attack
            const int atknbsmpls = MsecToNbSamples( smplrate, DSEEnveloppeDurationToMSec( static_cast<int8_t>(env.attack), 
                                                                                          static_cast<int8_t>(env.envmulti) ) );
            const double atklvl   = (( (static_cast<double>(env.atkvol) * 100.0 ) / 128.0 ) / 100.0) * volmul;
            if( env.attack != 0 )
                LerpVol( 0, atknbsmpls, atklvl, MaxVol, smpl );

            //Hold
            const int holdbeg     = atknbsmpls;
            const int holdnbsmpls = MsecToNbSamples( smplrate, DSEEnveloppeDurationToMSec( static_cast<int8_t>(env.hold), 
                                                                                           static_cast<int8_t>(env.envmulti) ) );
            const int holdend     = holdbeg + holdnbsmpls;

            //Decay
            const int    decaybeg     = holdend;
            int          decaynbsmpls = 0; 
            const double sustainlvl   = ( ( (static_cast<double>(env.sustain) * 100.0 ) / 128.0 ) / 100.0) * volmul;
            if( env.decay != 0x7F && !(env.decay == 0 && sustainlvl == 0) )
            {
                decaynbsmpls += MsecToNbSamples( smplrate, DSEEnveloppeDurationToMSec( static_cast<int8_t>(env.decay), 
                                                                                       static_cast<int8_t>(env.envmulti) ) );
                LerpVol( decaybeg, decaybeg + decaynbsmpls, MaxVol, sustainlvl, smpl );
                lastvolumelvl = sustainlvl;
            }
            
            if( env.decay2 != 0x7F )
            {
                const int decay2beg     = decaybeg + decaynbsmpls;
                const int decay2nbsmpls = MsecToNbSamples( smplrate, DSEEnveloppeDurationToMSec( static_cast<int8_t>(env.decay2), 
                                                                                                 static_cast<int8_t>(env.envmulti) ) );
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
        void ApplyFx( vector<int16_t> & smpl, int smplrate, const std::vector<DSE::LFOTblEntry> & lfos )
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