/*
*/
#include "riff_palette.hpp"
#include <array>
#include <algorithm>
#include <ppmdu/containers/color.hpp>
#include <ppmdu/utils/gbyteutils.hpp>
#include <ppmdu/utils/utility.hpp>

using namespace std;
using namespace utils;

/*
http://worms2d.info/Palette_file
As like every other RIFF file, the PAL files start off with the RIFF header:

    4-byte RIFF signature "RIFF"
    4-byte file length in bytes (excluding the RIFF header)
    4-byte PAL signature "PAL " (note the space / 0x20 at the end) 

The PAL files then include 4 different chunks, of which only the "data" chunk is important. The other chunks "offl", "tran" and "unde" are all 32 bytes long, filled with 0x00. Their purpose remains unknown since they can be deleted and the game still accepts the PAL files. Like other RIFF chunks, the data chunk starts with its signature and length:

    4-byte data chunk signature "data"
    4-byte data chunk size excluding the chunk header
    2-byte PAL version. This version is always 0x0300.
    2-byte color entry count. This determines how many colors are following. 

Each color consists of 4 bytes holding the following data:

    1-byte red amount of color
    1-byte green amount of color
    1-byte blue amount of color
    1-byte "flags" - W:A PAL files always have the 0x00 flag, so no flag is set. 
*/
namespace riffpal_io
{
    using gimg::colorRGB24;

    // --------------- RIFF Specific constants --------------- 
    static const int  RIFF_MAGIC_NUMBER_LEN   = 4;
    static const int  RIFF_PAL_SIG_LEN        = 4;
    static const int  RIFF_DATA_CHUNK_SIG_LEN = 4;

    //Total Length of the RIFF header + the data chunk header. Not including the 2 bytes for the 
    // version or the 2 bytes for the nb of colors
    static const int  RIFF_PAL_HEADER_AND_DATA_CHUNK_HEADER_TOTAL_LEN = RIFF_MAGIC_NUMBER_LEN + 
                                                                        4 + 
                                                                        RIFF_PAL_SIG_LEN + 
                                                                        RIFF_DATA_CHUNK_SIG_LEN + 
                                                                        4;

    static const array<uint8_t,RIFF_MAGIC_NUMBER_LEN>   RIFF_MAGIC_NUMBER   = { 0x52, 0x49, 0x46, 0x46 }; // "RIFF"
    static const array<uint8_t,RIFF_PAL_SIG_LEN>        RIFF_PAL_SIG        = { 0x50, 0x41, 0x4C, 0x20 }; // "PAL "
    static const array<uint8_t,RIFF_DATA_CHUNK_SIG_LEN> RIFF_DATA_CHUNK_SIG = { 0x64, 0x61, 0x74, 0x61 }; // "data"
    static const uint16_t      RIFF_PAL_VERSION                             = 0x300;

//==================================================================
// RIFF Palette Tools
//==================================================================
    //Exports a palette to a Microsoft RIFF ".pal" file
    void ExportTo_RIFF_Palette( const vector<colorRGB24> & in_palette, vector<uint8_t> & out_riffpalette )
    {
        const uint16_t nbColorEntries   = in_palette.size();  //nb of colors in the palette
        const uint32_t dataChunkSize    = (nbColorEntries * 4) + 4;          //Length of the actual data / list of colors. Add 4 because the version number and nb of colors are counted as part of it, and are 2bytes each
        const uint32_t fileLengthTotal  = RIFF_PAL_HEADER_AND_DATA_CHUNK_HEADER_TOTAL_LEN + dataChunkSize; //Length of the riff file starting after the file length field
        const uint32_t fileLengthHeader = fileLengthTotal - 8; //remove the 8 bytes that arent't counted when calculating the file size

        //#1 - Resize the output vector
        out_riffpalette.reserve( fileLengthTotal );
        auto itbackins = std::back_inserter( out_riffpalette );

        //#2 - Write the header
        {
            //uint8_t fileLengthHeaderBytes[sizeof(int32_t)];
            //uint8_t dataChunkSizeBytes[sizeof(int32_t)];
            //uint8_t nbColorEntriesBytes[sizeof(int16_t)];
            //uint8_t riffPalVersionBytes[sizeof(int16_t)];

            //Convert everything to byte arrays (screw casting, this even gets everything in the right byte order and all)
            //UnsignedIntToByteBuff( fileLengthHeader, fileLengthHeaderBytes );
            //UnsignedIntToByteBuff( dataChunkSize, dataChunkSizeBytes );
            //Int16ToByteBuff( nbColorEntries, nbColorEntriesBytes );
            //Int16ToByteBuff( RIFF_PAL_VERSION, riffPalVersionBytes );

            //Write "RIFF"
            //out_riffpalette.insert( out_riffpalette.end(), RIFF_MAGIC_NUMBER.begin(), RIFF_MAGIC_NUMBER.end() );
            std::copy_n( RIFF_MAGIC_NUMBER.begin(), RIFF_MAGIC_NUMBER.size(), itbackins );

            //Write 4 bytes File Length
            //out_riffpalette.insert( out_riffpalette.end(), fileLengthHeaderBytes, (fileLengthHeaderBytes + SZ_INT32) );
            WriteIntToByteVector( fileLengthHeader, itbackins );

            //Write "PAL "
            //out_riffpalette.insert( out_riffpalette.end(), RIFF_PAL_SIG.begin(), RIFF_PAL_SIG.end() );
            std::copy_n( RIFF_PAL_SIG.begin(), RIFF_PAL_SIG.size(), itbackins );

            //Write "data"
            //out_riffpalette.insert( out_riffpalette.end(), RIFF_DATA_CHUNK_SIG.begin(), RIFF_DATA_CHUNK_SIG.end() );
            std::copy_n( RIFF_DATA_CHUNK_SIG.begin(), RIFF_DATA_CHUNK_SIG.size(), itbackins );

            //Write 4 bytes data chunk size
            //out_riffpalette.insert( out_riffpalette.end(), dataChunkSizeBytes, (dataChunkSizeBytes + SZ_INT32) );
            WriteIntToByteVector( dataChunkSize, itbackins );

            //Write 2 bytes riff pal version number
            //out_riffpalette.insert( out_riffpalette.end(), riffPalVersionBytes, (riffPalVersionBytes + SZ_INT16) );
            WriteIntToByteVector( RIFF_PAL_VERSION, itbackins );

            //Write 2 bytes nb of colors
            //out_riffpalette.insert( out_riffpalette.end(), nbColorEntriesBytes, (nbColorEntriesBytes + SZ_INT16) );
            WriteIntToByteVector( nbColorEntries, itbackins );
        }

        //#3 - Write the colors into the output vector
        for( auto & col : in_palette )
        {
            //Write the colors ordered as RGB, Ignore the X from the palette as its ignored, and its always 0x80 anyways
            out_riffpalette.push_back( col.red );
            out_riffpalette.push_back( col.green );
            out_riffpalette.push_back( col.blue );
            out_riffpalette.push_back( 0 );
        }
        
        //#4 - Profits!
    }

    void ExportTo_RIFF_Palette( const std::vector<gimg::colorRGB24> & in_palette, const std::string & outputpath )
    {
        vector<uint8_t> output;
        ExportTo_RIFF_Palette( in_palette, output );
        WriteByteVectorToFile( outputpath, output );
    }

    //Import a Microsoft RIFF ".pal" file into a RGB24 color palette
    std::vector<gimg::colorRGB24> ImportFrom_RIFF_Palette( const std::vector<uint8_t> & in_riffpalette )
    {
        uint16_t nbcolors = 0;

        //#1 - Validate palette
        std::vector<gimg::colorRGB24>   out_palette;
        vector<uint8_t>::const_iterator foundRiff = find_first_of( in_riffpalette.begin(), in_riffpalette.end(), RIFF_MAGIC_NUMBER.begin(), RIFF_MAGIC_NUMBER.end() );
        vector<uint8_t>::const_iterator foundPal;
        vector<uint8_t>::const_iterator foundDataChunk;

        if( foundRiff != in_riffpalette.end() )
        {
            foundPal = find_first_of( in_riffpalette.begin(), in_riffpalette.end(), RIFF_PAL_SIG.begin(), RIFF_PAL_SIG.end() );
            if( foundPal != in_riffpalette.end() )
            {
                foundDataChunk = std::find_first_of( in_riffpalette.begin(), in_riffpalette.end(), RIFF_DATA_CHUNK_SIG.begin(), RIFF_DATA_CHUNK_SIG.end() );
                if( foundDataChunk != in_riffpalette.end() )
                {
                    //#2 - move 10 bytes forward and read the nb of colors
                    foundDataChunk += 10;
                    nbcolors = ReadIntFromByteVector<uint16_t>( foundDataChunk );
                }
                else
                    throw exception("Invalid RIFF palette!");
            }
            else
                throw exception("Invalid RIFF palette!");
        }
        else
            throw exception("Invalid RIFF palette!");

        //#3 - Populate the palette
        //vector<uint8_t>::const_iterator itcolor = foundDataChunk + 2; //move the iterator to the first color
        const unsigned int StopAt = (in_riffpalette.size()-3);
        for( unsigned int i = 0x18; i < StopAt; )
        {
            colorRGB24 tmpcolor;
            //Everything in here is in RGB order, and there is the useless flag byte to skip at the end as well

            //Red
            tmpcolor.red = in_riffpalette[i];

            //Green
            ++i;
            tmpcolor.green = in_riffpalette[i];

            //Blue
            ++i;
            tmpcolor.blue = in_riffpalette[i];

            //Push the color into the color palette
            out_palette.push_back(tmpcolor);

            //Skip on flag byte
            i += 2;
        }

        //for(; out_palette.size() < nbcolors && itcolor != in_riffpalette.end(); /*++itcolor*/ )
        //{
        //    colorRGB24 tmpcolor;
        //    //Everything in here is in RGB order, and there is the useless flag byte to skip at the end as well

        //    //Red
        //    tmpcolor.red = *itcolor;

        //    //Green
        //    ++itcolor;
        //    tmpcolor.green = *itcolor;

        //    //Blue
        //    ++itcolor;
        //    tmpcolor.blue = *itcolor;

        //    //Push the color into the color palette
        //    out_palette.push_back(tmpcolor);

        //    //Skip on flag byte
        //    std::advance( itcolor, 2 );
        //}

        return std::move( out_palette );
    }

    std::vector<gimg::colorRGB24> ImportFrom_RIFF_Palette( const std::string & inputpath )
    {
        vector<uint8_t>          riffpaldata = ReadFileToByteVector( inputpath );
        vector<gimg::colorRGB24> output      = ImportFrom_RIFF_Palette( riffpaldata );
        return std::move(output);
    }

};