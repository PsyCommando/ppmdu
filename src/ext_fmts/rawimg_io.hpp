#ifndef RAWIMG_IO_HPP
#define RAWIMG_IO_HPP
/*
rawimg_io.hpp
2014/12/24
psycommando@gmail.com
Description: 
    Function for exporting an image's raw data, to a file directly, long with a riff palette.
    And some more functions for a very simple custom container for raw image data extracted 
    from the pmd2 games and etc..
*/
#include <array>
#include <cstdint>
#include <string>
#include <ppmdu/containers/tiled_image.hpp>
#include <utils/utility.hpp>

namespace utils{ namespace io
{
//=====================================================================
//                          RAW Data IO
//=====================================================================

//--------------------------------
//  Export to raw pixels !
//--------------------------------
    /*
        Export an image to a tiled form, pixel by pixel into a 
        file, without any other form of processing or a header.
    */
    template<class _TImg_t>
        bool ExportRawImg( const _TImg_t     & in_indexed,
                           const std::string & filepath );

    /*
        Export an image to a tiled form, pixel by pixel into a 
        file, without any other form of processing or a header.
        #This one exports only the pixel data, not the palette!
    */
    template<class _TImg_t>
        bool ExportRawImg_NoPal( const _TImg_t     & in_indexed,
                                 const std::string & filepath );

//--------------------------------
//  Import from ANY !
//--------------------------------
    //Generic Import Functions
    // Calls the correct function depending on the type of the tiled image!

    template<class _TImg_t>
        bool ImportRawImg( _TImg_t            & out_indexed,
                            const std::string & filepath,
                            utils::Resolution   imgres);
    
    template<class _TImg_t>
        bool ImportRawImg_NoPal( _TImg_t           & out_indexed, 
                                 const std::string & filepath, 
                                 utils::Resolution   imgres);


//=====================================================================
//                          PRI format
//=====================================================================

    //#TODO: Move PRI stuff to its own header and source file!

//=====================================================================
// Constants
//=====================================================================
    static const std::string RawImg_FileExtension = "ri";       // Raw image file extension
    static const std::string PRI_FileExtension    = "pri";      // PRI raw image container extension
    static const uint32_t    PRI_MagicNumber      = 0x5052496D; // "PRIm" { 0x50, 0x52, 0x49, 0x6D }
    static const uint32_t    PRI_PAL_id           = 0x50414C20; // "PAL\0x20" { 0x50, 0x41, 0x4C, 0x20 }
    static const uint32_t    PRI_IMG_id           = 0x494D4720; // "IMG\0x20" { 0x49, 0x4D, 0x47, 0x20 }

//=====================================================================
// Structs for PRI container
//=====================================================================
    struct pri_header
    {
        uint32_t magicn;     // "PRIm" { 0x50, 0x52, 0x49, 0x6D }
        uint32_t filelength; // total length of the file
        uint32_t ptrpal;     // pointer to the palette header chunk, if its there. Else is 0.
        uint32_t ptrimg;     // pointer to the image header chunk.
    };

    struct pri_palette_chunk_header
    {
        uint32_t           palid;      // "PAL\0x20" { 0x50, 0x41, 0x4C, 0x20 }
        uint32_t           offspaldat; // Offset from the beginning of the chunk header where the palette data begins.
        uint32_t           palLen;     // The length of the palette in bytes. Not counting header chunk.
        uint32_t           nbcolors;   // the nb of colors in the palette.
        //16
        //std::array<char,16> formatinfo; // A short string desribing the palette color format. Ex: "RGB24\0\0\0\0\0\0\0\0\0\0\0"
        //Its always possible to add extra custom data following the chunk header.
    };

    struct pri_image_chunk_header
    {
        //The image is automatically considered indexed if there is a palette chunk!
        //0
        uint32_t           imgid;         // "IMG\0x20" { 0x49, 0x4D, 0x47, 0x20 }
        uint32_t           offsimgdat;    // Offset from the beginning of the chunk header where the image data begins.
        uint32_t           imgdatalength; // The length of the image data, not counting the chunk header.
        uint16_t           bitdepth;      // the bitdepth of the image.
        uint16_t           bytesperpixel; // the nb of bytes per pixel used in the image.
        //16
        uint32_t           imgwidth;      // The width of the image in pixels
        uint32_t           imgheight;     // The height of the image in pixels
        uint32_t           tilewidth;     // The width of a single tile. If not tiled, is 1.
        uint32_t           tilewheight;   // The height of a single tile. If not tiled, is 1.

        //Its always possible to add extra custom data following the chunk header.
    };

//=====================================================================
// IO Function
//=====================================================================

    //#TODO: Implement !
    //Functions for the custom PRI raw image container 
    // "filepath" is the path to the file to be outputed. You must append the file extension to this path yourselves.
    //bool ImportFrom4bppPRI( gimg::tiled_image_i4bpp       & out_indexed, const std::string & filepath );
    //bool ExportTo4bppPRI(   const gimg::tiled_image_i4bpp & in_indexed,  const std::string & filepath );
};};

#endif