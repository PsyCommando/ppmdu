#include "palettetool.hpp"
#include <ppmdu/utils/utility.hpp>
#include <ppmdu/utils/cmdline_util.hpp>
#include <ppmdu/ext_fmts/riff_palette.hpp>
#include <ppmdu/ext_fmts/png_io.hpp>
#include <ppmdu/ext_fmts/bmp_io.hpp>
#include <ppmdu/ext_fmts/txt_palette_io.hpp>
#include <ppmdu/containers/tiled_image.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/Exception.h>
using namespace ::std;
using namespace ::utils::cmdl;
using namespace ::utils::io;

namespace palettetool
{
    static const string TXTPAL_Filext = "txt";

//=================================================================================================
//  CPaletteUtil
//=================================================================================================

//------------------------------------------------
//  Constants
//------------------------------------------------
    const string CPaletteUtil::Exe_Name            = "ppmd_palettetool.exe";
    const string CPaletteUtil::Title               = "Palette Dumper/Bulder";
    const string CPaletteUtil::Version             = "0.1";
    const string CPaletteUtil::Short_Description   = "A utility to dump/build palette files from png and bmp images";
    const string CPaletteUtil::Long_Description    = 
        "#TODO";                                    //#TODO
    const string CPaletteUtil::Misc_Text           = 
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
    const vector<argumentparsing_t> CPaletteUtil::Arguments_List =
    {{
        //Input Path argument
        { 
            0,      //first arg
            false,  //mandatory
            true,   //guaranteed to appear in order
            "input path", 
            "The file to work on. It can be either an image, or a palette file. The program will operate differently based on the input!",
#ifdef WIN32
            "\"c:/mysprites/image.png\"",
#elif __linux__
            "\"/mysprites/image.png\"",
#endif
            std::bind( &CPaletteUtil::ParseInputPath, &GetInstance(), placeholders::_1 ),
        },
        //Output Path argument
        { 
            1,      //second arg
            true,   //optional
            true,   //guaranteed to appear in order
            "output path", 
            "Output path. The result of the operation will be placed, and named according to this path!",
#ifdef WIN32
            "\"c:/mysprites/palette.pal\"",
#elif __linux__
            "\"/mysprites/palette.pal\"",
#endif
            std::bind( &CPaletteUtil::ParseOutputPath, &GetInstance(), placeholders::_1 ),
        },
    }};

//------------------------------------------------
//  Options Info
//------------------------------------------------

    /*
        Information on all the switches / options to allow the automated parser 
        to parse them.
    */
    const vector<optionparsing_t> CPaletteUtil::Options_List=
    {{
        //Quiet
        {
            "riffpal",
            0,
            "Dump/convert palette to the RIFF pal format.",
            "-riffpal",
            std::bind( &CPaletteUtil::ParseOptionToRIFF, &GetInstance(), placeholders::_1 ),
        },
        //Quiet
        {
            "8bpp",
            0,
            "When loading an image, its loaded as a 8bpp image instead of the 4bpp by default.",
            "-8bpp",
            std::bind( &CPaletteUtil::ParseOption8bpp, &GetInstance(), placeholders::_1 ),
        },
    }};

//------------------------------------------------
// Misc Methods
//------------------------------------------------

    CPaletteUtil & CPaletteUtil::GetInstance()
    {
        static CPaletteUtil s_util;
        return s_util;
    }

    CPaletteUtil::CPaletteUtil()
        :CommandLineUtility()
    {
        m_outPalType      = ePalType::TEXT;
        m_operationMode   = eOpMode::Invalid;
        m_imgFormatIs8bpp = false;
    }

    const vector<argumentparsing_t> & CPaletteUtil::getArgumentsList   ()const { return Arguments_List;          }
    const vector<optionparsing_t>   & CPaletteUtil::getOptionsList     ()const { return Options_List;            }
    const argumentparsing_t         * CPaletteUtil::getExtraArg        ()const { return &Arguments_List.front(); }
    const string                    & CPaletteUtil::getTitle           ()const { return Title;                   }
    const string                    & CPaletteUtil::getExeName         ()const { return Exe_Name;                }
    const string                    & CPaletteUtil::getVersionString   ()const { return Version;                 }
    const string                    & CPaletteUtil::getShortDescription()const { return Short_Description;       }
    const string                    & CPaletteUtil::getLongDescription ()const { return Long_Description;        }
    const string                    & CPaletteUtil::getMiscSectionText ()const { return Misc_Text;               }

//--------------------------------------------
//  Parse Args
//--------------------------------------------
    bool CPaletteUtil::ParseInputPath( const string & path )
    {
        Poco::File inputfile(path);

        //check if path exists
        if( inputfile.exists() && inputfile.isFile() )
        {
            m_inputPath = path;
            return true;
        }
        return false;
    }
    
    bool CPaletteUtil::ParseOutputPath( const string & path )
    {
        Poco::Path outpath(path);

        if( outpath.isFile() )
        {
            m_outputPath = path;
            return true;
        }
        return false;
    }

//
//  Parse Options
//
    bool CPaletteUtil::ParseOptionToRIFF( const std::vector<std::string> & optdata )
    {
        m_outPalType = ePalType::RIFF;
        return true;
    }

    bool CPaletteUtil::ParseOption8bpp( const std::vector<std::string> & optdata )
    {
        return m_imgFormatIs8bpp = true;
    }

//
//
//
    int CPaletteUtil::GatherArgs( int argc, const char * argv[] )
    {
        int returnval = 0;
        //Parse arguments and options
        try
        {
            if( !SetArguments(argc,argv) )
                return -3;

            DetermineOperation();
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
            return -3;
        }
        return returnval;
    }

    void CPaletteUtil::DetermineOperation()
    {
        using utils::io::eSUPPORT_IMG_IO;
        Poco::Path inpath( m_inputPath );
        eSUPPORT_IMG_IO imgtype = GetSupportedImageType(m_inputPath);

        if( imgtype == eSUPPORT_IMG_IO::BMP || 
            imgtype == eSUPPORT_IMG_IO::PNG )
        {
            if( !m_outputPath.empty() )
            {
                Poco::Path outpath( m_outputPath );
                Poco::File outparentdir(outpath.parent());

                if( !( outparentdir.exists() && outparentdir.isDirectory() ) )
                    throw runtime_error("Error, output path parent directory doesn't exist!");
            }

            m_operationMode = eOpMode::Dump;
        }
        else //Input is not an image
        {
            ePalType inpaltype = (inpath.getExtension() == utils::io::RIFF_PAL_Filext)? ePalType::RIFF : ePalType::TEXT;

            if( m_outputPath.empty() )
            {
                //If output empty assume we want palette conversion
                if( inpaltype != m_outPalType )
                    m_operationMode = eOpMode::Convert;
                else
                    throw runtime_error( "The palette input type is the same as the palette output type! Skipping conversion!" );
            }
            else //Output path not empty
            {
                //Poco::Path      outpath( m_outputPath );
                eSUPPORT_IMG_IO outimgty = GetSupportedImageType(m_outputPath);

                //Check if the output has an image file extension
                if( outimgty == eSUPPORT_IMG_IO::BMP || outimgty == eSUPPORT_IMG_IO::PNG )
                {
                    m_operationMode = eOpMode::Inject;
                }
                else//( outimgty == eSUPPORT_IMG_IO::INVALID )
                {
                    //Assume the output is a palette!
                    m_operationMode = eOpMode::Convert;
                }
            }
        }
        //CASES:
        //#1 Input is image
        //If Input is palette:
        //   #2 Output is Palette
        //   #3 Output is Image
    }

    int CPaletteUtil::Execute()
    {
        int returnval = -1;
        try
        {
            switch(m_operationMode)
            {
                case eOpMode::Dump:
                {
                    DumpPalette();
                    break;
                }
                case eOpMode::Convert:
                {
                    ConvertPalette();
                    break;
                }
                case eOpMode::Inject:
                {
                    InjectPalette();
                    break;
                }
                default:
                {
                    throw runtime_error( "Invalid operation mode. Something is wrong with the arguments!" );
                }
            };
        }
        catch(Poco::Exception & e )
        {
            cerr <<"\n" << "<!>- POCO Exception - " <<e.name() <<"(" <<e.code() <<") : " << e.message() <<endl;
        }
        catch( exception &e )
        {
            cerr <<"\n" << "<!>- Exception - " <<e.what() <<"\n";
        }
        return returnval;
    }

//--------------------------------------------
//  Operation
//--------------------------------------------
    int CPaletteUtil::DumpPalette()
    {
        using namespace gimg;
        eSUPPORT_IMG_IO imgtype = GetSupportedImageType(m_inputPath);
        vector<gimg::colorRGB24> palette; 

        cout <<"Dumping Palette...\n";

        if( imgtype == eSUPPORT_IMG_IO::BMP )
        {
            //Importing a BMP requires importing the whole image anyways...
            palette = utils::io::ImportPaletteFromBMP( m_inputPath );
        }
        else if( imgtype == eSUPPORT_IMG_IO::PNG )
        {
            palette = utils::io::ImportPaletteFromPNG( m_inputPath );
        }

        //Dump the palette
        if( m_outPalType == ePalType::RIFF )
        {
            string outputpath;

            if( m_outputPath.empty() )
                outputpath = Poco::Path(m_inputPath).parent().append( "palette.pal" ).toString();
            else
                outputpath = m_outputPath;

            utils::io::ExportTo_RIFF_Palette( palette, outputpath );
        }
        else if( m_outPalType == ePalType::TEXT )
        {
            string outputpath;

            if( m_outputPath.empty() )
                outputpath = Poco::Path(m_inputPath).parent().append( "palette.txt" ).toString();
            else
                outputpath = m_outputPath;

            utils::io::ExportTo_TXT_Palette( palette, outputpath );
        }

        cout <<"Palette dump complete!\n";
        return 0;
    }
    
    int CPaletteUtil::ConvertPalette()
    {
        using namespace gimg;
        Poco::Path               inputPal(m_inputPath);
        vector<gimg::colorRGB24> palette;
        cout <<"Converting Palette...\n";


        if( inputPal.getExtension() == utils::io::RIFF_PAL_Filext )
            palette = utils::io::ImportFrom_RIFF_Palette( m_inputPath );
        else //Else always assume its a txt palette until the end!
            palette = utils::io::ImportFrom_TXT_Palette( m_inputPath );


        if( m_outPalType == ePalType::RIFF )
        {
            string outputPath;

            if( m_outputPath.empty() )
                outputPath = Poco::Path(m_inputPath).parent().append( "palette.pal" ).toString();
            else
                outputPath = m_outputPath;

            utils::io::ExportTo_RIFF_Palette( palette, outputPath );
            
        }
        else if( m_outPalType == ePalType::TEXT )
        {
            string outputPath;

            if( m_outputPath.empty() )
                outputPath = Poco::Path(m_inputPath).parent().append( "palette.txt" ).toString();
            else
                outputPath = m_outputPath;

            utils::io::ExportTo_TXT_Palette( palette, outputPath );
        }

        cout <<"Palette converted!\n";
        return 0;
    }
    
    int CPaletteUtil::InjectPalette()
    {
        using namespace gimg;
        cout <<"Injecting Palette...\n";
        Poco::Path               inputPal(m_inputPath);
        vector<gimg::colorRGB24> palette;

        if( inputPal.getExtension() == utils::io::RIFF_PAL_Filext )
            palette = utils::io::ImportFrom_RIFF_Palette( m_inputPath );
        else //Else always assume its a txt palette until the end!
            palette = utils::io::ImportFrom_TXT_Palette( m_inputPath );

        eSUPPORT_IMG_IO imgtype = GetSupportedImageType(m_outputPath);

        if( imgtype == eSUPPORT_IMG_IO::BMP )
        {
            //Importing a BMP requires importing the whole image anyways...
            utils::io::SetPaletteBMPImg( palette, m_outputPath );
        }
        else if( imgtype == eSUPPORT_IMG_IO::PNG )
        {
            utils::io::SetPalettePNGImg( palette, m_outputPath );
        }

        cout <<"Palette injected!\n";
        return 0;
    }

//--------------------------------------------
//  Main Methods
//--------------------------------------------
    int CPaletteUtil::Main(int argc, const char * argv[])
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
    using namespace palettetool;
    CPaletteUtil & application = CPaletteUtil::GetInstance();
    return application.Main(argc,argv);
}