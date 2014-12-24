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
#include <ppmdu/basetypes.hpp>
#include <ppmdu/pmd2/pmd2_palettes.hpp>
#include <ppmdu/utils/utility.hpp>
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
    static const utils::Resolution RES_INVALID       ={  0,  0 };
    static const utils::Resolution RES_8x8_SPRITE    ={  8,  8 }; 
    static const utils::Resolution RES_16x8_SPRITE   ={ 16,  8 }; 
    static const utils::Resolution RES_16x16_SPRITE  ={ 16, 16 };
    static const utils::Resolution RES_16x32_SPRITE  ={ 16, 32 };
    static const utils::Resolution RES_32x32_SPRITE  ={ 32, 32 };
    static const utils::Resolution RES_PORTRAIT      ={ 40, 40 };


    //struct entryrestable
    //{
    //    uint32_t          bytesz;
    //    utils::Resolution resolution;
    //};

    //Resolution to 4bpp, byte size map
    //static const std::array<entryrestable,3> BYTESZ_4BPP_TO_RES=
    //{{
    //    { 256u, RES_16x16_SPRITE },
    //    { 512u, RES_32x32_SPRITE },
    //    { 800u, RES_PORTRAIT     },
    //}};

    //This function returns the appropriate Resolution for the specified amount of pixels
    // of the image data. 
    const utils::Resolution & GetImgResolutionForPixelAmt( uint32_t pixelamount );
 


//==================================================================
// Structs
//==================================================================

    /*******************************************************
        rgb24pal_and_8bpp_tiled
            A simple utility struct to make things 
            less confusing than by using std::pairs !

            #REMOVEME: Not used anymore
    *******************************************************/
    //struct rgb24pal_and_8bpp_tiled
    //{
    //    graphics::rgb24palette_t   _palette;
    //    graphics::indexed8bppimg_t _8bpp_timg;
    //};

    /*******************************************************
        RLE_TableEntry
            Simple struct containing the raw data of an
            RLE table in-between the time its stored in 
            memory and the time its written to file.
    *******************************************************/
    struct RLE_TableEntry
    {
        uint32_t  pixelsource_,  //Either an offset within the file, or a value to repeat several times.
                  pixelamount_,  //Either the length to read at the offset, or the amount of times to repeat the value of pixel source
                  unknown_val_;  //No ideas what its for at this point
        bool      bPSourceIsOffset;

        RLE_TableEntry( uint32_t pixelsrc = 0, uint32_t pixelamt = 0, uint32_t unknval = 0 );

        bool isNullEntry()const;
        bool isPixelSourceOffset()const;
        void setPixelSourceIsOffset( bool ispsourceoffset ); //Set wether the "pixelsource_" value is an offset, or not

        uint32_t & operator[]( unsigned int index );
    };

//==================================================================
// Image Exporter
//==================================================================


    //#TODO: Write a functor version that takes a folder path
    //       and output all images converted in that folder!

    /*******************************************************
        export_8bppTiled_to_png
            Functor for exporting several 8 bpp indexed
            tiled image with their palette, to a png image
            into the same folder.

            Outputed images will be numbered in the order
            they are handled.
    *******************************************************/
    //class export_8bppTiled_to_png
    //{
    //public:
    //    /*
    //        - outputfolderpath : Sets the export path for the all the images to be exported by the functor.
    //        - filenameprefix   : Set the prefix to put before the number given to every image as filename.
    //    */
    //    export_8bppTiled_to_png( const std::string & outputfolderpath, const std::string & filenamesuffix = "" );

    //    //Handles a struct containing both palette and 8bpp tiled image
    //    //void operator()( const bitmap8bpp_t & indexed8bppbitmap, const rgb24palette_t & palette );

    //    void operator()(  const indexed8bppimg_t & indexed8bpptiledimg, const rgb24palette_t & palette );

    //    //Handles a struct containing both palette and 8bpp tiled image
    //    //void operator()( const rgb24pal_and_8bpp_tiled & combopalimg, 
    //    //                 const std::string             * psuffixoverride = nullptr, 
    //    //                 const uint32_t                * pnumberoverride = nullptr );

    //private:
    //    unsigned int m_cptimgname;
    //    std::string  m_suffix;
    //    std::string  m_folder;
    //};


    /*******************************************************
        Export_8bppIndexedBitmapToPNG
            Exports an indexed 8bpp image to PNG, using the 
            specified palette. 
    *******************************************************/
    void Export_8bppIndexedBitmapToPNG( const bitmap8bpp_t         & indexedimg, 
                                        const rgb24palette_t       & palette,
                                        const std::string          & filepath );

//==================================================================
// Image Importer
//==================================================================
 
    /*******************************************************
        TODO!
        Import8bppPNG_To_8bppImgAndPal
            Imports an indexed 8bpp PNG image to a palette, 
            and a raw 8bpp bitmap
    *******************************************************/
    void importPNG_To_8bppImgAndPal( std::vector<uint8_t> & out_indexedtiled8bppimg,
                                     rgb24palette_t       & out_palette,
                                     const std::string    & filepath );

    //void importPNG_To_4bppImgAndPal( indexed4bppimg_t  & out_indexedtiled4bppimg,
    //                                 rgb24palette_t    & out_palette,
    //                                 const std::string & filepath );

//==================================================================
// Image Format Converter
//==================================================================
    //unsigned int GetTiledIndexFromUntiledImageCoord( unsigned int x, unsigned int y, unsigned int height ); //#REMOVEME: DEPRECATED since tiled_image container

    /*******************************************************
        Untile4bpp
            Untile sprites, portraits, and other tiled 
            image data. 
            Takes 4bpp tiled image as input
    *******************************************************/
    //void Untile4bpp( unsigned int             width, 
    //                 unsigned int             height,
    //                 const indexed8bppimg_t & in_timg, 
    //                 bitmap8bpp_t           & out_bitmap );

    /*******************************************************
        Untile8bpp
            Untile sprites, portraits, and other tiled 
            image data. 
            Takes 8bpp tiled image as input
    *******************************************************/
    //void Untile8bpp( unsigned int                 width, 
    //                 unsigned int                 height,
    //                 const std::vector<uint8_t> & in_timg, 
    //                 bitmap8bpp_t               & out_bitmap );



    /*******************************************************
        Tile8bppBitmap
            Convert a 8bpp bitmap to a "tiled" sequence of
            bytes.
    *******************************************************/
    //void Tile8bppBitmapTo4bppRaw( const bitmap8bpp_t & bitmap, indexed4bppimg_t & out_timg );



    /*******************************************************
        Expand4bppTo8bpp
            Turns a 4bpp image into a 8bpp one. Simply copy 
            each nybbles into its own byte!
    *******************************************************/
    std::vector<uint8_t> Expand4bppTo8bpp( std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend );

    std::vector<uint8_t> Shrink8bppTo4bpp( std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend );
    void Shrink8bppTo4bpp( std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend, std::vector<uint8_t> & output );

//==================================================================
// #TODO: check if those are actually useful!
//==================================================================
    //Convert an indexed 8 bits bitmap to a 24 bits RGB bitmap
    //void ConvertToRGB_24bits_Raw_Bitmap( const std::vector< std::vector<uint8_t> > & in_indexed4bpp_bitmap, 
    //                                     std::vector< std::vector<colorRGB24> >    & out_24bits_rgb_bitmap );

    ////Convert an indexed 8 bits bitmap to a 24 bits RGB EasyBmp bitmap
    //void ConvertToRGBA_32bits_Easy_Bitmap( const std::vector< std::vector<uint8_t> > & in_indexed4bpp_bitmap, 
    //                                       BMP                                       & out_32bits_rgba_bmp );

//==================================================================
// PMD2 Sprite RLE Functions
//==================================================================

    //#TODO : Make a functors for all that stuff. It will help reduce the massive
    //        amount of sub-function we got in the cpp right now !

    //#TODO : Change the logic behind the RLE encoding, because the game seems to actually not
    //        handle encoding anything but sequences of zeros.. 

    //read the data of a frameblock inside a vector of chars. 
    //The itterator should be at the FRM_IN position of the ptr table near the end of the frame data.
    //The second iterator should be either the FRM_BEG of the next frame block, or the PAL_BEG if its the last frame block !
    // in_rawfile is the vector containing all the raw data for the whole file
    void RLE_DecodeFrame( const std::vector<uint8_t>      & in_rawfile, 
                           std::vector<uint8_t>::iterator   ittptrs, 
                           std::vector<uint8_t>           & out_decompframe );

    //Compress a frame to the PMD RLE format
    //Input: 
    //      - Raw image/frame data
    //      - baseoffset : the offset to add to all the offset computed in the RLE table.
    //Output:
    //      - out_resultdata : The image data with the compressed sections removed 
    //      - out_rletable   : The RLE table with all the compression info
    void RLE_EncodeFrame( const std::vector<uint8_t>     & in_frame, 
                          std::vector<uint8_t>           & out_resultdata, 
                          std::vector<RLE_TableEntry>    & out_rletable, 
                          std::vector<uint8_t>::size_type  baseoffset );


    //Compress a frame using the RLE format that PMD2 uses. And output both the uncompressable data and RLE table into a vector
    //  of std::pair. This is to allow to adjust the offsets in the RLE table easily when comes the time to assemble the file back together 
    void RLE_EncodeFrame_PushBackToVector( const std::vector<uint8_t> & in_frame, 
                                           std::vector< std::pair< std::vector<uint8_t>, std::vector<RLE_TableEntry> > > & out_result );


};};

#endif