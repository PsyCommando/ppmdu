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
    std::vector<std::string> ReadTextFileLineByLine( const std::string & filepath );


    /*
        This class redirect the stream for the duration of its lifetime.
    */
    //template<class _StremaTy>
    //    class streamRedirector
    //{
    //public:
    //    typedef _StreamTy stream_t;

    //    streamRedirector( stream_t & stream, std::streambuf * target )
    //        :m_stream(stream)
    //    {
    //        m_oldStreambuf = m_stream.rdbuf(target);
    //    }
    //    
    //    virtual ~streamRedirector()
    //    {
    //        m_stream.rdbuf(m_oldStreambuf);
    //    }

    //protected:
    //    std::streambuf * m_oldStreambuf;
    //    stream_t       & m_stream;
    //};

    ///*
    //*/
    //template<class _StreamTy>
    //    class strRedirectToFile : public streamRedirector<_StreamTy>
    //{
    //public:
    //    typedef _StreamTy stream_t;

    //    strRedirectToFile( stream_t & stream, const std::string & outfilepath )
    //        :m_redirect()
    //    {
    //    }

    //private:
    //};

};};

#endif