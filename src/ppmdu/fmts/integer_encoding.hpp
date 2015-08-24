#ifndef INTEGER_ENCODING_HPP
#define INTEGER_ENCODING_HPP
/*
integer_encoding.hpp
2015/03/10
psycommando@gmail.com
Description:
    Contains utilities for decoding common ways to encode integers on less bytes.

    #TODO: Find the actual name of that encoding technique !
*/
#include <array>
#include <vector>
#include <sstream>
#include <string>
#include <cstdint>
#include <utils/gbyteutils.hpp>

namespace utils
{
//=========================================================================================================================
//  Encoding 
//=========================================================================================================================

    /***********************************************************************************
    ***********************************************************************************/
    inline uint8_t HandleEncodeByte( uint8_t thebyte, bool islowest, bool hasHigherNonZero )
    {
        uint8_t curbyte = thebyte & 0x7Fu;
        if( islowest || !hasHigherNonZero && curbyte == 0 ) //If we're the last byte OR we don't have a higher non-zero byte, and the byte we're handling is 0, return the current byte as-is !
             return curbyte;
        else
            return ( curbyte | 0x80 );
    }

    /***********************************************************************************
    ***********************************************************************************/
    template<class T, class _itbackins>
        inline _itbackins EncodeAnInteger( T val, _itbackins itout_encoded )
    {
        static_assert( std::is_integral<T>::value, "EncodeInteger(): Type T is not an integer!" );
        static const int SizeOfT  = sizeof(T);
        static const int BitsUsed = (SizeOfT * 8) - SizeOfT; //The number of bits we can use from the integer (we give up one bit per byte!)
    
        if( ( val & ~(utils::do_exponent_of_2_<BitsUsed>::value - 1) ) > 0 )
        {
            std::stringstream sstr;
            sstr << "ERROR: the integer specified has a value too high for being properly encoded! The highest "
                 << SizeOfT << " bit(s) is/are reserved for the encoding! Strip those bits first!";
            throw std::overflow_error(sstr.str());
        }

        if( val == 0 )
        {
            (*itout_encoded) = 0;
            ++itout_encoded;
            return itout_encoded;
        }

        T result = 0;

        bool hasHigherNonZero = false;
        for( int i = (SizeOfT-1); i >= 0; --i )
        {
            uint8_t curbyte = static_cast<uint8_t>( utils::IsolateBits( val, 7, (i * 7) ) );
            if( curbyte == 0 && !hasHigherNonZero )
                continue;
            else
            {
                hasHigherNonZero = true;
                (*itout_encoded) = ( HandleEncodeByte( curbyte, 
                                                       (i == 0), 
                                                       hasHigherNonZero ) );
                ++itout_encoded;
            }
        }

        return itout_encoded;
    }

    /***********************************************************************************
        EncodeIntegers
            Encodes A list of integer values, without any further processing.
            Expects the ouput to be at least as big as the distance between itbeg and itend, plus 1 !
    ***********************************************************************************/
    template<class _init, class _outit>
        inline _outit EncodeIntegers( _init itbeg, _init itend, _outit itout )
    {
        for(; itbeg != itend; ++itbeg )
            itout = EncodeAnInteger( *itbeg, itout );
        //Append 0
        (*itout) = 0;
        ++itout;
        return itout;
    }

    /***********************************************************************************
        EncodeIntegers
            Encodes A list of integer values, without any further processing.
    ***********************************************************************************/
    template<class _init>
        inline std::vector<uint8_t> EncodeIntegers( _init itbeg, _init itend )
    {
        std::vector<uint8_t> result;
        result.reserve( std::distance(itbeg,itend) + 1 );
        EncodeIntegers( itbeg, itend, std::back_inserter(result) );
        result.shrink_to_fit();
        return std::move(result);
    }


//=========================================================================================================================
//  Decoding 
//=========================================================================================================================

    /*
        DecodeAnInteger **This function is recursive.**
            Returns the input iterator position after all the integer's encoded bytes were read.

            Use the DecodeAnInteger below instead of calling this one directly.
    */
    template<class T, class _init>
        inline T DecodeAnInteger( _init & itbeg, _init & itend, unsigned int curbyindex, T curout )
    {
        if( itbeg == itend ){ throw std::out_of_range("DecodeAnInteger(): The integer to decode extends out of range!"); }
        T curbyte = (*itbeg);
        ++itbeg;
        curout |= (curbyte & 0x7F);
        if( ( curbyte & 0x80) > 0 )
        {
            if( curbyindex == 0 ){ throw std::overflow_error("DecodeAnInteger(): The integer to decode is larger than the output!"); }
            curout = curout << 7;
            return DecodeAnInteger( itbeg, itend, (curbyindex-1), curout );
        }
        else
            return curout;
    }

    /*
        DecodeAnInteger
            Use this to avoid having to pass the "curbyindex" parameter to the function above!

            Increments the itbeg iterator!
    */
    template<class T, class _init>
        inline T DecodeAnInteger( _init & itbeg, _init itend )
    {
        static const unsigned int Sz_T = sizeof(T);
        T tempbuf = 0;
        return DecodeAnInteger( itbeg, itend, Sz_T, tempbuf );
    }

    /*
        DecodeIntegers
            Decodes a list of encoded integers.

            * itend : Is NOT the end of an encoded sequence. 
                      Its just there as a sanity check to avoid going past the 
                      end of the container if no null terminating byte is ever found.

            Returns the read positions iterator after all the integers were read.
    */
    template<class _init, class _outit>
        _init DecodeIntegers( _init itbeg, _init itend, _outit & itout )
    {
        typedef typename _outit::container_type::value_type val_ty;
        static_assert( sizeof(val_ty) > sizeof(typename typename _init::value_type), 
                       "DecodeIntegers(): Output type is smaller than the input type!" );
        
        bool wasNullTerminated = false;
        while( itbeg != itend )
        {
            val_ty value = 0;

            //If we're on a byte sequence that begin with zero, don't bother!
            if( (*itbeg) != 0 )
            {
                value = DecodeAnInteger<val_ty>( itbeg, itend );
                (*itout) = value;
                ++itout;
            }
            else
            {
                ++itbeg;
                wasNullTerminated = true;
                break;  //Stop when we hit the ending 0 !
            }
        }

        if( !wasNullTerminated )
            throw std::runtime_error("DecodeIntegers(): The encoded integer list did not end on a 00 byte! Result is probably invalid!");
        
        return itbeg;
    }

    /*
        DecodeIntegers
            Decodes a list of encoded integers.

            * itend : Is NOT the end of an encoded sequence. 
                      Its just there as a sanity check to avoid going past the 
                      end of the container if no null terminating byte is ever found.

            Returns a vector with all the decoded integers.
    */
    template<class T, class _init>
        inline std::vector<T> DecodeIntegers( _init itbeg, _init itend )
    {
        std::vector<T> result;
        DecodeIntegers( itbeg, itend, std::back_inserter(result) );
        return std::move(result);
    }

};
#endif