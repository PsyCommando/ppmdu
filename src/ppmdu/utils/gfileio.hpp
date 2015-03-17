#ifndef G_FILE_IO_HPP
#define G_FILE_IO_HPP
/*
gfileio.hpp
2014/12/21
psycommando@gmail.com
Description: A few utilities to encapsulate writing stuff to files.
*/
#include <vector>
#include <string>
#include <cstdint>
#include <locale>
//#include <iostream>

namespace utils{ namespace io
{
    /************************************************************************
        ReadFileToByteVector
            Read the file content straight into a byte vector, with no
            processing at all. Takes the path to the file and a vector to
            put the data as parameters.
    ************************************************************************/
    void                 ReadFileToByteVector(const std::string & path, std::vector<uint8_t> & out_filedata);
    std::vector<uint8_t> ReadFileToByteVector(const std::string & path );

    /************************************************************************
        WriteByteVectorToFile
            Write the byte vector content straight into a file, with no
            processing at all. Takes the path to the file and a vector with
            the data as parameters.
    ************************************************************************/
    void WriteByteVectorToFile(const std::string & path, const std::vector<uint8_t> & in_filedata);


    /*
        Reads a file line by line, and put each lines into a vector of string.
    */
    std::vector<std::string> ReadTextFileLineByLine( const std::string & filepath, 
                                                     const std::locale & txtloc = std::locale::classic() );

    /*
        Write a file line by line, from a vector of string.
    */
    void WriteTextFileLineByLine( const std::vector<std::string> & data, 
                                  const std::string              & filepath, 
                                  const std::locale              & txtloc = std::locale::classic() );

};};

#endif