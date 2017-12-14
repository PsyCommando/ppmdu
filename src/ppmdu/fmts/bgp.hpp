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
#include <ext_fmts/supported_io.hpp>

namespace filetypes
{
    extern const ContentTy  CnTy_BGP; //Content ID handle
    const std::string       BGP_FileExt = "bgp";
    const utils::Resolution BGP_RES     = { 256, 192 };

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
            itwriteto = utils::WriteIntToBytes( palbeg,     itwriteto );
            itwriteto = utils::WriteIntToBytes( pallen,     itwriteto );
            itwriteto = utils::WriteIntToBytes( tilesptr,   itwriteto );
            itwriteto = utils::WriteIntToBytes( tileslen,   itwriteto );
            itwriteto = utils::WriteIntToBytes( tmapdatptr, itwriteto );
            itwriteto = utils::WriteIntToBytes( tmapdatlen, itwriteto );
            itwriteto = utils::WriteIntToBytes( bgpunk3,    itwriteto );
            itwriteto = utils::WriteIntToBytes( bgpunk4,    itwriteto );
            return itwriteto;
        }

        //
        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itPastEnd )
        {
            itReadfrom = utils::ReadIntFromBytes( palbeg,     itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( pallen,     itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( tilesptr,   itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( tileslen,   itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( tmapdatptr, itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( tmapdatlen, itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( bgpunk3,    itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( bgpunk4,    itReadfrom, itPastEnd );
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

    /*
    */
    void ExportBGP( const BGP & bgpimg, const std::string & outf, utils::io::eSUPPORT_IMG_IO imgty );
    BGP  ImportBGP( const std::string & infile );
};

#endif