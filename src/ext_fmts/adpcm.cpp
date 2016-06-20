#include "adpcm.hpp"
#include <utils/utility.hpp>
#include <vector>
#include <array>
#include <cstdint>
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>

using namespace std;
using namespace utils;

/*
    #TODO: Move classes into their own header. Leave the function definitions in here and include that other header!
*/

namespace audio
{
//==============================================================================================
// Constants
//==============================================================================================

    namespace IMA_ADPCM    
    {
        static const int NbBitsPerSample = 4;
        static const int NbPossibleCodes = utils::do_exponent_of_2_<NbBitsPerSample>::value;
        static const int NbSteps         = 89;

        static const std::array<int8_t,NbPossibleCodes> IndexTable =
        {
            -1, -1, -1, -1, 
             2,  4,  6,  8,
            -1, -1, -1, -1, 
             2,  4,  6,  8,
        };

        static const std::array<int16_t,NbSteps> StepSizes = 
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
    };


//==============================================================================================
//  ADPCMTraits
//==============================================================================================
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
        operator std::vector<int16_t>()
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


        std::vector<int16_t> DoParse()
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
            ach.predictor = ReadIntFromBytes<int16_t>(m_itread,m_data.end()); //Increments iterator
            ach.stepindex = mytrait::ClampStepIndex( ReadIntFromBytes<int16_t>(m_itread,m_data.end()) );
            ach.step      = mytrait::StepSizes[ach.stepindex];
        }

        std::vector<int16_t> ParseSamples()
        {
            std::vector<int16_t> results;
            results.reserve( m_data.size() * 2 );
            
            uint32_t cnt = 0;
            while( m_itread != m_data.end() )
            {
                //Read two 4 bits samples
                uint8_t          curbuff = ReadIntFromBytes<int8_t>(m_itread,m_data.end()); //iterator is incremented 
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

        int16_t ParseSample( uint8_t smpl, chanstate & curchan )
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
// IMA ADPCM Realtime Decoder
//==============================================================================================

    //#TODO: Needs to be tested !!
    //This is a verison of the adpcm decoder optimized for real-time applications
    template<class _ADPCM_Trait, bool _LittleEndian = true>
        class IMA_APCM_RT_Decoder 
    {
        typedef _ADPCM_Trait mytrait;
        //static const bool    IsLittleEndian = _LittleEndian;

    public:
        //For decoding multi-channels adpcm
        struct chanstate
        {
            int32_t predictor = 0;
            int16_t stepindex = 0;
            int16_t step      = 0;
        };

        IMA_APCM_RT_Decoder( unsigned int nbchannels = 1 )
            :m_chan(nbchannels)
        {}

        /*
            SetPreambleData
                Initialises the channel state with the data from the ADPCM preamble.
        */
        void SetPreambleData( size_t chan, int32_t predictor = 0, int16_t stepindex = 0 )
        {
            auto & ach = m_chan.at(chan);
            //Init channels with initial values for the predictor and step index
            ach.predictor = mytrait::ClampPredictor( predictor );
            ach.stepindex = mytrait::ClampStepIndex( stepindex );
            ach.step      = mytrait::StepSizes[ach.stepindex];
        }

        /*
            SetPreambleDataFromBytes
                Initialises the channel states for each channel, using the preamble data at the iterator specified.
                It will read (4 * nbchannels) bytes from the iterator!
        */
        template<class _init>
            _init SetPreambleDataFromBytes( _init itpreamble, _init itend )
        {
            //Init preamble data
            for( size_t cntchan = 0; cntchan < m_chan.size() && itpreamble != itend; ++cntchan )
            {
                int16_t pred    = ReadIntFromBytes<int16_t>(itpreamble, itend);
                int16_t stepind = ReadIntFromBytes<int16_t>(itpreamble, itend);
                SetPreambleData( cntchan, pred, stepind );
            }
            return itpreamble;
        }


        /*
            SetStateData
                Restore a channel state.
                Mainly used for looping.
        */
        inline void SetStateData( size_t chan, const chanstate & cstate )
        {
            m_chan.at(chan) = cstate;
        }

        /*
            GetStateData
                Get a channel's current state.
                Mainly used for looping.
        */
        inline chanstate GetStateData( size_t chan )const
        {
            return m_chan.at(chan);
        }

        /*
            ParseSome
                Will parse a set amount of adpcm data and write it starting from itout.
                - nbtoparse : the nb of BYTES to handle. Not ADPCM samples.
                - chan      : the channel state to use for parsing.

                The method expects that there are at least "nbtoparse" elemts at itread, and
                that there is at least ("nbtoparse" * 2) at itout!

                Be sure to set the preamble data before. Since this is meant for realtime, ADPCM preamble
                data is never assumed to be in the bytes passed to this method!
        */
        template<class _init, class _outit>
            _init ParseSome( _init itread, size_t bytestoparse, _outit itout, size_t chan )
        {
            if( chan >= m_chan.size() )
                throw runtime_error( "IMA_APCM_RT_Decoder::ParseSome() : Invalid channel specified!" );

            for( size_t cntpt = 0;  cntpt < bytestoparse; ++cntpt, ++itread )
                ParseAByte( *itread, m_chan[chan], itout );
            
            return itread;
        }

        template<class _init, class _outit>
            _init ParseSomeBlocks( _init itread, _init itend, size_t nbblocks, _outit itout, size_t chan )
        {
            if( chan >= m_chan.size() )
                throw runtime_error( "IMA_APCM_RT_Decoder::ParseSomeBlocks() : Invalid channel specified!" );

            for( size_t cntblk = 0;  cntblk < nbblocks && itread != itend; ++cntblk )
                ParseABlock( itread, itend, m_chan[chan], itout );
            
            return itread;
        }


        /*
            SkipSome
                Allows to skip over a number of ADPCM samples, to get to the desired offset in the sample, 
                while making sure the predictor and step indexes are valid as if the sample had been played up until that
                point normally.
        */
        template<class _init>
            _init SkipSome( _init itread, size_t bytestoskip, size_t chan )
        {
            if( chan >= m_chan.size() )
                throw runtime_error( "IMA_APCM_RT_Decoder::SkipSome() : Invalid channel specified!" );

            for( size_t cntpt = 0;  cntpt < bytestoskip; ++cntpt, ++itread )
                SkipAByte( *itread, m_chan[chan] );

            return itread;
        }


    private:
        //void ParsePreamble( chanstate & ach )
        //{
        //    //Init channels with initial values for the predictor and step index
        //    ach.predictor = ReadIntFromBytes<int16_t>(m_itread); //Increments iterator
        //    ach.stepindex = mytrait::ClampStepIndex( ReadIntFromBytes<int16_t>(m_itread) );
        //    ach.step      = mytrait::StepSizes[ach.stepindex];
        //}

        template<class _init, class _outit>
            void ParseABlock( _init itread, _init itend, chanstate & curchan, _outit itout )
        {
            //std::vector<int16_t> buf;
            //auto itbackins = std::back_inserter(buf);

            for( size_t cntpt = 0;  cntpt < 4 && itread != itend; ++cntpt, ++itread )
                ParseAByte( *itread, curchan, itout );

            //Reverse the byte order
 /*           if( ! buf.empty() )
            {
                for( size_t cntpt = (buf.size() - 1);  cntpt >= 0; --cntpt, ++itout )
                    (*itout) = buf[cntpt];
            }*/
        }

        //---------------
        // ParseAByte
        //---------------

        //Little endian
        template<class _outit, bool _E = _LittleEndian >
            typename std::enable_if<_E, void>::type ParseAByte( uint8_t by, chanstate & curchan, _outit itout )
        {
            static_assert(_E, "IMA_APCM_RT_Decoder::ParseAByte() : Little endian function used for big endian !!"); //#REMOVEME Just there to ensure nothing broke
            (*itout) = ParseSample( by        & 0x0F, curchan );
            ++itout;
            (*itout) = ParseSample( (by >> 4) & 0x0F, curchan );
            ++itout;
        }

        //Big endian
        template<class _outit, bool _E = _LittleEndian >
            typename std::enable_if<!_E, void>::type ParseAByte( uint8_t by, chanstate & curchan, _outit itout )
        {   
            static_assert(!_E, "IMA_APCM_RT_Decoder::ParseAByte() : Big endian function used for little endian !!"); //#REMOVEME Just there to ensure nothing broke
            (*itout) = ParseSample( (by >> 4) & 0x0F, curchan );
            ++itout;
            (*itout) = ParseSample( by        & 0x0F, curchan );
            ++itout;
        }

        //---------------
        // SkipAByte
        //---------------

        //Little endian
        template<bool _E = _LittleEndian >
            typename std::enable_if<_E, void>::type SkipAByte( uint8_t by, chanstate & curchan )
        {
            static_assert(_E, "IMA_APCM_RT_Decoder::SkipAByte() : Little endian function used for big endian !!"); //#REMOVEME Just there to ensure nothing broke
            ParseSample( by        & 0x0F, curchan );
            ParseSample( (by >> 4) & 0x0F, curchan );
        }

        //Big endian
        template<bool _E = _LittleEndian >
            typename std::enable_if<!_E, void>::type SkipAByte( uint8_t by, chanstate & curchan )
        {   
            static_assert(!_E, "IMA_APCM_RT_Decoder::SkipAByte() : Big endian function used for little endian !!"); //#REMOVEME Just there to ensure nothing broke
            ParseSample( (by >> 4) & 0x0F, curchan );
            ParseSample( by        & 0x0F, curchan );
        }

        //---------------
        // ParseSample
        //---------------
        int16_t ParseSample( uint8_t smpl, chanstate & curchan )
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
        vector<chanstate> m_chan;
    };

//==============================================================================================
// IMA ADPCM Encoder
//==============================================================================================

    class IMA_ADPCM_Encoder
    {
    public:
        IMA_ADPCM_Encoder( const vector<int16_t> & samples, unsigned int nbchannels = 1 )
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
    
    std::vector<int16_t> DecodeADPCM_IMA( const std::vector<uint8_t> & rawadpcmdata,
                                           unsigned int                 nbchannels  )
    {
        return IMA_APCM_Decoder<ADPCM_Trait_IMA>(rawadpcmdata,nbchannels);
    }

    std::vector<uint8_t> EncodeADPCM_IMA( const std::vector<int16_t> & pcmdata,
                                          unsigned int                 nbchannels )
    {
        return IMA_ADPCM_Encoder(pcmdata,nbchannels);
    }

    size_t ADPCMSzToPCM16Sz( size_t adpcmbytesz )
    {
        return (adpcmbytesz - IMA_ADPCM_PreambleLen) * 2;
    }

    std::vector<int16_t> DecodeADPCM_NDS( const std::vector<uint8_t> & rawadpcmdata,
                                           unsigned int                 nbchannels  )
    {
        return IMA_APCM_Decoder<ADPCM_Trait_NDS>(rawadpcmdata,nbchannels);
    }


    /*
        DumpADPCM
            Write the raw ADPCM data to a file.
    */
    void DumpADPCM( const std::string          & outfile,
                    const std::vector<uint8_t> & rawadpcm )
    {
        utils::io::WriteByteVectorToFile( outfile, rawadpcm );
    }

    /*
        ReadADPCMDump
            Read a raw ADPCM dump made with DumpADPCM from a file.
    */
    std::vector<uint8_t> ReadADPCMDump( const std::string & infile )
    {
        return move( utils::io::ReadFileToByteVector(infile) );
    }

    /*
        ConvertRangeADPCM_NDS
            Convert only a range of ADPCM samples into signed PCM16. 
            - offset     : The start counted as blocks of 4 bytes of the start of the part to convert.
            - len        : The length counted as blocks of 4 bytes of the part to convert.
            - nbchannels : The nb of channels the adpcm data has.
    */
    std::vector<std::vector<int16_t>>   ConvertRangeADPCM_NDS( const std::vector<uint8_t>  & rawadpcmdata,
                                                  uint32_t                      offset,
                                                  uint32_t                      len,
                                                  unsigned int                  nbchannels )
    {
        auto                                 itread = rawadpcmdata.begin();
        auto                                 itend  = rawadpcmdata.end();
        IMA_APCM_RT_Decoder<ADPCM_Trait_NDS> decoder(nbchannels);

        //Init preamble data
        itread = decoder.SetPreambleDataFromBytes( itread, itend );

        //Sanity check
        if( itread == itend )
            throw std::runtime_error("ConvertRangeADPCM_NDS(): No ADPCM sample data to parse!");

        for( size_t cntchan = 0; cntchan < nbchannels && itread != itend; ++cntchan )
        {
            //Skip over the bytes before the desired offset
            itread = decoder.SkipSome( itread, 4 * offset, cntchan );
        }

        //Sanity check
        if( itread == itend )
            throw std::runtime_error("ConvertRangeADPCM_NDS(): No ADPCM sample data to parse past specified offset!");

        //Convert the samples
        std::vector<vector<int16_t>> chanbuf(nbchannels);
        for( size_t cntchan = 0; cntchan < nbchannels && itread != itend; ++cntchan )
        {
            itread = decoder.ParseSomeBlocks( itread, itend, len, std::back_inserter( chanbuf[cntchan] ), cntchan );
            //for( size_t cntblock = 0; cntblock < len && itread != itend; ++cntblock )
            //{
            //    auto itblock = std::back_inserter( blockbuf[cntchan] );

            //    //Parse the 4 bytes in the block //#FIXME: Iterators prevent us from pre-checking if we can parse an entire block.
            //    for( size_t cntbyte = 0; cntbyte < 4 && itread != itend; ++cntbyte )
            //        itread = decoder.ParseSome( itread, 1, itblock, cntchan );
            //}
        }

        return std::move(chanbuf);

        //Interlace if multi-channels //#FIXME: We should seriously just return a multi-dimensionnal vector instead!! 
    }



    /*  
        LoopAndConvertADPCM_NDS
            Convert ADPCM for the NDS while also looping it a specified number of times
            - loopbeg    : The start, counted as blocks of 4 bytes, of the loop's beginning.
            - looplen    : The length, counted as blocks of 4 bytes, of the loop.
            - nbloops    : The nb of times to loop the loop in the converted sample.
            - nbchannels : The nb of channels the adpcm data has.
    */
    std::pair<std::vector<std::vector<int16_t>>, size_t> LoopAndConvertADPCM_NDS( const std::vector<uint8_t>  & rawadpcmdata,
                                                  uint32_t                      loopbeg,
                                                  uint32_t                      looplen,
                                                  uint32_t                      nbloops,
                                                  unsigned int                  nbchannels )
    {
        vector<vector<int16_t>>              chanbuff(nbchannels);
        size_t                               newlpbeg = 0;
        auto                                 itread   = rawadpcmdata.begin();
        auto                                 itend    = rawadpcmdata.end();
        IMA_APCM_RT_Decoder<ADPCM_Trait_NDS> decoder(nbchannels);
        size_t                               lpbegsmpl = loopbeg - ( nbchannels ); //Get the actual loop beg without the preamble (counted in int32 )

        itread = decoder.SetPreambleDataFromBytes( itread, itend );

        //Sanity check
        if( itread == itend )
            throw std::runtime_error("LoopAndConvertADPCM_NDS(): No ADPCM sample data to parse!");

        //convert everything before the loop beg
        if( lpbegsmpl != 0 )
        {
            for( size_t cntchan = 0; cntchan < nbchannels && itread != itend; ++cntchan )
            {
                auto itbackins = back_inserter( chanbuff[cntchan] );
                itread = decoder.ParseSomeBlocks( itread, itend, lpbegsmpl, itbackins, cntchan );
            }
        }

        newlpbeg = chanbuff.front().size();

        //Sanity check
        if( itread == itend )
            throw std::runtime_error("LoopAndConvertADPCM_NDS(): Missing loop samples!!");


        //Convert the loop once, and copy it nbloops times.
        vector<int16_t> loopbuf;
        auto itlp = back_inserter(loopbuf);
        for( size_t cntchan = 0; cntchan < nbchannels && itread != itend; ++cntchan )
        {
            loopbuf.resize(0);
            //Parse the loop
            itread = decoder.ParseSomeBlocks( itread, itend, looplen, itlp, cntchan );

            //Copy the loop
            auto itbackins = back_inserter( chanbuff[cntchan] );

            for( size_t cntlp = 0; cntlp < nbloops; ++cntlp )
                std::copy( loopbuf.begin(), loopbuf.end(), itbackins );
        }

        //return
        return std::make_pair( std::move(chanbuff), newlpbeg );
    }
    
};