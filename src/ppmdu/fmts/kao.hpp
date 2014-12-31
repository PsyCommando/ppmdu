#ifndef KAO_HPP
#define KAO_HPP
/*
kao.hpp
2014/09/14
psycommando@gmail.com
Description: File for handling editing, extracting, and rebuilding .kao files from the PMD2 games.
There is only one known .kao file named "kaomado.kao".

It should be noted that .kao files are headerless.

#TODO: Gotta make sure that we can properly handle ToC entries
       having a non-default size, once we're sure the game can handle
       bigger ToC entries.

A big thanks to Zhorken for reversing most of the format!
https://github.com/Zhorken
*/
#include <ppmdu/basetypes.hpp>
#include <ppmdu/pmd2/pmd2_palettes.hpp>
#include <ppmdu/pmd2/pmd2_image_formats.hpp>
#include <ppmdu/utils/utility.hpp>
#include <ppmdu/containers/tiled_image.hpp>
#include <ppmdu/ext_fmts/supported_io.hpp>
#include <vector>
//#include <memory>
//#include <thread>
#include <string>
//#include <map>
#include <utility>

using namespace utils::io;

namespace pmd2{ namespace filetypes
{
//==================================================================
// Constants
//==================================================================
    static const types::bytevec_szty_t DEF_KAO_TOC_ENTRY_SZ     = 160u;        //160 (0xA0) bytes
    static const types::bytevec_szty_t DEF_KAO_TOC_ENTRY_NB_PTR = DEF_KAO_TOC_ENTRY_SZ / 4u; //the amount of pointer/entry pairs within a single entry 
    static const types::bytevec_szty_t DEF_KAO_TOC_NB_ENTRIES   = 1155u;       //1155 (0x483) entries

    static const uint32_t              KAO_PORTRAIT_PAL_BPC     = 3; //Bytes per color
    static const uint32_t              KAO_PORTRAIT_PAL_NB_COL  = 16;
    static const uint32_t              KAO_PORTRAIT_PAL_LEN     = KAO_PORTRAIT_PAL_BPC * KAO_PORTRAIT_PAL_NB_COL;
    static const unsigned int          KAO_PORTRAIT_IMG_RAW_SIZE= 800u;
    static const bool                  KAO_PORTRAIT_PIXEL_ORDER_REVERSED = true; //This tells the I/O methods to invert pixel endianess

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
            If path is folder it tries to import the folder structure.
            If path is a kaomado.kao file it imports the file.
    ********************************************************************************/
    class KaoParser
    {
    public:
        KaoParser()
            :m_pImportTo(nullptr)
        {}

        CKaomado & operator()( const std::string & importfrom, CKaomado & importto );

    private:
        CKaomado * m_pImportTo;
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
                   bool                              bequiet          = false )
            :m_bZealousStrSearch(zealousstrsearch), m_pExportFrom(nullptr), m_bQuiet(bequiet),
             m_pFolderNames(pfoldernames), m_pSubEntryNames(psubentrynames),
             m_itImgBuffPushBack(std::back_inserter(m_imgBuff)),
             m_itOutBuffPushBack(std::back_inserter(m_outBuff))
        {}

        //This will export to a kaomado.kao file, but will return the buffer directly
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
    /*
        CKaomado
            This object can handle the loading/writing of content 
            from a kaomado file, along with various other operations.

            This object is not a throwaway object, and can be 
            re-used, or used for storage.

    */
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

        //enum struct eSUPPORT_IMG_IO
        //{
        //    EX_INVALID,
        //    EX_PNG,
        //    EX_RAW,
        //    EX_BMP,
        //};


        //When building from scratch
        // Need to specify the initial desired sizes and quantites, to pre-allocate memory. 
        // The internal vectors are still size == 0 after this!
        CKaomado( unsigned int nbentries = DEF_KAO_TOC_NB_ENTRIES, unsigned int nbsubentries = DEF_KAO_TOC_ENTRY_NB_PTR );
        
        //--------
        
        //Takes an iterator to the beginning of the entire kaomado file data, and one to the end of 
        // the entire kaomado file data. Return true if it works, and false on failure.
        bool ReadEntireKaomado( types::constitbyte_t itinbegin, types::constitbyte_t itinend, bool bBeQuiet = false );

        //The object will write its current content to the returned vector (Move constructor make this efficient)
        // DO NOT USE if the m_data vector is empty. You HAVE to put some data in before. Either manually, or
        // through "Read_Data" !
        std::vector<uint8_t> WriteKaomado( bool bBeQuiet = false, bool bZealousStringSearchEnabled = true ); //Move constructor avoid the copy for returning by value!

        //Import content from filestructure
        /*
            This is one of the most practical aspect of this class. Rebuilding a kaomado from
            a directory + file structure.
            Just give the path to the root folder containing all the entries.

            The directory structure must be the same as what the "ExportToFolders" method exports. 

            Everything must be in uncompressed form(png, or bmp images, or even just raw palettes and raw images)
        */
        void BuildFromFolder( std::string & folderpath, bool bBeQuiet = false );

        //Export content to filestructure
        /*
            This is one of the most practical aspect of this class. Exporting a kaomado to
            a directory + file structure.

            Just give the path to the root folder containing all the entries.

            The optional "kaoentrynames" vector is a list of names to give to the folders in addition to their
            numerical names.

            The resulting folder structure can be easily re-built into a kaomado using the "BuildFromFolder"
            method.

            Everything must be in uncompressed form(png, or bmp images, or even just raw palettes and raw images)
        */
        void ExportToFolders( std::string                    & folderpath, 
                              const std::vector<std::string> *  pfoldernames   = nullptr, 
                              const std::vector<std::string> *  psubentrynames = nullptr,
                              eSUPPORT_IMG_IO                  exporttype     = eSUPPORT_IMG_IO::PNG, 
                              bool                              bBeQuiet       = false );

    private:
        //Write an entry in the toc at the index specified, in the subentry specified. The value written is the data index in the m_imgdata vector
        inline void registerToCEntry( std::size_t tocindex, std::size_t subentryindex, std::size_t dataindex )
        {
            m_tableofcontent[tocindex]._portraitsentries[subentryindex] = tocsubentry_t( dataindex);
        }

        static inline bool isToCSubEntryValid( const tocsubentry_t & entry )
        {
            return (entry > 0);
        }

        //Write all the valid portraits out for a single ToC entry
        void ExportAToCEntry( const std::vector<tocsubentry_t> & entry, 
                              const std::string                & outputfoldername, 
                              const std::vector<std::string>   * psubentrynames,
                              eSUPPORT_IMG_IO                   exporttype = eSUPPORT_IMG_IO::PNG );

        //Params are entire file begining iterator, iterator of the entry, and a map containing a list of all the unique data
        // offsets and where they were stored in the data vector.
        //The buffer passed is to avoid re-allocating each turns !
        types::constitbyte_t CKaomado::ReadAToCEntry( std::vector<kao_toc_entry>::size_type  & indexentry,
                                                      types::constitbyte_t                     itbegindata, 
                                                      types::constitbyte_t                     itrawtocentry, 
                                                      std::vector<uint8_t>  &                  imagebuffer,
                                                      bool                                     bBeQuiet = false);

        tocsubentry_t        ReadAToCSubEntry( types::constitbyte_t & itbegindata );


        //Value1 is toclen, value2 is biggest image len!
        std::pair<uint32_t,uint32_t> EstimateKaoLenAndBiggestImage()const;
        uint32_t                     CalculateEntryLen( types::constitbyte_t itdatabeg, tocsubentry_t entryoffset );

        void ImportDirectory( kao_file_wrapper & foldertohandle );

        //tocindex   = the toc entry that will contain this subentry
        //foldername = the name of the parent directory containing the imagefile we're importing.
        void ImportImage( kao_file_wrapper & imagefile, unsigned int tocindex, const std::string & foldername );

        static tocsubentry_t GetInvalidToCEntry();

        //Parse, convert, and insert an image file into the data vector, and return the index it got inserted at!
        std::vector<kao_toc_entry>::size_type InputAnImageToDataVector( kao_file_wrapper & foldertohandle );

        void WriteAPortrait( kao_toc_entry::subentry_t &                     portrait,
                             std::vector<uint8_t> &                          outputbuffer, 
                             std::back_insert_iterator<std::vector<uint8_t>> itoutputpushback,
                             std::vector<uint8_t> &                          imgbuffer, 
                             std::back_insert_iterator<std::vector<uint8_t>> itimgbufpushback,
                             tocsubentry_t &                                 lastValidEndOffset,
                             uint32_t &                                      offsetWriteatTocSub );

        //Return whether the file located at the specified path is a supported 
        // image type for import.
        //bool isSupportedImageType( const std::string & path )const;

        // --- Variables ---
        bool                        m_bZealousStrSearch; //This is set using the methods for rebuilding the kaomado
        uint32_t                    m_nbtocsubentries;
        std::vector<kao_toc_entry>  m_tableofcontent;
        std::vector<data_t>         m_imgdata;


    };


};};

#endif