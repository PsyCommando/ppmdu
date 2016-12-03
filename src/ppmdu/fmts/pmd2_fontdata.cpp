#include "pmd2_fontdata.hpp"
#include <utils/utility.hpp>
#include <ppmdu/containers/linear_image.hpp>
#include <sstream>
#include <vector>
#include <cstdint>
using namespace std;
using namespace utils;


//! #TODO: FIX THIS SHIT
#if 0
namespace pmd2 { namespace filetypes 
{
    static const uint32_t          SizeMainFontDataEntry = 28; //bytes
    static const uint32_t          SizeMainFontImage     = 24; //bytes
    static const utils::Resolution MainFontRes           = { 16, 12 };


//
//  Struct
//

    /*
        The default state of this struct is the same as null entries have !
    */
    struct RawFontToCEntry
    {
        int8_t  width  = 0;
        int8_t  height = 0;
        int16_t offset = -1;

        static const uint32_t SZ = sizeof(decltype(width)) + sizeof(decltype(height)) + sizeof(decltype(offset));

        template<class _init>
            _init ReadFromContainer( _init itreadat, _init itpastend )
        {
            width  = utils::ReadIntFromBytes<int8_t> (itreadat, itpastend);
            height = utils::ReadIntFromBytes<int8_t> (itreadat, itpastend);
            offset = utils::ReadIntFromBytes<int16_t>(itreadat, itpastend);
            return itreadat;
        }

        template<class _outit>
            _outit WriteToContainer( _outit itwriteat )const
        {
            utils::WriteIntToBytes( width,  itwriteat );
            utils::WriteIntToBytes( height, itwriteat );
            utils::WriteIntToBytes( offset, itwriteat );
            return itwriteat;
        }

        //static inline uint32_t size() { return SZ; }
    };

//======================================================================================
//  FontData
//======================================================================================
// For font data organized in a font data file.

//---------------------------
//  FontDataCharEntry
//---------------------------
    //struct FontDataCharEntry
    //{
    //    uint16_t     charc;
    //    uint16_t     unk1;
    //    vector<bool> imgdata;
    //};

//---------------------------
//  FontDataReader
//---------------------------

    template<class _PIXEL_T>
        class FontDataReader
    {
    public:
        typedef _PIXEL_T               pixel_t;
        typedef CharacterData<pixel_t> chardat_t;


        FontDataReader( const vector<uint8_t> & fontfiledata )
            :m_fontfiledata(fontfiledata)
        {}

        operator vector<CharacterData<pixel_t>>()
        {
            //Ensure the state of the vector is ok, in case someone re-uses the object...
            //m_fontdata = vector<CharacterData<gimg::trait_indexed_1bpp>>();
            Parse();
            return std::move( m_fontdata );
        }

    private:

        void Parse()
        {
            //vector<uint8_t> fdata  = utils::io::ReadFileToByteVector( m_fontfile );
            auto            itRead = m_fontfiledata.begin();
            auto            itEnd  = m_fontfiledata.end();

            //Get nb of character entries
            uint32_t nbentries = utils::ReadIntFromBytes<uint32_t>( itRead, itEnd );

            //Validate size
            uint32_t expected  = (m_fontfiledata.size() - 4) / SizeMainFontDataEntry;
            uint32_t remainder = (m_fontfiledata.size() - 4) %  SizeMainFontDataEntry;

            uint32_t hdrnbent  = nbentries * SizeMainFontDataEntry + 4; //The +4 is for the uint32 at the beginning that contains the nb of entries 

            if( remainder != 0 && expected != hdrnbent )
            {
                stringstream sstr;
                sstr << "Font data file has unxepected number of entries. Expected " << expected <<" entriy(es), and got " <<hdrnbent <<" !";
                throw runtime_error( sstr.str() );
            }

            //Allocate
            m_fontdata.resize( nbentries );

            //Parse entries
            for( unsigned int i = 0; itRead != itEnd && i < nbentries; ++i )
            {
                chardat_t & acharent = m_fontdata[i];

                acharent.charcode = utils::ReadIntFromBytes<uint16_t>( itRead, itEnd );
                acharent.unk1     = utils::ReadIntFromBytes<uint16_t>( itRead, itEnd );

                //Read image data here
                acharent.imgdat.resize( MainFontRes.width, MainFontRes.height );
                auto itOutImg = gimg::PxlReadIter<typename chardat_t::img_t>( acharent.imgdat ); 
                copy_n( itRead, SizeMainFontImage, itOutImg );
            }
        }

    private:
        const vector<uint8_t>                           & m_fontfiledata;
        vector<CharacterData<gimg::pixel_indexed_1bpp>>   m_fontdata;
    };

//---------------------------
//  FontDataWriter
//---------------------------
    template<class _PIXEL_T>
        class FontDataWriter
    {
    public:

        typedef CharacterData<_PIXEL_T> chardat_t;

        FontDataWriter( const vector<CharacterData<_PIXEL_T>> & fontdata )
            :m_fontdata(fontdata)
        {}

        operator std::vector<uint8_t>()
        {
            vector<uint8_t> outdat( 4 + (m_fontdata.size() * SizeMainFontDataEntry) );
            auto            itout    = outdat.begin();
            auto            itoutend = outdat.end();

            itout = WriteIntToBytes( static_cast<uint32_t>(m_fontdata.size()), itout );

            auto writeAnEntry = [&itout,&outdat]( const chardat_t & entry )
            {
                itout = WriteIntToBytes( entry.charcode, itout );
                itout = WriteIntToBytes( entry.unk1,     itout );
                gimg::PxlWriteIter<_PIXEL_T, vector<uint8_t>> itwimg(outdat);
                copy( entry.imgdat.begin(), entry.imgdat.end(), itwimg );

                //for( const auto & pix : entry.imgdat )
                //{
                //    (*itwimg) = pix;
                //}
            };

            for_each( m_fontdata.begin(), m_fontdata.end(), writeAnEntry );

            return std::move(outdat);
        }

    private:

        const vector<CharacterData<_PIXEL_T>> & m_fontdata;
    };

//======================================================================================
//  RawFontData
//======================================================================================
// For font data where each characters is a 8bpp image one after the other in a file.

    /************************************************************************************************
        RawFontDataReader
    ************************************************************************************************/
    class RawFontDataReader
    {
    public:
        RawFontDataReader( const std::vector<uint8_t> & fdata )
            :m_fdata(fdata)
        {}

        operator std::vector<gimg::image_i8bpp>()
        {
            RawFontToCEntry                first;
            first.ReadFromContainer(m_fdata.begin(), m_fdata.end());

            auto                           itToC    = m_fdata.begin();
            auto                           itToCEnd = m_fdata.begin() + first.offset;
            std::vector<gimg::image_i8bpp> imgdat;
            imgdat.reserve( first.offset / RawFontToCEntry::SZ );

            while( itToC != itToCEnd )
            {
                //Read ToC entry
                RawFontToCEntry cur;
                itToC = cur.ReadFromContainer( itToC, itToCEnd );

                gimg::image_i8bpp curimg;

                //Skip null entries
                if( cur.offset != -1 )
                {
                    curimg.resize( cur.width, cur.height );
                    gimg::PxlReadIter<gimg::image_i8bpp> itparse( curimg );

                    for( uint32_t offp = static_cast<uint32_t>(cur.offset); offp < curimg.size(); ++offp )
                        (*itparse) = m_fdata[offp];
                }

                imgdat.push_back( std::move(curimg) );
            }

            return std::move(imgdat);
        }

    private:
        const vector<uint8_t> & m_fdata;
    };

    /************************************************************************************************
        RawFontDataWriter
    ************************************************************************************************/
    class RawFontDataWriter
    {
    public:
        RawFontDataWriter( const std::vector<gimg::image_i8bpp> & fontdata )
            :m_fontdata(fontdata)
        {}

        operator std::vector<uint8_t>()
        {
            const int16_t        ToCLen = m_fontdata.size() * RawFontToCEntry::SZ;
            std::vector<uint8_t> fdat( ToCLen, 0 ); //Reserve ToC
            int16_t              curToCIndex = 0;

            for( const auto & entry : m_fontdata )
            {
                if( !m_fontdata.empty() )
                {
                    RawFontToCEntry tocentry;
                    tocentry.width  = entry.width();
                    tocentry.height = entry.height();
                    tocentry.offset = fdat.size();

                    //Write ToC entry
                    tocentry.WriteToContainer( (fdat.data() + (curToCIndex * RawFontToCEntry::SZ)) );

                    //Write img data
                    gimg::PxlWriteIter<typename gimg::image_i8bpp::pixel_t, std::vector<uint8_t>> itconvert(fdat); 
                    copy( entry.begin(), entry.end(), itconvert );
                }
                else
                {
                    RawFontToCEntry().WriteToContainer( (fdat.data() + (curToCIndex * RawFontToCEntry::SZ)) );
                }
                ++curToCIndex;
            }

            return std::move(fdat);
        }

    private:
        const std::vector<gimg::image_i8bpp> & m_fontdata;
    };

//======================================================================================
//  Functions
//======================================================================================
    /*
        Main font data:
            Main 1bpp font used in dialogs and menus in the PMD2 Explorers of Sky game.
    */
    vector<CharacterData<gimg::pixel_indexed_1bpp>> ParseMainFontData( const string & fontfile )
    {
        return ParseMainFontData( utils::io::ReadFileToByteVector( fontfile ) );
    }

    std::vector<CharacterData<gimg::pixel_indexed_1bpp>> ParseMainFontData( const std::vector<uint8_t> & fontfiledata )
    {
        return FontDataReader<gimg::pixel_indexed_1bpp>(fontfiledata); //Implicit conversion operator called :D
    }
    
    vector<uint8_t> WriteMainFontData( const vector<CharacterData<gimg::pixel_indexed_1bpp>> & imgsdata )
    {
        return FontDataWriter<gimg::pixel_indexed_1bpp>(imgsdata); //Implicit conversion operator called :D
    }

    void WriteMainFontData( const vector<CharacterData<gimg::pixel_indexed_1bpp>> & imgsdata, 
                            const string                                          & destfontfile )
    {
        utils::io::WriteByteVectorToFile( destfontfile, WriteMainFontData(imgsdata) );
    }

    /*
        Main font data Import/Export:
            These function export the font data as 1 bpp images to a directory and import them back.

            The output format is simply a bunch of png images in a directory, and in their name is their character code
            in hexadecimal, followed with an underscore, followed with the unk1 value in hexadecimal. 
    */
    //void ExportMainFontData( const mfontdat_t  & data, 
    //                         const std::string & destdir )
    //{
    //}

    //mfontdat_t ImportMainFontData( const std::string & srcdir )
    //{
    //}


    /*
    */
    std::vector<gimg::image_i8bpp> ParseRawFontFile( const std::string & fontfile )
    {
        return ParseRawFontFile( utils::io::ReadFileToByteVector(fontfile) );
    }
    
    std::vector<gimg::image_i8bpp> ParseRawFontFile( const std::vector<uint8_t> & fontfiledata )
    {
        return RawFontDataReader(fontfiledata); //Implicit conversion operator called :D
    }

    std::vector<uint8_t> WriteRawFontFile( const std::vector<gimg::image_i8bpp> & fontdata )
    {
        return RawFontDataWriter(fontdata); //Implicit conversion operator called :D
    }

    void WriteRawFontFile( const std::vector<gimg::image_i8bpp> & fontdata, const std::string & destfontfile )
    {
        utils::io::WriteByteVectorToFile( destfontfile, WriteRawFontFile(fontdata) );
    }

    /*
        Font Data Import/Export :
            These functions export the font images as 8 bpp images into the specified directory.

            The output format is simply a directory containing one image for every character entry.
            Each images is numbered to match the character's entry index. Null entries are not exported.
            
            Each images has a copy of the palette, but only the first image's palette is loaded for all other characters.
    */
    //void ExportFontData( const std::vector<gimg::image_i8bpp> & data, 
    //                     const std::string                    & destdir )
    //{
    //}

    //std::vector<gimg::image_i8bpp> ImportFontData( const std::string & srcdir )
    //{
    //}

};};
#endif