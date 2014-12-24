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
#include <vector>
#include <memory>
#include <thread>
#include <string>
#include <map>
#include <utility>


//#TODO: Reconsider the design choice of handling everything as 8bpp.. 
//       Its forcing us to do a lot of needless conversion, even if ironically, it was meant to help out with that!
//       The kaomado file should be 4bpp only.

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
        Forward declare to cover up the file type from our libraries!
    */
    struct kao_file_wrapper;

    /*
        Entry for a single portrait in the kaomado ToC
    */
    //struct kao_portrait_toc_entry : public utils::data_array_struct
    //{
    //    static const uint32_t MY_SIZE = 8u; //bytes
    //    int32_t _begoffset, 
    //            _endoffset; 

    //    kao_portrait_toc_entry( int32_t begoffset = 0, int32_t endoffset = 0 ) :_begoffset(begoffset),_endoffset(endoffset) {}

    //    bool isValid()const;

    //    //Makes the entry invalid by setting both pointers to the same, negative value
    //    void setInvalid();

    //    unsigned int    size()const {return MY_SIZE;}
    //    std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const;
    //    std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom );
    //};

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
    public:
        //Typedef:
        typedef kao_toc_entry::subentry_t                                             tocsubentry_t;
        typedef std::vector<kao_toc_entry::subentry_t>::size_type                     tocsz_t; 
        typedef gimg::tiled_indexed_image<gimg::pixel_indexed_4bpp, gimg::colorRGB24> data_t;

        static const unsigned int SUBENTRY_SIZE = kao_toc_entry::SUBENTRY_SIZE;

        enum struct eEXPORT_t
        {
            EX_PNG,
            EX_RAW,
            EX_BMP,
        };


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
                              eEXPORT_t                         exporttype     = eEXPORT_t::EX_PNG, 
                              bool                              bBeQuiet       = false );

    private:
        //Write an entry in the toc at the index specified, in the subentry specified. The value written is the data index in the m_imgdata vector
        void registerToCEntry( std::size_t tocindex, std::size_t subentryindex, std::size_t dataindex )
        {
            m_tableofcontent[tocindex]._portraitsentries[subentryindex] = tocsubentry_t( dataindex);
        }

        //Write all the valid portraits out for a single ToC entry
        void ExportAToCEntry( const std::vector<tocsubentry_t> & entry, 
                              const std::string                & outputfoldername, 
                              const std::vector<std::string>   * psubentrynames,
                              eEXPORT_t                          exporttype        = eEXPORT_t::EX_PNG );

        //Params are entire file begining iterator, iterator of the entry, and a map containing a list of all the unique data
        // offsets and where they were stored in the data vector.
        //The buffer passed is to avoid re-allocating each turns !
        types::constitbyte_t CKaomado::ReadAToCEntry( std::vector<kao_toc_entry>::size_type  & indexentry,
                                                      types::constitbyte_t                     itbegindata, 
                                                      types::constitbyte_t                     itrawtocentry, 
                                                      /*std::map<int32_t, size_t >            &  referencemap,*/
                                                      std::vector<uint8_t>  &                  imagebuffer,
                                                      bool                                     bBeQuiet = false);

        tocsubentry_t        ReadAToCSubEntry( types::constitbyte_t & itbegindata );
        bool                 isToCSubEntryValid( const tocsubentry_t & entry )const;

        //Value1 is toclen, value2 is biggest image len!
        std::pair<uint32_t,uint32_t> EstimateToCLengthAndBiggestImage()const;
        uint32_t                     CalculateEntryLen( types::constitbyte_t itdatabeg, tocsubentry_t entryoffset );

        void                 HandleAFolder( kao_file_wrapper & foldertohandle );

        static tocsubentry_t GetInvalidToCEntry();

        std::vector<kao_toc_entry>::size_type InputAnImageToDataVector( kao_file_wrapper & foldertohandle );

        void WriteAPortrait( kao_toc_entry::subentry_t &                     portrait,
                             std::vector<uint8_t> &                          outputbuffer, 
                             std::back_insert_iterator<std::vector<uint8_t>> itoutputpushback,
                             std::vector<uint8_t> &                          imgbuffer, 
                             std::back_insert_iterator<std::vector<uint8_t>> itimgbufpushback,
                             bool                                            bZealousStringSearch,
                             tocsubentry_t &                                 lastValidEndOffset,
                             uint32_t &                                      offsetWriteatTocSub );

        //Variables
        uint32_t                                   m_nbtocsubentries;

        //New stuff
        std::vector<kao_toc_entry>                     m_tableofcontent;
        std::vector<data_t>                            m_imgdata;
    };


};};

#endif