#include "config_io.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
using namespace std;

namespace utils {namespace io
{

    vector<string> ConfigHandler::ParseLine(const string & line)
    {
        vector<string> entries;

        //First trim out the comments
        string strtoproc;
        size_t commentpos = line.find( "//", 0 );

        if( commentpos != string::npos )
            strtoproc = line.substr( 0, commentpos ); //Remove the comments
        else
            strtoproc = line;

        //If we still have text that's not comments
        if( !strtoproc.empty() )
        {
            //Parse individual strings
            stringstream sstr(strtoproc);
                
            while( sstr.good() && !sstr.eof() )
            {
                string astr;
                sstr >> astr;

                //If we got a quote wrapped string
                if( !astr.empty() && 
                    astr.front() == '\"' &&
                    astr.back()  == '\"' )
                {
                    entries.push_back( astr.substr( 1, (astr.size()-2) ) ); //Remove the quotes, and add it to the entries
                }
            }
        }

        return std::move(entries);
    }

    void ConfigHandler::Load( const std::string & path )
    {
        ifstream configfile(path);
        if( configfile.good() && configfile.is_open() )
        {
            while( configfile.good() && !configfile.eof() )
            {
                string line;
                std::getline( configfile, line );

                vector<string> parsedline = ParseLine(line);

                if( !parsedline.empty() )
                    m_entries.push_back( std::move(parsedline) );
            }
        }
        else
        {
            stringstream sstr;
            sstr << "ERROR: Unable to open configuration file \"" <<path <<"\"!";
            throw runtime_error(sstr.str());
        }
    }
    
    void ConfigHandler::Write( const std::string & path )const
    {
        ofstream configfile(path);
        if( configfile.good() && configfile.is_open() )
        {
            for( const auto & entry : m_entries )
            {
                for( const auto & str : entry )
                    configfile << "\"" <<str <<"\" ";

                configfile << "\n";
            }
        }
        else
        {
            stringstream sstr;
            sstr << "ERROR: Unable to write configuration file to \"" <<path <<"\"!";
            throw runtime_error(sstr.str());
        }
    }

};};