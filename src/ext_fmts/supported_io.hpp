#ifndef SUPPORTED_IO_HPP
#define SUPPORTED_IO_HPP
/*
supported_io.hpp
2014/12/30
psycommando@gmail.com
Description: A file containing common information on the supported input/output file formats.
*/
#include <array>
#include <string>
#include "bmp_io.hpp"
#include "png_io.hpp"
#include "rawimg_io.hpp"

namespace utils{ namespace io
{
//================================================================================================
// Image Formats Support
//================================================================================================
    //The types of images that are supported for import/export
    //The values of each entries matches their slot in the std::array "SupportedInputImageTypes"
    enum struct eSUPPORT_IMG_IO : size_t
    {
        PNG = 0,
        RAW,
        BMP,
        //> Insert new formats here <

        NB_SUPPORTED,
        INVALID,
    };

    //File format support stuff
    struct supportedimg_t
    {
        const std::string extension;
        eSUPPORT_IMG_IO   type;
    };

    //Array with all the supported file types and their extensions.
    static const std::array<supportedimg_t, static_cast<size_t>(eSUPPORT_IMG_IO::NB_SUPPORTED)> SupportedInputImageTypes=
    {{
        { PNG_FileExtension,    eSUPPORT_IMG_IO::PNG },
        { BMP_FileExtension,    eSUPPORT_IMG_IO::BMP },
        { RawImg_FileExtension, eSUPPORT_IMG_IO::RAW },
    }};

    /********************************************************
        GetSupportedImageType
            This returns the type of the image if supported. 
            Otherwise returns "eSUPPORT_IMG_IO::INVALID".
            It only checks the file extension, and does 
            nothing else.
    ********************************************************/
    eSUPPORT_IMG_IO GetSupportedImageType( const std::string & pathtoimg );

    /********************************************************
        IsSupportedImageType
            Return true if the file extension of the 
            file path is supported.
    ********************************************************/
    inline bool IsSupportedImageType( const std::string & pathtoimg )
    {
        return GetSupportedImageType(pathtoimg) != eSUPPORT_IMG_IO::INVALID;
    }



//================================================================================================
// File Formats Support
//================================================================================================
};};

#endif