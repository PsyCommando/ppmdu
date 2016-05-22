#ifndef BASETYPES_H
#define BASETYPES_H
/*
basetypes.h
2014/09/14
psycommando@gmail.com
Description: Handle the definitions for basic types, and attempt to ensure portability.


##TODO: Is this really useful ?

*/
//!#DELETE : Delete this file, its useless!
#include <vector>
#include <array>
#include <string>
#include <exception>
#include <cstdint>  //<- If this include fails, your compiler doesn't support C++ 11. If some typedefs are wrong, 
                    //   your platform or compiler doesn't support those types.
                    //   This code was made to support C++ 11, on MSVC and GNU GCC compilers, used on x86-x64 PCs.

//=========================================================
//Some commonly re-used types
//=========================================================

namespace pmd2 { namespace types 
{
//=========================================================
// Typedefs
//=========================================================
    //Iterators
    typedef std::vector<uint8_t>::iterator       itbyte_t;
    typedef std::vector<uint8_t>::const_iterator constitbyte_t;

    //Vectors
    typedef std::vector<uint8_t> bytevec_t;      //Alias for vector<uint8_t>
    typedef bytevec_t::size_type bytevec_szty_t; //size_type for vector<uint8_t>

//=========================================================
// Exceptions
//=========================================================
    /*
        Ex_StringException
            Base exception type for this library!
    */
    class Ex_StringException : public std::exception
    { 
    public: 
        Ex_StringException( const char * text = "Unknown Exception" )
            :m_what(text)
        {
        }

        virtual ~Ex_StringException()throw() {}

        virtual const char * what()const throw()
        {
            return m_what.c_str();
        }

    protected:
        std::string m_what;
    };

    /*
        Exception for failed IO
    */
    //class Ex_IOException : public Ex_StringException
    //{
    //public:
    //    Ex_IOException( const char * target, const char * text = "Unknown Exception" )
    //        :Ex_StringException(text), m_target(target)
    //    {}

    //    Ex_IOException( const std::string & target, const std::string & text = "Unknown Exception" )
    //        :Ex_StringException(text), m_target(target)
    //    {}

    //    virtual const char * what()const throw()
    //    {
    //        std::stringstream strs;
    //        strs << "IO Exception when accessing " <<m_target <<"! " <<m_what;
    //        return  strs.str().c_str();
    //    }

    //    virtual const char * target()const throw()
    //    {
    //        return m_target.c_str();
    //    }

    //private:
    //    std::string m_target;
    //};

};};

#endif