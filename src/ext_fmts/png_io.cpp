#include "png_io.hpp"
#include <ppmdu/containers/tiled_image.hpp>
#include <ppmdu/pmd2/pmd2_palettes.hpp>
#include <utils/library_wide.hpp>
#include <utils/handymath.hpp>
#include <png++/png.hpp>
#include <iostream>
using namespace std;

namespace utils{ namespace io
{
    //A little helper to know the correct palette length to expect depending the 
    // png++ pixel type!
    template<class _pngimagepixel>
        struct pngpix_pal_len;

    template<>
        struct pngpix_pal_len<png::index_pixel_4>
        {
            static const uint32_t nbcolors = 16u; //16 colors
        };

    template<>
        struct pngpix_pal_len<png::index_pixel>
        {
            static const uint32_t nbcolors = 256u; //256 colors
        };


//
// Copy from indexed to PNG palette
//
    png::palette PalToPngPal( const std::vector<gimg::colorRGB24> & srcpal )
    {
        png::palette palette(srcpal.size());

        //copy palette
        for( unsigned int i = 0; i < palette.size(); ++i )
        {
            palette[i].red   = srcpal[i].red;
            palette[i].green = srcpal[i].green;
            palette[i].blue  = srcpal[i].blue;
        }

        return std::move(palette);
    }

//
// Read an indexed png of a specific bitdepth
//
    template<class _pngimagepixel, class _outTImg>
        void readPNG_indexed( _outTImg          & out_indexed, 
                              const std::string & filepath,
                              unsigned int        forcedwidth     = 0,
                              unsigned int        forcedheight    = 0,
                              bool                erroronwrongres = false )
    {
        png::image<_pngimagepixel> input;
        input.read( filepath, png::require_color_space<_pngimagepixel>() );

        const auto & mypalette         = input.get_palette();
        auto       & outpal            = out_indexed.getPalette();
        bool         isWrongResolution = false;

        //Only force resolution when we were asked to!
        if( forcedwidth != 0 && forcedheight != 0 )
            isWrongResolution = input.get_width() != forcedwidth || input.get_height() != forcedheight;
        if( erroronwrongres && isWrongResolution )
        {
            cerr <<"\n<!>-ERROR: The file : " <<filepath <<" has an unexpected resolution!\n"
                 <<"             Expected :" <<forcedwidth <<"x" <<forcedheight <<", and got " <<input.get_width() <<"x" <<input.get_height() 
                 <<"! Skipping!\n";
            return;
        }

        if( pngpix_pal_len<_pngimagepixel>::nbcolors != mypalette.size() )
        {
            //Mention the palette length mismatch
            if( utils::LibraryWide::getInstance().Data().isVerboseOn() )
            {
                cerr <<"\n<!>-Warning: " <<filepath <<" has a different palette length than expected!\n"
                     <<"Fixing and continuing happily..\n";
            }

            //Adjust the palette to the appropriate nb of colors
            out_indexed.setNbColors(pngpix_pal_len<_pngimagepixel>::nbcolors);
        }
        else
            out_indexed.setNbColors( mypalette.size() );

        //Build palette
        for( unsigned int i = 0; i < mypalette.size(); ++i )
        {
            outpal[i].red   = mypalette[i].red;
            outpal[i].green = mypalette[i].green;
            outpal[i].blue  = mypalette[i].blue;
        }

        //Image Resolution
        int tiledwidth  = (forcedwidth  != 0)? forcedwidth  : input.get_width();
        int tiledheight = (forcedheight != 0)? forcedheight : input.get_height();

        //Make sure the height and width are divisible by the size of the tiles!
        if( tiledwidth % _outTImg::tile_t::WIDTH )
            tiledwidth = CalcClosestHighestDenominator( tiledwidth,  _outTImg::tile_t::WIDTH );

        if( tiledheight % _outTImg::tile_t::HEIGHT )
            tiledheight = CalcClosestHighestDenominator( tiledheight,  _outTImg::tile_t::HEIGHT );

        //Resize target image
        out_indexed.setPixelResolution( tiledwidth, tiledheight );

        //If the image we read is not divisible by the dimension of our tiles, 
        // we have to ensure that we won't got out of bound while copying!
        unsigned int maxCopyWidth  = out_indexed.getNbPixelWidth();
        unsigned int maxCopyHeight = out_indexed.getNbPixelHeight();

        if( maxCopyWidth != input.get_width() || maxCopyHeight != input.get_height() )
        {
            //Take the smallest resolution, so we don't go out of bound!
            maxCopyWidth  = std::min( static_cast<unsigned int>(input.get_width()),  maxCopyWidth );
            maxCopyHeight = std::min( static_cast<unsigned int>(input.get_height()), maxCopyHeight );
        }

        //Fill the pixels
        //out_indexed.setPixelResolution( input.get_width(), input.get_height() );

        for( unsigned int i = 0; i < maxCopyWidth; ++i )
        {
            for( unsigned int j = 0; j < maxCopyHeight; ++j )
            {
                out_indexed.getPixel( i, j ) = _outTImg::pixel_t::GetAcomponentBitmask(0) & static_cast<uint8_t>( input.get_pixel(i,j) );
            }
        }
    }

//==============================================================================================
//  Import/Export from/to 4bpp
//==============================================================================================

    bool ImportFrom4bppPNG( gimg::tiled_image_i4bpp  & out_indexed,
                            const std::string        & filepath,
                            unsigned int              forcedwidth,
                            unsigned int              forcedheight,
                            bool                      erroronwrongres )
    {
        bool HasRead_4bpp_failed = false;

        //Try reading the image as 4 bpp
        try
        {
            readPNG_indexed<png::index_pixel_4>( out_indexed, filepath );
        }
        catch( png::error e )
        {
            //cerr << "<!>- PNG image is not in 4 bpp format.. Attempting 8 bpp !\n";
            HasRead_4bpp_failed = true;
        }

        if( !HasRead_4bpp_failed )
            return true;

        //Try again, reading as 8 bpp this time
        try
        {
            readPNG_indexed<png::index_pixel>( out_indexed, filepath );
        }
        catch( const png::error & )
        {
            cerr << "<!>- PNG image is not in 4 or 8 bpp format.. Not using a palette would lead to unforseen consequences!\n"
                 << "     The required input format is an indexed PNG, 4 or 8 bits per pixels, 16 colors !\n";
            return false;
        }

        return true;
    }


    bool ExportTo4bppPNG( const gimg::tiled_image_i4bpp  & in_indexed,
                          const std::string              & filepath )
    {
        png::image<png::index_pixel_4> output;
        //png::palette                   palette(in_indexed.getNbColors());

        ////copy palette
        //for( unsigned int i = 0; i < palette.size(); ++i )
        //{
        //    palette[i].red   = in_indexed.getPalette()[i].red;
        //    palette[i].green = in_indexed.getPalette()[i].green;
        //    palette[i].blue  = in_indexed.getPalette()[i].blue;
        //}
        output.set_palette( PalToPngPal(in_indexed.getPalette()) );

        //Copy image
        output.resize( in_indexed.getNbPixelWidth(), in_indexed.getNbPixelHeight() );

        for( unsigned int i = 0; i < output.get_width(); ++i )
        {
            for( unsigned int j = 0; j < output.get_height(); ++j )
            {
                auto &  refpixel = in_indexed.getPixel( i, j );
                uint8_t temp     = static_cast<uint8_t>( refpixel.getWholePixelData() );
                //output[j][i] = png::index_pixel_4( temp );
                output.set_pixel( i,j, temp ); //If only one component returns the entire pixel data
            }
        }

        try
        {
            output.write( filepath );
        }
        catch( const std::exception & e )
        {
            cerr << "<!>- Error outputing image : " << filepath <<"\n"
                 << "     Exception details : \n"     
                 << "        " <<e.what()  <<"\n";

            assert(false);
            return false;
        }
        return true;
    }

//==============================================================================================
//  Import/Export from/to 8bpp
//==============================================================================================

    bool ImportFrom8bppPNG( gimg::tiled_image_i8bpp & out_indexed,
                            const std::string       & filepath, 
                            unsigned int              forcedwidth,
                            unsigned int              forcedheight,
                            bool                      erroronwrongres )
    {
        try
        {
            readPNG_indexed<png::index_pixel>( out_indexed, filepath );
        }
        catch( const png::error & e )
        {
            static const uint32_t bpp = gimg::tiled_image_i8bpp::pixel_t::mypixeltrait_t::BITS_PER_PIXEL;
            cerr << "<!>- PNG image is not in " <<bpp <<" bpp format.. Not using a palette would lead to unforseen consequences!\n"
                 << "     The required input format is an indexed PNG, " <<bpp <<" bits per pixels, "
                 <<utils::do_exponent_of_2_<bpp>::value  <<" colors !\n";
            return false;
        }

        return true;
    }


    bool ExportTo8bppPNG( const gimg::tiled_image_i8bpp & in_indexed,
                          const std::string             & filepath )
    {
        png::image<png::index_pixel> output;
        output.set_palette( PalToPngPal(in_indexed.getPalette()) );

        //Copy image
        output.resize( in_indexed.getNbPixelWidth(), in_indexed.getNbPixelHeight() );

        for( unsigned int i = 0; i < output.get_width(); ++i )
        {
            for( unsigned int j = 0; j < output.get_height(); ++j )
                output.set_pixel( i,j, static_cast<uint8_t>( in_indexed.getPixel( i, j ).getWholePixelData() ) ); //If only one component returns the entire pixel data
        }

        try
        {
            output.write( filepath );
        }
        catch( const std::exception & e )
        {
            cerr << "<!>- Error outputing image : " << filepath <<"\n"
                 << "     Exception details : \n"     
                 << "        " <<e.what()  <<"\n";

            assert(false);
            return false;
        }
        return true;
    }

    bool ExportTo8bppPNG( const gimg::tiled_image_i8bpp & in_indexed,
                          const std::string             & filepath,
                          unsigned int                    begpixX,
                          unsigned int                    begpixY,
                          unsigned int                    endpixX,
                          unsigned int                    endpixY )
    {
        png::image<png::index_pixel> output;
        output.set_palette( PalToPngPal(in_indexed.getPalette()) );
        const size_t srcMaxX = in_indexed.getNbPixelWidth()  - endpixX;
        const size_t srcMaxY = in_indexed.getNbPixelHeight() - endpixY;

        //Copy image
        output.resize( (srcMaxX - begpixX), (srcMaxY - begpixY) );

        for( unsigned int i = 0; i < output.get_width(); ++i )
        {
            for( unsigned int j = 0; j < output.get_height(); ++j )
            {
                size_t srcX = i + begpixX;
                size_t srcY = j + begpixY;

                if( srcX < srcMaxX && srcY < srcMaxY )
                    output.set_pixel( i,j, static_cast<uint8_t>( in_indexed.getPixel( srcX, srcY ).getWholePixelData() ) );
                else 
                    break;
            }
        }

        try
        {
            output.write( filepath );
        }
        catch( const std::exception & e )
        {
            cerr << "<!>- Error outputing image : " << filepath <<"\n"
                 << "     Exception details : \n"     
                 << "        " <<e.what()  <<"\n";

            assert(false);
            return false;
        }
        return true;
    }


    std::vector<gimg::colorRGB24> ImportPaletteFromPNG( const std::string & filepath )
    {
        fstream                   inimg( filepath, std::ios_base::in | std::ios_base::binary );
        png::reader<std::fstream> reader(inimg);
        reader.read_info();

        std::vector<gimg::colorRGB24> outpal;
        const auto & mypalette = reader.get_image_info().get_palette();
        outpal.resize( mypalette.size() );

        //Build palette
        for( unsigned int i = 0; i < mypalette.size(); ++i )
        {
            outpal[i].red   = mypalette[i].red;
            outpal[i].green = mypalette[i].green;
            outpal[i].blue  = mypalette[i].blue;
        }

        return std::move( outpal );
    }

    void SetPalettePNGImg( const std::vector<gimg::colorRGB24> & srcpal, 
                           const std::string                   & filepath )
    {
        fstream                    inimg(filepath, std::ios_base::in | std::ios_base::binary );
        png::reader< std::fstream > reader(inimg);

        reader.read_info();
        png::color_type colorType = reader.get_color_type();
        const uint32_t  bitdepth  = reader.get_bit_depth();

        if( colorType != png::color_type::color_type_palette )  
            throw runtime_error( "Error: the image to inject a palette into does not have already a color palette !" );

        //Build palette
        png::palette mypalette( srcpal.size() );
        for( unsigned int i = 0; i < mypalette.size(); ++i )
        {
            mypalette[i].red    = srcpal[i].red;
            mypalette[i].green  = srcpal[i].green;
            mypalette[i].blue   = srcpal[i].blue;
        }

        //Set palette
        if( bitdepth == 4 )
        {
            png::image<png::index_pixel_4> tmpimg;
            tmpimg.read( filepath, png::require_color_space<png::index_pixel_4>() );
            tmpimg.set_palette( mypalette );
            tmpimg.write(filepath);
        }
        else if( bitdepth == 8 )
        {
            png::image<png::index_pixel> tmpimg;
            tmpimg.read( filepath, png::require_color_space<png::index_pixel>() );
            tmpimg.set_palette( mypalette );
            tmpimg.write(filepath);
        }
        else
            throw runtime_error("Error: the image to inject a palette into has an unsupported bitdepth!");
    }

    image_format_info GetPNGImgInfo(const std::string & filepath)
    {
        image_format_info         imginf;
        fstream                   inimg(filepath, std::ios_base::in | std::ios_base::binary );
        png::reader<std::fstream> reader(inimg);

        reader.read_info();
        png::color_type colorType = reader.get_color_type();

        imginf.usesPalette = colorType == png::color_type::color_type_palette;
        imginf.bitdepth    = reader.get_bit_depth();
        imginf.height      = reader.get_height();
        imginf.width       = reader.get_width();

        return imginf;
    }

//==============================================================================================
//  Import/Export from/to 24bits RGB PNG
//==============================================================================================

//================================================================================================
//  Generic Specializatin
//================================================================================================
    template<>
        bool ExportToPNG( const gimg::tiled_image_i4bpp & in_indexed,
                          const std::string             & filepath )
    {
        return ExportTo4bppPNG( in_indexed, filepath );
    }

    template<>
        bool ExportToPNG( const gimg::tiled_image_i8bpp & in_indexed,
                          const std::string             & filepath )
    {
        return ExportTo8bppPNG( in_indexed, filepath );
    }


    template<>
        bool ExportToPNG_AndCrop(   const gimg::tiled_image_i8bpp     & in_indexed,
                                    const std::string                 & filepath,
                                    unsigned int                        begpixX,
                                    unsigned int                        begpixY,
                                    unsigned int                        endpixX,
                                    unsigned int                        endpixY )
        {
            return ExportTo8bppPNG(in_indexed, filepath, begpixX, begpixY, endpixX, endpixY);
        }
                





    template<>
        bool ImportFromPNG( gimg::tiled_image_i4bpp & out_indexed,
                            const std::string       & filepath, 
                            unsigned int              forcedwidth,
                            unsigned int              forcedheight,
                            bool                      erroronwrongres )
    {
        return ImportFrom4bppPNG( out_indexed, filepath, forcedwidth, forcedheight, erroronwrongres );
    }

    template<>
        bool ImportFromPNG( gimg::tiled_image_i8bpp & out_indexed,
                            const std::string       & filepath, 
                            unsigned int              forcedwidth,
                            unsigned int              forcedheight,
                            bool                      erroronwrongres )
    {
        return ImportFrom8bppPNG( out_indexed, filepath, forcedwidth, forcedheight, erroronwrongres );
    }


    bool ExportToPNG( std::vector<gimg::colorRGBX32>    & bitmap,
                      const std::string                 & filepath, 
                      unsigned int                      forcedwidth,
                      unsigned int                      forcedheight,
                      bool                              erroronwrongres )
    {
        png::image<png::rgba_pixel> output;

        //Copy image
        output.resize( forcedwidth, forcedheight );

        auto itpixel = bitmap.begin();
        for( unsigned int i = 0; i < output.get_width(); ++i )
        {
            for( unsigned int j = 0; j < output.get_height(); ++j )
            {
                png::rgba_pixel pix;
                if( itpixel != bitmap.end() )
                {
                    pix.alpha = 255;
                    pix.red   = itpixel->_red;
                    pix.blue  = itpixel->_blue;
                    pix.green = itpixel->_green;
                    ++itpixel;
                }
                else //In case some pixels have no data
                {
                    pix.alpha = 255;
                    pix.red   = 255;
                    pix.blue  = 255;
                    pix.green = 255;
                }
                output.set_pixel( i,j,  pix ); //If only one component returns the entire pixel data
            }
        }

        try
        {
            output.write( filepath );
        }
        catch( const std::exception & e )
        {
            cerr << "<!>- Error outputing image : " << filepath <<"\n"
                 << "     Exception details : \n"     
                 << "        " <<e.what()  <<"\n";

            assert(false);
            return false;
        }
        return true;
    }

    /*
        ExportToPNG
    */
    bool ExportToPNG( const std::vector<std::vector<uint8_t>>   & indexed8bpp,
                      const std::vector<gimg::colorRGB24>       & palette,
                      const std::string                         & filepath, 
                      bool                                        erroronwrongres)
    {
        png::image<png::index_pixel> output;
        output.set_palette( PalToPngPal(palette) );

        //Copy image
        output.resize( indexed8bpp.front().size(), indexed8bpp.size() );

        for( unsigned int i = 0; i < output.get_width(); ++i )
        {
            for( unsigned int j = 0; j < output.get_height(); ++j )
                output.set_pixel( i,j, indexed8bpp[j][i] );
        }

        try
        {
            output.write( filepath );
        }
        catch( const std::exception & e )
        {
            cerr << "<!>- Error outputing image : " << filepath <<"\n"
                 << "     Exception details : \n"     
                 << "        " <<e.what()  <<"\n";

            assert(false);
            return false;
        }
        return true;
    }


};};