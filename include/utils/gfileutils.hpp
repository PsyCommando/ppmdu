#ifndef G_FILE_UTILS_HPP
#define G_FILE_UTILS_HPP
/*
gfileutils.hpp
18/05/2014
psycommando@gmail.com

Description:
Header file with some handy tools for working with files.

No crappyrights. All wrongs reversed!
*/
#include <string>
#include <vector>
#include <locale>

namespace utils 
{
    const std::string DefaultPathSeparators = "\\/";

    //Take a path to a file, and return the path without the filename+extension, or else the exact same path
	//std::string GetPathOnly( const std::string & fullpath, const std::string & separator = DefaultPathSeparators );

    //Take a path, and return only the filename+extension if there is one! Else returns empty string.
    std::string GetFilenameFromPath( const std::string & fullpath, const std::string & separator = DefaultPathSeparators );

    //Returns the path without the file extension if there is one, or else the exact same path
	std::string GetPathWithoutFileExt( const std::string & fullpath );

    /************************************************************************
        GetPathComponents
            Cuts a path into all its parts for example:
            "c:/users/bob/my documents/some file.txt"
            would end up as :
            "c:", "users", "bob", "my documents", "some file.txt"
    ************************************************************************/
    std::vector<std::string> GetPathComponents( const std::string & path, const std::string & separator = DefaultPathSeparators );

    /*
        AssemblePath
            Assemble a path from a vector of strings.
    */
    std::string AssemblePath( std::vector<std::string> & components, char separator = '/' );

    /************************************************************************
        TryAppendSlash
            Appends a trailing slash to a path if its not there in the first 
            place ! Move constructor makes this fast !
    ************************************************************************/
    std::string TryAppendSlash( const std::string &path );

    /************************************************************************
        RemEscapedDblQuoteFromPath
            Remove the escaped \" from the end of the path, if there is one!
            This is a common bug on windows, given it uses backslashes as
            path separators instead of the harmless slashes.

            This happens when someone encase a path between double quote, 
            like they should, but also ends the path with a backslash..

            - printwarning : if is true, will print a short message to cout
              to mention the issue with the path passed to the function !
    ************************************************************************/
    std::string RemEscapedDblQuoteFromPath( const std::string &path, bool printwarning = false );

    /************************************************************************
        CleanFilename
            Remove unprintable characters from a filename.
    ************************************************************************/
    std::string CleanFilename( const std::string & fname, std::locale loc = std::locale::classic() );

};

#endif