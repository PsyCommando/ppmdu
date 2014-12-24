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

    const colorRGB24 colorRGB24::getAsRGB24()const
    {
        //A little warning about this..
#ifndef COLOR_RGB24_WARN_OFF
    #ifdef _DEBUG
        cerr<<"PERFORMANCE WARNING: colorRGB24::getAsRGB24() calling this on a colorRGB24 object copies the object needlesly!\n"
            <<"This warning will disapear in the release build! You can turn it off in debug by defining COLOR_RGB24_WARN_OFF !\n";
    #endif
#endif
        return (*this);
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
};