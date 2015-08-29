#include "pmd2_palettes.hpp"

#include <algorithm>
#include <array>
#include <iostream>

using namespace std;

namespace filetypes
{
    const ContentTy CntTy_RawRGBX32 {"rgbx32"}; //Content ID handle
};

namespace pmd2 { namespace graphics 
{
//==================================================================
// Constants
//==================================================================
    //static const uint16_t MASK_UPPER      = 0x7C00, // 0111 1100 0000 0000  
    //                      MASK_MIDDLE     = 0x3E0,  // 0000 0011 1110 0000  
    //                      MASK_LOWER      = 0x1F;   // 0000 0000 0001 1111 
    static const uint8_t  MASK_BYTE_5BITS = 0x1F;   // 0001 1111 



//==================================================================
// rgbx32_parser
//==================================================================
    rgbx32_parser::rgbx32_parser( std::vector<colRGB24>::iterator itoutpalette )
        :_curchannel(0), _curcolor(itoutpalette)
    {}

    void rgbx32_parser::operator()( uint8_t abyte )
    {
        if( _curchannel != 3 ) //Avoid the dummy 0x80 byte 
            (*_curcolor)[_curchannel] = abyte;

        ++_curchannel;

        if( _curchannel >= 4 )  //If we did all 4 bytes of a single color, go to the next color!
        {
            ++_curcolor;
            _curchannel = 0;
        }
    }

//==================================================================
// rgb15_parser
//==================================================================
    rgb15_parser::rgb15_parser( std::vector<colRGB24>::iterator itoutpalette )
        :_curchannel(0), _curcolor(itoutpalette)
    {}

    //Takes a byte from the palette raw data
    void rgb15_parser::operator()( uint8_t abyte )
    {
        uint8_t channelvalue = abyte;// (abyte >> 3) & MASK_BYTE_5BITS; //We need to shift them to the right by 3 
        //channelvalue = ( (channelvalue * 254) / 31); //Re-scale to 8-bit per channel intensity range

        (*_curcolor)[_curchannel] = channelvalue; 

        ++_curchannel;

        if( _curchannel >= 3 )  //If we did all 3 bytes of a single color, go to the next color!
        {
            ++_curcolor;
            _curchannel = 0;
        }
    }

//==================================================================
// indexed_to_nonindexed
//==================================================================
    //indexed_to_nonindexed::indexed_to_nonindexed( const std::vector<colorRGB24> & palette, 
    //                                              std::vector<colorRGB24>::iterator & inout_itimg )
    //    :_palette(palette), _itimg(inout_itimg)
    //{}

//==================================================================
// indexedcolor_4bits_to_32bits_bitmap
//==================================================================
    //indexedcolor_4bits_to_24bits_pixels::indexedcolor_4bits_to_24bits_pixels( const std::vector<colorRGB24> & palette, 
    //                                                                          std::vector<colorRGB24>::iterator & inout_itimg )
    //:indexed_to_nonindexed(palette, inout_itimg)
    //{

    //}

    //void indexedcolor_4bits_to_24bits_pixels::operator()( uint8_t pixels )
    //{
    //    for( int i = 0; i < 2; ++i, ++_itimg ) //increment the output iterator!
    //    {
    //        uint8_t colindex = ( (pixels >> (i * 4) ) & 0x0F );
    //        const colorRGB24 & mycolor = _palette[colindex];
    //        _itimg->red   = mycolor.red;
    //        _itimg->green = mycolor.green;
    //        _itimg->blue  = mycolor.blue;
    //        //_itimg->alpha = (colindex)? 255u : 0u; //If we're color 0, its the non-opaque color!
    //    }
    //}


//==================================================================
// indexedcolor_8bits_to_32bits_bitmap
//==================================================================

    //indexedcolor_8bits_to_24bits_pixels::indexedcolor_8bits_to_24bits_pixels( const std::vector<colorRGB24> & palette, 
    //                                                                          std::vector<colorRGB24>::iterator & inout_itimg )
    //:indexed_to_nonindexed(palette, inout_itimg)
    //{
    //}

    //void indexedcolor_8bits_to_24bits_pixels::operator()( uint8_t pixel )
    //{
    //    const colorRGB24 & mycolor = _palette[pixel];
    //    _itimg->red   = mycolor.red;
    //    _itimg->green = mycolor.green;
    //    _itimg->blue  = mycolor.blue;
    //    //_itimg->alpha = (pixel)? 255u : 0u; //If we're color 0, its the non-opaque color!

    //    ++_itimg; //increment the output iterator!
    //}
};};

