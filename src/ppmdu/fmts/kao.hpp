#ifndef KAO_HPP
#define KAO_HPP
/*
kao.hpp
2014/09/14
psycommando@gmail.com
Description: File for handling editing, extracting, and rebuilding .kao files from the PMD2 games.
There is only one known .kao file named "kaomado.kao".

It should be noted that .kao files are headerless.

A big thanks to Zhorken for reversing most of the format!
https://github.com/Zhorken
*/
#include <ppmdu/pmd2/pmd2_palettes.hpp>
#include <ppmdu/pmd2/pmd2_filetypes.hpp>
#include <ppmdu/pmd2/pmd2_image_formats.hpp>
#include <utils/utility.hpp>
#include <ppmdu/containers/tiled_image.hpp>
#include <ext_fmts/supported_io.hpp>
#include <vector>
#include <string>
#include <utility>

using namespace utils::io;

namespace filetypes
{
//==================================================================
// Constants
//==================================================================
    static const size_t         DEF_KAO_TOC_ENTRY_SZ     = 160u;        //160 (0xA0) bytes
    static const size_t         DEF_KAO_TOC_ENTRY_NB_PTR = DEF_KAO_TOC_ENTRY_SZ / 4u; //the amount of pointer/entry pairs within a single entry 
    static const size_t         DEF_KAO_TOC_NB_ENTRIES   = 1155u;       //1155 (0x483) entries

    static const uint32_t       KAO_PORTRAIT_PAL_BPC     = 3; //Bytes per color
    static const uint32_t       KAO_PORTRAIT_PAL_NB_COL  = 16;
    static const uint32_t       KAO_PORTRAIT_PAL_LEN     = KAO_PORTRAIT_PAL_BPC * KAO_PORTRAIT_PAL_NB_COL;
    static const unsigned int   KAO_PORTRAIT_IMG_RAW_SIZE= 800u;
    static const bool           KAO_PORTRAIT_PIXEL_ORDER_REVERSED = true; //This tells the I/O methods to invert pixel endianess

    extern const ContentTy CnTy_Kaomado; //Contetn ID db handle

//==================================================================
// Structs
//==================================================================
    
    /*
        Forward declare to cover up the Poco::File type from our libraries!
    */
    struct kao_file_wrapper;

    /*
        kao_toc_entry
            An entry from a kaomado file's table of content. 
            The table of content in a kaomado file is important for
            more than just finding file offsets. It lists all the 
            face portraits for each pokemon.
    */
    struct kao_toc_entry
    {
        typedef int32_t subentry_t;
        static const unsigned int SUBENTRY_SIZE = sizeof(subentry_t);

        kao_toc_entry( std::vector<subentry_t>::size_type size = DEF_KAO_TOC_ENTRY_NB_PTR )
            :_portraitsentries(size, 0)
        {}

        kao_toc_entry( std::vector<subentry_t>::size_type size, int32_t defaultval )
            :_portraitsentries(size, subentry_t(defaultval))
        {}

        //sub-entries refs are stored in there
        std::vector<subentry_t> _portraitsentries; //Indexes to the m_data vector, in the parent CKaomado object for each pointers in the ToC.
                                                    // NOTE: if the m_data vector(in the CKaomado object) is empty, then this contains the 
                                                    //       actual offset from the file that was just read !
    };

//==================================================================
// Functors
//==================================================================
    class CKaomado;

    /********************************************************************************  
        KaoParser
            A functor for loading/importing a kaomado file into a CKaomado object
                * If path is folder it tries to import the folder structure.
                * If path is a kaomado.kao file it imports the file.
    ********************************************************************************/
    class KaoParser
    {
        typedef kao_toc_entry::subentry_t                         tocsubentry_t;
        typedef std::vector<kao_toc_entry::subentry_t>::size_type tocsz_t; 
        static const unsigned int SUBENTRY_SIZE = kao_toc_entry::SUBENTRY_SIZE;
    public:
        KaoParser( bool bequiet = false, bool bverbose = false )
            :m_pImportTo(nullptr), m_bQuiet(bequiet), m_bVerbose(bverbose)
        {}

        void operator()( const std::string & importfrom, CKaomado & importto );

    private:

        void                 ParseKaomado();
        std::vector<uint8_t>::const_iterator ParseToCEntry( std::vector<kao_toc_entry>::size_type  & indexentry, 
                                                            std::vector<uint8_t>::const_iterator     itrawtocentry );
        uint32_t             GetLenRawPortraitData( std::vector<uint8_t>::const_iterator itdatabeg, 
                                                    std::vector<uint8_t>::const_iterator itdataend, 
                                                    tocsubentry_t                        entryoffset );

        void ImportFromFolders();
        void ImportDirectory( kao_file_wrapper & foldertohandle );

        //tocindex   = the toc entry that will contain this subentry
        //foldername = the name of the parent directory containing the imagefile we're importing.
        void ImportImage( kao_file_wrapper & imagefile, unsigned int tocindex, const std::string & foldername );
        
        //Parse, convert, and insert an image file into the data vector, and return the index it got inserted at!
        std::vector<kao_toc_entry>::size_type AddImageToDataVector( kao_file_wrapper & imagefile );

    private:
        CKaomado * m_pImportTo;
        bool       m_bQuiet;
        bool       m_bVerbose;

        //Temporary variables - Parse Kaomado
        std::vector<uint8_t>                 m_kaomadoBuff;     //Buffer containing the kaomado file's raw data.
        std::vector<uint8_t>                 m_imgBuffer;       //Temp buffer for decompressing images
        std::vector<uint8_t>::const_iterator m_itInBeg;

        //Temporary variables - Parse Folders
        const std::string                   *m_pInputPath;
    };


    /******************************************************************************** 
        KaoWriter
            A functor for writing/exporting portraits data into a kaomado.kao file!
    ********************************************************************************/
    class KaoWriter
    {
        typedef kao_toc_entry::subentry_t                         tocsubentry_t;
        typedef std::vector<kao_toc_entry::subentry_t>::size_type tocsz_t; 
        static const unsigned int SUBENTRY_SIZE = kao_toc_entry::SUBENTRY_SIZE;
    public:

        //Constructor sets the options
        KaoWriter( const std::vector<std::string> *  pfoldernames     = nullptr, 
                   const std::vector<std::string> *  psubentrynames   = nullptr,
                   bool                              zealousstrsearch = true, 
                   bool                              bequiet          = false,
                   bool                              bverbose         = false )
            :m_bZealousStrSearch(zealousstrsearch), 
             m_pExportFrom(nullptr), 
             m_bQuiet(bequiet),
             m_pFolderNames(pfoldernames), 
             m_pSubEntryNames(psubentrynames),
             m_itImgBuffPushBack(std::back_inserter(m_imgBuff)),
             m_itOutBuffPushBack(std::back_inserter(m_outBuff)),
             m_bVerbose(bverbose)
        {}

        //This will export a CKaomado to a "kaomado.kao" file, but will return the buffer directly
        // instead of writing it directly.
        std::vector<uint8_t> operator()( const CKaomado & exportfrom );

        //This will export to a kaomado.kao file.
        void operator()( const CKaomado & exportfrom, const std::string & exportto );
        
        //This forces to extract to a folder structure with the specified image type.
        void operator()( const CKaomado & exportfrom, const std::string & exportto, eSUPPORT_IMG_IO exporttype );

    private:
        void Reset();

        void ExportToFolders();
        void ExportAToCEntry( const std::vector<tocsubentry_t> & entry, const std::string & directoryname );

        std::vector<uint8_t> WriteToKaomado();
        void                 WriteAPortrait( const kao_toc_entry::subentry_t & portrait );

    private:
        //
        const std::string              *m_pDestination;         //The path where the data will be written
        const CKaomado                 *m_pExportFrom;          //The kaomado container to use as source
        const std::vector<std::string> *m_pFolderNames;         //The string list for the names to give to the exported folders
        const std::vector<std::string> *m_pSubEntryNames;       //The string list for the names to give to the exported subentry images
        bool                            m_bZealousStrSearch;    //Whether compression will use zealous string search
        bool                            m_bQuiet;               //Whether we should print at the console
        bool                            m_bVerbose;             //Whether to print more verbose output
        
        //Temporary variables - kaomado.kao output
        std::vector<uint8_t>                            m_outBuff;             //Kaomado output buffer
        std::back_insert_iterator<std::vector<uint8_t>> m_itOutBuffPushBack;   //back_inserter on m_outBuff
        std::vector<uint8_t>                            m_imgBuff;             //Used to compress image
        std::back_insert_iterator<std::vector<uint8_t>> m_itImgBuffPushBack;   //back_inserter on m_imgBuff
        tocsubentry_t                                   m_lastNullEntryVal;    //This is the null value to use currently, when writing the kaomado
        uint32_t                                        m_curOffTocSub;        //This is the offset to write at in the output buffer the next pointer in the ToC

        //Temporary variables - folder output
        eSUPPORT_IMG_IO m_exportType;
    };


//==================================================================
// Classes
//==================================================================
    /******************************************************************
        CKaomado
            Container for portrait data found in the kaomado.kao file.

    ******************************************************************/
    class CKaomado
    {
        friend class KaoWriter;
        friend class KaoParser;
    public:
        //Typedef:
        typedef kao_toc_entry::subentry_t                                             tocsubentry_t;
        typedef std::vector<kao_toc_entry::subentry_t>::size_type                     tocsz_t;
        typedef gimg::tiled_indexed_image<gimg::pixel_indexed_4bpp, gimg::colorRGB24> data_t;

        static const unsigned int SUBENTRY_SIZE = kao_toc_entry::SUBENTRY_SIZE;

        //When building from scratch
        // Need to specify the initial desired sizes and quantites, to pre-allocate memory. 
        // The internal vectors are still size == 0 after this!
        CKaomado( unsigned int nbentries = DEF_KAO_TOC_NB_ENTRIES, unsigned int nbsubentries = DEF_KAO_TOC_ENTRY_NB_PTR );

    private:
        //Write an entry in the toc at the index specified, in the subentry specified. The value written is the data index in the m_imgdata vector
        inline void registerToCEntry( std::size_t tocindex, std::size_t subentryindex, std::size_t dataindex )
        { m_tableofcontent[tocindex]._portraitsentries[subentryindex] = tocsubentry_t( dataindex); }

        static inline bool isToCSubEntryValid( const tocsubentry_t & entry ) { return (entry > 0); }

        //Value1 is toclen, value2 is biggest image len!
        std::pair<uint32_t,uint32_t> EstimateKaoLenAndBiggestImage()const;

        //Return the offset where it was added in the data vector
        std::vector<kao_toc_entry>::size_type AddImageToDataVector( data_t && img ); //moves the image
        std::vector<kao_toc_entry>::size_type AddImageToDataVector( const data_t & img ); //Copy the image

        static tocsubentry_t GetInvalidToCEntry();

        // --- Variables ---
        uint32_t                    m_nbtocsubentries;
        std::vector<kao_toc_entry>  m_tableofcontent;
        std::vector<data_t>         m_imgdata;
    };


};

#endif