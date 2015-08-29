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
#include <type_traits>

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
    void         UnsignedIntToByteBuff( unsigned int value, byte buff[] );


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
    void           Int16ToByteBuff( unsigned short value, byte outbuff[] );


    /*********************************************************************************************
        WriteIntToByteVector
            Tool to write integer values into a byte vector!
            Returns the new pos of the iterator after the operation.
    *********************************************************************************************/
    template<class T, class _outit>
        inline _outit WriteIntToByteVector( T val, _outit itout, bool basLittleEndian = true )
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
            for( int i = (sizeof(T)-1); i >= 0 ; --i, ++itout )
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
        inline T ReadIntFromByteVector( _init & itin, bool basLittleEndian = true ) //#TODO : Need to make sure that the iterator is really incremented!
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
            for( int i = (sizeof(T)-1); i >= 0; --i, ++itin )
                lambdaShiftBitOr(i);
        }

        return out_val;
    }

    /*********************************************************************************************
        ReadIntFromByteContainer
            Tool to read integer values from a byte container!
            
            #NOTE :The iterator is passed by copy here !! And the incremented iterator is returned!
    *********************************************************************************************/
    template<class T, class _init> 
        inline _init ReadIntFromByteContainer( T & dest, _init itin, bool basLittleEndian = true ) //#TODO : Need to make sure that the iterator is really incremented!
    {
        dest = ReadIntFromByteVector<typename T>( itin, basLittleEndian );
        return itin;
    }

    /*********************************************************************************************
        ChangeValueOfASingleByte
            Allows to change the value of a single byte in a larger type! 
            As if it was an array.
    *********************************************************************************************/
    template< class T >
        inline T ChangeValueOfASingleByte( T containinginteger, uint8_t newvalue, uint32_t byteoffset  )
    {
        static_assert( std::is_pod<T>::value, "ChangeValueOfASingleByte(): Tried to change a byte on a non-POD type!!" );
        T mask = 0xFFu,
          val  = newvalue;

        mask = (mask << (sizeof(T) - byteoffset) * 8); //Am I over cautious ?
        val  = (val  << (sizeof(T) - byteoffset) * 8);

        return ( (val & mask) | containinginteger );
    }

    /*********************************************************************************************
        ChangeValueOfASingleBit
    *********************************************************************************************/
    template< class T >
        inline T ChangeValueOfASingleBit( T containinginteger, uint8_t newvalue, uint32_t offsetrighttoleft  )
    {
        static_assert( std::is_pod<T>::value, "ChangeValueOfASingleBit(): Tried to change a bit on a non-POD type!!" );
        if( offsetrighttoleft >= (sizeof(T) * 8) )
            throw std::overflow_error("ChangeValueOfASingleBit(): Offset too big for integer type specified!");

        //Clean the bit then OR the value!
        return ( (0x01u & newvalue) << offsetrighttoleft ) | (containinginteger & ( ~(0x01u << offsetrighttoleft) ) );
    }

    /*********************************************************************************************

        * offsetrighttoleft : offset of the bit to isolate from right to left.
    *********************************************************************************************/
    template< class T >
        inline T GetBit( T containinginteger, uint32_t offsetrighttoleft  )
    {
        static_assert( std::is_pod<T>::value, "GetBit(): Tried to get a bit on a non-POD type!!" );
        if( offsetrighttoleft >= (sizeof(T) * 8) )
            throw std::overflow_error("GetBit(): Offset too big for integer type specified!");

        //Isolate, then shift back to position 1 !
        return (containinginteger & ( (0x01u << offsetrighttoleft) ) ) >> offsetrighttoleft;
    }

    /*********************************************************************************************
        IsolateBits
            Isolate some adjacent bits inside a value of type T, 
            and shift them back to offset 0. 

            * nbbits    : Nb of bits to isolate. Used to make the bitmask.
            * bitoffset : Offset of the bits from the right to the left. From the last bit. 

            Ex1 : we want those bits 0000 1110, the params are ( 0xE, 3, 1 ),  returns 0000 0111
            Ex2 : we want those bits 0011 0000, the params are ( 0x30, 2, 4 ), returns 0000 0011
    *********************************************************************************************/
    template<class T>
        inline T IsolateBits( T src, unsigned int nbBits, unsigned int bitoffset )
    {
        static_assert( std::is_pod<T>::value, "IsolateBits(): Tried to isolate bits of a non-POD type!!" );
        T mask = static_cast<T>( ( pow( 2, nbBits ) - 1u ) ); //Subtact one to get the correct mask
        return ( ( src & (mask << bitoffset) ) >> bitoffset );
    }

    /*********************************************************************************************
        Helper method to get whether a certain bit's state as a boolean, 
        instead of only isolating it.
    *********************************************************************************************/
    template< class T >
        inline bool IsBitOn( T containinginteger, uint32_t offsetrighttoleft  ) 
    { 
        return GetBit( containinginteger, offsetrighttoleft ) > 0; 
    }

    /*
        WriteStrToByteContainer
            Write a c string to a byte container, via iterator.
            strl is the length of the string!
    */
    template<class _outit> 
        _outit WriteStrToByteContainer( _outit itwhere, const char * str, size_t strl )
    {
        //#FIXME: The static assert below is broken with non-backinsert iterators
        //static_assert( typename std::is_same<typename _init::container_type::value_type, uint8_t>::type::value, "WriteStrToByteContainer: Target container's value_type can't be assigned bytes!" );
        
        //#FIXME: Highly stupid... If we cast the values inside the string to the target type, that means we could easily
        //        go out of bound if the target container isn't containing bytes..
        //return std::copy_n( reinterpret_cast<const typename _outit::container_type::value_type*>(str), strl,  itwhere );

        for( size_t i = 0; i < strl; ++i, ++itwhere )
        {
            (*itwhere) = str[i];/*static_cast<typename _outit::container_type::value_type>(str[i]);*/
        }
        return itwhere;
    }

    /*
        WriteStrToByteContainer
            Write a c string to a byte container, via iterator.
            strl is the length of the string!
    */
    template<class _init> 
        _init ReadStrFromByteContainer( _init itread,  char * str, size_t strl )
    {
        //#FIXME: The static assert below is broken with non-backinsert iterators
        //static_assert( typename std::is_same<typename _init::container_type::value_type, uint8_t>::type::value, "WriteStrToByteContainer: Target container's value_type can't be assigned bytes!" );
        
        for( size_t i = 0; i < strl; ++i, ++itread )
            str[i] = *itread;
        return itread;

        //return std::copy_n( , strl, reinterpret_cast<const typename _init::container_type::value_type*>(str) );
    }

    /*
        WriteStrToByteContainer
            Write a std::string as a null-terminated string into a byte container!

            If is the string has only a \0 character, only that character will be written!
    */
    template<class _outit> 
        _outit WriteStrToByteContainer( _outit itwhere, const std::string & str )
    {
        //if( str.empty() )
        //{
        //    (*itwhere) = '\0';
        //    ++itwhere;
        //    return itwhere;
        //}
        //else
            return WriteStrToByteContainer( itwhere, str.c_str(), str.size()+1 );
    }

    /*********************************************************************************************
        AppendPaddingBytes
            This function takes a back insert iterator and the length of the container to append padding
            to, along with the divisor to determine how much padding is needed.
    *********************************************************************************************/
    template<class _backinit>
        void AppendPaddingBytes( _backinit itinsertat, unsigned int lentoalign, unsigned int alignon, const uint8_t PadByte = 0 )
    {
    //# Insert padding at the current write position, to align the next entry on "alignon" bytes
        if( (lentoalign % alignon) != 0 )
        {
            uint32_t lenpadding = ( CalcClosestHighestDenominator( lentoalign, alignon ) -  lentoalign );
            for( unsigned int ctpad = 0; ctpad < lenpadding; ++ctpad, ++itinsertat )
                itinsertat = PadByte;
        }
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