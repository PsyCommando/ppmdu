#include "png_io.hpp"
#include <ppmdu/containers/tiled_image.hpp>
#include <ppmdu/pmd2/pmd2_palettes.hpp>
#include <ppmdu/utils/library_wide.hpp>
#include <png++/png.hpp>
#include <iostream>
using namespace std;

namespace pmd2{ namespace pngio
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
// Read an indexed png of a specific bitdepth
//
    template<class _pngimagepixel>
        void readPNG_indexed( gimg::tiled_image_i4bpp  & out_indexed, const std::string & filepath)
    {
        png::image<_pngimagepixel> input;
        input.read( filepath, png::require_color_space<_pngimagepixel>() );

        const auto & mypalette = input.get_palette();
        auto       & outpal    = out_indexed.getPalette();

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

        //Fill the pixels
        out_indexed.setPixelResolution( input.get_width(), input.get_height() );

        for( unsigned int i = 0; i < input.get_width(); ++i )
        {
            for( unsigned int j = 0; j < input.get_height(); ++j )
            {
                out_indexed.getPixel( i, j ) = gimg::pixel_indexed_4bpp::GetAcomponentBitmask(0) & 
                                               static_cast<uint8_t>( input.get_pixel(i,j) );
            }
        }
    }



    bool ImportFrom4bppPNG( gimg::tiled_image_i4bpp  & out_indexed,
                            const std::string        & filepath )
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
        catch( png::error e )
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
        png::palette                   palette(in_indexed.getNbColors());

        //copy palette
        for( unsigned int i = 0; i < palette.size(); ++i )
        {
            palette[i].red   = in_indexed.getPalette()[i].red;
            palette[i].green = in_indexed.getPalette()[i].green;
            palette[i].blue  = in_indexed.getPalette()[i].blue;
        }
        output.set_palette(palette);

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
        catch( std::exception e )
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