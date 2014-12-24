#ifndef POCO_WRAPPER_HPP
#define POCO_WRAPPER_HPP
/*
poco_wrapper.hpp
2014/12/21
psycommando@gmail.com
Description: A wrapper to avoid including POCO for doing some common file operations.
*/
#include <string>

namespace utils
{
    // ---------------------------------------------------------------
    // File related stuff 
    // ---------------------------------------------------------------

    /************************************************************************
        DoCreateDirectory
            Creates a new subdirectory at the path specified
    ************************************************************************/
    bool DoCreateDirectory( const std::string & path );

    /************************************************************************
        isFolder
            Returns true if the path specified is a directory and exists.
    ************************************************************************/
    bool isFolder( const std::string & inputpath );

    /************************************************************************
        isFile
            Returns true if the path specified is a file and exists.
    ************************************************************************/
    bool isFile( const std::string & inputpath );

    /************************************************************************
        pathExists
            Returns true if the path specified is valid and is either a file 
            or directory.
    ************************************************************************/
    bool pathExists( const std::string & inputpath );
};

#endif