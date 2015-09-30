#include "bgp.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ppmdu/fmts/at4px.hpp>

using namespace std;

namespace filetypes
{
    static const ContentTy CnTy_BGP{"bgp"}; //Content ID handle
    static const size_t    PaletteByteLength = 64;//bytes
    static const size_t    PaletteNbColors   = 16;
    static const size_t    BGPTileNbPix      = 64; //pixels 8x8
    static const size_t    BGPTileNbBytes    = BGPTileNbPix/2; //bytes

//
//  BGPParser
//
    class BGPParser
    {
    public:
        BGPParser(const string & filepath)
            :m_filepath(filepath)
        {}

        operator BGP()
        {
            vector<uint8_t> filedata = utils::io::ReadFileToByteVector( m_filepath );
            vector<uint8_t> bgpdata;
            DecompressAT4PX( filedata.begin(), filedata.end(), bgpdata );

            bgp_header hdr;
            BGP        result;
            hdr.ReadFromContainer( bgpdata.begin() );

            if( hdr.palbeg != bgp_header::LENGTH )
                throw runtime_error( "BGPParser::operator BGP(): BGP header's palbeg field doesn't match the expected value !" );

            //Read palette
            if( (hdr.pallen % 16) != 0 )
            {
                stringstream sstr;
                sstr << "BGPParser::operator BGP(): Palette length is not divisible by 16 !";
                throw runtime_error( sstr.str() );
            }

            auto         itreadpal  = bgpdata.begin() + hdr.palbeg;
            const size_t nbpalettes = hdr.pallen / PaletteByteLength;

            result.m_palettes.resize(nbpalettes);

            for( auto & apal : result.m_palettes )
            {
                apal.resize(PaletteNbColors);
                for( size_t cntcol = 0; cntcol < PaletteNbColors; ++cntcol )
                    itreadpal = apal[cntcol].ReadAsRawByte(itreadpal);
            }

            //Read tile mapping data
            auto         itreadmap        = bgpdata.begin() + hdr.tmapdatptr;
            const size_t nbtilemapentries = hdr.tmapdatlen / sizeof(uint16_t);

            result.m_mappingdat.resize(nbtilemapentries);

            for( size_t cnttme = 0; cnttme < nbtilemapentries; ++cnttme )
            {
                result.m_mappingdat[cnttme] = move( DecodeTileMappingData( utils::ReadIntFromByteVector<uint16_t>(itreadmap) ) ); //iterator incremented
            }

            //Read tiles
            auto         itreadtiles = bgpdata.begin() + hdr.tilesptr;
            const size_t nbtiles     = hdr.tileslen / BGPTileNbBytes;

            if( (hdr.tileslen % 2) != 0 )
            {
                stringstream sstr;
                sstr << "BGPParser::operator BGP(): Tile data length is not divisible by 2 !";
                throw runtime_error( sstr.str() );
            }

            result.m_tiles.resize(nbtiles);

            for( size_t cnttiles = 0; cnttiles < nbtiles; ++cnttiles )
            {
                result.m_tiles[cnttiles].resize(BGPTileNbPix);
                for( size_t cntpix = 0; cntpix < BGPTileNbPix; cntpix+=2, ++itreadtiles )
                {
                    auto & curpix  = result.m_tiles[cnttiles][cntpix];
                    auto & curpix2 = result.m_tiles[cnttiles][cntpix + 1];

                    curpix.pixeldata  = ((*itreadtiles) & 0xF0) >> 4;
                    curpix2.pixeldata = ((*itreadtiles) & 0x0F);
                }
            }

        }

    private:
        static BGP::tilemapdata DecodeTileMappingData( uint16_t entry )
        {
            return move( BGP::tilemapdata{  entry & 0x3FF,              //Get the 10 lowest bits
                                            (entry & 0xF000) >> 12,     //Get the highest 4 bits
                                            (entry & 0x400) > 0,        //0000 0100 0000 0000
                                            (entry & 0x800) > 0 } );    //0000 1000 0000 0000
        }

    private:
        string   m_filepath;
        ifstream m_infile;
    };


//
//  BGPWriter
//
    class BGPWriter
    {
    public:
        BGPWriter( const BGP & img )
            :m_img(img)
        {}

        void Write(const string & filepath)
        {
            assert(false);
        }

    private:

    private:
        const BGP & m_img;
    };


//
//
//
    BGP ParseBGP( const std::string & fpath )
    {
        return move( BGPParser(fpath).operator filetypes::BGP() );
    }

    void WriteBGP( const BGP & img, const std::string & fpath )
    {
        BGPWriter(img).Write(fpath);
    }
};