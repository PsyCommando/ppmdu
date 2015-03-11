#ifndef INTEGER_ENCODING_HPP
#define INTEGER_ENCODING_HPP
/*
integer_encoding.hpp
2015/03/10
psycommando@gmail.com
Description:
    Contains utilities for decoding common ways to encode integers on less bytes.
*/
#include <array>
#include <vector>
#include <sstream>
#include <string>
#include <cstdint>
#include <ppmdu/utils/gbyteutils.hpp>

namespace utils
{
//=========================================================================================================================
//  Encoding 
//=========================================================================================================================

    /***********************************************************************************
    ***********************************************************************************/
    template<class T>
        inline T HandleEncodeByte( T thebyte, bool islowest, T curEncodedRes )
    {
        uint8_t curbyte = thebyte & 0x7Fu;
        if( islowest || curEncodedRes == 0 && curbyte == 0 ) //If we're the last byte OR we don't have a higher non-zero byte, and the byte we're handling is 0, return the current byte as-is !
             return curbyte;
        else
            return ( curbyte | 0x80 );
    }

    /***********************************************************************************
    ***********************************************************************************/
    template<class T>
        T EncodeAnInteger( T val )
    {
        static_assert( std::is_integral<T>::value, "EncodeInteger(): Type T is not an integer!" );
        static const int Sz_T     = sizeof(T);
        static const int BitsUsed = (Sz_T * 8) - Sz_T; //The number of bits we can use from the integer (we give up one bit per byte!)
    
        if( ( val & ~(utils::do_exponent_of_2_<BitsUsed>::value - 1) ) > 0 )
        {
            std::stringstream sstr;
            sstr << "ERROR: the integer specified has a value too high for being properly encoded! The highest "
                 << Sz_T << " bit(s) is/are reserved for the encoding! Strip those bits first!";
            throw std::overflow_error(sstr.str());
        }

        if( val == 0 )
            return 0;

        T result = 0;

        for( int i = (Sz_T-1); i >= 0; --i )
        {
            result |= ( HandleEncodeByte( utils::IsolateBits( val, 7, (i * 7) ), 
                                          (i == 0), 
                                          result ) 
                        << (i * 7) );
        }

        return result;
    }

    /***********************************************************************************
    ***********************************************************************************/
    template<class T, class _itbackins>
        inline _itbackins EncodeAnInteger( T val, _itbackins itout_encoded )
    {
        return utils::WriteIntToByteVector( EncodeAnInteger( val ), itout_encoded, false ); //output as big endian

        //T offsetSoFar = 0; //used to add up the sum of all the offsets up to the current one

        //for( const auto & anoffset : listoffsetptrs )
        //{
        //    T        offsetToEncode        = anoffset - offsetSoFar;
        //    bool     hasHigherNonZero      = false; //This tells the loop whether it needs to encode null bytes, if at least one higher byte was non-zero
        //    offsetSoFar = anoffset; //set the value to the latest offset, so we can properly subtract it from the next offset.

        //    //Encode every bytes of the 4 bytes integer we have to
        //    for( int32_t i = sizeof(int32_t); i > 0; --i )
        //    {
        //        uint8_t currentbyte = ( offsetToEncode >> (7 * (i - 1)) ) & 0x7Fu;
        //            
        //        if( i == 1 ) //the lowest byte to encode is special
        //        {
        //            //If its the last byte to append, leave the highest bit to 0 !
        //            itout_encoded = ( currentbyte );
        //            ++itout_encoded;
        //        }
        //        else if( currentbyte != 0 || hasHigherNonZero ) //if any bytes but the lowest one! If not null OR if we have encoded a higher non-null byte before!
        //        {
        //            //Set the highest bit to 1, to signifie that the next byte must be appended
        //            itout_encoded = ( currentbyte | 0x80u ); 
        //            ++itout_encoded;
        //            hasHigherNonZero = true;
        //        }
        //    }
        //}

        ////Append the closing 0
        //out_encoded.push_back(0);
    }

    /***********************************************************************************
        EncodeIntegers
            Encodes A list of integer values, without any further processing.
            Expects the ouput to be at least as big as the distance between itbeg and itend, plus 1 !
    ***********************************************************************************/
    template<class _init, class _outit>
        inline _outit EncodeIntegers( _init itbeg, _init itend, _outit itout )
    {
        static_assert( std::is_integral<typename _init::container_type::valu_type>::value, "EncodeIntegers(): list to encode is not a list of integers!" ); 
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
        DecodeAnInteger
            Returns the input iterator position after all the integer's encoded bytes were read.

            **This function is recursive.**
    */
    template<class _init, class T>
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
    template<class _init, class T>
        inline T DecodeAnInteger( _init & itbeg, _init itend )
    {
        static const unsigned int Sz_T = sizeof(T);
        T tempbuf = 0;
        return DecodeAnInteger( itbeg, itend, Sz_T, tempbuf );
    }

    /*
        DecodeIntegers
            Decodes a list of encoded integers.

            Returns the read positions iterator after all the integers were read.
    */
    template<class _init, class _outit>
        _init DecodeIntegers( _init itbeg, _init itend, _outit & itout )
    {
        typedef typename _outit::container_type::value_type val_ty;
        static_assert( sizeof(val_ty) > sizeof(typename typename _init::container_type::value_type), "DecodeIntegers(): Output type is smaller than the input type!" );
        
        bool wasNullTerminated = false;
        while( itbeg != itend )
        {
            val_ty value = 0;
            value = DecodeAnInteger<val_ty>( itbeg, itend );
            if( value != 0 )
            {
                (*itout) = value;
                ++itout;
            }
            else
            {
                wasNullTerminated = true;
                break;  //Stop when we hit the ending 0 !
            }
        }
        if( !wasNullTerminated )
            throw std::runtime_error("DecodeIntegers(): ERROR: The encoded integer list did not end on a 00 byte! Result is probably invalid!");
        return itbeg;
    }

    /*
    */
    template<class _init, class T>
        inline std::vector<T> DecodeIntegers( _init itbeg, _init itend )
    {
        std::vector<T> result;
        DecodeIntegers( itbeg, itend, std::back_inserter(result) );
        return std::move(result);
    }


    //std::vector<uint32_t> DecodeSIR0PtrOffsetList( const std::vector<uint8_t>  &ptroffsetslst )
    //{
    //    vector<uint32_t> decodedptroffsets( ptroffsetslst.size() ); //worst case scenario
    //    decodedptroffsets.resize(0);

    //    auto itcurbyte  = ptroffsetslst.begin();
    //    auto itlastbyte = ptroffsetslst.end();

    //    uint32_t offsetsum = 0; //This is used to sum up all offsets and obtain the offset relative to the file, and not the last offset
    //    uint32_t buffer    = 0; //temp buffer to assemble longer offsets
    //    uint8_t curbyte    = *itcurbyte;
    //    bool    LastHadBitFlag = false; //This contains whether the byte read on the previous turn of the loop had the bit flag indicating to append the next byte!

    //    while( itcurbyte != itlastbyte && ( LastHadBitFlag || (*itcurbyte) != 0 ) )
    //    {
    //        curbyte = *itcurbyte;
    //        buffer |= curbyte & 0x7Fu;
    //    
    //        if( (0x80u & curbyte) != 0 )
    //        {
    //            LastHadBitFlag = true;
    //            buffer <<= 7u;
    //        }
    //        else
    //        {
    //            LastHadBitFlag = false;
    //            offsetsum += buffer;
    //            decodedptroffsets.push_back(offsetsum);
    //            buffer = 0;
    //        }
    //    
    //        ++itcurbyte;
    //    }

    //    return std::move(decodedptroffsets);
    //}
};
#endif