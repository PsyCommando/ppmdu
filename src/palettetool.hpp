#ifndef PALETTE_TOOL_HPP
#define PALETTE_TOOL_HPP
/*
palettetool.hpp
2015/02/22
psycommando@gmail.com
Description: A little utility to export a color palette from a bmp or png image to a 
text file, or a riff palette.
*/
#include <ppmdu/basetypes.hpp>
#include <ppmdu/ext_fmts/supported_io.hpp>
#include <ppmdu/utils/cmdline_util.hpp>
#include <string>
#include <vector>

namespace palettetool
{
    class CPaletteUtil : public utils::cmdl::CommandLineUtility
    {
    public:
        static CPaletteUtil & GetInstance();

        // -- Overrides --
        //Those return their implementation specific arguments, options, and extra parameter lists.
        const std::vector<utils::cmdl::argumentparsing_t> & getArgumentsList()const;
        const std::vector<utils::cmdl::optionparsing_t>   & getOptionsList()const;
        const utils::cmdl::argumentparsing_t              * getExtraArg()const; //Returns nullptr if there is no extra arg. Extra args are args preceeded by a "+" character, usually used for handling files in batch !
       
        //For writing the title and readme!
        const std::string & getTitle()const;            //Name/Title of the program to put in the title!
        const std::string & getExeName()const;          //Name of the executable file!
        const std::string & getVersionString()const;    //Version number
        const std::string & getShortDescription()const; //Short description of what the program does for the header+title
        const std::string & getLongDescription()const;  //Long description of how the program works
        const std::string & getMiscSectionText()const;  //Text for copyrights, credits, thanks, misc..

        //Main method
        int Main(int argc, const char * argv[]);

    private:
        CPaletteUtil();

        //Parse Arguments
        bool ParseInputPath  ( const std::string              & path );
        bool ParseOutputPath ( const std::string              & path );

        //Parse Options
        bool ParseOptionToRIFF( const std::vector<std::string> & optdata );
        bool ParseOption8bpp  ( const std::vector<std::string> & optdata );

        //Execution
        void DetermineOperation();
        int Execute();
        int GatherArgs( int argc, const char * argv[] );

        //Exec methods
        int DumpPalette();
        int ConvertPalette();
        int InjectPalette();

        //Constants
        static const std::string                                 Exe_Name;
        static const std::string                                 Title;
        static const std::string                                 Version;
        static const std::string                                 Short_Description;
        static const std::string                                 Long_Description;
        static const std::string                                 Misc_Text;
        static const std::vector<utils::cmdl::argumentparsing_t> Arguments_List;
        static const std::vector<utils::cmdl::optionparsing_t>   Options_List;

        enum struct ePalType
        {
            TEXT,
            RIFF,
        };
        enum struct eOpMode
        {
            Invalid,
            Dump,
            Convert,
            Inject,
        };

        //Variables
        std::string                    m_inputPath;      //This is the input path that was parsed 
        std::string                    m_outputPath;     //This is the output path that was parsed
        ePalType                       m_outPalType;     //This is the type of palette to output
        eOpMode                        m_operationMode;  //This holds what the program should do
        bool                           m_imgFormatIs8bpp;//This changes whether the image will be read as 4bpp or 8bpp  
    };

};

#endif