#ifndef PMD2_PALETTES_HPP
#define PMD2_PALETTES_HPP
/*
pmd2_palettes.hpp
psycommando@gmail.com
2014/09/23
Description: A bunch of utilities for dealing with palettes, and color data for pmd2!

#TODO: rename this ! It kinda grew to be a little different than its initial purpose !!
*/
#include <vector>
#include <array>
#include <cstdint>
#include <ppmdu/containers/color.hpp>
#include <types/content_type_analyser.hpp>

//Define the file type for the RGBX32 raw palette
namespace filetypes
{
    extern const ContentTy CntTy_RawRGBX32; //Content ID handle
};

namespace pmd2 { namespace graphics
{
    typedef gimg::colorRGB24 colRGB24;
 
//==================================================================
// Constants
//==================================================================
    static const uint8_t               RGBX_UNUSED_BYTE_VALUE          = 0x80u; //The value of the unused byte in the 32 bits palette used in PMD2
   // static const types::bytevec_szty_t PALETTE_15_BPC_1BPCHAN_AT4PX_SZ = 48; //The length of the 15bits per color, 1 byte per channel, 16 colors palette that come with AT4PX containers in some instances

    

//==================================================================
// Typedefs
//==================================================================
    typedef std::vector<colRGB24>  rgb24palette_t;

//==================================================================
// Raw Palettes to RGB24 Converters/Parser
//==================================================================
    /****************************************************************
        rgbx32_parser
            A functor for parsing a raw RGBX 32 bits color palette. 
            As used in most sprites files.
    ****************************************************************/
    class rgbx32_parser
    {
    public:
        //Takes an iterator to the output palette
        rgbx32_parser( std::vector<colRGB24>::iterator itoutpalette );

        //Takes a byte from the palette raw data
        void operator()( uint8_t abyte );

    private:
        std::vector<colRGB24>::iterator _curcolor;
        uint8_t                         _curchannel;
    };

    /****************************************************************
        rgb15_parser
            A functor for parsing a raw RGB 15 bits color palette. 
            Those are on 3 bytes, and in each of those, only the 
            first 5 bits are used. At least, in theory.. They're
            mainly used in the kaomado.kao file. 
    ****************************************************************/
    class rgb15_parser
    {
    public:
        //Takes an iterator to the output palette
        rgb15_parser( std::vector<colRGB24>::iterator itcolor );

        //Takes a byte from the palette raw data
        void operator()( uint8_t abyte );

    private:
        std::vector<colRGB24>::iterator _curcolor;
        uint8_t                           _curchannel;
    };

    //Read a palette to a container. Range to read must be divisible by colorRGB24::NB_COMPONENTS !
    template<class _init>
        inline _init ReadRawPalette_RGB24_As_RGB24( _init            itbeg,
                                                    _init            itend,
                                                    rgb24palette_t & out_palette )
    {
        colRGB24   temp;
        unsigned int size = std::distance( itbeg, itend );

        if( size % colRGB24::NB_COMPONENTS != 0  )
            throw std::length_error("ReadRawPalette_RGB24_As_RGB24() : Nb of bytes to build palette from not divisible by " + std::to_string(colRGB24::NB_COMPONENTS ));

        out_palette.reserve( size / colRGB24::NB_COMPONENTS );
        out_palette.resize(0);

        //Write palette
        while( itbeg != itend )
        {
            itbeg = temp.ReadAsRawByte( itbeg, itend );
            out_palette.push_back( std::move(temp) );
        }

        return itbeg;
    }

    //Writes a palette to a container. Expects to have enough room to output everything!
    template<class _outit>
        inline _outit WriteRawPalette_RGB24_As_RGB24( _outit itwhere, 
                                                      rgb24palette_t::const_iterator itpalbeg,
                                                      rgb24palette_t::const_iterator itpalend )
    {
        //Write palette
        while( itpalbeg != itpalend )
        {
            itwhere = itpalbeg->WriteAsRawByte( itwhere );
            ++itpalbeg;
        }

        return itwhere;
    }

    /****************************************************************
        ReadRawPalette_RGBX32_As_RGB24
            Read a palette to a container. 
            Length must be divisible by 4 !
    ****************************************************************/
    template<class _init>
        inline _init ReadRawPalette_RGBX32_As_RGB24( _init            itbeg,
                                                     _init            itend,
                                                     rgb24palette_t & out_palette )
    {
        unsigned int size = std::distance( itbeg, itend );

        if( size % 4u != 0u  )
            throw std::length_error("ReadRawPalette_RGBX32_As_RGB24() : Nb of bytes to build palette from not divisible by 4 !");

        out_palette.reserve( size / 4u );
        out_palette.resize(0u);

        //Write palette
        while( itbeg != itend )
        {
            colRGB24 temp;
            itbeg = temp.ReadAsRawByte( itbeg, itend, false );
            ++itbeg; //Skip the ignored 0x80 byte
            out_palette.push_back( std::move(temp) );
        }

        return itbeg;
    }

    /****************************************************************
        WriteRawPalette_RGB24_As_RGBX32
            Writes a palette to a container. 
            Expects to have enough room to output everything!
    ****************************************************************/
    template<class _outit>
        inline _outit WriteRawPalette_RGB24_As_RGBX32( _outit itwhere, 
                                                       rgb24palette_t::const_iterator itpalbeg,
                                                       rgb24palette_t::const_iterator itpalend )
    {
        //Write palette
        while( itpalbeg != itpalend )
        {
            itwhere = itpalbeg->WriteAsRawByte( itwhere );
            (*itwhere) = RGBX_UNUSED_BYTE_VALUE;
            ++itwhere;
            ++itpalbeg;
        }

        return itwhere;
    }



//==================================================================
// Indexed Image Data to Non-Indexed Converters
//================================================================== 

    // #TODO: everything in there should go !!!

    /****************************************************************
        indexed_to_nonindexed
            Parent class for our two functor below.
    ****************************************************************/
    //class indexed_to_nonindexed
    //{
    //public:
    //    //The source palette to use on the range when called
    //    indexed_to_nonindexed( const std::vector<colorRGB24> & palette, std::vector<colorRGB24>::iterator & inout_itimg );

    //    virtual void operator()( uint8_t ) = 0;

    //protected:
    //    const std::vector<colorRGB24> &      _palette;
    //    std::vector<colorRGB24>::iterator & _itimg;
    //};

    ///****************************************************************
    //    indexedcolor_4bits_to_24bits_pixels
    //        A functor for converting palette indexes into a list of
    //        RGB24 colors.
    //        Works on 4bits palette indexes. 
    //****************************************************************/
    //class indexedcolor_4bits_to_24bits_pixels : public indexed_to_nonindexed
    //{
    //public:
    //    //using indexed_to_nonindexed::indexed_to_nonindexed; //C++ 11 constructor inheritance, sadly, msvc don't know about those
    //    indexedcolor_4bits_to_24bits_pixels( const std::vector<colorRGB24> & palette, std::vector<colorRGB24>::iterator & inout_itimg );

    //    void operator()( uint8_t pixels );
    //};

    ///****************************************************************
    //    indexedcolor_8bits_to_24bits_pixels
    //        A functor for converting palette indexes into a list of
    //        RGB24 colors.
    //        Works on 8bits palette indexes. 
    //****************************************************************/
    //class indexedcolor_8bits_to_24bits_pixels : public indexed_to_nonindexed
    //{
    //public:
    //    //using indexed_to_nonindexed::indexed_to_nonindexed; //C++ 11 constructor inheritance, sadly, msvc don't know about those
    //    indexedcolor_8bits_to_24bits_pixels( const std::vector<colorRGB24> & palette, std::vector<colorRGB24>::iterator & inout_itimg );

    //    void operator()( uint8_t pixel );
    //};

};};

#endif