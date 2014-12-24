#include "pmd2_image_formats.hpp"
#include <ppmdu/utils/gbyteutils.hpp>
#include <ppmdu/utils/handymath.hpp>
#include <ppmdu/utils/utility.hpp>
#include <algorithm>
#include <vector>
#include <array>
#include <cassert>
#include <iostream>
#include <iomanip>
#include "png++/png.hpp"
#include <sstream>
#include <ppmdu/containers/tiled_image.hpp>
using namespace std;
using ::utils::Resolution;

namespace pmd2 { namespace graphics
{

//==================================================================
// Typedefs 
//==================================================================
    //typedef array< array<uint8_t,8>, 8>                                   imgmatrix_t;
    typedef std::pair<types::constitbyte_t, types::constitbyte_t>         similarTableEntry_t;  //Dat long-ass type -_-
    typedef std::pair<std::vector<uint8_t>, std::vector<RLE_TableEntry> > rleCompressedFrame_t;

//==================================================================
// Constants
//==================================================================
    static const unsigned int _MIN_AMNT_BYTE_FOR_RLE_COMP = 25u; //There must be at least 25 similar bytes in a row to 
                                                                 // save any space by using RLE compression given
                                                                 // each RLE table entries are 12 bytes long, and 
                                                                 // the obligatory 12 bytes empty entry..

    static const unsigned int PIXELS_PER_TILE             = 64u; //Ammount of pixels per tiles
    static const unsigned int WIDTH_TILE                  = 8u;  //Width in pixels of a tile
    static const unsigned int HEIGHT_TILE                 = 8u;  //Height in pixels of a tile


    //#TODO: remove this eventually. I just needed something to visualize the issue!
    static const array< array<unsigned int,8>, 8> A_TILE = 
    {{
        { { 0,  1,  2,  3,  4,  5,  6,  7 } },
        { { 8,  9, 10, 11, 12, 13, 14, 15 } },
        { {16, 17, 18, 19, 20, 21, 22, 23 } },
        { {24, 25, 26, 27, 28, 29, 30, 31 } },
        { {32, 33, 34, 35, 36, 37, 38, 39 } },
        { {40, 41, 42, 43, 44, 45, 46, 47 } },
        { {48, 49, 50, 51, 52, 53, 54, 55 } },
        { {56, 57, 58, 59, 60, 61, 62, 63 } },
    }};

    //#TODO: remove this eventually. I just needed something to visualize the issue!
    //template< int TILES_X, int TILES_Y >
    //    struct MrTileHelper
    //{
        //typedef array<array<unsigned int,8>, 8> tile_t;

        unsigned int GetTiledIndexFromUntiledImageCoord( unsigned int x, unsigned int y, unsigned int height )
        {
            const unsigned int TILES_Y = height / HEIGHT_TILE;

            unsigned int tilex = x / WIDTH_TILE,
                         tiley = y / HEIGHT_TILE,
                         xWithinTile = x - (tilex * WIDTH_TILE),
                         yWithinTile = y - (tiley * HEIGHT_TILE);

            unsigned int tilepixindex = A_TILE[yWithinTile][xWithinTile];

            return tilepixindex + (tilex * PIXELS_PER_TILE + (tiley * (TILES_Y * PIXELS_PER_TILE)) );
        }

        //static const array<array<tile_t, TILES_X>, TILES_Y> _TILEDINDEXES = {A_TILE};
    //};

//=== Had to do this crap, because VS2012's compiler is really messed up.. ==//
const Resolution & GetImgResolutionForPixelAmt( uint32_t pixelamount )
{
    //#TODO: Do something more flexible/elegant once we actually know how to guess 
    //       resolution from the actual sprite files !

    if( pixelamount == 64u )
        return RES_8x8_SPRITE;
    else if( pixelamount == 128u )
    {
        cerr << "!- Warning: Guessed 16x8 resolution for unusal image pixel count!\n";
        return RES_16x8_SPRITE;
    }
    else if( pixelamount == 256u )
    {
        cerr << "!- Warning: Guessed 16x16 resolution for unusal image pixel count!\n";
        return RES_16x16_SPRITE;
    }
    else if( pixelamount == 512u )
    {
        cerr << "!- Warning: Guessed 16x32 resolution for unusal image pixel count!\n";
        return RES_16x32_SPRITE;
    }
    else if( pixelamount == 1024u )
        return RES_32x32_SPRITE;
    else if( pixelamount == 1600u )
        return RES_PORTRAIT;
    else
        return RES_INVALID;
}

//=====================================================================================
//                                RLE_TableEntry
//=====================================================================================

    RLE_TableEntry::RLE_TableEntry( uint32_t pixelsrc, uint32_t pixelamt, uint32_t unknval )
        :pixelsource_(pixelsrc), pixelamount_(pixelamt), unknown_val_(unknval)
    {
    }

    bool RLE_TableEntry::isNullEntry()const
    {
        return ( pixelsource_ == 0 && pixelamount_ == 0 && unknown_val_ == 0 );
    }

    bool RLE_TableEntry::isPixelSourceOffset()const
    {
        return bPSourceIsOffset;
    }

    void RLE_TableEntry::setPixelSourceIsOffset( bool ispsourceoffset )
    {
        bPSourceIsOffset = ispsourceoffset;
    }

    uint32_t & RLE_TableEntry::operator[]( unsigned int index )
    {
        switch(index)
        {
        case 0:
            return pixelsource_;
            break;
        case 1:
            return pixelamount_;
            break;
        case 2:
            return unknown_val_;
            break;
        default:
            return *( reinterpret_cast<uint32_t* >(0) ); //Trigger a crash
            break;
        };
    }

//=====================================================================================
// Image Format Converter
//=====================================================================================
    void Export_8bppIndexedBitmapToPNG( const bitmap8bpp_t             & indexedimg, 
                                        const graphics::rgb24palette_t & palette,
                                        const std::string              & out_filepath )
    {
        png::image< png::index_pixel > output;
        png::palette                 outpalette(palette.size());

        //Build palette
        for( unsigned int i = 0; i < palette.size(); ++i )
            outpalette[i] = png::color( palette[i].red, palette[i].green, palette[i].blue );

        output.set_palette( outpalette );

        //Fill the pixels
        output.resize( indexedimg.front().size(), indexedimg.size() );
        for( unsigned int i = 0; i < indexedimg.size(); ++i )
        {
            for( unsigned int j = 0; j < indexedimg[i].size(); ++j )
                output[i][j] = indexedimg[i][j];
        }

        output.write(out_filepath);
    }

    /*
        Expand4bppTo8bpp
    */
    indexed8bppimg_t Expand4bppTo8bpp( indexed4bppimg_t::const_iterator itbeg, indexed4bppimg_t::const_iterator itend )
    {
        indexed8bppimg_t larger( distance(itbeg, itend) * 2 );
        auto itput = larger.begin();

        for(; itbeg != itend; ++itbeg )
        {
            (*itput) = (*itbeg) & 0x0F;
            ++itput;
            (*itput) = ((*itbeg) & 0xF0) >> 4;
            ++itput;
        }

        return std::move(larger);
    }

    indexed4bppimg_t Shrink8bppTo4bpp( indexed8bppimg_t::const_iterator itbeg, indexed8bppimg_t::const_iterator itend )
    {
        unsigned int   inputsize = distance(itbeg, itend);
        assert(inputsize % 8 == 0);

        indexed4bppimg_t smaller( inputsize / 2 );
        auto itput = smaller.begin();

        for(; itbeg != itend; itbeg+=2 )
        {
            (*itput) = (*itbeg) & 0x0F;
            (*itput) |= ((*(itbeg+1)) << 4) & 0xF0;
            ++itput;
        }

        return std::move(smaller);
    }


    void Shrink8bppTo4bpp( indexed8bppimg_t::const_iterator itbeg, indexed8bppimg_t::const_iterator itend, indexed4bppimg_t & output )
    {
        unsigned int   inputsize = distance(itbeg, itend);
        assert(inputsize % 8 == 0);
        output.resize( inputsize / 2 );
        auto itput = output.begin();

        for(; itbeg != itend; itbeg+=2 )
        {
            (*itput) = (*itbeg) & 0x0F;
            (*itput) |= ((*(itbeg+1)) << 4) & 0xF0;
            ++itput;
        }
    }

//=====================================================================================
// export_8bppTiled_to_png
//=====================================================================================
    //export_8bppTiled_to_png::export_8bppTiled_to_png( const std::string & outputfolderpath, const std::string & filenamesuffix )
    //    :m_suffix(filenamesuffix), m_cptimgname(0), m_folder(outputfolderpath)
    //{}

    //void export_8bppTiled_to_png::operator()( const bitmap8bpp_t & bitmap8bpp, const rgb24palette_t & palette )
    //{
    //    stringstream ss;
    //    ss << utils::AppendTraillingSlashIfNotThere(m_folder) << setfill('0') << setw(4)  << m_cptimgname; 

    //    if( !m_suffix.empty() )
    //        ss <<"_" <<m_suffix;

    //    ss << ".png";

    //    Export_8bppIndexedBitmapToPNG( bitmap8bpp, palette, ss.str() );

    //    ++m_cptimgname; //increment the image name counter
    //}

    //void export_8bppTiled_to_png::operator()( const indexed8bppimg_t & indexed8bpptiledimg, const rgb24palette_t & palette )
    //{
    //    //Untile the image first
    //    bitmap8bpp_t untiledimg;
    //    Resolution res = GetImgResolutionForPixelAmt( indexed8bpptiledimg.size() );

    //    if( res == RES_INVALID )
    //    {
    //        stringstream sstr;
    //        sstr << "Unexpected image resolution! " << indexed8bpptiledimg.size() << " pixels total !\n";
    //        assert(false);
    //        throw length_error( sstr.str().c_str() );
    //    }

    //    Untile8bpp( res.width, res.height, indexed8bpptiledimg, untiledimg );

    //    //Then just call the code for the untiled 8bpp bitmap!
    //    (*this)( untiledimg, palette );
    //}

    //Handles a struct containing both palette and 8bpp tiled image
    //void export_8bppTiled_to_png::operator()( const rgb24pal_and_8bpp_tiled & combopalimg, 
    //                                          const string                  * psuffixoverride, 
    //                                          const uint32_t                * pnumberoverride)
    //{
    //    //If we override the suffix or filenumber do it
    //    if( psuffixoverride != nullptr )
    //        m_suffix = *psuffixoverride;
    //    else
    //        m_suffix = string();

    //    if( pnumberoverride != nullptr )
    //        m_cptimgname = *pnumberoverride;

    //    //Then just call the code for the tiled 8bpp!
    //    (*this)( combopalimg._8bpp_timg, combopalimg._palette );
    //}

//=====================================================================================
// ImportPNG_To_4bppImgAndPal
//=====================================================================================
    //void importPNG_To_4bppImgAndPal( indexed4bppimg_t  & out_indexedtiled4bppimg,
    //                                 rgb24palette_t    & out_palette,
    //                                 const std::string & filepath )
    //{
    //    png::image<png::index_pixel_4> input;
    //    input.read(filepath);

    //    const auto & mypalette = input.get_palette();

    //    //Build palette
    //    for( unsigned int i = 0; i < mypalette.size(); ++i )
    //    {
    //        out_palette[i].red   = mypalette[i].red;
    //        out_palette[i].green = mypalette[i].green;
    //        out_palette[i].blue  = mypalette[i].blue;
    //    }


    //    //Fill the pixels
    //    out_indexedtiled4bppimg.resize( input.get_height() * input.get_width(), 0 );

    //    for( unsigned int i = 0; i < input.get_width(); ++i )
    //    {
    //        for( unsigned int j = 0; j < input.get_height(); ++j )
    //        {
    //            png::byte    apixel          = static_cast<png::byte>( input.get_pixel(i,j) );
    //            unsigned int indextoinsertat = GetTiledIndexFromUntiledImageCoord( i, j, input.get_height() );
    //            out_indexedtiled4bppimg[indextoinsertat] = apixel;
    //        }
    //    }
    //}

    void importPNG_To_8bppImgAndPal( indexed8bppimg_t  & out_indexedtiled8bppimg,
                                     rgb24palette_t    & out_palette,
                                     const std::string & filepath )
    {
        png::image<png::index_pixel> input;
        input.read( filepath, png::require_color_space<png::index_pixel>() ); 

        const auto & mypalette = input.get_palette();
        out_palette.resize(mypalette.size());

        //Build palette
        for( unsigned int i = 0; i < mypalette.size(); ++i )
        {
            out_palette[i].red   = mypalette[i].red;
            out_palette[i].green = mypalette[i].green;
            out_palette[i].blue  = mypalette[i].blue;
        }


        //Fill the pixels
        out_indexedtiled8bppimg.resize( input.get_height() * input.get_width(), 0 );

        for( unsigned int i = 0; i < input.get_width(); ++i )
        {
            for( unsigned int j = 0; j < input.get_height(); ++j )
            {
                unsigned int indextoinsertat = GetTiledIndexFromUntiledImageCoord( i, j, input.get_height() );
                out_indexedtiled8bppimg[indextoinsertat] = input[i][j];
            }
        }
    }



//=====================================================================================
//                    Function for Handling Conversion to Bitmap
//=====================================================================================


    // y = cptpixel / 4 
    // x = cptpixel - (y * 4)



    //void Untile4bpp( unsigned int                                width, 
    //                 unsigned int                                height,
    //                 const std::vector<uint8_t>                & in_framedata, 
    //                 std::vector< graphics::indexed8bppimg_t > & out_indexed8bpp_bitmap )
    //{
    //    assert( (in_framedata.size() % 8) == 0 ); //Make sure the size is divisible by 8 !
    //    
    //    const auto                nbtiles  = (in_framedata.size() * 2) / PIXELS_PER_TILE, // multiplied by 2 to get the actual amount of pixels
    //                              nbtilesX = width  / WIDTH_TILE,
    //                              nbtilesY = height / HEIGHT_TILE;
    //    
    //    //const unsigned int        sqrtnbtiles = sqrt( nbtiles );
    //    types::constitbyte_t      itpixels    = in_framedata.begin();

    //    //#1 - Resize output bitmap to specified size
    //    out_indexed8bpp_bitmap.resize( height );
    //    for( vector<uint8_t> &row : out_indexed8bpp_bitmap )
    //        row.resize( width );

    //    //#2 - Untile
    //    for( unsigned int cpttile = 0; cpttile < nbtiles; ++cpttile )
    //    {
    //        unsigned int tileX = cpttile % nbtilesX,
    //                     tileY = cpttile / nbtilesY;

    //        for( unsigned int cptpixel = 0; cptpixel < PIXELS_PER_TILE; cptpixel+=2, ++itpixels ) //incr by two, since we do 2 pixels at a time
    //        {
    //            unsigned int x = ( tileX * WIDTH_TILE )  + ( cptpixel  % WIDTH_TILE ),
    //                         y = ( tileY * HEIGHT_TILE ) + ( cptpixel / HEIGHT_TILE );

    //            out_indexed8bpp_bitmap[y][x+1] = ((*itpixels) >> 4) & 0xF;
    //            out_indexed8bpp_bitmap[y][x]   = (*itpixels)        & 0xF;
    //        }
    //    }
    //}

    //std::vector<std::vector<uint8_t> >  BuildATile( types::constitbyte_t itin, unsigned int nbtilesH, unsigned int nbtilesV )
    //{
    //    vector<vector<uint8_t> >   mytile(HEIGHT_TILE,  vector<uint8_t>(WIDTH_TILE) );
    //    auto                       itout = mytile.begin();

    //    for( unsigned int i = 0; i < nbbytes4bpppertile; ++i, ++itin )
    //    {
    //        (*itout) = twopixels & 0x0F;
    //        ++itout;
    //    }

    //    return std::move(mytile);
    //}

    //std::vector<uint8_t> StitchTiles( std::vector<std::vector<uint8_t> > & intiles )
    //{

    //}


    void Untile8bpp(    unsigned int                                width, 
                        unsigned int                                height,
                        const std::vector<uint8_t>                & in_framedata8bpp, 
                        std::vector< graphics::indexed8bppimg_t > & out_indexed8bpp_bitmap)
    {
        assert( (in_framedata8bpp.size() % 8) == 0 ); //Make sure the size is divisible by 8 !

        //#1 - Resize output bitmap to specified size
        out_indexed8bpp_bitmap.resize( height );
        for( vector<uint8_t> &row : out_indexed8bpp_bitmap )
            row.resize( width );

        const unsigned int    nbtiles       = in_framedata8bpp.size() / PIXELS_PER_TILE,
                              nbtilesperrow = width / WIDTH_TILE;
        types::constitbyte_t  itpixels      = in_framedata8bpp.begin();

        //#2 - Untile
        for( unsigned int cpttile = 0; cpttile < nbtiles; ++cpttile )
        {
            unsigned int tileX = cpttile % nbtilesperrow,
                         tileY = cpttile / nbtilesperrow;

            for( unsigned int cptpixel = 0; cptpixel < PIXELS_PER_TILE; ++cptpixel, ++itpixels )
            {
                unsigned int x = ( tileX * WIDTH_TILE )  + ( cptpixel % WIDTH_TILE ),
                             y = ( tileY * HEIGHT_TILE ) + ( cptpixel / WIDTH_TILE );

                out_indexed8bpp_bitmap[y][x] = (*itpixels);
            }
        }
    }


//=====================================================================================
//            Function for Handling Conversion from Bitmap to Tiled
//=====================================================================================
    void Tile8bppBitmapTo4bppRaw( const bitmap8bpp_t & bitmap, indexed4bppimg_t & out_timg )
    {
        assert(false); //UNTESTED !!!

        assert(!bitmap.empty());
        //#1 - Resize output bitmap to specified size
        unsigned int nbpixels = 0;
        unsigned int width    = bitmap.front().size();

        //Get nb pixels!
        for( const vector<uint8_t> &row : bitmap )
            nbpixels += row.size();

        assert( (nbpixels % 8) == 0 ); //Make sure the size is divisible by 8 !

        //Resize output
        out_timg.resize(0);
        out_timg.reserve(nbpixels);

        const unsigned int    nbtiles       = nbpixels / PIXELS_PER_TILE,
                              nbtilesperrow = width / WIDTH_TILE;
        //types::constitbyte_t  itpixels      = in_framedata8bpp.begin();

        //#2 - Untile
        for( unsigned int cpttile = 0; cpttile < nbtiles; ++cpttile )
        {
            unsigned int tileX = cpttile % nbtilesperrow,
                         tileY = cpttile / nbtilesperrow;

            for( unsigned int cptpixel = 0; cptpixel < PIXELS_PER_TILE; ++cptpixel )
            {
                unsigned int x = ( tileX * WIDTH_TILE )  + ( cptpixel % WIDTH_TILE ),
                             y = ( tileY * HEIGHT_TILE ) + ( cptpixel / WIDTH_TILE );

                out_timg.push_back( bitmap[y][x] );
            }
        }
    }


//=====================================================================================
//                    Function for Compressing a Frame Using PMD2's RLE
//=====================================================================================

    //---------------------------------------------------------------------------------
    //Insert a range of differing elements, during RLE compression, into the Output data table and into the RLE table
    //---------------------------------------------------------------------------------
    void InsertRangeIntoRLETable( vector<uint8_t>&                out_resultdata, 
                                  vector<RLE_TableEntry> &     out_rletable, 
                                  vector<uint8_t>::const_iterator itInputBytes, 
                                  vector<uint8_t>::const_iterator itInputBytesEnd )
    {
        vector<uint8_t>::const_iterator itinsertpos      = out_resultdata.insert( out_resultdata.end(), itInputBytes, itInputBytesEnd );
        vector<uint8_t>::const_iterator itOutputBegining = out_resultdata.begin();

        out_rletable.push_back( 
                                RLE_TableEntry( std::distance( itOutputBegining, itinsertpos), std::distance(itInputBytes, itInputBytesEnd) )
                              );
        out_rletable.back().setPixelSourceIsOffset(true); //The entry is an offset
    }


    //---------------------------------------------------------------------------------
    //Insert a compressed repeated value, during RLE compression, into the Output data table and into the RLE table
    //---------------------------------------------------------------------------------
    void InsertCompressedIntoRLETable( vector<RLE_TableEntry>& out_rletable, vector<similarTableEntry_t>::const_iterator similarSerie ) 
    {
        out_rletable.push_back( RLE_TableEntry( *(similarSerie->first), std::distance( similarSerie->first, similarSerie->second ) ) );
        out_rletable.back().setPixelSourceIsOffset(false); //The entry is NOT an offset
    }


    //---------------------------------------------------------------------------------
    //Function that builds the list of similar series for the RLE compression function!
    //---------------------------------------------------------------------------------
    void BuildSimilarValueSeriesTable( vector<similarTableEntry_t>& out_similarSerie, const vector<uint8_t> & in_frame )
    {
        vector<uint8_t>::const_iterator itframecur  = in_frame.begin(),
                                     itframeend  = in_frame.end(),
                                     itfound;
        auto lambda_valcmpr = [&](const uint8_t& by)
                              { 
                                  return (*itframecur) == by; 
                              };

       for( vector<uint8_t>::size_type i = 0; i < in_frame.size(); )
        {
            //vector<uint8_t>::const_iterator itCur = ( in_frame.begin() + i );
            itframecur = ( in_frame.begin() + i );

            itfound = std::find_if_not( itframecur, itframeend, lambda_valcmpr );

            vector<uint8_t>::size_type posFound = distance( in_frame.begin(), itfound );

            vector<uint8_t>::size_type iDiv16_        = (i > 0)? CalcClosestHighestDenominator( i, 16 ) : 0,
                                       posFoundDiv16_ = CalcClosestLowestDenominator( posFound, 16 );

            if( posFoundDiv16_ > iDiv16_ )
            {
                if( (posFoundDiv16_ - iDiv16_) >= _MIN_AMNT_BYTE_FOR_RLE_COMP )
                {
                    out_similarSerie.push_back( std::make_pair( in_frame.begin() + iDiv16_, in_frame.begin() + posFoundDiv16_ ) );

                    //unsigned short curvalue = in_frame[iDiv16_];
                    //std::cerr << "# PushingBack series of similar values " <<"[" <<std::dec <<iDiv16_ << ", " <<std::dec <<posFoundDiv16_  
                    //          <<"]\n   (len: 0x" <<std::hex << posFoundDiv16_ - iDiv16_ <<", val:0x" <<std::hex <<curvalue <<" )\n";
                    i = posFoundDiv16_+1;
                }
                else
                {
                    i = posFound+1;
                }
            }
            else
            {
                i = posFound+1;
            }
            
        }
    }


    //---------------------------------------------------------------------------------
    //Function that builds the RLE table and fill the output vector with the raw uncompressable data!
    //---------------------------------------------------------------------------------
    void Fill_RLE_TableAndRawDataOut(
                                        const vector<uint8_t>            & in_frame,
                                        const vector<similarTableEntry_t>& in_similarserieslist,
                                        vector<uint8_t>                  & out_resultdata,
                                        vector<RLE_TableEntry>           & out_rletable
                                    )
    {
        vector<uint8_t>::const_iterator             itInputBytes        = in_frame.begin(),
                                                    itInputBytesEnd     = in_frame.end();

        vector<similarTableEntry_t>::const_iterator itCurSimElmSerie    = in_similarserieslist.begin(),
                                                    itCurSimElmSerieEnd = in_similarserieslist.end();

        if( itCurSimElmSerie == itCurSimElmSerieEnd )
        {
            //We have no similar element series. Just make a single entry in the RLE table and slap everythin in the output.
            InsertRangeIntoRLETable( out_resultdata, out_rletable, itInputBytes, itInputBytesEnd );
        }
        else
        {
            //Iterate until we're at the end of the input byte vector
            while( itInputBytes != itInputBytesEnd )
            {
                if( itCurSimElmSerie == itCurSimElmSerieEnd ) //No more similar serie, pack everything that's left.
                {
                    InsertRangeIntoRLETable( out_resultdata, out_rletable, itInputBytes, itInputBytesEnd );
                    itInputBytes = itInputBytesEnd; //Move iterator
                }
                else
                {
                    if( itCurSimElmSerie->first == itInputBytes ) //We're on the first element of a serie of similar elements
                    {
                        InsertCompressedIntoRLETable( out_rletable, itCurSimElmSerie );
                        itInputBytes = itCurSimElmSerie->second; //Move iterator
                        ++itCurSimElmSerie; //Move to next serie of similar elements
                    }
                    else //Our iterator on the input vector is NOT at the start of the current serie of similar elements.
                    {
                        InsertRangeIntoRLETable( out_resultdata, out_rletable, itInputBytes, itCurSimElmSerie->first );
                        itInputBytes = itCurSimElmSerie->first; //Move iterator
                    }
                }
            }
        }

        //Add null entry at the end of the table
        out_rletable.push_back( RLE_TableEntry() );
        out_rletable.back().setPixelSourceIsOffset(false); //The entry is NOT an offset
    }


    //---------------------------------------------------------------------------------
    //Compress a frame to the PMD RLE format
    //---------------------------------------------------------------------------------
    void RLE_EncodeFrame( const vector<uint8_t>      & in_frame, 
                          vector<uint8_t>            & out_resultdata, 
                          vector<RLE_TableEntry>     & out_rletable, 
                          vector<uint8_t>::size_type   baseoffset  ) //Overwrites the output vector
    {
        //#0 - Validate 
        if( in_frame.size() % 16 != 0 )
        {
            assert(false); //Ideally a frame aligned on 16 bytes will compress much better. Not to mention, 
                           // frames actually have to be divisible by 16 to actually be legal frames..
        }

        //Store similar series in there, so we can deal more easily with extracting everything later on
        vector<similarTableEntry_t> similarserieslist;

        //#1 - List all similar values series first
        BuildSimilarValueSeriesTable( similarserieslist, in_frame );
        std::cerr << "\n\n";

        //#2 - Split the similar from different, fill the output data vector with the uncompressable data, fill up the RLE table
        std::cerr << "Building RLE table and output data..\n";
        Fill_RLE_TableAndRawDataOut( in_frame, similarserieslist, out_resultdata, out_rletable );

        std::cerr << "RLE table built, null entry appended! Output data ready!\n";
    }


    //---------------------------------------------------------------------------------
    // Push back the compressed frame into a vector
    //---------------------------------------------------------------------------------
    void RLE_EncodeFrame_PushBackToVector( const vector<uint8_t> & in_frame, vector<rleCompressedFrame_t>  & out_result )
    {
        vector<uint8_t>            uncompdata;
        vector<RLE_TableEntry>  rletabl;

        //#1 - Run compression
        RLE_EncodeFrame( in_frame, uncompdata, rletabl, 0 );

        std::cerr << "--<< Final RLE Table content >>--\n";
        for( vector<RLE_TableEntry>::size_type i = 0; i < rletabl.size(); ++i )
        {
            std::cerr << std::dec << i << " ";
            if(rletabl[i].isPixelSourceOffset())
                std::cerr <<"PixelSrc";
            else
                std::cerr <<"PixelVal";

            std::cerr <<": 0x" << std::hex << rletabl[i].pixelsource_ << ", PixelAmt: 0x" << rletabl[i].pixelamount_ <<", UnkVal : 0x" << rletabl[i].unknown_val_ << "\n";
        }
        std::cerr << "---------------------------------\n";

        //#2 - Append the result
        out_result.push_back( std::make_pair( uncompdata, rletabl ) );
    }

    //---------------------------------------------------------------------------------
    //Writes an RLE Compressed frame to the output SIR0 file buffer
    //Returns the offset of the beginning of the RLE table
    //---------------------------------------------------------------------------------
    vector<uint8_t>::size_type WriteOut_RLE_EncodedFrame( const rleCompressedFrame_t  & in_rleCompFrame, vector<uint8_t> & out_rawBytes )
    {
        vector<uint8_t>::size_type baseOffset       = out_rawBytes.size(),
                                totalCompFrameSz = in_rleCompFrame.first.size() + in_rleCompFrame.second.size();

        //Allocate some memory
        out_rawBytes.reserve( totalCompFrameSz );

        //#1 Output the raw data first
        out_rawBytes.insert( out_rawBytes.end(), in_rleCompFrame.first.begin(), in_rleCompFrame.first.end() );

        

        //#2 Apply the base offset of the output vector to the RLE table as we write them out
        auto                       itinsert = std::back_inserter( out_rawBytes );
        vector<uint8_t>::size_type rleTableOffset = out_rawBytes.size();

        for( auto & rletblentry : in_rleCompFrame.second )
        {
            //uint8_t intbuff[4];

            unsigned int pxSrc = ( rletblentry.isPixelSourceOffset() )? 
                                    rletblentry.pixelsource_ + baseOffset  //If we output an offset, add the base offset to it!
                                    : rletblentry.pixelsource_;            //If we output a value, leave it as-is !

            utils::WriteIntToByteVector( pxSrc, itinsert );
            //utils::UnsignedIntToByteBuff( pxSrc, intbuff );         //Convert the int to an array of bytes
            //out_rawBytes.insert( out_rawBytes.end(), intbuff, intbuff+4 ); //Insert the actual entry in the output

            utils::WriteIntToByteVector( rletblentry.pixelamount_, itinsert );
            //g::byteutils::UnsignedIntToByteBuff( rletblentry.pixelamount_, intbuff );
            //out_rawBytes.insert( out_rawBytes.end(), intbuff, intbuff+4 );

            utils::WriteIntToByteVector( rletblentry.unknown_val_, itinsert );
            //g::byteutils::UnsignedIntToByteBuff( rletblentry.unknown_val_, intbuff );
            //out_rawBytes.insert( out_rawBytes.end(), intbuff, intbuff+4 );
        }

        return rleTableOffset;
    }


//=====================================================================================
//                  Function for Decompressing PMD2's RLE Compressed Frames
//=====================================================================================

    //---------------------------------------------------------------------------------
    // Populate the RLE table starting at ittptrs, returns the total number of bytes of the uncompresse image
    //---------------------------------------------------------------------------------
    uint32_t PopulateRLETable( vector<RLE_TableEntry>& out_rletable, std::vector<uint8_t>::iterator ittptrs )
    {
        uint32_t frmlengthtotal=0; //Length in bytes/pixels that the uncompressed frame has in total

        //cptabort is to avoid a very long loop in case of weirdness
        // And we stop when we stumble on a null entry, as it signify the end of the table
        {
            int cptabort=0;
            vector<uint8_t>::iterator itt = ittptrs,
                                   ittend;
            while( !out_rletable.back().isNullEntry() && cptabort < 1024 && itt != ittend )
            {
                RLE_TableEntry curentry(0,0,0);

                //Read 3 entries, of 4 bytes 
                for( int i = 0; (i < 3) && (itt != ittend); ++i, itt+=4, cptabort+=4 )
                {
                    //Read 4 bytes
                    curentry[i] = utils::ReadIntFromByteVector<uint32_t>(itt);

                    //If we just read the length/pixelamount entry, add it to the total
                    if( i == 1 ) 
                        frmlengthtotal += curentry.pixelamount_;
                }

                out_rletable.push_back(curentry);
            }
        }

        return frmlengthtotal;
    }

    //---------------------------------------------------------------------------------
    // Handles the actual work of rebuilding the image from the RLE table
    //---------------------------------------------------------------------------------
    void RebuildFrameFrom_RLE_Table( const std::vector<uint8_t> & in_rawfile, 
                                     vector<RLE_TableEntry> & in_rletable, 
                                     vector<uint8_t> & out_decompframe )
    {
        vector<uint8_t>::size_type cptinsert = 0;

        //Go through all the RLE table entries
        for(RLE_TableEntry &entry: in_rletable)
        {
            if( entry.pixelsource_ <= 0xFF ) //We want to repeat this value
            {
                //Pixel source is a value
                auto i        = cptinsert;
                auto pixvalue = static_cast<uint8_t>( entry.pixelsource_ );

                for(; i < entry.pixelamount_; ++i )
                {
                    out_decompframe[i] = pixvalue;
                }

                cptinsert = i; //advance the insert counter
            }
            else //We want to insert the pixels starting from this position
            {
                //Pixel source is an offset
                auto i   = cptinsert;

                for(; i < entry.pixelamount_; ++i )
                {
                    out_decompframe[i] = in_rawfile[(entry.pixelsource_ + i)];
                }

                cptinsert = i; //advance the insert counter
            }
        }
    }

    //---------------------------------------------------------------------------------
    // Decompresses PMD2 RLE compressed frames
    //---------------------------------------------------------------------------------
    void RLEDecodeFrame( const std::vector<uint8_t>     & in_rawfile, 
                         std::vector<uint8_t>::iterator   ittptrs, 
                         vector<uint8_t>                & out_decompframe )
    {
        vector<RLE_TableEntry> rletable;
        uint32_t               frmlengthtotal=0; //Length in bytes/pixels that the uncompressed frame has in total

        //#1 - Fill our RLE table
        frmlengthtotal = PopulateRLETable( rletable, ittptrs );

        //Resize the output vector to the correct size
        out_decompframe.resize( frmlengthtotal );

        //#2 - Decompress the image
        RebuildFrameFrom_RLE_Table( in_rawfile, rletable, out_decompframe );

        //Done
    }


};};