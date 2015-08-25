#ifndef SPRITE_IO_HPP
#define SPRITE_IO_HPP
/*
sprite_io.hpp
2015/02/11
psycommando@gmail.com
Description:
    Sprite data xml parser/writer implementation.
*/
#include "sprite_data.hpp"
#include <utils/utility.hpp>
#include <ext_fmts/supported_io.hpp>
#include <vector>
#include <string>
#include <cstdint>
#include <atomic>

namespace pmd2 { namespace graphics
{
//==============================================================================================
// Constants
//==============================================================================================

    //Filesnames
    //static const std::string SPRITE_Properties_fname;
    //static const std::string SPRITE_Palette_XML_fname;
    //static const std::string SPRITE_Animations_fname;
    //static const std::string SPRITE_Frames_fname;
    //static const std::string SPRITE_Offsets_fname;
    //static const std::string SPRITE_IMGs_DIR;         //Name of the sub-folder for the images
    //static const std::string SPRITE_Palette_fname;

    //Filesnames
    static const std::string SPRITE_Properties_fname  = "spriteinfo.xml";
    static const std::string SPRITE_Palette_XML_fname = "palette.xml";
    static const std::string SPRITE_Animations_fname  = "animations.xml";
    static const std::string SPRITE_Frames_fname      = "frames.xml";
    static const std::string SPRITE_Offsets_fname     = "offsets.xml";
    static const std::string SPRITE_IMGs_DIR          = "imgs";         //Name of the sub-folder for the images
    static const std::string SPRITE_Palette_fname     = "palette.pal";
    static const std::string SPRITE_ImgsInfo_fname    = "imgsinfo.xml"; 

//=============================================================================================
//  Structs
//=============================================================================================

    /**************************************************************
    Used to hold a couple of useful statistics to determine task completion.
    **************************************************************/
    struct spriteWorkStats
    {
        uint32_t propFrames    = 0;
        uint32_t propAnims     = 0;
        uint32_t propMFrames   = 0;
        uint32_t propOffsets   = 0;
        uint32_t propImgInfo   = 0;
        uint32_t totalAnimFrms = 0;
        uint32_t totalAnimSeqs = 0;
    };
    
//=============================================================================================
// Sprite IO Handling
//=============================================================================================

    /*
        Export a sprite to a directory structure, a palette, and several xml data files.
        -imgtype   : The supported image type to use for exporting the individual frames. 
        -usexmlpal : If true, the palette will be exported as an xml file, and not a
                     RIFF palette.
        -progress  : An atomic integer to increment all the way to 100, to indicate
                     current progress with export.
    */
    template<class _Sprite_T>
        void ExportSpriteToDirectory( const _Sprite_T            & srcspr, 
                                      const std::string          & outpath, 
                                      utils::io::eSUPPORT_IMG_IO   imgtype     = utils::io::eSUPPORT_IMG_IO::PNG,
                                      bool                         usexmlpal   = false,
                                      std::atomic<uint32_t>      * progresscnt = nullptr );

    void ExportSpriteToDirectoryPtr( const graphics::BaseSprite * srcspr, 
                                      const std::string          & outpath, 
                                      utils::io::eSUPPORT_IMG_IO   imgtype     = utils::io::eSUPPORT_IMG_IO::PNG,
                                      bool                         usexmlpal   = false,
                                      std::atomic<uint32_t>      * progresscnt = nullptr );

    /*
        ImportSpriteFromDirectory
            Call this to import any types of Sprite.

            -bReadImgByIndex : If true we'll enforce the image order indicated by the number 
                               in the name of the image. If false, we'll simply pushback images
                               in the alpha-numeric order they're in the folder, applying no
                               check on the index number.
            -bParseXmlPal    : Whether we should try parsing a palette from xml!
            -bNoResAutoFix   : If true, when a resolution mismatch between an image and a meta-frame occur
                               the meta-frame resolution will not be changed to match the image's!
    */
    template<class _Sprite_T>
        _Sprite_T ImportSpriteFromDirectory( const std::string     & inpath, 
                                             bool                    bReadImgByIndex = false,
                                             bool                    bParseXmlPal    = false,
                                             std::atomic<uint32_t> * progresscnt     = nullptr,
                                             bool                    bNoResAutoFix   = false );


//---------------------------------------------------------------------------------------------
//  Sprite Validation and Information
//---------------------------------------------------------------------------------------------

    /*
        Check if all the required files and subfolders are in the filelist passed as param!
        Use this before calling ImportSpriteFromDirectory on the list of the files present
        in the dir to make sure everything is ok!
    */
    bool                AreReqFilesPresent_Sprite( const std::vector<std::string> & filelist );
    bool                AreReqFilesPresent_Sprite( const std::string              & directorypath );

    /*
        Return the missing required files in the file list specified.
    */
    std::vector<std::string> GetMissingRequiredFiles_Sprite( const std::vector<std::string> & filelist );
    std::vector<std::string> GetMissingRequiredFiles_Sprite( const std::string              & directorypath );


    /*
        Whether the image resolution is one of the valid sprite image resolution.
    */
    bool Sprite_IsResolutionValid( uint16_t width, uint16_t height );


    /*
        QuerySpriteImgTypeFromDirectory
            Check the directory to get what's the sprite_ty of the sprite.
    */
    eSpriteImgType QuerySpriteImgTypeFromDirectory( const std::string & dirpath );


//=============================================================================================
//  XML Handling
//=============================================================================================

    /******************************************************************************************
    ParseXMLDataToSprite
        Use this to read the expected XML files into the specified sprite container.
    ******************************************************************************************/
    void ParseXMLDataToSprite( BaseSprite        * out_spr, 
                               const std::string & spriteFolderPath, 
                               uint32_t            nbimgs,
                               bool                parsexmlpal = false);

    /******************************************************************************************
    WriteSpriteDataToXML
        Use this to write to a set of XML files the data from the specified sprite container.
    ******************************************************************************************/
    void WriteSpriteDataToXML( const BaseSprite      * spr, 
                               const std::string     & spriteFolderPath, 
                               const spriteWorkStats & stats, 
                               bool                    writexmlpal = false, 
                               std::atomic<uint32_t> * progresscnt = nullptr );
       
};};


#endif