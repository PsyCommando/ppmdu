#ifndef ADPCM_HPP
#define ADPCM_HPP
/*
adpcm.hpp
2015/06/07
psycommando@gmail.com
Description: Utilities for handling ADPCM data.
*/
#include <ppmdu/containers/audio_sample.hpp>
#include <cstdint>
#include <vector>

namespace audio
{
//====================================================================================================
// Functions
//====================================================================================================

    //
    //  ADPCM Encoding/Decoding
    //
    std::vector<audio::pcm16s_t> DecodeADPCM_IMA( const std::vector<uint8_t>         & rawadpcmdata,
                                                  unsigned int                         nbchannels   = 1 );

    std::vector<uint8_t>         EncodeADPCM_IMA( const std::vector<audio::pcm16s_t> & pcmdata, 
                                                  unsigned int                         nbchannels   = 1 );

};

#endif