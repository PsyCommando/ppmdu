#ifndef HANDY_MATH_HPP
#define HANDY_MATH_HPP
/*
handymath.hpp
A bunch of common math operations put in here. Because they're handy!
*/


inline unsigned int CalcClosestHighestDenominator( unsigned int val, unsigned int divisor )
{
    return (( ( val / divisor ) + 1 ) * divisor);
}

inline unsigned int CalcClosestLowestDenominator( unsigned int val, unsigned int divisor )
{
    return (( val / divisor ) * divisor);
}


#endif