#include "gbyteutils.hpp"
/*
gbyteutils.cpp
18/05/2014
psycommando@gmail.com

Description:
A bunch of simple tools for doing common tasks when manipulating bytes.
*/
#include <vector>
#include <algorithm>
#include <cstdint>
using namespace std;

namespace utils 
{


	unsigned int LittleEndianToBigEndianUInt32( unsigned int val )
	{
		unsigned int b[4] = {0};
		b[0] = (val & MASK_UINT32_BYTE0) << 24u;
		b[1] = (val & MASK_UINT32_BYTE1) <<  8u;
		b[2] = (val & MASK_UINT32_BYTE2) >>  8u;
		b[3] = (val & MASK_UINT32_BYTE3) >> 24u;

		return ( b[0] | b[1] | b[2] | b[3] );
	}

	unsigned int ByteBuffToUnsignedInt( const byte buff[] )
	{
		unsigned int value = 0;

		value =   buff[0]		  & MASK_UINT32_BYTE0;
		value |= (buff[1] <<  8u) & MASK_UINT32_BYTE1;
		value |= (buff[2] << 16u) & MASK_UINT32_BYTE2;
		value |= (buff[3] << 24u) & MASK_UINT32_BYTE3;

		return value;
	}

    void UnsignedIntToByteBuff( unsigned int value, byte buff[] )
    {
	    buff[3] = static_cast<char>( (value >> 24u) & MASK_UINT32_BYTE0 );
	    buff[2] = static_cast<char>( (value >> 16u) & MASK_UINT32_BYTE0 );
	    buff[1] = static_cast<char>( (value >>  8u) & MASK_UINT32_BYTE0 );
	    buff[0] = static_cast<char>(  value         & MASK_UINT32_BYTE0 );
    }

    unsigned short ByteBuffToInt16( const byte buff[] )
    {
        unsigned int value = 0;
        value =   buff[0]		  & MASK_UINT32_BYTE0;
		value |= (buff[1] <<  8u) & MASK_UINT32_BYTE1;

        return value;
    }

    void Int16ToByteBuff( unsigned short value, byte outbuff[] )
    {
	    outbuff[1] = static_cast<char>( (value >>  8u) & MASK_UINT32_BYTE0 );
	    outbuff[0] = static_cast<char>(  value         & MASK_UINT32_BYTE0 );
    }


    unsigned int readInt32FromByteVector( std::vector<byte>::const_iterator first )
    {
        byte tmpbuff [4] = {0};
        copy_n( first, 4, tmpbuff );
        return ByteBuffToUnsignedInt( tmpbuff );
    }

    unsigned short readInt16FromByteVector( std::vector<byte>::const_iterator first )
    {
        byte tmpbuff[2] = {0};
        copy_n( first, 2, tmpbuff );
        return ByteBuffToInt16(tmpbuff);
    }


    //TODO: What to do with those ?
    uint32_t ReadUInt32FromUInt8VectorBigEndian( std::vector<uint8_t>::const_iterator itbeg ) //Little endian to big endian!
    {
        uint32_t result   = 0;
        uint8_t  cptbytes = 0;

        auto lambdaReadByte = [&result,&cptbytes](uint8_t val)
        {
            uint32_t tmp = (val << ( cptbytes * 8 )) & (0xff << ( cptbytes * 8 )); //Shift the ammount of bytes required! Apply mask because paranoia
            result |= tmp;
            ++cptbytes; //increment the counter
        };

        std::for_each( itbeg, itbeg+4, lambdaReadByte );

        return result;
    }

    uint32_t ReadUInt32FromUInt8VectorLittleEndian( std::vector<uint8_t>::const_iterator itbeg ) //Little endian to Little endian
    {
        uint32_t result   = 0;
        uint8_t  cptbytes = 0;

        auto lambdaReadByte = [&result,&cptbytes](uint8_t val)
        {
            uint32_t tmp = val << ( ( 4 - cptbytes) * 8 ); //Shift the ammount of bytes required!
            result |= tmp;
            ++cptbytes; //increment the counter
        };

        std::for_each( itbeg, itbeg+4, lambdaReadByte );

        return result;
    }

};