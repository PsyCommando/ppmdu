#ifndef WAN_HPP
#define WAN_HPP
/*
wan.hpp
2014/12/31
psycommando@gmail.com
Description: Utilities for reading ".wan" sprite files, and its derivatives.
*/
#include <ppmdu/fmts/sir0.hpp>
#include <ppmdu/utils/utility.hpp>

namespace pmd2 { namespace filetypes 
{
//=============================================================================================
//  WAN Structures
//=============================================================================================
    /**********************************************************************
        The 12 bytes sub-header that is linked to by an SIR0 header.
        Contains pointers to important sprite information.
    **********************************************************************/
    struct wan_sub_header : public utils::data_array_struct
    {
        uint32_t ptr_infochunk,
                 ptr_frameschunk;
        uint16_t unknown0,
                 unknown1;

        static const unsigned int DATA_LEN = 12u;

        unsigned int size()const{return DATA_LEN;}
        std::string toString()const;

        std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const;
        std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom );
    };

    /**********************************************************************
        The part of the header containing the pointers to the actual frames 
        of the sprite, and the palette.
    **********************************************************************/
    struct wan_frame_data : public utils::data_array_struct
    {
        static const unsigned int DATA_LEN = 16u;

        uint32_t ptr_frm_ptrs_table,        // Pointer to the the table of pointer to the individual frames
                 ptr_palette;               // Pointer to the pointer to the palette info
        uint16_t unkn_1,                    // Unknown, seems to range between 0 and 1..
                 unkn_2,                    // Unknown, seems to range between 0 and 1..
                 unkn_3,                    // Unknown, seems to range between 0 and 1..
                 nb_ptrs_frm_ptrs_table;    // Number of entries in the table of pointers to each frames.

        unsigned int size()const{return DATA_LEN;}
        std::string toString()const;

        std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const;
        std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom );
    };

    /**********************************************************************
        The part of the header containing the pointers to unidentified kind 
        of info for the sprite
    **********************************************************************/
    struct wan_info_data : public utils::data_array_struct
    {
        static const unsigned int DATA_LEN = 24u;

        uint32_t ptr_ptrstable_e, //pointer to the pointers table at OFF_E
                 ptr_offset_f,
                 ptr_offset_g;
        uint16_t nb_blocks_in_offset_g,
                 nb_entries_offset_e,  //<-- The name for this is wrong, we don't know what this value does
                 unknown1,
                 unknown2,
                 unknown3,
                 unknown4;

        unsigned int size()const{return DATA_LEN;}
        std::string toString()const;

        std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const;
        std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom );
    };

//=============================================================================================
//  Functions / Functors
//=============================================================================================

    /*
    */
    class Parse_WAN
    {
    };

};};

#endif