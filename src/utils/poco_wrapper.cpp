#include "poco_wrapper.hpp"
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/Util/Application.h>
#include <Poco/Util/OptionSet.h>
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

    std::string GetPathOnly(const std::string & path)
    {
        return std::move( Poco::Path::transcode(Poco::Path(path).makeParent().toString()) );
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

//#ifndef POCO_STATIC
//    /*
//        StubApp
//            Minimal application class with only use
//            determining the application path.
//
//            Its hardly ideal, but I can't be bothered to 
//            write my own stuff right before a 
//            possible switch/rewrite. Especially
//            for something that should be called once
//            per execution..
//    */
//    template<typename _CHARTY>
//    class StubApp : public Poco::Util::Application
//    {
//    public:
//        StubApp( int argc, _CHARTY * argv[] )
//            :Poco::Util::Application(argc,argv)
//        {}
//
//        auto GetAppPath()const->decltype(config().getString("application.dir"))
//        {
//            return config().getString("application.dir");
//        }
//
//        void initialize( Poco::Util::Application & self )
//        {
//            Poco::Util::Application::initialize(self);
//        }
//
//        void reinitialize( Poco::Util::Application & self )
//        {
//            Poco::Util::Application::reinitialize(self);
//        }
//
//        void uninitialize()
//        {
//            Poco::Util::Application::uninitialize();
//        }
//
//        void defineOptions( Poco::Util::OptionSet & options )
//        {
//            Poco::Util::Application::defineOptions(options);
//        }
//
//        void handleOption( const std::string & name, const std::string & value )
//        {
//            Poco::Util::Application::handleOption(name,value);
//        }
//
//        int main( const std::vector<std::string> & args )
//        {
//            return Poco::Util::Application::ExitCode::EXIT_OK;
//        }
//    };
//#endif
//
//    std::string GetApplicationDirectory(int argc, char * argv[])
//    {
//#ifndef POCO_STATIC
//        return StubApp<char>(argc, argv).GetAppPath();
//#else
//        //Well, it seems the Poco::Application class can only work with a dynamic library for some dubious reasons, 
//        // so that 4 messages can be parser from binary??
//
//        //So we'll do it the old-fashioned way, and hope the first parameter is the path to the executable....
//#ifdef WIN32
//
//#else
//        static_assert(false, "GetApplicationDirectory(): Sorry, you'll need to implement something to get the application path for your platform. I give up at this point..");
//#endif // WIN32
//
//#endif
//    }



};