#ifndef PNG_IO_HPP
#define PNG_IO_HPP
/*
png_io.hpp
2014/11/13
psycommando@gmail.com
Description: Utilities for importing and exporting PNG images and their palettes into the formats used in the lib.
*/
#include <ppmdu/containers/tiled_image.hpp>
#include <ext_fmts/supported_io_info.hpp>
#include <string>


namespace utils{ namespace io
{
    static const std::string PNG_FileExtension = "png";

    struct PNG_ImgInfo
    {
        unsigned int width;
        unsigned int height;
        unsigned int bitdepth;
        unsigned int nbcolorspal;
    };

//==============================================================================================
//  Import/Export from/to 4bpp
//==============================================================================================
    //bool ImportFrom4bppPNG( gimg::tiled_image_i4bpp & out_indexed,
    //                        const std::string       & filepath, 
    //                        unsigned int              forcedwidth     = 0,
    //                        unsigned int              forcedheight    = 0,
    //                        bool                      erroronwrongres = false );


    //bool ExportTo4bppPNG( const gimg::tiled_image_i4bpp & in_indexed,
    //                      const std::string             & filepath );

//==============================================================================================
//  Import/Export from/to 8bpp
//==============================================================================================
    //bool ImportFrom8bppPNG( gimg::tiled_image_i8bpp & out_indexed,
    //                        const std::string       & filepath, 
    //                        unsigned int              forcedwidth     = 0,
    //                        unsigned int              forcedheight    = 0,
    //                        bool                      erroronwrongres = false );


    //bool ExportTo8bppPNG( const gimg::tiled_image_i8bpp & in_indexed,
    //                      const std::string             & filepath );

//==============================================================================================
//  Import/Export from/to ANY !
//==============================================================================================
    //Generic Export Functions
    // Calls the correct function depending on the type of the tiled image!
    template<class _TImg_t>
        bool ExportToPNG( const _TImg_t     & in_indexed,
                          const std::string & filepath );

    /*
    */
    template<class _TImgTy>
        bool ExportToPNG_AndCrop(   const _TImgTy     & in_indexed,
                                    const std::string & filepath,
                                    unsigned int        begpixX,
                                    unsigned int        begpixY,
                                    unsigned int        endpixX = 0,
                                    unsigned int        endpixY = 0 );


    //Generic Import Functions
    // Calls the correct function depending on the type of the tiled image!

    template<class _TImg_t>
        bool ImportFromPNG( _TImg_t           & out_indexed,
                            const std::string & filepath, 
                            unsigned int        forcedwidth     = 0,
                            unsigned int        forcedheight    = 0,
                            bool                erroronwrongres = false );


    std::vector<gimg::colorRGB24> ImportPaletteFromPNG( const std::string & filepath );
    void                          SetPalettePNGImg( const std::vector<gimg::colorRGB24> & srcpal, 
                                                    const std::string & filepath);

    image_format_info GetPNGImgInfo(const std::string & filepath);


    bool ExportToPNG( std::vector<gimg::colorRGBX32>    & bitmap,
                      const std::string                 & filepath, 
                      unsigned int                      forcedwidth,
                      unsigned int                      forcedheight,
                      bool                              erroronwrongres = false );


    bool ExportToPNG( const std::vector<std::vector<uint8_t>>   & indexed8bpp,
                      const std::vector<gimg::colorRGB24>       & palette,
                      const std::string                         & filepath, 
                      bool                                        erroronwrongres = false );

};};

#endif