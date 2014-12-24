#include "ppx_compressor.hpp"
#include <ppmdu/fmts/content_type_analyser.hpp>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <chrono>
#include <ppmdu/fmts/at4px.hpp>
#include <ppmdu/fmts/pkdpx.hpp>
#include <ppmdu/fmts/px_compression.hpp>
#include <ppmdu/utils/utility.hpp>
#include <ppmdu/pmd2/pmd2_palettes.hpp>
#include <ppmdu/utils/utility.hpp>
#include <ppmdu/utils/library_wide.hpp>
#include <Poco/Path.h>

#include <ppmdu/utils/cmdline_util.hpp>
using namespace utils::cmdl;

using namespace std;
using namespace pmd2;
using namespace pmd2::compression;
using namespace utils;

namespace ppx_compress
{
//=================================================================================================
//Function Declare
//=================================================================================================
    bool IsOutputFileHeaderPKDPX( const string & outputpath );


//=================================================================================================
// Constants
//=================================================================================================
    static const string                          OPTION_COMPRESSION_LVL = "l";
    static const string                          OPTION_ZEALOUS         = "z";
    static const string                          OPTION_QUIET           = "q";
    static const array<optionparsing_t,3>        MY_OPTIONS     = 
    {{
        //Option to disable progress output
        {
            OPTION_QUIET,
            0,
            "Disable progress output to console! Faster !",
        },
        //Sets the compression level to use
        {
            OPTION_COMPRESSION_LVL,
            1,
            "Set the compression level. It defaults at the maximum of 3..",
        }, 
        //Option to enable zealous string search
        {
            OPTION_ZEALOUS,
            0,
            "Prioritize compression efficiency over speed.\n Search for matching strings first, instead of\ntrying faster methods of compression first !", 
        },
    }};

    static const string EXE_NAME             = "ppmd_pxcomp.exe";
    static const string PVERSION             = "0.3";

    //A little struct to make it easier to throw around any new parsed parameters !
    struct pxcomp_params
    {
        Poco::Path     inputpath;
        Poco::Path     outputpath; 
        ePXCompLevel   compressionlvl;
        bool           isZealous;
        bool           isQuiet;
    };

//=================================================================================================
// Decompression Handlers
//=================================================================================================

    void DoCompress( vector<uint8_t>::const_iterator itdatabeg, vector<uint8_t>::const_iterator itdataend, const pxcomp_params & params )
                     //const string & infilenameonly, const string & outfilepat, ePXCompLevel compressionlvl, bool isZealous )
    {
        if( !params.isQuiet )
            cout << "\n-----------------------------------------------------------\n";

        vector<uint8_t> compressed;
        Poco::Path      infile( params.inputpath.getFileName() ),
                        outputfile(params.outputpath);

        //Write the approriate header first !
        if( params.outputpath.getExtension().compare(filetypes::AT4PX_FILEX) == 0 )
        {
            outputfile.setExtension(filetypes::AT4PX_FILEX );
            filetypes::CompressToAT4PX( itdatabeg, itdataend, compressed, params.compressionlvl, params.isZealous, !(params.isQuiet), false );
        }
        else //default to PKDPX in any case its not a AT4PX !
        {
            outputfile.setExtension(filetypes::PKDPX_FILEX );
            filetypes::CompressToPKDPX( itdatabeg, itdataend, compressed, params.compressionlvl, params.isZealous, !(params.isQuiet), false );
        }

        if( !params.isQuiet )
            cout <<"\nWriting data to : \n" << outputfile.toString() <<"\n\n";
        utils::WriteByteVectorToFile( outputfile.toString(), compressed );
    }

    void ReadAndCompressFile( const pxcomp_params & params ) // const string & inputpath, const string & outputpath, ePXCompLevel compressionlevel, bool isZealous )
    {
        vector<uint8_t> filedata;
        utils::ReadFileToByteVector( params.inputpath.toString(), filedata );

        DoCompress( filedata.begin(), filedata.end(), params ); //params.inputpath.getFileName(), outputpath, compressionlevel, isZealous );
    }

//=================================================================================================
// Utility
//=================================================================================================
    void PrintUsage()
    {
	    cout << EXE_NAME <<"  (option \"optionvalue\") \"inputpath\" \"outputpath\"\n\n"
             << "-> option(opt)     : An optional option from the list below..\n"
             << "-> optionvalue     : An optional value for the specified option..\n"
		     << "-> inputpath       : file to compress.\n"
		     << "-> outputpath(opt) : folder to output the file(s) to, or output filename.\n\n\n"
             << "Options:\n"
             << "   -" <<OPTION_COMPRESSION_LVL <<" (compression level) : Sets the compression level. Value from\n"
             << "                            0 to 3.\n"
             << "                             0 : Disable compression, only format data\n"
             << "                                 so the game can read it as a compressed\n"
             << "                                 file!\n"
             << "                             1 : Only compress sequences of 2\n"
             << "                                 similar bytes.\n"
             << "                             2 : The above, plus a few extra cases.\n"
             << "                             3 : All the above, plus enable matching\n"
             << "                                 string compression. Basically LZ..\n"
             << "   -"<<OPTION_ZEALOUS <<"                     : Zealous search. This means that instead\n"
             << "                            of avoiding searching through the \n"
             << "                            lookback buffer as often as possible it will\n"
             << "                            instead do it first everytimes! It results\n"
             << "                            in better compression efficiency, at the\n"
             << "                            cost of speed!\n"
             << "   -"<<OPTION_QUIET  <<"                     : Disable console progress output.\n"
             << "                            This will make the whole thing a little faster!\n"
		     << "Example:\n"
             <<EXE_NAME <<" ./file.txt\n"
		     <<EXE_NAME <<" ./file.sir0 ./\n"
             <<EXE_NAME <<" -l 3 ./file.sir0 ./\n"
             <<EXE_NAME <<" -l 3 -z ./file.sir0 ./\n"
             << "\n\n"
             << "Compresses files using PX compression(custom LZ?). Supports both AT4PX\n"
             << "and PKDPX output. By default, all files will be compressed to PKDPX,\n" 
             << "unless the output filename is specified and ends with the \".at4px\"\n"
             << "file extension !\n"
             << "----------------------------------------------------------\n"
		     << "Named in honour of Baz, the awesome Poochyena of doom, which was my\n"
             << "hero character in my PMD2 run ! :D\n"
		     << "No crappyrights, all wrongs reversed !\n"
             << "(In short, consider this Public Domain, or CC0!)\n"
             << "Sources and specs Included in original package!\n" <<endl;
    }


    bool HandleArguments( int argc, const char * argv[], pxcomp_params & params )// string & inputpath, string & outputpath, ePXCompLevel & compressionlvl, bool & isZealous )
    {
        //#0 - Handle options
        CArgsParser        argsparser( vector<optionparsing_t>( MY_OPTIONS.begin(), MY_OPTIONS.end() ), argv, argc );
        auto               optionsfound = argsparser.getAllFoundOptions();
        string             firstarg     = argsparser.getNextParam(),
                           secondarg    = argsparser.getNextParam();
        Poco::Path         inputfile,
                           outputfile;
        
        //#1 - Handle the parameters
        if( !firstarg.empty() )
        {
            if( inputfile.tryParse(firstarg) && inputfile.isFile() )
            {
                //Parse first argument
                params.inputpath = inputfile.makeAbsolute().toString();

                //Check second argument !
                if( !secondarg.empty() && outputfile.tryParse(secondarg) )
                {
                    params.outputpath = outputfile.makeAbsolute().toString();
                }
                else
                {
                    params.outputpath = Poco::Path(firstarg).makeParent().setBaseName( inputfile.getBaseName() ).toString(); //Get the directory the input file is in
                }

                //Get all valid command line options !
                for( auto & anoption : optionsfound )
                {
                    //If we want to set the compression level
                    if( anoption.size() == 2 && anoption.front().compare(OPTION_COMPRESSION_LVL) == 0 )
                    {
                        stringstream   strs;
                        unsigned int   clvl;
                        strs << anoption[1];
                        strs >> clvl;

                        //Verify if the compression lvl is valid
                        if( clvl >= static_cast<unsigned int>(ePXCompLevel::LEVEL_0) && 
                            clvl <= static_cast<unsigned int>(ePXCompLevel::LEVEL_3) )
                        {
                            if( !params.isQuiet )
                                cout<<"-" <<OPTION_COMPRESSION_LVL <<" specified, compressing using level " <<clvl <<" compression !\n";
                            params.compressionlvl = static_cast<ePXCompLevel>(clvl);
                        }
                        else
                        {
                            if( !params.isQuiet )
                            {
                                cout<<"-" <<OPTION_COMPRESSION_LVL <<" specified with invalid compression level.\nDefaulting to level " 
                                    <<static_cast<unsigned int>(ePXCompLevel::LEVEL_3) <<" compression !\n";
                            }
                            params.compressionlvl = ePXCompLevel::LEVEL_3; //Default to lvl 3 !
                        }

                    }

                    if( anoption.size() == 1 )
                    {
                        if( anoption.front().compare(OPTION_ZEALOUS) == 0 )
                        {
                            params.isZealous = true; //Don't put it outside the "if" or it will get reset to false every turns.. 

                            if( !params.isQuiet )
                                cout<<"-" <<OPTION_ZEALOUS <<" specified, enabling zealous search mode!\n";
                        }
                        else if(  anoption.front().compare(OPTION_QUIET) == 0 )
                        {
                            params.isQuiet = true; //Don't put it outside the "if" or it will get reset to false every turns.. 
                            //Don't echo anything at the console
                        }
                    }
                }
            }
            else
            {
                cerr << "<!>-Fatal Error: Input file or path invalid!\n";
                return false;
            }
        }
        else
        {
            PrintUsage();
            return false;
        }
        
        return true;
    }

    //If returns false, the header is AT4PX !
    //bool IsOutputFileHeaderPKDPX( const string & outputpath )
    //{
    //    Poco::Path outfile(outputpath);

    //    if( outfile.isFile() )
    //    {
    //        if( !(outfile.getExtension().empty()) && outfile.getExtension().compare( filetypes::AT4PX_FILEX ) == 0 )
    //            return false;
    //    }

    //    //No output filename specified, or pkdpx extension specified, default to PKDPX
    //    return true;
    //}
};
//=================================================================================================
// Main Function
//=================================================================================================
int main( int argc, const char * argv[] )
{
    using namespace ppx_compress;
    pxcomp_params params =
    {
        "",                     //Input path
        Poco::Path::current(),  //Output path
        ePXCompLevel::LEVEL_3,  //Compression level
        false,                  //Use zealous string search ?
        false,                  //Disable progress output
    };

	cout <<"==================================================\n"
            <<"==  Baz the Poochyena's PMD:EoS/T/D PXComp - "<<PVERSION <<" ==\n"
            <<"==================================================\n"
            <<"A PX compressor.\n"
            <<endl;

    if( HandleArguments( argc, argv, params ) )
    {
        MrChronometer mychrono("Total");
        ReadAndCompressFile( params );
    }
    else
    {
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

    return 0;
    }