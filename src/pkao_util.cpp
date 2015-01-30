/*
pkao_util.cpp
2014/11/02
psycommando@gmail.com
Description:  Code for the kaomado utility
*/
#include "pkao_util.hpp"
#include <ppmdu/pmd2/pmd2_filetypes.hpp>
#include <ppmdu/fmts/kao.hpp>
#include <ppmdu/utils/utility.hpp>
#include <ppmdu/utils/library_wide.hpp>
#include <ppmdu/containers/tiled_image.hpp>
#include <ppmdu/utils/cmdline_util.hpp>
#include <ppmdu/ext_fmts/rawimg_io.hpp>
#include <ppmdu/ext_fmts/riff_palette.hpp>
#include <ppmdu/ext_fmts/supported_io.hpp>
#include <sstream>
#include <iostream>
#include <string>
#include <fstream>
#include <cassert>
#include <iomanip>
#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/Exception.h>
using namespace utils::cmdl;
using namespace utils::io;
using namespace std;
using namespace pmd2;
using namespace utils;

using filetypes::CKaomado;

namespace pkao_util
{
//=================================================================================================
// Constants 
//=================================================================================================
    static const string EXE_NAME                            = "ppmd_kaoutil.exe";
    static const string PVERSION                            = "0.4";

    static const string DEFAULT_FACENAMES_FILENAME          = "facenames.txt";
    static const string DEFAULT_POKENAMES_FILENAME          = "pokenames.txt";
    static const string OPTION_SET_TOC_ENTRY_NAME_LIST      = "pn";
    static const string OPTION_SET_FACE_NAME_LIST           = "fn";
    static const string OPTION_SET_TOTAL_NB_ENTRIES_KAO_TOC = "n";
    static const string OPTION_SET_EXPORT_TO_RAW_IMG        = "raw";
    static const string OPTION_SET_EXPORT_TO_BMP            = "bmp";
    static const string OPTION_QUIET                        = "q";
    static const string OPTION_NON_ZEALOUS_STR_SEARCH       = "nz";


    //Definition of all the possible options for the program!
    static const array<optionparsing_t, 7> MY_OPTIONS  =
    {{
        //Disable console output except errors!
        {
            OPTION_QUIET,
            0,
            "Disable console output except errors!",
        },
        //Set a text file to read for naming exported folders!
        { 
            OPTION_SET_TOC_ENTRY_NAME_LIST,      
            1,
            "Set a text file to read for naming exported folders!",
        }, 
        //Set a text file to read for the name of each exported faces within a folder!
        { 
            OPTION_SET_FACE_NAME_LIST,           
            1,
            "Set a text file to read for the name of each exported faces within a folder!",
        }, 
        //Force the ToC to this size.(Def = 1,154)
        { 
            OPTION_SET_TOTAL_NB_ENTRIES_KAO_TOC,
            1,
            "Force the ToC to this size.(Def = " + to_string( filetypes::DEF_KAO_TOC_NB_ENTRIES ) + ")",
        }, 
        //Will output the images as a raw 4bpp tiled image, preceeded by a RGB24 palette!
        { 
            OPTION_SET_EXPORT_TO_RAW_IMG,        
            0,
            "Will output the images as a raw 4bpp tiled \"." + RawImg_FileExtension + "\" image, along with a \"." + RIFF_PAL_Filext + "\" 16 colors RIFF palette.",
        },
        //Will make the program skip string search as often as possible
        {
            OPTION_NON_ZEALOUS_STR_SEARCH,
            0,
            "Disable zealous string search! Reduce compression efficiency, in favor of speed!",
        },
        //Will output the images as .bmp 4bpp images!
        {
            OPTION_SET_EXPORT_TO_BMP,
            0,
            "Will output the images as \".bmp\" 4bpp images!",
        },
    }};

    //A little struct to make it easier to throw around any new parsed parameters !
    struct kao_params
    {
        Poco::Path     inputpath;
        string         outputpath; 
        vector<string> pokenames;
        vector<string> facenames;
        uint32_t       totalnbkaoentries;
        bool           bshouldWriteRawImg;
        bool           bisQuiet;
        bool           bIsZealous;
        bool           bExportAsBmp;
    };


//=================================================================================================
// Utility
//=================================================================================================
//
    vector<string> ReadStringListFile( const Poco::Path & filepath )
    {
        vector<string> stringlist;
        ifstream       strfile( filepath.toString() );

        if( !( strfile.good() && strfile.is_open() ) )
        {
            std::stringstream strs;
            strs << "ReadStringListFile(): Error: stringlist file is missing or cannot be opened ! Path :\n"
                 << filepath.toString();
            throw runtime_error(strs.str());
        }

        while( !strfile.eof() && !strfile.bad() )
        {
            string tmp;
            getline( strfile, tmp );
            stringlist.push_back(tmp);
        }

        return std::move(stringlist);
    }


    void PrintUsage()
    {
	    cout << EXE_NAME <<" (option \"optionvalue\") \"inputpath\" \"outputpath\"\n"
             << "\n" 
             << "-> option          : An optional option from the list below..\n"
             << "-> optionvalue     : An optional value for the specified option..\n"
		     << "-> inputpath       : folder/file to pack/unpack.\n"
		     << "-> outputpath(opt) : folder/file to output the packed/unpacked file(s).\n"
             << "\n\n"
             << "Options:\n";
             //<< "      -" <<OPTION_SET_TOC_ENTRY_NAME_LIST 
             //          <<" \"filepath\" : Specifying this will make the program attempt to\n"
             //<< "                         read the specified file as a list of names to\n"
             //<< "                         give individual ToC entries(each folders).\n"
             //<< "                         Works only when unpacking!!"
             //<< "      -" <<OPTION_SET_FACE_NAME_LIST 
             //          <<" \"filepath\" : Specifying this will make the program attempt to\n"
             //<< "                         read the specified file as a list of names to\n"
             //<< "                         give each individual images for all ToC entries.\n"
             //<< "                         Works only when unpacking!!"
        //List our options
        for( auto & anoption : MY_OPTIONS )
            cout <<"    -" <<left  <<setw(5) <<setfill(' ') <<anoption.optionsymbol <<right <<" : " <<anoption.description <<"\n";

        cout << "\n"
		     << "Example:\n"
             << "---------\n"
             << EXE_NAME <<" ./kaomado.kao\n"
		     << EXE_NAME <<" ./kaomado.kao ./kaomado/\n"
             << EXE_NAME <<" -pn ./pokenames.txt ./kaomado.kao\n"
             << EXE_NAME <<" -fn ./facenames.txt ./kaomado.kao\n"
             << EXE_NAME <<" -pn ./pokenames.txt -fn ./facenames.txt ./kaomado.kao ./kaomado/\n"
             << "\n"
             << "To sum it up :\n"
             << "--------------\n"
             << "- If a file is passed as input path, the file will be unpacked.\n\n"
             << "\n"
             << "----------------------------------------------------------\n"
		     << "Named in honour of Baz, the awesome Poochyena of doom ! :D\n\n"
		     << "No crappyrights, all wrongs reversed !\n" <<endl;
    }

    bool HandleAnOption( const vector<string> & parsedoption, kao_params& parameters, string & facenamefile, string & pokenamefile )
    {
        bool success = false;

        if(  parsedoption.size() == 2 ) //Only take options that have an argument parsed
        {
            if( parsedoption.front().compare(OPTION_SET_TOC_ENTRY_NAME_LIST) == 0 )
            {
                //Set toc entry names
                pokenamefile = parsedoption.back();
                success = true;
                if( !parameters.bisQuiet )
                {
                    //parameters.pokenames = ReadStringListFile( anoption.back() );
                    cout <<"Option " <<OPTION_SET_TOC_ENTRY_NAME_LIST <<" specified. Using " 
                            <<parsedoption.back() <<" as source for folder names!\n";
                }
            }
            else if( parsedoption.front().compare(OPTION_SET_FACE_NAME_LIST) == 0 )
            {
                //Set faces names
                facenamefile = parsedoption.back();
                success = true;
                //parameters.facenames = ReadStringListFile( anoption.back() );
                if( !parameters.bisQuiet )
                {
                    cout <<"Option " <<OPTION_SET_FACE_NAME_LIST <<" specified. Using " 
                            <<parsedoption.back() <<" as source for face names!\n";
                }
            }
            else if( parsedoption.front().compare(OPTION_SET_TOTAL_NB_ENTRIES_KAO_TOC) == 0  )
            {
                //Overrides the default total amount of entries in the kaomado file
                stringstream strs;
                strs << parsedoption.back();
                strs >> parameters.totalnbkaoentries;
                success = true;
                if( !parameters.bisQuiet )
                {
                    cout <<"Option " <<OPTION_SET_TOTAL_NB_ENTRIES_KAO_TOC <<" specified. Using " 
                        <<parameters.totalnbkaoentries <<" as total nb of kaomado table entries!\n";
                }
            }
        }
        else if( parsedoption.size() == 1 ) //For any options with no parameters
        {
            if( parsedoption.front().compare(OPTION_SET_EXPORT_TO_RAW_IMG) == 0 )
            {
                //Overrides the default export to PNG, so it export to a raw tiled image instead
                parameters.bshouldWriteRawImg = true;
                success                       = true;
                if( !parameters.bisQuiet )
                {
                    cout <<"Option " <<OPTION_SET_EXPORT_TO_RAW_IMG 
                        <<" specified. Exporting to raw 16 color RGB palette + 4 bpp tiled image!\n";
                }
            }
            else if( parsedoption.front().compare(OPTION_NON_ZEALOUS_STR_SEARCH) == 0 )
            {
                parameters.bIsZealous = false;
                success               = true;
                if( !parameters.bisQuiet )
                {
                    cout <<"Option " <<OPTION_NON_ZEALOUS_STR_SEARCH 
                        <<" specified. Disabling zealous string search!\n";
                }
            }
            else if( parsedoption.front().compare(OPTION_SET_EXPORT_TO_BMP) == 0 )
            {
                parameters.bExportAsBmp = true;
                success                 = true;
                if( !parameters.bisQuiet )
                {
                    cout <<"Option " <<OPTION_SET_EXPORT_TO_BMP 
                        <<" specified. Exporting to 4bpp bitmaps!\n";
                }
            }
            else if( parsedoption.front().compare(OPTION_QUIET) == 0 )
            {
                parameters.bisQuiet = true;
                success             = true;
            }
        }

        return success;
    }

    bool HandleArguments( int argc, const char* argv[], kao_params& parameters )
    {
        CArgsParser parser( vector<optionparsing_t>( MY_OPTIONS.begin(), MY_OPTIONS.end() ), argv, argc );

        //We have 2 parameters max, at least one, and possibly one option
        string      paramOne       = parser.getNextParam(),
                    paramTwo       = parser.getNextParam();
        auto        validoptsfound = parser.getAllFoundOptions();
        string      facenamefile   = DEFAULT_FACENAMES_FILENAME,
                    pokenamefile   = DEFAULT_POKENAMES_FILENAME;
        bool        bsuccess       = false;

        //Handle evil backslash escaping the double quote
        if( has_suffix( paramOne, "\"" ) )
        {
            paramOne = paramOne.substr( 0, paramOne.size()-1 );
            cout << "<!>- Detected invalid trailing double-quote character in input path!\n"
                 << "     I'll fix the path for you, but don't put a trailling backslash\n"
                 << "     right before a double quote next time, its dangerous!\n"
                 << "     Use slashes instead!\n\n";
        }
        if( has_suffix( paramTwo, "\"" ) )
        {
            paramTwo = paramTwo.substr( 0, paramTwo.size()-1 );
            cout << "<!>- Detected invalid trailing double-quote character in output path!\n"
                 << "     I'll fix the path for you, but don't put a trailling backslash\n"
                 << "     right before a double quote next time, its dangerous!\n"
                 << "     Use slashes instead!\n\n";
        }


        if( !paramOne.empty() )
        {
            if( parameters.inputpath.tryParse(paramOne) )
            {

                //Parse first argument
                parameters.inputpath.makeAbsolute();

                //Check second argument !
                if( !paramTwo.empty()  )
                {
                    parameters.outputpath = paramTwo; //don't validate yet, we're doing it later
                }

                for( auto & anoption: validoptsfound )
                {
                    if( !HandleAnOption(anoption, parameters, facenamefile, pokenamefile ) )
                        cerr << "<!>-WARNING: Invalid option \"" <<anoption.front() <<"\" detected!\n";
                }
                bsuccess = true;
            }
            else
            {
                cerr << "<!>-Fatal Error: Input file or path invalid!\n";
                bsuccess = false;
            }

            //Fill up facenames and pokenames 
            try
            {
                parameters.facenames = ReadStringListFile( facenamefile );
            }
            catch(exception e)
            {
                cerr <<"<!>-Warning: The name list for giving human-readable names to the outputed images is missing or can't be read!\n"
                     <<e.what() <<"\n";
            }
            
            try
            {
                parameters.pokenames = ReadStringListFile( pokenamefile );
            }
            catch(exception e)
            {
                cerr <<"<!>-Warning: The name list for giving human-readable names to the outputed directories is missing or can't be read!\n"
                     <<e.what() <<"\n";
            }
            
        }

        return bsuccess;
    }

    //This determines what the output path should be considering the input path,
    // 
    Poco::Path PrepareOutputPath( bool isPacking, kao_params & parameters )
    {
        // check if output path empty
        if( !parameters.outputpath.empty() )
        {
            if( isPacking ) 
            {
                //Ouput path is file!
                Poco::Path outfile(parameters.outputpath);
                Poco::File testfile( Poco::Path(outfile).makeParent() );
                
                if( testfile.exists() && testfile.isDirectory() && !(outfile.getFileName().empty()) )
                    outfile.makeAbsolute();
                else
                    throw runtime_error("<!>-Fatal Error: Specified output path is invalid ! Aborting !");
                return std::move(outfile);
            }
            else 
            {
                //Output path is folder!
                Poco::Path outfolder(parameters.outputpath);
                Poco::File testfile(outfolder);

                if( testfile.exists() && testfile.isDirectory() )
                    outfolder.makeAbsolute();
                else
                    throw runtime_error("<!>-Fatal Error: Specified output path is invalid ! Aborting !");

                return std::move(outfolder);
            }
        }
        else
        {
            //Make our own path for packing  a folder to a file, and to unpack a file to a folder
            // input path was already validated
            if( isPacking )
            {
                //Output path is file!
                return Poco::Path(parameters.inputpath).makeFile().setExtension(filetypes::KAOMADO_FILEX);  //Turn into a filename, set the extension
            }
            else
            {
                //Output path is folder!
                return Poco::Path(parameters.inputpath).setFileName(parameters.inputpath.getBaseName()).makeDirectory(); //Remove extension, and turn into directory path
            }
        }
    }



//=================================================================================================
// Packing / Unpacking
//=================================================================================================

    bool DoUnpack( kao_params & parameters )
    {
        try
        {
            Poco::Path outpath = PrepareOutputPath( false, parameters );

            //Construct the byte vector and the kaomado object
            if( !parameters.bisQuiet )
                cout<<"Allocating..\n";
            CKaomado         kao;
            //types::bytevec_t filedata;

            //Read the file to the byte vector

            //ReadFileToByteVector( parameters.inputpath.toString(), filedata );

            //Parse the kaomado
            if( !parameters.bisQuiet )
            {
                cout << "\nUnpacking file : \n" 
                    << "   " << parameters.inputpath.toString() <<"\n"
		            <<"into:\n" 
                    << "   " <<outpath.toString() <<"\n" <<endl;
            }

            filetypes::KaoParser(parameters.bisQuiet)( parameters.inputpath.toString(), kao );
            //kao.ReadEntireKaomado( filedata.begin(), filedata.end(), parameters.bisQuiet );

            //Then depending on what the user gave us, we convert and output the kaomado data
            const vector<string> *ppokenames = ( (parameters.pokenames.empty() )? nullptr : &parameters.pokenames );
            const vector<string> *pfacenames = ( (parameters.facenames.empty() )? nullptr : &parameters.facenames );
            eSUPPORT_IMG_IO       outputTy;

            if( parameters.bshouldWriteRawImg )
                outputTy = eSUPPORT_IMG_IO::RAW;
            else if( parameters.bExportAsBmp )
                outputTy = eSUPPORT_IMG_IO::BMP;
            else
                outputTy = eSUPPORT_IMG_IO::PNG;

            filetypes::KaoWriter mywriter( ppokenames, pfacenames, true, parameters.bisQuiet );
            mywriter( kao, outpath.toString(), outputTy );

            if( !parameters.bisQuiet )
                cout << "Done! Deallocating..\n"; 
        }
        catch( Poco::Exception pe ) 
        {
            cerr << "DoUnpack(): Error: POCO(" <<pe.code() <<") " << pe.className() << "\n";
            pe.displayText();
            return false;
        }
        catch( std::exception e ) 
        { 
            cerr << "DoUnpack(): Error encountered while unpacking the file: \n    \"" 
                <<parameters.inputpath.toString()
                <<"\"\n    " <<e.what() <<"\n";
            return false;
        }

        return true;
    }

    bool DoPack( kao_params & parameters )
    {
        try
        {
            if( !parameters.bisQuiet )
                cout<<"Allocating..\n";
            CKaomado   kao(parameters.totalnbkaoentries);
            Poco::Path outpath = PrepareOutputPath( true, parameters );

            if( !parameters.bisQuiet )
            {
	            cout << "\nPacking Directory : \n"
                     << "   " <<parameters.inputpath.toString() <<"\n"
			         <<"into:\n" 
                     << "   " <<outpath.toString() <<"\n" <<endl;
            }

            filetypes::KaoParser(parameters.bisQuiet)( parameters.inputpath.toString(), kao );

            //kao.BuildFromFolder(parameters.inputpath.toString(), parameters.bisQuiet );
            //types::bytevec_t filedata = std::move( kao.WriteKaomado( parameters.bisQuiet, parameters.bIsZealous ) );

            if( !parameters.bisQuiet )
                cout<<"Writing to file..\n";

            filetypes::KaoWriter mywriter( nullptr, nullptr, true, parameters.bisQuiet );
            mywriter( kao, outpath.toString() );

            //WriteByteVectorToFile( outpath.toString(), filedata );


            if( !parameters.bisQuiet )
                cout << "Done! Deallocating..\n"; 
        }
        catch( Poco::Exception pe ) 
        {
            cerr << "DoPack(): Error: POCO(" <<pe.code() <<") " << pe.className() << "\n";
            pe.displayText();
            return false;
        }
        catch( std::exception e ) 
        { 
            cerr << "DoPack(): Error encountered while building the kaomado file from : \n    \"" 
                <<parameters.inputpath.toString()
                <<"\"\n    " <<e.what() <<"\n";
            return false;
        }
        return true;
    }
};

//=================================================================================================
// Main Function
//=================================================================================================
int main( int argc, const char * argv[] )
{
    using namespace pkao_util;
    int        result     = 0;
    kao_params parameters = 
    { 
        Poco::Path(),                       //inputpath;
        string(),                           //outputpath; 
        vector<string>(),                   //pokenames;
        vector<string>(),                   //facenames;
        filetypes::DEF_KAO_TOC_NB_ENTRIES,  //totalnbkaoentries;
        false,                              //bshouldWriteRawImg;
        false,                              //bisQuiet
        true,                               //bIsZealous
        false,                              //bExportAsBmp
    };

	cout << "================================================\n"
            << "== Baz the Poochyena's PMD:EoS Kao util - " <<PVERSION <<" ==\n" 
            << "================================================\n"
            << "Utility for unpacking and building kaomado files!\n"
            << endl;

    try
    {
        //#1 - Get everything we need from the command line!
        if( !HandleArguments( argc, argv, parameters ) )
        {
            PrintUsage();
            return -1;
        }
        
        //#2 - Determine whether we're packing something, or unpacking something!
        {
            MrChronometer mychrono("Total");
            Poco::File    pathTester(parameters.inputpath);

            if( pathTester.exists() )
            {
                if( pathTester.isDirectory() )
                {
                    //We pack a folder
                    if( ! DoPack( parameters ) )
                    {
                        cerr << "<!>- Packing failed! Aborting !\n";
                        result = -1;
                    }
                }
                else
                {
                    //We unpack a file
                    if( ! DoUnpack( parameters ) )
                    {
                        cerr << "<!>- Unpacking failed! Aborting !\n";
                        result = -1;
                    }
                }
            }
            else
            {
                cerr << "<!>- FATAL ERROR: Input path argument is invalid ! Path doesn't exists!\n    Invalid Argument:\n    \""
                        << parameters.inputpath.toString() << "\"\nAborting !" 
                        <<endl;
                result = -1;
            }
        }
    }
    catch( Poco::Exception pe )
    {
        cerr << "<!>- FATAL ERROR: POCO(" <<pe.code() <<") " <<pe.className() <<"\n";
        pe.displayText();
        return -1;
    }

#ifdef _DEBUG
    #ifdef WIN32
	    system("pause");
    #elif  _linux_
        char a;
        std::cin >> a;
    #endif
#endif
    return result;
}