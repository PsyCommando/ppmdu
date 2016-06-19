#ifndef CONFIG_IO_HPP
#define CONFIG_IO_HPP
/*
config_io.hpp
2015/03/21
psycommando@gmail.com
Description: 
    Utilities for parsing simple config strings file.

    Its a very simple file format.
    // == comment
    "" == a string to parse

    Each line represent one entry, and each line can have more than one string, separated by white spaces.

    Example: 

        //This is a comment
        "Variable1" "Charmander!" "50" //This is a comment
        "Var2"      "false"
        //"Var3"      "yes" this line is commented


        #TODO: not gonna use this..

*/
//!Will use json or xml instead!!!

#include <vector>
#include <string>
#include <sstream>

namespace utils{ namespace io
{
    /*
        ConfigHandler
            This class loads and write config files. And it allows modifying them and writing them again.
    */
    class ConfigHandler
    {
    public:
        typedef std::vector<std::vector<std::string>> entry_t;

        void Load( const std::string & path );
        void Write( const std::string & path )const;

        inline entry_t       & Entries()      { return m_entries; }
        inline const entry_t & Entries()const { return m_entries; }

    private:
        std::vector<std::string> ParseLine(const std::string & line);

        std::vector<std::vector<std::string>> m_entries;
    };

};};

#endif