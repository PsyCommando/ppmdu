#ifndef UUID_GEN_WRAPPER_HPP
#define UUID_GEN_WRAPPER_HPP
/*
uuid_gen_wrapper.hpp
2015/09/21
psycommando@gmail.com
Description: A wrapper for the UUID generator. 
*/

namespace utils
{
    typedef int puuid_t;
    puuid_t GenerateUUID();
};

#endif