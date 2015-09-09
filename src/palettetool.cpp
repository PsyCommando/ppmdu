#include "palettetool.hpp"
#include <utils/utility.hpp>
#include <utils/cmdline_util.hpp>
#include <ext_fmts/riff_palette.hpp>
#include <ext_fmts/png_io.hpp>
#include <ext_fmts/bmp_io.hpp>
#include <ext_fmts/txt_palette_io.hpp>
#include <ppmdu/containers/tiled_image.hpp>
#include <ext_fmts/supported_io_info.hpp>
#include <ppmdu/pmd2/pmd2_filetypes.hpp>
#include <ppmdu/pmd2/pmd2_palettes.hpp>
#include <types/content_type_analyser.hpp>
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
    const string CPaletteUtil::Version             = "0.2";
    const string CPaletteUtil::Short_Description   = "A utility to dump/build palette files from png and bmp images, and more.";
    const string CPaletteUtil::Long_Description    = 
        "This little utility is meant to be used to easily dump a palette\n"
        "to a text file or RIFF palette, from an image. It can also convert\n"
        "RIFF palettes to and from a text file palette!\n"
        "It can also insert a dummy color in the first palette slot for image\n"
        "without transparency(Ex: pokemon portraits), and preserve the\n"
        "15 or 254 previous colors!\n"
        "And finally, it also can inject a palette back into an image!\n"
        "\n"
        "The text file contains one color per line in HTML notation, which\n"
        "should make it very easy for peope to edit a palette! and convert\n"
        "it to a RIFF palette to use with gfxutil for example!\n";
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
        //Prepend dummy color
        {
            "adddummy",
            0,
            "Tell the program to insert a dummy color in color slot 0, and shift all colors by 1, while preserving the color of each pixels in the resulting image. !!Output path is ignored!!",
            "-adddummy",
            std::bind( &CPaletteUtil::ParseOptionAddDummy, &GetInstance(), placeholders::_1 ),
        },
        //Force output to RIFF palette
        {
            "outriff",
            0,
            "Dump/convert palette to the RIFF pal format.",
            "-outriff",
            std::bind( &CPaletteUtil::ParseOptionToRIFF, &GetInstance(), placeholders::_1 ),
        },
        //Force output to txt palette
        {
            "outtxt",
            0,
            "Dump/convert palette to a text file, in HTML notation.",
            "-outtxt",
            std::bind( &CPaletteUtil::ParseOptionToTxt, &GetInstance(), placeholders::_1 ),
        },
        //Force output to RIFF palette
        {
            "outrgbx32",
            0,
            "Dump/convert palette to a raw rgbx32 palette, the same format as Pokemon Mystery Dungeon 1-2 uses.",
            "-outrgbx32",
            std::bind( &CPaletteUtil::ParseOptionToRGBX32, &GetInstance(), placeholders::_1 ),
        },
        //Force import as txt palette
        {
            "intxt",
            0,
            "Force input to be interpreted as a HTML notation text file palette regardless of its file extension!",
            "-intxt",
            std::bind( &CPaletteUtil::ParseOptionInAsTxt, &GetInstance(), placeholders::_1 ),
        },
        //Force import as RGBX32 palette
        {
            "inrgbx32",
            0,
            "Force input to be interpreted as a raw RGBX32 palette regardless of its file extension!",
            "-inrgbx32",
            std::bind( &CPaletteUtil::ParseOptionInAsRGBX32, &GetInstance(), placeholders::_1 ),
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
        m_inPalType       = ePalType::Invalid;
        m_operationMode   = eOpMode ::Invalid;
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

    bool CPaletteUtil::ParseOptionAddDummy( const std::vector<std::string> & optdata )
    {
        m_operationMode = eOpMode::AddDummyColor;
        return true;
    }

    bool CPaletteUtil::ParseOptionToTxt( const std::vector<std::string> & optdata )
    {
        m_outPalType = ePalType::TEXT;
        return true;
    }
    
    bool CPaletteUtil::ParseOptionToRGBX32( const std::vector<std::string> & optdata )
    {
        m_outPalType = ePalType::RGBX32;
        return true;
    }

    bool CPaletteUtil::ParseOptionInAsTxt( const std::vector<std::string> & optdata )
    {
        m_inPalType = ePalType::TEXT;
        return true;
    }
    
    bool CPaletteUtil::ParseOptionInAsRGBX32( const std::vector<std::string> & optdata )
    {
        m_inPalType = ePalType::RGBX32;
        return true;
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

        if( m_operationMode != eOpMode::Invalid )
            return; //Skip if we have a forced mode

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
            using namespace pmd2::filetypes;

            //Skip palette type check if palette type is forced!
            if( m_inPalType == ePalType::Invalid )
            {
                const string ext = inpath.getExtension();

                if( ext == utils::io::RIFF_PAL_Filext )
                    m_inPalType = ePalType::RIFF;
                else if( ext == RGBX32_RAW_PAL_FILEX )
                    m_inPalType = ePalType::RGBX32;
                else 
                    m_inPalType = ePalType::TEXT;
            }

            if( m_outputPath.empty() )
            {
                //If output empty assume we want palette conversion
                if( m_inPalType != m_outPalType )
                {
                    m_operationMode = eOpMode::Convert;
                }
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
            utils::MrChronometer chronoexecuter("Total time elapsed");
            switch(m_operationMode)
            {
                case eOpMode::Dump:
                {
                    returnval = DumpPalette();
                    break;
                }
                case eOpMode::Convert:
                {
                    returnval = ConvertPalette();
                    break;
                }
                case eOpMode::Inject:
                {
                    returnval = InjectPalette();
                    break;
                }
                case eOpMode::AddDummyColor:
                {
                    returnval = AddDummyColorAndShift();
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

    std::vector<gimg::colorRGB24> CPaletteUtil::ImportPalette( const std::string & inpath )
    {
        std::vector<gimg::colorRGB24> palette;
        cout<<"Importing \"" <<inpath <<"\" as ";

        if( m_inPalType == ePalType::RIFF )
        {
            cout<<"RIFF palette...";
            palette = utils::io::ImportFrom_RIFF_Palette( m_inputPath );
        }
        else if( m_inPalType == ePalType::RGBX32 )
        {
            cout<<"raw RGBX32 palette...";
            vector<uint8_t> paldata = utils::io::ReadFileToByteVector( m_inputPath );
            pmd2::graphics::ReadRawPalette_RGBX32_As_RGB24( paldata.begin(), paldata.end(), palette );
        }
        else if( m_inPalType == ePalType::TEXT )
        {
            cout<<"HTML text palette...";
            palette = utils::io::ImportFrom_TXT_Palette( m_inputPath );
        }
        else
            throw runtime_error( "ERROR: Unrecognized input palette format!" );
        cout<<"done!\n";

        return std::move(palette);
    }


    void CPaletteUtil::ExportPalette( const std::string & inparentdirpath, const std::vector<gimg::colorRGB24> & palette )
    {
        Poco::Path inputPal(inparentdirpath);
        Poco::Path outputPath;
        cout <<"Exporting to ";

        if( m_outputPath.empty() )
        {
            outputPath = inputPal.append("palette").makeFile();

            if     (m_outPalType == ePalType::RIFF)   { outputPath.setExtension( utils::io::RIFF_PAL_Filext );           }
            else if(m_outPalType == ePalType::RGBX32) { outputPath.setExtension( pmd2::filetypes::RGBX32_RAW_PAL_FILEX );}
            else if(m_outPalType == ePalType::TEXT)   { outputPath.setExtension( TXTPAL_Filext );                        }
            else
                throw std::runtime_error("ERROR: Invalid output palette type!");
        }
        else
            outputPath = m_outputPath;

        if( m_outPalType == ePalType::RIFF )
        {
            cout<<"RIFF palette \"" <<outputPath.toString() <<"\"\n";
            utils::io::ExportTo_RIFF_Palette( palette, outputPath.toString() );
        }
        else if( m_outPalType == ePalType::RGBX32 )
        {
            cout<<"raw RGBX32 palette\"" <<outputPath.toString() <<"\"\n";
            vector<uint8_t> outdata;
            outdata.reserve( palette.size() * gimg::colorRGB24::getSizeRawBytes() );
            pmd2::graphics::WriteRawPalette_RGB24_As_RGBX32( back_inserter(outdata), palette.begin(), palette.end() );
            utils::io::WriteByteVectorToFile( outputPath.toString(), outdata );
        }
        else if( m_outPalType == ePalType::TEXT )
        {
            cout<<"HTML text palette \"" <<outputPath.toString() <<"\"\n";
            utils::io::ExportTo_TXT_Palette( palette, outputPath.toString() );
        }
    }


    int CPaletteUtil::DumpPalette()
    {
        using namespace gimg;
        eSUPPORT_IMG_IO imgtype = GetSupportedImageType(m_inputPath);
        vector<gimg::colorRGB24> palette; 

        cout <<"Dumping palette from \"" <<m_inputPath <<"\"...\n";

        if( imgtype == eSUPPORT_IMG_IO::BMP )
        {
            //Importing a BMP requires importing the whole image anyways...
            palette = utils::io::ImportPaletteFromBMP( m_inputPath );
        }
        else if( imgtype == eSUPPORT_IMG_IO::PNG )
        {
            palette = utils::io::ImportPaletteFromPNG( m_inputPath );
        }
        cout<<"\n";
        ExportPalette( Poco::Path(m_inputPath).makeAbsolute().makeParent().toString(), palette );

        cout <<"Palette dumped successfully!\n";
        return 0;
    }
    
    int CPaletteUtil::ConvertPalette()
    {
        using namespace gimg;
        Poco::Path               inputPal(m_inputPath);
        vector<gimg::colorRGB24> palette = ImportPalette(m_inputPath);

        cout<<"\n";
        ExportPalette( inputPal.absolute().makeParent().toString(), palette );

        cout <<"Palette converted successfully!\n";
        return 0;
    }
    
    int CPaletteUtil::InjectPalette()
    {
        using namespace gimg;
        Poco::Path               inputPal(m_inputPath);
        vector<gimg::colorRGB24> palette = ImportPalette(m_inputPath);
        cout<<"\n";
        cout <<"Injecting source palette \"" <<m_inputPath <<"\"\ninto \"" <<m_outputPath <<"\"...\n";

        //if( m_inPalType == ePalType::RIFF )
        //    palette = utils::io::ImportFrom_RIFF_Palette( m_inputPath );
        //else if( m_inPalType == ePalType::RGBX32 )
        //{
        //    vector<uint8_t> paldata = utils::io::ReadFileToByteVector( m_inputPath );
        //    pmd2::graphics::ReadRawPalette_RGBX32_As_RGB24( paldata.begin(), paldata.end(), palette );
        //}
        //else if( m_inPalType == ePalType::TEXT )
        //    palette = utils::io::ImportFrom_TXT_Palette( m_inputPath );
        //else
        //    throw runtime_error( "ERROR: Unrecognized input palette format!" );

        eSUPPORT_IMG_IO imgtype = GetSupportedImageType(m_outputPath);
        cout<<"\n";
        if( imgtype == eSUPPORT_IMG_IO::BMP )
            utils::io::SetPaletteBMPImg( palette, m_outputPath );
        else if( imgtype == eSUPPORT_IMG_IO::PNG )
            utils::io::SetPalettePNGImg( palette, m_outputPath );
        cout<<"\n";
        cout <<"Palette injected succesfully!\n";
        return 0;
    }

    template<class TImg_T>
        void DoAddDummyColorAndShift( TImg_T & image )
    {
        using namespace gimg;
        typedef typename TImg_T::pixel_t          mypixel_t;
        static const typename TImg_T::pal_color_t DummyColor = colorRGB24( 0, 255, 0 );

        //insert dummy
        std::vector<typename TImg_T::pal_color_t> & refpal = image.getPalette();
        refpal.pop_back();  //Discard anything in the last slot!
        refpal.insert( refpal.begin(), DummyColor );

        //Shift pixel values, and clamp the value to the max of the pixel!
        for( mypixel_t & pixel : image )
        {
            if( (mypixel_t::GetMaxValuePerComponent()) > static_cast<mypixel_t::pixeldata_t>(pixel+1) )
                ++pixel;
            else 
                pixel =  mypixel_t::GetMaxValuePerComponent();
        }
    }

    int CPaletteUtil::AddDummyColorAndShift()
    {
        using namespace gimg;
        Poco::Path inimg(m_inputPath);

        //Determine image type
        eSUPPORT_IMG_IO outimgty = GetSupportedImageType(m_inputPath);

        //Validate
        auto format = utils::io::GetSupportedImageFormatInfo(m_inputPath);

        if( ! format.usesPalette )
            throw runtime_error("ERROR: The image does not have a color palette!");

        cout <<"Adding dummy unused color to the begining of \"" <<m_inputPath <<"\"'s palette!\n"
             <<"If all palette slots were filled, the last color will be discarded, and pixels of that color will now refer to the previous color!\n";

        if( outimgty == eSUPPORT_IMG_IO::BMP )
        {
            if( format.bitdepth == 4 )
            {
                tiled_image_i4bpp img;
                ImportFromBMP( img, m_inputPath );
                DoAddDummyColorAndShift(img);
                ExportToBMP( img, m_inputPath );
            }
            else if(format.bitdepth == 8)
            {
                tiled_image_i8bpp img;
                ImportFromBMP( img, m_inputPath );
                DoAddDummyColorAndShift(img);
                ExportToBMP( img, m_inputPath );
            }
        }
        else if( outimgty == eSUPPORT_IMG_IO::PNG )
        {
            if( format.bitdepth == 4 )
            {
                tiled_image_i4bpp img;
                ImportFromPNG( img, m_inputPath );
                DoAddDummyColorAndShift(img);
                ExportToPNG( img, m_inputPath );
            }
            else if(format.bitdepth == 8)
            {
                tiled_image_i8bpp img;
                ImportFromPNG( img, m_inputPath );
                DoAddDummyColorAndShift(img);
                ExportToPNG( img, m_inputPath );
            }
        }

        cout<<"Done!\n";

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
    using namespace palettetool;
    CPaletteUtil & application = CPaletteUtil::GetInstance();
    return application.Main(argc,argv);
}