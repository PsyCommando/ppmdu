#ifndef SUPPORTED_IO_INFO_HPP
#define SUPPORTED_IO_INFO_HPP
/*
supported_io_info.hpp
2015/02/25
psycommando@gmail.com
Description: 
*/
#include <string>

namespace utils{ namespace io
{
    /*
    */
    struct image_format_info
    {
        unsigned int bitdepth    = 0;
        unsigned int width       = 0;
        unsigned int height      = 0;
        bool         usesPalette = false;
    };

    /*
        If is a supported image returns data about it!
        If not a supported image, will return a struct with all the default values.
    */
    image_format_info GetSupportedImageFormatInfo( const std::string & pathtoimg );

};};

#endif