#include "sprite_util.hpp"
#include <ppmdu/fmts/content_type_analyser.hpp>
#include <ppmdu/utils/utility.hpp>
#include <ppmdu/pmd2/pmd2_sprites.hpp>
#include <ppmdu/fmts/wan.hpp>
#include <string>
#include <algorithm>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cassert>
#include <utility>
#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/Exception.h>

using namespace ::std;
using namespace ::pmd2;
using namespace ::utils::cmdl;
using namespace ::utils::io;

namespace sprite_util
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


//=================================================================================================
//  CSpriteUtil
//=================================================================================================

//------------------------------------------------
//  Constants
//------------------------------------------------
    const string CSpriteUtil::Exe_Name          = "ppmd_spriteutil.exe";
    const string CSpriteUtil::Title             = "Baz the Poochyena's PMD:EoS/T/D Sprite util";
    const string CSpriteUtil::Version           = "0.1";
    const string CSpriteUtil::Short_Description = "A utility to unpack and re-pack pmd2 sprite!";
    const string CSpriteUtil::Long_Description  = 
        "#TODO";                                    //#TODO
    const string CSpriteUtil::Misc_Text         = 
        "Named in honour of Baz, the awesome Poochyena of doom ! :D\n"
        "My tools in binary form are basically public domain / CC0.\n"
        "Free to re-use in any ways you may want to!\n"
        "No crappyrights, all wrongs reversed! :3";

//------------------------------------------------
//  Arguments Info
//------------------------------------------------
    const vector<argumentparsing_t> CSpriteUtil::Arguments_List =
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
            std::bind( &CSpriteUtil::ParseInputPath, &GetInstance(), placeholders::_1 ),
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
            std::bind( &CSpriteUtil::ParseOutputPath, &GetInstance(), placeholders::_1 ),
        },
    }};

//------------------------------------------------
//  Options Info
//------------------------------------------------
    const vector<optionparsing_t> CSpriteUtil::Options_List =
    {{
        //Quiet
        {
            "q",
            0,
            "Disables console progress output.",
            "-q",
            std::bind( &CSpriteUtil::ParseOptionQuiet, &GetInstance(), placeholders::_1 ),
        },
    }};

//------------------------------------------------
//  Methods
//------------------------------------------------

    CSpriteUtil & CSpriteUtil::GetInstance()
    {
        static CSpriteUtil s_util;
        return s_util;
    }

    CSpriteUtil::CSpriteUtil()
        :CommandLineUtility()
    {
        _Construct();
    }

    void CSpriteUtil::_Construct()
    {
        m_bQuiet      = false;
        m_programMode = eExecMode::INVALID_Mode;
        m_pInputPath.reset( new pathwrapper_t );
        m_pOutputPath.reset( new pathwrapper_t );
    }

    const vector<argumentparsing_t> & CSpriteUtil::getArgumentsList   ()const { return Arguments_List;          }
    const vector<optionparsing_t>   & CSpriteUtil::getOptionsList     ()const { return Options_List;            }
    const argumentparsing_t         * CSpriteUtil::getExtraArg        ()const { return &Arguments_List.front(); }
    const string                    & CSpriteUtil::getTitle           ()const { return Title;                   }
    const string                    & CSpriteUtil::getExeName         ()const { return Exe_Name;                }
    const string                    & CSpriteUtil::getVersionString   ()const { return Version;                 }
    const string                    & CSpriteUtil::getShortDescription()const { return Short_Description;       }
    const string                    & CSpriteUtil::getLongDescription ()const { return Long_Description;        }
    const string                    & CSpriteUtil::getMiscSectionText ()const { return Misc_Text;               }

    int CSpriteUtil::UnpackSprite()
    {
        return 0;
    }

    int CSpriteUtil::BuildSprite()
    {
        return 0;
    }


    bool CSpriteUtil::ParseInputPath( const string & path )
    {
        Poco::Path inputPath(path);
        Poco::File inputfile(inputPath);

        //check if path exists
        if( inputfile.exists() )
        {
            if( inputfile.isFile()           && isValidInputFile( path ) )
                m_programMode = eExecMode::UNPACK_Mode;
            else if( inputfile.isDirectory() && isValidInputDirectory( path ) )
                m_programMode = eExecMode::BUILD_Mode;
            else
                return false; //File does not exist return failure

            m_pInputPath-> mypath = inputPath;
            m_pOutputPath->mypath = inputPath;
            m_pOutputPath->mypath.makeParent();
            return true;
        }
        return false;
    }
    
    bool CSpriteUtil::ParseOutputPath( const string & path )
    {
        Poco::Path outpath(path);
        bool       success = false;

        if( outpath.isDirectory() && m_programMode == eExecMode::UNPACK_Mode )
        {
            m_pOutputPath->mypath = outpath;
            success = true;
        }
        else if( outpath.isFile() && m_programMode == eExecMode::BUILD_Mode )
        {
            m_pOutputPath->mypath = outpath;
            success = true;
        }
        
        return success;
    }

    bool CSpriteUtil::ParseOptionQuiet( const vector<string> & optdata )
    {
        //If this is called, we don't need to do any additional validation!
        return m_bQuiet = true;
    }

    bool CSpriteUtil::isValidInputFile( const string & path )
    {
        Poco::Path pathtovalidate(path);

        //A very quick and simple first stage check. The true validation will happen later, when its 
        // less time consuming to do so!
        if( pathtovalidate.getExtension().compare( filetypes::WAN_FILEX ) == 0 )
            return true;

        return false;
    }

    bool CSpriteUtil::isValidInputDirectory( const string & path )
    {
        //Poco::DirectoryIterator itdir(path);
        //Search for the required files, and subfolders
        //#TODO !
        assert(false);
        return false;
    }

    //Main method
    int CSpriteUtil::Main(int argc, const char * argv[])
    {
        int returnval = -1;
        PrintTitle();

        //Parse arguments and options
        try
        {
            SetArguments(argc,argv);
        }
        catch( Poco::Exception pex )
        {
            cerr <<"\n" << "<!>- POCO Exception - " <<pex.name() <<"(" <<pex.code() <<") : " << pex.message() <<endl;
            PrintReadme();
        }
        catch( exception e )
        {
            cerr <<"\n" << e.what() <<endl;
            PrintReadme();
        }
        
        //Exec the operation
        try
        {
            switch( m_programMode )
            {
                case eExecMode::BUILD_Mode:
                {
                    returnval = BuildSprite();
                    break;
                }
                case eExecMode::UNPACK_Mode:
                {
                    returnval = UnpackSprite();
                    break;
                }
                default:
                {
                    assert(false); //This should never happen
                }
            };
        }
        catch( Poco::Exception pex )
        {
            cerr <<"\n" << "<!>- POCO Exception - " <<pex.name() <<"(" <<pex.code() <<") : " << pex.message() <<endl;
        }
        catch( exception e )
        {
            cerr <<"\n" << e.what() <<endl;
        }

        return returnval;
    }
};

//=================================================================================================
// Main Function
//=================================================================================================
#include <ppmdu/fmts/sir0.hpp>
#include  <fstream>

int main( int argc, const char * argv[] )
{
    //using namespace sprite_util;
    //sprite_util::CSpriteUtil & application = sprite_util::CSpriteUtil::GetInstance();
    //return application.Main(argc,argv);

    //test
    std::vector<uint8_t> ptroffsetlistraw =  
    {{
0x04, 0x04,0xD6, 0x78, 0x18, 0x18, 0x82, 0x44, 0x18, 0x18, 0x82, 0x44, 0x18, 0x18, 0x82, 0x44, 
0x18, 0x18, 0x82, 0x24, 0x18, 0x18, 0x82, 0x44, 0x18, 0x18, 0x82, 0x44, 0x18, 0x18, 0x82, 0x44, 
0x18, 0x18, 0x82, 0x30, 0x18, 0x18, 0x82, 0x44, 0x18, 0x18, 0x82, 0x24, 0x18, 0x18, 0x82, 0x44, 
0x18, 0x18, 0x82, 0x44, 0x18, 0x18, 0x82, 0x44, 0x18, 0x18, 0x82, 0x44, 0x18, 0x18, 0x82, 0x10, 
0x18, 0x18, 0x82, 0x10, 0x18, 0x18, 0x82, 0x30, 0x18, 0x18, 0x82, 0x30, 0x18, 0x18, 0x82, 0x30, 
0x18, 0x18, 0x82, 0x30, 0x18, 0x18, 0x82, 0x30, 0x18, 0x18, 0x82, 0x30, 0x18, 0x18, 0x82, 0x30, 
0x18, 0x18, 0x82, 0x30, 0x18, 0x18, 0x82, 0x30, 0x18, 0x18, 0x81, 0x70, 0x18, 0x18, 0x82, 0x44, 
0x18, 0x18, 0x82, 0x44, 0x18, 0x18, 0x82, 0x44, 0x18, 0x18, 0x82, 0x44, 0x18, 0x18, 0x82, 0x44, 
0x18, 0x18, 0x82, 0x44, 0x18, 0x18, 0x82, 0x30, 0x18, 0x18, 0x82, 0x30, 0x18, 0x18, 0x64, 0x10, 
0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x93, 0x08, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x10, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
0x64, 0x08, 0x08, 0x08, 0x20, 0x08, 0x08, 0x08, 0x08, 0x81, 0x38, 0x04, 0x04, 0x04, 0x04, 0x04, 
0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 
0x10, 0x04, 0x0C, 0x04, 0x00, 
    }};
    //{{
    //        0x04,
    //        0x04,
    //        0x92,
    //        0x0C,
    //        0x14,
    //        0,
    //}};

    cout <<"Decoding values: " <<endl;
    vector<uint32_t> result = filetypes::DecodeSIR0PtrOffsetList(ptroffsetlistraw);

    for( auto & entry : result )
        cout <<hex <<entry <<endl;
    cout <<endl;

    cout <<"Encoding values: " <<endl;
    //std::vector<uint32_t> ptroffsestsEnc =  
    //{{
    //        0x904,
    //        0x918,
    //}};

    auto resultenc =  filetypes::EncodeSIR0PtrOffsetList( result );
    //filetypes::MakeSIR0ForData( vector<uint32_t>( result.begin() + 2, result.end()), 0x69F4, 0x6AD0 ); //Ignore the first 2 val

    ofstream outencoded("encoded.bin", ios::binary );
    for( auto & entry : resultenc )
        outencoded.write(reinterpret_cast<char*>(&entry), 1 );
    cout <<endl;

    system("pause");

    return 0;
}