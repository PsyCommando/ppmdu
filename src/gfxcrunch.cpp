#include "gfxcrunch.hpp"
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
    const vector<optionparsing_t> CGfxUtil::Options_List =
    {{
        //Quiet
        {
            "q",
            0,
            "Disables console progress output.",
            "-q",
            std::bind( &CGfxUtil::ParseOptionQuiet, &GetInstance(), placeholders::_1 ),
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

    int CGfxUtil::UnpackSprite()
    {
        return 0;
    }

    int CGfxUtil::BuildSprite()
    {
        return 0;
    }


    bool CGfxUtil::ParseInputPath( const string & path )
    {
        Poco::Path inputPath(path);
        Poco::File inputfile(inputPath);

        //check if path exists
        if( inputfile.exists() )
        {
            if( inputfile.isFile()           && isValidInputFile( path ) )
                m_execMode = eExecMode::UNPACK_SPRITE_Mode;
            else if( inputfile.isDirectory() && isValidInputDirectory( path ) )
                m_execMode = eExecMode::BUILD_SPRITE_Mode;
            else
                return false; //File does not exist return failure

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

        if( outpath.isDirectory() && m_execMode == eExecMode::UNPACK_SPRITE_Mode )
        {
            m_pOutputPath->mypath = outpath;
            success = true;
        }
        else if( outpath.isFile() && m_execMode == eExecMode::BUILD_SPRITE_Mode )
        {
            m_pOutputPath->mypath = outpath;
            success = true;
        }
        
        return success;
    }

    bool CGfxUtil::ParseOptionQuiet( const vector<string> & optdata )
    {
        //If this is called, we don't need to do any additional validation!
        return m_bQuiet = true;
    }

    bool CGfxUtil::isValidInputFile( const string & path )
    {
        Poco::Path pathtovalidate(path);

        //A very quick and simple first stage check. The true validation will happen later, when its 
        // less time consuming to do so!
        if( pathtovalidate.getExtension().compare( filetypes::WAN_FILEX ) == 0 )
            return true;

        return false;
    }

    bool CGfxUtil::isValidInputDirectory( const string & path )
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
            switch( m_execMode )
            {
                case eExecMode::BUILD_SPRITE_Mode:
                {
                    returnval = BuildSprite();
                    break;
                }
                case eExecMode::UNPACK_SPRITE_Mode:
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
//#include <ppmdu/fmts/sir0.hpp>
//#include  <fstream>
int main( int argc, const char * argv[] )
{
    using namespace gfx_util;
    CGfxUtil & application = CGfxUtil::GetInstance();
    return application.Main(argc,argv);
}