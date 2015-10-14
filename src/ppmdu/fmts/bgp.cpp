#include "bgp.hpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ppmdu/fmts/at4px.hpp>
#include <ppmdu/containers/tiled_image.hpp>
#include <ppmdu/containers/linear_image.hpp>
#include <ext_fmts/png_io.hpp>
#include <ext_fmts/bmp_io.hpp>
#include <ext_fmts/supported_io.hpp>

using namespace std;

namespace filetypes
{
    static const ContentTy CnTy_BGP{"bgp"}; //Content ID handle
    static const size_t    PaletteByteLength = 64;//bytes
    static const size_t    PaletteNbColors   = 16;
    static const size_t    BGPTileNbPix      = 64; //pixels 8x8
    static const size_t    BGPTileNbBytes    = BGPTileNbPix/2; //bytes

    //Default sizes
    const size_t BGPDefPalByteLen       = 1024;
    const size_t BGPDefPalNbCol         = (BGPDefPalByteLen / 4);//colors
    const size_t BGPDefTilesByteLen     = 32768;
    const size_t BGPDefTilesNB          = (BGPDefTilesByteLen / BGPTileNbBytes ); //tiles
    const size_t BGPDefTileMapByteLen   = 2048; 
    const size_t BGPDefTileMapNbEntries = BGPDefTileMapByteLen / 2;

//============================================================================
//  BGPParser
//============================================================================
    class BGPParser
    {
    public:
        BGPParser(const string & filepath, bool islittleendian = true)
            :m_filepath(filepath),m_littleendian(islittleendian)
        {}

        BGP Parse()
        {
            DoParse();
            return move( m_out );
        }

    private:
        static BGP::tilemapdata DecodeTileMappingData( uint16_t entry )
        {
            return move( BGP::tilemapdata{  entry & 0x3FF,              //0000 0011 1111 1111, tile index
                                            (entry & 0xF000) >> 12,     //1111 0000 0000 0000, pal index
                                            (entry & 0x800) > 0,        //0000 1000 0000 0000, vflip
                                            (entry & 0x400) > 0} );     //0000 0100 0000 0000, hflip 
        }

        void DoParse()
        {
            DecompressBGP();
            m_hdr.ReadFromContainer( m_bgpdata.begin() );
            ParsePalette();
            ParseTileMapping();
            ParseTiles();
        }

        void DecompressBGP()
        {
            vector<uint8_t> filedata = utils::io::ReadFileToByteVector( m_filepath );
            DecompressAT4PX( filedata.begin(), filedata.end(), m_bgpdata );
        }

        void ParsePalette()
        {
            if( m_hdr.palbeg != bgp_header::LENGTH )
                throw runtime_error( "BGPParser::operator BGP(): BGP header's palbeg field doesn't match the expected value !" );

            //Read palette
            if( (m_hdr.pallen % 16) != 0 )
            {
                stringstream sstr;
                sstr << "BGPParser::operator BGP(): Palette length is not divisible by 16 !";
                throw runtime_error( sstr.str() );
            }

            auto         itreadpal  = m_bgpdata.begin() + m_hdr.palbeg;
            const size_t nbpalettes = m_hdr.pallen / PaletteByteLength;

            m_out.m_palettes.resize(nbpalettes);

            for( auto & apal : m_out.m_palettes )
            {
                apal.resize(PaletteNbColors);
                for( size_t cntcol = 0; cntcol < PaletteNbColors; ++cntcol )
                    itreadpal = apal[cntcol].ReadAsRawByte(itreadpal);
            }
        }

        void ParseTileMapping()
        {
            //Read tile mapping data
            auto         itreadmap        = m_bgpdata.begin() + m_hdr.tmapdatptr;
            const size_t nbtilemapentries = m_hdr.tmapdatlen / sizeof(uint16_t);

            m_out.m_mappingdat.resize(nbtilemapentries);

            for( size_t cnttme = 0; cnttme < nbtilemapentries; ++cnttme )
            {
                m_out.m_mappingdat[cnttme] = move( DecodeTileMappingData( utils::ReadIntFromByteVector<uint16_t>(itreadmap) ) ); //iterator incremented
            }
        }

        void ParseTiles()
        {
            //Read tiles
            auto         itreadtiles = m_bgpdata.begin() + m_hdr.tilesptr;
            const size_t nbtiles     = m_hdr.tileslen / BGPTileNbBytes;

            if( (m_hdr.tileslen % 2) != 0 )
            {
                stringstream sstr;
                sstr << "BGPParser::operator BGP(): Tile data length is not divisible by 2 !";
                throw runtime_error( sstr.str() );
            }

            m_out.m_tiles.resize(nbtiles);

            for( size_t cnttiles = 0; cnttiles < nbtiles; ++cnttiles )
            {
                m_out.m_tiles[cnttiles].resize(BGPTileNbPix);
                for( size_t cntpix = 0; cntpix < BGPTileNbPix; cntpix+=2, ++itreadtiles )
                {
                    auto & curpix  = m_out.m_tiles[cnttiles][cntpix];
                    auto & curpix2 = m_out.m_tiles[cnttiles][cntpix + 1];

                    if( m_littleendian )
                    {
                        curpix.pixeldata  = ((*itreadtiles) & 0x0F);
                        curpix2.pixeldata = ((*itreadtiles) & 0xF0) >> 4;
                    }
                    else
                    {
                        curpix.pixeldata  = ((*itreadtiles) & 0xF0) >> 4;
                        curpix2.pixeldata = ((*itreadtiles) & 0x0F);
                    }
                }
            }
        }

    private:
        string          m_filepath;
        BGP             m_out;
        bgp_header      m_hdr;
        vector<uint8_t> m_bgpdata;
        bool            m_littleendian;
    };


//============================================================================
//  BGPWriter
//============================================================================
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


//============================================================================
//
//============================================================================
    BGP ParseBGP( const std::string & fpath )
    {
        return move( BGPParser(fpath).Parse() );
    }

    void WriteBGP( const BGP & img, const std::string & fpath )
    {
        BGPWriter(img).Write(fpath);
    }


    /*
    */
    void ExportBGP( const BGP & bgpimg, const std::string & outf, utils::io::eSUPPORT_IMG_IO imgty )
    {
        using namespace gimg;

        tiled_image_i8bpp target( BGP_RES.width, BGP_RES.height );

        //Fill Palette
        target.setNbColors( bgpimg.m_palettes.size() * PaletteNbColors );
        auto & targetpal = target.getPalette();
        size_t cntcol = 0;

        for( const auto & pal : bgpimg.m_palettes )
        {
            for( const auto & acol : pal )
            {
                targetpal[cntcol] = acol.getAsRGB24();
                ++cntcol;
            }
        }
        
        //We convert the 4bpp pixels to 8bpp based on the current palette index
        size_t cntouttiles = 0;
        for( const auto & tilemapdat : bgpimg.m_mappingdat )
        {
            if( tilemapdat.tileindex == 0 )
                break;  //When we hit a tile using the first null tile, that means nothing is left to copy !

            const auto & curtile = bgpimg.m_tiles   [tilemapdat.tileindex];
            //const auto & curpal  = bgpimg.m_palettes[tilemapdat.palindex];
            auto       & outtile = target.getTile( cntouttiles );

            for( size_t cntpix = 0; cntpix < curtile.size(); ++cntpix )
            {
                outtile[cntpix] = curtile[cntpix].pixeldata + ( tilemapdat.palindex * PaletteNbColors ); //Get a color index in the 256 color palette
            }

            if( tilemapdat.hflip )
                outtile.flipH();

            if( tilemapdat.vflip )
                outtile.flipV();

            ++cntouttiles;
        }

        bool result = false;
        switch(imgty)
        {
            case utils::io::eSUPPORT_IMG_IO::BMP:
            {
                result = utils::io::ExportToBMP( target, outf );
                break;
            }
            case utils::io::eSUPPORT_IMG_IO::PNG:
            {
                result = utils::io::ExportToPNG( target, outf );
                break;
            }
            case utils::io::eSUPPORT_IMG_IO::INVALID:
            default:
            {
                clog << "ExportBGP(): Invalid image format!!!!\n";
                assert(false);
                break;
            }
        };
        
        //Export the resulting 8bpp image
        if( !result )
        {
            throw runtime_error( "ExportBGP(): Couldn't write PNG image to path specified \"" + outf + "\" !" );
        }
    }

    BGP ImportBGP( const std::string & infile )
    {
        using namespace gimg;
        tiled_image_i8bpp img( BGP_RES.width, BGP_RES.height );

        auto imgty = utils::io::GetSupportedImageType(infile);

        switch(imgty)
        {
            case utils::io::eSUPPORT_IMG_IO::BMP:
            {
                utils::io::ImportFromBMP( img, infile, BGP_RES.width );
                break;
            }
            case utils::io::eSUPPORT_IMG_IO::PNG:
            {
                utils::io::ImportFromPNG( img, infile, BGP_RES.width );
                break;
            }
            case utils::io::eSUPPORT_IMG_IO::INVALID:
            default:
            {
                clog << "ImportBGP(): Invalid image format!!!!\n";
                assert(false);
                break;
            }
        };

        BGP target;
        target.m_palettes.resize(BGPDefPalNbCol, vector<colorRGBX32>(PaletteNbColors));
        target.m_mappingdat.resize(BGPDefTileMapNbEntries, {0,0,0,0} );
        target.m_tiles.resize(BGPDefTilesNB, vector<pixel_indexed_4bpp>(BGPTileNbPix) );


        //#TODO: Need to find a way to sort colors so that each tiles refers to exactly one 16 colors palette..

        //#1 - look at every 64 pixels tile, and make sure each has no more than 16 colors in use. 
        //     if there are any extra colors, try to replace them with the closest within a certain limit.
        //     If no closest color can be found, error out!
        //

        //for( size_t cntcol = 0; cntcol < img.getNbColors(); ++cntcol )
        //{
        //    target.m_palettes[cntcol / PaletteNbColors][cntcol % PaletteNbColors].setFromRGB24( img.getColor(cntcol) );
        //}

        assert(false);
        return move(target);
    }
};