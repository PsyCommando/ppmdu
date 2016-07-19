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
#include <regex>
using namespace std;

namespace utils 
{
	string GetPathOnly( const string & fullpath, const std::string & separator )
	{
		return fullpath.substr( 0, fullpath.find_last_of(separator) );
	}

	string GetFilenameFromPath( const string & fullpath, const std::string & separator )
	{
        string::size_type poslastslash = fullpath.find_last_of(separator);
        return fullpath.substr( poslastslash + 1, fullpath.size() - poslastslash  );
	}

	string GetPathWithoutFileExt( const string & fullpath )
	{
		return fullpath.substr( 0, fullpath.find_last_of(".") );
	}

    std::vector<std::string> GetPathComponents(const std::string & path, const std::string & separator )
    {
        std::vector<std::string> target;
        for( size_t cnt = 0; cnt < path.size(); ++cnt )
        {
            string::size_type found = path.find_first_of(separator, cnt);
            if( found != string::npos )
            {
                target.push_back( std::move(path.substr(cnt,(found - cnt))) );
                cnt = found;
            }
            else
            {
                target.push_back( std::move(path.substr(cnt)) );
                break;
            }
        }
        return std::move(target);
    }

    std::string AssemblePath(std::vector<std::string>& components, char separator)
    {
        stringstream sstr;
        for( size_t cntcmp = 0; cntcmp < components.size(); ++cntcmp )
        {
            if( cntcmp != 0 )
                sstr<<separator;
            sstr<<components[cntcmp];
        }
        return std::move(sstr.str());
    }

    /*
        Appends a trailing slash to a path if its not there in the first place !
        Move constructor makes this fast !
    */
    std::string TryAppendSlash( const std::string &path )
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
            Remove unprintable characters from a filename.
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