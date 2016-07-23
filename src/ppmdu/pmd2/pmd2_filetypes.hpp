#ifndef PMD2_FILETYPES_HPP
#define PMD2_FILETYPES_HPP
/*
pmd2_filetypes.hpp
24/05/2014
psycommando@gmail.com

Description:

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P
*/
//!#MOSTLY OBSOLETE!! 


#include <string>
#include <vector>
#include <ppmdu/pmd2/pmd2.hpp>
#include <utils/handymath.hpp>
#include <types/content_type_analyser.hpp>

namespace pmd2 { namespace filetypes 
{
//==================================================================
// Typedefs
//==================================================================
    typedef unsigned int filesize_t;

//==================================================================
// Constants
//==================================================================
    


    //namespace magicnumbers
    //{
    //    //#TODO: Not sure if we should put those in here :/
	   // //Common Magic Numbers 
    //    static const std::array<uint8_t,4> SIR0_MAGIC_NUMBER     = { 0x53, 0x49, 0x52, 0x30 };       //"SIR0"
    //    static const uint32_t              SIR0_MAGIC_NUMBER_INT = 0x53495230;                       //"SIR0", stored as unsigned int for convenience
    //    static const std::array<uint8_t,5> PKDPX_MAGIC_NUMBER    = { 0x50, 0x4B, 0x44, 0x50, 0x58 }; //"PKDPX"
    //    static const std::array<uint8_t,5> AT4PX_MAGIC_NUMBER    = { 0x41, 0x54, 0x34, 0x50, 0x58 }; //"AT4PX"
    //};

    //File extensions used in the library
    static const std::string AT4PX_FILEX             = "at4px";
    static const std::string AT4PN_FILEX             = "at4pn";
    static const std::string IMAGE_RAW_FILEX         = "rawimg";
    static const std::string KAOMADO_FILEX           = "kao";
    static const std::string PACK_FILEX              = "bin";
    static const std::string PKDPX_FILEX             = "pkdpx";
    static const std::string SIR0_FILEX              = "sir0";
    static const std::string WAN_FILEX               = "wan";
    static const std::string WTE_FILEX               = "wte";
    static const std::string WTU_FILEX               = "wtu";
    static const std::string BGP_FILEX               = "bgp";
    static const std::string SIR0_PKDPX_FILEX        = "sir0pkdpx";    //For a sir0 wrapped pkdpx
    static const std::string SIR0_AT4PX_FILEX        = "sir0at4px";    //For a sir0 wrapped at4px
    static const std::string RGBX32_RAW_PAL_FILEX    = "rgbx32";       //For a file containing only a raw RGBX32 palette
    static const std::string MD_FILEX                = "md";           //For the monster.md file

    //DSE Types
    static const std::string SMDL_FILEX              = "smd";
    static const std::string SWDL_FILEX              = "swd";
    static const std::string SEDL_FILEX              = "sed";

    //Padding bytes
    static const uint8_t COMMON_PADDING_BYTE        = 0xAA; //The most common padding byte in all PMD2 files !


    /*
        A little struct for the special pokemon sprite pack file list below.
    */
    struct packedPokeSprs
    {
        std::string name;
        bool        isCompressed;
    };

    /*
        The 3 files that contain the pokemon sprites in pmd2 are special.
        They all end with the ".bin" file extension.
    */
    static const std::array<packedPokeSprs,3> PackedPokemonSpritesFiles = 
    {{
        { "m_attack", true  },
        { "m_ground", false },
        { "monster",  true  },
    }};


//==================================================================
// Enums
//==================================================================
    /*
        The possible content types that can be found within a file / another container.
    */
    enum struct e_ContentType : unsigned int
    {
        PKDPX_CONTAINER,
        AT4PX_CONTAINER,
        AT4PN_CONTAINER,
        SIR0_CONTAINER,
        SIR0_AT4PX_CONTAINER, //SIR0-wrapped AT4PX
        SIR0_PKDPX_CONTAINER, //SIR0-wrapped PKDPX
        COMPRESSED_DATA,    //For the content of PKDPX container, given we can't decompress when analysing
        PACK_CONTAINER,
        KAOMADO_CONTAINER,
        WAN_SPRITE_CONTAINER,
        WTE_FILE,
        WTU_FILE,
        BGP_FILE,
        RAW_RGBX32_PAL_FILE,
        MONSTER_MD_FILE,

        //DSE Types
        SMDL_FILE,
        SWDL_FILE,
        SEDL_FILE,

        //#If you add any more, be sure to modify GetContentTypeName
        UNKNOWN_CONTENT,
    };

//==================================================================
// Functions
//==================================================================

    /*
        Pass the file extension of a file and get the possible matches list.
        Several kinds of files share the same type of extension.
    */
    std::vector<::filetypes::cnt_t> GetFileTypeFromExtension( const std::string & ext );

    //For the given data, returns a file extension
    std::string GetAppropriateFileExtension( std::vector<uint8_t>::const_iterator & itdatabeg,
                                             std::vector<uint8_t>::const_iterator & itdataend );

    //#TODO: Deprecate this !
    //Returns a short string identifying what is the type of content is in this kind of file !
    //std::string GetContentTypeName( e_ContentType type );



};};

#endif
