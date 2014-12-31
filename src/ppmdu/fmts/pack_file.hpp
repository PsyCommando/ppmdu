#ifndef PACK_FILE_HPP
#define PACK_FILE_HPP
/*
pack_file.hpp
2014/10/10
psycommando@gmail.com
Description:  Code for handling "pack files".

**** TODO *****
 - Implement content type detection rule!
 - rewrite this mess into something simpler
 - improve for memory loading and manipulation

*/
#include <exception>
#include <string>
#include <vector>
#include <memory>
#include <ppmdu/basetypes.hpp>
#include <ppmdu/utils/utility.hpp>

namespace pmd2 { namespace filetypes 
{
//===============================================================================
// Struct
//===============================================================================

	//--------------------------------------------------------------------------
	//								fileIndex
	//--------------------------------------------------------------------------
	//Element containing a file offset and a file length, for use in a vector container
	struct fileIndex : utils::data_array_struct
	{
        static const uint32_t ENTRY_LEN = 8u;
		fileIndex()
		:_fileOffset(0), _fileLength(0)
		{}

		fileIndex( uint32_t offset, uint32_t length )
			:_fileOffset(offset), _fileLength(length)
		{}

        inline unsigned int size()const{return ENTRY_LEN;}

        std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const;
        std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom );


		uint32_t _fileOffset,
                 _fileLength;
	};


	//--------------------------------------------------------------------------
	//								pfheader
	//--------------------------------------------------------------------------
    struct pfheader : utils::data_array_struct
    {
        static const uint32_t HEADER_LEN = 8u;

        uint32_t _zeros, 
                 _nbfiles;

        inline unsigned int size()const{return HEADER_LEN;}
        bool                isValid()const;

        std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const;
        std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom );
    };

//===============================================================================
//                                Constants
//===============================================================================

	static const uint32_t OFFSET_TBL_FIRST_ENTRY          = 0x8; // First file's entry in the offset table
    static const uint32_t SZ_OFFSET_TBL_ENTRY             = 0x8; // The size in bytes of one entry in the offset table.

    //Pack File End Of Index Table End Delemiter
    static const int                                       SZ_OFFSET_TBL_DELIM = 8;
    static const std::array<uint8_t, SZ_OFFSET_TBL_DELIM>  OFFSET_TBL_DELIM    =
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

//===============================================================================
//								       CPack
//===============================================================================
//Class for handling the creation of a pack file entirely in memory for faster and cleaner processing.
// Files are loaded one after the other in memory and added to the file offset table on the fly, which allows to compute 
// padding and etc much more cleanly and with less chances for bugs/mistakes !
    class CPack
    {
    public:
        //-------------------------------
        //Constructors & Destructors
        //-------------------------------
        CPack();

        //-------------------------------
        //Accessors
        //-------------------------------

        //Return the predicted header length. This is also the predicted offset of the first file.
        // IT DOES NOT TAKE INTO ACCOUNT THE FORCED FIRST FILE OFFSET !
        uint32_t getCurrentPredictedHeaderLengthWithForcedOffset()const;

        //Whether the first file should be at this offset as much as possible. For special pack files in PMD2. 
        // It will add extra padding to align the first file, just like in some of the original files shipped with the game !
        // If not == 0, the offset will be enforced as best as possible, however its not always possible to. To check if the 
        // offset will be forced for sure, call IsForcedOffsetCurrentlyPossible(). Set to 0 to disable.
        void setForceFirstFilePosition( uint32_t forcedoffset ) { m_ForcedFirstFileOffset = forcedoffset; }
        uint32_t getForcedFirstPosition()const                  { return m_ForcedFirstFileOffset; }

        //Will return whether the forced offset can be enforced with the current content.
        bool IsForcedOffsetCurrentlyPossible()const;

        //-------------------------------
        //Methods
        //-------------------------------


        //------- I/O --------

        //If path is a pack file, its loaded into memory.
        void LoadPack( types::constitbyte_t beg, types::constitbyte_t end );

        //If the input path is a folder, a pack file is made with the files from the folder. 
        void LoadFolder( const std::string & pathdir );

        //Write the pack file to the output file path.
        // The method calls BuildFOT() before outputting the file, so its not neccessary to call it before.
        types::bytevec_t OutputPack(); //Move constructor should make this very efficient

        //This allow to output the sub files of this pack file into a specified directory.
        void OutputToFolder( const std::string & pathdir );


        //------- Pack Manipulation --------

        //#TODO: Provide better controlled ways to access those things:
        std::vector<types::bytevec_t> & SubFiles()                                {return m_SubFiles;}
        types::bytevec_t              & getSubFile( types::bytevec_szty_t index ) {return m_SubFiles[index];}

    private:
        //-------------------------------
        //Methods
        //-------------------------------

        //After files have been changed this recalculate the offset table, and padding. 
        void BuildFOT();

        //Clear the file and fot vectors and reset the forced offset to 0
        void ClearState();

        //Returns a vector filled with the rigth ammount of padding bytes for the header based on the current object's state
        void MakeHeaderPaddingBytes( types::bytevec_t & paddingbytes )const;
        uint32_t CalcAmountHeaderPaddingBytes()const;

        //Used to calculate the size of a packfile header based only on the parameters of the method
        static uint32_t PredictHeaderSize( uint32_t nbsubfiles );
        //Same as above, but this one takes into account ONLY the BARE MINIMUM in terms of header padding.
        static uint32_t PredictHeaderSizeWithPadding( uint32_t nbsubfiles );
        //Calculate the expected total filesize from the current object's state
        uint32_t PredictTotalFileSize()const;
        
        
        //------- Raw Reading Pack File Operations --------
        types::constitbyte_t ReadHeader( types::constitbyte_t itbegin, pfheader & out_mahead )const;

        //Reads the File Offset Table from the raw pack file data into the object's FOT
        void     ReadFOTFromPackFile( types::constitbyte_t itbegin, unsigned int nbsubfiles );

        //Reads subfiles from the raw file data based on what is currently in the object's File Offset Table
        // The iterator must be at the beginning of the entire file's raw data !
        //  NOTE: This method expects you to have called ReadFOTFromPackFile first to populate the FOT! 
        //        Or at least to have a FOT longer than 0 !  
        void     ReadSubFilesFromPackFileUsingFOT( types::constitbyte_t itbegin );

        //Returns the forced offset from the file's raw data is using one, or 0 if its not!
        uint32_t CPack::IsPackFileUsingForcedFFOffset( types::constitbyte_t itbeg, unsigned int nbsubfiles )const;

        //Opens for copying to the subfile vector a single file. 
        void     ReadLooseFileToFileDataVector( const std::string & inpath, unsigned long long filesize, uint32_t insertatindex );

        //------- Raw Writing Pack File Operations --------

        //Write the full header at the position pointed by the iterator
        types::itbyte_t WriteFullHeader( types::itbyte_t writeat )const;

        //Write all the file's data in the subfiles vector to the uint8_t vector passed as parameter, at the position pointed by the iterator  
        types::itbyte_t WriteFileData( types::itbyte_t writeat ); //#todo: it should be const, but a stupid mistake with the CPack file makes it fail..

        //Write a subfile from the subfile vector to the specified file. 
        void WriteSubFileToFile( types::bytevec_t & file, const std::string & path, unsigned int fileindex );

        //-------------------------------
        //Variables
        //-------------------------------
        uint32_t                      m_ForcedFirstFileOffset;
        std::vector<fileIndex>        m_OffsetTable;
        std::vector<types::bytevec_t> m_SubFiles;
    };

};};

#endif