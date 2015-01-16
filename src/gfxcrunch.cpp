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
// Handle Export
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

    bool MatchesPokeSpritePackFileName( const std::string & filename )
    {
        for( const auto & afilename : pmd2::filetypes::PackedPokemonSpritesFiles )
        {
            if( filename.compare( afilename ) == 0 )
                return true;
        }
        return false;
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
    }};

//------------------------------------------------
//  Methods
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
        m_bQuiet      = false;
        m_execMode    = eExecMode::INVALID_Mode;
        m_pInputPath.reset( new pathwrapper_t );
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


    void UpdateHPBar( atomic<bool> & shouldstop, atomic<uint32_t> & parsingprogress, atomic<uint32_t> & writingprogress )
    {
        static const int NB_Bars       = 25u;
        static const int TOTAL_PERCENT = 200u;
        while( !shouldstop )
        {
            unsigned int nbbars = ( ( TOTAL_PERCENT - (parsingprogress + writingprogress) ) * NB_Bars ) / TOTAL_PERCENT; 
            cout <<"\r[" <<setw(NB_Bars) <<setfill(' ') <<string( nbbars, '=' ) <<"] " 
                 <<setw(3) <<setfill(' ') << ( ( ( TOTAL_PERCENT - (parsingprogress + writingprogress)  ) * 100 ) / TOTAL_PERCENT ) <<"%";

            this_thread::sleep_for( chrono::milliseconds(10) );
        }
    }

    int CGfxUtil::UnpackSprite()
    {
        atomic<uint32_t>     parsingprogress(0);
        atomic<uint32_t>     writingprogress(0);
        atomic<bool>         stopupdateprogress(false);
        Poco::File           infileinfo(m_pInputPath->mypath);
        filetypes::Parse_WAN parser( ReadFileToByteVector( m_pInputPath->mypath.toString() ) );

        uint32_t level = (m_pInputPath->mypath.depth() + ((( infileinfo.getSize() & 0xFF ) * 100) / 255) );

        cout <<m_pInputPath->mypath.getFileName() <<setw(25) <<setfill(' ') <<"lvl " <<level <<"\n"
             <<"HP:\n";

        auto myfuture = std::async( std::launch::async, UpdateHPBar, std::ref(stopupdateprogress), std::ref(parsingprogress), std::ref(writingprogress) );

        auto sprty = parser.getSpriteType();
        if( sprty == filetypes::Parse_WAN::eSpriteType::spr4bpp )
        {
            auto       sprite = parser.ParseAs4bpp(&parsingprogress);
            Poco::Path outpath(m_pOutputPath->mypath);
            outpath.pushDirectory(m_pInputPath->mypath.getBaseName());

            graphics::ExportSpriteToDirectory( sprite, outpath.toString(), m_PrefOutFormat, false, &writingprogress );
            
        }
        else if( sprty == filetypes::Parse_WAN::eSpriteType::spr8bpp )
        {
            auto       sprite = parser.ParseAs8bpp(&parsingprogress);
            Poco::Path outpath(m_pOutputPath->mypath);
            outpath.pushDirectory(m_pInputPath->mypath.getBaseName());

            graphics::ExportSpriteToDirectory( sprite, outpath.toString(), m_PrefOutFormat, false, &writingprogress );
        }


        stopupdateprogress = true;
        myfuture.get();

        return 0;
    }

    int CGfxUtil::BuildSprite()
    {
        Poco::File           infileinfo(m_pInputPath->mypath);
        cout <<m_pInputPath->mypath.getFileName() <<setw(25) <<setfill(' ') <<"lvl " <<(m_pInputPath->mypath.depth() + ( infileinfo.getSize() & 0xF )) <<"\n"
             <<"HP:\n";

        return 0;
    }


    bool CGfxUtil::ParseInputPath( const string & path )
    {
        Poco::Path inputPath(path);
        Poco::File inputfile(inputPath);

        //check if path exists
        if( inputfile.exists() )
        {
            //if( inputfile.isFile()           && isValid_WAN_InputFile( path ) )
            //    m_execMode = eExecMode::UNPACK_SPRITE_Mode;
            //else if( inputfile.isDirectory() && isValid_WANT_InputDirectory( path ) )
            //    m_execMode = eExecMode::BUILD_SPRITE_Mode;
            //else
            //    return false; //File does not exist return failure

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
        bool       success = false;

        //if( outpath.isDirectory() && m_execMode == eExecMode::UNPACK_SPRITE_Mode )
        //{
        //    m_pOutputPath->mypath = outpath;
        //    success = true;
        //}
        //else if( outpath.isFile() && m_execMode == eExecMode::BUILD_SPRITE_Mode )
        //{
        //    m_pOutputPath->mypath = outpath;
        //    success = true;
        //}
        if( outpath.isDirectory() ||  outpath.isFile() )
        {
            m_pOutputPath->mypath = outpath;
            success = true;
        }
        
        return success;
    }

    void CGfxUtil::DetermineOperationMode()
    {
        using namespace pmd2::filetypes;
       // eExecMode::BUILD_SPRITE_Mode;
        Poco::File theinput( m_pInputPath->mypath );

        if( theinput.isFile() )
        {
            //Working on a file
            if( theinput.getSize() < 5000000u ) 
            {
                //If less than 5mb load it in to run the file format tester
                vector<uint8_t> tmp = utils::io::ReadFileToByteVector(theinput.path());
                auto result = CContentHandler::GetInstance().AnalyseContent(analysis_parameter(tmp.begin(), tmp.end(), m_pInputPath->mypath.getExtension() ) );

                if( result._type == e_ContentType::WAN_SPRITE_CONTAINER )
                {
                    m_execMode = eExecMode::UNPACK_SPRITE_Mode;
                }
                else if( result._type == e_ContentType::PACK_CONTAINER )
                {
                    m_execMode = eExecMode::EXPORT_POKE_SPRITES_PACK_Mode;
                }
                else if( result._type == e_ContentType::AT4PX_CONTAINER )
                {
                }
                else if( result._type == e_ContentType::PKDPX_CONTAINER )
                {
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
                    if( !m_bQuiet )
                        cerr << "No ideas what to do with that input parameter ^^;\n";
                }
            }
            else
            {
                //Too big to load.. Use the file extension..
                assert(false);
            }
        }
        else if( theinput.isDirectory() )
        {
            //Working on a directory

            //Check the content and find out what to do
            Poco::DirectoryIterator diter( theinput );
            Poco::DirectoryIterator diterend;
            vector<string> content; //list of the filenames found at the root of the input folder

            //If the folder name matches one of the 3 special sprite pack file names
            if( MatchesPokeSpritePackFileName( m_pInputPath->mypath.getBaseName() ) )
            {
                m_execMode = eExecMode::IMPORT_POKE_SPRITES_PACK_Mode;
                if( !m_bQuiet )
                    cerr << "Input folder name matches the name of one of the pokemon sprites pack file!\n"
                         << "Preparing to convert all sprites directories in the input directory into WAN sprites, and packing them into a pack file!";
            }
            
            //Otherwise, analyse the content of the folder!
            while( diter != diterend )
            {
                content.push_back( Poco::Path( diter->path() ).getFileName() );
                ++diter;
            }

            if( AreReqFilesPresent_Sprite( content ) )
            {
                //We got all we need to build a sprite !
                m_execMode = eExecMode::BUILD_SPRITE_Mode;
                if( !m_bQuiet )
                    cerr << "Required files to build a sprite found! Get the duct tape ready, we're building a sprite!\n";
            }
            else
            {
                assert(false); //crap
                m_execMode = eExecMode::INVALID_Mode;
                if( !m_bQuiet )
                    cerr << "No ideas what to do with that input parameter ^^;\n";
            }

        }
        else
        {
            //Unknown..
            m_execMode = eExecMode::INVALID_Mode;
            if( !m_bQuiet )
                cerr << "No ideas what to do with that input parameter ^^;\n";
        }
    }

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

    bool CGfxUtil::isValid_WAN_InputFile( const string & path )
    {
        //Poco::Path pathtovalidate(path);

        //A very quick and simple first stage check. The true validation will happen later, when its 
        // less time consuming to do so!
        //if( pathtovalidate.getExtension().compare( filetypes::WAN_FILEX ) == 0 )
        //    return true;

        return true;
    }

    bool CGfxUtil::isValid_WANT_InputDirectory( const string & path )
    {
        //Poco::DirectoryIterator itdir(path);
        //Search for the required files, and subfolders
        //#TODO !
        assert(false);
        return false;
    }

    //Main method
    int CGfxUtil::Main(int argc, const char * argv[])
    {
        int returnval = -1;
        PrintTitle();

        //Parse arguments and options
        try
        {
            SetArguments(argc,argv);
            cout << "\"" <<m_pInputPath->mypath.getFileName() <<"\" wants to battle!\n"
                 << "Poochyena can't wait to begin!\n";
        }
        catch( Poco::Exception pex )
        {
            cerr <<"\n" << "<!>- POCO Exception - " <<pex.name() <<"(" <<pex.code() <<") : " << pex.message() <<endl;
            PrintReadme();
            return returnval;
        }
        catch( exception e )
        {
            cerr <<"\n" 
                 <<"\nWelp.. Poochyena hit herself in confusion while biting through the parameters!\nShe's in a bit of a pinch. It looks like she might cry...\n"
                 << e.what() <<endl;
            PrintReadme();
            return returnval;
        }
        
        //Exec the operation
        try
        {
            //Determine Execution mode
            DetermineOperationMode();

            cout << "\nPoochyena used Crunch on \"" <<m_pInputPath->mypath.getFileName() <<"\"!\n";
            switch( m_execMode )
            {
                case eExecMode::BUILD_SPRITE_Mode:
                {
                    returnval = BuildSprite();
                    cout << "\nIts super-effective!!\n"
                         <<"\"" <<m_pInputPath->mypath.getFileName() <<"\" fainted!\n"
                         <<"You got \"" <<m_pOutputPath->mypath.getFileName() <<"\" for your victory!\n";
                    break;
                }
                case eExecMode::UNPACK_SPRITE_Mode:
                {
                    cout << "\nPoochyena is so in sync with your wishes that she landed a critical hit!\n\n";
                    returnval = UnpackSprite();
                    cout << "\nIts super-effective!!\n"
                         << "\nThe sprite's copy got shred to pieces thanks to the critical hit!\n"
                         << "The pieces landed all neatly into \"" <<m_pOutputPath->mypath.toString() <<"\"!\n";
                    break;
                }
                default:
                {
                    assert(false); //This should never happen
                }
            };

            cout << "Poochyena used Rest! She went to sleep!\n";
        }
        catch( Poco::Exception pex )
        {
            cerr <<"\n" << "<!>- POCO Exception - " <<pex.name() <<"(" <<pex.code() <<") : " << pex.message() <<endl;
        }
        catch( exception e )
        {
            cerr <<"\n" 
                 <<"\nWelp.. Poochyena almost choked while crunching the pixels! <she gave you an apologetic look>\n"
                 << e.what() <<endl;
        }

#ifdef _DEBUG
        system("pause");
#endif

        return returnval;
    }
};

//=================================================================================================
// Main Function
//=================================================================================================
void Test();
//void TestEncode();

int main( int argc, const char * argv[] )
{
    using namespace gfx_util;
    CGfxUtil & application = CGfxUtil::GetInstance();
    return application.Main(argc,argv);

    //Test();
    //TestEncode();
    return 0;
}

//#include "Poco/DOM/DOMParser.h"
//#include "Poco/DOM/Document.h"
//#include "Poco/DOM/NodeIterator.h"
//#include "Poco/DOM/NodeFilter.h"
//#include "Poco/DOM/AutoPtr.h"
//#include "Poco/SAX/InputSource.h"
//#include "Poco/XML/XMLWriter.h"
//#include "Poco/SAX/AttributesImpl.h"
//#include <fstream>
//
//void Test()
//{
//    typedef Poco::XML::Document PDoc;
//
//    std::ifstream in("test.xml");
//    Poco::XML::InputSource src(in);
//    Poco::XML::DOMParser parser;
//    Poco::AutoPtr<PDoc> pDoc = parser.parse(&src);
//    Poco::XML::NodeIterator it(pDoc, Poco::XML::NodeFilter::SHOW_ALL);
//    Poco::XML::Node* pNode = it.nextNode();
//    while (pNode)
//    {
//        std::cout<<pNode->nodeName()<<":"<< pNode->nodeValue()<<std::endl;
//        pNode = it.nextNode();
//    }
//
//
//    using namespace Poco::XML;
//
//    std::ofstream str("test.xml");
//    XMLWriter     writer(str, XMLWriter::WRITE_XML_DECLARATION | XMLWriter::PRETTY_PRINT);
//    writer.setNewLine("\n");
//    writer.startDocument();
//
//
//    //AttributesImpl attrs;
//    //attrs.addAttribute("", "", "a1", "", "v1");
//    //attrs.addAttribute("", "", "a2", "", "v2");
//    
//    //writer.startElement("","","");
//
//    writer.startElement("", "animation_data", "");//, attrs);
//   
//    AttributesImpl attrs;
//    attrs.addAttribute("","", "name", "string", "run");
//
//    writer.startElement("", "", "group",attrs);
//
//    AttributesImpl seqattrs;
//    seqattrs.addAttribute("","", "name", "string", "Up");
//
//    writer.startElement("","","sequence", seqattrs);
//    writer.endElement("","","sequence");
//
//    writer.endElement("", "", "group");
//
//    writer.endElement("", "animation_data", "");
//    writer.endDocument();
//
//    system("pause");
//}

//Encoding/decoding tests
//#include <ppmdu/fmts/sir0.hpp>
//#include  <fstream>
//
//
//void TestEncode()
//{
//    vector<uint32_t> toencode={{ 0x130000 & 0x7FFFFF }};
//    vector<uint8_t>  encoded;
//    cout <<"Encoded :\n";
//    encoded = filetypes::EncodeSIR0PtrOffsetList( toencode );
//    for( auto & entry : encoded )
//        cout <<hex <<"0x" << static_cast<unsigned short>(entry) <<"\n";
//
//    cout <<"\nRe-decoded :\n";
//    vector<uint32_t> resultdecode = filetypes::DecodeSIR0PtrOffsetList( encoded );
//
//    for( auto & entry : resultdecode )
//        cout <<hex <<"0x" <<entry <<"\n";
//
//
//    system("pause");
//}
//
//void Tests()
//{
//    std::vector<uint8_t> todecode=
//    {{
//0x04, 0x04, 0xEB, 0x4C, 0x18, 0x18, 0x83, 0x24, 0x18, 0x18, 0x18, 0x83, 0x30, 0x18, 0x18, 0x83, 
//0x44, 0x18, 0x82, 0x70, 0x18, 0x18, 0x83, 0x30, 0x18, 0x83, 0x10, 0x18, 0x18, 0x82, 0x70, 0x18, 
//0x83, 0x30, 0x18, 0x83, 0x30, 0x18, 0x18, 0x83, 0x30, 0x18, 0x18, 0x18, 0x83, 0x24, 0x18, 0x18, 
//0x83, 0x04, 0x18, 0x18, 0x18, 0x83, 0x24, 0x18, 0x18, 0x82, 0x30, 0x18, 0x82, 0x64, 0x18, 0x83, 
//0x30, 0x18, 0x18, 0x83, 0x10, 0x18, 0x18, 0x18, 0x83, 0x10, 0x18, 0x18, 0x82, 0x70, 0x18, 0x18, 
//0x18, 0x83, 0x38, 0x18, 0x83, 0x30, 0x18, 0x18, 0x18, 0x83, 0x38, 0x18, 0x18, 0x83, 0x04, 0x18, 
//0x18, 0x18, 0x83, 0x24, 0x18, 0x18, 0x83, 0x64, 0x18, 0x83, 0x70, 0x83, 0x64, 0x18, 0x83, 0x44, 
//0x18, 0x18, 0x83, 0x64, 0x18, 0x83, 0x50, 0x83, 0x10, 0x82, 0x70, 0x18, 0x83, 0x10, 0x18, 0x83, 
//0x04, 0x18, 0x18, 0x83, 0x10, 0x18, 0x18, 0x83, 0x30, 0x18, 0x83, 0x24, 0x18, 0x18, 0x82, 0x70, 
//0x18, 0x18, 0x83, 0x30, 0x18, 0x18, 0x83, 0x30, 0x18, 0x18, 0x82, 0x64, 0x18, 0x83, 0x30, 0x18, 
//0x18, 0x83, 0x30, 0x18, 0x18, 0x83, 0x30, 0x18, 0x18, 0x82, 0x50, 0x18, 0x83, 0x24, 0x18, 0x18, 
//0x83, 0x24, 0x18, 0x18, 0x83, 0x30, 0x18, 0x18, 0x81, 0x64, 0x18, 0x82, 0x64, 0x18, 0x83, 0x10, 
//0x18, 0x18, 0x83, 0x44, 0x18, 0x18, 0x83, 0x10, 0x18, 0x18, 0x83, 0x30, 0x18, 0x18, 0x82, 0x70, 
//0x18, 0x83, 0x30, 0x18, 0x18, 0x82, 0x70, 0x18, 0x82, 0x70, 0x18, 0x82, 0x70, 0x18, 0x82, 0x70, 
//0x18, 0x82, 0x70, 0x18, 0x82, 0x70, 0x18, 0x18, 0x18, 0x82, 0x70, 0x18, 0x18, 0x18, 0x82, 0x70, 
//0x18, 0x18, 0x18, 0x82, 0x50, 0x18, 0x18, 0x18, 0x82, 0x30, 0x18, 0x18, 0x82, 0x30, 0x18, 0x18, 
//0x82, 0x10, 0x18, 0x18, 0x82, 0x10, 0x18, 0x18, 0x81, 0x50, 0x18, 0x81, 0x50, 0x18, 0x83, 0x10, 
//0x18, 0x18, 0x83, 0x10, 0x82, 0x70, 0x18, 0x82, 0x30, 0x82, 0x10, 0x18, 0x18, 0x81, 0x70, 0x18, 
//0x83, 0x10, 0x83, 0x10, 0x83, 0x10, 0x83, 0x10, 0x64, 0x10, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x9B, 0x64, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x14, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x10, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x08, 0x28, 0x08, 0x20, 0x08, 0x08, 0x08, 
//0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 
//0x08, 0x08, 0x08, 0x08, 0x08, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
//0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x10, 0x04, 0x0C, 0x04, 0x00, 
//    }};
//
//
//    
//    cout <<"Deccoding test :\n";
//
//    ofstream outputf("converted_shinx_sir0ptr_table.txt", ios::binary);
//    vector<uint32_t> result = filetypes::DecodeSIR0PtrOffsetList( todecode );
//
//    for( auto & entry : result )
//        outputf <<hex <<"0x" <<entry <<"\n";
//
//    cout <<"Re-encoding :\n";
//    /*vector<uint32_t> toencode={{ 0x130001 & 0x7FFFFF }};*/
//    vector<uint8_t>  encoded;
//    encoded = filetypes::EncodeSIR0PtrOffsetList( result );
//    ofstream outputreencodedf("re-encoded_shinx_sir0ptr_table.bin", ios::binary);
//    for( auto & entry : encoded )
//        outputreencodedf.write( reinterpret_cast<char*>(&entry),1);
//
//    cout <<"result :\n";
//    for( auto & abyte : encoded )
//        cout <<hex <<"0x" <<static_cast<unsigned short>(abyte) <<" ";
//
//    cout <<"\nRe-decoding :\n";
//    ofstream outputredecodef("redecoded_shinx_sir0ptr_table.txt", ios::binary);
//    vector<uint32_t> resultdecode = filetypes::DecodeSIR0PtrOffsetList( encoded );
//
//    for( auto & entry : resultdecode )
//        outputredecodef <<hex <<"0x" <<entry <<"\n";
//
//    system("pause");
//}