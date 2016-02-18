#include "gfileio.hpp"
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <exception>
using namespace std;

namespace utils{ namespace io
{
    void ReadFileToByteVector(const std::string & path, std::vector<uint8_t> & out_filedata)
    {
        ifstream inputfile(path, ios::in | ios::binary | ios::ate); //ate : Opens the file, with the read pos at the end, to allow getting the file size

        if (!(inputfile.good() && inputfile.is_open()))
        {
            stringstream sstr;
            sstr <<"ReadFileToByteVector() : impossible to open file \"" <<path <<"\"!\n";
            throw runtime_error(sstr.str());
        }

        uint64_t filesize = inputfile.tellg();

        //Ensure we got the capacity for reading it!
        out_filedata.resize(static_cast<uint32_t>(filesize));

        //Reset the read position and file state
        inputfile.seekg(0, ios::beg);

        //Copy the whole file to a vector
        inputfile.read(reinterpret_cast<char*>(out_filedata.data()), filesize); //needing to do a cast because streams are dumb and are needing some love
    }

    std::vector<uint8_t> ReadFileToByteVector( const std::string & path )
    {
        std::vector<uint8_t> output;
        ReadFileToByteVector(path, output);
        return std::move( output );
    }

    /*
    Write the byte vector content straight into a file, with no processing at all.
    Takes the path to the file and a vector with the data as parameters.
    */
    void WriteByteVectorToFile(const std::string & path, const std::vector<uint8_t> & filedata)
    {
        ofstream outputfile(path, ios::binary);

        if (!(outputfile.good() && outputfile.is_open()))
        {
            stringstream sstr;
            sstr <<"WriteByteVectorToFile() : impossible to open file \"" <<path <<"\"!\n";
            throw runtime_error(sstr.str());
        }

        outputfile.write(reinterpret_cast<const char*>(filedata.data()), filedata.size());
    }

    std::vector<std::string> ReadTextFileLineByLine( const std::string & filepath, const std::locale & txtloc )
    {
        vector<string> stringlist;
        ifstream       strfile( filepath );

        if( !( strfile.good() && strfile.is_open() ) )
        {
            std::stringstream strs;
            strs << "ReadTextFileLineByLine(): Error: file is missing or cannot be opened ! Path :\n"
                 << filepath;
            throw runtime_error(strs.str());
        }
        strfile.imbue(txtloc);

        while( !strfile.eof() && !strfile.bad() )
        {
            string tmp;
            getline( strfile, tmp );
            stringlist.push_back(tmp);
        }

        return std::move(stringlist);
    }

    void WriteTextFileLineByLine( const std::vector<std::string> & data, const std::string & filepath, const std::locale & txtloc )
    {
        static const char EOL = '\n';
        ofstream output( filepath/*, std::ios::binary*/ );

        if( !( output.good() && output.is_open() ) )
        {
            std::stringstream strs;
            strs << "WriteTextFileLineByLine(): Error: file is missing or cannot be opened ! Path :\n"
                 << filepath;
            throw runtime_error(strs.str());
        }
        output.imbue(txtloc);

        const unsigned int lastentry = data.size() - 1;
        for( unsigned int i = 0; i < data.size(); ++i )// const auto & entry : data )
        {
            //output.write( entry.c_str(), entry.size() + 1 );
            //output.write( &EOL, 1 );
            output << data[i];
            if( i < lastentry ) //Avoid the extra unneeded EoL
               output <<"\n";
        }
    }


};};