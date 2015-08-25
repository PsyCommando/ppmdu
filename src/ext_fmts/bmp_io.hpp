#ifndef BMP_IO_HPP
#define BMP_IO_HPP
/*
bmp_io.hpp
2014/12/24
psycommando@gmail.com
Description: Interface for loading bmp files into common containers, encapsulating the library used to read it.
*/
#include <ppmdu/containers/tiled_image.hpp>
#include <ext_fmts/supported_io_info.hpp>
#include <string>

namespace utils{ namespace io
{
    static const std::string BMP_FileExtension = "bmp";

//==============================================================================================
//  Import/Export from/to 4bpp
//==============================================================================================
    //#TODO: Make the return type a "gimg::tiled_image_i4bpp" instead.
    //bool ImportFrom4bppBMP( gimg::tiled_image_i4bpp & out_indexed,
    //                        const std::string       & filepath,
    //                        unsigned int              forcedwidth     = 0,
    //                        unsigned int              forcedheight    = 0,
    //                        bool                      erroronwrongres = false );


    //bool ExportTo4bppBMP( const gimg::tiled_image_i4bpp & in_indexed,
    //                      const std::string             & filepath );

//==============================================================================================
//  Import/Export from/to 8bpp
//==============================================================================================
    //#TODO: Make the return type a "gimg::tiled_image_i4bpp" instead.
    //bool ImportFrom8bppBMP( gimg::tiled_image_i8bpp & out_indexed,
    //                        const std::string       & filepath,
    //                        unsigned int              forcedwidth     = 0,
    //                        unsigned int              forcedheight    = 0,
    //                        bool                      erroronwrongres = false );


    //bool ExportTo8bppBMP( const gimg::tiled_image_i8bpp & in_indexed,
    //                      const std::string             & filepath );

//==============================================================================================
//  Import/Export from/to ANY !
//==============================================================================================
    //Generic Export Functions
    // Calls the correct function depending on the type of the tiled image!
    template<class _TImg_t>
        bool ExportToBMP( const _TImg_t     & in_indexed,
                          const std::string & filepath );



    //Generic Import Functions
    // Calls the correct function depending on the type of the tiled image!

    template<class _TImg_t>
        bool ImportFromBMP( _TImg_t           & out_indexed,
                            const std::string & filepath, 
                            unsigned int        forcedwidth     = 0,
                            unsigned int        forcedheight    = 0,
                            bool                erroronwrongres = false );



    std::vector<gimg::colorRGB24> ImportPaletteFromBMP( const std::string & filepath );
    void                          SetPaletteBMPImg( const std::vector<gimg::colorRGB24> & srcpal, 
                                                    const std::string & filepath);

    image_format_info GetBMPImgInfo( const std::string & filepath );


};};

#endif