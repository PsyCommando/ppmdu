#include "pmd2_filetypes.hpp"
#include <types/content_type_analyser.hpp>
#include <string>
#include <vector>
#include <algorithm>
using std::vector;
using std::string;
using namespace filetypes;



namespace pmd2 { namespace filetypes
{ 

    struct filext_and_ty
    {
        cnt_t   type;
        string  filext;
    };

    //#TODO: actually put this directly in the file format analyser rules !!! To keep everything together!
    static const vector<filext_and_ty> EXT_TO_TYPES =
    {{
        { static_cast<unsigned int>(e_ContentType::PKDPX_CONTAINER),      PKDPX_FILEX          },
        { static_cast<unsigned int>(e_ContentType::AT4PX_CONTAINER),      AT4PX_FILEX          },
        { static_cast<unsigned int>(e_ContentType::AT4PN_CONTAINER),      AT4PN_FILEX          },
        { static_cast<unsigned int>(e_ContentType::SIR0_CONTAINER),       SIR0_FILEX           },
        { static_cast<unsigned int>(e_ContentType::SIR0_AT4PX_CONTAINER), SIR0_AT4PX_FILEX     },
        { static_cast<unsigned int>(e_ContentType::SIR0_PKDPX_CONTAINER), SIR0_PKDPX_FILEX     },
        { static_cast<unsigned int>(e_ContentType::PACK_CONTAINER),       PACK_FILEX           },
        { static_cast<unsigned int>(e_ContentType::KAOMADO_CONTAINER),    KAOMADO_FILEX        },
        { static_cast<unsigned int>(e_ContentType::WAN_SPRITE_CONTAINER), WAN_FILEX            },
        { static_cast<unsigned int>(e_ContentType::WTE_FILE),             WTE_FILEX            },
        { static_cast<unsigned int>(e_ContentType::WTU_FILE),             WTU_FILEX            },
        { static_cast<unsigned int>(e_ContentType::BGP_FILE),             BGP_FILEX            },
        { static_cast<unsigned int>(e_ContentType::RAW_RGBX32_PAL_FILE),  RGBX32_RAW_PAL_FILEX },
        { static_cast<unsigned int>(e_ContentType::MONSTER_MD_FILE),      MD_FILEX             },

        //DSE
        { static_cast<unsigned int>(e_ContentType::SMDL_FILE),            SMDL_FILEX           },
        { static_cast<unsigned int>(e_ContentType::SWDL_FILE),            SWDL_FILEX           },
        { static_cast<unsigned int>(e_ContentType::SEDL_FILE),            SEDL_FILEX           },
    }};


    std::vector<cnt_t> GetFileTypeFromExtension( const std::string & ext )
    {
        vector<cnt_t> matches;

        //for( const auto & entry : EXT_TO_TYPES )
        //{
        //    if( ( ext.compare( entry.filext ) == 0 ) )
        //        matches.push_back( entry.type );
        //}

        auto ptrf = ContentIDManager::GetInstance().FindMatchingCnt( ext );

        if( ptrf != nullptr )
        {
            matches.push_back( ptrf->id() );
        }

        return std::move( matches );
    }

    std::string GetAppropriateFileExtension( std::vector<uint8_t>::const_iterator & itdatabeg,
                                             std::vector<uint8_t>::const_iterator & itdataend )
    {
        auto result = CContentHandler::GetInstance().AnalyseContent( analysis_parameter(itdatabeg, itdataend) );

        auto ptrf = ContentIDManager::GetInstance().FindMatchingCnt( result._type );
        if( ptrf != nullptr )
            return ptrf->name();

        //for( const auto & extentionty : EXT_TO_TYPES )
        //{
        //    if( result._type == extentionty.type )
        //        return extentionty.filext;
        //}


        //switch(result._type)
        //{
        //case e_ContentType::PKDPX_CONTAINER:
        //    return PKDPX_FILEX;
        //case e_ContentType::AT4PX_CONTAINER:
        //    return AT4PX_FILEX;
        //case e_ContentType::SIR0_CONTAINER:
        //    {
        //        //if( !result._hierarchy.empty() && (result._hierarchy.front()._type == e_ContentType::SPRITE_CONTAINER) )
        //        //    return WAN_FILEX; //Deprecate this way of doing things..
        //        //else
        //            return SIR0_FILEX;
        //    }
        //case e_ContentType::WAN_SPRITE_CONTAINER:
        //    return WAN_FILEX;

        //case e_ContentType::BGP_FILE:
        //    return BGP_FILEX;
        //case e_ContentType::WTE_FILE:
        //    return WTE_FILEX;
        //};

        return string();
    }

    //#FIXME: Is this even used anymore ?
    //Returns a short string identifying what is the type of content is in this kind of file !
    std::string GetContentTypeName( e_ContentType type )
    {
        //#TODO: Maybe do something a little less reliant on having to change things in several places?
        switch(type)
        {
        case e_ContentType::WAN_SPRITE_CONTAINER:
            return "WAN";
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