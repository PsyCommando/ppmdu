#ifndef UTILITY_HPP
#define UTILITY_HPP
/*
utility.hpp
2014/12/21
psycommando@gmail.com
Description: A header with a bunch of useful includes for the PPMD utilities. 
*/
#include "gfileutils.hpp"
#include "gfileio.hpp"
#include "gstringutils.hpp"
#include "gbyteutils.hpp"
#include "poco_wrapper.hpp"
#include <chrono>
#include <string>
#include <iosfwd>
#include <type_traits>
#include <vector>
#include <sstream>

namespace utils
{
//===============================================================================================
// Struct
//===============================================================================================

    /************************************************************************
        Resolution
            A struct with human readable naming to make handling resolution
            values easier.
    ************************************************************************/
    struct Resolution
    {
        uint32_t width, height;

        inline bool operator==( const Resolution & right )const
        {
            return (this->height == right.height) && (this->width == right.width);
        }

        friend std::ostream & operator<<(std::ostream &os, Resolution &res);
    };

    /************************************************************************
        value_limit
            Struct meant to store the limits for a given value.
            Useful for validating ranges while parsing for instance.
    ************************************************************************/
    template<class _ValTy>
        struct value_limits
    {
        typedef _ValTy myty;
        myty min_; //Minimum Value
        myty def_; //Default Value
        myty max_; //Maximum Vale
        myty mid_; //Middle Value
    };


    /*
        This is used to get the symbol for each standard time unit.
    */
    template<class TimeScaleT_symbol> struct gtimesymbol     { static const std::string symbol; };
    template<> struct gtimesymbol<std::chrono::hours>        { static const std::string symbol; };
    template<> struct gtimesymbol<std::chrono::minutes>      { static const std::string symbol; };
    template<> struct gtimesymbol<std::chrono::seconds>      { static const std::string symbol; };
    template<> struct gtimesymbol<std::chrono::milliseconds> { static const std::string symbol; };
    template<> struct gtimesymbol<std::chrono::microseconds> { static const std::string symbol; };
    template<> struct gtimesymbol<std::chrono::nanoseconds>  { static const std::string symbol; };

    /************************************************************************
        ChronoRAII
            A small utility RAII class that that tells the time elapsed 
            between its construction and destruction.
    ************************************************************************/
    template<class TimescaleT = std::chrono::milliseconds>
        struct ChronoRAII
    {
        typedef TimescaleT timescale_t;

        ChronoRAII( const std::string & name = "*", std::ostream * messageoutput = nullptr )
            :_name(name)
        {
            _start  = std::chrono::steady_clock::now();

            if( messageoutput == nullptr )
                _output = &(std::cout);
            else
                _output = messageoutput;
        }

        ~ChronoRAII()
        {
            auto myduration = std::chrono::steady_clock::now() - _start;
            (*_output) << "#" <<_name << ": Time elapsed : " << std::chrono::duration_cast<timescale_t>( myduration ).count() 
                       << gtimesymbol<timescale_t>::symbol << "\n";
        }

        /*
            Get the current time elapsed, automatically casted in the unit desired.
        */
        template<class DesiredTScale = TimescaleT>
            DesiredTScale getElapsed()const
        {
            auto myduration = std::chrono::steady_clock::now() - _start;
            return std::chrono::duration_cast<DesiredTScale>( myduration );
        }

        std::chrono::steady_clock::time_point  _start;
        std::string                            _name;
        std::ostream                         * _output;
    };

    //Default chrono
    typedef ChronoRAII<> MrChronometer;

//===============================================================================================
// Classes
//===============================================================================================


//===============================================================================================
// Function
//===============================================================================================

    /************************************************************************
        SimpleHandleException
            A function to avoid having to rewrite a thousand time the same 2 
            lines of code in case of exception.

            Simply write the exception's "what()" function output to "cerr", 
            and triggers an assert in debug.
    ************************************************************************/
    void SimpleHandleException( const std::exception & e );


    /************************************************************************************
        advAsMuchAsPossible
            Advance an iterator until either the given number of increments are made, 
            or the end is reached!
    ************************************************************************************/
    template<class _init>
        inline _init advAsMuchAsPossible( _init iter, _init itend, unsigned int displacement )
    {
        for( unsigned int i = 0; i < displacement && iter != itend; ++i, ++iter );
        return iter;
    }


    /************************************************************************************
        CountEqualConsecutiveElem
            Count the ammount of similar consecutive values between two sequences. 
            It stops counting once it stumbles on a differing value.
    ************************************************************************************/
    template<class _init>
        inline unsigned int CountEqualConsecutiveElem( _init first_1,
                                                       _init last_1,
                                                       _init first_2,
                                                       _init last_2 )
    {
        unsigned int cpt = 0;
        for(; first_1 != last_1 && 
              first_2 != last_2 &&
              *first_1 == *first_2; 
              ++first_1, ++first_2, ++cpt );
        return cpt;
    }

    /************************************************************************************
        Clamp
            Clamps a a value between min and max. 
            Uses bigger than and smaller than operators.
    ************************************************************************************/
    template< class _Ty, class _Ty2 > 
        inline _Ty Clamp( _Ty val, _Ty2 min, _Ty2 max )
    {
        if( val < min )
            return min;
        else if( val > max )
            return max;
        else
            return val;
    }

    /************************************************************************************
        PortablePause
            Use this to make a pause for user input that will work on any OS.
    ************************************************************************************/
    void PortablePause();


    /*
        PrintNestedExceptions
            Simple recursive function to print nested exception.
    */
    inline void PrintNestedExceptions( std::ostream & output, const std::exception & e, unsigned int level = 0 )
    {
        if( level == 0 )
            output <<std::string(level,' ') <<"Exception: " << e.what() <<"\n";
        else
            output <<std::string(level,' ') <<"(" <<level  <<") : " << e.what() <<"\n";
        try
        {
            std::rethrow_if_nested(e);
        }
        catch(const std::exception & e)
        {
            PrintNestedExceptions(output, e, level + 1);
        }
        catch(...){}
    }


    //!#TODO: Move me into a formating header or something!!!
    /*
        NumberToHexString
    */
    template<typename _intty>
        std::string NumberToHexString( _intty val )
    {
        std::stringstream sstr; 
        sstr <<std::hex <<"0x" <<std::uppercase <<val;
        return std::move(sstr.str());
    }

};


#endif 