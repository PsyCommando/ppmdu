#include "adpcm.hpp"
#include <utils/utility.hpp>
#include <vector>
#include <array>
#include <cstdint>
#include <algorithm>
#include <cassert>
using namespace std;
using namespace utils;

namespace audio
{
//==============================================================================================
// Constants
//==============================================================================================

    struct IMA_ADPCM    //#FIXME: When did I write that ? This ain't Java !!
    {
        static const int NbBitsPerSample = 4;
        static const int NbPossibleCodes = utils::do_exponent_of_2_<NbBitsPerSample>::value;
        static const int NbSteps         = 89;

        static const array<int8_t,  NbPossibleCodes> IndexTable;
        static const array<int16_t, NbSteps>         StepSizes;
    };

    const std::array<int8_t,IMA_ADPCM::NbPossibleCodes> IMA_ADPCM::IndexTable =
    {
        -1, -1, -1, -1, 
         2,  4,  6,  8,
        -1, -1, -1, -1, 
         2,  4,  6,  8,
    };

    const std::array<int16_t,IMA_ADPCM::NbSteps> IMA_ADPCM::StepSizes = 
    {
        7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 
        19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 
        50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 
        130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
        337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
        876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 
        2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
        5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 
        15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767 
    };


//
//  ADPCMTraits
//
    //----------------
    //  IMA ADPCM
    //----------------
    /*
    */
    class ADPCM_Trait_IMA
    {
    public:

        static inline int32_t ClampStepIndex( int32_t index )
        {
            if(index < 0)
                return 0;

            if( static_cast<uint32_t>(index) >= IMA_ADPCM::StepSizes.size() )
                return (IMA_ADPCM::StepSizes.size() - 1);

            return index;
        }

        static inline int32_t ClampPredictor( int32_t predictor )
        {
            if (predictor > std::numeric_limits<int16_t>::max() ) 
			    return std::numeric_limits<int16_t>::max();
		    else if (predictor < std::numeric_limits<int16_t>::min())
			    return std::numeric_limits<int16_t>::min();
            else
                return predictor;
        }


        static const array<int8_t,  IMA_ADPCM::NbPossibleCodes> & IndexTable;
        static const array<int16_t, IMA_ADPCM::NbSteps>         & StepSizes; 
    };
    const array<int8_t,  IMA_ADPCM::NbPossibleCodes> & ADPCM_Trait_IMA::IndexTable = IMA_ADPCM::IndexTable;
    const array<int16_t, IMA_ADPCM::NbSteps>         & ADPCM_Trait_IMA::StepSizes  = IMA_ADPCM::StepSizes;


    //----------------
    //  NDS ADPCM
    //----------------
    /*
        The NDS clamps samples differently.
    */
    class ADPCM_Trait_NDS
    {
    public:

        static inline int32_t ClampStepIndex( int32_t index )
        {
            if(index < 0)
                return 0;

            if( static_cast<uint32_t>(index) >= IMA_ADPCM::StepSizes.size())
                return (IMA_ADPCM::StepSizes.size() - 1);

            return index;
        }

        static inline int32_t ClampPredictor( int32_t predictor )
        {
            if (predictor > SignedMaxSample ) 
			    return SignedMaxSample;
		    else if (predictor < SignedMinSample)
			    return SignedMinSample;
            else
                return predictor;
        }

        static const array<int8_t,  IMA_ADPCM::NbPossibleCodes> & IndexTable;
        static const array<int16_t, IMA_ADPCM::NbSteps>         & StepSizes;
        static const int16_t                                      SignedMaxSample =  0x7FFF;
        static const int16_t                                      SignedMinSample = -0x7FFF;
    };
    const array<int8_t,  IMA_ADPCM::NbPossibleCodes> & ADPCM_Trait_NDS::IndexTable = IMA_ADPCM::IndexTable;
    const array<int16_t, IMA_ADPCM::NbSteps>         & ADPCM_Trait_NDS::StepSizes  = IMA_ADPCM::StepSizes;

//==============================================================================================
// IMA ADPCM Decoder
//==============================================================================================

    template<class _ADPCM_Trait>
        class IMA_APCM_Decoder 
    {
        typedef _ADPCM_Trait mytrait;

        //For decoding multi-channels adpcm
        struct chanstate
        {
            int32_t predictor = 0;
            int16_t stepindex = 0;
            int16_t step      = 0;
            void reset(){ (*this) = chanstate(); }
        };

    public:

        IMA_APCM_Decoder( const vector<uint8_t> & rawadpcmdata, unsigned int nbchannels = 1 )
            :m_data(rawadpcmdata), m_chan(nbchannels)
        {}

        //Convert to pcm16 signed samples
        operator std::vector<pcm16s_t>()
        {
            return DoParse();
        }

        //Convert straigth to raw bytes
        operator std::vector<uint8_t>()
        {
            //We could really just cast the entire vector, but that wouldn't be implementation nor architecture safe..
            std::vector<pcm16s_t> pcmdat = DoParse();
            std::vector<uint8_t>  rawdat;
            rawdat.reserve( pcmdat.size() * 2 );
            auto itinsert = std::back_inserter(rawdat); 

            //Convert from 16 bits pcm to raw data.
            for( const auto & smpl : pcmdat )
                WriteIntToBytes( smpl, itinsert );

            return std::move(rawdat);
        }

    private:


        std::vector<pcm16s_t> DoParse()
        {
            //Clear state
            m_itread = m_data.begin();
            for( auto & achannel : m_chan ) 
                achannel.reset();

            //Init our state using the preamble
            for( auto & achannel : m_chan ) 
                ParsePreamble( achannel ); //#TODO: Reorganize to handle each datablocks for each channels

            //Parse and convert our samples
            return ParseSamples();
        }

        void ParsePreamble( chanstate & ach )
        {
            //Init channels with initial values for the predictor and step index
            ach.predictor = ReadIntFromBytes<int16_t>(m_itread); //Increments iterator
            ach.stepindex = mytrait::ClampStepIndex( ReadIntFromBytes<int16_t>(m_itread) );
            ach.step      = mytrait::StepSizes[ach.stepindex];
        }

        std::vector<pcm16s_t> ParseSamples()
        {
            std::vector<pcm16s_t> results;
            results.reserve( m_data.size() * 2 );
            
            uint32_t cnt = 0;
            while( m_itread != m_data.end() )
            {
                //Read two 4 bits samples
                uint8_t          curbuff = ReadIntFromBytes<int8_t>(m_itread); //iterator is incremented 
                array<int8_t, 2> smpls   = { curbuff & 0x0F, (curbuff >> 4) & 0x0F };

                //Decode them
                for( auto & smpl : smpls )
                {
                    uint32_t curchan = (cnt % m_chan.size()); //Pick current channel depending on what sample we're working on
                    results.push_back( ParseSample( smpl, m_chan[curchan]) );
                    ++cnt;
                }

            }
            return std::move( results );
        }

        pcm16s_t ParseSample( uint8_t smpl, chanstate & curchan )
        {
		    curchan.step = mytrait::StepSizes[curchan.stepindex];
            int32_t diff = curchan.step >> 3;

		    if (smpl & 1)
			    diff += ( curchan.step >> 2 );
		    if (smpl & 2)
			    diff += ( curchan.step >> 1 );
		    if (smpl & 4)
			    diff += curchan.step;
		    if (smpl & 8)
                curchan.predictor = mytrait::ClampPredictor( curchan.predictor - diff );
            else
                curchan.predictor = mytrait::ClampPredictor( curchan.predictor + diff );

		    curchan.stepindex = mytrait::ClampStepIndex( curchan.stepindex + mytrait::IndexTable[smpl] );

            return curchan.predictor;
        }

    private:
        vector<chanstate>                m_chan;
        const vector<uint8_t>          & m_data;
        vector<uint8_t>::const_iterator  m_itread;
    };

//==============================================================================================
// IMA ADPCM Encoder
//==============================================================================================

    class IMA_ADPCM_Encoder
    {
    public:
        IMA_ADPCM_Encoder( const vector<pcm16s_t> & samples, unsigned int nbchannels = 1 )
        {}

        operator vector<uint8_t>()
        {
            assert(false); //#TODO: Implement me !
            return vector<uint8_t>();
        }

    private:
    };

//==============================================================================================
// Functions
//==============================================================================================
    
    
    std::vector<pcm16s_t> DecodeADPCM_IMA( const std::vector<uint8_t> & rawadpcmdata,
                                           unsigned int                 nbchannels  )
    {
        return IMA_APCM_Decoder<ADPCM_Trait_IMA>(rawadpcmdata,nbchannels);
    }

    std::vector<uint8_t> EncodeADPCM_IMA( const std::vector<pcm16s_t> & pcmdata,
                                          unsigned int                 nbchannels )
    {
        return IMA_ADPCM_Encoder(pcmdata,nbchannels);
    }

    size_t ADPCMSzToPCM16Sz( size_t adpcmbytesz )
    {
        return (adpcmbytesz - IMA_ADPCM_PreambleLen) * 2;
    }

    std::vector<pcm16s_t> DecodeADPCM_NDS( const std::vector<uint8_t> & rawadpcmdata,
                                           unsigned int                 nbchannels  )
    {
        return IMA_APCM_Decoder<ADPCM_Trait_NDS>(rawadpcmdata,nbchannels);
    }
    
};