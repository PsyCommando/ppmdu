#ifndef G_STRING_UTILS_HPP
#define G_STRING_UTILS_HPP
/*
gstringutils.hpp
2014/12/21
psycommando@gmail.com
Description: Some functions for common operations on strings of characters.
*/
#include <string>
#include <algorithm>
#include <locale>


namespace utils
{
    /************************************************************************
        has_suffix
        *In :
            - const string &str		: string to search for the suffix.
            - const string &suffix	: string containing the suffix to look for.
        *Out:
            - bool					: Whether the suffix was found or not.

        *Description: Compare the end of a string with another string 
                      containg a suffix, and return true if the
                      suffix matches the end of the string.
    ************************************************************************/
    inline bool has_suffix(const std::string &str, const std::string &suffix)
    {
		return str.size() >= suffix.size() &&
				str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    }


    /************************************************************************
        RemCharFromPath
            Removes all instances of the specified char in the string!
    ************************************************************************/
    inline std::string RemCharFromString(std::string str, char ch)
    {
        auto itnewend = std::remove_if(str.begin(), str.end(), [&ch](char c){ return (c == ch); });
        return std::move(std::string(str.begin(), itnewend));
    }

    /************************************************************************
        StrRemoveAfter
            Returns the part before the position the delimiter is found!
    ************************************************************************/
    inline std::string StrRemoveAfter( const std::string & str, const std::string & delim )
    {
        return str.substr( 0, str.find( delim, 0 ) );
    }


    /*
        CompareStrIgnoreCase
            Compare 2 strings ignoring whether they're upper or lower case!
    */
    inline bool CompareStrIgnoreCase( const std::string & strA, const std::string & strB, std::locale curloc = std::locale::classic() )
    {
        if( strA.size() != strB.size() )
            return false;
        auto lambdacmpchar = [&curloc]( char c1, char c2 )->bool
        {
            return (std::tolower(c1, curloc ) == std::tolower(c2, curloc ));
        };
        return std::equal( strA.begin(), strA.end(), strB.begin(), lambdacmpchar );
    }

    /*
        MakeLowerCase
    */
    inline std::string MakeLowerCase( const std::string & str, std::locale curloc = std::locale::classic() )
    {
        std::string result;
        result.resize(str.size());
        std::transform( str.begin(), str.end(), result.begin(), [&curloc](char c1){return std::tolower(c1,curloc);} );
        return std::move(result);
    }
};

#endif 