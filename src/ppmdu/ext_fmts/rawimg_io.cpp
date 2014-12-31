#include "rawimg_io.hpp"
#include "riff_palette.hpp"
#include <sstream>
#include <iostream>
#include <Poco/File.h>
#include <Poco/Path.h>
using namespace std;

namespace utils{ namespace io
{
    //Functions for exporting a raw headerless img, along with an accompanying riff palette
    bool ImportFrom4bppRawImgAndPal( gimg::tiled_image_i4bpp & out_indexed, const std::string & filepath, utils::Resolution imgres )
    {
        //find both needed files first
        Poco::Path               riffpath  ( filepath );
        Poco::Path               rawimgpath( filepath );

        riffpath.setExtension( RIFF_PAL_Filext );
        rawimgpath.setExtension( RawImg_FileExtension );

        Poco::File rifffile(riffpath);
        Poco::File rawimgfile(rawimgpath);

        if( !( rawimgfile.exists() && rawimgfile.isFile() ) )
        {
            cerr << "<!>-Error: raw image \"" <<rawimgpath.toString() <<"\" does not exist!\n";
            return false;
        }
        if( !( rifffile.exists() && rifffile.isFile() ) )
        {
            cerr << "<!>-Error: raw image \"" <<rawimgpath.toString() 
                    <<"\" does not have a matching palette file \"" <<riffpath.toString() <<"\" !\n";
            return false;
        }

        try
        {
            vector<uint8_t> imgdat = ReadFileToByteVector( rawimgpath.toString() );
            gimg::ParseTiledImg<gimg::tiled_image_i4bpp>( imgdat.begin(), imgdat.end(), imgres, out_indexed );
            (out_indexed.getPalette()) = ImportFrom_RIFF_Palette( riffpath.toString() );
        }
        catch( exception e )
        {
            //stringstream strs;
            cerr <<"\n<!>- Error while importing raw image : " <<e.what() <<"\n";
            return false;
        }

        return true;
    }

    bool ExportTo4bppRawImgAndPal( const gimg::tiled_image_i4bpp & in_indexed, const std::string & filepath )
    {
        Poco::Path rawimgpath( filepath );
        Poco::Path palpath   ( filepath );
        vector<uint8_t> outputimage;
        auto            itbins = back_inserter( outputimage );

        rawimgpath.setExtension( RawImg_FileExtension );
        palpath.setExtension( RIFF_PAL_Filext );

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