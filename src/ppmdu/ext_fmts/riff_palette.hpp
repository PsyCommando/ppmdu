#ifndef RIFF_PALETTE_HPP
#define RIFF_PALETTE_HPP
/*
riff_palette.hpp
2014/10/10
psycommando@gmail.com
Description: Utilities for importing and exporting RIFF color palettes.
*/
#include <cstdint>
#include <ppmdu\containers\color.hpp>
#include <vector>

namespace utils
{

    //==============================================================================
    //  RGB24
    //==============================================================================
    //Import
    void ImportFrom_RIFF_Palette( const std::vector<uint8_t> & in_riffpalette, std::vector<gimg::colorRGB24> & out_palette  );

    //Export
    void ExportTo_RIFF_Palette( const std::vector<gimg::colorRGB24> & in_palette, std::vector<uint8_t> & out_riffpalette );
};

#endif