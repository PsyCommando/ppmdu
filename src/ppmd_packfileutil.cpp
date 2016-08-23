/*
ppmd_packfileutil.cpp
2014/10/01
psycommando@gmail.com
Description:  


*/
#include "ppmd_packfileutil.hpp"
#include <ppmdu/pmd2/pmd2_filetypes.hpp>
#include <ppmdu/fmts/pack_file.hpp>
#include <utils/utility.hpp>
#include <utils/library_wide.hpp>
#include <utils/cmdline_util.hpp>
#include <cassert>
#include <sstream>
#include <fstream>
#include <iostream>
#include <Poco/Path.h>
using namespace ::std;
using namespace ::pmd2;
using namespace ::utils::io;
using namespace ::utils::cmdl;
using namespace ::utils;
using namespace ::filetypes;

namespace ppmd_packfileutil
{
//=================================================================================================
// Constants 
//=================================================================================================
    static const string                    ALIGN_FIRST_OFFSET_SYMBOL = "a";
    static const array<optionparsing_t, 1> MY_OPTIONS =
    {
        { ALIGN_FIRST_OFFSET_SYMBOL, 1 }, //Align first entry to forced offset
    };

    static const string OUTPUT_FOLDER_SUFFIX; //= "_out";
    static const string EXE_NAME             = "ppmd_packfileutil.exe";
    static const string PVERSION             = "0.53";

//=================================================================================================
// Pack Handling
//=================================================================================================
    void DoUnpack( string inpath, string outpath )
    {
        CPack pack;
        vector<uint8_t> filedata;
        ReadFileToByteVector( inpath, filedata );

        cout << "\nUnpacking file : \n" 
            << "   " << inpath <<"\n"
		    <<"into:\n" 
            << "   " <<outpath <<"\n" <<endl;

        pack.LoadPack( filedata.begin(), filedata.end() );
        pack.OutputToFolder( outpath );
    }

    void DoPack( string inpath, string outpath, unsigned int forcedoffset )
    {
        CPack pack;
        vector<uint8_t> outfiledata;

	    cout << "\nPacking Directory : \n" 
                << "   " << inpath <<"\n"
			    <<"into:\n" 
                << "   " <<outpath <<"\n" <<endl;

        pack.LoadFolder( inpath );
        pack.setForceFirstFilePosition( forcedoffset );
        outfiledata = std::move( pack.OutputPack() ); //Explicit move constructor

        WriteByteVectorToFile( outpath, outfiledata );
    }



//=================================================================================================
// Utility
//=================================================================================================

    void PrintUsage()
    {
	    cout <<EXE_NAME <<" (option \"optionvalue\") \"inputpath\" \"outputpath\"\n"
             << "\n" 
             << "-> option          : An optional option from the list below..\n"
             << "-> optionvalue     : An optional value for the specified option..\n"
		     << "-> inputpath       : folder/file to pack/unpack.\n"
		     << "-> outputpath(opt) : folder/file to output the packed/unpacked file(s).\n"
             << "\n\n"
             << "Options:\n"
             << "      -" <<ALIGN_FIRST_OFFSET_SYMBOL <<" \"offset\" : Specifying this will make the program attempt to\n"
             << "                      align the first file to the specified offset\n"
             << "                      (offset is in heaxadecimal !) !\n"
             << "\n"
		     << "Example:\n"
             << "---------\n"
             << EXE_NAME <<" ./m_ground/\n"
		     << EXE_NAME <<" ./m_ground/ m_ground.bin\n"
             << EXE_NAME <<" -" <<ALIGN_FIRST_OFFSET_SYMBOL <<" 0x1300 ./ground/\n"
             << EXE_NAME <<" -" <<ALIGN_FIRST_OFFSET_SYMBOL <<" 0x1300 ./ground/ m_ground.bin\n"
             << "\n"
             << "To sum it up :\n"
             << "--------------\n"
		     << "- If a directory is passed as input path, the content will be packed.\n"
             << "- If a file is passed as input path, the file will be unpacked.\n\n"
             << "- If the option \"-" <<ALIGN_FIRST_OFFSET_SYMBOL <<"\" with a valid offset is found on the command line\n"
             << "  the pack file to be built will have its first sub-file aligned to that\n"
             << "  offset, provided the generated ToC length doesn't exceed this offset !\n"
             << "\n"
             << "More details:\n"
             << "--------------\n"
             << "-> If a folder to pack is passed as input, and if it ends with the \"" <<OUTPUT_FOLDER_SUFFIX <<"\"\n"
             << "   suffix, the suffix will be omitted in the outputed file's name!\n"
             << "-> Omitting the destination path will result in the program creating a\n" 
             << "   new file, or folder with the same name plus either a \"." <<pmd2::filetypes::PACK_FILEX <<"\"\n"
             << "   suffix for a file, or a \"" << OUTPUT_FOLDER_SUFFIX << "\" suffix for a directory.\n"
             << "-> In both cases, the output file/directory will be created in the input\n" 
             << "   path's parent directory.\n"
             << "-> All files in the input dir will be packed without exceptions, and they'll\n" 
             << "   be packed in alphabetical/numerical order.\n"
             << "-> When an output directory is specified, with an asterisk \"*\" appended,\n"
             << "   like so:\n" 
             << "       \"/path/to/directory/*\"\n"
             << "   Then a sub-folder with the name of the pack file being unpacked with\n"
             << "   \"" << OUTPUT_FOLDER_SUFFIX << "\" suffixed, will be created at the root of said path,\n"
             << "   containing the unpacked data!"
             << "-> When an output directory is specified, and no asterisk \"*\" is\n"
             << "   appended, like so:\n"
             << "       \"/path/to/directory\"\n"
             << "   Then the content of the packed file will be extracted directly\n"
             << "   in there!\n"
             << "\n"
             << "About The Optional Param:\n"
             << "----------------------------------------------------------\n"
             << "The optional parameter to align the first file at the specified offset\n"
             << "is essential for the files linked to pokemon sprites. Because the game\n"
             << "seems to have the first file offset hard-coded into it. Not re-packing\n"
             << "a file that needs it with this option, will result in no sprites being shown\n"
             << "in-game, shortly before freezing.\n\n"
             << "On the other hand, packing other files not related to pokemon sprites\n"
             << "with a forced first file offset showed no issues. But it doesn't mean its\n"
             << "safe to just force the offset on every single files..\n"
             << "\n"
             << "----------------------------------------------------------\n"
		     << "Named in honour of Baz, the awesome Poochyena of doom ! :D\n\n"
		     << "No crappyrights, all wrongs reversed !\n" 
             << "Sources and specs Included in original package!\n" <<endl;
    }

    bool HandleArguments( int argc, const char * argv[], string & inputpath, string & outputpath, unsigned int & forcedoffset )
    {
        CArgsParser parser( vector<optionparsing_t>( MY_OPTIONS.begin(), MY_OPTIONS.end() ), argv, argc );

        //We have 2 parameters max, at least one, and possibly one option
        string paramOne       = parser.getNextParam(),
               paramTwo       = parser.getNextParam();
        auto   validoptsfound = parser.getAllFoundOptions();

        if( !paramOne.empty() )
        {
            inputpath = paramOne;

            if( !utils::pathExists(inputpath) )
            {
                cerr <<"<!>-Fatal Error: Input path is invalid!\n";
                return false;
            }

            if( !paramTwo.empty() )
                outputpath = paramTwo;

            if( !validoptsfound.empty() && validoptsfound.front().size() == 2 )
            {
                stringstream sstr;
                unsigned int foffset = 0;

                sstr << validoptsfound.front()[1];
                if( validoptsfound.front()[1].find( "0x", 0 ) != string::npos )
                    sstr >> hex >> foffset;
                else
                    sstr >> foffset;

                if( foffset != 0 )
                    forcedoffset = foffset;
                else
                    cerr << "!-WARNING: Forced offset of 0 is invalid and will be ignored !!\n";
            }

            return true;
        }

        return false;
    }

    string PrepareOutputFileNameFromInput( string inputdirectory )
    {
        //if( has_suffix( inputdirectory, "\\" ) || has_suffix( inputdirectory, "/" ) )
        //    inputdirectory = inputdirectory.erase( (inputdirectory.size()-1) ); //erase the slash if there is one

        Poco::Path outputfile(inputdirectory);
        outputfile = outputfile.makeAbsolute().makeFile().setExtension(pmd2::filetypes::PACK_FILEX);
        return outputfile.toString(); //implicit move constructor call
    }

    string PrepareOutputFolderNameFromInput( const string & inputpath )
    {
        Poco::Path outputfolder(inputpath);
        string     foldername = outputfolder.getBaseName() + OUTPUT_FOLDER_SUFFIX; //filename without extension + suffix

        outputfolder = outputfolder.makeAbsolute().makeParent().pushDirectory(foldername);
        return outputfolder.toString(); //implicit move constructor call
    }

    string PrepareOutputFolderNameAsterisk( const string & inputpath,  const string & outputpathwithoutasterisk  )
    {
        //Make a subfolder to the specified output path, using the input filename
        Poco::Path inputfilepath(inputpath);
        Poco::Path outputfolder(outputpathwithoutasterisk);

        outputfolder = outputfolder.makeAbsolute().append( inputfilepath.getBaseName() + OUTPUT_FOLDER_SUFFIX );
        return outputfolder.toString();
    }

    //This determines what the output path should be considering the input path,
    // 
    string PrepareOutputPath( bool isPacking, const string & inputpath, const string & outputpath )
    {
        // check if output path empty
        if( !outputpath.empty() )
        {
            // Check if asterisk appended
            if( outputpath.back() == '*' )
            {
                string pathwithoutasterisk = outputpath.substr( 0, outputpath.size()-1 );

                if( isPacking ) //input path is folder
                    return PrepareOutputFileNameFromInput(pathwithoutasterisk);
                else //input path is file
                    return PrepareOutputFolderNameAsterisk( inputpath, pathwithoutasterisk );
            }
            else
            {
                if( isPacking ) //input path is folder
                    return outputpath;
                else //output path is folder
                    return TryAppendSlash(outputpath);
            }
        }
        else
        {
            //Make our own path
            if( isPacking )
            {
                if( has_suffix( inputpath, OUTPUT_FOLDER_SUFFIX ) ) //Remove the "_out" suffix that we add usually
                    return PrepareOutputFileNameFromInput( inputpath.substr( 0, inputpath.size() - OUTPUT_FOLDER_SUFFIX.size() ) );
                else
                    return PrepareOutputFileNameFromInput(inputpath);
            }
            else
                return PrepareOutputFolderNameFromInput(inputpath);
        }
    }
};

//=================================================================================================
// Main Function
//=================================================================================================
int main( int argc, const char * argv[] )
{
    using namespace ppmd_packfileutil;
    string       inputpath,
                    outputpath;
    unsigned int forcedoffset = 0;
    int          result       = 0;


	cout << "=================================================\n"
            << "== Baz the Poochyena's PMD:EoS Pack Tool - " <<PVERSION <<" ==\n" 
            << "=================================================\n"
            << "Utility for extracting and packing pack files!\n"
            << endl;

    //#1 - Get everything we need from the command line!
    if( !HandleArguments( argc, argv, inputpath, outputpath, forcedoffset ) )
    {
        PrintUsage();
        return -1;
    }
        
    //#2 - Determine whether we're packing something, or unpacking something!
    {
        MrChronometer mychrono("Total");
        if( isFolder( inputpath ) )
        {
            //We pack a folder
            DoPack( inputpath, PrepareOutputPath( true, inputpath, outputpath ), forcedoffset );
        }
        else
        {
            //We unpack a file
            DoUnpack( inputpath, PrepareOutputPath( false, inputpath, outputpath ) );
        }
    }

#ifdef _DEBUG
        utils::PortablePause();
#endif

    return result;
}