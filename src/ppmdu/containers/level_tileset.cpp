#include "level_tileset.hpp"
#include <ext_fmts/png_io.hpp>
#include <ppmdu/containers/tiled_image.hpp>
#include <ppmdu/fmts/bpc.hpp>
#include <ppmdu/fmts/bpa.hpp>
#include <ppmdu/fmts/bpl.hpp>
#include <ppmdu/fmts/bma.hpp>
#include <sstream>
#include <fstream>
#include <iterator>
#include <unordered_map>
using namespace std;

namespace pmd2
{

    const std::string FnameUpperLayerData = "upper";
    const std::string FnameLowerLayerData = "lower";

    const std::array<tsetconstants, 3> TSetIndependantSelectConstants
    {{
        //02323920 - When levelid == 0xFFFFFFFF, 
        //           OR when level entry map type is 0,1,2,3,4,5, bigger than 12!
        {
            0x0001, 0x0001, 0x0001, 0x0000, 0x000E, 0x0000, 0x0400, 
            0x0400, 0x0000, 0x00BA, 0x003E, 0x0000, 0x00000000, 
        },

        //0232393C - When level entry map type is 6,7,8,9,10
        {
            0x0001, 0x0000, 0x0002, 0x0000, 0x000E, 0x0000, 0x0400, 
            0x0200, 0x0400, 0x00BA, 0x003E, 0x0000, 0x00000000, 
        },

        //02323958 - When level entry map type is 11,12
        {
            0x0001, 0x0001, 0x0001, 0x0000, 0x000E, 0x0000, 0x0400, 
            0x0400, 0x0000, 0x00BA, 0x003E, 0x0000, 0x00000000, 
        },
    }};

    extern const std::array<tsetconstants, 3> TSetGroundSelectConstants
    {{
        //02320CD8 - When levelid == 0xFFFFFFFF
        //           OR when level entry map type is 0,1,2,3,4,5, bigger than 12!
        {
            0x0000, 0x0001, 0x0001, 0x0000, 0x000E, 0x0000, 0x0400, 
            0x0400, 0x0000, 0x00BA, 0x003E, 0x0000, 0x022F1800,   
        },
    
        //02320CF4 - When level entry map type == 6,7,8,9,10
        {
            0x0000, 0x0000, 0x0002, 0x0000, 0x000E, 0x0000, 0x0400, 
            0x0200, 0x0400, 0x00BA, 0x003E, 0x0000, 0x022F1800,   
        },
    
        //02320D10 - When level entry map type == 11,12
        {
            0x0000, 0x0001, 0x0001, 0x0000, 0x000E, 0x0000, 0x0400, 
            0x0400, 0x0000, 0x00BA, 0x003E, 0x0000, 0x022F1800,
        },
    }};

    extern const std::array<tsetconstants, 2> TSetGroundSubWorldSelectConstants
    {{
        //02323394 - When levelid == 0xFFFFFFFF
        //           OR level list entry map type == 1,2,3,4,5 OR bigger than 12!
        {
            0x0001, 0x0001, 0x0001, 0x0000, 0x000E, 0x0000, 0x0400, 
            0x0400, 0x0000, 0x00BA, 0x003E, 0x0000, 0x00000000, 
        },
    
        //023233B0 - When level list entry map type == 0,6,7,8,9,10,11,12
        {
            0x0001, 0x0000, 0x0002, 0x0000, 0x000E, 0x0000, 0x0400, 
            0x0200, 0x0400, 0x00BA, 0x003E, 0x0000, 0x00000000,   
        },
    }};


//
//
//
    void ExportTilesetToRaw(const std::string & destdir, const std::string & basename, const Tileset & tset )
    {
        stringstream sstrimg;
        //stringstream sstrtmap;
        stringstream sstrpal;
        stringstream sstrsecpal;
        stringstream sstrpalindextbl;
        sstrimg          <<utils::TryAppendSlash(destdir) <<basename;
        //sstrtmap         <<sstrimg.str() <<"_tilemap.bin";
        sstrpal          <<sstrimg.str() <<"_mainpal.rgbx32";
        sstrsecpal       <<sstrimg.str() <<"_secpal.rgbx32";
        sstrpalindextbl  <<sstrimg.str() <<"_secpalidxtbl.bin";
        sstrimg          <<"_img";

        utils::DoCreateDirectory(destdir);

        size_t cntlayer = 0;
        for( const auto & layer : tset.Layers() )
        {
            stringstream curimgname;
            curimgname << sstrimg.str() <<"_l" <<cntlayer;
            //#1 export the image data
            {
                ofstream ofimg(curimgname.str() + ".4bpp", ios::binary | ios::out );
                ofimg.exceptions(ios::badbit);
                ostreambuf_iterator<char> itoutimg(ofimg);
                for( const auto & tile : layer.Tiles() )
                {
                    for( size_t cntpix = 0; cntpix < (tile.size()-1); ++cntpix )
                    {
                        uint8_t pixel4bpp = tile[cntpix] | static_cast<uint8_t>(tile[cntpix + 1]) << 4;
                        utils::WriteIntToBytes( pixel4bpp, itoutimg );
                    }
                }
            }

            //#2 export the tiling data
            {
                stringstream sstrtmap;
                sstrtmap<<curimgname.str() <<"_tilemap.bin";

                ofstream oftile(sstrtmap.str(), ios::binary | ios::out );
                oftile.exceptions(ios::badbit);

                ostreambuf_iterator<char> itouttile(oftile);

                for( const auto & word : layer.TileMap() )
                    utils::WriteIntToBytes( static_cast<uint16_t>(word), itouttile );
            }
            ++cntlayer;
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

    Tileset LoadTileset(const std::string & mapbgdir, const filetypes::LevelBgEntry & tsetinf, const pmd2::level_info & lvlinf)
    {
        Tileset tset;
        string bpcname( tsetinf.bpcname.begin(), tsetinf.bpcname.end() ); //
        bpcname = bpcname.c_str();
        string bplname( tsetinf.bplname.begin(), tsetinf.bplname.end() ); //Use the array pointer, because of the terminating 0 character
        bplname = bplname.c_str();
        string bmaname( tsetinf.bmaname.begin(), tsetinf.bmaname.end() ); //
        bmaname = bmaname.c_str();
        //!#TODO : Handle bpa list!

        bpcname = utils::MakeLowerCase(bpcname);
        bplname = utils::MakeLowerCase(bplname);
        bmaname = utils::MakeLowerCase(bmaname);
        //!#TODO : Handle bpa list!

        auto lambdamakepath = [&mapbgdir](const string & fname, const string & ext)->string
        {
            stringstream sstr;
            sstr <<utils::TryAppendSlash(mapbgdir) <<fname <<"." <<ext;
            return std::move(sstr.str());
        };

        // Load BPC
        tset.Layers() = filetypes::ParseBPC( lambdamakepath(bpcname,filetypes::BPC_FileExt) );

        // Load BMA
        tset.BMAData() = filetypes::ParseBMA( lambdamakepath(bpcname,filetypes::BMA_FileExt) );

        // Load BPA
        //!#TODO : Handle bpa list!
        //tset.BPAData() = filetypes::ParseBPA( lambdamakepath(bpcname,filetypes::BPA_FileExt) );

        // Load BPL
        tset.Palettes() = filetypes::ParseBPL( lambdamakepath(bplname,filetypes::BPL_FileExt) );


        //ExportTilesetToRaw( outpath.makeAbsolute().toString(), tsetinf.bpcname, tset );


        //auto tilesets = ::filetypes::ParseBPC(Poco::Path(inpath).setExtension("bpc").toString());
        //tilesets.first.Palettes()  = ::filetypes::ParseBPL(Poco::Path(inpath).setExtension("bpl").toString());
        //tilesets.second.Palettes() = tilesets.first.Palettes();

        //tilesets.first.BMAData()  = ::filetypes::ParseBMA(Poco::Path(inpath).setExtension("bma").toString());
        //tilesets.second.BMAData() = tilesets.first.BMAData();

        //ExportTilesetPairToRaw( outpath.makeAbsolute().toString(), &tilesets.first, &tilesets.second );
        //if( !(tilesets.first.Tiles().empty()) )
        //{
        //    PrintAssembledTilesetPreviewToPNG( Poco::Path(outpath).append("upper_"  + Poco::Path(inpath).setExtension("png").getFileName()).toString(), tilesets.first);
        //    DumpCellsToPNG( Poco::Path(outpath).append("upper_map_"  + Poco::Path(inpath).setExtension("png").getFileName()).toString(), tilesets.first );
        //}

        //if( !(tilesets.second.Tiles().empty()) )
        //{
        //    PrintAssembledTilesetPreviewToPNG(Poco::Path(outpath).append("lower_"  + Poco::Path(inpath).setExtension("png").getFileName()).toString(), tilesets.second);
        //    DumpCellsToPNG( Poco::Path(outpath).append("lower_map_"  + Poco::Path(inpath).setExtension("png").getFileName()).toString(), tilesets.second );
        //}



        return std::move(tset);
    }

    //
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

    //std::vector<std::vector<pmd2::tileproperties>> TileTileMaps( const Tileset & tileset )
    //{
    //    const uint16_t tmgrpsz = tileset.BMAData().unk1 * tileset.BMAData().unk2;

    //    //Make sure the nb of tiles is divisible by 9!
    //    size_t nbtmgrpztoalloc = (tileset.TileMap().size() % tmgrpsz != 0)? 
    //                             (tileset.TileMap().size() / tmgrpsz) + 1 :
    //                             (tileset.TileMap().size() / tmgrpsz);

    //    if( nbtmgrpztoalloc < tmgrpsz )
    //        nbtmgrpztoalloc = tmgrpsz; //Clamp to minimum 9 of!

    //    std::vector<std::vector<pmd2::tileproperties>> tiledtmaps( nbtmgrpztoalloc, std::vector<pmd2::tileproperties>(tmgrpsz) );

    //    for( size_t cntt = 0; cntt < tileset.TileMap().size(); ++cntt )
    //    {
    //        size_t desttile    = cntt / tmgrpsz;
    //        size_t destsubtile = cntt % tmgrpsz;
    //        tiledtmaps[desttile][destsubtile] = tileset.TileMap()[cntt];
    //    }

    //    return std::move(tiledtmaps);
    //}

    //gimg::tiled_image_i8bpp PreparePixels_TiledTiles(const Tileset & tileset)
    //{
    //    const size_t tilegrpnbtiles = 9;
    //    const size_t tilegrpwidth   = 3;
    //    const size_t tilegrpheight  = 3;
    //    gimg::tiled_image_i8bpp assembledimg;
    //    //Make sure the nb of tiles is divisible by the tiles group dimensions!
    //    size_t imgtileswidth  = (tileset.BMAData().width % tilegrpwidth != 0)? 
    //                            (tileset.BMAData().width % tilegrpwidth) + (tileset.BMAData().width) :
    //                            (tileset.BMAData().width);
    //    size_t imgtilesheight = (tileset.BMAData().height % tilegrpheight != 0)? 
    //                            (tileset.BMAData().height % tilegrpheight) + (tileset.BMAData().height) :
    //                            (tileset.BMAData().height);

    //    
    //    assembledimg.setNbTilesRowsAndColumns(imgtileswidth, imgtilesheight);

    //    //Tile in 9x9 tiles, tiles.
    //    std::vector<std::vector<pmd2::tileproperties>> tiledtmap(TileTileMaps(tileset));

    //    //const size_t nbtgrpperrow       = imgtileswidth / tilegrpwidth;
    //    const size_t totaltiles         = tiledtmap.size() * tiledtmap.front().size();
    //    const size_t ntmgrpperrowtotal  = tileset.BMAData().width / 3;
    //    //const size_t nbsubtmappertmgrprow = nbtgrpperrow * tilegrpnbtiles;

    //    size_t cnttotalsubtiles = 0; 

    //    //Iterate and copy to the structured image
    //    for( size_t cnttmpgrp = 0; (cnttmpgrp < tiledtmap.size()) && (cnttotalsubtiles < tileset.Tiles().size());  )
    //    {
    //        //Iterate on all individual rows of all the tmap groups on this row
    //        for( size_t cnttmgrprow = 0; cnttmgrprow < tilegrpheight; ++cnttmgrprow )
    //        {
    //            //Iterate on an individual tmap group row of 3x3 tiles linearly as a single 1D array
    //            for( size_t cntsubtiles = 0; 
    //                (cntsubtiles < ntmgrpperrowtotal) && 
    //                (cnttotalsubtiles < tileset.Tiles().size()); 
    //                ++cntsubtiles, ++cnttotalsubtiles )
    //            {
    //                size_t indextmapgrp = (cntsubtiles / tilegrpwidth) % ntmgrpperrowtotal; //This gives us what of the few current tilemap groups we're in.
    //                size_t indexsubtm   = (cntsubtiles - (indextmapgrp * tilegrpwidth) ) + (cnttmgrprow * tilegrpwidth);

    //                cout <<"Accessing sub-tile number " <<cnttotalsubtiles <<" ( " <<cnttmpgrp + indextmapgrp <<", " <<indexsubtm <<" )\n";

    //                CopyATile( tiledtmap[cnttmpgrp + indextmapgrp][indexsubtm], tileset.Tiles(), assembledimg.getTile(cnttotalsubtiles) );
    //            }
    //        }
    //        cnttmpgrp += ntmgrpperrowtotal;
    //    }

    //    return std::move(assembledimg);
    //}


    //gimg::tiled_image_i8bpp PreparePixels_TiledMapPixelByPixel(const Tileset & tileset)
    //{
    //    const size_t tilegrpnbtiles = 9;
    //    const size_t tilegrpwidth   = 3;
    //    const size_t tilegrpheight  = 3;
    //    const size_t tilesqrt       = 8;
    //    static const size_t NbColors4bpp        = 16;
    //    //Set image pixel by pixel from the tilemap
    //    gimg::tiled_image_i8bpp assembledimg;

    //    //Tile in 9x9 tiles, tiles.
    //    std::vector<std::vector<pmd2::tileproperties>> tiledtmap(TileTileMaps(tileset));

    //    //Try a few different values here
    //    size_t imgtileswidth  = (tileset.BMAData().width );
    //    size_t imgtilesheight = (tileset.BMAData().height);

    //    assembledimg.setNbTilesRowsAndColumns(imgtileswidth, imgtilesheight);

    //    for(uint16_t pixy = 0; pixy < assembledimg.height(); ++pixy)
    //    {
    //        for(uint16_t pixx = 0; pixx < assembledimg.width(); ++pixx)
    //        {
    //            //Absolute tile coordinate
    //            size_t abstlx = (pixx / tilesqrt);
    //            size_t abstly = (pixy / tilesqrt);
    //            size_t abstileindex = (abstly * imgtileswidth) + abstlx;                    //Index of the current pixel tile containing the specified pixel coordinates, in absolute nb of tiles
    //            
    //            //Absolute tile group coordinate
    //            size_t abstgrpindex = abstileindex / tilegrpnbtiles;                        //Index of the current tile group containing the specified pixel coordinates, in absolute nb of tile groups

    //            //Relative coord
    //            size_t reltgrptileidx = abstileindex - (abstgrpindex * tilegrpnbtiles);                      //index of a PixelTile within a tilemap group (0-8)
    //            size_t reltlpixidx    = ((pixy % tilesqrt) * tilesqrt) + (pixx % tilesqrt); //Index of a pixel within the current 8x8 pixel tile (0-63)

    //            if( abstgrpindex < tiledtmap.size() )
    //            {
    //                const auto & curtmap = tiledtmap[abstgrpindex][reltgrptileidx];
    //                if(curtmap.tileindex < tileset.Tiles().size())
    //                {
    //                    gimg::tiled_image_i8bpp::tile_t tile;
    //                    for(size_t cntpix = 0; cntpix < tileset.Tiles()[curtmap.tileindex].size(); ++cntpix )
    //                        tile[cntpix] = tileset.Tiles()[curtmap.tileindex][cntpix] + (curtmap.palindex * NbColors4bpp);
    //                    if(curtmap.hflip)
    //                        tile.flipH();
    //                    if(curtmap.vflip)
    //                        tile.flipV();
    //                    assembledimg.getPixel(pixx, pixy) = tile[reltlpixidx];
    //                    //cout <<"Img(" <<pixx <<", " <<pixy <<") -> Tilegrp: " <<abstgrpindex <<", tile#: " <<reltgrptileidx <<", pixel index: " <<reltlpixidx <<"\n";
    //                }
    //            }
    //            else
    //            {
    //                //cerr<<"Bad tgrp index " <<abstgrpindex <<" for pixel coord ( " <<pixx <<", " <<pixy <<" )\n";
    //            }
    //        }
    //    }

    //    return std::move(assembledimg);
    //}

    /*
            -img         : Destination image
            -tileoffsetx : Offset in pixel of the tile's first upper left pixel
            -tileoffsety : Offset in pixel of the tile's first upper left pixel
            -tinfo       : Tile mapping data
            -ittilebeg   : Iterator on the first pixel of the tile. (The pixel data must be stored linearly)
            -ittileend   : Iterator past the last pixel of the tile. (The pixel data must be stored linearly)
    */
    template<class _init>
        void WriteTileAtCoordinates( gimg::tiled_image_i8bpp & img, size_t tileoffsetx, size_t tileoffsety, const tileproperties & tinfo, _init ittilebeg, _init ittileend )
    {
        static const size_t TileWidth   = 8;
        static const size_t TileWHeight = 8;
        static const size_t SizePalette = 16;
        const size_t oldsize = tileoffsetx * tileoffsety;
        const size_t newsize = (( tileoffsetx + TileWidth ) * ( tileoffsety + TileWHeight )) + oldsize;

        if( (tileoffsetx + TileWidth)   > img.width() ||
            (tileoffsety + TileWHeight) > img.height() ||
            newsize > img.getTotalNbPixels()  )
        {
            assert(false);
            throw std::runtime_error( "WriteTileAtCoordinates(): Not enough pixels left to write current tile!" );
        }

        const int XIncr = (tinfo.hflip)? -1        : 1;
        const int XTarg = (tinfo.hflip)? -1        : TileWidth;
        const int XInit = (tinfo.hflip)? (TileWidth - 1) : 0;

        const int YIncr = (tinfo.vflip)? -1          : 1;
        const int YTarg = (tinfo.vflip)? -1          : TileWHeight;
        const int YInit = (tinfo.vflip)? (TileWHeight - 1) : 0;

        for( int y = YInit; (y != YTarg) && (ittilebeg != ittileend); y += YIncr )
        {
            for( int x = XInit; (x != XTarg) && (ittilebeg != ittileend); x += XIncr, ++ittilebeg )
                img.getPixel(tileoffsetx + x, tileoffsety + y) = (*ittilebeg) + (tinfo.palindex * SizePalette);
        }
    }


    /*
            -tgrpoffsetx : Offset of the first upper right pixel of the first tile in the tile group on the target image.
            -tgrpoffsety : Offset of the first upper right pixel of the first tile in the tile group on the target image.
            -itcurtmap   : Current tilemap data entry.
            -itendtmap   : Past the last tilemap entry for this tile group.
            -tiles       : all the tiles for this tileset
    */
    template<class _inittmap>
        void WriteTileGroupAtCoord( gimg::tiled_image_i8bpp      & img, 
                                    size_t                         tgrpoffsetx, 
                                    size_t                         tgrpoffsety, 
                                    _inittmap                    & itcurtmap, 
                                    _inittmap                      itendtmap, 
                                    const TilesetLayer::imgdat_t & tiles,
                                    size_t                       & mapcnt)
    {
        static const size_t TileGroupWidth  = 3;
        static const size_t TileGroupHeight = 3;
        static const size_t TileWidth       = 8;
        static const size_t TileWHeight     = 8;

        size_t currxoff = tgrpoffsetx;
        size_t curryoff = tgrpoffsety;
        const size_t oldsize  = (tgrpoffsetx * tgrpoffsety);
        const size_t newsize  = ((TileGroupHeight * TileWHeight) * (TileGroupWidth  * TileWidth)) + oldsize;

        if( (tgrpoffsetx + (TileGroupWidth  * TileWidth))   > img.width()  || 
            (tgrpoffsety + (TileGroupHeight * TileWHeight)) > img.height() ||
            newsize > img.size() )
        {
            assert(false);
            throw std::runtime_error( "WriteTileGroupAtCoord(): Not enough pixels left to write current tile!" );
        }

        for( size_t tmy = 0; (tmy < TileGroupHeight) && (itcurtmap != itendtmap); ++tmy )
        {
            for( size_t tmx = 0; (tmx < TileGroupWidth) && (itcurtmap != itendtmap); ++tmx, ++itcurtmap )
            {
                size_t tindex = itcurtmap->tileindex;
                if( itcurtmap->tileindex >= tiles.size() )
                {
                    clog << "    *Invalid tile index " <<itcurtmap->tileindex <<"\n";
                    tindex = 0;
                }
                clog <<"TileMap #" <<mapcnt <<" - Target Tile #" <<tindex <<" at (" <<(tgrpoffsetx + (tmx * TileWidth)) <<", " <<curryoff <<")\n";
                WriteTileAtCoordinates( img, (tgrpoffsetx + (tmx * TileWidth)), curryoff, *itcurtmap, std::begin(tiles[tindex]), std::end(tiles[tindex]) );
                //currxoff += TileWidth;
                ++mapcnt;
            }
            curryoff += TileWHeight;
            //currxoff = tgrpoffsetx;
        }
    }


    gimg::tiled_image_i8bpp PreparePixels_LinearTiling(const TilesetLayer & tileset, const TilesetBMAData & bmadat)
    { 
        static const size_t tilegrpnbtiles = 9;
        static const size_t tilegrpwidth   = 3;
        static const size_t tilegrpheight  = 3;
        static const size_t tilesqrt       = 8;
        size_t       currentoffsetx = 0;
        size_t       currentoffsety = 0;
        auto         ittmap         = tileset.TileMap().begin();
        auto         ittmapend      = tileset.TileMap().end();
        size_t       mapcnt         = 0;
        gimg::tiled_image_i8bpp assembledimg;

        //!FIXME: This calculation for the image size is wrong!
        const size_t imgtilewidth   = bmadat.width; 
        const size_t imgtileheight  = tileset.TileMap().size() /imgtilewidth;
        const size_t imgwidthtgrp   = (imgtilewidth  / tilegrpwidth ) + ( (imgtilewidth  % tilegrpwidth  != 0)? 1 : 0);  
        const size_t imgheighttgrp  = (imgtileheight / tilegrpheight) + ( (imgtileheight % tilegrpheight != 0)? 1 : 0);  

        assembledimg.setNbTilesRowsAndColumns( (imgwidthtgrp * tilegrpwidth), (imgtileheight * tilegrpheight) );
        clog <<"Has " <<tileset.TileMap().size() <<" Tilemap entries\nAllocated a " <<((imgwidthtgrp * tilegrpwidth) * tilesqrt) <<"x" <<((imgheighttgrp * tilegrpheight) * tilesqrt) <<" pixels image, or " <<(imgwidthtgrp * tilegrpwidth) <<"x" <<(imgheighttgrp * tilegrpheight) <<" tiles \n";
        
        
        //std::advance( ittmap, 9 );
        
        for( size_t tmgrpy = 0; tmgrpy < imgheighttgrp; ++tmgrpy ) 
        {
            for( size_t tmgrpx = 0; tmgrpx < imgwidthtgrp; ++tmgrpx )
            {
                currentoffsetx = tmgrpx * (tilegrpwidth * tilesqrt);
                WriteTileGroupAtCoord( assembledimg, currentoffsetx, currentoffsety, ittmap, ittmapend, tileset.Tiles(), mapcnt );
            }
            currentoffsety += (tilegrpheight * tilesqrt);
            currentoffsetx = 0;
        }

        return std::move(assembledimg);
    }


    void PrintAssembledTilesetPreviewToPNG(const std::string & fpath, const Tileset & tileset)
    {
        size_t cntlayer = 0;
        for( const auto & layer : tileset.Layers() )
        {
            stringstream name;
            name <<fpath <<"_l" <<cntlayer <<".png";
            clog <<"Writing layer #" <<cntlayer <<" with" <<layer.TileMap().size() <<" tiles to \"" <<name.str() <<"\"" <<"\n";
            gimg::tiled_image_i8bpp assembledimg(PreparePixels_LinearTiling(layer,tileset.BMAData()));
            //gimg::tiled_image_i8bpp assembledimg(PreparePixels_TiledMapPixelByPixel(tileset));
            //gimg::tiled_image_i8bpp assembledimg(PreparePixels_TiledTiles(tileset));
            assembledimg.setNbColors(256);

            //Fill the color palette
            for( size_t cntpal = 0; cntpal < tileset.Palettes().mainpals.size(); ++cntpal )
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

            utils::io::ExportToPNG(assembledimg, name.str());
            clog <<"\n";
            ++cntlayer;
        }
    }


    void DumpCellsToPNG(const std::string & outdir, const Tileset & tileset)
    {
        //static const uint16_t NbColorsPalette    = 16;
        static const uint16_t NbColorsPerPalette = 16;
        static const size_t   ImgRowLenTiles     = 36;

        typedef uint16_t tileindex_t;
        typedef uint8_t  palindex_t;
        size_t cntlayers = 0;

        for( const auto & layer : tileset.Layers() )
        {
            //First build a palette index lookup table
            std::unordered_map<tileindex_t, palindex_t> pallut;
            for( const auto & tmap : layer.TileMap() )
                pallut[tmap.tileindex] = tmap.palindex;

            gimg::tiled_image_i8bpp assembledimg;
            size_t                  nbrows = (layer.Tiles().size() / ImgRowLenTiles) + ( (layer.Tiles().size() % ImgRowLenTiles != 0)? 1 : 0);
            assembledimg.setNbTilesRowsAndColumns( ImgRowLenTiles, nbrows );

            //Write all tiles
            for( size_t tidx = 0; tidx < layer.Tiles().size(); ++tidx )
            {
                for( size_t pidx = 0; pidx < layer.Tiles()[tidx].size(); ++pidx )
                {
                    assembledimg.getTile(tidx)[pidx] = layer.Tiles()[tidx][pidx] + (pallut[tidx] * NbColorsPerPalette);
                }
            }

            //Fill the color palette
            assembledimg.setNbColors(256);
            for( size_t cntpal = 0; cntpal < tileset.Palettes().mainpals.size(); ++cntpal )
            {
                for( size_t cntc = 0; cntc < NbColorsPerPalette; ++cntc ) 
                {
                    auto & curcol = assembledimg.getPalette()[(cntpal * NbColorsPerPalette) + cntc];
                    auto & cursrc = tileset.Palettes().mainpals[cntpal][cntc];
                    curcol.red   = cursrc._red;
                    curcol.green = cursrc._green;
                    curcol.blue  = cursrc._blue;
                }
            }
            
            //Export the image
            stringstream pngname;
            pngname << utils::TryAppendSlash(outdir) << "cells_layer" <<cntlayers << ".png";

            if( assembledimg.getTotalNbPixels() != 0 )
                utils::io::ExportToPNG(assembledimg, pngname.str());
            else
            {
                cerr << "\n<!>- WARNING: Got empty image as result from dumping the tiles..\n";
                clog<<"\n<!>- WARNING: Got empty image as result from dumping the tiles..\n";
            }
            ++cntlayers;
        }
    }

};
