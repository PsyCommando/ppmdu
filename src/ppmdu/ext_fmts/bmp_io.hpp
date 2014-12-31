#ifndef BMP_IO_HPP
#define BMP_IO_HPP
/*
bmp_io.hpp
2014/12/24
psycommando@gmail.com
Description: Interface for loading bmp files into common containers, encapsulating the library used to read it.
*/
#include <ppmdu/containers/tiled_image.hpp>
#include <string>

namespace utils{ namespace io
{
    static const std::string BMP_FileExtension = "bmp";

    //#TODO: Make the return type a "gimg::tiled_image_i4bpp" instead.
    bool ImportFrom4bppBMP( gimg::tiled_image_i4bpp & out_indexed,
                            const std::string       & filepath );


    bool ExportTo4bppBMP( const gimg::tiled_image_i4bpp & in_indexed,
                          const std::string             & filepath );
};};

#endif