#ifndef WAVE_IO_HPP
#define WAVE_IO_HPP
/*
wave_io.hpp
2015/10/15
psycommando@gmail.com
Description:
    Utilities for accessing a RIFF WAVE file.
*/
#include <cstdint>
#include <vector>
#include <string>
#include <ext_fmts/riff.hpp>

namespace wave
{

    const uint32_t WAVE_FormatTag = 0x57415645; //"WAVE"
    const uint32_t FMT_ChunkTag   = 0x666d7420; //"fmt "
    const uint32_t DATA_ChunkTag  = 0x64617461; //"data"
    const uint32_t SMPL_ChunkTag  = 0x736D706C; //"smpl"

//=============================================================================
//  WAVE file structure
//=============================================================================

    /**************************************************************************************************
        eAudioFormat
            The possible audio formats for the WAVE file!
            In no way an extensive list! See full list there: ftp://ftp.isi.edu/in-notes/rfc2361.txt
    **************************************************************************************************/
    enum struct eAudioFormat : uint16_t
    {
        UNKNOWN             = 0x0000,   //Microsoft Unknown Wave Format
        PCM                 = 0x0001,   //Microsoft PCM Format
        ADPCM               = 0x0002,   //Microsoft ADPCM Format
        IEEE_FLOAT          = 0x0003,   //IEEE Float

        ALAW                = 0x0006,   //Microsoft ALAW
        MULAW               = 0x0007,   //Microsoft MULAW

        OKI_ADPCM           = 0x0010,   //OKI ADPCM

        SIERRA_ADPCM        = 0x0013,   //Sierra ADPCM

        DIALOGIC_OKI_ADPCM  = 0x0017,   //Dialogic OKI ADPCM

        YAMAHA_ADPCM        = 0x0020,   //Yamaha ADPCM

        MPEG                = 0x0050,   //MPEG

        MPEGLAYER3          = 0x0055,   //MPEG Layer 3

    };

    /*************************************************
        WAVE_fmt_chunk
            Details on the content of the WAVE file.
    **************************************************/
    struct WAVE_fmt_chunk
    {
        template<class _outit>
            _outit Write( _outit itwrite )
        {
            itwrite = utils::WriteIntToBytes( audiofmt_,      itwrite );
            itwrite = utils::WriteIntToBytes( nbchannels_,    itwrite );
            itwrite = utils::WriteIntToBytes( samplerate_,    itwrite );
            itwrite = utils::WriteIntToBytes( byterate_,      itwrite );
            itwrite = utils::WriteIntToBytes( blockalign_,    itwrite );
            itwrite = utils::WriteIntToBytes( bitspersample_, itwrite );
            return itwrite;
        }

        template<class _init>
            _init Read( _init itread, _init itend )
        {
            itread = utils::ReadIntFromBytes( audiofmt_,      itread, itend );
            itread = utils::ReadIntFromBytes( nbchannels_,    itread, itend );
            itread = utils::ReadIntFromBytes( samplerate_,    itread, itend );
            itread = utils::ReadIntFromBytes( byterate_,      itread, itend );
            itread = utils::ReadIntFromBytes( blockalign_,    itread, itend );
            itread = utils::ReadIntFromBytes( bitspersample_, itread, itend );
            return itread;
        }

        inline void Read( const riff::Chunk & chunk )
        {
            Read(chunk.data_.begin(), chunk.data_.end());
        }

        WAVE_fmt_chunk & operator=( const riff::Chunk & chunk )
        {
            Read(chunk);
            return *this;
        }

        uint16_t audiofmt_;
        uint16_t nbchannels_;
        uint32_t samplerate_;
        uint32_t byterate_;
        uint16_t blockalign_;
        uint16_t bitspersample_;
    };

    /*******************************************************************************
        smpl_chunk_content
            Sampler chunk. Contains info on loop points for a wave file!
    *******************************************************************************/
    struct smpl_chunk_content
    {
        smpl_chunk_content()
            :manufacturer_(0), product_(0), samplePeriod_(0), MIDIUnityNote_(60),
             MIDIPitchFraction_(0),SMPTEFormat_(0),SMPTEOffset_(0),sampleLoops_(0),
             samplerData_(0)
        {}

        template<class _outit>
            _outit Write( _outit itwrite )const
        {
            itwrite = utils::WriteIntToBytes( manufacturer_,                        itwrite );
            itwrite = utils::WriteIntToBytes( product_,                             itwrite );
            itwrite = utils::WriteIntToBytes( samplePeriod_,                        itwrite );
            itwrite = utils::WriteIntToBytes( MIDIUnityNote_,                       itwrite );
            itwrite = utils::WriteIntToBytes( MIDIPitchFraction_,                   itwrite );
            itwrite = utils::WriteIntToBytes( SMPTEFormat_,                         itwrite );
            itwrite = utils::WriteIntToBytes( SMPTEOffset_,                         itwrite );
            itwrite = utils::WriteIntToBytes( static_cast<uint32_t>(loops_.size()), itwrite );
            itwrite = utils::WriteIntToBytes( samplerData_,                         itwrite );

            for( const auto & aloop : loops_ )
                itwrite = aloop.Write(itwrite);

            return itwrite;
        }

        template<class _init>
            _init Read( _init itread, _init itend )
        {
            itread = utils::ReadIntFromBytes( manufacturer_,        itread, itend );
            itread = utils::ReadIntFromBytes( product_,             itread, itend );
            itread = utils::ReadIntFromBytes( samplePeriod_,        itread, itend );
            itread = utils::ReadIntFromBytes( MIDIUnityNote_,       itread, itend );
            itread = utils::ReadIntFromBytes( MIDIPitchFraction_,   itread, itend );
            itread = utils::ReadIntFromBytes( SMPTEFormat_,         itread, itend );
            itread = utils::ReadIntFromBytes( SMPTEOffset_,         itread, itend );
            itread = utils::ReadIntFromBytes( sampleLoops_,         itread, itend );
            itread = utils::ReadIntFromBytes( samplerData_,         itread, itend );
            loops_.resize(sampleLoops_);
            for( auto & aloop : loops_ )
                itread = aloop.Read(itread, itend);
            return itread;
        }

        inline void Read( const riff::Chunk & chunk )
        {
            Read(chunk.data_.begin(), chunk.data_.end() );
        }

        smpl_chunk_content & operator=( const riff::Chunk & chunk )
        {
            Read(chunk);
            return *this;
        }

        operator riff::Chunk()const
        {
            riff::Chunk deschunk( SMPL_ChunkTag );
            auto        itwrite = std::back_inserter(deschunk.data_);
            Write(itwrite);
            return std::move(deschunk);
        }

        /*
            Indicates the position of all loop points and other details.
        */
        struct SampleLoop
        {
            SampleLoop( uint32_t id = 0, uint32_t ty = 0, uint32_t start = 0, uint32_t end = 0, uint32_t frac = 0, uint32_t count = 0 )
                :identifier_(id), type_(ty), start_(start), end_(end), fraction_(frac), playCount_(count)
            {}

            uint32_t identifier_;   //Unique ID to give this loop. May match a cue point.
            uint32_t type_;         //0== loop forward, 1==alternating loop(forward/backward), 2==loop backward
            uint32_t start_;        //Start byte offset of first sample point of the loop!
            uint32_t end_;          //End byte offset of last sample point of the loop!
            uint32_t fraction_;     //loop fractional areas between samples.
            uint32_t playCount_;    //0 == infinite, n == not infinite

            template<class _outit>
                _outit Write( _outit itwrite )const
            {
                itwrite = utils::WriteIntToBytes( identifier_, itwrite );
                itwrite = utils::WriteIntToBytes( type_,       itwrite );
                itwrite = utils::WriteIntToBytes( start_,      itwrite );
                itwrite = utils::WriteIntToBytes( end_,        itwrite );
                itwrite = utils::WriteIntToBytes( fraction_,   itwrite );
                itwrite = utils::WriteIntToBytes( playCount_,  itwrite );
                return itwrite;
            }

            template<class _init>
                _init Read( _init itread, _init itend )
            {
                itread = utils::ReadIntFromBytes( identifier_, itread, itend );
                itread = utils::ReadIntFromBytes( type_,       itread, itend );
                itread = utils::ReadIntFromBytes( start_,      itread, itend );
                itread = utils::ReadIntFromBytes( end_,        itread, itend );
                itread = utils::ReadIntFromBytes( fraction_,   itread, itend );
                itread = utils::ReadIntFromBytes( playCount_,  itread, itend );
                return itread;
            }
        };

        uint32_t                manufacturer_;          //Manufacturer ID (set to 0)
        uint32_t                product_;               //Product ID (set to 0)
        uint32_t                samplePeriod_;          //normally 1/nSamplesPerSec from the Format chunk.
        uint32_t                MIDIUnityNote_;         //MIDI note number at which the instrument plays back the waveform data without pitch modification
        uint32_t                MIDIPitchFraction_;     //Semitone fraction between note, 0 is default. 0x80000000 is 50 cents
        uint32_t                SMPTEFormat_;           //SMPTE synch method. 0 is unused.
        uint32_t                SMPTEOffset_;           //If above is 0, set to 0!
        uint32_t                sampleLoops_;           //The nb of loop points in the loops_ array!
        uint32_t                samplerData_;           //size of sampler exclusive data appended after the loop array. Set to 0!
        std::vector<SampleLoop> loops_;                 //List of loop points.
    };


//=============================================================================
//  Utilities
//=============================================================================
    /*****************************************************************************
        GetWaveFormatInfo
            Read and returns the fmt chunk for the WAVE file/container specified.
    ******************************************************************************/
    template<class _init>
        WAVE_fmt_chunk GetWaveFormatInfo( _init itwavbeg )
    {
        using namespace riff;
        ChunkHeader hdr;
        itwavbeg = hdr.Read( itwavbeg );

        if( hdr.chunk_id != static_cast<uint32_t>(eChunkIDs::RIFF) )
            throw std::runtime_error("GetWaveFormatInfo(): Error, the container specified is not a valid WAVE file. RIFF header is invalid!");

        //Read format tag
        uin32_t fmttag = utils::ReadIntFromBytes<uint32_t>( itwavbeg, false ); //Iterator incremented!

        if( fmttag != WAVE_FormatTag )
            throw std::runtime_error("GetWaveFormatInfo(): Error, the container specified is not a valid WAVE file! Missing WAVE format tag after the header!");

        WAVE_fmt_chunk fmtchunk;
        itwavbeg = fmtchunk.Read( itwavbeg );
        return move(fmtchunk); 
    }

//=============================================================================
//  WAVE file handler
//=============================================================================
    /**************************************
        WaveTrait
            Base class for wav file traits
    ***************************************/
    template<unsigned int _BitDepth, class _SampleTy, eAudioFormat _Format, bool _LittleEndian = true>
        class WaveTrait
    {
    public:
        typedef _SampleTy sample_t;
        static const unsigned int BitDepth       = _BitDepth;
        static const bool         IsLittleEndian = _LittleEndian;
        static const eAudioFormat AudioFormat    = _Format;
    };

    /************************************************
        WaveTrait_PCM16s
            Trait for a PCM16 bits signed wav file!
    ************************************************/
    class WaveTrait_PCM16s : public WaveTrait< 16, int16_t, eAudioFormat::PCM, true>
    {
    public:
        template<class _init>
           static sample_t ParseASample( _init & itread )
        {
            sample_t tmpsmpl = 0;
            tmpsmpl = (*itread);
            ++itread;
            tmpsmpl |= static_cast<int16_t>( (*itread) ) << 8;
            ++itread;
            return tmpsmpl;
        }

        template<class _outit>
           static _outit WriteASample( sample_t smpl, _outit itwrite  )
        {
            (*itwrite) = static_cast<uint8_t>(smpl);
            ++itwrite;
            (*itwrite) = static_cast<uint8_t>(smpl >> 8);
            ++itwrite;
            return itwrite;
        }

    };

    /************************************************
        WaveTrait_PCM8
            Trait for a PCM 8bits wav file!
    ************************************************/
    class WaveTrait_PCM8 : public WaveTrait< 8, uint8_t, eAudioFormat::PCM, true>
    {
    public:
        template<class _init>
           static sample_t ParseASample( _init & itread )
        {
            sample_t tmpsmpl = (*itread);
            ++itread;
            return tmpsmpl;
        }

        template<class _outit>
           static _outit WriteASample( sample_t smpl, _outit itwrite  )
        {
            (*itwrite) = smpl;
            ++itwrite;
            return itwrite;
        }

    };

    /*************************************************************************
        WaveFile
            Represent a wave file and operations that can be performed on it.
    **************************************************************************/
    template<class _WaveTrait>
        class WaveFile
    {
    public:
        typedef typename _WaveTrait          trait_t;
        typedef typename trait_t::sample_t   sample_t;

        WaveFile( uint32_t samplerate = 44100 )
            :m_samplerate(samplerate)
        {}

        void WriteWaveFile( const std::string & fname )
        {
            std::ofstream outfile( fname, std::ios::out | std::ios::binary );

            if( !outfile.is_open() || !outfile.good() )
                throw std::runtime_error( "WaveFile::WriteWaveFile(): Error, couldn't open output file!!" );

            std::ostreambuf_iterator<char> itout(outfile);
            WriteWave(itout);
        }

        void ReadWaveFile( const std::string & fname )
        {
            std::ifstream infile( fname, std::ios::in | std::ios::binary );

            if( !infile.is_open() || !infile.good() )
                throw std::runtime_error( "WaveFile::ReadWaveFile(): Error, couldn't open input file!!" );

            std::istreambuf_iterator<char> itin(infile);
            ReadWave(itin);
        }

        template<class _outit>
            _outit WriteWave( _outit itwrite )
        {
            //Validate!
            if( m_samples.empty() )
                throw std::runtime_error("WaveFile::WriteWave(): Error, there are no samples to write!");
            for( const auto & achan : m_samples )
            {
                if( achan.empty() )
                    throw std::runtime_error("WaveFile::WriteWave(): Error, a channel contains no samples to write!");
            }

            //Build the RIFF container!
            riff::RIFF_Container waveout( WAVE_FormatTag );

            //Make fmt chunk!
            riff::Chunk    fmtchnk( FMT_ChunkTag );
            WAVE_fmt_chunk fmtdat;
            auto           itbackinsfmt = std::back_inserter( fmtchnk.data_ );

            fmtdat.audiofmt_      =  static_cast<uint16_t>(trait_t::AudioFormat);
            fmtdat.bitspersample_ = trait_t::BitDepth;
            fmtdat.nbchannels_    = static_cast<uint16_t>(m_samples.size());
            fmtdat.blockalign_    = (fmtdat.nbchannels_ * fmtdat.bitspersample_) / 8;
            fmtdat.samplerate_    = m_samplerate;
            fmtdat.byterate_      = (m_samplerate * fmtdat.nbchannels_ * fmtdat.bitspersample_) / 8;

            itbackinsfmt = fmtdat.Write(itbackinsfmt);
            waveout.subchunks_.push_back(std::move(fmtchnk));

            //Make data chunk!
            riff::Chunk datachnk( DATA_ChunkTag );

            //Alloc
            datachnk.data_.reserve( ( (fmtdat.nbchannels_ * m_samples[0].size()) * sizeof(sample_t) ) + 1 ); //Add one for a possible padding byte

            auto itbackinsdata = std::back_inserter( datachnk.data_ );

            for( size_t cntsamples = 0; cntsamples < m_samples[0].size(); ++cntsamples )
            {
                for( size_t cntchan = 0; cntchan < m_samples.size(); ++cntchan )
                    trait_t::WriteASample( m_samples[cntchan][cntsamples], itbackinsdata );
            }

            waveout.subchunks_.push_back( std::move(datachnk) );

            //Append extra chunks
            for( size_t cntchunk = 0; cntchunk < m_extrachunks.size(); ++cntchunk )
                waveout.subchunks_.push_back( m_extrachunks[cntchunk] );

            //Write the RIFF
            itwrite = waveout.Write( itwrite );

            if( (datachnk.data_.size() % 2) != 0 )
                datachnk.data_.push_back(0xFF); //Put a padding byte

            return itwrite;
        }

        template<class _init>
            _init ReadWave( _init itfile, _init itfileend )
        {
            //First check if our format matches 
            WAVE_fmt_chunk info = GetWaveFormatInfo( itfile );
            
            if( info.audiofmt_ != static_cast<uint16_t>(trait_t::AudioFormat) )
                throw std::runtime_error( "WaveFile::ReadWave(): Error, the format of the wave file doesn't match the parser's!" );

            if( info.bitspersample_ != trait_t::BitDepth )
            {
                std::stringstream sstr;
                sstr << "WaveFile::ReadWave(): Error the bit depth of the parser, " << trait_t::BitDepth <<", does not match the bit rate of the WAVE container being parsed, " 
                     <<info.bitspersample_ <<"!";
                throw std::runtime_error(sstr.str());
            }

            //Copy samplerate!
            m_samplerate = info.samplerate_;

            //Parse the RIFF container!
            riff::RIFF_Container myriff;
            itfile = myriff.ReadAndValidate( itfile, itfileend );

            if( myriff.subchunks_.size() >= 2 )
            {
                //Find the data chunk
                size_t datachnkindex = 0;
                for( size_t cntchunk = 0; cntchunk < myriff.subchunks_.size(); ++cntchunk )
                {
                    if( myriff.subchunks_[cntchunk].fourcc_ == DATA_ChunkTag )
                        datachnkindex = cntchunk;
                    else
                        m_extrachunks.push_back( myriff.subchunks_[cntchunk] ); //Copy other chunks for later processing
                }

                if( datachnkindex == myriff.subchunks_.size() )
                    throw std::runtime_error( "WaveFile::ReadWave(): Error, no data chunk present in the WAVE container!!" );

                //Begin parsing
                auto       & datachunk        = myriff.subchunks_[datachnkindex];
                const size_t totalnbsamples   = datachunk.size() / ( info.nbchannels_ * sizeof(sample_t) );
                const size_t nbsamplesperchan = totalnbsamples / info.nbchannels_;
                
                //Resize for all the channels
                m_samples.resize(info.nbchannels_, move(std::vector<sample_t>(nbsamplesperchan, 0)) );

                auto itrawdata    = datachunk.begin();
                auto itendrawdata = datachunk.end();

                for( size_t cntsample = 0; cntsample < nbsamplesperchan && itrawdata != itendrawdata; ++cntsample )
                {
                    //Samples are interleaved for each channels
                    for( size_t cntchan = 0; cntchan < info.nbchannels_; ++cntchan )
                        m_samples[cntchan][cntsample] = trait_t::ParseASample( itrawdata );
                }

                if( itrawdata != itendrawdata )
                    std::clog << "<!>- Warning: Some samples were omitted while parsing the WAVE container! The size of the data did not match the Nb of samples hinted at in the WAVE header!\n";
            }
            else
                throw std::runtime_error( "WaveFile::ReadWave(): Error, one or more chunks missing from the wave container!!" );

            return itfile;
        }


        const std::vector<std::vector<sample_t>> & GetSamples()const {return m_samples;}
        std::vector<std::vector<sample_t>>       & GetSamples()      {return m_samples;}

        inline uint32_t SampleRate()const               { return m_samplerate;     }
        inline void     SampleRate( uint32_t smplrate ) { m_samplerate = smplrate; }

        inline const std::deque<riff::Chunk> & ExtraChunks()const { return m_extrachunks; }
        inline std::deque<riff::Chunk>       & ExtraChunks()      { return m_extrachunks; }

        void AddRIFFChunk( riff::Chunk && achunk )
        {
            m_extrachunks.emplace_back( std::move(achunk) );
        }

    private:
        uint32_t                           m_samplerate;
        std::vector<std::vector<sample_t>> m_samples;
        std::deque<riff::Chunk>            m_extrachunks;
    };


//=============================================================================
//  Handy Typedefs
//=============================================================================
    typedef WaveFile<WaveTrait_PCM16s> PCM16sWaveFile;
    typedef WaveFile<WaveTrait_PCM8>   PCM8WaveFile;
};


#endif
