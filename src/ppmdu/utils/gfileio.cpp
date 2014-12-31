#include "gfileio.hpp"
#include <cassert>
#include <fstream>
#include <exception>
using namespace std;

namespace utils{ namespace io
{
    void ReadFileToByteVector(const std::string & path, std::vector<uint8_t> & out_filedata)
    {
        ifstream inputfile(path, ios::binary | ios::ate); //ate : Opens the file, with the read pos at the end, to allow getting the file size

        if (!(inputfile.good() && inputfile.is_open()))
        {
            assert(false);
            throw exception("ReadFileToByteVector(): Error, impossible to open file!");
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
            assert(false);
            throw exception("WriteByteVectorToFile(): Error, impossible to open file!");
        }

        outputfile.write(reinterpret_cast<const char*>(filedata.data()), filedata.size());
    }
};};