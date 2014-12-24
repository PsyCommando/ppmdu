#ifndef UTILITY_HPP
#define UTILITY_HPP
/*
utility.hpp
2014/12/21
psycommando@gmail.com
Description: A header with a bunch of useful includes for the PPMD utilities. 
*/
#include <chrono>
#include <string>
#include "gfileutils.hpp"
#include "gfileio.hpp"
#include "gstringutils.hpp"
#include "gbyteutils.hpp"
#include "poco_wrapper.hpp"
#include <iosfwd>

namespace utils
{
//===============================================================================================
// Struct
//===============================================================================================

    /************************************************************************
        data_array_struct
            A base structure for structures to be read from files, such as 
            headers, and any constant size blocks of data !
    ************************************************************************/
    struct data_array_struct
    {
        virtual ~data_array_struct(){}
        virtual unsigned int size()const=0;

        //The method expects that the interators can be incremented at least as many times as "size()" returns !
        virtual std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const = 0;

        //The method expects that the interators can be incremented at least as many times as "size()" returns !
        virtual std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom )     = 0;
    };


    /************************************************************************
        Resolution
            A struct with human readable naming to make handling resolution
            values easier.
    ************************************************************************/
    struct Resolution
    {
        uint16_t width, height;

        inline bool operator==( const Resolution & right )const
        {
            return (this->height == right.height) && (this->width == right.width);
        }
    };

    /************************************************************************
        MrChronometer
            A small utility RAII class that that tells the time elapsed 
            between its construction and destruction.
    ************************************************************************/
    struct MrChronometer
    {

        MrChronometer( const std::string name = "*", std::ostream * messageoutput = nullptr );
        ~MrChronometer();

        std::chrono::steady_clock::time_point  _start;
        std::string                            _name;
        std::ostream                         * _output;
    };

    /************************************************************************
        optionparsing_t
            Struct for holding details on how to parse arguments from the commandline.
    ************************************************************************/
    //struct optionparsing_t
    //{
    //    std::string  optionsymbol;      //Symbol to look for, only if its an option/switch, excluding the '-' !
    //    unsigned int nbvalues;          //Nb of values to consider part of this arg option from the following whitespace bounded arguments.
    //    std::string  description;       //Short description
    //};

//===============================================================================================
// Classes
//===============================================================================================

    /************************************************************************
        CArgsParser
            A small class to parse parameters passed at the command line.
    ************************************************************************/
    //class CArgsParser
    //{
    //public:
    //    CArgsParser( const char * argv[], int argc  );
    //    CArgsParser( std::vector<optionparsing_t> optionlist, const char * argv[], int argc );
    //    
    //    //Return the reading pos to the beginning of the Parameter list (Affects only parameter reading, not option reading)
    //    void ResetReadPos();

    //    // ----- Arguments -----
    //    //Return the current whitespace bounded string of character that isn't a switch/option
    //    // If no more parameters, returns an empty string!
    //    std::string getCurrentParam();

    //    //Return the next whitespace bounded string of character that isn't a switch/option
    //    // If no more parameters, returns an empty string!
    //    std::string getNextParam();

    //    //Skip the current parameter whithout attempting to parse it.
    //    // Useful for skipping the program's name in the list of arguments.
    //    void skipCurrentParam();

    //    //Returns whether the whole argument array has been scanned for arguments
    //    bool hasReachedTheEndOfArgList()const;

    //    // ----- Options -----
    //    //Takes the index of the option from the option list passed at construction, if its the case!
    //    // -> returns a vector of strings.
    //    //      -> is empty if option not found
    //    //      -> if found 
    //    //          -> first entry is symbol of option/switch
    //    //          -> additional entries are the parameters for this particular options. As specified in the optionlist at construction.
    //    std::vector< std::string > getOption( std::vector<optionparsing_t>::size_type optionindex );

    //    //Returns a list of all the found options considering the params passed at construction.
    //    // -> The first entry in each std::vector<std::string> is the symbol of the option that was found
    //    std::vector< std::vector<std::string> > getAllFoundOptions();

    //    //Appends all the parameters preceded with a "+". It removes the "+" from the appended elements.
    //    //It appends the results to the vector specified, using "push_back".
    //    // Returns false if there was nothing to add.
    //    bool appendAllAdditionalInputParams( std::vector<std::string> & out_vtoappendto );

    //private:
    //    unsigned int getNbArgsForOption( const std::string & option );

    //    std::vector<std::string>                 m_rawargs;
    //    std::vector<std::string>::const_iterator m_rawcurarg;
    //    std::vector<optionparsing_t>             m_optionlist;

    //    static const std::string SWITCH_SYMBOL;
    //    static const std::string ADDITIONAL_INPUT_PARAM_SYMBOL;
    //};

//===============================================================================================
// Function
//===============================================================================================

    /************************************************************************
        SimpleHandleException
            A function to avoid having to rewrite a thousand time the same 2 
            lines of code in case of exception.

            Simply write the exception's "what()" function output to "cerr", 
            and triggers an assert in debug.
    ************************************************************************/
    void SimpleHandleException( std::exception & e );

};

#endif 