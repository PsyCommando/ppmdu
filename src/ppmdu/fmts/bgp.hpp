#ifndef BGP_HPP
#define BGP_HPP
/*
bgp.hpp
2015/08/28
psycommando@gmail.com
Description: Utilities for handling BGP images.
*/
#include <vector>
#include <string>
#include <cstdint>
#include <types/content_type_analyser.hpp>
#include <utils/utility.hpp>
#include <ppmdu/containers/img_pixel.hpp>

namespace filetypes
{
    extern const ContentTy CnTy_BGP; //Content ID handle
     

    /*
    */
    struct bgp_header
    {
        static const size_t LENGTH = 32; //bytes

        uint32_t palbeg;
        uint32_t pallen;
        uint32_t tilesptr;
        uint32_t tileslen;
        uint32_t tmapdatptr;
        uint32_t tmapdatlen;
        uint32_t bgpunk3;
        uint32_t bgpunk4;

        //
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToByteContainer( palbeg,     itwriteto );
            itwriteto = utils::WriteIntToByteContainer( pallen,     itwriteto );
            itwriteto = utils::WriteIntToByteContainer( tilesptr,   itwriteto );
            itwriteto = utils::WriteIntToByteContainer( tileslen,   itwriteto );
            itwriteto = utils::WriteIntToByteContainer( tmapdatptr, itwriteto );
            itwriteto = utils::WriteIntToByteContainer( tmapdatlen, itwriteto );
            itwriteto = utils::WriteIntToByteContainer( bgpunk3,    itwriteto );
            itwriteto = utils::WriteIntToByteContainer( bgpunk4,    itwriteto );
            return itwriteto;
        }

        //
        template<class _init>
            _init ReadFromContainer(  _init itReadfrom )
        {
            itReadfrom = utils::ReadIntFromByteContainer( palbeg,     itReadfrom );
            itReadfrom = utils::ReadIntFromByteContainer( pallen,     itReadfrom );
            itReadfrom = utils::ReadIntFromByteContainer( tilesptr,   itReadfrom );
            itReadfrom = utils::ReadIntFromByteContainer( tileslen,   itReadfrom );
            itReadfrom = utils::ReadIntFromByteContainer( tmapdatptr, itReadfrom );
            itReadfrom = utils::ReadIntFromByteContainer( tmapdatlen, itReadfrom );
            itReadfrom = utils::ReadIntFromByteContainer( bgpunk3,    itReadfrom );
            itReadfrom = utils::ReadIntFromByteContainer( bgpunk4,    itReadfrom );
            return itReadfrom;
        }
    };

    /*
        BGP
            A class for containing the data from a BGP file.
    */
    class BGP
    {
    public:
        struct tilemapdata
        {
            uint16_t tileindex;
            uint8_t  palindex;
            bool     vflip;
            bool     hflip;
        };


        //#TODO: Encapsulate everything properly

        std::vector< std::vector<gimg::pixel_indexed_4bpp> > m_tiles;
        std::vector<tilemapdata>                             m_mappingdat;
        std::vector< std::vector<gimg::colorRGBX32> >        m_palettes;
    };

    /*
    */
    BGP  ParseBGP( const std::string & fpath );
    void WriteBGP( const BGP         & img, const std::string & fpath );
};

#endif