#ifndef LSD_HPP
#define LSD_HPP
/*
lsd.hpp

Description: Tools for reading LSD files from the PMD2 scripts!
*/
#include <cstdint>
#include <deque>
#include <string>
#include <array>

namespace filetypes
{
    const std::string  LSD_FileExt  = "lsd";
    const unsigned int LSDStringLen = 8;  //8 characters

    /*
        Type for the LSD content
    */
    typedef std::deque< std::array<char,LSDStringLen> > lsddata_t;

    /*
        ParseLSD
            Parse a lsd file to the lsddata_t type.
    */
    lsddata_t ParseLSD( const std::string & fpath );

    /*
        WriteLSD
            Write a lsd file from the content supplied.
    */
    void WriteLSD( const lsddata_t & lsdcontent, const std::string & fpath );

};

#endif