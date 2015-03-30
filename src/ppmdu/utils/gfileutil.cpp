/*
gfileutils.cpp
18/05/2014
psycommando@gmail.com

Description:
File with some handy tools for working with files.
Is as portable and simple as possible.

No crappyrights. All wrongs reversed!
*/
#include "gfileutils.hpp"
#include "gstringutils.hpp"
#include <sstream>
#include <iostream>
#include <string>
#include <locale>
using namespace std;

namespace utils 
{
	string GetPathOnly( const string & fullpath )
	{
		return fullpath.substr( 0, fullpath.find_last_of("\\/") );
	}

	string GetFilenameFromPath( const string & fullpath )
	{
        string::size_type poslastslash = fullpath.find_last_of("\\/");
        return fullpath.substr( poslastslash + 1, fullpath.size() - poslastslash  );
	}

	string GetPathWithoutFileExt( const string & fullpath )
	{
		return fullpath.substr( 0, fullpath.find_last_of(".") );
	}

    /*
        Appends a trailing slash to a path if its not there in the first place !
        Move constructor makes this fast !
    */
    std::string AppendTraillingSlashIfNotThere( const std::string &path )
    {
        if( !has_suffix( path, "/")  && !has_suffix( path, "\\" ) )
        {
            return path + "/";  //Put a slash if there isn't one already
        }
        else
            return path;
    }

    std::string RemEscapedDblQuoteFromPath( const std::string &path, bool printwarning )
    {
        //Handle evil backslash escaping the double quote
        if( has_suffix( path, "\"" ) )
        {
            if( printwarning )
            {
                cout << "<!>- Detected invalid trailing double-quote character in path:\n"
                     << path <<"\n"
                     << "     I'll fix the path for you, but don't put a trailling backslash\n"
                     << "     right before a double quote next time, its dangerous!\n"
                     << "     Use slashes instead!\n\n";
            }
            return path.substr( 0, path.size()-1 );
        }
        return path;
    }

    /*
        CleanFilename
            Remove special invalid characters from a filename.
    */
    std::string CleanFilename( const std::string & fname, std::locale loc )
    {
        using namespace std;
        stringstream outsstr;
        for( char c : fname )
        {
            if( isprint( c, loc ) /*&& ! isgraph( c, loc )*/ && ! isspace( c, loc ) )
            {
                //Skip any punctuation but dots
                if( ispunct(c,loc) && c != '.' )
                    outsstr << '_';
                else
                    outsstr << c;
            }
            else
                outsstr << '_';
        }
        return outsstr.str();
    }

};