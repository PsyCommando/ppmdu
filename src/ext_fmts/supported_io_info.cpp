#include "supported_io_info.hpp"
#include <ext_fmts/supported_io.hpp>
using namespace std;

namespace utils{ namespace io
{
    image_format_info GetSupportedImageFormatInfo( const std::string & pathtoimg )
    {
        eSUPPORT_IMG_IO imgty = GetSupportedImageType(pathtoimg);
        if( imgty == eSUPPORT_IMG_IO::BMP )
            return GetBMPImgInfo( pathtoimg );
        else if( imgty == eSUPPORT_IMG_IO::PNG )
            return GetPNGImgInfo( pathtoimg );
        else 
            return image_format_info();
    }

};};