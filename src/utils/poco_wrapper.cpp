#include "poco_wrapper.hpp"
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
#include <cassert>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
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
            if( outfolder.isFile() ) //This is really bad!
                throw runtime_error("A file with the same name as the directory to create exists!:" + path);
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

    bool pathIsAbsolute(const std::string & inputpath)
    {
        return Poco::Path(inputpath).isAbsolute();
    }

    bool pathIsRelative(const std::string & inputpath)
    {
        return Poco::Path(inputpath).isRelative();
    }

    std::string getCWD()
    {
        return Poco::Path::current();
    }

    //Read entire directory content to string vector
    std::vector<std::string> ListDirContent_FilesAndDirs( const std::string & dirpath, bool bFilenameOnly, bool noslashaftdir )
    {
        vector<string>          dircontent;
        Poco::DirectoryIterator itdir(dirpath);
        Poco::DirectoryIterator itdirend;

        while( itdir != itdirend )
        {
            if( bFilenameOnly )
            {
                if( itdir->isFile() )
                    dircontent.push_back( Poco::Path::transcode(Poco::Path( itdir->path() ).getFileName()) );
                else if( itdir->isDirectory() )
                {
                    if( noslashaftdir )
                        dircontent.push_back( Poco::Path::transcode(Poco::Path( itdir->path() ).makeFile().getFileName()) );
                    else
                        dircontent.push_back( Poco::Path::transcode(Poco::Path( itdir->path() ).makeFile().getFileName()) + "/" );
                }
            }
            else
            {
                dircontent.push_back( Poco::Path::transcode(itdir->path()) );
            }

            ++itdir;
        }

        return std::move( dircontent );
    }

    /*
        GetBaseNameOnly
            The basename is the the last dir in the path, or the name of the file without extension.
    */
    string GetBaseNameOnly( const string & path )
    {
        return std::move(Poco::Path::transcode(Poco::Path(path).getBaseName()));
    }

    /*
        GetFileExtension
            Return the file extension only.
    */
    string GetFileExtension( const string & path )
    {
        return std::move(Poco::Path::transcode(Poco::Path(path).getExtension()));
    }

    /*
        GetFilename
            Return the file name and extension only.
    */
    string GetFilename( const string & path )
    {
        return std::move(Poco::Path::transcode(Poco::Path(path).getFileName()));
    }

    /*
    */
    std::string MakeAbsolutePath(const std::string & relp, const std::string & absbasep)
    {
        Poco::Path relative(relp);
        if( relative.isAbsolute() )
            return relp;
        return std::move(relative.makeAbsolute(absbasep).toString());
    }
    std::string MakeAbsolutePath(const std::string & relp)
    {
        Poco::Path relative(relp);
        if( relative.isAbsolute() )
            return relp;
        return std::move(relative.makeAbsolute().toString());
    }
};