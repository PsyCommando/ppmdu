#include "game_strings.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <pugixml.hpp>
#include <utils/pugixml_utils.hpp>
#include <utils/parse_utils.hpp>
#include <utils/library_wide.hpp>
#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
using namespace std;
using namespace pugi;
using namespace pugixmlutils;



namespace pmd3
{
//=================================================================================
//  Constant
//=================================================================================
    namespace strpoolxml
    {
    };


//============================================================================================
//  GameStringPool_XML_Parser
//============================================================================================

    /*
        GameStringPool_XML_Parser
            Disposable parsing context.
            Basically encapsulate variables that several functions would need to share to parse the file 
            within a single-use class instead.
            It avoids filling the main objects with useless IO junk methods and etc..
    */
    class GameStringPool_XML_Parser
    {
    public:
        GameStringPool_XML_Parser( const string & dirpath )
            :m_dirpath(dirpath)
        {}

        inline operator GameStringsPool()
        {
            return move(Parse());
        }

        GameStringsPool Parse()
        {
            using namespace Poco;
            Poco::File      indir(m_dirpath);

            if( indir.exists() && indir.isDirectory() && DirectoryIterator(indir) != DirectoryIterator()  )
            {
                _Parse();
            }
            else    
            {
                stringstream sstr;
                sstr <<"GameStringPool_XML_Parser::Parse(): Specified path \"" <<m_dirpath <<"\" doesn't exists, is not a directory, or is empty!";
                throw runtime_error( sstr.str() );
            }

            return move(m_output);
        }

    private:

        void _Parse()
        {

        }

    private:
        GameStringPool_XML_Parser( const GameStringPool_XML_Parser& )             = delete;
        GameStringPool_XML_Parser & operator=( const GameStringPool_XML_Parser& ) = delete;

        string          m_dirpath;
        GameStringsPool m_output;
    };

//============================================================================================
//  GameStringPool_XML_Writer
//============================================================================================
    class GameStringPool_XML_Writer
    {
    public:

        GameStringPool_XML_Writer( const GameStringsPool & strpool )
            :m_gstrpool(strpool)
        {
        }

        void Write( const string & filepath )
        {
            using namespace Poco;
            Poco::File outdir(filepath);

            //Make directory if neccessary
            if( !outdir.exists() )
            {
                if( utils::LibWide().isLogOn() )
                    clog << "<*>- Creating directory \"" <<filepath <<"\"!\n";

                if( !outdir.createDirectory() )
                {
                    stringstream sstr;
                    sstr <<  "GameStringPool_XML_Writer::Write() :Couldn't create output directory \"" + filepath + "\"!";
                    throw runtime_error( sstr.str() );
                }

            }
            else if ( !outdir.isDirectory() )
                throw runtime_error( "GameStringPool_XML_Writer::Write() : Output path exists but is not a directory!" );

            //Begin with writing

        }

    private:

        void _Write()
        {
        }

    private:
        const GameStringsPool & m_gstrpool;
    };

//============================================================================================
//  GameStringsPool
//============================================================================================
    //Export
    void GameStringsPool::ExportXML ( const std::string & filepath )
    {
        GameStringPool_XML_Writer(*this).Write( filepath );
    }

    //Import
    void GameStringsPool::ImportXML ( const std::string & filepath )
    {
        (*this) = move( GameStringPool_XML_Parser(filepath).Parse() );
    }
};