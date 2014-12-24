#include "pspr_extractor.hpp"
#include <ppmdu/fmts/content_type_analyser.hpp>
#include <ppmdu/utils/utility.hpp>
#include <ppmdu/pmd2/pmd2_sprites.hpp>
#include <string>
#include <algorithm>
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cassert>

using namespace ::std;
using namespace ::pmd2;
using ::pmd2::graphics::CCharSpriteData;
using ::pmd2::graphics::sprite_parser;
//using ::pmd2::graphics::export_8bppTiled_to_png;
using ::utils::GetPathWithoutFileExt;
using ::utils::GetFilenameFromPath;
using ::utils::GetPathOnly;
using ::utils::MrChronometer;

namespace pspr_extract
{
    //=================================================================================================
    // Handle Export
    //=================================================================================================

    bool FileIsValidSprite( const vector<uint8_t> & filedata )
    {
        auto paramtopass = filetypes::analysis_parameter( filedata.begin(), filedata.end() );
        auto result      = filetypes::CContentHandler::GetInstance().AnalyseContent( paramtopass );

        return (result._type == filetypes::e_ContentType::SPRITE_CONTAINER);
    }


    bool ExportFramesToPNGs( const vector<uint8_t> &filedata, const string &inputpath, const string &outputpath )
    {
        assert(false); //This thing needs a rewrite, the sprite format is better understood now !

        //using ::pmd2::graphics::export_8bppTiled_to_png;

        ////#1 - Create the output folder
        //string outputfolderpath= utils::AppendTraillingSlashIfNotThere( outputpath );
        //outputfolderpath += GetPathWithoutFileExt( GetFilenameFromPath( inputpath ) );

        ////Make the directory
        //if( utils::DoCreateDirectory( outputfolderpath ) )
        //{
        //    cout <<"*Created or re-used directory at\n   " <<outputfolderpath <<"\n";
        //    //#2 - Run the parser functor
        //    unsigned int    cptframes = 0;
        //    CCharSpriteData asprite;
        //    sprite_parser parser( asprite );
        //    parser( filedata.begin(), filedata.end() );

        //    //#3 - Run the export functor
        //    export_8bppTiled_to_png exporter( outputfolderpath );
        //
        //    vector<graphics::indexed8bppimg_t> &refframes = asprite.getAllFrames();
        //    for( auto &tiledimg : refframes )
        //    {
        //        exporter( tiledimg, asprite.getPalette() );
        //        ++cptframes;
        //    }
        //    cout << "\nOutputed " <<cptframes <<" frames!\n";
        //    return true;
        //}
        return false;
    }

    //=================================================================================================
    // Utility
    //=================================================================================================

    void PrintUsage()
    {
	    cout << "ppmd_sprex.exe \"inputpath\" \"outputpath\"\n\n"
		     << "-> inputpath       : sprite file to read from.\n"
		     << "-> outputpath(opt) : folder to output the file(s) to.\n\n\n"
		     << "Example:\n"
             << "ppmd_sprex.exe ./file.sir0\n"
		     << "ppmd_sprex.exe ./file.sir0 ./outputfolder/\n\n"
             << "If a file is passed as input path, the frames in the sprite file.\n"
             << "will be exported to PNG images within a folder with the same name as\n"
             << "the input file, excluding the file extension. The files will be numbered\n"
             << "in the same order they are numbered in the file.\n"
             << "The PNG format is 8bpp, indexed RGB24. An each PNG contains the palette!\n\n"
             << "Omitting the destination path will result in the program outputing the\n" 
             << "resulting sub-folder and PNGs to the current working directory.\n"
             << "----------------------------------------------------------\n"
		     << "Named in honour of Baz, the awesome Poochyena of doom, which was my hero character in my PMD2 run ! :D\n\n"
		     << "No crappyrights, all wrongs reversed !\n"
             << "(In short, consider this Public Domain, or CC0!)\n"
             << "Sources and specs Included in original package!\n" <<endl;
    }

    int HandleArguments( int argc, const char * argv[], string & inputpath, string & outputpath )
    {

        //#1 - Handle the parameters
        if( argc >= 2 && argc <= 3  )
        {
            inputpath = argv[1];

            if( argc == 3 )
            {
                outputpath = argv[2];
            }
            else
            {
                outputpath = GetPathOnly( inputpath );
            }

            if( !utils::isFile(inputpath) )
            {
                cerr << "!-Fatal Error: Input file or path invalid!\n";
                return -1;
            }
            if( !utils::isFolder(outputpath) )
            {
                cerr << "!-Fatal Error: Output path invalid!\n";
                return -1;
            }
        }
        else
        {
            PrintUsage();
            return 1;
        }
        return 0;
    }
};

//=================================================================================================
// Main Function
//=================================================================================================
int main( int argc, const char * argv[] )
{
    using namespace pspr_extract;
    string inputpath,
            outputpath;
    int    result = 0;


	cout << "==================================================\n"
            << "==  Baz the Poochyena's PMD:EoS/T/D SprEx - 0.1 ==\n" 
            << "==================================================\n"
            << "A sprite+palette extractor / converter.\n"
            << endl;

    if( HandleArguments( argc, argv, inputpath, outputpath ) !=0 )
    {
        return -1;
    }
        

    //#2 - Verify the file is a valid sprite file
    //#3 - Execute the extractor
    {
        MrChronometer mychrono("Total");

        //Read File to memory
        vector<uint8_t> filedata;
        utils::ReadFileToByteVector( inputpath, filedata );

        if( FileIsValidSprite(filedata) ) //Validate
        {
            result = ExportFramesToPNGs( filedata, inputpath, outputpath ); //Export
        }
        else
        {
            cerr << "!- Fatal Error: The input file is not a valid sprite file!\nAborting!" <<endl;
            result = -1;
        }
    }

#ifdef _DEBUG
    system("pause");
#endif

    return result;
}