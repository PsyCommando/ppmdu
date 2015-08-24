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
//  Constants
//====================================================================================================
    static const uint32_t IMA_ADPCM_PreambleLen = 4;//bytes

//====================================================================================================
// Functions
//====================================================================================================

    /*
        ADPCMSzToPCM16Sz
            This converts the size of a IMA ADPCM compressed block, to the size of the 
            equivalent block of uncompressed PCM16 samples.
            Use this to predict the decompressed size of a ADPCM block of data !
    */
    size_t ADPCMSzToPCM16Sz( size_t adpcmbytesz );


    //
    //  ADPCM Encoding/Decoding
    //
    std::vector<pcm16s_t> DecodeADPCM_IMA( const std::vector<uint8_t>  & rawadpcmdata,
                                           unsigned int                  nbchannels   = 1 );

    std::vector<uint8_t>  EncodeADPCM_IMA( const std::vector<pcm16s_t> & pcmdata, 
                                           unsigned int                  nbchannels   = 1 );


    //
    //  NDS ADPCM
    //
    /*
        DecodeADPCM_NDS
            The NDS's ADPCM format has a slight difference in the way it clamps the 
            sample's values. 
    */
    std::vector<pcm16s_t> DecodeADPCM_NDS( const std::vector<uint8_t>  & rawadpcmdata,
                                           unsigned int                  nbchannels   = 1 );

};

#endif