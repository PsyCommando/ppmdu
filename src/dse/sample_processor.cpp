#include <dse/dse_conversion.hpp>
#include <vector>
#include <map>

using namespace std;

namespace DSE
{
    /*
        SampleProcessor
            The sample processor upsamples DSE sound samples, and bakes the full envelope into it.

            The release phase is whatever is after the current sample playback position. 
            So essentially, samples that have 2 decay phases will not be looped in the soundfont, while those that have one, will loop, and then play their release phase right after the loop!
    */
    class SampleProcessor
    {
    public:
        SampleProcessor( const SampleBank &srcsmpl, int desiredsmplrate = -1, bool bakeenv = true )
            :m_srcsmpl(srcsmpl), m_desiredsmplrate(desiredsmplrate), m_bshouldbakeenv(bakeenv)
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
        /*vector< pair<SrcSampleInf, vector<int16_t>> >*/
        ProcessedPresets Process( const ProgramBank & prestoproc )
        {
            //( DSE::DSE_MetaDataSWDL && meta, std::unique_ptr<ProgramBank> && pInstrument, std::unique_ptr<SampleBank>  && pSmpl )
            ProcessedPresets processed;
            //vector<SampleBank::smpldata_t> nsmpls;
            //vector<ProgramBank::ptrprg_t>  nprgs(prestoproc.PrgmInfo().size());

            for( const auto & inf : prestoproc.PrgmInfo() )
            {
                if( inf != nullptr )
                    ProcessAPrgm( *inf, processed );
            }

            /*
            DSE::ProgramInfo                    prginf;       //
            std::vector< DSE::WavInfo >         splitsmplinf; //Modified sample info for a split's sample.
            std::vector< std::vector<int16_t> > splitsamples; //Sample for each split of a preset.
            */
            

            //return move( PresetBank( 
            //                         move( DSE::DSE_MetaDataSWDL() ), 
            //                         move( std::unique_ptr<ProgramBank> ( new ProgramBank( move(nprgs), 
            //                                                                               move(vector<KeyGroup>(prestoproc.Keygrps())) ) ) ),
            //                        move( std::unique_ptr<SampleBank>   (new SampleBank(move(nsmpls))) )
            //            ) );
        }


    private:
        void ProcessAPrgm( const DSE::ProgramInfo & prgm, ProcessedPresets & processed )
        {

            /*
            DSE::ProgramInfo                    prginf;       //
            std::vector< DSE::WavInfo >         splitsmplinf; //Modified sample info for a split's sample.
            std::vector< std::vector<int16_t> > splitsamples; //Sample for each split of a preset.
            */
            ProcessedPresets::PresetEntry entry;
            entry.prginf = prgm;

            for( const auto & split : prgm.m_splitstbl )
            {
                auto psmpl    = m_srcsmpl.sample(split.smplid);
                auto psmplinf = m_srcsmpl.sampleInfo(split.smplid);

                if( psmpl != nullptr && psmplinf != nullptr )
                {
                    const size_t curindex = entry.splitsamples.size();

                    //1. Convert sample to pcm16
                    entry.splitsamples.push_back( move( ConvertSample( *psmpl, psmplinf->smplfmt ) ) );
                    entry.splitsmplinf.push_back( *psmplinf ); //Copy sample info #FIXME: maybe get a custom way to store the relevant data for loop points and sample rate instead ?

                    //Check if envelope looped or not
                    if( split.envon != 0 && split.decay2 != 0 ) //When the secondary decay is not 0, we do not loop!
                    {
                        //Non-looped sample

                        //Calculate total sound length
                        const int32_t totaldur     = DSEEnveloppeDurationToMSec( split.attack, split.envmult ) +
                                                     DSEEnveloppeDurationToMSec( split.hold,   split.envmult ) +
                                                     DSEEnveloppeDurationToMSec( split.decay,  split.envmult ) +
                                                     DSEEnveloppeDurationToMSec( split.decay2, split.envmult );
                        const int32_t totaldursmpl = static_cast<int>( ceil( ( totaldur * (psmplinf->smplrate * 1000.0) ) / 1000.0 ) ); //Result is a number of samples.

                        const size_t  origsmpllen  = entry.splitsamples[curindex].size();
                        //We need to calculate the duration of the sample without looping
                        // If its shorter than the enveloppe, we loop until the total duration is reached.
                        const size_t newsmpllen = ( origsmpllen >= totaldursmpl )? origsmpllen : totaldursmpl; //in samples

                        //2. Lengthen the sound sample, or not, depending on the volume enveloppe
                        if( newsmpllen > origsmpllen )
                            Lenghten( entry.splitsamples[curindex], newsmpllen );

                        //3. Apply Volume enveloppe
                        ApplyEnveloppe( entry.splitsamples[curindex], DSEEnvelope(split) );

                        //Disable looping on this sample!
                        entry.splitsmplinf[curindex].smplloop = 0;
                        entry.splitsmplinf[curindex].loopbeg  = 0;
                        entry.splitsmplinf[curindex].looplen  = 0;
                    }
                    else
                    {
                        //Looped sample
                        assert(false);
                    }

                    //4. Resample
                    if( m_desiredsmplrate != -1 && m_desiredsmplrate != psmplinf->smplrate )
                    {
                        Resample( entry.splitsamples[curindex], m_desiredsmplrate );
                        //Update sample rate info
                        entry.splitsmplinf[curindex].smplrate = m_desiredsmplrate;
                    }
                }
            }
            processed.AddEntry( move(entry) );
        }

        /*
        */
        vector<int16_t> ConvertSample( const vector<uint8_t> & srcsmpl, int16_t smplty )
        {
            //Depending on sample format, convert to pcm16 !
        }

        /*
            Make the sample the length of the entire envelope.
        */
        void Lenghten( vector<int16_t> & smpl, size_t destlen )
        {
        }

        /*
            Resample the sample.
        */
        void Resample( vector<int16_t> & smpl, int samplerte )
        {

        }

        /*
            Apply the envelope phases over time on the sample
        */
        void ApplyEnveloppe( vector<int16_t> & smpl, const DSE::DSEEnvelope & env )
        {
        }

        void LerpVol( size_t begsmpl, size_t endsmpl, double initvol, double destvol )
        {

        }

    private:
        const SampleBank & m_srcsmpl;
        int                m_desiredsmplrate;
        bool               m_bshouldbakeenv;
    };


};