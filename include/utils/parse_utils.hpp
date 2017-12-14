#ifndef PARSE_UTILS_HPP
#define PARSE_UTILS_HPP
/*
parse_utils.hpp
2015/03/22
psycommando@gmail.com
Description:
    A few useful utilities for parsing values.
*/
#include <sstream>
#include <string>
#include <iomanip>
#include <cstdint>

namespace utils
{
//=============================================================================================
// Utility Functions
//=============================================================================================
    /**************************************************************
        Parses a value directly as is.
        Return value is a parameter, to avoid having to specify
        a template parameter.
    **************************************************************/
    template<class _Ty>
        void parseValToValue( const std::string & str, _Ty out_val )
    {
        using namespace std;
        stringstream sstr(str);
        sstr <<dec << str;
        sstr >> out_val;
    }

    /**************************************************************
        Parse a value that might be an hex number or not.
    **************************************************************/
    template<class _Ty>
        void parseHexaValToValue( const std::string & str, _Ty & out_val )
    {
        using namespace std;
        stringstream sstr;
        std::size_t  foundprefix = str.find( "0x" );

        if( foundprefix != string::npos )
            sstr <<hex <<string( foundprefix + str.begin(), str.end() ).substr(2); //make sure the string begins at "0x" and skip "0x"
        else
            sstr <<dec <<str;

        //Handle bytes differently
        if(std::is_same<_Ty, uint8_t>::value)
        {
            uint16_t temp = 0;
            sstr >> temp;
            out_val = static_cast<uint8_t>(temp);
        }
        else if(std::is_same<_Ty, int8_t>::value)
        {
            int16_t temp = 0;
            sstr >> temp;
            out_val = static_cast<int8_t>(temp);
        }
        else
        {
            sstr >> out_val;
        }
    }

    template<class _Ty>
        inline _Ty parseHexaValToValue( const std::string & str )
    {
        using namespace std;
        _Ty          out_val;
        //stringstream sstr;
        //std::size_t  foundprefix = str.find( "0x" );

        //if( foundprefix != string::npos )
        //    sstr <<hex <<string( foundprefix + str.begin(), str.end() ).substr(2); //make sure the string begins at "0x" and skip "0x"
        //else
        //    sstr <<dec <<str;

        //sstr >> out_val;
        parseHexaValToValue( str, out_val );
        return out_val;
    }

    inline uint8_t parseByte( const std::string & str )
    {
        using namespace std;
        return static_cast<uint8_t>(parseHexaValToValue<uint16_t>(str));
    }

    inline int8_t parseSignedByte( const std::string & str )
    {
        using namespace std;
        return static_cast<int8_t>(parseHexaValToValue<int16_t>(str));
    }
};

#endif