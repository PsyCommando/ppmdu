#ifndef SIR0_HPP
#define SIR0_HPP
/*
sir0.hpp
20/05/2014
psycommando@gmail.com

Description:
A set of helper tools and data structures for working
with SIR0 containers !

No crappyrights. All wrongs reversed !
*/
#include <ppmdu/pmd2/pmd2_filetypes.hpp>
#include <ppmdu/fmts/content_type_analyser.hpp>
#include <ppmdu/utils/utility.hpp>

namespace pmd2 { namespace filetypes
{
//====================================================================================================
//  Typedefs
//====================================================================================================
    //The list of pointer offsets in the file to be wrapped by the SIR0 container.
    //typedef std::vector<uint32_t> sir0_ptr_list_t;

//====================================================================================================
// Structs
//====================================================================================================
    /*
        Header structure for a SIR0 file.
    */
    struct sir0_header : public utils::data_array_struct
    {
        static const unsigned int HEADER_LEN       = 16u; //bytes
        static const unsigned int MAGIC_NUMBER_LEN = 4u;

        uint32_t magic;
        uint32_t subheaderptr;
        uint32_t ptrPtrOffsetLst;        //#TODO: Rename !!!
        uint32_t _null;

        std::string toString()const;
        unsigned int   size()const { return HEADER_LEN; }

        sir0_header( uint32_t magicnumber = 0u, uint32_t subhdroffset = 0u, uint32_t offptrlst = 0u )
            :magic(magicnumber), subheaderptr(subhdroffset), ptrPtrOffsetLst(offptrlst), _null(0u)
        {}

        std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const;
        std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom );
    };

    /*
        sir0_head_and_list
            This is used to contain the sir0 data computed for a given block of data.
    */
    struct sir0_head_and_list
    {
        sir0_header          hdr;
        std::vector<uint8_t> ptroffsetslst;
    };

//====================================================================================================
// Functions
//====================================================================================================

    /***********************************************************************************
        MakeSIR0ForData
            Description:
                This will compute a SIR0 header, and the pointer offset list 
                to be inserted at the end of the file. 

                It needs a list containing the offsets relative to the 
                beginning of the data to be wrapped in a SIR0 container.
                (not including the length of the SIR0 header, as the 
                function will compensate for that when encoding the list)

                It also needs the offset where the sub-header is. 
                (again not counting the SIR0 header length)

                And finally the offset one past the last byte of 
                data to be wrapped. (again not counting the SIR0 
                header length) This is where the
                list of pointer offsets will be inserted. 
                So, the padding bytes in-between the end of the 
                data and the list must be counted!

                It will return the computed SIR0 header, 
                and the full encoded list of pointer offsets. 
                The list doesn't include the padding bytes however.

    ***********************************************************************************/
    sir0_head_and_list MakeSIR0ForData( const std::vector<uint32_t> &listoffsetptrs,
                                        uint32_t                     offsetsubheader,
                                        uint32_t                     offsetendofdata );


    /**************************************************************************
        EncodeSIR0PtrOffsetList
            Description:
                Encode a list of offsets into a 
                null-terminated string of bytes,
                to be placed at the end of a SIR0 container.
    ***************************************************************************/
    std::vector<uint8_t>  EncodeSIR0PtrOffsetList( const std::vector<uint32_t> &listoffsetptrs );
    void                  EncodeSIR0PtrOffsetList( const std::vector<uint32_t> &listoffsetptrs, std::vector<uint8_t> & out_encoded );

    /**************************************************************************
        DecodeSIR0PtrOffsetList
            Description:
                Decode the encoded list of pointers 
                offsets and turns them into offsets 
                relative to the beginning of the SIR0.
                The list must only include the first 
                byte of the encoded offsets, and the 
                last one, 0x0.
    ***************************************************************************/
    std::vector<uint32_t> DecodeSIR0PtrOffsetList( const std::vector<uint8_t>  &ptroffsetslst  );

    /*
        Test a sir0_header struct to verify if its from a valid SIR0 container.
    */
    //inline bool IsValid_SIR0_Header( const sir0_header & header, filesize_t filesizetotal )
    //{
    //    using namespace magicnumbers;
    //    return ( 
    //                header.magic        == SIR0_MAGIC_NUMBER_INT && 
    //                header._null        == 0x0                   && 
    //                header.subheaderptr <  filesizetotal         && header.subheaderptr > 0x0 &&
    //                header.ptrPtrOffsetLst       <= filesizetotal         && header.subheaderptr > 0x0
    //           );
    //}
    
 //#TODO: maybe add some functors or something in here ? Or a wrapper or something

};};
#endif