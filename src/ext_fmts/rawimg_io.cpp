#include "rawimg_io.hpp"
#include "riff_palette.hpp"
#include <sstream>
#include <iostream>
#include <Poco/File.h>
#include <Poco/Path.h>
using namespace std;

namespace utils{ namespace io
{

//==============================================================================================
//  ExportToRawImg
//==============================================================================================
    template<class _TImg_t>
        bool _ExportToRawImg( const _TImg_t & in_indexed, const std::string & filepath )
    {
        vector<uint8_t> outputimage;
        Poco::Path      rawimgpath( filepath );
        auto            itbins = back_inserter( outputimage );
        
        rawimgpath.setExtension( RawImg_FileExtension );
        try
        {
            gimg::WriteTiledImg( itbins, in_indexed );
            WriteByteVectorToFile( rawimgpath.toString(), outputimage );
        }
        catch( exception e )
        {
            cerr <<"\n<!>- Error while exporting raw image : " <<e.what() <<"\n";
            return false;
        }
        return true;
    }

//==============================================================================================
//  ExportToRawImgAndPal
//==============================================================================================
    template<class _TImg_t>
        bool _ExportToRawImgAndPal( const _TImg_t & in_indexed, const std::string & filepath )
    {
        Poco::Path palpath   = filepath;
        palpath.setExtension( RIFF_PAL_Filext );

        if( !_ExportToRawImg( in_indexed, filepath ) )
            return false;

        try
        {
            ExportTo_RIFF_Palette( in_indexed.getPalette(), palpath.toString() );
        }
        catch( exception e )
        {
            cerr <<"\n<!>- Error while exporting palette : " <<e.what() <<"\n";
            return false;
        }

        return true;
    }

//==============================================================================================
//  ImportFromRawImg
//==============================================================================================
    template<class _TImg_t>
        bool ImportFromRawImg( _TImg_t           & out_indexed, 
                               const std::string & filepath, 
                               utils::Resolution   imgres )
    {
        Poco::Path rawimgpath( filepath );
        rawimgpath.setExtension( RawImg_FileExtension );
        Poco::File rawimgfile(rawimgpath);
        if( !( rawimgfile.exists() && rawimgfile.isFile() ) )
        {
            cerr << "<!>-Error: raw image \"" <<rawimgpath.toString() <<"\" does not exist!\n";
            return false;
        }

        try
        {
            vector<uint8_t> imgdat = ReadFileToByteVector( rawimgpath.toString() );
            gimg::ParseTiledImg<_TImg_t>( imgdat.begin(), imgdat.end(), imgres, out_indexed );
        }
        catch( exception e )
        {
            cerr <<"\n<!>- Error while importing raw image : " <<e.what() <<"\n";
            return false;
        }

        return true;
    }


//==============================================================================================
//  ImportFromRawImgAndPal
//==============================================================================================
    template<class _TImg_t>
        bool ImportFromRawImgAndPal( _TImg_t           & out_indexed, 
                                     const std::string & filepath, 
                                     utils::Resolution   imgres )
    {
        //find both needed files first
        Poco::Path rawimgpath( filepath );
        Poco::Path riffpath  ( filepath );
        riffpath.setExtension( RIFF_PAL_Filext );
        Poco::File rifffile(riffpath);

        if( !( rifffile.exists() && rifffile.isFile() ) )
        {
            cerr << "<!>-Error: raw image \"" <<rawimgpath.toString() 
                    <<"\" does not have a matching palette file \"" <<riffpath.toString() <<"\" !\n";
            return false;
        }

        if( ImportFromRawImg( out_indexed, filepath, imgres ) )
        {
            try
            {
                (out_indexed.getPalette()) = ImportFrom_RIFF_Palette( riffpath.toString() );
            }
            catch( exception e )
            {
                cerr <<"\n<!>- Error while importing raw image : " <<e.what() <<"\n";
                return false;
            }
        }

        return true;
    }

//==============================================================================================
//  ExportRawImg
//==============================================================================================
    template<>
        bool ExportRawImg( const gimg::tiled_image_i4bpp & in_indexed,
                           const std::string             & filepath )
    {
        return _ExportToRawImgAndPal(in_indexed, filepath);
    }

    template<>
        bool ExportRawImg( const gimg::tiled_image_i8bpp & in_indexed,
                           const std::string             & filepath )
    {
        return _ExportToRawImgAndPal(in_indexed, filepath);
    }

//==============================================================================================
//  ExportRawImg_NoPal
//==============================================================================================
    template<>
        bool ExportRawImg_NoPal( const gimg::tiled_image_i4bpp & in_indexed,
                                 const std::string             & filepath )
    {
        return _ExportToRawImg( in_indexed, filepath );
    }

    template<>
        bool ExportRawImg_NoPal( const gimg::tiled_image_i8bpp & in_indexed,
                                 const std::string             & filepath )
    {
        return _ExportToRawImg( in_indexed, filepath );
    }

//==============================================================================================
//  ImportRawImg
//==============================================================================================
    template<>
        bool ImportRawImg( gimg::tiled_image_i4bpp & out_indexed,
                            const std::string      & filepath,
                            utils::Resolution        imgres)
    {
        return ImportFromRawImgAndPal( out_indexed, filepath, imgres );
    }

    template<>
        bool ImportRawImg( gimg::tiled_image_i8bpp & out_indexed,
                            const std::string      & filepath,
                            utils::Resolution        imgres)
    {
        return ImportFromRawImgAndPal( out_indexed, filepath, imgres );
    }

//==============================================================================================
//  ImportRawImg_NoPal
//==============================================================================================
    template<>
        bool ImportRawImg_NoPal( gimg::tiled_image_i4bpp & out_indexed, 
                                 const std::string       & filepath, 
                                 utils::Resolution         imgres)
    {
        return ImportFromRawImg( out_indexed, filepath, imgres );
    }

    template<>
        bool ImportRawImg_NoPal( gimg::tiled_image_i8bpp & out_indexed, 
                                 const std::string       & filepath, 
                                 utils::Resolution         imgres)
    {
        return ImportFromRawImg( out_indexed, filepath, imgres );
    }

//==============================================================================================
//  Import/Export 4bpp 
//==============================================================================================
    //Functions for exporting a raw headerless img, along with an accompanying riff palette
    //bool ImportFrom4bppRawImgAndPal( gimg::tiled_image_i4bpp & out_indexed, const std::string & filepath, utils::Resolution imgres )
    //{
    //    return ImportFromRawImgAndPal( out_indexed, filepath, imgres );
    //}

    //bool ExportTo4bppRawImgAndPal( const gimg::tiled_image_i4bpp & in_indexed, const std::string & filepath )
    //{
    //    return ExportToRawImgAndPal(in_indexed, filepath);
    //}

//==============================================================================================
//  Import/Export 8bpp 
//==============================================================================================

    //bool ImportFrom8bppRawImgAndPal( gimg::tiled_image_i8bpp       & out_indexed, 
    //                                 const std::string             & filepath, 
    //                                 utils::Resolution               imgres )
    //{
    //    return ImportFromRawImgAndPal( out_indexed, filepath, imgres );
    //}

    //bool ExportTo8bppRawImgAndPal  ( const gimg::tiled_image_i8bpp & in_indexed, 
    //                                 const std::string             & filepath )
    //{
    //    return ExportToRawImgAndPal(in_indexed, filepath);
    //}

//==============================================================================================
//  PRI Format Stuff 
//==============================================================================================
    //Functions for the custom PRI raw image container
    bool ImportFrom4bppPRI( gimg::tiled_image_i4bpp & out_indexed, const std::string & filepath )
    {
        assert(false);
        //#TODO: Implement !
        return false;
    }

    bool ExportTo4bppPRI( const gimg::tiled_image_i4bpp & in_indexed, const std::string & filepath )
    {
        assert(false);
        //#TODO: Implement !
        return false;
    }
};};