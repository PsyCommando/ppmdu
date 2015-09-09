#include "utility.hpp"
//#include <Poco/DirectoryIterator.h>
#include "gfileutils.hpp"
#include <iostream>
#include <chrono>
#include <fstream>
#include <cassert>
#include <future>
using namespace std;

namespace utils
{
    const std::string gtimesymbol<std::chrono::hours>::symbol        = "h";
    const std::string gtimesymbol<std::chrono::minutes>::symbol      = "m";
    const std::string gtimesymbol<std::chrono::seconds>::symbol      = "s";
    const std::string gtimesymbol<std::chrono::milliseconds>::symbol = "ms";
    const std::string gtimesymbol<std::chrono::microseconds>::symbol = "us";
    const std::string gtimesymbol<std::chrono::nanoseconds>::symbol  = "ns";

//============================================================================================
//  Constants
//============================================================================================



//============================================================================================
//  Simple Exception Handler
//============================================================================================

    void SimpleHandleException( const std::exception & e )
    {
#ifdef _DEBUG
        assert(false);
#endif
        cerr << "<!>-EXCEPTION: " <<e.what() <<endl;
    }

//============================================================================================
//  Debug Assert
//============================================================================================
    template<class _IsDebug>
        struct DbgAssertFalse
    {};

    template<>
        struct DbgAssertFalse<std::true_type>
    {
        DbgAssertFalse()
        {
            assert(false);
        }
    };

#ifdef _DEBUG
    typedef DbgAssertFalse<std::true_type>  AssertFalse;
#else
    typedef DbgAssertFalse<std::false_type> AssertFalse;
#endif
        
//============================================================================================
//  Ostream Operator for Resolution ###FIXME: Why is this here ?
//============================================================================================
    std::ostream & operator<<(std::ostream &os, utils::Resolution &res)
    {
        return os << res.width <<"x" <<res.height;
    }

//
//
//
    void PortablePause()
    {
    #ifdef WIN32
            system("pause");
    #else
            char c = 0;
            cout <<"Press any key and enter to continue..";
            cin >> c;
    #endif
    }
};