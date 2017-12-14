#ifndef WHERE_AM_I_WRAPPER_HPP
#define WHERE_AM_I_WRAPPER_HPP
/*
whereami_wrapper.hpp
psycommando@gmail.com
28/07/2016
Description: A simple layer in-between the c functions of whereami and std lib strings.
*/
#include <string>

namespace utils
{
    /*
        GetPathExeDirectory
            Returns the path to the directory containing the executable
    */
    std::string GetPathExeDirectory();

    /*
        GetPathExe
            Returns the path to the executable
    */
    std::string GetPathExe();
};

#endif // !WHERE_AM_I_WRAPPER_HPP

