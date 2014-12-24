#ifndef G_BYTE_UTILS_H
#define G_BYTE_UTILS_H
/*
gbyteutils.h
18/05/2014
psycommando@gmail.com

Description:
A bunch of simple tools for doing common tasks when manipulating bytes.
*/
#include <vector>
#include <cstdint>
#include <limits>

namespace utils 
{

    typedef uint8_t byte;

	//Consts
    //static const unsigned int SZ_INT32          = 0x4;
    //static const unsigned int SZ_INT16          = 0x2;
	static const unsigned int MASK_UINT32_BYTE0 = 0X000000FF,  //0000 0000 - 0000 0000 - 0000 0000 - 1111 1111
							  MASK_UINT32_BYTE1 = 0x0000FF00,  //0000 0000 - 0000 0000 - 1111 1111 - 0000 0000
							  MASK_UINT32_BYTE2 = 0x00FF0000,  //0000 0000 - 1111 1111 - 0000 0000 - 0000 0000
							  MASK_UINT32_BYTE3 = 0xFF000000;  //1111 1111 - 0000 0000 - 0000 0000 - 0000 0000


    /*********************************************************************************************
        conditional_value
            If _BoolExpr is true,  _OPTION_A will be contained in value. 
            If its false, it will be _OPTION_B instead.
    *********************************************************************************************/
    template< bool _BoolExpr, class T, T _OPTION_A, T _OPTION_B>
        struct conditional_value
    {
        static const T value = (_BoolExpr)? _OPTION_A : _OPTION_B;
    };


    /*********************************************************************************************
        do_exponent_of_2_
            Computes 2^n, where n is _Exponent, and store it into its static "value" member.
            Very handy for computing bitmasks and such at compile time.
    *********************************************************************************************/
    template<unsigned long long _Exponent> struct do_exponent_of_2_
    {
        static const unsigned long long value = do_exponent_of_2_< (_Exponent - 1u) >::value * 2;
    };

    template<> struct do_exponent_of_2_<0>
    {
        static const unsigned long long value = 1;
    };

	/*********************************************************************************************
		Name: LittleEndianToBigEndianUInt32
		In:
			- unsigned int val : bytes to convert from little to big endian.
		Out:
			- unsigned int	   : bytes converted to big endian.

		Description: 
		This simply swap the order of bytes from a little endian order to a big endian order.
		It works on a per byte level.

	*********************************************************************************************/
	unsigned int LittleEndianToBigEndianUInt32( unsigned int val );


	/*********************************************************************************************
		Name: ByteBuffToUnsignedInt
		In:
			- const char buff[] : bytes to assemble into an unsigned 32bits integer.
		Out:
			- unsigned int	    : an unsigned integer assembled from the bytes passed in the buffer.

		Description: 
		Assembles the bytes into an unsigned int, and applies masks to clean up any weirdness caused by bitshifts!

	*********************************************************************************************/
	unsigned int ByteBuffToUnsignedInt( const byte buff[] );
    void UnsignedIntToByteBuff( unsigned int value, byte buff[] );


    /*********************************************************************************************
        Name: ByteBuffToInt16
        In:
            - const char buff[] : bytes to assemble into an int16
        Out:
            - unsigned short    : the assembled int16 from the byte buffer.

        Description:
        Assembles the bytes in the buffer into an int16, applying masks to clean it up..
    *********************************************************************************************/
    unsigned short ByteBuffToInt16( const byte buff[] );
    void Int16ToByteBuff( unsigned short value, byte outbuff[] );


    /*********************************************************************************************
        WriteIntToByteVector
            Tool to write integer values into a byte vector!
            Returns the new pos of the iterator after the operation.
    *********************************************************************************************/
    template<class T, class _outit>
        _outit WriteIntToByteVector( T val, _outit itout, bool basLittleEndian = true )
    {
        static_assert( std::numeric_limits<T>::is_integer, "WriteIntToByteVector() : Used another types than an integer!" );

        auto lambdaShiftAssign = [&val]( unsigned int shiftamt )->uint8_t
        {
            T tempshift = 0;
            tempshift = ( val >> (shiftamt * 8) ) & 0xFF;
            return tempshift & 0xFF;
        };

        if( basLittleEndian )
        {
            for( unsigned int i = 0; i < sizeof(T); ++i, ++itout )
                (*itout) = lambdaShiftAssign( i );
        }
        else
        {
            for( unsigned int i = (sizeof(T)-1); i >= 0 ; --i, ++itout )
                (*itout) = lambdaShiftAssign( i );
        }

        return itout;
    }

    /*********************************************************************************************
        ReadIntFromByteVector
            Tool to read integer values from a byte vector!
            ** The iterator's passed as input, has its position changed !!
    *********************************************************************************************/
    template<class T, class _init> 
        T ReadIntFromByteVector( _init & itin, bool basLittleEndian = true ) //#TODO : Need to make sure that the iterator is really incremented!
    {
        static_assert( std::numeric_limits<T>::is_integer, "ReadIntFromByteVector() : Error, used another types than an integer!" );
        T out_val = 0;

        auto lambdaShiftBitOr = [&itin, &out_val]( unsigned int shiftamt )
        {
            T tmp = (*itin);
            out_val |= ( tmp << (shiftamt * 8) ) & ( 0xFF << (shiftamt*8) );
        };

        if( basLittleEndian )
        {
            for( unsigned int i = 0; i < sizeof(T); ++i, ++itin )
                lambdaShiftBitOr(i);
        }
        else
        {
            for( unsigned int i = (sizeof(T)-1); i >= 0; --i, ++itin )
                lambdaShiftBitOr(i);
        }

        return out_val;
    }

    /*********************************************************************************************
        ChangeValueOfASingleByte
            Allows to change the value of a single byte in a larger type! 
            As if it was an array.
    *********************************************************************************************/
    template< class T >
        T ChangeValueOfASingleByte( T containinginteger, uint8_t newvalue, uint32_t byteoffset  )
    {
        T mask = 0xFFu,
            val  = newvalue;

        mask = (mask << (sizeof(T) - byteoffset) * 8); //Am I over cautious ?
        val  = (val  << (sizeof(T) - byteoffset) * 8);

        return ( (val & mask) | containinginteger );
    }

//===============================================================================
//								Utility
//===============================================================================
    inline unsigned int GetNextInt32DivisibleBy16( unsigned int baseoffset )
    {
        if( (baseoffset % 16) != 0 )
        {
            return ( ( baseoffset / 16 ) + 1 ) * 16;
        }
        return baseoffset;
    }

};

#endif