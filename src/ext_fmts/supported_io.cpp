#include "supported_io.hpp"

#include <utils/utility.hpp>

namespace utils{ namespace io
{
//================================================================================================
// Image Formats Support
//================================================================================================

    eSUPPORT_IMG_IO GetSupportedImageType( const std::string & pathtoimg )
    {
        for( auto & afiletype : SupportedInputImageTypes )
        {
            if( has_suffix( pathtoimg, afiletype.extension ) )
                return afiletype.type;
        }
        return eSUPPORT_IMG_IO::INVALID;
    }
};};