#include "poco_wrapper.hpp"
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
#include <cassert>
#include <iostream>
using namespace std;

namespace utils
{
//===============================================================================
//Files
//===============================================================================
    /*
        Creates a new subdirectory at the path specified
    */
    bool DoCreateDirectory( const std::string & path )
    {
        Poco::File outfolder(path); 

        if( outfolder.exists() )
        {
            if( outfolder.isFile() )
            {
                assert(false);
                cerr << "<!>-Error: A file with the same name as the directory to create exists!!\n";
                return false;
            }
            else
                return true;
        }
        else
            return outfolder.createDirectory();
    }

    //From the first path specified in the arguments its possible to detect whether we must unpack or pack a file.
    bool isFolder( const string & inputpath )
    {
        Poco::File test(inputpath);
        return test.exists() && test.isDirectory();
    }

    bool isFile( const string & inputpath )
    {
        Poco::File test(inputpath);
        return test.exists() && test.isFile();
    }

    bool pathExists( const std::string & inputpath )
    {
        return Poco::File(inputpath).exists();
    }
};