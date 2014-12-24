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
};

#endif 