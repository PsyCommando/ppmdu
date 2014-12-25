#ifndef RAWIMG_IO_HPP
#define RAWIMG_IO_HPP
/*
rawimg_io.hpp
2014/12/24
psycommando@gmail.com
Description: A very simple custom container for raw image data extracted from the pmd2 games and etc..
*/
#include <array>
#include <cstdint>

namespace rawimg_io
{
//=====================================================================
// Constants
//=====================================================================
    static const uint32_t RAWIMG_MagicNumber = 0x5052496D; // "PRIm" { 0x50, 0x52, 0x49, 0x6D }
    static const uint32_t RAWIMG_PAL_id      = 0x50414C20; // "PAL\0x20" { 0x50, 0x41, 0x4C, 0x20 }
    static const uint32_t RAWIMG_IMG_id      = 0x494D4720; // "IMG\0x20" { 0x49, 0x4D, 0x47, 0x20 }

//=====================================================================
// Structs
//=====================================================================
    struct rawimg_header
    {
        uint32_t magicn;     // "PRIm" { 0x50, 0x52, 0x49, 0x6D }
        uint32_t filelength; // total length of the file
        uint32_t ptrpal;     // pointer to the palette header chunk, if its there. Else is 0.
        uint32_t ptrimg;     // pointer to the image header chunk.
    };

    struct palette_chunk_header
    {
        uint32_t           palid;      // "PAL\0x20" { 0x50, 0x41, 0x4C, 0x20 }
        uint32_t           offspaldat; // Offset from the beginning of the chunk header where the palette data begins.
        uint32_t           palLen;     // The length of the palette in bytes. Not counting header chunk.
        uint32_t           nbcolors;   // the nb of colors in the palette.
        //16
        //std::array<char,16> formatinfo; // A short string desribing the palette color format. Ex: "RGB24\0\0\0\0\0\0\0\0\0\0\0"
        //Its always possible to add extra custom data following the chunk header.
    };

    struct image_chunk_header
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
};

#endif