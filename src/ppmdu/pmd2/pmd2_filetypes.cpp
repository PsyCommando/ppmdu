#include "pmd2_filetypes.hpp"
#include <ppmdu/fmts/content_type_analyser.hpp>
#include <string>
#include <vector>
#include <algorithm>
using std::vector;
using std::string;

namespace pmd2 { namespace filetypes
{ 
    using namespace magicnumbers;


    std::string GetAppropriateFileExtension( std::vector<uint8_t>::const_iterator & itdatabeg,
                                             std::vector<uint8_t>::const_iterator & itdataend )
    {
        auto result = pmd2::filetypes::CContentHandler::GetInstance().AnalyseContent( analysis_parameter(itdatabeg, itdataend) );

        switch(result._type)
        {
        case e_ContentType::PKDPX_CONTAINER:
            return PKDPX_FILEX;
        case e_ContentType::AT4PX_CONTAINER:
            return AT4PX_FILEX;
        case e_ContentType::SIR0_CONTAINER:
            {
                if( !result._hierarchy.empty() && (result._hierarchy.front()._type == e_ContentType::SPRITE_CONTAINER) )
                    return WAN_FILEX;
                else
                    return SIR0_FILEX;
            }
        case e_ContentType::SPRITE_CONTAINER:
            return WAN_FILEX;
        };

        return string();
    }

    //Returns a short string identifying what is the type of content is in this kind of file !
    std::string GetContentTypeName( e_ContentType type )
    {
        //#TODO: Maybe do something a little less reliant on having to change things in several places?
        switch(type)
        {
        case e_ContentType::SPRITE_CONTAINER:
            return "SpriteData";
        case e_ContentType::PKDPX_CONTAINER:
            return "PKDPX";
        case e_ContentType::AT4PX_CONTAINER:
            return "AT4PX";
        //case e_ContentType::PALETTE_16_COLORS:
        //    return "Palette-16_Colors";
        case e_ContentType::SIR0_CONTAINER:
            return "SIR0";
        case e_ContentType::COMPRESSED_DATA:
            return "CompressedData";
        case e_ContentType::UNKNOWN_CONTENT:
        default:
            return "Unknown";
        };
    }

};};