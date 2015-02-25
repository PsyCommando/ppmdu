#ifndef TXT_PALETTE_IO_HPP
#define TXT_PALETTE_IO_HPP
/*
txt_palette_io.hpp
2015/02/22
psycommando@gmail.com
Description: Utilities for reading from and writing to a color palette stored as a text file.
The colors are stored one per line, in html notation, in hexadecimal:
#RRGGBB
*/
#include <string>
#include <vector>
#include <ppmdu/containers/color.hpp>

namespace utils{ namespace io
{
    std::vector<gimg::colorRGB24> ImportFrom_TXT_Palette( const std::string & inputpath );
    void                          ExportTo_TXT_Palette( const std::vector<gimg::colorRGB24> & in_palette, const std::string & outputpath );

};};

#endif