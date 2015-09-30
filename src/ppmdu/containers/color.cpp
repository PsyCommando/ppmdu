/*
*/
#include "color.hpp"
#include <iostream>
using namespace std;

namespace gimg
{
//==================================================================
// colorRGB24
//==================================================================
    colorRGB24::colorRGB24() 
        :color_container<uint8_t,3,colorRGB24>(), red(m_colcomponents[0]), green(m_colcomponents[1]), blue(m_colcomponents[2])
    { 
    }

    colorRGB24::colorRGB24( colordata_t r, colordata_t g, colordata_t b )
        :color_container<uint8_t,3,colorRGB24>(), red(m_colcomponents[0]), green(m_colcomponents[1]), blue(m_colcomponents[2])
    {
        red   = r;
        green = g;
        blue  = b;
    }

    colorRGB24 colorRGB24::getAsRGB24()const
    {
        //A little warning about this..
#ifndef COLOR_RGB24_WARN_OFF
    #ifdef _DEBUG
        cerr<<"PERFORMANCE WARNING: colorRGB24::getAsRGB24() calling this on a colorRGB24 object copies the object needlesly!\n"
            <<"This warning will disapear in the release build! You can turn it off in debug by defining COLOR_RGB24_WARN_OFF !\n";
    #endif
#endif
        return move(colorRGB24(*this));
    }
    void colorRGB24::setFromRGB24( uint8_t r, uint8_t g, uint8_t b )
    {
        red   = r;
        green = g;
        blue  = b;
    }

    colorRGB24::colorRGB24( const colorRGB24 & other )
        :color_container<uint8_t, 3, colorRGB24>(), red(m_colcomponents[0]), green(m_colcomponents[1]), blue(m_colcomponents[2])
    { m_colcomponents = other.m_colcomponents; }

    colorRGB24 & colorRGB24::operator=( const colorRGB24 & other )
    { 
        m_colcomponents = other.m_colcomponents;
        return *this;
    }

//==================================================================
// colorRGBX32
//==================================================================
    colorRGBX32::colorRGBX32()
        :color_container<uint8_t,4,colorRGB24>(), 
        _red(m_colcomponents[0]), 
        _green(m_colcomponents[1]), 
        _blue(m_colcomponents[2]), 
        _x(m_colcomponents[3])
    {}


    colorRGBX32::colorRGBX32( colordata_t r, colordata_t g, colordata_t b, colordata_t x )
        :color_container<uint8_t,4,colorRGB24>(), 
         _red(m_colcomponents[0]), 
         _green(m_colcomponents[1]), 
         _blue(m_colcomponents[2]), 
         _x(m_colcomponents[3])
    {
        _red   = r;
        _green = g;
        _blue  = g;
        _x     = x;
    }

    colorRGBX32::colorRGBX32( const colorRGBX32 & other )
        :color_container<uint8_t, 4, colorRGB24>(), 
        _red(m_colcomponents[0]), 
        _green(m_colcomponents[1]), 
        _blue(m_colcomponents[2]),
        _x(m_colcomponents[3])
    { m_colcomponents = other.m_colcomponents; }

    colorRGBX32 & colorRGBX32::operator=( const colorRGBX32 & other )
    { 
        m_colcomponents = other.m_colcomponents;
        return *this;
    }

    colorRGB24 colorRGBX32::getAsRGB24()const
    {
        return move(colorRGB24( _red, _green, _blue ));
    }
    void colorRGBX32::setFromRGB24( uint8_t r, uint8_t g, uint8_t b )
    {
        _red   = r;
        _green = g;
        _blue  = b;
        _x     = 0;
    }

    //class colorRGBX32 : public color_container<uint8_t, 4, colorRGB24>
    //{
    //public:
    //    colordata_t & red, & green, & blue, & unused; //Aliases

    //    colorRGBX32();
    //    colorRGBX32( colordata_t r, colordata_t g, colordata_t b, colordata_t x );
    //    colorRGBX32( const colorRGBX32 & other );
    //    colorRGBX32 & operator=( const colorRGBX32 & other );
    //    
    //    //Overrides
    //    ~colorRGBX32(){}
    //    const colorRGB24 getAsRGB24  ()const;
    //    void             setFromRGB24( uint8_t r, uint8_t g, uint8_t b );
    //};
};