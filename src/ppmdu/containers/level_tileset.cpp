#include "level_tileset.hpp"
#include <ext_fmts/png_io.hpp>
#include <ppmdu/containers/tiled_image.hpp>
#include <sstream>
#include <fstream>
#include <iterator>
using namespace std;

namespace pmd2
{

    const std::string FnameUpperScrData = "upper";
    const std::string FnameLowerScrData = "lower";



//
//
//
    void ExportTilesetToRaw(const std::string & destdir, const std::string & basename, const Tileset & tset )
    {
        stringstream sstrimg;
        stringstream sstrtmap;
        stringstream sstrpal;
        stringstream sstrsecpal;
        stringstream sstrpalindextbl;
        sstrimg  <<utils::TryAppendSlash(destdir) <<basename;
        sstrtmap <<sstrimg.str() <<"_tilemap.bin";
        sstrpal  <<sstrimg.str() <<"_mainpal.rgbx32";
        sstrsecpal  <<sstrimg.str() <<"_secpal.rgbx32";
        sstrpalindextbl  <<sstrimg.str() <<"_secpalidxtbl.bin";
        sstrimg  <<"_img.8bpp";

        utils::DoCreateDirectory(destdir);

        //#1 export the image data
        {
            ofstream ofimg(sstrimg.str(), ios::binary | ios::out );
            ofimg.exceptions(ios::badbit);
            ostreambuf_iterator<char> itoutimg(ofimg);
            for( const auto & tile : tset.Tiles() )
            {
                for( const auto & pixel : tile )
                    utils::WriteIntToBytes( static_cast<uint8_t>(pixel), itoutimg );
            }
        }

        //#2 export the tiling data
        {
            ofstream oftile(sstrtmap.str(), ios::binary | ios::out );
            oftile.exceptions(ios::badbit);
            ostreambuf_iterator<char> itouttile(oftile);
            for( const auto & word : tset.TileMap() )
                utils::WriteIntToBytes( static_cast<uint16_t>(word), itouttile );
        }
        //#3 export the palette
        {
            {
                ofstream ofpal(sstrpal.str(), ios::binary | ios::out );
                ofpal.exceptions(ios::badbit);
                ostreambuf_iterator<char> itoutpal(ofpal);
                for( const auto & pal : tset.Palettes().mainpals )
                {
                    for( const auto & color : pal )
                    {
                        color.WriteAsRawByte(itoutpal);
                    }
                }
                ofpal.close();
            }
            {
                ofstream ofsecpal(sstrsecpal.str(), ios::binary | ios::out );
                ofsecpal.exceptions(ios::badbit);
                ostreambuf_iterator<char> itoutsecpal(ofsecpal);
                for( const auto & color : tset.Palettes().palette2 )
                {
                    color.WriteAsRawByte(itoutsecpal);
                }
                ofsecpal.close();
            }
            ofstream ofsecpalidxtbl(sstrpalindextbl.str(), ios::binary | ios::out );
            ofsecpalidxtbl.exceptions(ios::badbit);
            ostreambuf_iterator<char> itoutidxtbl(ofsecpalidxtbl);
            for( const auto & w : tset.Palettes().dba )
            {
                utils::WriteIntToBytes(w.unk3, itoutidxtbl );
                utils::WriteIntToBytes(w.unk4, itoutidxtbl );
            }
        }
    }

//
//
//
    void ExportTilesetPairToRaw(const std::string & destdir, const Tileset * pupscrtset, const Tileset * plowscrtset)
    {
        if( !pupscrtset && !plowscrtset )
        {
            assert(false);
            throw std::logic_error("ExportTilesetPair(): Both tilesets are null!");
        }

        if(pupscrtset)
        {
            ExportTilesetToRaw(destdir, FnameUpperScrData, *pupscrtset);
        }

        if(plowscrtset)
        {
            ExportTilesetToRaw(destdir, FnameLowerScrData, *plowscrtset);
        }

        //! #TODO
    }

    //
    void CopyATile( const pmd2::tileproperties & curtmap, const std::vector<std::vector<gimg::pixel_indexed_4bpp>> & tiles, gimg::tiled_image_i8bpp::tile_t & outitle )//_outit & itout )
    {
        static const size_t tilesqrtres         = 8;
        static const size_t NbBytesPer4bppTile  = 64;
        static const size_t NbColors4bpp        = 16;

        const std::vector<gimg::pixel_indexed_4bpp> * curtile = nullptr;
        if( curtmap.tileindex >= tiles.size() )
        {
            curtile = &(tiles.front());
        }
        else
            curtile = &(tiles[curtmap.tileindex]);


        size_t cntoutpix = 0;
        //_init itcurtile = std::next(itbegcnt,tileindex);
        if(!curtmap.hflip && !curtmap.vflip)
        {
            for( size_t i = 0; i < NbBytesPer4bppTile; ++i, ++cntoutpix )
            {
                uint8_t curby = (*curtile)[i];
                outitle[cntoutpix] = (curby & 0xF) + (curtmap.palindex * NbColors4bpp);
            }
        }
        else
        {
            const int InitRowCnt= (!curtmap.vflip)? 0 : tilesqrtres;
            const int TargetRow = (!curtmap.vflip)? tilesqrtres : 0;
            const int StepRow   = (!curtmap.vflip)? 1 :-1;
            const int CntRAdjust= (!curtmap.vflip)? 0 :-1; //Modifier added to the row counter, when interpreting its value on an index starting at 0

            for( int cntrow = InitRowCnt; cntrow != TargetRow; cntrow += StepRow )
            {
                size_t currowbeg = ((cntrow + CntRAdjust) * tilesqrtres);
                int InitColCnt= (!curtmap.hflip)? 0 : tilesqrtres;
                int TargetCol = (!curtmap.hflip)? tilesqrtres : 0;
                int StepCol   = (!curtmap.hflip)? 1 :-1;
                int CntCAdjust= (!curtmap.hflip)? 0 :-1; //Modifier added to the column counter, when interpreting its value on an index starting at 0

                for( int cntcol = InitColCnt; cntcol != TargetCol; cntcol += StepCol, ++cntoutpix )
                {
                    size_t curpos = (cntcol + CntCAdjust) + currowbeg;
                    uint8_t curby = (*curtile)[curpos];
                    outitle[cntoutpix] = (curby & 0xF) + (curtmap.palindex * NbColors4bpp);
                }

            }
        }
    }

    std::vector<std::vector<pmd2::tileproperties>> TileTileMaps( const Tileset & tileset )
    {
        const uint16_t tmgrpsz = tileset.BMAData().unk1 * tileset.BMAData().unk2;

        //Make sure the nb of tiles is divisible by 9!
        size_t nbtmgrpztoalloc = (tileset.TileMap().size() % tmgrpsz != 0)? 
                                 (tileset.TileMap().size() / tmgrpsz) + 1 :
                                 (tileset.TileMap().size() / tmgrpsz);

        if( nbtmgrpztoalloc < tmgrpsz )
            nbtmgrpztoalloc = tmgrpsz; //Clamp to minimum 9 of!

        std::vector<std::vector<pmd2::tileproperties>> tiledtmaps( nbtmgrpztoalloc, std::vector<pmd2::tileproperties>(tmgrpsz) );

        for( size_t cntt = 0; cntt < tileset.TileMap().size(); ++cntt )
        {
            size_t desttile    = cntt / tmgrpsz;
            size_t destsubtile = cntt % tmgrpsz;
            tiledtmaps[desttile][destsubtile] = tileset.TileMap()[cntt];
        }

        return std::move(tiledtmaps);
    }

    gimg::tiled_image_i8bpp PreparePixels_TiledTiles(const Tileset & tileset)
    {
        const size_t tilegrpnbtiles = 9;
        const size_t tilegrpwidth   = 3;
        const size_t tilegrpheight  = 3;
        gimg::tiled_image_i8bpp assembledimg;
        //Make sure the nb of tiles is divisible by the tiles group dimensions!
        size_t imgtileswidth  = (tileset.BMAData().width % tilegrpwidth != 0)? 
                                (tileset.BMAData().width % tilegrpwidth) + (tileset.BMAData().width) :
                                (tileset.BMAData().width);
        size_t imgtilesheight = (tileset.BMAData().height % tilegrpheight != 0)? 
                                (tileset.BMAData().height % tilegrpheight) + (tileset.BMAData().height) :
                                (tileset.BMAData().height);

        
        assembledimg.setNbTilesRowsAndColumns(imgtileswidth, imgtilesheight);

        //Tile in 9x9 tiles, tiles.
        std::vector<std::vector<pmd2::tileproperties>> tiledtmap(TileTileMaps(tileset));

        //const size_t nbtgrpperrow       = imgtileswidth / tilegrpwidth;
        const size_t totaltiles         = tiledtmap.size() * tiledtmap.front().size();
        const size_t ntmgrpperrowtotal  = tileset.BMAData().width / 3;
        //const size_t nbsubtmappertmgrprow = nbtgrpperrow * tilegrpnbtiles;

        size_t cnttotalsubtiles = 0; 

        //Iterate and copy to the structured image
        for( size_t cnttmpgrp = 0; (cnttmpgrp < tiledtmap.size()) && (cnttotalsubtiles < tileset.Tiles().size());  )
        {
            //Iterate on all individual rows of all the tmap groups on this row
            for( size_t cnttmgrprow = 0; cnttmgrprow < tilegrpheight; ++cnttmgrprow )
            {
                //Iterate on an individual tmap group row of 3x3 tiles linearly as a single 1D array
                for( size_t cntsubtiles = 0; 
                    (cntsubtiles < ntmgrpperrowtotal) && 
                    (cnttotalsubtiles < tileset.Tiles().size()); 
                    ++cntsubtiles, ++cnttotalsubtiles )
                {
                    size_t indextmapgrp = (cntsubtiles / tilegrpwidth) % ntmgrpperrowtotal; //This gives us what of the few current tilemap groups we're in.
                    size_t indexsubtm   = (cntsubtiles - (indextmapgrp * tilegrpwidth) ) + (cnttmgrprow * tilegrpwidth);

                    cout <<"Accessing sub-tile number " <<cnttotalsubtiles <<" ( " <<cnttmpgrp + indextmapgrp <<", " <<indexsubtm <<" )\n";

                    CopyATile( tiledtmap[cnttmpgrp + indextmapgrp][indexsubtm], tileset.Tiles(), assembledimg.getTile(cnttotalsubtiles) );
                }
            }
            cnttmpgrp += ntmgrpperrowtotal;
        }

        return std::move(assembledimg);
    }


    gimg::tiled_image_i8bpp PreparePixels_TiledMapPixelByPixel(const Tileset & tileset)
    {
        const size_t tilegrpnbtiles = 9;
        const size_t tilegrpwidth   = 3;
        const size_t tilegrpheight  = 3;
        const size_t tilesqrt       = 8;
        static const size_t NbColors4bpp        = 16;
        //Set image pixel by pixel from the tilemap
        gimg::tiled_image_i8bpp assembledimg;

        //Tile in 9x9 tiles, tiles.
        std::vector<std::vector<pmd2::tileproperties>> tiledtmap(TileTileMaps(tileset));

        //Try a few different values here
        size_t imgtileswidth  = (tileset.BMAData().width );
        size_t imgtilesheight = (tileset.BMAData().height);

        assembledimg.setNbTilesRowsAndColumns(imgtileswidth, imgtilesheight);

        for(uint16_t pixy = 0; pixy < assembledimg.height(); ++pixy)
        {
            for(uint16_t pixx = 0; pixx < assembledimg.width(); ++pixx)
            {
                //Absolute tile coordinate
                size_t abstlx = (pixx / tilesqrt);
                size_t abstly = (pixy / tilesqrt);
                size_t abstileindex = (abstly * imgtileswidth) + abstlx;                    //Index of the current pixel tile containing the specified pixel coordinates, in absolute nb of tiles
                
                //Absolute tile group coordinate
                size_t abstgrpindex = abstileindex / tilegrpnbtiles;                        //Index of the current tile group containing the specified pixel coordinates, in absolute nb of tile groups

                //Relative coord
                size_t reltgrptileidx = abstileindex - (abstgrpindex * tilegrpnbtiles);                      //index of a PixelTile within a tilemap group (0-8)
                size_t reltlpixidx    = ((pixy % tilesqrt) * tilesqrt) + (pixx % tilesqrt); //Index of a pixel within the current 8x8 pixel tile (0-63)

                if( abstgrpindex < tiledtmap.size() )
                {
                    const auto & curtmap = tiledtmap[abstgrpindex][reltgrptileidx];
                    if(curtmap.tileindex < tileset.Tiles().size())
                    {
                        gimg::tiled_image_i8bpp::tile_t tile;
                        for(size_t cntpix = 0; cntpix < tileset.Tiles()[curtmap.tileindex].size(); ++cntpix )
                            tile[cntpix] = tileset.Tiles()[curtmap.tileindex][cntpix] + (curtmap.palindex * NbColors4bpp);
                        if(curtmap.hflip)
                            tile.flipH();
                        if(curtmap.vflip)
                            tile.flipV();
                        assembledimg.getPixel(pixx, pixy) = tile[reltlpixidx];
                        cout <<"Img(" <<pixx <<", " <<pixy <<") -> Tilegrp: " <<abstgrpindex <<", tile#: " <<reltgrptileidx <<", pixel index: " <<reltlpixidx <<"\n";
                    }
                }
                else
                {
                    cerr<<"Bad tgrp index " <<abstgrpindex <<" for pixel coord ( " <<pixx <<", " <<pixy <<" )\n";
                }
            }
        }

        return std::move(assembledimg);
    }



    void PrintAssembledTilesetPreviewToPNG(const std::string & fpath, const Tileset & tileset)
    {
        gimg::tiled_image_i8bpp assembledimg(PreparePixels_TiledMapPixelByPixel(tileset));
        //gimg::tiled_image_i8bpp assembledimg(PreparePixels_TiledTiles(tileset));
        assembledimg.setNbColors(256);

        //Fill the color palette
        for( size_t cntpal = 0; cntpal < 16; ++cntpal )
        {
            for( size_t cntc = 0; cntc < 16; ++cntc ) 
            {
                auto & curcol = assembledimg.getPalette()[(cntpal * 16) + cntc];
                auto & cursrc = tileset.Palettes().mainpals[cntpal][cntc];
                curcol.red   = cursrc._red;
                curcol.green = cursrc._green;
                curcol.blue  = cursrc._blue;
            }
        }

        utils::io::ExportToPNG(assembledimg, fpath);
    }


    void DumpTilesToPNG(const std::string & fpath, const Tileset & tileset)
    {
    }
};
