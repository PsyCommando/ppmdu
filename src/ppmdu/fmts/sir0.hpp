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

//==================================================================
// Structs
//==================================================================
    /*
        Header structure for a SIR0 file.
    */
    struct sir0_header : public utils::data_array_struct
    {
        static const unsigned int HEADER_LEN       = 16u; //bytes
        static const unsigned int MAGIC_NUMBER_LEN = 4u;

        uint32_t magic;
        uint32_t subheaderptr;
        uint32_t eofptr;
        uint32_t _null;

        std::string toString()const;
        unsigned int   size()const { return HEADER_LEN; }

        std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const;
        std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom );
    };


//==================================================================
// Functions
//==================================================================
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
    //                header.eofptr       <= filesizetotal         && header.subheaderptr > 0x0
    //           );
    //}
    
 //#TODO: maybe add some functors or something in here ? Or a wrapper or something

};};
#endif