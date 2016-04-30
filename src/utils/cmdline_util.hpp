#ifndef CMDLINE_UTIL_HPP
#define CMDLINE_UTIL_HPP
/*
cmdline_util.hpp
2014/12/06
psycommando@gmail.com
Description: A baseclass for automating common tasks performed by simple command line utilities.
             Meant to make creating command lines utilities easier to maintain and maintain consistence
             between them all.
*/
//#include <utils/utility.hpp>
#include <vector>
#include <deque>
#include <string>
#include <functional>
#include <memory>
#include <exception>
#include <iostream>
#include <fstream>

namespace utils{ namespace cmdl 
{
//===============================================================================================
// Typedefs
//===============================================================================================

    //Types for the parsing functions for each parameters (Wrap it in a bind or something to handle the hidden "this" ptr )
    typedef bool(*argparsefun_t)(const std::string&); 
    typedef bool(*optparsefun_t)(const std::vector<std::string>&);

    //Thrown when a required parameter was not found at the command line!
    class exMissingParameter : public std::runtime_error
    { public: exMissingParameter( std::string message ):std::runtime_error(message){} };

//===============================================================================================
// Structs
//===============================================================================================

    /************************************************************************
        optionparsing_t
            Struct for holding details on how to parse arguments from the commandline.
    ************************************************************************/
    struct optionparsing_t
    {
        std::string  optionsymbol;      //Symbol to look for, only if its an option/switch, excluding the '-' !
        unsigned int nbvalues;          //Nb of values to consider part of this arg option from the following whitespace bounded arguments.
        std::string  description;       //Short description
        std::string  example;           //An example of this option on the command line

        //Function pointers
        std::function<bool(const std::vector<std::string>&)> myOptionParseFun;
        //optparsefun_t myOptionParseFun; //Pointer to a function that will parse this option for the program. 
                                        //Must return false if it fails to parse !
    };

    /************************************************************************
        argumentparsing_t
            Struct for holding details on how to parse arguments from the commandline.
    ************************************************************************/
    struct argumentparsing_t
    {
        int          argumentorder;     //The order this argument should appear on the parameter list. Counting only other parameters, not options. Starts at 0!
        bool         isoptional;        //Whether the absense of the parameter can be tolerated or not. (The myIsRequired function pointer will be used if not null)
        bool         isguaranteedorder; //Whether this parameter is guaranteed appear in this argument order.
        std::string  name;              //A unique name to identify this argument
        std::string  description;       //Short description
        std::string  example;           //An example of this argument on the command line

        //Functions pointers
        std::function<bool(const std::string&)> myParseFun;  //Pointer to a function that will parse this argument for the program. 
                                                             //Must return false if it fails to parse !

        //std::string *dependson;         //What other argument needs to be parsed before! Use their "name" field's value. Must refer to a static table, because the pointer is never deleted!
        //int          totaldependson;    //The size of the "dependson" string array.

        //Functions pointers
        std::function<bool(const std::vector<std::vector<std::string>>&, 
                           const std::deque<std::string>&,
                           size_t)> myShouldParse;  // The first is a list of all options parsed, the second is a list of all arguments parsed before this one! The third is the amount of parameters left to parse, including the once currently being parsed.
    };


//===============================================================================================
// Classes
//===============================================================================================

    /************************************************************************
        CArgsParser
            A small class to parse parameters passed at the command line.
    ************************************************************************/
    class CArgsParser
    {
    public:
        CArgsParser( const char * argv[], int argc  );
        CArgsParser( std::vector<optionparsing_t> optionlist, const char * argv[], int argc );
        
        //Return the reading pos to the beginning of the Parameter list (Affects only parameter reading, not option reading)
        void ResetReadPos();

        // ----- Arguments -----
        //Return the current whitespace bounded string of character that isn't a switch/option
        // If no more parameters, returns an empty string!
        std::string getCurrentParam();

        //Return the next whitespace bounded string of character that isn't a switch/option
        // If no more parameters, returns an empty string!
        std::string getNextParam();

        //Skip the current parameter whithout attempting to parse it.
        // Useful for skipping the program's name in the list of arguments.
        void skipCurrentParam();

        //Returns whether the whole argument array has been scanned for arguments
        bool hasReachedTheEndOfArgList()const;

        // ----- Options -----
        //Takes the index of the option from the option list passed at construction, if its the case!
        // -> returns a vector of strings.
        //      -> is empty if option not found
        //      -> if found 
        //          -> first entry is symbol of option/switch
        //          -> additional entries are the parameters for this particular options. As specified in the optionlist at construction.
        std::vector< std::string > getOption( std::vector<optionparsing_t>::size_type optionindex );

        //Returns a list of all the found options considering the params passed at construction.
        // -> The first entry in each std::vector<std::string> is the symbol of the option that was found
        std::vector< std::vector<std::string> > getAllFoundOptions();

        //Returns a list of all the found parameters passed at construction.
        std::deque< std::string > getAllFoundParams();

        //Returns a list of all the found extra parameters passed at construction.
        std::vector< std::string > getAllAdditionalParams();

        //Appends all the parameters preceded with a "+". It removes the "+" from the appended elements.
        //It appends the results to the vector specified, using "push_back".
        // Returns false if there was nothing to add.
        bool appendAllAdditionalInputParams( std::vector<std::string> & out_vtoappendto );

    private:
        unsigned int getNbArgsForOption( const std::string & option );

        std::vector<std::string>                 m_rawargs;
        std::vector<std::string>::const_iterator m_rawcurarg;
        std::vector<optionparsing_t>             m_optionlist;

        static const std::string SWITCH_SYMBOL;
        static const std::string ADDITIONAL_INPUT_PARAM_SYMBOL;
    };


    /************************************************************************
        RAIIClogRedirect
             Handle redirecting the clog stream for the lifetime of the 
             object! Then safely restore clog to its original state.
    *************************************************************************/
    class RAIIClogRedirect
    {
    public:
        RAIIClogRedirect();
        virtual ~RAIIClogRedirect();

        bool IsRedirecting()const;
        void Redirect( const std::string & filename );
        void StopRedirect();

    protected:
        std::streambuf * m_oldbuf;
        std::filebuf     m_filebuf;
        std::filebuf     m_nullbuff;
        bool             m_bIsRedirecting;
    };


    /************************************************************************
        CommandLineUtility
            A class to inherit from to implement a commandline program with 
            automated args parsing, and other various tedious tasks.
    ************************************************************************/
    class CommandLineUtility
    {
    public:

        CommandLineUtility()
        {}

        //Destructor
        virtual ~CommandLineUtility()
        {}

        //SetArguments returns false, when there are no args to parse !
        bool SetArguments( int argc, const char * argv[], bool noargsprintreadme = true )
        {
            if( argc == 1 ) //No args, print the readme then!
            {
                if(noargsprintreadme) 
                    PrintReadme();
                return false;
            }
            else
            {
                CArgsParser parsemyargs( getOptionsList(), argv, argc ); //#TODO: eventually combine CArgParser with this class!
                parseArgs(parsemyargs);
                parseOptions(parsemyargs);
                parseExtraArgs(parsemyargs);
            }
            return true;
        }
        void PrintReadme();
        void PrintTitle();

        // -- Overrides --

        //Those return their implementation specific arguments, options, and extra parameter lists.
        virtual const std::vector<argumentparsing_t> & getArgumentsList()const    = 0;
        virtual const std::vector<optionparsing_t>   & getOptionsList()const      = 0;
        virtual const argumentparsing_t              * getExtraArg()const         = 0; //Returns nullptr if there is no extra arg. Extra args are args preceeded by a "+" character, usually used for handling files in batch !
       
        //For writing the title and readme!
        virtual const std::string                    & getTitle()const            = 0; //Name/Title of the program to put in the title!
        virtual const std::string                    & getExeName()const          = 0; //Name of the executable file!
        virtual const std::string                    & getVersionString()const    = 0; //Version number
        virtual const std::string                    & getShortDescription()const = 0; //Short description of what the program does for the header+title
        virtual const std::string                    & getLongDescription()const  = 0; //Long description of how the program works
        virtual const std::string                    & getMiscSectionText()const  = 0; //Text for copyrights, credits, thanks, misc..

        //Main method
        virtual int  Main( int argc, const char * argv[] )  = 0;

    protected:

        void parseArgs( CArgsParser & argsparse );
        void parseOptions( CArgsParser & argsparse );

        //If AbortOnError is true, the method will throw an exception if the parameter can't be parsed. 
        // Otherwise, it will only print a warning to cerr !
        bool parseExtraArgs( CArgsParser & argsparse, bool bAbortOnError = false ); 

        //Disable copy and move
        CommandLineUtility( const CommandLineUtility & );
        CommandLineUtility( CommandLineUtility && );
        CommandLineUtility& operator=(const CommandLineUtility&);
        CommandLineUtility& operator=(const CommandLineUtility&&);

    private:
        //No copy!
        //CommandLineUtility( const CommandLineUtility* );
        //CommandLineUtility& operator=( const CommandLineUtility* );
    };

};};

#endif 