#ifndef TEXT_STR_HPP
#define TEXT_STR_HPP
/*
text_str.hpp
2015/03/07
psycommando@gmail.com
Description:
    Utilities for handling the "MESSAGE/text_*.str" files used in PMD2.
*/
#include <ppmdu/pmd2/pmd2.hpp>
#include <cstdint>
#include <string>
#include <vector>
#include <locale>

namespace pmd2{ namespace filetypes
{
//============================================================================================
//  Constants
//============================================================================================
    //static const uint32_t    TextEStr_DefNBEntries = 18452; //EoS English

    static const std::string TextStr_FExt     = "str";
    static const std::string TextStr_EngFName = "text_e.str";
    //static const std::string TextStr_JapFName = "text_j.str";
    
//============================================================================================
//  Function
//============================================================================================
    
    /*
        ParseTextStrFile
            Parse a text_*.str file from PMD2, to a string vector.
    */
    std::vector<std::string> ParseTextStrFile( const std::string              & filepath, 
                                               eGameRegion                      gver,
                                               const std::locale              & txtloc = std::locale::classic() );

    /*
        WriteTextStrFile
            Write a string vector to a text_*.str file!
    */
    void                     WriteTextStrFile( const std::string              & filepath, 
                                               const std::vector<std::string> & text, 
                                               eGameRegion                      gver,
                                               const std::locale              & txtloc = std::locale::classic() );

//============================================================================================
//  Classes
//============================================================================================



};};

#endif 