#include "bmp_io.hpp"
#include <EasyBMP/EasyBMP.h>
#include <utils/library_wide.hpp>
#include <utils/handymath.hpp>
#include <iostream>
#include <algorithm>
using namespace std;

namespace utils{ namespace io
{
    //A little operator to make dealing with finding the color index easier
    inline bool operator==( const RGBApixel & a, const gimg::colorRGB24 & b )
    {
        return (a.Red == b.red) && (a.Green == b.green) && (a.Blue == b.blue);
    }

    //A little operator to make converting color types easier
    inline RGBApixel colorRGB24ToRGBApixel( const gimg::colorRGB24 & c )
    {
        RGBApixel tmp;
        tmp.Red   = c.red;
        tmp.Green = c.green;
        tmp.Blue  = c.blue;
        return tmp;
    }

    //A little operator to make converting color types easier
    inline gimg::colorRGB24 RGBApixelTocolorRGB24( const RGBApixel & c )
    {
        gimg::colorRGB24 tmp;
        tmp.red   = c.Red;
        tmp.green = c.Green;
        tmp.blue  = c.Blue;
        return tmp;
    }

    //Returns -1 if it doesn't find the color !
    template< class paletteT >
        inline int FindIndexForColor( const RGBApixel & pix, const paletteT & palette )
    {
        for( unsigned int i = 0; i < palette.size(); ++i )
        {
            if( pix == palette[i] )
                return i;
        }

        return -1;
    }

//
//
//
    template<class _TImg_t>
        bool ImportBMP( _TImg_t           & out_timg, 
                        const std::string & filepath, 
                        unsigned int        forcedwidth, 
                        unsigned int        forcedheight, 
                        bool                erroronwrongres )
    {
        //The max nb of colors possible with that bitdepth!
        static const unsigned int NB_Colors_Support = utils::do_exponent_of_2_<_TImg_t::pixel_t::mypixeltrait_t::BITS_PER_PIXEL>::value;
        
        bool   hasWarnedOORPixel = false; //Whether we warned about out of range pixels at least once during the pixel loop! If applicable..
        bool   isWrongResolution = false;
        BMP    input;
        auto & outpal = out_timg.getPalette();

        input.ReadFromFile( filepath.c_str() );

        if( input.TellBitDepth() != _TImg_t::pixel_t::GetBitsPerPixel() )
        {
            //We don't support anything with a different bitdepth!
            //Mention the palette length mismatch
            cerr <<"\n<!>-ERROR: The file : " <<filepath <<", is not a " <<_TImg_t::pixel_t::GetBitsPerPixel() 
                 <<"bpp indexed bmp ! Its in fact " <<input.TellBitDepth() <<"bpp!\n"
                 <<"Make sure the bmp file is saved as " <<_TImg_t::pixel_t::GetBitsPerPixel() <<"bpp, "
                 <<NB_Colors_Support << " colors!\n";

            return false;
        }

        //Only force resolution when we were asked to!
        if( forcedwidth != 0 && forcedheight != 0 )
            isWrongResolution = input.TellWidth() != forcedwidth || input.TellHeight() != forcedheight;
        if( erroronwrongres && isWrongResolution )
        {
            cerr <<"\n<!>-ERROR: The file : " <<filepath <<" has an unexpected resolution!\n"
                 <<"             Expected :" <<forcedwidth <<"x" <<forcedheight <<", and got " <<input.TellWidth() <<"x" <<input.TellHeight() 
                 <<"! Skipping!\n";
            return false;
        }


        if( utils::LibraryWide::getInstance().Data().isVerboseOn() && input.TellNumberOfColors() <= NB_Colors_Support )
        {
            //Mention the palette length mismatch
            cerr <<"\n<!>-Warning: " <<filepath <<" has a different palette length than expected!\n"
                 <<"Fixing and continuing happily..\n";
        }
        out_timg.setNbColors( NB_Colors_Support );

        //Build palette
        const unsigned int nbColors = static_cast<unsigned int>( ( input.TellNumberOfColors() > 0 )? input.TellNumberOfColors() : 0 );
        if( nbColors == 0 )
            throw runtime_error("ERROR: BMP image being imported has an invalid palette!");
        for( unsigned int i = 0; i < nbColors; ++i )
        {
            RGBApixel acolor = input.GetColor(i);
            outpal[i].red   = acolor.Red;
            outpal[i].green = acolor.Green;
            outpal[i].blue  = acolor.Blue;
            //Alpha is ignored
        }

        //Image Resolution
        int tiledwidth  = (forcedwidth  != 0)? forcedwidth  : input.TellWidth();
        int tiledheight = (forcedheight != 0)? forcedheight : input.TellHeight();

        //Make sure the height and width are divisible by the size of the tiles!
        if( tiledwidth % _TImg_t::tile_t::WIDTH )
            tiledwidth = CalcClosestHighestDenominator( tiledwidth,  _TImg_t::tile_t::WIDTH );

        if( tiledheight % _TImg_t::tile_t::HEIGHT )
            tiledheight = CalcClosestHighestDenominator( tiledheight,  _TImg_t::tile_t::HEIGHT );

        //Resize target image
        out_timg.setPixelResolution( tiledwidth, tiledheight );

        //If the image we read is not divisible by the dimension of our tiles, 
        // we have to ensure that we won't got out of bound while copying!
        int maxCopyWidth  = out_timg.getNbPixelWidth();
        int maxCopyHeight = out_timg.getNbPixelHeight();

        if( maxCopyWidth != input.TellWidth() || maxCopyHeight != input.TellHeight() )
        {
            //Take the smallest resolution, so we don't go out of bound!
            maxCopyWidth  = std::min( input.TellWidth(),  maxCopyWidth );
            maxCopyHeight = std::min( input.TellHeight(), maxCopyHeight );
        }

        //Copy pixels over
        for( int i = 0; i < maxCopyWidth; ++i )
        {
            for( int j = 0; j < maxCopyHeight; ++j )
            {
                RGBApixel apixel = input.GetPixel(i,j);

                //First we need to find out what index the color is..
                gimg::colorRGB24::colordata_t colorindex = FindIndexForColor( apixel, outpal );

                if( !hasWarnedOORPixel && colorindex == -1 )
                {
                    //We got a problem
                    cerr <<"\n<!>-Warning: Image " <<filepath <<", has pixels with colors that aren't in the colormap/palette!\n"
                         <<"Defaulting pixels out of range to color 0!\n";
                    hasWarnedOORPixel = true;
                    colorindex        = 0;
                }

                out_timg.getPixel( i, j ) = colorindex;
            }
        }

        return true;
    }

    template<class _TImg_t>
        bool ExportBMP( const _TImg_t     & in_indexed,
                        const std::string & filepath )
    {
        //shorten this static constant
        typedef utils::do_exponent_of_2_<_TImg_t::pixel_t::mypixeltrait_t::BITS_PER_PIXEL> NbColorsPP_t;
        BMP output;
        output.SetBitDepth(_TImg_t::pixel_t::GetBitsPerPixel());

        if( in_indexed.getNbColors() != NbColorsPP_t::value )
        {
#ifdef _DEBUG
            assert(false);
#endif
            throw std::runtime_error( "ERROR: The tiled image to write to a bitmap image file has an invalid amount of color in its palette!" );
        }
        //copy palette
        for( int i = 0; i < NbColorsPP_t::value; ++i )
            output.SetColor( i, colorRGB24ToRGBApixel( in_indexed.getPalette()[i] ) );

        //Copy image
        output.SetSize( in_indexed.getNbPixelWidth(), in_indexed.getNbPixelHeight() );

        for( int i = 0; i < output.TellWidth(); ++i )
        {
            for( int j = 0; j < output.TellHeight(); ++j )
            {
                auto &  refpixel = in_indexed.getPixel( i, j );
                uint8_t temp     = static_cast<uint8_t>( refpixel.getWholePixelData() );
                output.SetPixel( i,j, colorRGB24ToRGBApixel( in_indexed.getPalette()[temp] ) ); //We need to input the color directly thnaks to EasyBMP
            }
        }

        bool bsuccess = false;
        try
        {
            bsuccess = output.WriteToFile( filepath.c_str() );
        }
        catch( std::exception e )
        {
            cerr << "<!>- Error outputing image : " << filepath <<"\n"
                 << "     Exception details : \n"     
                 << "        " <<e.what()  <<"\n";

            assert(false);
            bsuccess = false;
        }
        return bsuccess;
    }


    std::vector<gimg::colorRGB24> ImportPaletteFromBMP( const std::string & filepath )
    {
        std::vector<gimg::colorRGB24> outpal;
        BMP    input;
        input.ReadFromFile( filepath.c_str() );

        const unsigned int nbColors = static_cast<unsigned int>( ( input.TellNumberOfColors() > 0 )? input.TellNumberOfColors() : 0 );
        if( nbColors == 0 )
            throw runtime_error("ERROR: BMP image being imported has an invalid palette!");

        outpal.resize( nbColors );

        for( unsigned int i = 0; i < nbColors; ++i )
        {
            RGBApixel acolor = input.GetColor(i);
            outpal[i].red   = acolor.Red;
            outpal[i].green = acolor.Green;
            outpal[i].blue  = acolor.Blue;
            //Alpha is ignored
        }

        return std::move(outpal);
    }
    
    void SetPaletteBMPImg( const std::vector<gimg::colorRGB24> & srcpal, 
                           const std::string                   & filepath )
    {
        BMP      input;
        uint32_t nbcolstocopy = srcpal.size();
        input.ReadFromFile( filepath.c_str() );

        //Copy input img into output img
        BMP output(input);
        
        //Sanity check + for removing issues with signed/unsigned mismatch
        const unsigned int nbColorsOut = static_cast<unsigned int>( ( output.TellNumberOfColors() > 0 )? output.TellNumberOfColors() : 0 );
        if( nbColorsOut == 0 )
            throw runtime_error("ERROR: BMP image being imported has an invalid palette!");

        if( nbColorsOut < srcpal.size() )
        {
            nbcolstocopy = nbColorsOut;
            cerr <<"WARNING: the palette being injected into \"" <<filepath
                 <<"\" is larger than the palette of the image! Palette has " <<srcpal.size() <<" colors, while the image has "
                 <<output.TellNumberOfColors() <<" colors! Only  the first" <<output.TellNumberOfColors() <<" colors will be copied!\n";
        }
        else if( nbColorsOut > srcpal.size() )
        {
            nbcolstocopy = srcpal.size();
            cerr <<"WARNING: the palette being injected into "  <<filepath
                 <<" is smaller than the palette of the image! Only " <<srcpal.size() <<" colors will be written to the image!\n";
        }
         
        //reset colors to 0
        for( unsigned int i = 0; i < nbColorsOut; ++i )
            output.SetColor( i, RGBApixel() );

        //Copy colors
        for( unsigned int i = 0; i < nbcolstocopy; ++i )
        {
            RGBApixel acolor;
            acolor.Red   = srcpal[i].red;
            acolor.Green = srcpal[i].green;
            acolor.Blue  = srcpal[i].blue;
            acolor.Alpha = 255; //Always opaque
            output.SetColor( i, acolor );
        }

        output.WriteToFile(filepath.c_str());
    }


    image_format_info GetBMPImgInfo( const std::string & filepath )
    {
        image_format_info myinfo;
        auto              imginf = GetBMIH(filepath.c_str());

        myinfo.usesPalette = imginf.biBitCount < 16;
        myinfo.bitdepth = imginf.biBitCount;
        myinfo.width    = imginf.biWidth;
        myinfo.height   = imginf.biHeight;
        return myinfo;
    }

    template<>
        bool ExportToBMP( const gimg::tiled_image_i4bpp & in_indexed,
                          const std::string             & filepath )
    {
        return ExportBMP(in_indexed,filepath);
    }

    template<>
        bool ExportToBMP( const gimg::tiled_image_i8bpp & in_indexed,
                          const std::string             & filepath )
    {
        return ExportBMP(in_indexed,filepath);
    }

    template<>
        bool ImportFromBMP( gimg::tiled_image_i4bpp & out_indexed,
                            const std::string       & filepath, 
                            unsigned int              forcedwidth,
                            unsigned int              forcedheight,
                            bool                      erroronwrongres )
    {
        return ImportBMP( out_indexed, filepath, forcedwidth, forcedheight, erroronwrongres );
    }

    template<>
        bool ImportFromBMP( gimg::tiled_image_i8bpp & out_indexed,
                            const std::string       & filepath, 
                            unsigned int              forcedwidth,
                            unsigned int              forcedheight,
                            bool                      erroronwrongres )
    {
        return ImportBMP( out_indexed, filepath, forcedwidth, forcedheight, erroronwrongres );
    }

};};