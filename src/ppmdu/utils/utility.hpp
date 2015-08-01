#ifndef UTILITY_HPP
#define UTILITY_HPP
/*
utility.hpp
2014/12/21
psycommando@gmail.com
Description: A header with a bunch of useful includes for the PPMD utilities. 
*/
#include <chrono>
#include <string>
#include "gfileutils.hpp"
#include "gfileio.hpp"
#include "gstringutils.hpp"
#include "gbyteutils.hpp"
#include "poco_wrapper.hpp"
#include <iosfwd>
#include <type_traits>
#include <vector>

namespace utils
{
//===============================================================================================
// Struct
//===============================================================================================

    /************************************************************************
        data_array_struct
            A base structure for structures to be read from files, such as 
            headers, and any constant size blocks of data !
    ************************************************************************/
    struct data_array_struct
    {
        virtual ~data_array_struct(){}
        virtual unsigned int size()const=0;

        //The method expects that the interators can be incremented at least as many times as "size()" returns !
        virtual std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const = 0;

        //The method expects that the interators can be incremented at least as many times as "size()" returns !
        virtual std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom )     = 0;
    };

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
        //MrChronometer( const std::string name = "*", std::ostream * messageoutput = nullptr );
        //~MrChronometer();
        //static const std::string HOURS_Symbol;

        ChronoRAII( const std::string name = "*", std::ostream * messageoutput = nullptr )
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
    template< class _Ty, class _Ty2 > inline
        _Ty Clamp( _Ty val, _Ty2 min, _Ty2 max )
    {
        if( val < min )
            return min;
        else if( val > max )
            return max;
        else
            return val;
    }

};


#endif 