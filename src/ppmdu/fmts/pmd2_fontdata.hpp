#ifndef PMD2_FONT_DATA_HPP
#define PMD2_FONT_DATA_HPP
/*
pmd2_fontdata.hpp
2015/04/17
psycommando@gmail.com
Description:
    Contains utilities to handle the font data for the PMD2 games.
*/
#include <ppmdu/containers/linear_image.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace pmd2 { namespace filetypes 
{
//======================================================================================
//  Constants
//======================================================================================
    static const std::string FontEos_MainFontFile    = "kanji_rd.dat";
    static const std::string FontEos_MainFontDicFile = "kanji.dic";

//
//  Structs
//

    /*
        CharacterData
            Contains character data along with its image representation.
    */
    template<class _PIXEL_T>
        struct CharacterData
    {
        typedef gimg::linear_image<_PIXEL_T> img_t;

        uint16_t charcode = 0;
        uint16_t unk1     = 0;
        img_t    imgdat;
    };


//
//  Typedefs
//
    typedef std::vector<CharacterData<gimg::pixel_indexed_1bpp>> mfontdat_t;

//======================================================================================
//  Functions
//======================================================================================

    /*
        Main font data:
            Main 1bpp font used in dialogs and menus in the PMD2 Explorers of Sky game.
    */
    mfontdat_t           ParseMainFontData( const std::string          & fontfile );
    mfontdat_t           ParseMainFontData( const std::vector<uint8_t> & fontfiledata );

    std::vector<uint8_t> WriteMainFontData( const mfontdat_t           & imgsdata );
    void                 WriteMainFontData( const mfontdat_t           & imgsdata, 
                                            const std::string          & destfontfile );

    /*
        Main font data Import/Export:
            These function export the font data as 1 bpp images to a directory and import them back.

            The output format is simply a bunch of png images in a directory, and in their name is their character code
            in hexadecimal, followed with an underscore, followed with the unk1 value in hexadecimal. 
    */
    void                 ExportMainFontData( const mfontdat_t          & data, 
                                             const std::string         & destdir );
    mfontdat_t           ImportMainFontData( const std::string         & srcdir );


    /*
        Font Data:
            8bpp font data used in the PMD2 Explorers of Sky game.
    */
    std::vector<gimg::image_i8bpp> ParseRawFontFile( const std::string                    & fontfile );
    std::vector<gimg::image_i8bpp> ParseRawFontFile( const std::vector<uint8_t>           & fontfiledata );

    std::vector<uint8_t>           WriteRawFontFile( const std::vector<gimg::image_i8bpp> & fontdata );
    void                           WriteRawFontFile( const std::vector<gimg::image_i8bpp> & fontdata, const std::string & destfontfile );

    /*
        Font Data Import/Export :
            These functions export the font images as 8 bpp images into the specified directory.

            The output format is simply a directory containing one image for every character entry.
            Each images is numbered to match the character's entry index. Null entries are not exported.
            
            Each images has a copy of the palette, but only the first image's palette is loaded for all other characters.
    */
    void                           ExportFontData( const std::vector<gimg::image_i8bpp> & data, 
                                                   const std::string                    & destdir );
    std::vector<gimg::image_i8bpp> ImportFontData( const std::string                    & srcdir );


};};

#endif