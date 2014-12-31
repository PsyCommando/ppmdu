#include "kao.hpp"
#include <ppmdu/utils/utility.hpp>
#include <ppmdu/ext_fmts/png_io.hpp>
#include <ppmdu/ext_fmts/bmp_io.hpp>
#include <ppmdu/ext_fmts/rawimg_io.hpp>
#include <ppmdu/ext_fmts/riff_palette.hpp>
#include <iostream>
#include <cassert>
#include <map>
#include <sstream>
#include <utility>
#include <iomanip>
#include <ppmdu/fmts/at4px.hpp>
#include <Poco/DirectoryIterator.h>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <ppmdu/utils/gbyteutils.hpp>
using namespace std;
using namespace gimg;

namespace pmd2 { namespace filetypes 
{

//========================================================================================================
//  Typedefs
//========================================================================================================
    //typedef to make things readable
    typedef map<uint32_t, types::bytevec_szty_t>::const_iterator cmapiter_t;

//========================================================================================================
//  Utility Functions
//========================================================================================================

    inline bool IsValidFile( const Poco::File & f ) //A little function for determining valid files
    { 
        return f.isFile() && !(f.isHidden()); 
    }

    inline bool IsValidDirectory( const Poco::File & f ) //A little function for determining valid files
    { 
        return f.isDirectory() && !(f.isHidden()); 
    }

//==================================================================
// Structs
//==================================================================

    //This is to avoid including the POCO header in the "kao.hpp" file..
    struct kao_file_wrapper
    {
        Poco::File myfile;
    };

//========================================================================================================
//  KaoParser
//========================================================================================================

//========================================================================================================
//  KaoWriter
//========================================================================================================

    void KaoWriter::Reset()
    {
        m_pExportFrom      = nullptr;
        m_pDestination     = nullptr;
        m_exportType       = eSUPPORT_IMG_IO::PNG;
        m_curOffTocSub     = 0;
        m_lastNullEntryVal = 0;
        m_imgBuff.resize(0);
        m_outBuff.resize(0);
    }

    vector<uint8_t> KaoWriter::operator()( const CKaomado & exportfrom )
    {
        Reset();
        m_pExportFrom = &exportfrom;
        return WriteToKaomado();
    }

    //This will export to a kaomado.kao file
    void KaoWriter::operator()( const CKaomado & exportfrom, const string & exportto )
    {
        Reset();
        m_pExportFrom  = &exportfrom;
        vector<uint8_t> result = WriteToKaomado();

        //Write to file
        WriteByteVectorToFile( exportto, result );
    }
        
    //This forces to extract to a folder structure with the specified image type
    void KaoWriter::operator()( const CKaomado & exportfrom, const string & exportto, eSUPPORT_IMG_IO exporttype )
    {
        Reset();
        m_pExportFrom  = &exportfrom;
        m_pDestination = &exportto;
        m_exportType   = exporttype;

        ExportToFolders();
    }

    void KaoWriter::ExportToFolders()
    {
        //#1 - Go through the ToC, and make a sub-folder for each ToC entry
        //     with its index as name.

        //Make the parent folder
        utils::DoCreateDirectory( *m_pDestination );
        
        if( !m_bQuiet )
            cout<<"Exporting entries to folder..\n";

        //Make aliases
        const auto & toc = m_pExportFrom->m_tableofcontent;

        for( tocsz_t i = 1; i < toc.size(); )
        {
            //Create the sub-folder name
            stringstream outfoldernamess;
            outfoldernamess <<utils::AppendTraillingSlashIfNotThere(*m_pDestination)
                             <<std::setfill('0') <<std::setw(4) <<std::dec <<i;

            if( m_pFolderNames != nullptr && m_pFolderNames->size() > i && !m_pFolderNames->at(i).empty() )
                outfoldernamess<< "_" << m_pFolderNames->at(i);

            ExportAToCEntry( toc[i]._portraitsentries, outfoldernamess.str() );

            //Increment counter here, for the completion indicator to work
            ++i;
            if( !m_bQuiet )
                cout<<"\r" << (i*100) / toc.size() <<"%";
        }
        if( !m_bQuiet )
            cout<<"\n";
    }

    void KaoWriter::ExportAToCEntry( const std::vector<tocsubentry_t> & entry, const string & directoryname )
    {
        bool bmadeafolder = false; //Whether we made a folder already for this entry.
                                   // We're doing it this way, because we don't want to create empty folders 
                                   // for entry containing only null pointers!

        for( tocsz_t j = 0; j < entry.size(); ++j )
        {
            if( CKaomado::isToCSubEntryValid( entry[j] ) ) //If not a dummy/null pointer
            {
                stringstream strsOutputPath;

                //Create a folder if we haven't yet
                if(!bmadeafolder)
                {
                    utils::DoCreateDirectory(directoryname);
                    bmadeafolder = true;
                }

                strsOutputPath <<utils::AppendTraillingSlashIfNotThere(directoryname) 
                               <<setfill('0') <<setw(4) <<static_cast<uint32_t>(j); 

                if( m_pSubEntryNames != nullptr && m_pSubEntryNames->size() > j )
                    strsOutputPath <<"_" << (*m_pSubEntryNames)[j];

                if( m_exportType == eSUPPORT_IMG_IO::RAW )
                {
                    //Don't append an extension! We need only the filename!
                    ExportTo4bppRawImgAndPal( m_pExportFrom->m_imgdata[entry[j]], strsOutputPath.str() );
                }
                else if( m_exportType == eSUPPORT_IMG_IO::BMP )
                {
                    strsOutputPath <<"." << BMP_FileExtension;
                    ExportTo4bppBMP( m_pExportFrom->m_imgdata[entry[j]], strsOutputPath.str() );
                }
                else //If all else fail, export to PNG !
                {
                    strsOutputPath <<"." << PNG_FileExtension;
                    ExportTo4bppPNG( m_pExportFrom->m_imgdata[entry[j]], strsOutputPath.str() );
                }
            }
        }
    }

    vector<uint8_t> KaoWriter::WriteToKaomado()
    {
        if( m_pExportFrom->m_imgdata.empty() || m_pExportFrom->m_tableofcontent.empty() )
        {
            cerr << "<!>-WARNING: KaoWriter::WriteKaomado() : Nothing to write in the output kaomado file!\n";
            assert(false);
            return vector<uint8_t>();
        }

        //#0 - Gather some stats and do some checks
        const unsigned int  SzToCEntry       = m_pExportFrom->m_nbtocsubentries       * SUBENTRY_SIZE;
        const unsigned int  Expected_ToC_Len = m_pExportFrom->m_tableofcontent.size() * SzToCEntry;
        auto                resultlenghts    = m_pExportFrom->EstimateKaoLenAndBiggestImage();
        const unsigned int  estimatedlength  = resultlenghts.first;
        unsigned int        szBiggestImg     = resultlenghts.second;
        unsigned int        curoffsetToc     = 0;
        unsigned int        cptcompletion    = 0;
        unsigned int        nbentries        = m_pExportFrom->m_tableofcontent.size();

        //Allocate memory
        m_imgBuff.reserve( szBiggestImg );
        m_outBuff.reserve( utils::GetNextInt32DivisibleBy16( estimatedlength ) ); //align on 16 bytes

        //Resize raw output buf to ToC lenght so we can begin inserting data afterwards
        m_outBuff.resize( Expected_ToC_Len, 0 ); 

        //#2 - Skip the ToC in the output, and begin outputing portraits, writing down their offset as we go.
        if( !m_bQuiet )
            cout << "Building kaomado file..\n";

        for( const auto& tocentry : m_pExportFrom->m_tableofcontent )
        {
            m_curOffTocSub = curoffsetToc; //Where we'll be writing our next sub-entry

            for( const auto & portrait : tocentry._portraitsentries )
                WriteAPortrait( portrait );

            curoffsetToc += SzToCEntry; //jump to next ToC entry

            ++cptcompletion;
            if( !m_bQuiet )
                cout<<"\r" << (cptcompletion * 100) / nbentries <<"%";
        }
        if( !m_bQuiet )
            cout<<"\n";

        //Align the end of the file on 16 bytes
        const unsigned int nbpaddingbytes = utils::GetNextInt32DivisibleBy16( m_outBuff.size() ) - m_outBuff.size();

        //Add padding bytes
        for( int i = 0; i < nbpaddingbytes; ++i )
            m_outBuff.push_back(COMMON_PADDING_BYTE);

        //Doing this to avoid a copy, and make sure that in case of re-use, the state of our internal vector is still valid!
        vector<uint8_t> temp;
        std::swap( m_outBuff, temp );

        //Done, move the vector
        return std::move( temp );
    }

    void KaoWriter::WriteAPortrait( const kao_toc_entry::subentry_t & portrait )
    {
        //First set both to the last valid end of data offset. "null" them out basically!
        tocsubentry_t portraitpointer = m_lastNullEntryVal; //The offset from the beginning where we'll insert any new data!

        //Make sure we output a computed file offset for valid entry, and just the entry's value if the pointer value is invalid
        if( CKaomado::isToCSubEntryValid( portrait ) )
        {
            //If we have data to write
            auto & currentimg = m_pExportFrom->m_imgdata[portrait];
            portraitpointer   = m_outBuff.size(); //Set the current size as the offset to insert our stuff!

            //#3.1 - Write palette
            graphics::WriteRawPalette_RGB24_As_RGB24( m_itOutBuffPushBack, currentimg.getPalette().begin(), currentimg.getPalette().end() );

            //Keep track of where the at4px begins
            auto offsetafterpal = m_outBuff.size();

            //#3.2 - Compress the image
            unsigned int curimgmaxsz      = (currentimg.getSizeInBits() / 8u) +
                                            ( (currentimg.getSizeInBits() % 8u != 0)? 1u : 0u); //Add one more just in case we ever overflow a byte (fat chance..)
            unsigned int curimgszandheadr = curimgmaxsz + at4px_header::HEADER_SZ;

            // Make a raw tiled image
            m_imgBuff.resize(0);
            WriteTiledImg( m_itImgBuffPushBack, currentimg, KAO_PORTRAIT_PIXEL_ORDER_REVERSED );

            //#3.3 - Expand the output to at least the raw image's length, then write at4px
            compression::px_info_header pxinf = CompressToAT4PX( m_imgBuff.begin(), 
                                                                 m_imgBuff.end(), 
                                                                 m_itOutBuffPushBack,
                                                                 compression::ePXCompLevel::LEVEL_3,
                                                                 m_bZealousStrSearch );

            //Update the last valid end of data offset (We fill any subsequent invalid entry with this value!)
            m_lastNullEntryVal = - (static_cast<tocsubentry_t>(m_outBuff.size())); //Change the sign to negative too
        }

        //Write the ToC entry in the space we reserved at the beginning of the output buffer!
        utils::WriteIntToByteVector( portraitpointer, m_outBuff.begin() + m_curOffTocSub );
        m_curOffTocSub += sizeof(portraitpointer);
    }

//========================================================================================================
//  CKaomado
//========================================================================================================
    //const array<CKaomado::fexthndlr_t, 3> CKaomado::SupportedInputImageTypes =
    //{{
    //    { pngio::PNG_FileExtension,        eSUPPORT_IMG_IO::EX_PNG },
    //    { bmpio::BMP_FileExtension,        eSUPPORT_IMG_IO::EX_BMP },
    //    { rawimg_io::RawImg_FileExtension, eSUPPORT_IMG_IO::EX_RAW },
    //}};


    CKaomado::CKaomado( unsigned int nbentries, unsigned int nbsubentries )
        :m_bZealousStrSearch(true),m_nbtocsubentries(nbsubentries), m_tableofcontent(nbentries)/*,
        m_imgdata( nbentries * nbsubentries) */ //This pre-alloc caused the whole thing to hang for several seconds when constructing
    {
        m_tableofcontent.resize(0); //reset the size to 0, without de-allocating
        m_imgdata.resize(0);
    }

    bool CKaomado::ReadEntireKaomado( types::constitbyte_t itinbegin, types::constitbyte_t itinend, bool bBeQuiet )
    {
        //#1 - Iterate the ToC
        //Get the first null entry out of the way first to avoid checking each turns..
        auto    itfoundnonnull = std::find_if_not( itinbegin, itinend, [](const uint8_t& val){ return val == 0; } );
        int32_t firstentrylen  = std::distance( itinbegin, itfoundnonnull );

        //Entry size check 
        if( ( firstentrylen / SUBENTRY_SIZE ) > m_nbtocsubentries )
        {
            assert(false); //IMPLEMENT VARIABLE SIZE ENTRIES IF EVEN POSSIBLE!
            throw std::length_error("Error: First null entry has unexpected length of " + std::to_string(firstentrylen) );
            return false;
        }
        //If the entire thing is null..
        if( itfoundnonnull == itinend )
        {
            assert(false);
            throw std::runtime_error("Error: Entire kaomado is null!");
            return false;
        }

        //When we got our first non-null entry, and divide the ptr by the size of an entry, to get the amount of entries
        // And avoid further resizing.
        //Get the first non-null entry and get the nb of entries in the table of content of the file being read
        tocsubentry_t   firsptr      = utils::ReadIntFromByteVector<tocsubentry_t>(itfoundnonnull);
        uint32_t        nbtocentries = firsptr / (m_nbtocsubentries * SUBENTRY_SIZE);
        auto            ittoc        = itinbegin;
        vector<uint8_t> imagebuffer;    //A temporary buffer that images uses to decompress to. 
                                        //Its here to avoid having to re-allocate a new one for each toc sub-entries!

        //Ensure capacity
        m_tableofcontent.resize(nbtocentries, kao_toc_entry(m_nbtocsubentries) );
        m_imgdata       .resize(nbtocentries * m_nbtocsubentries); //Reserve extra slots to avoid having to shrink/grow when modifying
                                                                   // and ensuring easier/faster rebuilding of the ToC and file, by
                                                                   // avoiding having to update all references in the ToC.

        if(!bBeQuiet)
        {
            cout<<"Parsing kaomado file..\n";
            for( std::size_t i = 0; i < m_tableofcontent.size(); )
            {
                ittoc = ReadAToCEntry( i, itinbegin, ittoc, imagebuffer, bBeQuiet );
                ++i;
                cout<<"\r" << ((i * 100) / m_tableofcontent.size())  <<"%";
            }
            cout << "\n";
        }
        else
        {
            for( std::size_t i = 0; i < m_tableofcontent.size(); ++i )
                ittoc = ReadAToCEntry( i, itinbegin, ittoc, imagebuffer, bBeQuiet );
        }

        return true;
    }

    //itdatabeg
    uint32_t CKaomado::CalculateEntryLen( types::constitbyte_t itdatabeg, tocsubentry_t entryoffset )
    {
        //Skip palette, and read at4px header
        at4px_header head;
        std::advance( itdatabeg, 
                      static_cast<decltype(KAO_PORTRAIT_PAL_LEN)>(entryoffset) + KAO_PORTRAIT_PAL_LEN ); 
        head.ReadFromContainer( itdatabeg );
        return head.compressedsz + KAO_PORTRAIT_PAL_LEN;
    }

    CKaomado::tocsubentry_t CKaomado::ReadAToCSubEntry( types::constitbyte_t & itbegindata )
    {
        return utils::ReadIntFromByteVector<tocsubentry_t>( itbegindata );
    }

    CKaomado::tocsubentry_t CKaomado::GetInvalidToCEntry()
    {
        return numeric_limits<tocsubentry_t>::min();
    }

    types::constitbyte_t CKaomado::ReadAToCEntry( vector<kao_toc_entry>::size_type  & indexentry,
                                                  types::constitbyte_t                itbegindata, 
                                                  types::constitbyte_t                itrawtocentry, 
                                                  std::vector<uint8_t>  &            imagebuffer,
                                                  bool                               bBeQuiet ) 
    {
        //Alias to make things a little more readable
        vector<tocsubentry_t> & currententry = m_tableofcontent[indexentry]._portraitsentries;

        //Go through all our ToC entry's sub-entries
        for( tocsz_t cptsubentry = 0; cptsubentry <  currententry.size(); ++cptsubentry )
        {
            tocsubentry_t tocreadentry; //The entry we just read in the last loop
            tocreadentry = ReadAToCSubEntry( itrawtocentry );
            
            //Avoid null and invalid entries
            if( isToCSubEntryValid(tocreadentry) )
            {
                uint32_t entrylen       = CalculateEntryLen( itbegindata, tocreadentry );
                tocsz_t  entryinsertpos = (indexentry * DEF_KAO_TOC_ENTRY_NB_PTR) + cptsubentry; //Position to insert stuff for this entry in the data vector
                data_t & tmpPortrait    = m_imgdata[entryinsertpos]; //a little reference to make things easier
                auto     itentryread    = itbegindata + tocreadentry;
                auto     itentryend     = itbegindata + (tocreadentry + entrylen);
                auto     itpalend       = itbegindata + (tocreadentry + KAO_PORTRAIT_PAL_NB_COL * KAO_PORTRAIT_PAL_BPC);

                //A. Read the palette
                graphics::ReadRawPalette_RGB24_As_RGB24( itentryread, itpalend, tmpPortrait.getPalette() );

                //B. Read the image
                filetypes::DecompressAT4PX( itpalend, itentryend, imagebuffer );

                //C. Parse the image. 
                // Image pixels seems to be in little endian, and need to be converted to big endian
                ParseTiledImg<data_t>( imagebuffer.begin(), 
                                       imagebuffer.end(), 
                                       graphics::RES_PORTRAIT, 
                                       tmpPortrait, 
                                       KAO_PORTRAIT_PIXEL_ORDER_REVERSED );

                //Refer to the new entry
                registerToCEntry( indexentry, cptsubentry, entryinsertpos );
            }
            else
                currententry[cptsubentry] = GetInvalidToCEntry(); //Set anything invalid to invalid!
        }
        return itrawtocentry;
    }

    pair<uint32_t,uint32_t> CKaomado::EstimateKaoLenAndBiggestImage()const
    {
        //Estimate kaomado worst case scenario length. (The size as if all images weren't compressed)
        unsigned int estimatedlength = m_tableofcontent.size() * (m_nbtocsubentries * SUBENTRY_SIZE);
        unsigned int szBiggestImg    = 0;

        //Use the ToC to count only valid entries!
        for( auto & entry : m_tableofcontent )
        {
            for( auto & subentry : entry._portraitsentries )
            {
                if( isToCSubEntryValid( subentry ) )
                {
                    unsigned int  currentimgsz = (m_imgdata[subentry].getSizeInBits() / 8u) + 
                                                 KAO_PORTRAIT_PAL_LEN + 
                                                 at4px_header::HEADER_SZ;

                    if( szBiggestImg < currentimgsz ) //Keep track of the largest image, so we can avoid resizing our compression buffer later on
                        szBiggestImg = (currentimgsz / 8u) + currentimgsz; //Reserve some extra space for command bytes just in case

                    estimatedlength += (currentimgsz / 8u) + currentimgsz; //Reserve some extra space for command bytes just in case
                }
            }
        }
        return make_pair(estimatedlength, szBiggestImg);
    }

    void CKaomado::WriteAPortrait( kao_toc_entry::subentry_t &                     portrait,
                                   std::vector<uint8_t> &                          outputbuffer, 
                                   std::back_insert_iterator<std::vector<uint8_t>> itoutputpushback,
                                   std::vector<uint8_t> &                          imgbuffer, 
                                   std::back_insert_iterator<std::vector<uint8_t>> itimgbufpushback,
                                   tocsubentry_t &                                 lastvalideendofdataoffset,
                                   uint32_t &                                      offsetWriteatTocSub )
    {
        //First set both to the last valid end of data offset. "null" them out basically!
        tocsubentry_t portraitpointer = lastvalideendofdataoffset; //The offset from the beginning where we'll insert any new data!

        //Make sure we output a computed file offset for valid entry, and just the entry's value if the pointer value is invalid
        if( isToCSubEntryValid( portrait ) )
        {
            //If we have data to write
            auto & currentimg = m_imgdata[portrait];
            portraitpointer   = outputbuffer.size(); //Set the current size as the offset to insert our stuff!

            //#3.1 - Write palette
            graphics::WriteRawPalette_RGB24_As_RGB24( itoutputpushback, currentimg.getPalette().begin(), currentimg.getPalette().end() );

            //Keep track of where the at4px begins
            auto offsetafterpal = outputbuffer.size();

            //#3.2 - Compress the image
            unsigned int curimgmaxsz      = (currentimg.getSizeInBits() / 8u) +
                                            ( (currentimg.getSizeInBits() % 8u != 0)? 1u : 0u); //Add one more just in case we ever overflow a byte (fat chance..)
            unsigned int curimgszandheadr = curimgmaxsz + at4px_header::HEADER_SZ;

            // Make a raw tiled image
            imgbuffer.resize(0);
            WriteTiledImg( itimgbufpushback, currentimg, KAO_PORTRAIT_PIXEL_ORDER_REVERSED );

            //#3.3 - Expand the output to at least the raw image's length, then write at4px
            compression::px_info_header pxinf = CompressToAT4PX( imgbuffer.begin(), 
                                                                 imgbuffer.end(), 
                                                                 itoutputpushback,
                                                                 compression::ePXCompLevel::LEVEL_3,
                                                                 m_bZealousStrSearch );

            //Update the last valid end of data offset (We fill any subsequent invalid entry with this value!)
            lastvalideendofdataoffset = - (static_cast<tocsubentry_t>(outputbuffer.size())); //Change the sign to negative too
        }

        //Write the ToC entry in the space we reserved at the beginning of the output buffer!
        utils::WriteIntToByteVector( portraitpointer, outputbuffer.begin() + offsetWriteatTocSub );
        offsetWriteatTocSub += sizeof(portraitpointer);
    }

    std::vector<uint8_t> CKaomado::WriteKaomado(bool bBeQuiet, bool bZealousStringSearch )
    {
        if( m_imgdata.empty() || m_tableofcontent.empty() )
        {
            cerr << "<!>-WARNING: CKaomado::WriteKaomado() : Nothing to write in the output kaomado file!\n";
            assert(false);
            return vector<uint8_t>();
        }
        m_bZealousStrSearch = bZealousStringSearch; //#TODO: maybe do this via accessor ?

        //#0 - Gather some stats and do some checks
        const unsigned int   Expected_ToC_Len   = m_tableofcontent.size() * (m_nbtocsubentries * SUBENTRY_SIZE);
        auto                 resultlenghts      = EstimateKaoLenAndBiggestImage();
        const unsigned int   estimatedlength    = resultlenghts.first;
        unsigned int         szBiggestImg       = resultlenghts.second;
        std::vector<uint8_t> imgbuffer( szBiggestImg, 0 );  //Temp image buff used for decompression. Shared to avoid extra alloc.
        std::vector<uint8_t> outputbuffer( utils::GetNextInt32DivisibleBy16( estimatedlength ), 0 ); //align on 16 bytes
        unsigned int         curoffsetToc       = 0;
        unsigned int         offsetTocEnd       = Expected_ToC_Len;
        unsigned int         cptcompletion      = 0;
        unsigned int         nbentries          = m_tableofcontent.size();
        tocsubentry_t        lastValidEoDOffset = 0; //Used for filling in null entries! All null entries refer to the last valid end of data offset!
        auto                 itimgbufpushback   = std::back_inserter(imgbuffer);
        auto                 itoutputpushback   = std::back_inserter(outputbuffer);

        //Resize raw output buf to ToC lenght so we can begin inserting data afterwards
        outputbuffer.resize(Expected_ToC_Len); 

        //#2 - Skip the ToC in the output, and begin outputing portraits, writing down their offset as we go.
        if( !bBeQuiet )
            cout << "Building kaomado file..\n";

        for( auto& tocentry : m_tableofcontent )
        {
            uint32_t offsetWriteatTocSub = curoffsetToc; //Where we'll be writing our next sub-entry

            for( auto & portrait : tocentry._portraitsentries )
            {
                WriteAPortrait( portrait, 
                                outputbuffer, 
                                itoutputpushback, 
                                imgbuffer, 
                                itimgbufpushback, 
                                //bZealousStringSearch, 
                                lastValidEoDOffset, 
                                offsetWriteatTocSub );
            }
            curoffsetToc += (m_nbtocsubentries * SUBENTRY_SIZE); //jump to next ToC entry

            ++cptcompletion;
            if( !bBeQuiet )
                cout<<"\r" << (cptcompletion * 100) / nbentries <<"%";
        }
        if( !bBeQuiet )
            cout<<"\n";

        //Align the end of the file on 16 bytes
        const unsigned int nbpaddingbytes = utils::GetNextInt32DivisibleBy16( outputbuffer.size() ) - outputbuffer.size();

        for( int i = 0; i < nbpaddingbytes; ++i )
            outputbuffer.push_back(COMMON_PADDING_BYTE);

        //Done, move the vector
        return std::move( outputbuffer );
    }

    //bool CKaomado::isSupportedImageType( const std::string & path )const
    //{
    //    Poco::Path imagepath( path );
    //    string     imgext = imagepath.getExtension();

    //    for( auto & afiletype : SupportedInputImageTypes )
    //    {
    //        if( imgext.compare( afiletype.extension ) == 0 )
    //            return true;
    //    }
    //    return false;
    //}

    //Returns index inserted at!
    vector<kao_toc_entry>::size_type CKaomado::InputAnImageToDataVector( kao_file_wrapper & imagetohandle )
    {
        data_t     palimg;
        Poco::Path imagepath( imagetohandle.myfile.path() );
        //string           imgext     = imagepath.getExtension();
        //eSUPPORT_IMG_IO detectedty = eSUPPORT_IMG_IO::INVALID;

        //for( auto & afiletype : SupportedInputImageTypes )
        //{
        //    if( imgext.compare( afiletype.extension ) == 0 )
        //    {
        //        detectedty = afiletype.type;
        //        break;
        //    }
        //}

        //Proceed to validate the file and find out what to use to handle it!
        switch( GetSupportedImageType( imagepath.getFileName() ) )
        {
            case eSUPPORT_IMG_IO::PNG:
            {
                ImportFrom4bppPNG( palimg, imagetohandle.myfile.path() );
                break;
            }
            case eSUPPORT_IMG_IO::BMP:
            {
                ImportFrom4bppBMP( palimg, imagetohandle.myfile.path() );
                break;
            }
            case eSUPPORT_IMG_IO::RAW:
            {
                stringstream pathtoraw;
                pathtoraw << imagepath.parent().toString() << imagepath.getBaseName();
                ImportFrom4bppRawImgAndPal( palimg, pathtoraw.str(), graphics::RES_PORTRAIT );
                break;
            }
            default:
            {
                stringstream strserror;
                strserror<< "<!>-Error: Image " <<imagepath.toString() <<" doesn't look like a BMP, RAW or PNG image !";
                throw std::runtime_error(strserror.str());
            }
        };

        m_imgdata.push_back( std::move(palimg) );
        return m_imgdata.size() - 1;
    }

    void CKaomado::ImportImage( kao_file_wrapper & imagefile, unsigned int tocindex, const string & foldername )
    {
        unsigned int datavecindex = 0;
        Poco::File & animage      = imagefile.myfile; //make an alias

        try
        {
            //Convert the image and store it in our data vector. And get the index in the data vector it is at.
            datavecindex = InputAnImageToDataVector(imagefile); 
        }
        catch(exception e)
        {
            cerr <<"\n" <<e.what() <<"\n";
            return; //We can't guaranty the image came through properly, so abort.
        }

        Poco::Path   imgpath(animage.path());
        stringstream sstrimgindex;
        unsigned int imgindex = 0;

        //Parse index from image name
        sstrimgindex << imgpath.getBaseName();
        sstrimgindex >> imgindex;

        //validate the index
        if( imgindex >= m_tableofcontent.front()._portraitsentries.size() )
        {
            stringstream strserror;
            strserror <<"The index of " <<imgpath.getFileName() <<", in folder name " <<foldername 
                        <<", exceeds the number of slots available, " <<(m_nbtocsubentries-1) 
                        <<" !\n"
                        <<"Please, number the images from 0 to " <<(m_nbtocsubentries-1) <<"!\n";
            assert(false);
            throw domain_error( strserror.str() );
        }

        //Add an entry in the toc refering to the image data we just put in our data vector
        registerToCEntry( tocindex, imgindex, datavecindex );
    }

    void CKaomado::ImportDirectory( kao_file_wrapper & foldertohandle )
    {
        Poco::DirectoryIterator itdir(foldertohandle.myfile),
                                itdirend;
        string                  foldername = Poco::Path(foldertohandle.myfile.path()).makeFile().getBaseName();
        vector<Poco::File>      validImages(DEF_KAO_TOC_ENTRY_NB_PTR);
        validImages.resize(0);


        //#1 - Validate index value from folder name
        //Use folder names as indexes in the kaomado table, use image file names as toc subentry indexes!
        stringstream strsfoldername;
        unsigned int ToCindex = 0;

        //Parse the ToC index from the foldername
        strsfoldername << foldername;
        strsfoldername >> ToCindex; 

        if( ToCindex >= m_tableofcontent.size() )
        {
            cerr <<"The index number in the name of folder " <<foldername <<", is higher than the number of slots available in the ToC!\n"
                 <<"It will be ignored !. Next time, please number your folders from 1 to " <<(m_tableofcontent.size()-1) <<"!\n";
            return;
        }

        //#2 - Find all our valid images
        for(; itdir != itdirend; ++itdir ) 
        {
            if( IsSupportedImageType( itdir->path() ) )
                validImages.push_back(*itdir);
        }

        //#3 - Import the images
        for( auto & animage : validImages )
            ImportImage( kao_file_wrapper{animage}, ToCindex, foldername );
    }

    void CKaomado::BuildFromFolder( std::string & folderpath, bool bBeQuiet )
    {
        //Index name starts at 1 given index 0 holds the dummy first entry
        Poco::DirectoryIterator itdir(folderpath),
                                itdirend;
        vector<Poco::File>      ValidDirectories( m_tableofcontent.capacity() );
        //Resize to zero for pushback and preserve alloc
        ValidDirectories.resize(0);

        //Resize the ToC to its set size ! And set all entries to null by default !
        m_tableofcontent.resize( m_tableofcontent.capacity(), kao_toc_entry(DEF_KAO_TOC_ENTRY_NB_PTR) );

        //#1 - Count nb folders
        for( ; itdir != itdirend; ++itdir)
        {
            if( IsValidDirectory(*itdir) ) 
                ValidDirectories.push_back( *itdir );
        }

        if( ValidDirectories.size() == 0 )
            cout <<"<!>-Warning: Folder to build Kaomado from contain no valid directories!\n";
        else if( !bBeQuiet )
            cout <<"Found " <<ValidDirectories.size() <<" valid directories!\n";

        m_imgdata.reserve( (ValidDirectories.size() * m_nbtocsubentries) + 1u );  //Reserve some space +1 for the dummy first slot !
        m_imgdata.resize(1u); //Skip the first entry, so the first data entry in the toc is not null ! 

        if(!bBeQuiet)
            cout<<"Reading Sub-Directories..\n";

        //#2 - Handle all folders
        unsigned int cptdirs = 0;
        for( Poco::DirectoryIterator ithandledir(folderpath); ithandledir != itdirend; ++ithandledir )
        {
            //Need to wrap it so we keep the header clean.. It will probably get optimized out by the compiler anyways
            ImportDirectory( kao_file_wrapper{ (*ithandledir) } );

            ++cptdirs;
            if(!bBeQuiet)
                cout<<"\r" <<(cptdirs*100) / ValidDirectories.size() <<"%";
        }
        if( !bBeQuiet )
            cout<<"\n";
    }


    void CKaomado::ExportToFolders( std::string          & folderpath, 
                                    const vector<string> * pfoldernames, 
                                    const vector<string> * psubentrynames,
                                    eSUPPORT_IMG_IO        exporttype,
                                    bool                   bBeQuiet )
    {
        //#1 - Go through the ToC, and make a sub-folder for each ToC entry
        //     with its index as name.

        //Make the parent folder
        utils::DoCreateDirectory( folderpath );
        
        if( !bBeQuiet )
            cout<<"Exporting entries to folder..\n";

        for( tocsz_t i = 1; i < m_tableofcontent.size(); )
        {
            //Create the sub-folder
            stringstream outfoldernamess;
            outfoldernamess <<utils::AppendTraillingSlashIfNotThere(folderpath)
                             <<std::setfill('0') <<std::setw(4) <<std::dec <<i;

            if( pfoldernames != nullptr && pfoldernames->size() > i && !pfoldernames->at(i).empty() )
                outfoldernamess<< "_" << pfoldernames->at(i);

            ExportAToCEntry( m_tableofcontent[i]._portraitsentries, outfoldernamess.str(), psubentrynames, exporttype );

            //Increment counter here, for the completion indicator to work
            ++i;
            if( !bBeQuiet )
                cout<<"\r" << (i*100) / m_tableofcontent.size() <<"%";
        }
        if( !bBeQuiet )
            cout<<"\n";
    }

    void CKaomado::ExportAToCEntry( const vector<tocsubentry_t> & entry, 
                                    const string                & outputfoldername, 
                                    const vector<string>        * psubentrynames,
                                    eSUPPORT_IMG_IO              exporttype )
    {
        bool bmadeafolder = false; //Whether we made a folder already for this entry.
                                   // We're doing it this way, because we don't want to create empty folders 
                                   // for entry containing only null pointers!

        for( tocsz_t j = 0; j < entry.size(); ++j )
        {
            if( isToCSubEntryValid( entry[j] ) ) //If not a dummy/null pointer
            {
                stringstream strsOutputPath;

                //Create a folder if we haven't yet
                if(!bmadeafolder)
                {
                    utils::DoCreateDirectory(outputfoldername);
                    bmadeafolder = true;
                }

                strsOutputPath <<utils::AppendTraillingSlashIfNotThere(outputfoldername) 
                               <<setfill('0') <<setw(4) <<static_cast<uint32_t>(j); 

                if( psubentrynames != nullptr && psubentrynames->size() > j )
                    strsOutputPath <<"_" << (*psubentrynames)[j];

                if( exporttype == eSUPPORT_IMG_IO::RAW )
                {
                    //Don't append an extension! We need only the filename!
                    ExportTo4bppRawImgAndPal( m_imgdata[entry[j]], strsOutputPath.str() );
                }
                else if( exporttype == eSUPPORT_IMG_IO::BMP )
                {
                    strsOutputPath <<"." << BMP_FileExtension;
                    ExportTo4bppBMP( m_imgdata[entry[j]], strsOutputPath.str() );
                }
                else //If all else fail, export to PNG !
                {
                    strsOutputPath <<"." << PNG_FileExtension;
                    ExportTo4bppPNG( m_imgdata[entry[j]], strsOutputPath.str() );
                }
            }
        }
    }

};};