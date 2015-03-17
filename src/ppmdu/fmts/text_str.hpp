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
#include <locale>

namespace pmd2{ namespace filetypes
{
//============================================================================================
//  Constants
//============================================================================================
    static const uint32_t    TextEStr_DefNBEntries = 18452;
    
    //English locale for English text
    static const std::string TextEnglishLocale     = 
#ifdef _WIN32 || defined _WIN64
        "en-US";
#else
        "";
    static_assert(false, "If you're compiling this on another platform than windows, please put a valid locale name here!");
#endif

    //Japanese locale for Japanese text
    static const std::string TextJapaneseLocale    = 
#ifdef _WIN32 || defined _WIN64
        ".932";    //shift_jis - ANSI/OEM Japanese; Japanese (Shift-JIS)
#else
        "";
    static_assert(false, "If you're compiling this on another platform than windows, please put a valid locale name here!");
#endif


    //  Explorers of Sky string indexes
    static const unsigned int TextStr_PokeNames_EoS         = 8734; //Pokemon names begin at entry #8734, in Explorers of sky!
    static const unsigned int TextStr_PokeCategoryNames_EoS = 9334;
    static const unsigned int TextStr_MoveNames_EoS         = 8173; 
    static const unsigned int TextStr_ItemNames_EoS         = 6773; 

//============================================================================================
//  Function
//============================================================================================
    
    /*
        ParseTextStrFile
            Parse a text_*.str file from PMD2, to a string vector.
    */
    std::vector<std::string> ParseTextStrFile( const std::string              & filepath, 
                                               const std::locale              & txtloc = std::locale(TextEnglishLocale) );

    /*
        WriteTextStrFile
            Write a string vector to a text_*.str file!
    */
    void                     WriteTextStrFile( const std::string              & filepath, 
                                               const std::vector<std::string> & text, 
                                               const std::locale              & txtloc = std::locale(TextEnglishLocale) );

//============================================================================================
//  Classes
//============================================================================================



};};

#endif 