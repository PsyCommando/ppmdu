#include "gfxcrunch.hpp"
#include <ppmdu/fmts/content_type_analyser.hpp>
#include <ppmdu/utils/utility.hpp>
#include <ppmdu/pmd2/pmd2_sprites.hpp>
#include <ppmdu/containers/sprite_data.hpp>
#include <ppmdu/fmts/wan.hpp>
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
    static const int HPBar_NB_Bars           = 65u;
    static const int HPBar_UpdateMSecs       = 80; //Updates at every HPBar_UpdateMSecs mseconds

//=================================================================================================
// 
//=================================================================================================
    /*
        A little wrapper to avoid contaminating with the POCO library the other headers!
    */
    struct pathwrapper_t
    {
        Poco::Path mypath;
    };


//=================================================================================================
// Utility
//=================================================================================================

    /*
        Verify if "filename" is the name of one of the 3 special pack file that contain
        pokemon sprites.
    */
    bool MatchesPokeSpritePackFileName( const std::string & filename )
    {
        for( const auto & afilename : pmd2::filetypes::PackedPokemonSpritesFiles )
        {
            if( filename.compare( afilename ) == 0 )
                return true;
        }
        return false;
    }

    inline void DrawHPBar( unsigned int maxnbbars, unsigned int percent )
    {
        cout <<"\r\xb3HP:\xb4" <<setw(maxnbbars) <<setfill('\xb0') <<left <<string( ((percent * maxnbbars) / 100), '\xdb' ) <<"\xc3" 
                <<setw(3) <<setfill(' ') <<percent <<"%\xb3";
    }

    void UpdateHPBar( atomic<bool> & shouldstop, atomic<uint32_t> & parsingprogress, atomic<uint32_t> & writingprogress )
    {
        static const int TOTAL_PERCENT = 200u;
        while( !shouldstop )
        {
            DrawHPBar( HPBar_NB_Bars, ( ( ( TOTAL_PERCENT - (parsingprogress + writingprogress)  ) * 100 ) / TOTAL_PERCENT ) );
            this_thread::sleep_for( chrono::milliseconds(HPBar_UpdateMSecs) );
        }
    }

    void DrawHPBarHeader( const string & name, uint32_t level )
    {
        cout <<name <<"\n"
             <<"\xda" <<setw( HPBar_NB_Bars  + 11 ) <<setfill('\xC4') << "\xbf\n"
             <<"\xb3 " <<setw( HPBar_NB_Bars + 5 ) <<setfill(' ') <<"lvl " <<level <<" \xb3" <<"\n";
    }

    void DrawHPBarFooter()
    {
        cout <<"\n\xc0" <<right <<setw( HPBar_NB_Bars + 11 ) <<setfill('\xC4') << "\xd9\n";
    }

//=================================================================================================
//  CGfxUtil
//=================================================================================================

//------------------------------------------------
//  Constants
//------------------------------------------------
    const string CGfxUtil::Exe_Name          = "ppmd_gfxcrunch.exe";
    const string CGfxUtil::Title             = "Baz the Poochyena's PMD:EoS/T/D GfxCrunch";
    const string CGfxUtil::Version           = "0.1";
    const string CGfxUtil::Short_Description = "A utility to unpack and re-pack pmd2 sprite!";
    const string CGfxUtil::Long_Description  = 
        "#TODO";                                    //#TODO
    const string CGfxUtil::Misc_Text         = 
        "Named in honour of Baz, the awesome Poochyena of doom ! :D\n"
        "My tools in binary form are basically public domain / CC0.\n"
        "Free to re-use in any ways you may want to!\n"
        "No crappyrights, all wrongs reversed! :3";

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
        m_bQuiet        = false;
        m_ImportByIndex = false;
        m_execMode      = eExecMode::INVALID_Mode;
        m_PrefOutFormat = utils::io::eSUPPORT_IMG_IO::PNG;
        m_pInputPath.reset ( new pathwrapper_t );
        m_pOutputPath.reset( new pathwrapper_t );
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
        atomic<uint32_t>     parsingprogress(0);
        atomic<uint32_t>     writingprogress(0);
        atomic<bool>         stopupdateprogress(false);
        filetypes::Parse_WAN parser( ReadFileToByteVector( m_pInputPath->mypath.toString() ) );
        future<void>         runThUpHpBar;
        Poco::File           infileinfo(m_pInputPath->mypath);
        Poco::Path           outpath(m_pOutputPath->mypath);
        uint32_t             level = (m_pInputPath->mypath.depth() + ((( infileinfo.getSize() & 0xFF ) * 100) / 255) );
        
        outpath.append( m_pInputPath->mypath.getBaseName() );

        if( ! m_bQuiet )
        {
            if( ! m_bQuiet )
                cout << "\nPoochyena is so in sync with your wishes that she landed a critical hit!\n\n";
            DrawHPBarHeader( (m_pInputPath->mypath.getFileName()), level );
        }

        try
        {
            if( ! m_bQuiet )
            {
                runThUpHpBar = std::async( std::launch::async, UpdateHPBar, std::ref(stopupdateprogress), std::ref(parsingprogress), std::ref(writingprogress) );
            }

            auto sprty = parser.getSpriteType();
            if( sprty == graphics::eSpriteType::spr4bpp )
            {
                auto sprite = parser.ParseAs4bpp(&parsingprogress);
                graphics::ExportSpriteToDirectory( sprite, outpath.toString(), m_PrefOutFormat, false, &writingprogress );
            
            }
            else if( sprty == graphics::eSpriteType::spr8bpp )
            {
                auto sprite = parser.ParseAs8bpp(&parsingprogress);
                graphics::ExportSpriteToDirectory( sprite, outpath.toString(), m_PrefOutFormat, false, &writingprogress );
            }
        }
        catch( Poco::Exception e )
        {
            //Stop the thread
            stopupdateprogress = true;
            runThUpHpBar.get();
            //rethrow
            throw e;
        }
        catch( exception e )
        {
            //Stop the thread
            stopupdateprogress = true;
            runThUpHpBar.get();
            //rethrow
            throw e;
        }

        stopupdateprogress = true;
        runThUpHpBar.get();

        //draw one last time
        if( ! m_bQuiet )
        {
            DrawHPBar( HPBar_NB_Bars, 0 );
            DrawHPBarFooter();

            cout    << "\nIts super-effective!!\n"
                    << "\nThe sprite's copy got shred to pieces thanks to the critical hit!\n"
                    << "The pieces landed all neatly into \"" <<m_pOutputPath->mypath.toString() <<"\"!\n";

        }

        return 0;
    }

    int CGfxUtil::BuildSprite()
    {
        utils::MrChronometer chronopacker( "Building Sprite" );
        Poco::File           infileinfo(m_pInputPath->mypath);
        Poco::Path           outpath(m_pOutputPath->mypath);
        uint32_t             level = (m_pInputPath->mypath.depth() + ((( infileinfo.getSize() & 0xFF ) * 100) / 255) );
        atomic<uint32_t>     parsingprogress(0);
        atomic<uint32_t>     writingprogress(0);
        atomic<bool>         stopupdateprogress(false);
        future<void>         runThUpHpBar;

        if( ! m_bQuiet )
        {
            DrawHPBarHeader( (m_pInputPath->mypath.getFileName()), level );
        }

        outpath.append( m_pInputPath->mypath.getBaseName() );

        try
        {
            //if( ! m_bQuiet )
            //{
            //    runThUpHpBar = std::async( std::launch::async, UpdateHPBar, std::ref(stopupdateprogress), std::ref(parsingprogress), std::ref(writingprogress) );
            //}

            auto sprty = graphics::QuerySpriteTypeFromDirectory( infileinfo.path() );

            if( sprty == graphics::eSpriteType::spr4bpp )
            {
                auto sprite = graphics::ImportSpriteFromDirectory<SpriteData<gimg::tiled_image_i4bpp>>( infileinfo.path(), 
                                                                                                        m_ImportByIndex, 
                                                                                                        false, 
                                                                                                        &parsingprogress );
                filetypes::Write_WAN writer( &sprite );
                writer.write( outpath.toString(), &writingprogress );
            
            }
            else if( sprty == graphics::eSpriteType::spr8bpp )
            {
                auto sprite = graphics::ImportSpriteFromDirectory<SpriteData<gimg::tiled_image_i8bpp>>( infileinfo.path(), 
                                                                                                        m_ImportByIndex, 
                                                                                                        false, 
                                                                                                        &parsingprogress );
                filetypes::Write_WAN writer( &sprite );
                writer.write( outpath.toString(), &writingprogress );
            }
        }
        catch( Poco::Exception e )
        {
            //Stop the thread
            stopupdateprogress = true;
            if( runThUpHpBar.valid() )
                runThUpHpBar.get();
            //rethrow
            throw e;
        }
        catch( exception e )
        {
            //Stop the thread
            stopupdateprogress = true;
            if( runThUpHpBar.valid() )
                runThUpHpBar.get();
            //rethrow
            throw e;
        }

        stopupdateprogress = true;
        if( runThUpHpBar.valid() )
            runThUpHpBar.get();

        if( ! m_bQuiet )
        {
            DrawHPBarFooter();

            cout << "\nIts super-effective!!\n"
                 <<"\"" <<m_pInputPath->mypath.getFileName() <<"\" fainted!\n"
                 <<"You got \"" <<m_pOutputPath->mypath.getFileName() <<"\" for your victory!\n";
        }

        return 0;
    }

//--------------------------------------------
//  Parsing Args Methods
//--------------------------------------------
    bool CGfxUtil::ParseInputPath( const string & path )
    {
        Poco::Path inputPath(path);
        Poco::File inputfile(inputPath);

        //check if path exists
        if( inputfile.exists() )
        {
            m_pInputPath-> mypath = inputPath;
            m_pOutputPath->mypath = inputPath;
            m_pOutputPath->mypath.makeParent();
            return true;
        }
        return false;
    }
    
    bool CGfxUtil::ParseOutputPath( const string & path )
    {
        Poco::Path outpath(path);

        if( outpath.isDirectory() ||  outpath.isFile() )
        {
            m_pOutputPath->mypath = outpath;
            return true;
        }
        return false;
    }

    bool CGfxUtil::DetermineOperationMode()
    {
        using namespace pmd2::filetypes;
        Poco::File theinput( m_pInputPath->mypath );

        //If an operation mode was forced, don't try to determine what to do !
        if( m_execMode != eExecMode::INVALID_Mode )
            return true;

        if( theinput.isFile() )
        {
            //Working on a file
            /*if( theinput.getSize() < 5000000u ) */
            {
                //If less than 5mb, load it in to run the file format tester
                vector<uint8_t> tmp = utils::io::ReadFileToByteVector(theinput.path());
                auto result = CContentHandler::GetInstance().AnalyseContent(analysis_parameter(tmp.begin(), tmp.end(), m_pInputPath->mypath.getExtension() ) );

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
                     m_execMode = eExecMode::DECOMPRESS_AND_INDENTIFY_Mode;
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
                    assert(false); //crap
                    m_execMode = eExecMode::INVALID_Mode;
                    cerr << "No ideas what to do with that input parameter ^^;\n";
                    return false;
                }
            }
            //else
            //{
            //    //Too big to load.. Use the file extension..
            //    assert(false);
            //}
        }
        else if( theinput.isDirectory() )
        {
            //Working on a directory
            //Check the content and find out what to do

            //If the folder name matches one of the 3 special sprite pack file names
            if( MatchesPokeSpritePackFileName( m_pInputPath->mypath.getBaseName() ) )
            {
                m_execMode = eExecMode::BUILD_POKE_SPRITES_PACK_Mode;
                if( !m_bQuiet )
                {
                    cout << "Input folder name matches the name of one of the pokemon sprites pack file!\n"
                         << "Preparing to convert all sprites directories in the input directory into WAN sprites, and packing them into a pack file!";
                }
                assert(false); //#TODO: Implement
            }
            
            //Otherwise, analyse the content of the folder!
            vector<string> folderContent = utils::ListDirContent_FilesAndDirectories( theinput.path(), true );

            if( AreReqFilesPresent_Sprite( folderContent ) )
            {
                //We got all we need to build a sprite !
                m_execMode = eExecMode::BUILD_WAN_Mode;
                if( !m_bQuiet )
                    cout << "Required files to build a sprite found! Get the duct tape ready, we're building a sprite!\n";
            }
            else
            {
                vector<string> missingfiles = GetMissingRequiredFiles_Sprite( folderContent );
                m_execMode = eExecMode::INVALID_Mode;
                cerr << "This directory doesn't contain all the required files to build a sprite!\n"
                     << "The " <<missingfiles.size() <<" missing file(s) are :\n";
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
                cerr << "No ideas what to do with that input parameter ^^;\n";
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
            if( optdata[1].compare( "raw" ) == 0 )
            {
                m_PrefOutFormat = utils::io::eSUPPORT_IMG_IO::RAW;
            }
            else if( optdata[1].compare( utils::io::BMP_FileExtension ) == 0 )
            {
                m_PrefOutFormat = utils::io::eSUPPORT_IMG_IO::BMP;
            }
            else
            {
                //Fallback to png
                m_PrefOutFormat = utils::io::eSUPPORT_IMG_IO::PNG;
            }
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

            switch( type )
            {
                case e_ContentType::WAN_SPRITE_CONTAINER:
                {
                    m_execMode = eExecMode::UNPACK_WAN_Mode;
                    break;
                }
                case e_ContentType::KAOMADO_CONTAINER:
                {
                    m_execMode = eExecMode::UNPACK_KAOMADO_Mode;
                    break;
                }
                case e_ContentType::PACK_CONTAINER:
                {
                    m_execMode = eExecMode::UNPACK_POKE_SPRITES_PACK_Mode;
                    break;
                }
                case e_ContentType::WTE_FILE:
                {
                    m_execMode = eExecMode::EXPORT_WTE_Mode;
                    break;
                }
                case e_ContentType::BGP_FILE:
                {
                    m_execMode = eExecMode::EXPORT_BGP_Mode;
                    break;
                }
                case e_ContentType::AT4PX_CONTAINER:
                case e_ContentType::PKDPX_CONTAINER:
                {
                    //Ambiguous !!!!
                    m_execMode = eExecMode::DECOMPRESS_AND_INDENTIFY_Mode;
                    break;
                }
                default:
                    return false;
            };

            return true;
        }
        return false;
    }

    bool CGfxUtil::ParseOptionImportByIndex( const std::vector<std::string> & optdata )
    {
        //No validation required
        return (m_ImportByIndex = true);
    }

//--------------------------------------------
//  Validation Methods
//--------------------------------------------
    //bool CGfxUtil::isValid_WAN_InputFile( const string & path )
    //{
    //    //This doesn't work at all. Its too early on ! And there's a better check coming up anyways.

    //    //Poco::Path pathtovalidate(path);

    //    //A very quick and simple first stage check. The true validation will happen later, when its 
    //    // less time consuming to do so!
    //    //if( pathtovalidate.getExtension().compare( filetypes::WAN_FILEX ) == 0 )
    //    //    return true;

    //    return true;
    //}

    //bool CGfxUtil::isValid_WANT_InputDirectory( const string & path )
    //{
    //    //This doesn't work at all. Its too early on ! And there's a better check coming up anyways.
    //    //Poco::DirectoryIterator itdir(path);
    //    //Search for the required files, and subfolders
    //    //#TODO !
    //    assert(false);
    //    return false;
    //}

//--------------------------------------------
//  Main Exec functions
//--------------------------------------------
    int CGfxUtil::Execute()
    {
        int returnval = -1;

        try
        {
            //Determine Execution mode
            if( !DetermineOperationMode() )
                return -1;

            if( ! m_bQuiet )
                cout << "\nPoochyena used Crunch on \"" <<m_pInputPath->mypath.getFileName() <<"\"!\n";

            switch( m_execMode )
            {
                case eExecMode::BUILD_WAN_Mode:
                {
                    returnval = BuildSprite();
                    break;
                }
                case eExecMode::UNPACK_WAN_Mode:
                {
                    returnval = UnpackSprite();
                    break;
                }
                case eExecMode::INVALID_Mode:
                default:
                {
                    cerr<<"<!>- ERROR  : Unknown operation! Possibly an unimplemented feature! Exiting..\n";
                }
            };

            if( ! m_bQuiet && returnval == 0 )
                cout << "\n\nPoochyena used Rest! ...zZz..zZz...\n";
        }
        catch( Poco::Exception pex )
        {
            cerr <<"\n" << "<!>- POCO Exception - " <<pex.name() <<"(" <<pex.code() <<") : " << pex.message() <<endl;
            returnval = pex.code();
        }
        catch( exception e )
        {
            cerr <<"\n" 
                 <<"\nWelp.. Poochyena almost choked while crunching the pixels!\n<she gave you an apologetic look>\n"
                 << e.what() <<endl;
        }
        return returnval;
    }

    int CGfxUtil::GatherArgs( int argc, const char * argv[] )
    {
        //Parse arguments and options
        try
        {
            SetArguments(argc,argv);
            if( ! m_bQuiet )
            {
                cout << "\"" <<m_pInputPath->mypath.getFileName() <<"\" wants to battle!\n"
                     << "Poochyena can't wait to begin!\n";
            }
        }
        catch( Poco::Exception pex )
        {
            cerr <<"\n" << "<!>- POCO Exception - " <<pex.name() <<"(" <<pex.code() <<") : " << pex.message() <<endl;
            PrintReadme();
            return pex.code();
        }
        catch( exception e )
        {
            cerr <<"\n" 
                 <<"\nWelp.. Poochyena hit herself in confusion while biting through the parameters!\nShe's in a bit of a pinch. It looks like she might cry...\n"
                 << e.what() <<endl;
            PrintReadme();
            return -1;
        }
        return 0;
    }

//--------------------------------------------
//  Main Methods
//--------------------------------------------
    int CGfxUtil::Main(int argc, const char * argv[])
    {
        int returnval = -1;
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