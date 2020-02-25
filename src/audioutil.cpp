#include "audioutil.hpp"
#include <utils/utility.hpp>
#include <utils/cmdline_util.hpp>
#include <dse/dse_conversion.hpp>
#include <types/content_type_analyser.hpp>
#include <ppmdu/pmd2/pmd2_filetypes.hpp>
#include <utils/library_wide.hpp>
#include <ppmdu/fmts/swdl.hpp>
#include <dse/dse_conversion_info.hpp>
#include <dse/dse_interpreter.hpp>
#include <ppmdu/fmts/smdl.hpp>
#include <ppmdu/fmts/swdl.hpp>
#include <ppmdu/fmts/sedl.hpp>
#include <dse/bgm_container.hpp>
#include <ext_fmts/midi_fmtrule.hpp>

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>

#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/Exception.h>

#include <jdksmidi/world.h>
#include <jdksmidi/track.h>
#include <jdksmidi/multitrack.h>
#include <jdksmidi/filereadmultitrack.h>
#include <jdksmidi/fileread.h>
#include <jdksmidi/fileshow.h>
#include <jdksmidi/filewritemultitrack.h>

using namespace ::utils::cmdl;
using namespace ::utils::io;
using namespace ::std;
using namespace ::DSE;

/*
#TODO:
    - Make a single function for loading DSE data.
*/

namespace audioutil
{
//=================================================================================================
//  CAudioUtil
//=================================================================================================

//------------------------------------------------
//  Constants
//------------------------------------------------
    const string CAudioUtil::Exe_Name            = "ppmd_audioutil.exe";
    const string CAudioUtil::Title               = "Music and sound import/export tool.";
#ifdef _DEBUG
    const string CAudioUtil::Version             = AUDIOUTIL_VER" debug";
#else
    const string CAudioUtil::Version             = AUDIOUTIL_VER;
#endif
    const string CAudioUtil::Short_Description   = "A utility to export and import music and sounds from the PMD2 games.";
    const string CAudioUtil::Long_Description    = 
        "#TODO";
    const string CAudioUtil::Misc_Text           = 
        "Named in honour of Baz, the awesome Poochyena of doom ! :D\n"
        "My tools in binary form are basically Creative Commons 0.\n"
        "Free to re-use in any ways you may want to!\n"
        "No crappyrights, all wrongs reversed! :3";

    const int   CAudioUtil::MaxNbLoops           = 1200;


//------------------------------------------------
//  Arguments Info
//------------------------------------------------
    /*
        Data for the automatic argument parser to work with.
    */
    const vector<argumentparsing_t> CAudioUtil::Arguments_List =
    {{
        //Input Path argument
        { 
            0,      //first arg
            true,  //false == mandatory
            true,   //guaranteed to appear in order
            "input path", 
            "Path to the file/directory to export, or the directory to assemble.",
#ifdef WIN32
            "\"c:/pmd_romdata/data.bin\"",
#elif __linux__
            "\"/pmd_romdata/data.bin\"",
#endif
            std::bind( &CAudioUtil::ParseInputPath,       &GetInstance(), placeholders::_1 ),
            std::bind( &CAudioUtil::ShouldParseInputPath, &GetInstance(), placeholders::_1, placeholders::_2, placeholders::_3 ),
        },
        //Output Path argument
        { 
            1,      //second arg
            true,   //true == optional
            true,   //guaranteed to appear in order
            "output path", 
            "Output path. The result of the operation will be placed, and named according to this path!",
#ifdef WIN32
            "\"c:/pmd_romdata/data\"",
#elif __linux__
            "\"/pmd_romdata/data\"",
#endif
            std::bind( &CAudioUtil::ParseOutputPath,       &GetInstance(), placeholders::_1 ),
            std::bind( &CAudioUtil::ShouldParseOutputPath, &GetInstance(), placeholders::_1, placeholders::_2, placeholders::_3 ),
        },
    }};


//------------------------------------------------
//  Options Info
//------------------------------------------------

    const string OPTION_SwdlPathSym = "swdlpath";
    const string OPTION_SmdlPathSym = "smdlpath";
    const string OPTION_MkCvInfo    = "makecvinfo";
    const string OPTION_BgmCntPath  = "bgmcntpath";
    const string OPTION_BgmBlobPath = "blobpath";

    /*
        Information on all the switches / options to allow the automated parser 
        to parse them.
    */
    const vector<optionparsing_t> CAudioUtil::Options_List=
    {{
        //Tweak for General Midi format
        {
            "gm",
            0,
            "Specifying this will modify the MIDIs to fit as much as possible with the General Midi standard. "
            "Instruments remapped, and etc.. In short, the resulting MIDI will absolutely not "
            "be a 1:1 copy, and using the original samples with this MIDI won't sound good either.. "
            "WARNING: General MIDI offers 16 tracks, with one forced to play drums. But there isn't a track "
            "forced to play drums in the DSE system. So, some songs with just melody tracks simply can't be "
            "converted to GM and play correctly!!",
            "-gm",
            std::bind( &CAudioUtil::ParseOptionGeneralMidi, &GetInstance(), placeholders::_1 ),
        },

        //Force Loops
        {
            "fl",
            1,
            "This will export to the MIDI file the \"intro\" once, followed with the notes "
            "in-between the loop and end marker the specified number of times. Loop markers will also be omitted!",
            "-fl (nbofloops)",
            std::bind( &CAudioUtil::ParseOptionForceLoops, &GetInstance(), placeholders::_1 ),
        },

        //ExportPMD2
        {
            "pmd2",
            0,
            "Specifying this will tell the program that that input path is the root of the extracted ROM's \"data\" directory."
            "The utility will export the audio content from the entire game's \"/SOUND\" directory, to a bunch of MIDIs and some soundfonts!",
            "-pmd2",
            std::bind( &CAudioUtil::ParseOptionPMD2, &GetInstance(), placeholders::_1 ),
        },

        //SetPathToCvInfo
        {
            "cvinfo",
            1,
            "The cvinfo file is an XML file containing information on how to convert SMDL files to MIDI. "
            "It allows to change the original presets/bank to something else automatically."
            " It also allows to transpose notes by a certain preset, or change what key/preset "
            "is used when a certain key for a given original preset is played. See the example file for more details!",
            "-cvinfo \"path/to/conversion/info/file.xml\"",
            std::bind( &CAudioUtil::ParseOptionPathToCvInfo, &GetInstance(), placeholders::_1 ),
        },

        //MakeCvInfo
        {
            OPTION_MkCvInfo,
            0,
            "This will build a blank populated cvinfo.xml file from all the loaded swdl files specified using the " + OPTION_SwdlPathSym + " option."
            " or the " + OPTION_BgmCntPath + ". The first parameter specified on the commandline will be the name of the outputed cvinfo.xml file!",
            "-" + OPTION_MkCvInfo,
            std::bind( &CAudioUtil::ParseOptionMakeCvinfo, &GetInstance(), placeholders::_1 ),
        },

        //ForceMidi
        {
            "forcemidi",
            0,
            "Specifying this will force a MIDI only export, regardless of the parameters specified."
            "This is useful when loading bgm containers for example, and wanting to export them to MIDIs"
            "according or not to the rules specified in a cvinfo XML file.",
            "-forcemidi",
            std::bind( &CAudioUtil::ParseOptionForceMidi, &GetInstance(), placeholders::_1 ),
        },

        //Export Main Bank And Tracks
        //{
        //    "mbat",
        //    0,
        //    "Specifying this will tell the program that that input path is the root of the directory containing a main bank and its tracks."
        //    "The utility will export everything to MIDI files, and to a Sounfont file.",
        //    "-mbat",
        //    std::bind( &CAudioUtil::ParseOptionMBAT, &GetInstance(), placeholders::_1 ),
        //},


        //!#TODO : New Implementation for loading DSE stuff. Work in progress!
        //#################################################

        //Set Main Bank
        {
            "mbank",
            1,
            "Use this to specify the path to the main sample bank that the SMDL to export will use, if applicable!"
            "Is also used to specify where to put the assembled ",
            "-mbank \"SOUND/BGM/bgm.swd\"",
            std::bind( &CAudioUtil::ParseOptionMBank, &GetInstance(), placeholders::_1 ),
        },

        //Set SWDLPath
        {
            OPTION_SwdlPathSym,
            1,
            "Use this to specify the path to the folder where the SWDLs matching the SMDL to export are stored."
            "Is also used to specify where to put assembled DSE Preset during import.",
            "-swdlpath \"SOUND/BGM\"",
            std::bind( &CAudioUtil::ParseOptionSWDLPath, &GetInstance(), placeholders::_1 ),
        },

        //Set SMDLPath
        {
            OPTION_SmdlPathSym,
            1,
            "Use this to specify the path to the folder where the SMDLs to export are stored."
            "Is also used to specify where to put MIDI files converted to SMDL format.",
            "-smdlpath \"SOUND/BGM\"",
            std::bind( &CAudioUtil::ParseOptionSMDLPath, &GetInstance(), placeholders::_1 ),
        },

        //Set bgmcnt path
        {
            OPTION_BgmCntPath,
            2,
            "Use this to specify where to load bgm containers from. Aka, SMDL + SWDL wrapped together in a SIR0 wrapper."
            "The second argument is the file extension to look for, as most bgm containers have different file extensions!",
            "-bgmcntpath \"Path/to/BGMsDir\" \"bgm\"",
            std::bind( &CAudioUtil::ParseOptionBGMCntPath, &GetInstance(), placeholders::_1 ),
        },

        //Set BlobBGMPath
        {
            OPTION_BgmBlobPath,
            1,
            "Use this to specify the path to a blob of DSE containers to process. AKA a file that contains a bunch of"
            "SMDL, SWDL, SEDL containers one after the other with no particular structure."
            "You can also specify a folder or use several -blobpath to designate several files to be handled."
			"When using a folder, all files within that directory with no exception will be scanned and converted, so use with caution!",
            "-blobpath \"Path/to/blob/file.whatever\"",
            std::bind( &CAudioUtil::ParseOptionBlobPath, &GetInstance(), placeholders::_1 ),
        },

        //Export SWDL Preset + Sample List
        {
            "listpres",
            0,
            "Use this to make the program export a list of all the presets used by the SWDL contained within"
            "the swdlpath specified, or the input path if doing it for a single SWDL. "
            "If a swdlpath was specified, the path to the outputed list is the first argument."
            "If no swdlpath is specified the first argument is the path to the swdl to use, and"
            "optionally the second argument the path to the outputed list!",
            "-listpres",
            std::bind( &CAudioUtil::ParseOptionListPresets, &GetInstance(), placeholders::_1 ),
        },

        //HexNumbers
        {
            "hexnum",
            0,
            "If this is present, whenever possible, the program will export filenames with hexadecimal numbers in them!",
            "-hexnum",
            std::bind( &CAudioUtil::ParseOptionUseHexNumbers, &GetInstance(), placeholders::_1 ),
        },

        //sf2
        {
            "sf2",
            0,
            "Specifying this will export a swd soundbank to a soundfont if applicable.",
            "-sf2",
            std::bind( &CAudioUtil::ParseOptionOutputSF2, &GetInstance(), placeholders::_1 ),
        },

        //xml -> This is mainly useful when reimporting music tracks into the game!
        {
            "xml",
            0,
            "Specifying this will force outputing a swd soundbank to XML data and pcm16 samples(.wav files).",
            "-xml",
            std::bind( &CAudioUtil::ParseOptionOutputXML, &GetInstance(), placeholders::_1 ),
        },

        //nobake -> This disables sample baking
        {
            "nobake",
            0,
            "Specifying this will disable the rendering of individual samples for every preset split.",
            "-nobake",
            std::bind( &CAudioUtil::ParseOptionNoSampleBake, &GetInstance(), placeholders::_1 ),
        },

        //nofx
        {
            "nofx",
            0,
            "Specifying this will disable the processing of LFO effects data.",
            "-nofx",
            std::bind( &CAudioUtil::ParseOptionNoFX, &GetInstance(), placeholders::_1 ),
        },

        //noconvert
        {
            "noconvert",
            0,
            "Specifying this will disable the convertion of samples to pcm16 when exporting a bank. They'll be exported in their original format.",
            "-noconvert",
            std::bind( &CAudioUtil::ParseOptionNoConvertSamples, &GetInstance(), placeholders::_1 ),
        },

        //match blob scanned containers by internal name
        {
            "nmatchoff",
            0,
            "Specifying this will make the program match pairs in a blob by order instead of by internal name! Needed for some games with strange naming conventions.",
            "-nmatchoff",
            std::bind(&CAudioUtil::ParseOptionMatchByName, &GetInstance(), placeholders::_1),
        },

        //#################################################

        //Redirect clog to file
        {
            "log",
            1,
            "This option writes a log to the file specified as parameter.",
            "-log \"logfilename.txt\"",
            std::bind( &CAudioUtil::ParseOptionLog, &GetInstance(), placeholders::_1 ),
        },

        //Verbose output
        {
            "v",
            0,
            "This enables the writing of a lot more info to the logfile!",
            "-v",
            std::bind( &CAudioUtil::ParseOptionVerbose, &GetInstance(), placeholders::_1 ),
        },
    }};


//------------------------------------------------
// Misc Methods
//------------------------------------------------

    CAudioUtil & CAudioUtil::GetInstance()
    {
        static CAudioUtil s_util;
        return s_util;
    }

    CAudioUtil::CAudioUtil()
        :CommandLineUtility()
    {
        m_operationMode   = eOpMode::Invalid;
        m_bGM             = false;
        m_isPMD2          = false;
        m_isListPresets   = false;
        m_useHexaNumbers  = false;
        m_bBakeSamples    = true;
        m_bUseLFOFx       = true;
        m_bMakeCvinfo     = false;
        m_bConvertSamples = true;
        m_bmatchbyname    = true;
        m_nbloops         = 0;
        m_outtype         = eOutputType::SF2;
    }

    const vector<argumentparsing_t> & CAudioUtil::getArgumentsList   ()const { return Arguments_List;    }
    const vector<optionparsing_t>   & CAudioUtil::getOptionsList     ()const { return Options_List;      }
    const argumentparsing_t         * CAudioUtil::getExtraArg        ()const { return nullptr;           } //No extra args
    const string                    & CAudioUtil::getTitle           ()const { return Title;             }
    const string                    & CAudioUtil::getExeName         ()const { return Exe_Name;          }
    const string                    & CAudioUtil::getVersionString   ()const { return Version;           }
    const string                    & CAudioUtil::getShortDescription()const { return Short_Description; }
    const string                    & CAudioUtil::getLongDescription ()const { return Long_Description;  }
    const string                    & CAudioUtil::getMiscSectionText ()const { return Misc_Text;         }

//------------------------------------------------
//  Utility
//------------------------------------------------

    /*
        MakeOutputDirectory
            Create the output directory if necessary.
    */
    void CreateOutputDir( const std::string & outputdir )
    {
        Poco::File outdir(outputdir);
        if( !outdir.exists() )
        {
            if( outdir.createDirectory() )
                cout << "<*>- Created output directory \"" << outdir.path() <<"\" !\n";
            else
                cout << "<!>- Couldn't create output directory \"" << outdir.path() <<"\" !\n";
        }
        else if( !outdir.isDirectory() )
            throw std::runtime_error( "Error, output path " + outputdir + " already exists, but not as a directory!" );
    }

    /*
        ExportASequenceToMidi
            Convenience function for exporting SMDL to MIDI
    */
    void ExportASequenceToMidi( const MusicSequence & seq, 
                                const string          pairname, 
                                Poco::Path            outputfile, 
                                const std::string   & convinfo, 
                                int                   nbloops, 
                                bool                  asGM )
    {
        DSE::eMIDIMode convmode = (asGM)? DSE::eMIDIMode::GM : DSE::eMIDIMode::GS;

        if( asGM )
            clog << "<*>- Conversion mode set to General MIDI instead of the default Roland GS!\n";
        else
            clog << "<*>- Conversion mode set to Roland GS!\n";

        //Check if we have conversion info supplied
        if( ! convinfo.empty() )
        {
            DSE::SMDLConvInfoDB cvinf( convinfo );
            auto itfound = cvinf.FindConversionInfo( pairname );

            if( itfound != cvinf.end() )
            {
                clog << "<*>- Got conversion info for this track! MIDI will be remapped accordingly!\n";
                DSE::SequenceToMidi( outputfile.toString(), seq, itfound->second, nbloops, convmode );
                return;
            }
            else
                clog <<"<!>- Couldn't find an entry for this SMD + SWD pair! Falling back to converting as-is..\n";
        }
        else
            clog << "<*>- Received no conversion info for this track! The SMDL will be exported as-is!\n";

        DSE::SequenceToMidi( outputfile.toString(), seq, nbloops, convmode );
    }

    inline bool SameFileExt( std::string ext1, std::string ext2, std::locale curloc = locale::classic() )
    {
        auto lambdacmpchar = [&curloc]( char c1, char c2 )->bool
        {
            return (std::tolower(c1, curloc ) == std::tolower(c2, curloc ));
        };

        //Pre-check to make sure we even have the same lengths
        if (ext1.length() != ext2.length())
            return false;

        //std::transform(ext1.begin(), ext1.end(), ext1.begin(), ::tolower);
        //std::transform(ext2.begin(), ext2.end(), ext2.begin(), ::tolower);
        //return ext1 == ext2;
        return std::equal( ext1.begin(), ext1.end(), ext2.begin(), lambdacmpchar );
    }

    /*
        ProcessAllFilesWithExtInDir
            Run the specified function on each files in the directory with the matching extension!
    */
    void ProcessAllFilesWithExtInDir( const Poco::Path                        & dir, 
                                      const std::string                       & ext,
                                      const std::string                       & desc,       //Word displayed on the console to describle the action. Like "Exporting"
                                      std::function<void(const Poco::Path&)> && fun )
    {
        stringstream            sstrdesc;
        sstrdesc << "\r" <<desc <<" ";
        const std::string       descstr = sstrdesc.str(); //We cut on rebuilding the string each turns
        Poco::DirectoryIterator itdir(dir);
        Poco::DirectoryIterator itend;

        for( ; itdir != itend; ++itdir )
        {
            if( itdir->isFile() )
            {
                Poco::Path curpath(itdir->path());
                if( SameFileExt( curpath.getExtension(), ext ) )
                {
                    cout <<right <<setw(60) <<setfill(' ') << descstr << curpath.getBaseName() << "..\n";
                    fun(curpath);
                }
            }
        }
    }

//------------------------------------------------
//  Parse Args
//------------------------------------------------
    bool CAudioUtil::ParseInputPath( const string & path )
    {
        Poco::Path inputfile(path);

        //check if path exists
        if( /*inputfile.exists() &&*/ ( inputfile.isFile() || inputfile.isDirectory() ) )
        {
            m_inputPath = path;
            return true;
        }

        return false;
    }

    /*
        When we have some specific options specified, we don't need the input path argument!
    */
    bool CAudioUtil::ShouldParseInputPath( const vector<vector<string>> & options,  
                                           const deque<string>          & priorparams, 
                                           size_t                         nblefttoparse )
    {
        auto itfoundinswitch = std::find_if( options.begin(), 
                                             options.end(), 
                                             [](const vector<string>& curopt)
                                             { 
                                                return ( curopt.front() == OPTION_SwdlPathSym ) || 
                                                       ( curopt.front() == OPTION_SmdlPathSym ) ||
                                                       ( curopt.front() == OPTION_BgmCntPath )  ||
                                                       ( curopt.front() == OPTION_BgmBlobPath ) ||
                                                       ( curopt.front() == OPTION_MkCvInfo ); 

                                             } );

        //If we have an input path option, we do not need the input path parameter!
        return ( itfoundinswitch == options.end() );
    }

    bool CAudioUtil::ShouldParseOutputPath( const vector<vector<string>> & options, 
                                            const deque<string>          & priorparams, 
                                            size_t                         nblefttoparse )
    {
        auto itfoundinswitch = std::find_if( options.begin(), 
                                             options.end(), 
                                             [](const vector<string>& curopt)
                                             { 
                                                return ( curopt.front() == OPTION_SwdlPathSym ) || 
                                                       ( curopt.front() == OPTION_SmdlPathSym ) ||
                                                       ( curopt.front() == OPTION_BgmCntPath )  ||
                                                       ( curopt.front() == OPTION_BgmBlobPath ) ||
                                                       ( curopt.front() == OPTION_MkCvInfo ); 
                                             } );

        //If we have an input path option or parameter, and we still have parameters left to parse, this param should be parsed!
        return ( (itfoundinswitch != options.end()) || (priorparams.size() >= 1) ) 
                && (nblefttoparse > 0);
    }
    
    bool CAudioUtil::ParseOutputPath( const string & path )
    {
        Poco::Path outpath(path);

        if( outpath.isFile() || outpath.isDirectory() )
        {
            m_outputPath = path;
            return true;
        }
        return false;
    }


//------------------------------------------------
//  Parse Options
//------------------------------------------------

    bool CAudioUtil::ParseOptionGeneralMidi( const std::vector<std::string> & optdata )
    {
        m_bGM = true;
        return true;
    }

    bool CAudioUtil::ParseOptionForceLoops ( const std::vector<std::string> & optdata )
    {
        stringstream conv;
        conv << optdata[1];
        conv >> m_nbloops;

        if( m_nbloops >= 0 && m_nbloops < MaxNbLoops )
            return true;
        else
        {
            if( m_nbloops > MaxNbLoops )
                cerr <<"Too many loops requested! " <<m_nbloops <<" loops is a little too much to handle !! Use a number below " <<MaxNbLoops <<" please !\n";
            return false;
        }
    }

    bool CAudioUtil::ParseOptionPMD2( const std::vector<std::string> & optdata )
    {
        m_isPMD2 = true;
        return true;
    }

    bool CAudioUtil::ParseOptionMBAT( const std::vector<std::string> & optdata )
    {
        m_operationMode = eOpMode::ExportSWDLBank;
        return true;
    }

    bool CAudioUtil::ParseOptionLog( const std::vector<std::string> & optdata )
    {
        Poco::Path outpath(optdata[1]);
        if( outpath.isFile() )
        {
            Poco::File OutputDir( outpath.parent().makeAbsolute() );
            if( !OutputDir.exists() )
            {
                if( !OutputDir.createDirectory() )
                {
                    throw runtime_error( "Couldn't create output directory for log file!");
                }
            }
                

            m_redirectClog.Redirect(optdata[1]);
            utils::LibWide().isLogOn(true);
            return true;
        }
        else
        {
            cerr << "<!>- ERROR: Invalid path to log file specified! Path is not a file!\n";
            return false;
        }
    }

    bool CAudioUtil::ParseOptionVerbose( const std::vector<std::string> & optdata )
    {
        utils::LibWide().setVerbose(true);
        return true;
    }

    bool CAudioUtil::ParseOptionPathToCvInfo( const std::vector<std::string> & optdata )
    {
        Poco::File cvinfof( Poco::Path( optdata[1] ).makeAbsolute() );
        
        if( cvinfof.exists() && cvinfof.isFile() )
            m_convinfopath = optdata[1];
        else
        {
            cerr << "<!>- ERROR: Invalid path to cvinfo file specified !\n";
            return false;
        }
        return true;
    }

    bool CAudioUtil::ParseOptionMBank( const std::vector<std::string> & optdata )
    {
        Poco::File bank( Poco::Path( optdata[1] ).makeAbsolute() );
        
        if( bank.exists() && bank.isFile() )
            m_mbankpath = optdata[1];
        else
        {
            cerr << "<!>- ERROR: Invalid path to main swdl bank specified !\n";
            return false;
        }
        return true;
    }
    
    bool CAudioUtil::ParseOptionSWDLPath( const std::vector<std::string> & optdata )
    {
        Poco::File swdldir( Poco::Path( optdata[1] ).makeAbsolute() );
        
        if( swdldir.exists() && swdldir.isDirectory() )
            m_swdlpath = optdata[1];
        else
        {
            cerr << "<!>- ERROR: Invalid path to swdl directory specified !\n";
            return false;
        }
        return true;
    }

    bool CAudioUtil::ParseOptionSMDLPath( const std::vector<std::string> & optdata )
    {
        Poco::File smdldir( Poco::Path( optdata[1] ).makeAbsolute() );
        
        if( smdldir.exists() && smdldir.isDirectory() )
            m_smdlpath = optdata[1];
        else
        {
            cerr << "<!>- ERROR: Invalid path to smdl directory specified !\n";
            return false;
        }
        return true;
    }

    bool CAudioUtil::ParseOptionListPresets( const std::vector<std::string> & optdata )
    {
        m_isListPresets = true;
        return true;
    }

    bool CAudioUtil::ParseOptionUseHexNumbers( const std::vector<std::string> & optdata )
    {
        m_useHexaNumbers = true;
        return true;
    }

    bool CAudioUtil::ParseOptionOutputSF2( const std::vector<std::string> & optdata )
    {
        m_outtype = eOutputType::SF2;
        return true;
    }
    
    bool CAudioUtil::ParseOptionOutputXML( const std::vector<std::string> & optdata )
    {
        m_outtype = eOutputType::XML;
        return true;
    }

    bool CAudioUtil::ParseOptionNoSampleBake( const std::vector<std::string> & optdata )
    {
        m_bBakeSamples = false;
        return true;
    }

    bool CAudioUtil::ParseOptionNoFX( const std::vector<std::string> & optdata )
    {
        m_bUseLFOFx = false;
        return true;
    }

    bool CAudioUtil::ParseOptionMakeCvinfo( const std::vector<std::string> & optdata )
    {
        m_operationMode = eOpMode::MakeCvInfo;
        //m_bMakeCvinfo = true;
        return true; 
    }

    bool CAudioUtil::ParseOptionBGMCntPath( const std::vector<std::string> & optdata )
    {
        Poco::File bgmcntpath( Poco::Path( optdata[1] ).makeAbsolute() );

        if( bgmcntpath.exists() && bgmcntpath.isDirectory() )
        {
            m_bgmcntpath = optdata[1];
            m_bgmcntext  = optdata[2];
        }
        else
        {
            cerr << "<!>- ERROR: Invalid path to bgm container directory specified !\n";
            return false;
        }
        return true;
    }

    bool CAudioUtil::ParseOptionForceMidi( const std::vector<std::string> & optdata )
    {
        m_outtype = eOutputType::MIDI_Only;
        return true;
    }

    bool CAudioUtil::ParseOptionBlobPath( const std::vector<std::string> & optdata )
    {
        Poco::File bgmblobpath( Poco::Path( optdata[1] ).makeAbsolute() );

        if( bgmblobpath.exists() )
        {
            m_bgmblobpath.push_back(optdata[1]);
        }
        else
        {
            cerr << "<!>- ERROR: Invalid path to bgm blob file specified !\n";
            return false;
        }
        return true;
    }

    bool CAudioUtil::ParseOptionNoConvertSamples( const std::vector<std::string> & optdata )
    {
        return m_bConvertSamples = true;
    }

    bool CAudioUtil::ParseOptionMatchByName(const std::vector<std::string>& optdata)
    {
        m_bmatchbyname = false;
        return true;
    }

//------------------------------------------------
//  Program Setup and Execution
//------------------------------------------------
    int CAudioUtil::GatherArgs( int argc, const char * argv[] )
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
            cerr <<"\n<!>-POCO Exception - " <<pex.name() <<"(" <<pex.code() <<") : " << pex.message() <<"\n"
                 <<"You can print the readme by calling the program with no arguments!\n" <<endl;
            //cout <<"=======================================================================\n"
            //     <<"Readme\n"
            //     <<"=======================================================================\n";
            //PrintReadme();
            return pex.code();
        }
        catch( exception e )
        {
            cerr <<"\n<!>-Exception: " << e.what() <<"\n"
                 <<"You can print the readme by calling the program with no arguments!\n" <<endl;
            //cout <<"=======================================================================\n"
            //     <<"Readme\n"
            //     <<"=======================================================================\n";
            //PrintReadme();
            return -3;
        }
        return returnval;
    }

    void CAudioUtil::DetermineOperation()
    {
        using namespace pmd2::filetypes;
        using namespace filetypes;
        Poco::Path inpath( m_inputPath );
        Poco::File infile( inpath.absolute() );

        if( m_operationMode != eOpMode::Invalid )
            return; //Skip if we have a forced mode

        if( !m_outputPath.empty() && !Poco::File( Poco::Path( m_outputPath ).makeAbsolute().parent() ).exists() )
            throw runtime_error("Specified output path does not exists!");

        // ---- Handle cases when the blob container path was specified ----
        if( !m_bgmblobpath.empty() )
        {
            if( m_bMakeCvinfo )
                m_operationMode = eOpMode::MakeCvInfo;       //Make a blank CVinfo
            else
                m_operationMode = eOpMode::ExportBatchPairs; //We exports bgm cnts
            return;
        }

        // ---- Handle cases when the bgm container path was specified ----
        if( !m_bgmcntpath.empty() && !m_bgmcntext.empty() )
        {
            if( m_bMakeCvinfo )
                m_operationMode = eOpMode::MakeCvInfo;       //Make a blank CVinfo
            else
                m_operationMode = eOpMode::ExportBatchPairs; //We exports bgm cnts
            return;
        }

        // ---- Handle cases when the swdl, smdl and/or mainbank paths are specified! ----
        if( !m_mbankpath.empty() && !m_smdlpath.empty() && !m_swdlpath.empty() )
        {
            m_operationMode = eOpMode::ExportBatchPairsAndBank; //We exports pairs with a main bank
            return;
        }
        else if( !m_smdlpath.empty() && !m_swdlpath.empty() )
        {
            m_operationMode = eOpMode::ExportBatchPairs;        //We exports pairs without a main bank
            return;
        }
        else if( !m_swdlpath.empty() )
        {
            //Batch SWDL Export, or Batch SWDL listing
            if( m_isListPresets )
                m_operationMode = eOpMode::BatchListSWDLPrgm;   //List SWDL samples and programs
            else if( m_bMakeCvinfo )
                m_operationMode = eOpMode::MakeCvInfo;          //Make a blank CVinfo
            else
                m_operationMode = eOpMode::ExportBatchSWDL;     //Export a batch of SWDL files only !
            //m_outputPath = m_inputPath;                         //The only parameter will be the output
            return;
        }
        else if( !m_smdlpath.empty() )
        {
            //Batch SMDL only export
            m_operationMode = eOpMode::ExportBatchSMDL;         //Export a batch of SMDL files only !
            //m_outputPath = m_inputPath;                         //The only parameter will be the output
            return;
        }

        // ---- If the above fails, try to guess what to do by input type! ----
        if( infile.exists() )
        {
            if( infile.isFile() )
            {
                //Working on a file
                vector<uint8_t> tmp    = utils::io::ReadFileToByteVector(infile.path());
                auto            cntty  = DetermineCntTy(tmp.begin(), tmp.end(), inpath.getExtension());

                if( cntty._type == static_cast<unsigned int>(filetypes::CnTy_SMDL) )
                {
                    m_operationMode = eOpMode::ExportSMDL;
                }
                else if( cntty._type == static_cast<unsigned int>(filetypes::CnTy_SWDL) )
                {
                    if( m_isListPresets )
                        m_operationMode = eOpMode::ListSWDLPrgm;
                    else
                        m_operationMode = eOpMode::ExportSWDL;
                }
                else if( cntty._type == static_cast<unsigned int>(filetypes::CnTy_SEDL) )
                {
                    m_operationMode = eOpMode::ExportSEDL;
                }
                else if( cntty._type == static_cast<unsigned int>(filetypes::CnTy_MIDI) )
                {
                    m_operationMode = eOpMode::BuildSMDL;
                }
                else 
                    throw runtime_error("Unknown file format!");
            }
            else if( infile.isDirectory() )
            {
                if( m_isPMD2 )
                {
                    //Find if the /SOUND sub-directory exists
                    Poco::File soundir = Poco::Path(inpath).append("SOUND");

                    if( soundir.exists() && soundir.isDirectory() )
                    {
                        m_operationMode = eOpMode::ExportPMD2;
                    }
                    else
                        throw std::runtime_error("Couldn't find the directory \"./SOUND\" under the path specified!");
                }
                else
                {
                    //Handle assembling things
                    throw std::exception("Feature not implemented yet!");
                }
            }
            else
                throw runtime_error("Cannot determine the desired operation!");
        }
        else
            throw runtime_error("The input path does not exists!");

    }

    int CAudioUtil::Execute()
    {
        int returnval = -1;
        utils::MrChronometer chronoexecuter("Total time elapsed");
        try
        {
            switch(m_operationMode)
            {
                //case eOpMode::ExportSWDLBank:
                //{
                //    cout << "=== Exporting SWD Bank ===\n";
                //    returnval = ExportSWDLBank();
                //    break;
                //}
                case eOpMode::ExportSWDL:
                {
                    cout << "=== Exporting SWD ===\n";
                    returnval = ExportSWDL();
                    break;
                }
                case eOpMode::ExportSMDL:
                {
                    cout << "=== Exporting SMD ===\n";
                    returnval = ExportSMDL();
                    break;
                }
                case eOpMode::ExportSEDL:
                {
                    cout << "=== Exporting SED ===\n";
                    returnval = ExportSEDL();
                    break;
                }
                case eOpMode::ExportPMD2:
                {
                    cout <<"=== Exporting PMD2 /SOUND Directory ===\n"
                         <<"Friendly friends~\n";
                    returnval = ExportPMD2Audio();
                    break;
                }
                case eOpMode::BuildSWDL:
                {
                    cout << "=== Building SWD ===\n";
                    returnval = BuildSWDL();
                    break;
                }
                case eOpMode::BuildSMDL:
                {
                    cout << "=== Building SMD ===\n";
                    returnval = BuildSMDL();
                    break;
                }
                case eOpMode::BuildSEDL:
                {
                    cout << "=== Building SED ===\n";
                    returnval = BuildSEDL();
                    break;
                }
                case eOpMode::ExportBatchPairsAndBank:
                {
                    cout <<"=== Exporting Batch SMDL + SWDL + Main Bank ! ===\n";
                    returnval = ExportBatchPairsAndBank();
                    break;
                }
                case eOpMode::ExportBatchPairs:
                {
                    cout <<"=== Exporting Batch SMDL + SWDL ! ===\n";
                    returnval = ExportBatchPairs();
                    break;
                }
                case eOpMode::ExportBatchSWDL:
                {
                    cout <<"=== Exporting Batch SWDL ! ===\n";
                    returnval = ExportBatchSWDL();
                    break;
                }
                case eOpMode::ExportBatchSMDL:
                {
                    cout <<"=== Exporting Batch SMDL ! ===\n";
                    returnval = ExportBatchSMDL();
                    break;
                }
                case eOpMode::BatchListSWDLPrgm:
                {
                    cout <<"=== Batch Writing List of Presets + Samples Contained in SWDL ! ===\n";
                    returnval = BatchListSWDLPrgm( m_swdlpath );
                    break;
                }
                case eOpMode::ListSWDLPrgm:
                {
                    cout <<"=== Writing List of Presets + Samples Contained in SWDL ! ===\n";
                    returnval = BatchListSWDLPrgm( m_inputPath );
                    break;
                }
                case eOpMode::MakeCvInfo:
                {
                    cout <<"=== Writing a Populated CvInfo XML File ! ===\n";
                    returnval = MakeCvinfo();
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
            cerr <<"\n" << "<!>- POCO Exception - " <<e.name() <<"(" <<e.code() <<") : " << e.message() <<"\n" <<endl;
        }
        catch( exception &e )
        {
            cerr <<"\n" << "<!>- Exception - " <<e.what() <<"\n" <<"\n";
        }
        return returnval;
    }


//--------------------------------------------
//  Operations
//--------------------------------------------

    //#TODO: not sure if this is still relevant..
    //int CAudioUtil::ExportSWDLBank()
    //{
    //    cout<< "Not implemented!\n";
    //    assert(false);
    //    return -1;
    //}

    /*
        Extract the content of the PMD2
    */
    int CAudioUtil::ExportPMD2Audio()
    {
        //using namespace pmd2::audio;
        Poco::Path inputdir(m_inputPath);

        if( m_bGM )
            clog<<"<!>- Warning: Commandlin parameter GM specified, but GM conversion of PMD2 is currently unsuported! Falling back to Roland GS conversion!\n";

        //validate the /SOUND/BGM directory
        Poco::File bgmdir( Poco::Path(inputdir).append( "SOUND" ).append("BGM") );

        if( bgmdir.exists() && bgmdir.isDirectory() )
        {
            Poco::Path outputdir;
            if( ! m_outputPath.empty() )
                outputdir = Poco::Path(m_outputPath);
            else
                outputdir = inputdir;

            //Export the /BGM tracks
            Poco::Path mbankpath(Poco::Path(inputdir).append( "SOUND" ).append("BGM").append("bgm.swd").makeFile().toString());
            BatchAudioLoader bal( true, m_bUseLFOFx );
            
            //  1. Grab the main sample bank.
            cout <<"\n<*>- Loading master bank " << mbankpath.toString() <<"..\n";
            bal.LoadMasterBank( mbankpath.toString() );
            cout <<"..done\n";

            Poco::Path bgmdirpath( Poco::Path(inputdir).append( "SOUND" ).append("BGM") );
            const string bgmdir = bgmdirpath.toString();

            //  2. Grab all the swd and smd pairs in the folder
            bal.LoadMatchedSMDLSWDLPairs( bgmdir, bgmdir );

            const string outdir = outputdir.toString();
            CreateOutputDir(outdir);
            DoExportLoader( bal, outdir );

            //if( m_outtype == eOutputType::SF2 )
            //{
            //    //  3. Assign each instruments to a preset. 
            //    //     Put duplicates preset IDs into different bank for the same preset ID.
            //    //  4. Have the tracks exported to midi and refer to the correct preset ID + Bank
            //    cout << "-------------------------------------------------------------\n" 
            //         << "Exporting soundfont and MIDI files to " <<m_outputPath <<"..\n";
            //    bal.ExportSoundfontAndMIDIs( m_outputPath, m_nbloops, m_bBakeSamples );
            //}
            //else if( m_outtype == eOutputType::DLS )
            //{
            //    cout << "-------------------------------------------------------------\n" 
            //         << "Exporting DLS and MIDI files to " <<m_outputPath <<"..\n";
            //    assert(false);
            //}
            //else if( m_outtype == eOutputType::XML )
            //{
            //    cout << "-------------------------------------------------------------\n" 
            //         << "Exporting sample + instruments presets data and MIDI files to " <<m_outputPath <<"..\n";
            //    bal.ExportXMLAndMIDIs( m_outputPath, m_nbloops );
            //}
            //else if( m_outtype == eOutputType::MIDI_Only )
            //{
            //    cout << "-------------------------------------------------------------\n" 
            //         << "Exporting MIDI files only to " <<m_outputPath <<"..\n";
            //    bal.ExportMIDIs( m_outputPath, m_convinfopath, m_nbloops );
            //}
            //else
            //{
            //    cerr << "Internal Error: Output type is invalid!\n"
            //         << "Report this bug please!\n";
            //    assert(false);
            //}

            cout <<"..done\n";

        }
        else
            cout<<"Skipping missing /SOUND/BGM directory\n";

        //validate the /SOUND/ME directory
        Poco::File medir( Poco::Path(inputdir).append( "SOUND" ).append("ME") );

        //Export the /ME tracks
        if( medir.exists() && medir.isDirectory() )
        {
            //!#TODO
        }
        else
            cout<<"Skipping missing /SOUND/ME directory\n";

        //validate the /SOUND/SE directory
        Poco::File sedir( Poco::Path(inputdir).append( "SOUND" ).append("SE") );
        //Export the /SE sounds
        if( sedir.exists() && sedir.isDirectory() )
        {
            //!#TODO
        }
        else
            cout<<"Skipping missing /SOUND/SE directory\n";

        //validate the /SOUND/SWD directory
        Poco::File swddir( Poco::Path(inputdir).append( "SOUND" ).append("SWD") );
        //Export the /SWD samples
        if( swddir.exists() && swddir.isDirectory() )
        {
            //!#TODO
        }
        else
            cout<<"Skipping missing /SOUND/SWD directory\n";

        //validate the /SOUND/SYSTEM directory
        Poco::File sysdir( Poco::Path(inputdir).append( "SOUND" ).append("SYSTEM") );

        //Export the /SYSTEM sounds
        if( sysdir.exists() && sysdir.isDirectory() )
        {
            //!#TODO
        }
        else
            cout<<"Skipping missing /SOUND/SYSTEM directory\n";

        cout <<"All done!\n";

        return 0;
    }

    
    int CAudioUtil::ExportSWDL()
    {
        //using namespace pmd2::audio;
        Poco::Path inputfile(m_inputPath);
        Poco::Path outputfile;
        string     outfname;

        if( ! m_outputPath.empty() )
            outputfile = Poco::Path(m_outputPath);
        else
            outputfile = inputfile.parent().append( inputfile.getBaseName() ).makeDirectory();

        outfname = outputfile.getBaseName();

        // The only thing we can do with a single swd file is to output its content to a directory

        //Create directory
        const string outNewDir = outputfile.toString();
        CreateOutputDir( outNewDir );

        cout << "Exporting SWDL:\n"
             << "\t\"" << inputfile.toString() <<"\"\n"
             << "To:\n"
             << "\t\"" << outNewDir <<"\"\n";   

        //Load SWDL
        PresetBank swd = move( DSE::ParseSWDL( inputfile.toString() ) );
        ExportPresetBank( outNewDir, swd, true, m_useHexaNumbers, !m_bConvertSamples );

        return 0;
    }

    int CAudioUtil::ExportSMDL()
    {
        //using namespace pmd2::audio;
        Poco::Path inputfile(m_inputPath);
        Poco::Path outputfile;
        string     outfname;

        if( ! m_outputPath.empty() )
            outputfile = Poco::Path(m_outputPath);
        else
            outputfile = inputfile.parent().append( inputfile.getBaseName() ).makeFile();

        outfname = outputfile.getBaseName();

        cout << "Exporting SMDL:\n"
             << "\t\"" << inputfile.toString() <<"\"\n"
             << "To:\n"
             << "\t\"" << outfname <<"\"\n";

        //Check if we have a matching swd!
        //Poco::Path matchingswd(inputfile);
        //matchingswd.setExtension( "swd" );
        //Poco::File swdfile(matchingswd);

        //if( swdfile.exists() && swdfile.isFile() )
        //{
        //    SWDL_Header      matchswdhdr = ReadSwdlHeader( swdfile.path() );
        //    BatchAudioLoader myloader( true, m_bUseLFOFx );

        //    if( matchswdhdr.DoesContainsSamples() )
        //    {
        //        cout << "<*>- Found a matching SWDL file containing samples! Exporting a Soundfont along the MIDI!\n";
        //        //If the swdl contains samples, we don't need to load a master bank!
        //        //#Load SWDL
        //        cout << "<*>- Loading pair..\n";
        //        myloader.LoadSmdSwdPair( inputfile.toString(), swdfile.path() );

        //        //#Export
        //        cout << "<*>- Exporting to MIDI + sounfont..\n";
        //        myloader.ExportSoundfontAndMIDIs( outputfile.parent().toString(), m_nbloops, m_bBakeSamples );
        //        cout << "\nSuccess!\n";
        //        return 0;
        //    }
        //    else
        //    {
        //        //Check if we got a master bank
        //        Poco::DirectoryIterator diritbeg(inputfile.parent());
        //        Poco::DirectoryIterator diritend;
        //        bool                    foundsmplbank = false;
        //        string                  mainbankpath;

        //        for(; diritbeg != diritend; ++diritbeg )
        //        {
        //            if( diritbeg->isFile() && Poco::Path(diritbeg->path()).getExtension() == "swd" )
        //            {
        //               if( ReadSwdlHeader( diritbeg->path() ).IsSampleBankOnly() )
        //               {
        //                   foundsmplbank = true;
        //                   mainbankpath  = diritbeg->path();
        //               }
        //            }
        //        }

        //        if( foundsmplbank )
        //        {
        //            cout << "<*>- Found a matching SWDL refering to another SWDL bank file containing samples! Exporting a Soundfont along the MIDI!\n";
        //            
        //            //#Proceed with export
        //            cout << "<*>- Loading main bank file \"" <<mainbankpath <<"\"...";
        //            myloader.LoadMasterBank( mainbankpath );
        //            cout << "done!\n"
        //                 << "<*>- Loading SMD and SWD file..";
        //            myloader.LoadSmdSwdPair( inputfile.toString(), swdfile.path() );
        //            cout << "done!\n"
        //                 << "<*>- Exporting data to Midi and sounfont..\n";
        //            myloader.ExportSoundfontAndMIDIs( outputfile.parent().toString(), m_nbloops, m_bBakeSamples );
        //            cout << "\nSuccess!\n";

        //            return 0;
        //        }
        //        else
        //            cout << "<!>- Found a matching SWDL refering to another SWDL bank file containing samples! However, the sample bank was not found! Falling back to only exporting a MIDI file!\n";
        //        
        //    }
        //}
        //else
        //    cout << "<!>- Couldn't find a matching SWDL ! Falling back to only exporting a MIDI file!\n";

        cout << "<*>- Exporting SMDL to MIDI !\n";

        //By default export a sequence only!
        MusicSequence smd = move( DSE::ParseSMDL( inputfile.toString() ) );
        outputfile.setExtension("mid");

        ExportASequenceToMidi( smd, inputfile.getBaseName(), outputfile.toString(), m_convinfopath, m_nbloops, m_bGM );

        //Write meta
        //#Write the info that didn't fit in the midi here as XML.

        return 0;
    }

    int CAudioUtil::ExportSEDL()
    {
        cout<< "Not implemented!\n";
        assert(false);
        return 0;
    }

    int CAudioUtil::BuildSWDL()
    {
        cout<< "Not implemented!\n";
        assert(false);
        return 0;
    }

    int CAudioUtil::BuildSMDL()
    {
        cout<< "Not implemented!\n";
        assert(false);
        return 0;


        Poco::Path inputfile(m_inputPath);
        Poco::Path outputfile;

        if( ! m_outputPath.empty() )
            outputfile = Poco::Path(m_outputPath);
        else
            outputfile = inputfile.parent().append( inputfile.getBaseName() ).makeFile().setExtension("smd");

        if( m_bGM )
            clog<<"<!>- Warning: Commandline parameter GM specified, when building a SMDL the GM option does nothing!\n";
            

        cout << "Exporting SMDL:\n"
             << "\t\"" << inputfile.toString() <<"\"\n"
             << "To:\n"
             << "\t\"" << outputfile.getBaseName() <<"\"\n";

        DSE::MusicSequence seq = DSE::MidiToSequence(inputfile.toString() );
        seq.metadata().fname = inputfile.getFileName();
        DSE::WriteSMDL( outputfile.toString(), seq );
        return 0;
    }

    int CAudioUtil::BuildSEDL()
    {
        cout<< "Not implemented!\n";
        assert(false);
        return 0;
    }

    int CAudioUtil::ExportBatchPairsAndBank()
    {
        //using namespace pmd2::audio;
        BatchAudioLoader bal(true,m_bUseLFOFx);

        Poco::Path outputDir;

        if( ! m_outputPath.empty() )
            outputDir = Poco::Path(m_outputPath);
        else
            outputDir = Poco::Path(m_smdlpath);

        if( m_bGM )
            clog<<"<!>- Warning: Commandline parameter GM specified, but GM conversion of is currently unsuported in this mode! Falling back to Roland GS conversion!\n";
            
        //  1. Grab the main sample bank.
        cout <<"\n<*>- Loading master bank " << m_mbankpath <<"..\n";
        bal.LoadMasterBank( m_mbankpath );
        cout <<"..done\n";

        bal.LoadMatchedSMDLSWDLPairs( m_swdlpath, m_smdlpath );

        const string stroutpath = outputDir.toString();
        CreateOutputDir(stroutpath);
        DoExportLoader( bal, stroutpath );
        
        cout <<"..done\n";

        return 0;
    }

    int CAudioUtil::ExportBatchPairs()
    {
        //using namespace pmd2::audio;
        BatchAudioLoader bal(false, m_bUseLFOFx);

        Poco::Path   outputDir;

        if( ! m_outputPath.empty() )
            outputDir = Poco::Path(m_outputPath);
        else
        {
            if( !m_bgmcntpath.empty() && !m_bgmcntext.empty() )
                outputDir = Poco::Path(m_bgmcntpath);
            else
                outputDir = Poco::Path(m_smdlpath);
        }

        if( m_bGM )
            clog<<"<!>- Warning: Commandline parameter GM specified, but GM conversion of is currently unsuported in this mode! Falling back to Roland GS conversion!\n";

        if( !m_bgmcntpath.empty() && !m_bgmcntext.empty() )
            bal.LoadBgmContainers( m_bgmcntpath, m_bgmcntext );
        else if (!m_bgmblobpath.empty())
        {
            for(const auto & blobpath : m_bgmblobpath)
                bal.LoadSMDLSWDLSPairsFromBlob(blobpath, m_bmatchbyname);
        }
        else
            bal.LoadMatchedSMDLSWDLPairs( m_swdlpath, m_smdlpath );

        const string stroutpath = outputDir.toString();
        CreateOutputDir(stroutpath);
        DoExportLoader( bal, stroutpath );
        cout <<"..done\n";

        return 0;
    }


    int CAudioUtil::ExportBatchSWDL()
    {
        //using namespace pmd2::audio;
        Poco::Path   inputfile(m_swdlpath);
        Poco::Path   outputDir;

        if( ! m_outputPath.empty() )
            outputDir = Poco::Path(m_outputPath);
        else
            outputDir = inputfile;

        cout << "Exporting SWDL:\n"
             << "\t\"" << inputfile.toString() <<"\"\n"
             << "To:\n"
             << "\t\"" << outputDir.toString() <<"\"\n";

        //Create parent output dir
        CreateOutputDir( outputDir.toString() ); 

        auto lambdaExport = [&]( const Poco::Path & curpath )
        {
            string       infilename = curpath.getBaseName();
            const string outNewDir  = Poco::Path(outputDir).append(infilename).toString();
            //Load SWDL
            PresetBank swd = move( DSE::ParseSWDL( curpath.toString() ) );
            {
                auto ptrsmpls = swd.smplbank().lock();

                if( ptrsmpls != nullptr )
                    CreateOutputDir( outNewDir );  //Create sub directory
            }
            ExportPresetBank( outNewDir, swd, true, m_useHexaNumbers, !m_bConvertSamples );
        };

        ProcessAllFilesWithExtInDir( m_swdlpath, SWDL_FileExtension, "Exporting", lambdaExport );


        //Poco::DirectoryIterator itdir(m_swdlpath);
        //Poco::DirectoryIterator itend;

        //for( ; itdir != itend; ++itdir )
        //{
        //    if( itdir->isFile() )
        //    {
        //        Poco::Path curpath(itdir->path());
        //        if( !SameFileExt( curpath.getExtension(), SWDL_FileExtension ) )
        //            continue;

        //        string       infilename = curpath.getBaseName();
        //        const string outNewDir  = Poco::Path(outputDir).append(infilename).toString();

        //        cout <<right <<setw(60) <<setfill(' ') << "\rExporting " << infilename << "..";

        //        //Load SWDL
        //        PresetBank swd = move( DSE::ParseSWDL( curpath.toString() ) );
        //        {
        //            auto ptrsmpls = swd.smplbank().lock();

        //            if( ptrsmpls != nullptr )
        //                CreateOutputDir( outNewDir );  //Create sub directory
        //        }
        //        ExportPresetBank( outNewDir, swd, true, m_useHexaNumbers );
        //    }
        //}
        cout<<"\n\n<*>- Done !\n";
        return 0;
    }
    
    //#TODO: Replace this so it uses the batch converter class!!
    int CAudioUtil::ExportBatchSMDL()
    {
        //using namespace pmd2::audio;
        Poco::Path   inputfile(m_smdlpath);
        Poco::Path   outputDir;

        if( ! m_outputPath.empty() )
            outputDir = Poco::Path(m_outputPath);
        else
            outputDir = inputfile;

        cout << "Exporting SMDL:\n"
             << "\t\"" << inputfile.toString() <<"\"\n"
             << "To:\n"
             << "\t\"" << outputDir.toString() <<"\"\n";

        //Create parent output dir
        CreateOutputDir( outputDir.toString() ); 

        auto lambdaExport = [&]( const Poco::Path & curpath )
        {
            string       infilename = curpath.getBaseName();
            const string outNewDir  = Poco::Path(outputDir).append(infilename).toString();

            MusicSequence smd = move( DSE::ParseSMDL( curpath.toString() ) );
            ExportASequenceToMidi( smd, infilename, Poco::Path(outputDir).append(infilename).setExtension("mid").makeFile(), m_convinfopath, m_nbloops, m_bGM );
        };

        ProcessAllFilesWithExtInDir( m_smdlpath, SMDL_FileExtension, "Exporting", lambdaExport );

        //Poco::DirectoryIterator itdir(m_smdlpath);
        //Poco::DirectoryIterator itend;

        //for( ; itdir != itend; ++itdir )
        //{
        //    if( itdir->isFile() )
        //    {
        //        Poco::Path curpath(itdir->path());
        //        if( !SameFileExt( curpath.getExtension(), SMDL_FileExtension ) )
        //            continue;

        //        string        infilename = curpath.getBaseName();
        //        cout <<right <<setw(60) <<setfill(' ')  << "\rExporting " << infilename << "..";

        //        MusicSequence smd = move( DSE::ParseSMDL( curpath.toString() ) );
        //        ExportASequenceToMidi( smd, infilename, Poco::Path(outputDir).append(infilename).setExtension("mid").makeFile(), m_convinfopath, m_nbloops, m_bGM );
        //    }
        //}
        cout<<"\n\n<*>- Done !\n";
        return 0;
    }

    void WriteSWDLPresetAndSampleList( const std::string & infilename, const DSE::PresetBank & swd, std::ofstream & lstfile, bool bhexnumbers )
    {
        auto prgmptr = swd.prgmbank().lock();

        if( prgmptr != nullptr )
        {
            size_t cntprg = 0;
            lstfile << "\n======================================================================\n"
                    << "*" <<infilename <<"\n"
                    << "======================================================================\n"
                    << "Uses the following presets : \n";

            if( bhexnumbers )
                lstfile << hex <<showbase;
            else
                lstfile <<dec <<noshowbase;

            for( const auto & prgm : prgmptr->PrgmInfo() )
            {
                if( prgm != nullptr )
                {
                    lstfile <<"\n\t*Preset #"<<static_cast<int>(prgm->id) <<"\n"
                            <<"\t------------------------------------\n";

                    for( const auto & split : prgm->m_splitstbl )
                    {
                        lstfile << "\t\t->Split " <<right <<setw(4) <<setfill(' ') <<static_cast<int>(split.id) <<": " 
                                << "Sample ID( " <<right <<setw(6) <<setfill(' ') <<static_cast<int>(split.smplid) 
                                << " ), MIDI KeyRange( " <<right <<setw(4) <<setfill(' ') <<static_cast<int>(split.lowkey) <<" to " 
                                <<right <<setw(4) <<setfill(' ') <<static_cast<int>(split.hikey) <<" ),"
                                <<" \n";
                    }
                    ++cntprg;
                }
            }

            if( bhexnumbers )
                lstfile << dec <<noshowbase;
            lstfile << "\nTotal " <<cntprg << " Presets!\n\n";
        }
    }


    int CAudioUtil::BatchListSWDLPrgm( const string & SrcPath )
    {
        //using namespace pmd2::audio;
        Poco::Path   inputfile(SrcPath);
        Poco::Path   outputfile;

        if( ! m_outputPath.empty() )
            outputfile = Poco::Path(m_outputPath);
        else
            outputfile = inputfile.parent().append( inputfile.getBaseName() ).makeFile();

        cout << "Listing SWDL Files Presets From Directory:\n"
             << "\t\"" << inputfile.toString() <<"\"\n"
             << "To:\n"
             << "\t\"" << outputfile.toString() <<"\"\n";


        std::ofstream lstfile( outputfile.toString() );
        if( !lstfile.is_open() || lstfile.bad() )
            throw runtime_error( "CAudioUtil::BatchListSWDLPrgm(): Couldn't open output file " + outputfile.toString() + " for writing!" );

        //Work differently depending on what the input is
        Poco::File infile (inputfile);
        if( infile.exists() && infile.isFile() )
        {
            if( !SameFileExt( inputfile.getExtension(), SWDL_FileExtension ) )
            {
                cerr<< "<!>- ERROR: Input file is not a SWDL file!\n";
                return -1;
            }

            string infilename = inputfile.getBaseName();
            cout <<right <<setw(60) <<setfill(' ') << "\rAnalyzing " << infilename << "..";

            lstfile << "\n======================================================================\n"
                    << "*" <<infilename <<"\n"
                    << "======================================================================\n\n";

            PresetBank swd = move( DSE::ParseSWDL( inputfile.toString() ) );
            WriteSWDLPresetAndSampleList( infilename, swd, lstfile, m_useHexaNumbers );
        }
        else
        {

            auto lambdaExport = [&]( const Poco::Path & curpath )
            {
                string     infilename = curpath.getBaseName();
                PresetBank swd        = move( DSE::ParseSWDL( curpath.toString() ) );
                WriteSWDLPresetAndSampleList( infilename, swd, lstfile, m_useHexaNumbers );
            };

            ProcessAllFilesWithExtInDir( SrcPath, SWDL_FileExtension, "Analyzing", lambdaExport );

            //Poco::DirectoryIterator itdir(SrcPath);
            //Poco::DirectoryIterator itend;

            //for( ; itdir != itend; ++itdir )
            //{
            //    if( itdir->isFile() )
            //    {
            //        Poco::Path curpath(itdir->path());
            //        if( !SameFileExt( curpath.getExtension(), SWDL_FileExtension ) )
            //            continue;

            //        string infilename = curpath.getBaseName();
            //        cout <<right <<setw(60) <<setfill(' ') << "\rAnalyzing " << infilename << "..";

            //        PresetBank swd = move( DSE::ParseSWDL( curpath.toString() ) );
            //        WriteSWDLPresetAndSampleList( infilename, swd, lstfile, m_useHexaNumbers );
            //    }
            //}
        }
        cout<<"\n\n<*>- Done !\n";
        return 0;
    }

    void MakeInitialCvInfoForPrgmBank( DSE::SMDLPresetConversionInfo & dest, const DSE::ProgramBank & curbank )
    {
        for( const auto & aprgm : curbank.PrgmInfo() )
        {
            if( aprgm != nullptr )
            {
                uint16_t prgid = aprgm->id;
                DSE::SMDLPresetConversionInfo::PresetConvData mycvdat( static_cast<presetid_t>(prgid) );
                mycvdat.midipres = static_cast<presetid_t>(prgid);
                
                if( prgid == 0x7F )
                {
                    mycvdat.midibank  = 127;
                    mycvdat.idealchan = 10;
                }
                
                for( const auto & split : aprgm->m_splitstbl )
                {
                    mycvdat.origsmplids.push_back( split.smplid );

                    if( (split.hikey - split.lowkey) == 0 ) //Only care about single key splits
                    {
                        DSE::SMDLPresetConversionInfo::NoteRemapData nrmap;
                        nrmap.destnote   = split.hikey;
                        nrmap.origsmplid = split.smplid;
                        mycvdat.remapnotes.insert( make_pair( split.hikey, move(nrmap) ) );
                    }
                }

                dest.AddPresetConvInfo( prgid, move(mycvdat) );
            }
        }
    }

    int CAudioUtil::MakeCvinfo()
    {
        Poco::Path   inputfile(m_swdlpath);
        Poco::Path   outputfile;

        if( ! m_outputPath.empty() )
            outputfile = Poco::Path(m_outputPath);
        else
            outputfile = Poco::Path(inputfile).setFileName("cvinfo.xml");

        cout << "Exporting cvinfo file To:\n"
             << "\t\"" << outputfile.toString() <<"\"\n";

        DSE::SMDLConvInfoDB cvdb;

        auto lambdaParseSwdl = [&]( const Poco::Path & curpath )
        {
            try
            {
                string     infilename = curpath.getBaseName();
                PresetBank swd        = move( DSE::ParseSWDL( curpath.toString() ) );
                {
                    auto ptrprgms = swd.prgmbank().lock();

                    if( ptrprgms != nullptr )
                    {
                        DSE::SMDLPresetConversionInfo prgscvinf;
                        MakeInitialCvInfoForPrgmBank( prgscvinf, *ptrprgms );
                        //Make cvinfo data entry
                        cvdb.AddConversionInfo( infilename, move(prgscvinf) );
                    }
                }
            }
            catch( const exception & e )
            {
                cerr << "\n<!>- Error: " <<e.what() <<" Skipping file and attempting to continue!\n";
            }
        };

        auto lambdaParseBgmCnt = [&]( const Poco::Path & curpath )
        {
            try
            {
                string infilename = curpath.getBaseName();
                auto   bgmcnt     = move( DSE::ReadBgmContainer( curpath.toString() ) );
                {
                    auto ptrprgms = bgmcnt.first.prgmbank().lock();

                    if( ptrprgms != nullptr )
                    {
                        DSE::SMDLPresetConversionInfo prgscvinf;
                        MakeInitialCvInfoForPrgmBank( prgscvinf, *ptrprgms );
                        //Make cvinfo data entry
                        cvdb.AddConversionInfo( infilename, move(prgscvinf) );
                    }
                }
            }
            catch( const exception & e )
            {
                cerr << "\n<!>- Error: " <<e.what() <<" Skipping file and attempting to continue!\n";
            }
        };

        if( !m_swdlpath.empty() )
            ProcessAllFilesWithExtInDir( m_swdlpath, SWDL_FileExtension, "Analyzing", lambdaParseSwdl );
        else if( !m_bgmcntpath.empty() && !m_bgmcntext.empty() )
            ProcessAllFilesWithExtInDir( m_bgmcntpath, m_bgmcntext, "Analyzing", lambdaParseBgmCnt );
        else
            throw runtime_error( "CAudioUtil::MakeCvinfo(): No SWDL path or bgm container path specified!" );

        //for( ; itdir != itend; ++itdir )
        //{

        //    Poco::Path curpath(itdir->path());
        //    if( !SameFileExt( curpath.getExtension(), SWDL_FileExtension ) )
        //        continue;

        //    string       infilename = curpath.getBaseName();

        //    cout <<right <<setw(60) <<setfill(' ') << "\rParsing " << infilename << "..";

        //    //Load SWDL
        //    PresetBank swd = move( DSE::ParseSWDL( curpath.toString() ) );
        //    {
        //        auto ptrprgms = swd.prgmbank().lock();

        //        if( ptrprgms != nullptr )
        //        {
        //            DSE::SMDLPresetConversionInfo prgscvinf;
        //            MakeInitialCvInfoForPrgmBank( prgscvinf, *ptrprgms );
        //            //Make cvinfo data entry
        //            cvdb.AddConversionInfo( infilename, move(prgscvinf) );
        //        }
        //    }
        //}

        //Write cvinfo db
        cvdb.Write( outputfile.toString() );

        cout<<"\n\n<*>- Done !\n";
        return 0;
    }


    void CAudioUtil::DoExportLoader( DSE::BatchAudioLoader & bal, const std::string & outputpath )
    {
        cout << "-------------------------------------------------------------\n";
        if( m_outtype == eOutputType::SF2 )
        {
            cout << "Exporting soundfont and MIDI files to " <<outputpath <<"..\n";
            bal.ExportSoundfontAndMIDIs( outputpath, m_nbloops, m_bBakeSamples );
        }
        else if( m_outtype == eOutputType::DLS )
        {
            cout << "Exporting DLS and MIDI files to " <<outputpath <<"..\n";
            cerr << "DLS support not implemented!\n";
            assert(false);
        }
        else if( m_outtype == eOutputType::XML )
        {
            cout << "Exporting sample + instruments presets data and MIDI files to " <<outputpath <<"..\n";
            bal.ExportXMLAndMIDIs( outputpath, m_nbloops );
        }
        else if( m_outtype == eOutputType::MIDI_Only )
        {
            cout << "Exporting MIDI files only to " <<outputpath <<"..\n";
            bal.ExportMIDIs( outputpath, m_convinfopath, m_nbloops );
        }
        else
        {
            cerr << "Internal Error: Output type is invalid!\n"
                 << "Report this bug please!\n";
            assert(false);
        }
    }


//--------------------------------------------
//  Main Methods
//--------------------------------------------
    int CAudioUtil::Main(int argc, const char * argv[])
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
    using namespace audioutil;
    try
    {
        return CAudioUtil::GetInstance().Main(argc,argv);
    }
    catch( exception & e )
    {
        cout<< "<!>-ERROR:" <<e.what()<<"\n"
            << "If you get this particular error output, it means an exception got through, and the programmer should be notified!\n";
    }

    return 0;
}