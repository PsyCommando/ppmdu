#ifndef AUDIO_SAMPLE_HPP
#define AUDIO_SAMPLE_HPP
/*
audio_sample.hpp
2015/06/07
psycommando@gmail.com
Description: Various tools for representing raw audio data.
*/
#include <cstdint>
#include <vector>

namespace audio
{
    //Sample formats
    typedef int16_t pcm16s_t;
    typedef int8_t  pcm8s_t;

    /*
        Container for containing interlaced data.

        An audio sequence is made up of blocks of samples.
        Each blocks can have 1 or more sample(s), depending on the number
        of channels/strands interlaced.

        _SampleTy  : the type of the audio samples used.
        _NbStrands : the amount of channels speaking of audio data.
    */
    //template<class _SampleTy, int _NbStrands>
    //    class InterlacedContainer
    //{
    //public:
    //    typedef _SampleTy           smpl_t;
    //    typedef std::vector<smpl_t> block_t;
    //    static const int NbStrands = _NbStrands;

    //    InterlacedContainer(  )

    //    block_t operator[](unsigned int index)
    //    {
    //        return m_blocks[i];
    //    }

    //private:
    //    std::vector<block_t> m_blocks;
    //};
};

#endif