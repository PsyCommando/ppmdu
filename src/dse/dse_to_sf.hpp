#ifndef DSE_TO_SF_HPP
#define DSE_TO_SF_HPP
/*
dse_to_sf.hpp
2015/07/06
psycommando@gmail.com
Descritpton: Contains utilities for converting DSE audio data to the SoundFont format.
             Made this to isolate DSE specific stuff from the sounfont stuff as much as possible.
*/
#include <ppmdu/fmts/dse_common.hpp>
#include <ppmdu/fmts/dse_sequence.hpp>
#include <ppmdu/ext_fmts/sf2.hpp>
#include <ppmdu/pmd2/pmd2_audio_data.hpp>

namespace dse
{

    /*
        ConvertSWDToSF2
    */
    void ConvertSWDToSF2( const pmd2::audio::PresetBank & bank, const std::string & outfile, const std::string & sfname  );

};

#endif