#ifndef PMD2_IMAGE_FORMATS_HPP
#define PMD2_IMAGE_FORMATS_HPP
/*
pmd2_image_formats.hpp
psycommando@gmail.com
2014/09/23
Description: A bunch of utilities for dealing with the image formats
             used by pmd2. 
             Mainly misc stuff that applies to a lot of different 
             things.
*/
#include <ppmdu/pmd2/pmd2_palettes.hpp>
#include <utils/utility.hpp>
#include <array>

//#TODO: Clean up this file. Its not very true to its name right now !
//       We'll want to isolate the PNG stuff in another header!

namespace pmd2 { namespace graphics
{
//==================================================================
// Typedefs
//==================================================================
    //#REMOVEME: Those are deprecated since the introduction of the tiled_image container!
    //A few Typedefs for clarity's sake + possible modifications in the future!
    typedef std::vector<std::vector<uint8_t> > bitmap8bpp_t;     //An indexed bitmap(raster) image, 8 bits per pixel
    typedef std::vector<uint8_t>               indexed8bppimg_t; //An indexed tiled image, 8 bits per pixel 
    typedef std::vector<uint8_t>               indexed4bppimg_t; //An indexed tiled image, 4 bits per pixel
   
//==================================================================
// Constants
//==================================================================

    //Known Image Resolultions #TODO: Make a table with those
    static const utils::Resolution RES_INVALID      = {  0,  0 };

    //Legal resolutions for sprites
    static const utils::Resolution RES_8x8_SPRITE   = {  8,  8 };
    static const utils::Resolution RES_16x16_SPRITE = { 16, 16 };
    static const utils::Resolution RES_32x32_SPRITE = { 32, 32 };
    static const utils::Resolution RES_64x64_SPRITE = { 64, 64 };
    static const utils::Resolution RES_8x16_SPRITE  = {  8, 16 }; 
    static const utils::Resolution RES_16x8_SPRITE  = { 16,  8 }; 
    static const utils::Resolution RES_32x8_SPRITE  = { 32,  8 };
    static const utils::Resolution RES_8x32_SPRITE  = {  8, 32 };
    static const utils::Resolution RES_32x16_SPRITE = { 32, 16 };
    static const utils::Resolution RES_16x32_SPRITE = { 16, 32 };
    static const utils::Resolution RES_64x32_SPRITE = { 64, 32 };
    static const utils::Resolution RES_32x64_SPRITE = { 32, 64 };

    //Legal resolutions for portraits
    static const utils::Resolution RES_PORTRAIT     = { 40, 40 };


     
    //struct entryrestable
    //{
    //    uint32_t          bytesz;
    //    utils::Resolution resolution;
    //};

    //static const std::vector<entryrestable> NBPixelsToResolution=
    //{{
    //    {   64, RES_8x8_SPRITE   },
    //    {  256, RES_16x16_SPRITE },
    //    {  512, RES_32x32_SPRITE },
    //    { 4096, RES_64x64_SPRITE },
    //    {  128, RES_8x16_SPRITE  },

    //}};

    //This function returns the appropriate Resolution for the specified amount of pixels
    // of the image data. 
    //const utils::Resolution & GetImgResolutionForPixelAmt( uint32_t pixelamount );
 


//==================================================================
// Structs
//==================================================================

    /*******************************************************
        RLE_TableEntry
            Simple struct containing the raw data of an
            RLE table in-between the time its stored in 
            memory and the time its written to file.
    *******************************************************/
    //struct RLE_TableEntry
    //{
    //    uint32_t  pixelsource_,  //Either an offset within the file, or a value to repeat several times.
    //              pixelamount_,  //Either the length to read at the offset, or the amount of times to repeat the value of pixel source
    //              unknown_val_;  //No ideas what its for at this point
    //    bool      bPSourceIsOffset;

    //    RLE_TableEntry( uint32_t pixelsrc = 0, uint32_t pixelamt = 0, uint32_t unknval = 0 );

    //    bool isNullEntry()const;
    //    bool isPixelSourceOffset()const;
    //    void setPixelSourceIsOffset( bool ispsourceoffset ); //Set wether the "pixelsource_" value is an offset, or not

    //    uint32_t & operator[]( unsigned int index );
    //};

//==================================================================
// PMD2 Sprite RLE Functions
//==================================================================

    ////#TODO : Make a functors for all that stuff. It will help reduce the massive
    ////        amount of sub-function we got in the cpp right now !

    ////#TODO : Change the logic behind the RLE encoding, because the game seems to actually not
    ////        handle encoding anything but sequences of zeros.. 

    ////read the data of a frameblock inside a vector of chars. 
    ////The itterator should be at the FRM_IN position of the ptr table near the end of the frame data.
    ////The second iterator should be either the FRM_BEG of the next frame block, or the PAL_BEG if its the last frame block !
    //// in_rawfile is the vector containing all the raw data for the whole file
    //void RLE_DecodeFrame( const std::vector<uint8_t>      & in_rawfile, 
    //                       std::vector<uint8_t>::iterator   ittptrs, 
    //                       std::vector<uint8_t>           & out_decompframe );

    ////Compress a frame to the PMD RLE format
    ////Input: 
    ////      - Raw image/frame data
    ////      - baseoffset : the offset to add to all the offset computed in the RLE table.
    ////Output:
    ////      - out_resultdata : The image data with the compressed sections removed 
    ////      - out_rletable   : The RLE table with all the compression info
    //void RLE_EncodeFrame( const std::vector<uint8_t>     & in_frame, 
    //                      std::vector<uint8_t>           & out_resultdata, 
    //                      std::vector<RLE_TableEntry>    & out_rletable, 
    //                      std::vector<uint8_t>::size_type  baseoffset );


    ////Compress a frame using the RLE format that PMD2 uses. And output both the uncompressable data and RLE table into a vector
    ////  of std::pair. This is to allow to adjust the offsets in the RLE table easily when comes the time to assemble the file back together 
    //void RLE_EncodeFrame_PushBackToVector( const std::vector<uint8_t> & in_frame, 
    //                                       std::vector< std::pair< std::vector<uint8_t>, std::vector<RLE_TableEntry> > > & out_result );


};};

#endif