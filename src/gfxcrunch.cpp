#include "gfxcrunch.hpp"
#include <types/content_type_analyser.hpp>
#include <utils/utility.hpp>
//#include <ppmdu/pmd2/pmd2_sprites.hpp>
#include <ppmdu/pmd2/pmd2_filetypes.hpp>
#include <ppmdu/containers/sprite_data.hpp>
#include <utils/multiple_task_handler.hpp>
#include <utils/library_wide.hpp>
#include <ppmdu/fmts/wan.hpp>
#include <ppmdu/fmts/pack_file.hpp>
#include <ppmdu/fmts/pkdpx.hpp>
#include <ppmdu/fmts/at4px.hpp>
#include <ppmdu/fmts/wte.hpp>
#include <ppmdu/fmts/bgp.hpp>
#include <ppmdu/fmts/kao.hpp>
#include <cfenv>
#include <string>
#include <algorithm>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cassert>
#include <utility>
#include <atomic> 
#include <chrono>
#include <thread>
#include <future>
#include <fstream>
#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/Exception.h>

//!#FIXME: The program right now isn't very functional!!!
/*
#TODO: Once we got the proper replacement for loading game strings in place, replace this and remove
       all the other extra unneeded files included in compilation!
*/
#include <ppmdu/pmd2/pmd2_configloader.hpp>
#include <ppmdu/pmd2/pmd2_text.hpp>

using namespace ::std;
using namespace ::pmd2;
using namespace ::pmd2::graphics;
using namespace ::pmd2::filetypes;
using namespace ::utils::cmdl;
using namespace ::utils::io;
using namespace ::filetypes;


namespace gfx_util
{
//=================================================================================================
//  Constants
//=================================================================================================
    //static const int                  HPBar_NB_Bars        = 65u;
    //static const int                  HPBar_UpdateMSecs    = 80; //Updates at every HPBar_UpdateMSecs mseconds
    static const uint32_t             ForcedPokeSpritePack = 0x1300; //The offset where pack files containing pokemon sprites are forced to begin at
    static const chrono::milliseconds ProgressUpdThWait    = chrono::milliseconds(100);

    static const int                  RETVAL_GenericFail  = -1;
    static const int                  RETVAL_InvalidOp    = -2;
    static const int                  RETVAL_BadArg       = -3;
    static const int                  RETVAL_UnkException = -4;
    static const int                  RETVAL_IOException  = -5;

    //Just some silliness when converting stuff in batches!
    static const vector<string>       VariationOfAgain    = 
    {{
            "",
            "again",
            "eagerly",
            "once more",
            "with gutso",
            "another time",
            "stubbornly",
            "swiftly",
            "defiantly",
            "with devotion",
            "boldly",
            "deftly",
            "elegantly",
            "with moxie",
            "gracefully",
            "tenaciously",
            "tactfully",
            "audaciously",
            "esquisitely",
            "stumbling in the process",
            "with some hesitation",
            "with a bit less enthusiasm",
            "showing some fatigue",
            "with visibly less moxie",
            "lazily",
            "disinterested",
            "that's what I'd say if she had used it",
            "not",
            "actually she left",
            "that was a lie.. Ok, we're getting another pooch",
    }};

//=================================================================================================
// 
//=================================================================================================


//=================================================================================================
// Utility
//=================================================================================================

    /*
        Verify if "filename" is the name of one of the 3 special pack file that contain
        pokemon sprites.
    */
    bool MatchesPokeSpritePackFileName( const std::string & filename )
    {
        for( const auto & apack : PackedPokemonSpritesFiles )
        {
            if( filename.compare( apack.name ) == 0 )
                return true;
        }
        return false;
    }

    /*
        Compare the filname to the list of the compressed pokemon sprite pack files!
        And return if the file is indeed compressed.
    */
    bool MatchesCompressedPokeSpritePackFileName( const std::string & filename )
    {
        for( const auto & apack : PackedPokemonSpritesFiles )
        {
            if( filename.compare( apack.name ) == 0 && apack.isCompressed )
                return true;
        }
        return false;
    }

    void PrintProgressLoop( atomic<uint32_t> & completed, uint32_t total, atomic<bool> & bDoUpdate )
    {
        while( bDoUpdate )
        {
            if( completed.load() <= total )
            {
                uint32_t percent = ( (100 * completed.load()) / total );
                cout << "\r" <<setw(3) <<setfill(' ') <<percent <<"%" <<setw(15) <<setfill(' ') <<" ";
            }
            this_thread::sleep_for( ProgressUpdThWait );
        }
    }

    /*
    */
    CPack UnpackPackFile( const Poco::Path & packfilepath )
    {
        vector<uint8_t> result = utils::io::ReadFileToByteVector( packfilepath.toString() );
        CPack           mypack;

        mypack.LoadPack( result.begin(), result.end() );

        return std::move( mypack );
    }

    void ParseASprite( const std::vector<uint8_t> & srcraw, std::unique_ptr<graphics::BaseSprite> & targetptr )
    {
        WAN_Parser parser( srcraw );
        auto       sprty = parser.getSpriteType();
                
        if( sprty == graphics::eSpriteImgType::spr4bpp )
        {
            unique_ptr<SpriteData<gimg::tiled_image_i4bpp>> ptrtmp( new SpriteData<gimg::tiled_image_i4bpp>() );
            (*ptrtmp) = parser.ParseAs4bpp();
            targetptr.reset( ptrtmp.release() );
        }
        else if( sprty == graphics::eSpriteImgType::spr8bpp )
        {
            unique_ptr<SpriteData<gimg::tiled_image_i8bpp>> ptrtmp( new SpriteData<gimg::tiled_image_i8bpp>() );
            (*ptrtmp) = parser.ParseAs8bpp();
            targetptr.reset( ptrtmp.release() );
        }
        else
        {
            assert(false); //! #TODO
        }
    }

    /*
    */
    void TurnPackContentToSpriteData( CPack & srcpack, std::vector<std::unique_ptr<graphics::BaseSprite>> & out_table )
    {
        cout <<"Parsing sprites..\n";
        out_table.resize( srcpack.getNbSubFiles() );
        for( unsigned int i = 0; i < srcpack.getNbSubFiles(); )
        {
            auto & curSubFile = srcpack.getSubFile(i);
            auto   cnttype    = DetermineCntTy( curSubFile.begin(), curSubFile.end() );

            //
            if( cnttype._type == CnTy_WAN )
            {
                if( utils::LibWide().isLogOn() )
                {
                    clog <<"============================\n"
                         <<"== Parsing Sprite #" <<setfill('0') <<setw(3) <<i <<" ==\n"
                         <<"============================\n";
                }
                //Convert directly
                ParseASprite( curSubFile, out_table[i] );
            }
            else if( cnttype._type == CnTy_PKDPX )
            {
                //Do decompress before converting!
                vector<uint8_t> decompbuff;
                DecompressPKDPX( curSubFile.begin(), curSubFile.end(), decompbuff );

                //Do a check on the decompressed file
                if( DetermineCntTy( decompbuff.begin(), decompbuff.end() )._type != CnTy_WAN )
                    continue; //skip this file if not a wan sprite

                ParseASprite( decompbuff, out_table[i] );
            }
            //Skip anything else

            ++i;
            cout<<"\r" <<setw(3) <<setfill(' ') <<(( i * 100) / srcpack.getNbSubFiles()) <<"%";
        }
    }

    /*
        Validates a path option, and set it to the specified string!
        Return false if path is invalid
    */
    inline bool ParseOptionFilePath( const std::vector<std::string> & optdata, std::string & out_path )
    {
        if( optdata.size() > 1 )
        {
            Poco::File testpath( optdata[1] );

            if( testpath.exists() && testpath.isFile() )
            {
                out_path = optdata[1];
                return true;
            }
        }
        return false;
    }

    /*
    */
    inline bool IsFolderASpriteFolder( std::string path )
    {
        return AreReqFilesPresent_Sprite( utils::ListDirContent_FilesAndDirs( path, true ) );
    }

//=================================================================================================
//  CGfxUtil
//=================================================================================================

//------------------------------------------------
//  Constants
//------------------------------------------------
    const string CGfxUtil::Exe_Name            = "ppmd_gfxcrunch.exe";
    const string CGfxUtil::Title               = "Baz the Poochyena's PMD:EoS/T/D GfxCrunch";
    const string CGfxUtil::Version             = "0.13";
    const string CGfxUtil::Short_Description   = "A utility to unpack and re-pack pmd2 sprite!";
    const string CGfxUtil::Long_Description    = 
        "#TODO";                                    //#TODO
    const string CGfxUtil::Misc_Text           = 
        "Named in honour of Baz, the awesome Poochyena of doom ! :D\n"
        "My tools in binary form are basically public domain / CC0.\n"
        "Free to re-use in any ways you may want to!\n"
        "No crappyrights, all wrongs reversed! :3";

    const string CGfxUtil::DefPathAnimRes      = "animres.xml";
    const string CGfxUtil::DefPathPokeSprNames = "pokesprites_names.txt";
    const string CGfxUtil::DefPathFaceNames    = "facenames.txt";
    const string CGfxUtil::DefPathPokeNames    = "pokenames.txt";

//------------------------------------------------
//  Arguments Info
//------------------------------------------------

    /*
        Data for extra argument
    */
    const argumentparsing_t CGfxUtil::ExtraArg =
    {
            2,      //third arg
            true,  //optional
            true,   //guaranteed to appear in order
            "extra paths", 
            "Path to an additional item to process.",
#ifdef WIN32
            "\"c:/mysprites/sprite.wan\"",
#elif __linux__
            "\"/mysprites/sprite.wan\"",
#endif
            std::bind( &CGfxUtil::ParseExtraPath, &GetInstance(), placeholders::_1 ),
    };


    /*
        Data for the automatic argument parser to work with.
    */
    const vector<argumentparsing_t> CGfxUtil::Arguments_List =
    {{
        //Input Path argument
        { 
            0,      //first arg
            false,  //mandatory
            true,   //guaranteed to appear in order
            "input path", 
            "The path to either a folder structure containing the data of a sprite to build, or a sprite file to unpack.",
#ifdef WIN32
            "\"c:/mysprites/sprite.wan\"",
#elif __linux__
            "\"/mysprites/sprite.wan\"",
#endif
            std::bind( &CGfxUtil::ParseInputPath, &GetInstance(), placeholders::_1 ),
        },
        //Output Path argument
        { 
            1,      //second arg
            true,   //optional
            true,   //guaranteed to appear in order
            "output path", 
            "The path where to output the result of the operation. Can be a folder, or a file, depending on whether we're building a sprite, or unpacking one.",
#ifdef WIN32
            "\"c:/mysprites/sprite.wan\"",
#elif __linux__
            "\"/mysprites/sprite.wan\"",
#endif
            std::bind( &CGfxUtil::ParseOutputPath,       &GetInstance(), placeholders::_1 ),
            std::bind( &CGfxUtil::ShouldParseOutputPath, &GetInstance(), placeholders::_1, placeholders::_2, placeholders::_3 ),
        },
    }};

//------------------------------------------------
//  Options Info
//------------------------------------------------

    /*
        Information on all the switches / options to allow the automated parser 
        to parse them.
    */
    const vector<optionparsing_t> CGfxUtil::Options_List=
    {{
        //Quiet
        {
            "q",
            0,
            "Disables console progress output.",
            "-q",
            std::bind( &CGfxUtil::ParseOptionQuiet, &GetInstance(), placeholders::_1 ),
        },
        //Set path to PMD2 Config file
        {
            "cfg",
            1,
            "Set a non-default path to the pmd2data.xml file.",
            "-cfg \"path/to/pmd2/config/data/file\"",
            std::bind( &CGfxUtil::ParseOptionConfig, &GetInstance(), placeholders::_1 ),
        },

        // Enable Logging
        {
            "log",
            0,
            "Enable logging to file.",
            "-log",
            std::bind( &CGfxUtil::ParseOptionLog, &GetInstance(), placeholders::_1 ),
        },
        //Image Format For Export
        {
            "f",
            1,
            "Set the image format to use when exporting images to either  raw, png, or bmp. Def: png.",
            "-f (png,bmp,raw)",
            std::bind( &CGfxUtil::ParseOptionExportFormat, &GetInstance(), placeholders::_1 ),
        },
        //Import images by index, or by alphanumeric order
        {
            "byindex",
            0,
            "This force the program to enforce the index in the file names of the images being imported, instead of just ordering them in alphanumeric order. This only applies to wan sprites this far!",
            "-byindex",
            std::bind( &CGfxUtil::ParseOptionImportByIndex, &GetInstance(), placeholders::_1 ),
        },
        //Set path to animres.xml file
        {
            "animres",
            1,
            "Sets the path to the animres file to use.",
            "-animres \"PathToFile\"",
            std::bind( &CGfxUtil::ParseOptionAnimResPath, &GetInstance(), placeholders::_1 ),
        },
        //Sets the path to the facenames file to use to name each images in each slots for all emotions in a pokemon's entry.
        {
            "fn",
            1,
            "Sets the path to the facenames file to use to name each images in each slots for all emotions in a pokemon's entry, in the kaomado.kao file.",
            "-fn \"PathToFile\"",
            std::bind( &CGfxUtil::ParseOptionFaceNamesPath, &GetInstance(), placeholders::_1 ),
        },
        //Sets the path to the pokemon name file to use to name each pokemon's entry in the kaomado.kao file.
        {
            "pn",
            1,
            "Sets the path to the pokemon name file to use to name each pokemon's entry in the kaomado.kao file.",
            "-pn \"PathToFile\"",
            std::bind( &CGfxUtil::ParseOptionPokeNamesPath, &GetInstance(), placeholders::_1 ),
        },
        //Sets the path to the pokemon name file to use to name each pokemon's entry in the packed pokemon sprites file.
        {
            "psprn",
            1,
            "Sets the path to the pokemon name file to use to name each pokemon's entry in the packed pokemon sprites file.",
            "-psprn \"PathToFile\"",
            std::bind( &CGfxUtil::ParseOptionPokeSprNamesPath, &GetInstance(), placeholders::_1 ),
        },
        //This compress the output, when re-building something. Only works with individual sprites and pack files containing sprites this far!
        {
            "pkdpx",
            0,
            "Force the outputed file to be compressed as a PKDPX! (If specified while buiding a pack file of sprite, the individual sprites in the pack will be compressed as PKDPX!)",
            "-pkdpx",
            std::bind( &CGfxUtil::ParseOptionCompressPKDPX,  &GetInstance(), placeholders::_1 ),
        },
        //Pack a directory containing sprite xml data
        {
            "p",
            0,
            "Force the content of the directory to be handled as a pack file to assemble from unpacked sprites in its sub-directories.",
            "-p",
            std::bind( &CGfxUtil::ParseOptionBuildPack,  &GetInstance(), placeholders::_1 ),
        },
        //Force the amount of worker threads to  use.
        {
            "th",
            1,
            "Force the amount of worker threads to  use. Works best when matches half of the machine's hardware threads.(Default is 2 thread)",
            "-th 6",
            std::bind( &CGfxUtil::ParseOptionNbThreads,  &GetInstance(), placeholders::_1 ),
        },
        //Ignore resolution mismatch!
        {
            "noresfix",
            0,
            "If specified the program will not automatically fix resolution mismatch when building (a) sprite(s) from a folder!",
            "-noresfix",
            std::bind( &CGfxUtil::ParseOptionNoResFix,  &GetInstance(), placeholders::_1 ),
        },


    //=====================
    //  New System
    //=====================
        //Force Export
        {
            "e",
            0,
            "(New system)If specified, the program will assume we're exporting graphics from the game into an external format.",
            "-e",
            std::bind( &CGfxUtil::ParseOptionForceExport,  &GetInstance(), placeholders::_1 ),
        },

        //Force Import
        {
            "i",
            0,
            "(New system)If specified, the program will assume we're importing graphics into the game.",
            "-i",
            std::bind( &CGfxUtil::ParseOptionForceImport,  &GetInstance(), placeholders::_1 ),
        },

        //BGP
        {
            "bgp",
            0,
            "Specifying this will force the program to export a BGP file to an image, or import an image file into a BGP.",
            "-bgp",
            std::bind( &CGfxUtil::ParseOptionBGP,  &GetInstance(), placeholders::_1 ),
        },

        //WAN
        {
            "wan",
            0,
            "Specifying this will force the program to export a wan/wat sprite to a directory, or import it from a directory.",
            "-wan",
            std::bind( &CGfxUtil::ParseOptionWAN,  &GetInstance(), placeholders::_1 ),
        },

        //Other formats here

    //=====================
    //  Automated tasks:
    //=====================
        //Pokemon Portraits
        {
            "pkportraits",
            0,
            "Specifying this will export or import pokemon portraits from/to the game. When importing the input is the directory "
            "containing all the portraits for each pokemons, and the output the ROM's data root directory. When exporting, the "
            "input is the ROM's data directory, and the output the directory where the converted portraits will be placed.",
            "-pkportraits",
            std::bind( &CGfxUtil::ParseOptionPkPortraits,  &GetInstance(), placeholders::_1 ),
        },

        //Pokemon Sprites
        {
            "pkspr",
            0,
            "Specifying this will cause the program to import or export pokemon sprites. When importing, the input is the directory containing the 3 subdirectories matching the 3 sprite files, and the output is the ROM's data root directory. When exporting, the input is the ROM's data root directory, and the output is the directory where the three sprite files will be exported to.",
            "-pkspr",
            std::bind( &CGfxUtil::ParseOptionPkSprites,  &GetInstance(), placeholders::_1 ),
        },

        ////Props/misc Sprites
        //{
        //    "propsprites",
        //    0,
        //    "Specifying this will cause the program to import or export props sprites. When importing, the input is the directory containing the sprites sub-directory to import, and the output is the ROM's data root directory. When exporting, the input is the ROM's data root directory, and the output is the directory where the sprites will be exported to.",
        //    "-propsprites",
        //    std::bind( &CGfxUtil::ParseOptionPropSprites,  &GetInstance(), placeholders::_1 ),
        //},
    }};

//------------------------------------------------
// Misc Methods
//------------------------------------------------

    /*
    */
    CGfxUtil & CGfxUtil::GetInstance()
    {
        static CGfxUtil s_util;
        return s_util;
    }

    /*
    */
    CGfxUtil::CGfxUtil()
        :CommandLineUtility()
    {
        _Construct();
    }

    /*
    */
    void CGfxUtil::_Construct()
    {
        clog <<getExeName() <<" " <<getVersionString() <<" initializing..\n";
        m_bQuiet        = false;
        m_ImportByIndex = false;
        m_bRedirectClog = false;
        m_bNoResAutoFix = false;
        m_execMode      = eExecMode::INVALID_Mode;
        m_PrefOutFormat = utils::io::eSUPPORT_IMG_IO::PNG;

        //m_inputCompletion    = 0;
        //m_outputCompletion   = 0;
        //m_bStopProgressPrint = false;

        //By default, use a single thread!
        unsigned int nbThreadsToUse = ( ( thread::hardware_concurrency() <= 2 )? 1 : 2 ); //Use 2 thread on system with more than 2 hardware threads
        utils::LibraryWide::getInstance().Data().setNbThreadsToUse( nbThreadsToUse ); 
        clog << "Using " <<nbThreadsToUse <<" thread(s).\n";

        //m_packPokemonNameList;
        m_pathToAnimNameResFile  = DefPathAnimRes;
        m_pathToPokeSprNamesFile = DefPathPokeSprNames;
        m_pathToFaceNamesFile    = DefPathFaceNames;
        m_pathToPokeNamesFile    = DefPathPokeNames;
        m_pmd2cfg                = pmd2::DefConfigFileName;

    }

    const vector<argumentparsing_t> & CGfxUtil::getArgumentsList   ()const { return Arguments_List;          }
    const vector<optionparsing_t>   & CGfxUtil::getOptionsList     ()const { return Options_List;            }
    const argumentparsing_t         * CGfxUtil::getExtraArg        ()const { return &ExtraArg;               }
    const string                    & CGfxUtil::getTitle           ()const { return Title;                   }
    const string                    & CGfxUtil::getExeName         ()const { return Exe_Name;                }
    const string                    & CGfxUtil::getVersionString   ()const { return Version;                 }
    const string                    & CGfxUtil::getShortDescription()const { return Short_Description;       }
    const string                    & CGfxUtil::getLongDescription ()const { return Long_Description;        }
    const string                    & CGfxUtil::getMiscSectionText ()const { return Misc_Text;               }


    /*
    */
    void CGfxUtil::ChkAndHndlUnsupportedRawOutput()
    {
        //Currently, we do not support raw image export on sprites !
        if( m_PrefOutFormat == utils::io::eSUPPORT_IMG_IO::RAW )
        {
            cerr << "<!>-WARNING: At this time, raw image output has been disabled because it was creating issues.\nWill export to PNG instead!\n";
            m_PrefOutFormat = utils::io::eSUPPORT_IMG_IO::PNG;
        }
    }

//--------------------------------------------
//  Mode Execution Methods
//--------------------------------------------

    /*
    */
    int CGfxUtil::UnpackSprite()
    {
        utils::MrChronometer chronounpacker( "Unpacking Sprite" );
        WAN_Parser           parser( ReadFileToByteVector( m_inputPath ) );
        Poco::File           infileinfo(m_inputPath);
        Poco::Path           inputPath(m_inputPath);
        Poco::Path           outpath;
        
        //Currently, we do not support raw image export on sprites !
        ChkAndHndlUnsupportedRawOutput();

        if( m_outputPath.empty() )
            m_outputPath = inputPath.parent().append(inputPath.getBaseName()).toString();

        outpath = m_outputPath;

        if( ! m_bQuiet )
        {
            if( ! m_bQuiet )
                cout << "\nPoochyena is so in sync with your wishes that she landed a critical hit!\n\n";
        }

        auto sprty = parser.getSpriteType();
        clog <<"Sprite is ";
        if( sprty == graphics::eSpriteImgType::spr4bpp )
        {
            clog <<"4 bpp\n";
            auto sprite = parser.ParseAs4bpp();
            graphics::ExportSpriteToDirectory( sprite, outpath.toString(), m_PrefOutFormat, false );
            
        }
        else if( sprty == graphics::eSpriteImgType::spr8bpp )
        {
            clog <<"8 bpp\n";
            auto sprite = parser.ParseAs8bpp();
            graphics::ExportSpriteToDirectory( sprite, outpath.toString(), m_PrefOutFormat, false );
        }

        //draw one last time
        if( ! m_bQuiet )
        {
            cout    << "\nIts super-effective!!\n"
                    << "\nThe sprite's copy got shred to pieces thanks to the critical hit!\n"
                    << "The pieces landed all neatly into \"" <<m_outputPath <<"\"!\n";
        }

        return 0;
    }

    /*
    */
    int CGfxUtil::BuildSprite()
    {
        utils::MrChronometer chronopacker( "Building Sprite" );
        Poco::File           infileinfo(m_inputPath);
        Poco::Path           outpath;//(m_outputPath);
        Poco::Path           inputPath(m_inputPath);
        uint32_t             level = (inputPath.depth() + ((( infileinfo.getSize() & 0xFF ) * 100) / 255) );

        if( m_outputPath.empty() )
            m_outputPath = inputPath.parent().append( Poco::Path(inputPath).makeFile().getBaseName() ).toString();

        outpath = m_outputPath;

        auto sprty = graphics::QuerySpriteImgTypeFromDirectory( infileinfo.path() );

        if( sprty == graphics::eSpriteImgType::spr4bpp )
        {
            auto sprite = graphics::ImportSpriteFromDirectory<SpriteData<gimg::tiled_image_i4bpp>>( infileinfo.path(), 
                                                                                                    m_ImportByIndex,
                                                                                                    false,
                                                                                                    nullptr,
                                                                                                    m_bNoResAutoFix );
            WAN_Writer writer( &sprite );

            if( m_compressToPKDPX )
            {
                vector<uint8_t> result  = writer.write();
                vector<uint8_t> outdata;
                CompressToPKDPX( result.begin(), result.end(), outdata );
                utils::io::WriteByteVectorToFile( outpath.setExtension(PKDPX_FILEX).toString(), outdata );
            }
            else
                writer.write( outpath.setExtension(WAN_FILEX).toString() );
            
        }
        else if( sprty == graphics::eSpriteImgType::spr8bpp )
        {
            auto sprite = graphics::ImportSpriteFromDirectory<SpriteData<gimg::tiled_image_i8bpp>>( infileinfo.path(), 
                                                                                                    m_ImportByIndex );
            WAN_Writer writer( &sprite );

            if( m_compressToPKDPX )
            {
                vector<uint8_t> result  = writer.write();
                vector<uint8_t> outdata;
                CompressToPKDPX( result.begin(), result.end(), outdata );
                utils::io::WriteByteVectorToFile( outpath.setExtension(PKDPX_FILEX).toString(), outdata );
            }
            else
                writer.write( outpath.setExtension(WAN_FILEX).toString() );
        }


        if( ! m_bQuiet )
        {
            cout << "\nIts super-effective!!\n"
                 <<"\"" <<inputPath.getFileName() <<"\" fainted!\n"
                 <<"You got \"" <<outpath.getFileName() <<"\" for your victory!\n";
        }

        return 0;
    }

    /*
    */
    int CGfxUtil::UnpackAndExportPackedCharSprites()
    {
        utils::ChronoRAII<chrono::seconds> chronounpacker( "Unpacking & Exporting Sprites" );
        Poco::File                   infileinfo(m_inputPath);
        Poco::Path                   inputPath( m_inputPath );
        Poco::Path                   outpath;
        future<void>                 updtProgress;
        atomic<bool>                 shouldUpdtProgress = true;
        multitask::CMultiTaskHandler taskmanager;
        atomic<uint32_t>             completed = 1;

        //Currently, we do not support raw image export on sprites !
        ChkAndHndlUnsupportedRawOutput();

        auto lambdaExpSpriteWrap = [&]( const graphics::BaseSprite * srcspr, const std::string & outpath )->bool
        {
            graphics::ExportSpriteToDirectoryPtr(srcspr, outpath, m_PrefOutFormat);
            ++completed;
            return true;
        };
        auto lambdaWriteFileByVec = [&completed](const std::string & path, const std::vector<uint8_t> & filedata)->bool
        {
            utils::io::WriteByteVectorToFile( path, filedata );
            ++completed;
            return true;
        };

        
        if( m_outputPath.empty() )
            m_outputPath = inputPath.parent().append( Poco::Path(inputPath).makeFile().getBaseName() ).toString();

        outpath = m_outputPath;

        try
        {
            //#1 - Check if its one of the 3, uniquely named, special pack file. If not issue a warning, and continue.
            bool           isPokeSpriteFile = MatchesPokeSpritePackFileName( inputPath.getBaseName() );
            vector<string> pokesprnames;

            if( isPokeSpriteFile )
            {
                cout << "<!>-This pack is named after one of the three that contains pokemon sprite files!\n\tNaming sub-folders based on the content of \"" 
                     <<m_pathToPokeSprNamesFile <<"\"!\n";
                pokesprnames = utils::io::ReadTextFileLineByLine( m_pathToPokeSprNamesFile );
            }
            else
            {
                cout << "<!>-This file may or may not be a pack file that contains pokemon sprites!\n\tSub-folders won't be named!\n";
            }

            //#2 - Unpack files to raw data vector.
            auto inpack = UnpackPackFile( inputPath );

            //#3 - Run a check to find files that must be decompressed. And decompress them on the spot, replacing the raw data in the vector.
            vector<unique_ptr<graphics::BaseSprite>> mysprites(inpack.getNbSubFiles());
            TurnPackContentToSpriteData( inpack, mysprites );

            //Create output directory
            Poco::File outdir( outpath );
            if( ! outdir.exists() )
                outdir.createDirectory();

            //#4 - Run the sprite export code on every sprites, and export to output folder in its own named sub-folder.
            //     Use the pokemon name list if its one of the 3 special files.
            cout<<"\nWriting sprites to directories..\n";

            for( unsigned int i = 0; i < mysprites.size(); )
            {
                if( mysprites[i] == nullptr )
                {
                    //Output the packed file's content as is
                    auto & cursubf = inpack.getSubFile(i);
                    stringstream sstr;
                    sstr << inputPath.getBaseName()
                         <<"_" <<setw(4) <<setfill('0') <<i <<"." 
                         << GetAppropriateFileExtension( cursubf.begin(), cursubf.end() );                   

                    taskmanager.AddTask( 
                        multitask::pktask_t( 
                            std::bind(lambdaWriteFileByVec, Poco::Path(outpath).append(sstr.str()).toString(), std::ref(inpack.getSubFile(i)) ) ) );
                }
                else 
                {
                    //Build the sub-folder name
                    stringstream sstr;
                    BaseSprite * curspr = mysprites[i].get();

                    if( isPokeSpriteFile && pokesprnames.size() > i )
                    {
                        sstr <<setw(4) <<setfill('0') <<i <<"_" << pokesprnames[i];
                    }
                    else
                    {
                        sstr << inputPath.getBaseName()
                             <<"_" <<setw(4) <<setfill('0') <<i;   
                    }

                    taskmanager.AddTask( 
                        multitask::pktask_t( 
                            std::bind(lambdaExpSpriteWrap, (mysprites[i].get()), Poco::Path(outpath).append(sstr.str()).toString() ) ) );
                }
                ++i;
            }

            updtProgress = std::async( std::launch::async, PrintProgressLoop, std::ref(completed), mysprites.size(), std::ref(shouldUpdtProgress) );
            
            taskmanager.Execute();
            taskmanager.BlockUntilTaskQueueEmpty();
            taskmanager.StopExecute();

            shouldUpdtProgress = false;
            if( updtProgress.valid() )
                updtProgress.get();

            cout<<"\r100%";
        }
        catch( Poco::Exception e )
        {
            shouldUpdtProgress = false;
            if( updtProgress.valid() )
                updtProgress.get();

            //rethrow
            throw e;
        }
        catch( exception e )
        {
            shouldUpdtProgress = false;
            if( updtProgress.valid() )
                updtProgress.get();

            //rethrow
            throw e;
        }
        cout<<"\n";
        return 0;
    }

    /*
    */
    void BuildSprFromDirAndInsert( vector<uint8_t> & out_sprRaw, 
                                  const Poco::Path & inDirPath, 
                                  bool               importByIndex, 
                                  bool               bShouldCompress,
                                  bool               bNoResAutoFix )
    {
        auto sprty = graphics::QuerySpriteImgTypeFromDirectory( inDirPath.toString() );

        if( sprty == graphics::eSpriteImgType::spr4bpp )
        {
            auto sprite = graphics::ImportSpriteFromDirectory<SpriteData<gimg::tiled_image_i4bpp>>( inDirPath.toString(), 
                                                                                                    importByIndex, 
                                                                                                    false,
                                                                                                    nullptr,
                                                                                                    bNoResAutoFix );
            WAN_Writer writer( &sprite );
            if( bShouldCompress )
            {
                auto filedata = writer.write();
                CompressToPKDPX( filedata.begin(), filedata.end(), out_sprRaw, ::compression::ePXCompLevel::LEVEL_3, true );
            }
            else
                out_sprRaw = writer.write();
            
        }
        else if( sprty == graphics::eSpriteImgType::spr8bpp )
        {
            auto sprite = graphics::ImportSpriteFromDirectory<SpriteData<gimg::tiled_image_i8bpp>>( inDirPath.toString(), 
                                                                                                    importByIndex, 
                                                                                                    false,
                                                                                                    nullptr,
                                                                                                    bNoResAutoFix );
            WAN_Writer writer( &sprite );
            if( bShouldCompress )
            {
                auto filedata = writer.write();
                CompressToPKDPX( filedata.begin(), filedata.end(), out_sprRaw );
            }
            else
                out_sprRaw = writer.write();
        }
    }

    /*
    */
    int CGfxUtil::PackAndImportCharSprites()
    {
        utils::ChronoRAII<std::chrono::seconds> chronopacker( "Packing & Importing Sprites" );
        Poco::File                   infileinfo(m_inputPath);
        Poco::Path                   inpath(m_inputPath);
        Poco::Path                   outpath;
        Poco::DirectoryIterator      itDirCount( infileinfo );
        Poco::DirectoryIterator      itDirEnd;
        CPack                        mypack;
        vector<Poco::File>           validDirs;
        future<void>                 updtProgress;
        atomic<bool>                 shouldUpdtProgress = true;
        multitask::CMultiTaskHandler taskmanager;
        atomic<uint32_t>             completed = 0;

        auto lambdaWrapBuildSpr = [&]( vector<uint8_t> & out_sprRaw, const Poco::File & infile, bool importByIndex, bool bShouldCompress )->bool
        {
            BuildSprFromDirAndInsert(out_sprRaw, infile.path(), importByIndex, bShouldCompress, m_bNoResAutoFix);
            ++completed;
            return true;
        };

        //Count valid directories
        cout <<"\nGathering valid sprite sub-directories...\n";
        for(uint32_t nbfound = 0; itDirCount != itDirEnd; ++itDirCount )
        {
            if( itDirCount->isDirectory() )
            {
                if( IsFolderASpriteFolder( itDirCount->path() ) )
                {
                    validDirs.push_back( *itDirCount );
                    ++nbfound;
                    cout <<"\r" <<nbfound <<"..";
                }
                else
                    cerr << "Directory \"" <<itDirCount->path() <<"\" will be ignored! Its missing content to build a sprite from it!\n";
            }
        }
        cout <<"\rFound " <<validDirs.size() <<" valid sprites sub-directories!\n";

        cout <<"\nReading sprite data...\n";
        //Resize the file container
        mypack.SubFiles().resize( validDirs.size() );


        //if( m_ImportByIndex )
        //{
        //    //Iterate the directory's content and get only the extracted sprites
        //    for( unsigned int i = 0; i < validDirs.size(); ++i )
        //    {
        //        Poco::File & curDir = validDirs[i];

        //        uint32_t     insertat = 0;
        //        stringstream sstrindex;
        //        sstrindex << (Poco::Path( curDir.path() ).makeFile().getFileName()); //Get the name of the directory
        //        sstrindex >> insertat;  //Extract the index # prefix from the folder name!

        //        if( insertat < validDirs.size() )
        //        {
        //            taskmanager.AddTask(
        //                multitask::pktask_t(
        //                std::bind( lambdaWrapBuildSpr, ref(mypack.SubFiles()[insertat]), ref(validDirs[i]), m_ImportByIndex, m_compressToPKDPX ) ) );
        //        }
        //        else
        //            cerr<< "<!>-Warning file \"" <<curDir.path() <<"\" has an index number higher than the amount of sprite folders to pack!\nSkipping!";
        //    }
        //}
        //else
        //{
            //Iterate the directory's content and get only the extracted sprites
            for( unsigned int i = 0; i < validDirs.size(); ++i )
            {
                Poco::File & curDir = validDirs[i];
                taskmanager.AddTask(
                    multitask::pktask_t(
                    std::bind( lambdaWrapBuildSpr, ref(mypack.SubFiles()[i]), ref(validDirs[i]), m_ImportByIndex, m_compressToPKDPX ) ) );
            }
        //}

        try
        {
            updtProgress = std::async( std::launch::async, PrintProgressLoop, std::ref(completed), validDirs.size(), std::ref(shouldUpdtProgress) );
            taskmanager.Execute();
            taskmanager.BlockUntilTaskQueueEmpty();
            taskmanager.StopExecute();

            shouldUpdtProgress = false;
            if( updtProgress.valid() )
                updtProgress.get();
            cout<<"\r100%"; //Can't be bothered to make another drawing update
        }
        catch( Poco::Exception & )
        {
            shouldUpdtProgress = false;
            if( updtProgress.valid() )
                updtProgress.get();

            //rethrow
            rethrow_exception( current_exception() );
        }
        catch( exception & )
        {
            shouldUpdtProgress = false;
            if( updtProgress.valid() )
                updtProgress.get();

            //rethrow
            rethrow_exception( current_exception() );
        }
        cout<<"\n";

        //Don't forget to force the starting offset to this 
        mypack.setForceFirstFilePosition( ForcedPokeSpritePack );

        //If we don't have an output path, use the input path's parent, and create a file with the same name as the folder!
        if( m_outputPath.empty() )
            m_outputPath = Poco::Path(inpath).makeFile().setExtension(PACK_FILEX).toString();

        outpath = m_outputPath;

        string outfilepath = outpath.toString();
        cout <<"\n\nBuilding \"" <<outfilepath <<"\"...\n";
        utils::io::WriteByteVectorToFile( outfilepath, mypack.OutputPack() );
        cout <<"\nDone!\n";

        return 0;
    }

    /*
    */
    int CGfxUtil::DecompressAndHandle()
    {
        utils::MrChronometer chronounpacker( "Unpacking compressed sprite" );
        vector<uint8_t>      rawCompFile = ReadFileToByteVector( m_inputPath );
        Poco::File           infileinfo(m_inputPath);
        Poco::Path           inputPath(m_inputPath);
        Poco::Path           outpath;

        //Currently, we do not support raw image export on sprites !
        ChkAndHndlUnsupportedRawOutput();

        if( m_outputPath.empty() )
            m_outputPath = inputPath.parent().append(inputPath.getBaseName()).toString();

        outpath = m_outputPath;

        if( ! m_bQuiet )
        {
            if( ! m_bQuiet )
                cout << "\nPoochyena is so in sync with your wishes that she landed a critical hit!\n\n";
        }

        //Decompress
        vector<uint8_t> decompBuf; 
        DecompressPKDPX( rawCompFile.begin(), rawCompFile.end(), decompBuf );

        //Analyse content
        auto result = DetermineCntTy( decompBuf.begin(), decompBuf.end() );

        if( result._type != CnTy_WAN )
            throw runtime_error("ERROR: the compressed file doesn't contain a WAN sprite!");

        //Then handle the sprite!
        std::unique_ptr<graphics::BaseSprite> targetptr;
        ParseASprite( decompBuf, targetptr );

        //Write it out
        graphics::ExportSpriteToDirectoryPtr( targetptr.get(), outpath.toString(), m_PrefOutFormat );

        //write output message
        if( ! m_bQuiet )
        {
            cout    << "\nIts super-effective!!\n"
                    << "\nThe sprite's copy got shred to pieces thanks to the critical hit!\n"
                    << "The pieces landed all neatly into \"" <<m_outputPath <<"\"!\n";
        }

        return 0;
    }

    /*
    */
    int CGfxUtil::ImportMainFontFile()
    {
        vector<uint8_t> fdata = ReadFileToByteVector( m_inputPath );

        assert(false);

        return 0;
    }
    
    /*
    */
    int CGfxUtil::ExportMainFontFile()
    {
        assert(false);
        return 0;
    }

//--------------------------------------------
//  Parsing Args Methods
//--------------------------------------------

    /*
    */
    bool CGfxUtil::ParseInputPath( const string & path )
    {
        Poco::File inputfile(path);

        //check if path exists
        if( inputfile.exists() )
        {
            m_inputPath = path;
            return true;
        }
        return false;
    }
    
    /*
    */
    bool CGfxUtil::ParseOutputPath( const string & path )
    {
        Poco::Path outpath(path);

        if( outpath.isDirectory() ||  outpath.isFile() )
        {
            m_outputPath = path;
            return true;
        }
        return false;
    }

    /*
    */
    bool CGfxUtil::ShouldParseOutputPath( const std::vector<std::vector<std::string>> & optdata, 
                                          const std::deque<std::string>               & priorparam, 
                                          size_t                                        nblefttoparse )
    {

        return (priorparam.size() == 1) && (nblefttoparse != 0);
    }

    /*
    */
    bool CGfxUtil::ParseExtraPath( const string & path )
    {
        Poco::Path testpath(path);
        if( testpath.isDirectory() ||  testpath.isFile() )
        {
            m_extraargs.push_back(path);
            return true;
        }
        return false;
    }

    /*
    */
    void CGfxUtil::PrintOperationMode()
    {
        if( m_bQuiet )
            return;

        cout <<"\n--------------------------------------\n";
        switch( m_execMode )
        {
            case eExecMode::BUILD_WAN_Mode:
            {
                cout <<"Building WAN Sprite\n";
                break;
            }
            case eExecMode::UNPACK_WAN_Mode:
            {
                cout <<"Unpacking WAN Sprite\n";
                break;
            }
            case eExecMode::UNPACK_POKE_SPRITES_PACK_Mode:
            {
                cout <<"Unpacking Sprites-Containing Pack File\n";
                break;
            }
            case eExecMode::BUILD_POKE_SPRITES_PACK_Mode:
            {
                cout <<"Building Sprites-Containing Pack File\n";
                break;
            }
            case eExecMode::DECOMPRESS_AND_INDENTIFY_Mode:
            {
                cout <<"Decompressing and Unpacking Sprite\n";
                break;
            }
            case eExecMode::EXPORT_BGP_Mode:
            {
                cout <<"Exporting BGP image!\n";
                break;
            }
            case eExecMode::IMPORT_BGP_Mode:
            {
                cout <<"Importing BGP image!\n";
                break;
            }
            case eExecMode::INVALID_Mode:
            {
                cout <<"INVALID\n";
                break;
            }
            default:
            {
                cout <<"ERROR\n";
            }
        };
        cout <<"--------------------------------------\n";
    }

    bool CGfxUtil::DetermineOperationMode()
    {
        Poco::File theinput( m_inputPath );
        Poco::Path inputPath(m_inputPath);

        //If an operation mode was forced, don't try to determine what to do !
        if( m_execMode != eExecMode::INVALID_Mode )
            return true;

        if( theinput.isFile() )
        {
            //Working on a file
            vector<uint8_t> tmp    = utils::io::ReadFileToByteVector(theinput.path());
            auto            result = DetermineCntTy(tmp.begin(), tmp.end(), inputPath.getExtension());

            if( result._type == CnTy_WAN )
            {
                m_execMode = eExecMode::UNPACK_WAN_Mode;
            }
            else if( result._type == CnTy_PackFile )
            {
                m_execMode = eExecMode::UNPACK_POKE_SPRITES_PACK_Mode;
            }
            else if( result._type == CnTy_AT4PX )
            {
                if( inputPath.getExtension() == BGP_Ext )
                    m_execMode = eExecMode::EXPORT_BGP_Mode;
                else
                    throw exception("Raw AT4PX compressed files not supported at this time!");
                //m_execMode = eExecMode::DECOMPRESS_AND_INDENTIFY_Mode;
            }
            else if( result._type == CnTy_PKDPX )
            {
                m_execMode = eExecMode::DECOMPRESS_AND_INDENTIFY_Mode;
            }
            else if( result._type == CnTy_WTE )
            {
                m_execMode = eExecMode::EXPORT_WTE_Mode;
            }
            else
            {
                m_execMode = eExecMode::INVALID_Mode;
                cerr << "<!>-No ideas what to do with that input parameter ^^;\n";
                return false;
            }

        }
        else if( theinput.isDirectory() )
        {
            //Working on a directory
            //Check the content and find out what to do

            //If the folder name matches one of the 3 special sprite pack file names
            if( MatchesPokeSpritePackFileName( inputPath.getBaseName() ) )
            {
                m_execMode        = eExecMode::BUILD_POKE_SPRITES_PACK_Mode;
                m_compressToPKDPX = MatchesCompressedPokeSpritePackFileName( inputPath.getBaseName() );

                if( !m_bQuiet )
                {
                    cout << "<*>-Input folder name matches the name of one of the pokemon sprites pack file!\n"
                         << "<*>-Preparing to convert all sprites directories in the input directory\n"
                         << "    into WAN sprites, and packing them into a pack file!\n";
                
                    if( m_compressToPKDPX )
                    {
                        cout <<"<*>-Input directory name matches compressed pokemon sprite pack\n" 
                             <<"    file's name. Enabling compression!\n";
                    }
                }

                return true;
            }
            
            //Otherwise, analyse the content of the folder!
            vector<string> folderContent = utils::ListDirContent_FilesAndDirs( theinput.path(), true );

            if( AreReqFilesPresent_Sprite( folderContent ) )
            {
                //We got all we need to build a sprite !
                m_execMode = eExecMode::BUILD_WAN_Mode;
                if( !m_bQuiet )
                    cout << "<*>-Required files to build a sprite found! Get the duct tape ready! We're building a sprite!\n";
            }
            else
            {
                vector<string> missingfiles = GetMissingRequiredFiles_Sprite( folderContent );
                m_execMode = eExecMode::INVALID_Mode;
                cerr << "<!>-This directory doesn't contain all the required files to build a sprite!\n"
                     << "<!>-The " <<missingfiles.size() <<" missing file(s) are :\n";
                for( const auto & afile : missingfiles )
                {
                    cerr << afile <<"\n";
                }
                return false;
            }

        }
        else
        {
            //Unknown..
            m_execMode = eExecMode::INVALID_Mode;
            if( !m_bQuiet )
                cerr << "<!>-No ideas what to do with that input parameter ^^;\n";
            return false;
        }

        return true;
    }


    bool CGfxUtil::DetermineOperationModeNew()
    {
        Poco::File infile(m_inputPath);
        Poco::Path inpath(m_inputPath);

        if( !infile.exists() )
        {
            return false;
        }

        if( infile.isFile() )
        {
        }
        else if( infile.isDirectory() )
        {
        }

        return false;
    }

//--------------------------------------------
//  Parsing Options Methods
//--------------------------------------------
    bool CGfxUtil::ParseOptionQuiet( const vector<string> & optdata )
    {
        //If this is called, we don't need to do any additional validation!
        return m_bQuiet = true;
    }

    bool CGfxUtil::ParseOptionExportFormat( const std::vector<std::string> & optdata )
    {
        //First entry is the option symbol
        if( optdata.size() > 1 )
        {
            cout <<"<*>-Forcing export format to ";
            if( optdata[1].compare( "raw" ) == 0 )
            {
                m_PrefOutFormat = utils::io::eSUPPORT_IMG_IO::RAW;
                cout <<"raw image(s)";
            }
            else if( optdata[1].compare( utils::io::BMP_FileExtension ) == 0 )
            {
                m_PrefOutFormat = utils::io::eSUPPORT_IMG_IO::BMP;
                cout <<".bmp image(s)";
            }
            else
            {
                //Fallback to png
                m_PrefOutFormat = utils::io::eSUPPORT_IMG_IO::PNG;
                cout <<".png image(s)";
            }
            cout <<"!\n";
            return true;
        }

        return false;
    }

    //bool CGfxUtil::ParseOptionForceInputFormat( const std::vector<std::string> & optdata )
    //{

    //    if( optdata.size() == 2 )
    //    {
    //        auto result = GetFileTypeFromExtension( optdata.back() );
    //        auto type   = result.front();

    //        cout << "<*>-Forcing input as ";

    //        if( type == CnTy_WAN )
    //        {
    //            m_execMode = eExecMode::UNPACK_WAN_Mode;
    //            cout <<"WAN sprite";
    //        }
    //        else if( type == CnTy_Kaomado )
    //        {
    //            m_execMode = eExecMode::UNPACK_KAOMADO_Mode;
    //            cout <<"kaomado.kao file";
    //        }
    //        else if( type == CnTy_PackFile )
    //        {
    //            m_execMode = eExecMode::UNPACK_POKE_SPRITES_PACK_Mode;
    //            cout <<"pokemon sprites containing pack file";
    //        }
    //        else if( type == CnTy_WTE )
    //        {
    //            m_execMode = eExecMode::EXPORT_WTE_Mode;
    //            cout <<"WTE file";
    //        }
    //        else if( type == CnTy_BGP )
    //        {
    //            m_execMode = eExecMode::EXPORT_BGP_Mode;
    //            cout <<"BGP file";
    //        }
    //        else if( type == CnTy_AT4PX || type == CnTy_PKDPX )
    //        {
    //            //Ambiguous !!!!
    //            m_execMode = eExecMode::DECOMPRESS_AND_INDENTIFY_Mode;
    //            cout <<"AT4PX/PKDPX file";
    //        }
    //        else
    //        {
    //            cout <<"INVALID!\n";
    //            return false;
    //        }
    //        cout <<"!\n";
    //        return true;
    //    }
    //    return false;
    //}

    bool CGfxUtil::ParseOptionImportByIndex( const std::vector<std::string> & optdata )
    {
        cout <<"<*>-Importing images by index instead of by alphanumeric order !\n";
        return (m_ImportByIndex = true);
    }

    bool CGfxUtil::ParseOptionAnimResPath( const std::vector<std::string> & optdata )
    {
        bool result = ParseOptionFilePath( optdata, m_pathToAnimNameResFile );
        cout <<"<*>-Changed lookup path for animation handling resources to \"" <<m_pathToAnimNameResFile <<"\"!\n";
        return result;
    }

    bool CGfxUtil::ParseOptionFaceNamesPath( const std::vector<std::string> & optdata )
    {
        bool result = ParseOptionFilePath( optdata, m_pathToFaceNamesFile );
        cout <<"<*>-Changed lookup path for kaomado.kao face name list to \"" <<m_pathToFaceNamesFile <<"\"!\n";
        return result;
    }
    
    bool CGfxUtil::ParseOptionPokeNamesPath( const std::vector<std::string> & optdata )
    {
        bool result = ParseOptionFilePath( optdata, m_pathToPokeNamesFile );
        cout <<"<*>-Changed lookup path for kaomado.kao pokemon name list to \"" <<m_pathToPokeNamesFile <<"\"!\n";
        return result;
    }
    
    bool CGfxUtil::ParseOptionPokeSprNamesPath( const std::vector<std::string> & optdata )
    {
        bool result = ParseOptionFilePath( optdata, m_pathToPokeSprNamesFile );
        cout <<"<*>-Changed lookup path for pokemon sprite name list to \"" <<m_pathToPokeSprNamesFile <<"\"!\n";
        return result;
    }

    bool CGfxUtil::ParseOptionCompressPKDPX( const std::vector<std::string> & optdata )
    {
        cout <<"<*>-Forcing compressiong to PKDPX!\n";
        return m_compressToPKDPX = true;
    }

    bool CGfxUtil::ParseOptionBuildPack( const std::vector<std::string> & optdata )
    {
        cout <<"<*>-Forcing rebuilding from directory to a pokemon sprite containing pack file!\n";
        m_execMode = eExecMode::BUILD_POKE_SPRITES_PACK_Mode;
        return true;
    }

    bool CGfxUtil::ParseOptionNbThreads( const std::vector<std::string> & optdata )
    {
        if( optdata.size() == 2 )
        {
            unsigned int nbthreads = stoul(optdata.back());
            cout<<"<*>-Thread count set to " <<nbthreads <<" worker thread(s)!\n";
            utils::LibraryWide::getInstance().Data().setNbThreadsToUse( nbthreads );
            return true;
        }
        else
            return false;
    }

    bool CGfxUtil::ParseOptionLog( const std::vector<std::string> & optdata )
    {
        cout <<"<*>-Logging enabled!\n";
        utils::LibWide().isLogOn(true);
        return m_bRedirectClog = true;
    }

    bool CGfxUtil::ParseOptionNoResFix( const std::vector<std::string> & optdata )
    {
        cout <<"<*>-noresfix specified. Utility will not attempt to get correct resolution from the images in case of mismatch. Data from the XML file will be forced!\n";
        return m_bNoResAutoFix = true;
    }


    //New System
    bool CGfxUtil::ParseOptionForceExport( const std::vector<std::string> & optdata )
    {
        m_Export = true;
        m_Import = false;
        return true;
    }

    bool CGfxUtil::ParseOptionForceImport( const std::vector<std::string> & optdata )
    {
        m_Export = false;
        m_Import = true;
        return true;
    }

    bool CGfxUtil::ParseOptionBGP( const std::vector<std::string> & optdata )
    {
        m_GameFmt = eFMT::BGP;
        return true;
    }

    bool CGfxUtil::ParseOptionWAN( const std::vector<std::string> & optdata )
    {
        m_GameFmt = eFMT::WAN;
        return true;
    }

    bool CGfxUtil::ParseOptionPkPortraits( const std::vector<std::string> & optdata )
    {
        m_doPkKao = true;
        return true;
    }

    bool CGfxUtil::ParseOptionPkSprites( const std::vector<std::string> & optdata )
    {
        m_doPkSpr = true;
        return true;
    }

    bool CGfxUtil::ParseOptionPropSprites( const std::vector<std::string> & optdata )
    {
        m_doPropSpr = true;
        return true;
    }

    bool CGfxUtil::ParseOptionConfig( const std::vector<std::string> & optdata )
    {
        if( optdata.size() > 1 )
        {
            if( utils::isFile( optdata[1] ) )
            {
                m_pmd2cfg = optdata[1];
                cout << "<!>- Set \"" <<optdata[1]  <<"\" as path to pmd2data file!\n";
            }
            else
                throw runtime_error("New path to pmd2data file does not exists, or is inaccessible!");
        }
        return true;
    }


//--------------------------------------------
//  Main Exec functions
//--------------------------------------------

    /*
    */
    int CGfxUtil::Execute()
    {
        int returnval = RETVAL_GenericFail;
        try
        {
            Poco::Path inpath(m_inputPath);

            //Determine Execution mode
            if( !m_Import && !m_Export )
            {
                //Old Method
                if( !DetermineOperationMode() )
                    return RETVAL_GenericFail;
            }

            PrintOperationMode();

            if( ! m_bQuiet )
                cout << "\nPoochyena used Crunch on \"" <<inpath.getFileName() <<"\"!\n";

            if( m_bRedirectClog )
                m_redirectClog.Redirect( inpath.getBaseName() + ".log" );

            //Init or not the chrono for the log file
            unique_ptr<utils::ChronoRAII<>> chronototal = nullptr;
            if(utils::LibWide().isLogOn())
                chronototal.reset( new utils::ChronoRAII<>("Total Execution Time", &clog ) );

            if( m_Import || m_Export )
                returnval = ExecNew();
            else
                returnval = ExecOld();

            if( ! m_bQuiet && returnval == 0 )
                cout << "\n\nPoochyena used Rest! ...zZz..zZz...\n";
        }
        catch( Poco::Exception & pex )
        {
            cerr <<"\n" << "<!>-POCO Exception - " <<pex.name() <<"(" <<pex.code() <<") : " << pex.message() <<endl;
            returnval = pex.code();
        }
        catch( exception & e )
        {
            cerr <<"\n" 
                 <<"\nWelp.. Poochyena almost choked while crunching the pixels!\n<she gave you an apologetic look>\n"
                 << "Exception : " <<e.what() <<endl;
        }
        return returnval;
    }

    /*
    */
    int CGfxUtil::ExecOld()
    {
        int returnval = RETVAL_GenericFail;

        switch( m_execMode )
        {
            case eExecMode::BUILD_WAN_Mode:
            {
                if( utils::LibWide().isLogOn() )
                    clog <<"Building WAN sprite..\n";
                returnval = BuildSprite();
                break;
            }
            case eExecMode::UNPACK_WAN_Mode:
            {
                if( utils::LibWide().isLogOn() )
                    clog <<"Unpacking WAN sprite..\n";
                returnval = UnpackSprite();
                break;
            }
            case eExecMode::DECOMPRESS_AND_INDENTIFY_Mode:
            {
                if( utils::LibWide().isLogOn() )
                    clog <<"Encountered compressed container, decompressing and identifying!..\n";
                returnval = DecompressAndHandle();
                break;
            }
            case eExecMode::UNPACK_POKE_SPRITES_PACK_Mode:
            {
                if( utils::LibWide().isLogOn() )
                    clog <<"Unpacking sprite pack file..\n";
                returnval = UnpackAndExportPackedCharSprites();
                break;
            }
            case eExecMode::BUILD_POKE_SPRITES_PACK_Mode:
            {
                if( utils::LibWide().isLogOn() )
                    clog <<"Building sprite pack file..\n";
                returnval = PackAndImportCharSprites();
                break;
            }

            case eExecMode::IMPORT_MAINFONT:
            {
                if( utils::LibWide().isLogOn() )
                    clog <<"Importing main font file..\n";
                returnval = ImportMainFontFile();
                break;
            }

            case eExecMode::EXPORT_BGP_Mode:
            {
                if( utils::LibWide().isLogOn() )
                    clog <<"Exporting bgp file..\n";
                m_Export = true; //Gotta force this
                HandleBGP();
                returnval = 0;
                break;
            }

            case eExecMode::IMPORT_BGP_Mode:
            {
                if( utils::LibWide().isLogOn() )
                    clog <<"Exporting bgp file..\n";
                m_Import = true; //Gotta force this
                HandleBGP();
                returnval = 0;
                break;
            }

            case eExecMode::INVALID_Mode:
            {
                cerr<<"<!>-ERROR  : Nothing can be done here. Exiting..\n";
                if( utils::LibWide().isLogOn() )
                    clog<<"<!>- Got invalid operation mode!\n";
                returnval = RETVAL_InvalidOp;
                break;
            }
            default:
            {
                cerr<<"<!>-ERROR  : Unknown operation! Possibly an unimplemented feature! Exiting..\n";
                if( utils::LibWide().isLogOn() )
                    clog<<"<!>-Unimplemented operation mode!\n";
                returnval = RETVAL_InvalidOp;
            }
        };

        return returnval;
    }

    /*
    */
    int CGfxUtil::ExecNew()
    {
        int returnval = RETVAL_GenericFail;

        if( m_doPkKao )
        {
            if( m_Import )
            {
                DoImportPortraits();
                returnval = 0;
            }
            else if( m_Export )
            {
                DoExportPortraits();
                returnval = 0;
            }
        }

        if( m_doPkSpr )
        {
            if( m_Import )
            {
                DoImportPokeSprites();
                returnval = 0;
            }
            else if( m_Export )
            {
                DoExportPokeSprites();
                returnval = 0;
            }
        }

        if( m_doPropSpr )
        {
            if( m_Import )
            {
                DoImportMiscSprites();
                returnval = 0;
            }
            else if( m_Export )
            {
                DoExportMiscSprites();
                returnval = 0;
            }
        }

        //Handle specific content
        switch(m_GameFmt)
        {
            case eFMT::BGP:
            {
                HandleBGP();
                returnval = 0;
                break;
            }
            case eFMT::WAN:
            {
                HandleWAN();
                returnval = 0;
                break;
            }
            case eFMT::WTE:
            {
                HandleWTE();
                returnval = 0;
                break;
            }

            case eFMT::INVALID:
            default:
            {
                throw runtime_error("CGfxUtil::ExecNew(): Unsupported format got through validation!");
            }
        };

        return returnval;
    }

    /*
    */
    int CGfxUtil::GatherArgs( int argc, const char * argv[] )
    {
        if( argc == 1 )
        {
            PrintReadme();
            return -1;
        }

        //Parse arguments and options
        try
        {
            //SetArguments returns false, when there are no args to parse !
            SetArguments(argc,argv);

            //Log all arguments
            if( utils::LibWide().isLogOn() )
            {
                clog <<"Got " <<argc <<" argument(s):\n";
                for( int i = 0; i < argc; ++i )
                    clog <<argv[i] <<"\n";
                clog <<"\n";
            }

            Poco::Path inpath(m_inputPath);

            if( ! m_bQuiet )
            {
                cout << "\"" <<inpath.getFileName() <<"\" wants to battle!\n"
                     << "Poochyena can't wait to begin!\n";
            }
        }
        catch( const Poco::Exception & pex )
        {
            cerr <<"\n" << "<!>-POCO Exception - " <<pex.name() <<"(" <<pex.code() <<") : " << pex.message() <<endl;
            if( utils::LibWide().isLogOn() )
                clog <<"\n" << "<!>-POCO Exception - " <<pex.name() <<"(" <<pex.code() <<") : " << pex.message() <<endl;
            PrintReadme();
            return pex.code();
        }
        catch( const exception & e )
        {
            cerr <<"\n" 
                 <<"\nWelp.. Poochyena hit herself in confusion while biting through the parameters!\nShe's in a bit of a pinch. It looks like she might cry...\n"
                 << e.what() <<endl;
            if( utils::LibWide().isLogOn() )
                clog <<"\n<!>-Exception: " << e.what() <<endl;
            PrintReadme();
            return RETVAL_BadArg;
        }
        return 0;
    }


//--------------------------------------------
//  New System
//--------------------------------------------
    const std::string DefLangConfFile = "gamelang.xml";
    const std::string DefOutKaoDir    = "portraits";
    const std::string FONT_Dir        = "FONT";
    const std::string KaoFName        = "kaomado.kao";
    const std::string Monster_Dir     = "MONSTER";


    void ExportASpritePackFile( const std::string & fpath, const std::string & outdir, utils::io::eSUPPORT_IMG_IO imgty, const std::vector<string> & pokesprnames )
    {
        future<void>                 updtProgress;
        atomic<bool>                 shouldUpdtProgress = true;
        multitask::CMultiTaskHandler taskmanager;
        atomic<uint32_t>             completed = 1;
        Poco::Path inputPath(fpath);

        try
        {
            //Unpack files to raw data vector.
            auto inpack = UnpackPackFile( fpath );

            //Run a check to find files that must be decompressed. And decompress them on the spot, replacing the raw data in the vector.
            vector<unique_ptr<graphics::BaseSprite>> mysprites(inpack.getNbSubFiles());
            TurnPackContentToSpriteData( inpack, mysprites );

            auto lambdaExpSpriteWrap = [&]( const graphics::BaseSprite * srcspr, const std::string & outpath )->bool
            {
                graphics::ExportSpriteToDirectoryPtr(srcspr, outpath, imgty);
                ++completed;
                return true;
            };
            auto lambdaWriteFileByVec = [&completed](const std::string & path, const std::vector<uint8_t> & filedata)->bool
            {
                utils::io::WriteByteVectorToFile( path, filedata );
                ++completed;
                return true;
            };


            //#4 - Run the sprite export code on every sprites, and export to output folder in its own named sub-folder.
            //     Use the pokemon name list if its one of the 3 special files.
            cout<<"\nWriting sprites to directories..\n";

            for( unsigned int i = 0; i < mysprites.size(); )
            {
                if( mysprites[i] == nullptr )
                {
                    //Output the packed file's content as is
                    auto & cursubf = inpack.getSubFile(i);
                    stringstream sstr;
                    sstr << inputPath.getBaseName()
                         <<"_" <<setw(4) <<setfill('0') <<i <<"." 
                         << GetAppropriateFileExtension( cursubf.begin(), cursubf.end() );                   

                    taskmanager.AddTask( 
                        multitask::pktask_t( 
                            std::bind(lambdaWriteFileByVec, Poco::Path(outdir).append(sstr.str()).toString(), std::ref(inpack.getSubFile(i)) ) ) );
                }
                else 
                {
                    //Build the sub-folder name
                    stringstream sstr;
                    BaseSprite * curspr = mysprites[i].get();

                    if( pokesprnames.size() > i )
                    {
                        sstr <<setw(4) <<setfill('0') <<i <<"_" << pokesprnames[i];
                    }
                    else
                    {
                        sstr << inputPath.getBaseName()
                             <<"_" <<setw(4) <<setfill('0') <<i;   
                    }

                    taskmanager.AddTask( 
                        multitask::pktask_t( 
                            std::bind(lambdaExpSpriteWrap, (mysprites[i].get()), Poco::Path(outdir).append(sstr.str()).toString() ) ) );
                }
                ++i;
            }

            updtProgress = std::async( std::launch::async, PrintProgressLoop, std::ref(completed), mysprites.size(), std::ref(shouldUpdtProgress) );
            
            taskmanager.Execute();
            taskmanager.BlockUntilTaskQueueEmpty();
            taskmanager.StopExecute();

            shouldUpdtProgress = false;
            if( updtProgress.valid() )
                updtProgress.get();

            cout<<"\r100%";
        }
        catch( const Poco::Exception & )
        {
            shouldUpdtProgress = false;
            if( updtProgress.valid() )
                updtProgress.get();

            //rethrow
            std::rethrow_exception(current_exception());
        }
        catch( const exception & )
        {
            shouldUpdtProgress = false;
            if( updtProgress.valid() )
                updtProgress.get();

            //rethrow
            std::rethrow_exception(current_exception());
        }
        cout<<"\n";
    }


    void CGfxUtil::DoExportPortraits()
    {
        //We assume the input dir is the rom's root data folder
        Poco::Path inpath(m_inputPath);
        inpath.makeAbsolute().append(FONT_Dir).append(KaoFName).makeFile();
        Poco::Path outpath(m_outputPath);
        outpath.makeAbsolute().append(DefOutKaoDir).makeDirectory();
        Poco::File outdir(outpath);
        if( !outdir.exists() )
            outdir.createDirectory();

        CKaomado kao;
        KaoParser()( inpath.toString(), kao );

        //Load pokemon name and face names from the game text!
        auto gamedetails = pmd2::DetermineGameVersionAndLocale( m_inputPath );
        pmd2::ConfigLoader conf( gamedetails.first, gamedetails.second, m_pmd2cfg );
        pmd2::GameText     gt( m_inputPath, conf );
        gt.Load();

        auto itstrass = gt.GetStrings();
        if( itstrass == gt.end() )
            throw std::runtime_error("DUCK");
        
        auto boundsnames = itstrass->second.GetBoundsStringsBlock(pmd2::eStringBlocks::PkmnNames);
        auto boundsportr = itstrass->second.GetBoundsStringsBlock(pmd2::eStringBlocks::PortraitNames);

        vector<string> resfacenames;
        vector<string> rawfacenames;
        vector<string> respokenames;
        if( boundsnames.first != boundsnames.second && boundsportr.first != boundsportr.second )
        {
            auto itpokeins = back_inserter(respokenames);
            //Copy names twice, to match the content of the kaomado file!
            std::copy( boundsnames.first, boundsnames.second, itpokeins );
            std::copy( boundsnames.first, boundsnames.second, itpokeins );

            rawfacenames = std::move(vector<string>(boundsportr.first, boundsportr.second));
            resfacenames.reserve(DEF_KAO_TOC_ENTRY_NB_PTR);
        }

        //Prepare name tables
        //vector<string> pokenames( strass.GetPokemonNameBeg(), gs.GetPokemonNameEnd() );
        //vector<string> facenames( gs.GetPortraitNamesBeg(), gs.GetPortraitNamesEnd() );



        //Build facename list!
        if( rawfacenames.size() < DEF_KAO_TOC_ENTRY_NB_PTR )
        {
            //Fill
            for( size_t i = 0; i < rawfacenames.size(); ++i )
            {
                auto & fn = rawfacenames[i];
                resfacenames.push_back( move(fn) );
                resfacenames.push_back( "FLIP_" + *(resfacenames.end()) );
            }

            //Fill missing spot with empty strings
            const size_t slotsleft = ( DEF_KAO_TOC_ENTRY_NB_PTR - rawfacenames.size() );
            for( size_t i = 0; i < slotsleft; ++i )
                resfacenames.push_back( "" );
        }
        else
            resfacenames = move( rawfacenames );

        KaoWriter mywriter( &respokenames, &resfacenames );
        mywriter( kao, outpath.toString(), m_PrefOutFormat );

    }

    void CGfxUtil::DoImportPortraits()
    {
        Poco::Path inkao (m_inputPath);
        Poco::File indir(inkao);
        Poco::Path outkao(m_outputPath);
        inkao.makeAbsolute();
        outkao.makeAbsolute().append(FONT_Dir).append(KaoFName).makeFile();

        if( !indir.exists() )
        {
            stringstream sstr;
            sstr << "CGfxUtil::DoImportPortraits(): Input path \"" <<inkao.toString() <<"\" does not exists!";
            throw runtime_error(sstr.str());
        }
        else if( !indir.isDirectory() )
        {
            stringstream sstr;
            sstr << "CGfxUtil::DoImportPortraits(): Input path \"" <<inkao.toString() <<"\" is not a directory!";
            throw runtime_error(sstr.str());
        }

        CKaomado kao;
        KaoParser()( inkao.toString(), kao );
        KaoWriter()( kao, outkao.toString() );
    }

    void CGfxUtil::DoExportPokeSprites()
    {
        Poco::Path inmonster(m_inputPath);
        Poco::Path outdir   (m_outputPath);
        inmonster.makeAbsolute().append(Monster_Dir).makeDirectory();
        outdir.makeAbsolute().makeDirectory();
        Poco::File mnstrdir(inmonster);

        auto gamedetails = pmd2::DetermineGameVersionAndLocale( m_inputPath );
        pmd2::ConfigLoader conf( gamedetails.first, gamedetails.second, m_pmd2cfg );
        pmd2::GameText     gt  ( m_inputPath,       conf );
        gt.Load();

        auto itstrass = gt.GetStrings();

        if( itstrass == gt.end() )
            throw std::runtime_error("DUCK");

        auto strnamebounds = itstrass->second.GetBoundsStringsBlock(pmd2::eStringBlocks::PkmnNames);

        if( strnamebounds.first == strnamebounds.second )
            throw std::runtime_error("DUCK");

        //Currently, we do not support raw image export on sprites !
        ChkAndHndlUnsupportedRawOutput();

        if( !mnstrdir.exists() )
        {
            stringstream sstr;
            sstr << "CGfxUtil::DoExportPokeSprites(): Input path \"" <<inmonster.toString() <<"\" does not exists!";
            throw runtime_error(sstr.str());
        }
        else if( !mnstrdir.isDirectory() )
        {
            stringstream sstr;
            sstr << "CGfxUtil::DoExportPokeSprites(): Input path \"" <<inmonster.toString() <<"\" is not a directory!";
            throw runtime_error(sstr.str());
        }



        //Make the pokemon name vector
        vector<string> pknames( strnamebounds.first, strnamebounds.second );

        //Export the three sprite files
        for( size_t i = 0; i < PackedPokemonSpritesFiles.size(); ++i )
        {
            Poco::Path outdirname( PackedPokemonSpritesFiles[i].name );
            Poco::Path outsubdir = outdir;
            outsubdir.append( outdirname.getBaseName() ).makeDirectory();
            Poco::File outsubdirfile(outsubdir);
            
            if( ! outsubdirfile.exists() )
                outsubdirfile.createDirectory();

            else if( !outsubdirfile.isDirectory() )
            {
                stringstream sstr;
                sstr << "CGfxUtil::DoExportPokeSprites(): Output path \"" <<outsubdirfile.path() <<"\" is not a directory!";
                throw runtime_error(sstr.str());
            }

            Poco::Path insprpath(inmonster);
            insprpath.append(PackedPokemonSpritesFiles[i].name).makeFile();
            Poco::File inspr(insprpath);

            if( ! inspr.exists() )
            {
                clog << "Sprite file \"" <<inspr.path() << "\" doesn't exist! Skipping..\n";
                continue;
            }
            else if( ! inspr.isFile() )
            {
                clog << "Sprite file \"" <<inspr.path() << "\" is not a file! Skipping..\n";
                continue;
            }
            else
            {
                ExportASpritePackFile( inspr.path(), outsubdirfile.path(), m_PrefOutFormat, pknames );
            }
        }


    }

    void CGfxUtil::DoImportPokeSprites()
    {
                
        assert(false);
    }

    void CGfxUtil::DoExportMiscSprites()
    {
        assert(false);
    }

    void CGfxUtil::DoImportMiscSprites()
    {
        assert(false);
    }


    void CGfxUtil::HandleBGP()
    {
        vector<string> procqueue;
        auto itinstproc = back_inserter(procqueue);
        procqueue.push_back(m_inputPath);
        copy( m_extraargs.begin(), m_extraargs.end(), itinstproc );

        string fext;
        if(m_Import) 
            fext = BGP_FILEX;
        else if(m_Export)
        {
            if( m_PrefOutFormat == eSUPPORT_IMG_IO::PNG )
                fext = utils::io::PNG_FileExtension;
            else if( m_PrefOutFormat == eSUPPORT_IMG_IO::BMP )
                fext = utils::io::BMP_FileExtension;
        }

        int againcnt = 0;

        for( const auto & item : procqueue )
        {
            try
            {
                Poco::Path inpath(item);
                Poco::File infile(inpath);
                Poco::Path outpath;

                if( againcnt != 0 )
                    cout << "\nPoochyena used Crunch on \"" <<inpath.getFileName() <<"\", " <<VariationOfAgain[againcnt % VariationOfAgain.size()] <<"!\n";
                else
                    cout << "\nPoochyena used Crunch on \"" <<inpath.getFileName() <<"\"!\n";

                if( procqueue.size() > 1 )
                    outpath = Poco::Path(m_outputPath).makeAbsolute().makeParent().append(inpath.getBaseName()).makeFile().setExtension(fext);
                else if( m_outputPath.empty() )
                    outpath = Poco::Path(inpath).makeAbsolute().makeParent().append(inpath.getBaseName()).makeFile().setExtension(fext);
                else
                    outpath = Poco::Path(m_outputPath);

                if( infile.exists() && infile.isFile() )
                {
                    if( m_Import )
                    {
                        BGP bgpimg = ImportBGP(infile.path());
                        WriteBGP( bgpimg, outpath.makeAbsolute().toString() );
                    }
                    else if( m_Export )
                    {
                        BGP bgpimg = ParseBGP(infile.path());
                        ExportBGP( bgpimg, outpath.makeAbsolute().toString(), m_PrefOutFormat );
                    }
                    cout << "Its super effective!\n\"" <<inpath.getFileName() <<"\" fainted!\n";
                }
                else
                {
                    stringstream sstr;
                    sstr << "CGfxUtil::HandleBGP(): Input path \"" <<infile.path() <<"\" doesn't exist, or is not a file!";
                    throw runtime_error(sstr.str());
                }
            }
            catch( const Poco::Exception & e )
            {
                clog <<"Poco::Exception: " << e.what() <<"\n";
            }
            catch( const exception & e )
            {
                clog <<"Exception: " << e.what() <<"\n";
            }
            ++againcnt;
        }
    }

    void CGfxUtil::HandleWAN()
    {
        if( m_Import )
        {
        }
        else if( m_Export )
        {
        }
        assert(false);
    }

    void CGfxUtil::HandleWTE()
    {
        if( m_Import )
        {
        }
        else if( m_Export )
        {
        }
        assert(false);
    }

//--------------------------------------------
//  Main Methods
//--------------------------------------------
    int CGfxUtil::Main(int argc, const char * argv[])
    {
        int returnval = RETVAL_GenericFail;
        PrintTitle();

        //Handle arguments
        returnval = GatherArgs( argc, argv );
        if( returnval != 0 )
            return returnval;
        
        //Execute the utility
        returnval = Execute();

#ifdef _DEBUG
        utils::PortablePause();
#endif
        return returnval;
    }
};

//=================================================================================================
// Main Function
//=================================================================================================

//#TODO: Move the main function somewhere else !
int main( int argc, const char * argv[] )
{
    using namespace gfx_util;
    CGfxUtil & application = CGfxUtil::GetInstance();
    return application.Main(argc,argv);
}