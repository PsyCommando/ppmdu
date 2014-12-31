#ifndef PNG_IO_HPP
#define PNG_IO_HPP
/*
png_io.hpp
2014/11/13
psycommando@gmail.com
Description: Utilities for importing and exporting PNG images and their palettes into the formats used in the lib.
*/
#include <ppmdu/containers/tiled_image.hpp>
#include <string>


namespace utils{ namespace io
{
    static const std::string PNG_FileExtension = "png";

    bool ImportFrom4bppPNG( gimg::tiled_image_i4bpp & out_indexed,
                            const std::string       & filepath );


    bool ExportTo4bppPNG( const gimg::tiled_image_i4bpp & in_indexed,
                          const std::string             & filepath );
};};

#endif