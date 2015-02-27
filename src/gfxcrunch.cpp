#include "gfxcrunch.hpp"
#include <ppmdu/fmts/content_type_analyser.hpp>
#include <ppmdu/utils/utility.hpp>
//#include <ppmdu/pmd2/pmd2_sprites.hpp>
#include <ppmdu/containers/sprite_data.hpp>
#include <ppmdu/utils/multiple_task_handler.hpp>
#include <ppmdu/utils/library_wide.hpp>
#include <ppmdu/fmts/wan.hpp>
#include <ppmdu/fmts/pack_file.hpp>
#include <ppmdu/fmts/pkdpx.hpp>
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

using namespace ::std;
using namespace ::pmd2;
using namespace ::pmd2::graphics;
using namespace ::utils::cmdl;
using namespace ::utils::io;

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
        for( const auto & apack : pmd2::filetypes::PackedPokemonSpritesFiles )
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
        for( const auto & apack : pmd2::filetypes::PackedPokemonSpritesFiles )
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
            uint32_t percent = ( (100 * completed.load()) / total );
            cout << "\r" <<setw(3) <<setfill(' ') <<percent <<"%";
            this_thread::sleep_for( ProgressUpdThWait );
        }
    }

    /*
    */
    pmd2::filetypes::CPack UnpackPackFile( const Poco::Path & packfilepath )
    {
        using namespace pmd2::filetypes;
        vector<uint8_t> result = utils::io::ReadFileToByteVector( packfilepath.toString() );
        CPack           mypack;

        mypack.LoadPack( result.begin(), result.end() );

        return std::move( mypack );
    }

    void ParseASprite( const std::vector<uint8_t> & srcraw, std::unique_ptr<graphics::BaseSprite> & targetptr )
    {
        filetypes::WAN_Parser parser( srcraw );
        auto                  sprty = parser.getSpriteType();
                
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
    }

    /*
    */
    void TurnPackContentToSpriteData( pmd2::filetypes::CPack & srcpack, std::vector<std::unique_ptr<graphics::BaseSprite>> & out_table )
    {
        cout <<"Parsing sprites..\n";
        out_table.resize( srcpack.getNbSubFiles() );
        for( unsigned int i = 0; i < srcpack.getNbSubFiles(); )
        {
            auto & curSubFile = srcpack.getSubFile(i);
            auto   cnttype    = filetypes::DetermineCntTy( curSubFile.begin(), curSubFile.end() );

            //
            if( cnttype._type == filetypes::e_ContentType::WAN_SPRITE_CONTAINER )
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
            else if( cnttype._type == filetypes::e_ContentType::PKDPX_CONTAINER )
            {
                //Do decompress before converting!
                vector<uint8_t> decompbuff;
                filetypes::DecompressPKDPX( curSubFile.begin(), curSubFile.end(), decompbuff );

                //Do a check on the decompressed file
                if( filetypes::DetermineCntTy( decompbuff.begin(), decompbuff.end() )._type != filetypes::e_ContentType::WAN_SPRITE_CONTAINER )
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
        return AreReqFilesPresent_Sprite( utils::ListDirContent_FilesAndDirectories( path, true ) );
    }

//=================================================================================================
//  CGfxUtil
//=================================================================================================

//------------------------------------------------
//  Constants
//------------------------------------------------
    const string CGfxUtil::Exe_Name            = "ppmd_gfxcrunch.exe";
    const string CGfxUtil::Title               = "Baz the Poochyena's PMD:EoS/T/D GfxCrunch";
    const string CGfxUtil::Version             = "0.12";
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
            std::bind( &CGfxUtil::ParseOutputPath, &GetInstance(), placeholders::_1 ),
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

        //Forcing input format
        //{
        //    "as",
        //    1,
        //    "Force the content of a file to be handled as the type specified. This is ignored when the input is a directory!",
        //    "-as (wan,wte,bgp,...)",
        //    std::bind( &CGfxUtil::ParseOptionForceInputFormat, &GetInstance(), placeholders::_1 ),
        //},
    }};

//------------------------------------------------
// Misc Methods
//------------------------------------------------

    CGfxUtil & CGfxUtil::GetInstance()
    {
        static CGfxUtil s_util;
        return s_util;
    }

    CGfxUtil::CGfxUtil()
        :CommandLineUtility()
    {
        _Construct();
    }

    void CGfxUtil::_Construct()
    {
        clog <<getExeName() <<" " <<getVersionString() <<" initializing..\n";
        m_bQuiet        = false;
        m_ImportByIndex = false;
        m_bRedirectClog = false;
        m_execMode      = eExecMode::INVALID_Mode;
        m_PrefOutFormat = utils::io::eSUPPORT_IMG_IO::PNG;

        m_inputCompletion    = 0;
        m_outputCompletion   = 0;
        m_bStopProgressPrint = false;

        //By default, use a single thread!
        unsigned int nbThreadsToUse = ( ( thread::hardware_concurrency() <= 2 )? 1 : 2 ); //Use 2 thread on system with more than 2 hardware threads
        utils::LibraryWide::getInstance().Data().setNbThreadsToUse( nbThreadsToUse ); 
        clog << "Using " <<nbThreadsToUse <<" thread(s).\n";

        //m_packPokemonNameList;
        m_pathToAnimNameResFile  = DefPathAnimRes;
        m_pathToPokeSprNamesFile = DefPathPokeSprNames;
        m_pathToFaceNamesFile    = DefPathFaceNames;
        m_pathToPokeNamesFile    = DefPathPokeNames;

    }

    const vector<argumentparsing_t> & CGfxUtil::getArgumentsList   ()const { return Arguments_List;          }
    const vector<optionparsing_t>   & CGfxUtil::getOptionsList     ()const { return Options_List;            }
    const argumentparsing_t         * CGfxUtil::getExtraArg        ()const { return &Arguments_List.front(); }
    const string                    & CGfxUtil::getTitle           ()const { return Title;                   }
    const string                    & CGfxUtil::getExeName         ()const { return Exe_Name;                }
    const string                    & CGfxUtil::getVersionString   ()const { return Version;                 }
    const string                    & CGfxUtil::getShortDescription()const { return Short_Description;       }
    const string                    & CGfxUtil::getLongDescription ()const { return Long_Description;        }
    const string                    & CGfxUtil::getMiscSectionText ()const { return Misc_Text;               }

//--------------------------------------------
//  Mode Execution Methods
//--------------------------------------------
    int CGfxUtil::UnpackSprite()
    {
        utils::MrChronometer chronounpacker( "Unpacking Sprite" );
        filetypes::WAN_Parser parser( ReadFileToByteVector( m_inputPath ) );
        Poco::File           infileinfo(m_inputPath);
        Poco::Path           inputPath(m_inputPath);
        Poco::Path           outpath;
        
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
                                                                                                    m_ImportByIndex );
            filetypes::WAN_Writer writer( &sprite );

            if( m_compressToPKDPX )
            {
                vector<uint8_t> result  = writer.write();
                vector<uint8_t> outdata;
                filetypes::CompressToPKDPX( result.begin(), result.end(), outdata );
                utils::io::WriteByteVectorToFile( outpath.setExtension(filetypes::PKDPX_FILEX).toString(), outdata );
            }
            else
                writer.write( outpath.setExtension(filetypes::WAN_FILEX).toString() );
            
        }
        else if( sprty == graphics::eSpriteImgType::spr8bpp )
        {
            auto sprite = graphics::ImportSpriteFromDirectory<SpriteData<gimg::tiled_image_i8bpp>>( infileinfo.path(), 
                                                                                                    m_ImportByIndex );
            filetypes::WAN_Writer writer( &sprite );

            if( m_compressToPKDPX )
            {
                vector<uint8_t> result  = writer.write();
                vector<uint8_t> outdata;
                filetypes::CompressToPKDPX( result.begin(), result.end(), outdata );
                utils::io::WriteByteVectorToFile( outpath.setExtension(filetypes::PKDPX_FILEX).toString(), outdata );
            }
            else
                writer.write( outpath.setExtension(filetypes::WAN_FILEX).toString() );
        }


        if( ! m_bQuiet )
        {
            cout << "\nIts super-effective!!\n"
                 <<"\"" <<inputPath.getFileName() <<"\" fainted!\n"
                 <<"You got \"" <<outpath.getFileName() <<"\" for your victory!\n";
        }

        return 0;
    }

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
                         << filetypes::GetAppropriateFileExtension( cursubf.begin(), cursubf.end() );                   

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


    void BuildSprFromDirAndInsert( vector<uint8_t> & out_sprRaw, const Poco::Path & inDirPath, bool importByIndex, bool bShouldCompress )
    {
        auto sprty = graphics::QuerySpriteImgTypeFromDirectory( inDirPath.toString() );

        if( sprty == graphics::eSpriteImgType::spr4bpp )
        {
            auto sprite = graphics::ImportSpriteFromDirectory<SpriteData<gimg::tiled_image_i4bpp>>( inDirPath.toString(), 
                                                                                                    importByIndex, 
                                                                                                    false );
            filetypes::WAN_Writer writer( &sprite );
            if( bShouldCompress )
            {
                auto filedata = writer.write();
                filetypes::CompressToPKDPX( filedata.begin(), filedata.end(), out_sprRaw, compression::ePXCompLevel::LEVEL_3, true );
            }
            else
                out_sprRaw = writer.write();
            
        }
        else if( sprty == graphics::eSpriteImgType::spr8bpp )
        {
            auto sprite = graphics::ImportSpriteFromDirectory<SpriteData<gimg::tiled_image_i8bpp>>( inDirPath.toString(), 
                                                                                                    importByIndex, 
                                                                                                    false );
            filetypes::WAN_Writer writer( &sprite );
            if( bShouldCompress )
            {
                auto filedata = writer.write();
                filetypes::CompressToPKDPX( filedata.begin(), filedata.end(), out_sprRaw );
            }
            else
                out_sprRaw = writer.write();
        }
    }

    int CGfxUtil::PackAndImportCharSprites()
    {
        utils::ChronoRAII<std::chrono::seconds> chronopacker( "Packing & Importing Sprites" );
        Poco::File                   infileinfo(m_inputPath);
        Poco::Path                   inpath(m_inputPath);
        Poco::Path                   outpath;
        Poco::DirectoryIterator      itDirCount( infileinfo );
        Poco::DirectoryIterator      itDirEnd;
        filetypes::CPack             mypack;
        vector<Poco::File>           validDirs;
        future<void>                 updtProgress;
        atomic<bool>                 shouldUpdtProgress = true;
        multitask::CMultiTaskHandler taskmanager;
        atomic<uint32_t>             completed;

        auto lambdaWrapBuildSpr = [&completed]( vector<uint8_t> & out_sprRaw, const Poco::File & infile, bool importByIndex, bool bShouldCompress )->bool
        {
            BuildSprFromDirAndInsert(out_sprRaw, infile.path(), importByIndex, bShouldCompress);
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
        catch( Poco::Exception &e )
        {
            shouldUpdtProgress = false;
            if( updtProgress.valid() )
                updtProgress.get();

            //rethrow
            rethrow_exception( current_exception() );
        }
        catch( exception &e )
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
            m_outputPath = Poco::Path(inpath).makeFile().setExtension(filetypes::PACK_FILEX).toString();

        outpath = m_outputPath;

        string outfilepath = outpath.toString();
        cout <<"\n\nBuilding \"" <<outfilepath <<"\"...\n";
        utils::io::WriteByteVectorToFile( outfilepath, mypack.OutputPack() );
        cout <<"\nDone!\n";

        return 0;
    }


    int CGfxUtil::DecompressAndHandle()
    {
        utils::MrChronometer chronounpacker( "Unpacking compressed sprite" );
        vector<uint8_t>      rawCompFile = ReadFileToByteVector( m_inputPath );
        Poco::File           infileinfo(m_inputPath);
        Poco::Path           inputPath(m_inputPath);
        Poco::Path           outpath;

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
        filetypes::DecompressPKDPX( rawCompFile.begin(), rawCompFile.end(), decompBuf );

        //Analyse content
        auto result = filetypes::DetermineCntTy( decompBuf.begin(), decompBuf.end() );

        if( result._type != filetypes::e_ContentType::WAN_SPRITE_CONTAINER )
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

//--------------------------------------------
//  Parsing Args Methods
//--------------------------------------------
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
                cout <<"Unpacking Sprites Containing Pack File\n";
                break;
            }
            case eExecMode::BUILD_POKE_SPRITES_PACK_Mode:
            {
                cout <<"Building Sprites Containing Pack File\n";
                break;
            }
            case eExecMode::DECOMPRESS_AND_INDENTIFY_Mode:
            {
                cout <<"Decompressing and Unpacking Sprite\n";
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
        using namespace pmd2::filetypes;
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

            if( result._type == e_ContentType::WAN_SPRITE_CONTAINER )
            {
                m_execMode = eExecMode::UNPACK_WAN_Mode;
            }
            else if( result._type == e_ContentType::PACK_CONTAINER )
            {
                m_execMode = eExecMode::UNPACK_POKE_SPRITES_PACK_Mode;
            }
            else if( result._type == e_ContentType::AT4PX_CONTAINER )
            {
                throw exception("AT4PX compressed files not supported at this time!");
                //m_execMode = eExecMode::DECOMPRESS_AND_INDENTIFY_Mode;
            }
            else if( result._type == e_ContentType::PKDPX_CONTAINER )
            {
                m_execMode = eExecMode::DECOMPRESS_AND_INDENTIFY_Mode;
            }
            else if( result._type == e_ContentType::BGP_FILE )
            {
                m_execMode = eExecMode::EXPORT_BGP_Mode;
            }
            else if( result._type == e_ContentType::WTE_FILE )
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
            vector<string> folderContent = utils::ListDirContent_FilesAndDirectories( theinput.path(), true );

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

    bool CGfxUtil::ParseOptionForceInputFormat( const std::vector<std::string> & optdata )
    {
        using filetypes::e_ContentType;

        if( optdata.size() == 2 )
        {
            auto          result = filetypes::GetFileTypeFromExtension( optdata.back() );
            e_ContentType type   = result.front();

            cout << "<*>-Forcing input as ";

            switch( type )
            {
                case e_ContentType::WAN_SPRITE_CONTAINER:
                {
                    m_execMode = eExecMode::UNPACK_WAN_Mode;
                    cout <<"WAN sprite";
                    break;
                }
                case e_ContentType::KAOMADO_CONTAINER:
                {
                    m_execMode = eExecMode::UNPACK_KAOMADO_Mode;
                    cout <<"kaomado.kao file";
                    break;
                }
                case e_ContentType::PACK_CONTAINER:
                {
                    m_execMode = eExecMode::UNPACK_POKE_SPRITES_PACK_Mode;
                    cout <<"pokemon sprites containing pack file";
                    break;
                }
                case e_ContentType::WTE_FILE:
                {
                    m_execMode = eExecMode::EXPORT_WTE_Mode;
                    cout <<"WTE file";
                    break;
                }
                case e_ContentType::BGP_FILE:
                {
                    m_execMode = eExecMode::EXPORT_BGP_Mode;
                    cout <<"BGP file";
                    break;
                }
                case e_ContentType::AT4PX_CONTAINER:
                case e_ContentType::PKDPX_CONTAINER:
                {
                    //Ambiguous !!!!
                    m_execMode = eExecMode::DECOMPRESS_AND_INDENTIFY_Mode;
                    cout <<"AT4PX/PKDPX file";
                    break;
                }
                default:
                {
                    cout <<"INVALID!\n";
                    return false;
                }
            };
            cout <<"!\n";
            return true;
        }
        return false;
    }

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

//--------------------------------------------
//  Main Exec functions
//--------------------------------------------
    int CGfxUtil::Execute()
    {
        int returnval = RETVAL_GenericFail;
        try
        {
            Poco::Path inpath(m_inputPath);

            //Determine Execution mode
            if( !DetermineOperationMode() )
                return RETVAL_GenericFail;

            PrintOperationMode();

            if( ! m_bQuiet )
                cout << "\nPoochyena used Crunch on \"" <<inpath.getFileName() <<"\"!\n";

            if( m_bRedirectClog )
                m_redirectClog.Redirect( inpath.getBaseName() + ".log" );

            //Init or not the chrono for the log file
            unique_ptr<utils::ChronoRAII<>> chronototal = nullptr;
            if(utils::LibWide().isLogOn())
                chronototal.reset( new utils::ChronoRAII<>("Total Execution Time", &clog ) );

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

    int CGfxUtil::GatherArgs( int argc, const char * argv[] )
    {
        //Parse arguments and options
        try
        {
            //SetArguments returns false, when there are no args to parse !
            SetArguments(argc,argv);

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
        catch( Poco::Exception pex )
        {
            cerr <<"\n" << "<!>-POCO Exception - " <<pex.name() <<"(" <<pex.code() <<") : " << pex.message() <<endl;
            if( utils::LibWide().isLogOn() )
                clog <<"\n" << "<!>-POCO Exception - " <<pex.name() <<"(" <<pex.code() <<") : " << pex.message() <<endl;
            PrintReadme();
            return pex.code();
        }
        catch( exception e )
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
        system("pause");
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