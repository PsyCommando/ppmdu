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
#include <string>

namespace utils{ namespace io
{
    static const std::string RIFF_PAL_Filext = "pal";

    //==============================================================================
    //  RGB24
    //==============================================================================
    //Import
    std::vector<gimg::colorRGB24> ImportFrom_RIFF_Palette( const std::vector<uint8_t> & in_riffpalette );
    std::vector<gimg::colorRGB24> ImportFrom_RIFF_Palette( const std::string          & inputpath );

    //Export
    void ExportTo_RIFF_Palette( const std::vector<gimg::colorRGB24> & in_palette, std::vector<uint8_t> & out_riffpalette );
    void ExportTo_RIFF_Palette( const std::vector<gimg::colorRGB24> & in_palette, const std::string    & outputpath );
};};

#endif