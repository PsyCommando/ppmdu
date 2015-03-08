#ifndef TEXT_STR_HPP
#define TEXT_STR_HPP
/*
text_str.hpp
2015/03/07
psycommando@gmail.com
Description:
    Utilities for handling the "MESSAGE/text_*.str" files used in PMD2.
*/
#include <cstdint>
#include <string>
#include <vector>

namespace pmd2{ namespace filetypes
{
//============================================================================================
//  Constants
//============================================================================================
    static const uint32_t TextEStr_DefNBEntries = 18452;

//============================================================================================
//  Function
//============================================================================================
    std::vector<std::string> ParseTextStrFile( const std::string & filepath );
    void                     WriteTextStrFile( const std::string & filepath, const std::vector<std::string> & text );

//============================================================================================
//  Classes
//============================================================================================



};};

#endif 