#include "pmd2_langconf.hpp"

#include <utils/poco_wrapper.hpp>
#include <ppmdu/fmts/waza_p.hpp>
#include <ppmdu/fmts/monster_data.hpp>
#include <ppmdu/fmts/m_level.hpp>
#include <ppmdu/fmts/text_str.hpp>
#include <ppmdu/fmts/item_p.hpp>
#include <pugixml.hpp>
#include <utils/parse_utils.hpp>
#include <utils/library_wide.hpp>
#include <Poco/Path.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cassert>
#include <map>

#include <codecvt>

using namespace std;
using namespace pmd2::filetypes;
using namespace filetypes;

namespace pmd2
{
//==========================================================================================
//  GameLanguageLoader
//==========================================================================================
    namespace GameLangXMLStr
    {
        static const std::string ROOT_StrIndexes = "StringIndexData";

        static const std::string NODE_GameDef    = "Game";
        static const std::string NODE_Language   = "Language";
        static const std::string NODE_StrBlock   = "StringBlock";

        static const std::string PROP_Name       = "Name";
        static const std::string PROP_BegIndex   = "Begin";
        static const std::string PROP_EndIndex   = "End";
        
        static const std::string ATTR_Name       = "name";
        static const std::string ATTR_StrFileName= "strfile";
        static const std::string ATTR_LocaleStr  = "locale";
    };

    static const string DefMoveData1ExportDir = "move_data1";
    static const string DefMoveData2ExportDir = "move_data2";

    //const array<string, static_cast<uint32_t>(eStringBlocks::NBEntries)> StrBlocksNames = 
    //{
    //    "Pokemon Names",
    //    "Pokemon Categories",

    //    "Move Names",
    //    "Move Descriptions",

    //    "Item Names",
    //    "Item Short Descriptions",
    //    "Item Long Descriptions",

    //    "Ability Names",
    //    "Ability Descriptions",

    //    "Type Names",

    //    "Portrait Names",
    //};

#if 0
    /************************************************************************************
        GameLangXMLParser
            Parse the gamelang.xml file that contains the proper offsets for all strings
            for the supported game languages and versions!
    ************************************************************************************/
    class GameLangXMLParser
    {
        typedef GameLanguageLoader::blockoffs_t strblock_t;
    public:
        GameLangXMLParser(GameLanguageLoader& target)
            :m_target(target)
        {}

        void Parse(const std::string & confFilePath )
        {
            using namespace pugi;
            using namespace GameLangXMLStr;
            std::ifstream ingamelang( confFilePath );
            xml_document  langconf;

            if( ingamelang.bad() || !ingamelang.is_open() )
            {
                stringstream sstr;
                sstr << "Error: The XML file with info on the game's string data \""
                     <<confFilePath << "\" cannot be opened for some reasons..";
                string errorstr = sstr.str();
                clog << errorstr <<"\n";
                throw std::runtime_error( errorstr );
            }

            if( ! langconf.load(ingamelang) )
                throw std::runtime_error("Failed to create xml_document for parsing game language config file!");

            //iterate through all game definitions
            for( auto & gamedef : langconf.child(ROOT_StrIndexes.c_str()).children( NODE_GameDef.c_str() ) )
            {
                try
                {
                    if( !gamedef.empty() )
                        ParseGameDef(gamedef);
                    else
                        clog << "An empty \"" <<NODE_GameDef <<"\" node was ignored!\n";
                }
                catch( exception & )
                {
                    clog << "An invalid \"" <<NODE_GameDef <<"\" node was ignored!\n";
                }
            }
        }

    private:
        void ParseGameDef(const pugi::xml_node & gamedef)
        {
            using namespace pugi;
            using namespace GameLangXMLStr;
            std::string gameName;

            //Get the name
            if( gamedef.attributes_begin() != gamedef.attributes_end() )
            {
                xml_attribute gamenameattr = gamedef.attribute(ATTR_Name.c_str());
                if( !gamenameattr.empty() )
                {
                    gameName = gamenameattr.as_string();

                    if( gameName != GameVersionNames[ static_cast<size_t>(m_target.m_gameVersion)] )
                        return; //Don't parse what we don't need
                }
                else
                {
                    stringstream sstr;
                    sstr <<"WARNING: Missing \"name\" attribute for \"Game\" node in the XML data!";
                    string strerr = sstr.str();
                    clog << strerr <<"\n";
                    throw runtime_error(strerr);
                }
            }
            else
            {
                stringstream sstr;
                sstr <<"WARNING: Missing \"name\" attribute for \"Game\" node in the XML data!";
                string strerr = sstr.str();
                clog << strerr <<"\n";
                throw runtime_error(strerr);
            }

            for( auto & gamelang : gamedef.children() )
            {
                try
                {
                    if( ! gamelang.empty() )
                       m_target.m_langData.push_back( ParseLanguage( gamelang, gameName ) ); 
                    else
                        clog << "Empty \"" <<NODE_Language <<"\" node was ignored!\n";
                }
                catch( exception & )
                {
                    clog << "An invalid \"" <<NODE_Language <<"\" node was ignored!\n";
                }
            }
        }

        GameLanguageLoader::glang_t ParseLanguage(const pugi::xml_node & lang, const std::string & gamename )
        {
            using namespace pugi;
            using namespace GameLangXMLStr;
            GameLanguageLoader::glang_t gamelang;

            //Get the attributes
            if( lang.attributes_begin() != lang.attributes_end() )
            {
                xml_attribute langnameattr  = lang.attribute(ATTR_Name.c_str());
                xml_attribute strfnameattr  = lang.attribute(ATTR_StrFileName.c_str());
                xml_attribute localestrattr = lang.attribute(ATTR_LocaleStr.c_str());
                bool          langok        = !langnameattr.empty();
                bool          strnameok     = !strfnameattr.empty();
                bool          localeok      = !localestrattr.empty();

                if( langok && strnameok && localeok )
                {
                    gamelang.language     = langnameattr.as_string();
                    gamelang.textStrFName = strfnameattr.as_string();
                    gamelang.localeStr    = localestrattr.as_string();
                }
                else
                {
                    stringstream sstrerr;
                    sstrerr << "ERROR: A \"Language\" node for game \"" <<gamename <<"\" is missing its \"";
                    if( !langok )
                        sstrerr <<"name" << ( !strnameok || !localeok )? ", and " : "";
                    if(!strnameok)
                        sstrerr <<"strfile" << ( !localeok )? ", and " : "";
                    if(!localeok)
                        sstrerr <<"locale";
                    sstrerr << "\" attribute(s)! Skipping..";
                    string strerr = sstrerr.str();
                    clog << strerr <<"\n";
                    throw runtime_error(strerr);
                }
            }
            else
            {
                stringstream sstrerr;
                sstrerr << "ERROR: A \"Language\" node for game \"" <<gamename <<"\" is missing all of its attributes! Skipping..";
                string strerr = sstrerr.str();
                clog << strerr <<"\n";
                throw runtime_error(strerr);
            }

            //Then parse the indexes
            for( auto & strblock : lang.children() )
            {
                if( ! strblock.empty() )
                    gamelang.strBlockOffsets.insert( ParseStrBlock( strblock ) );
                else
                    clog << "Empty \"" <<NODE_StrBlock <<"\" node was ignored!\n";
            }

            return std::move(gamelang);
        }

        strblock_t ParseStrBlock( const pugi::xml_node & strblock )
        {
            using namespace pugi;
            using namespace GameLangXMLStr;
            string   name;
            uint32_t begindex = 0;
            uint32_t endindex = 0;

            for( auto & ablock : strblock )
            {
                if( ablock.name() == PROP_Name )
                    name = ablock.child_value();
                else if( ablock.name() == PROP_BegIndex )
                    begindex = utils::parseHexaValToValue<uint32_t>( ablock.child_value() );
                else if( ablock.name() == PROP_EndIndex )
                    endindex = utils::parseHexaValToValue<uint32_t>( ablock.child_value() );
            }

            return make_pair(name, make_pair(begindex, endindex) );
        }

        GameLanguageLoader& m_target;
    };

    GameLanguageLoader::GameLanguageLoader()
        :m_gameVersion(eGameVersion::Invalid)
    {}

    GameLanguageLoader::GameLanguageLoader( const std::string & textFileName, eGameVersion version )
        :m_gameVersion(version)
    {
        LoadFile(textFileName);
    }

    void GameLanguageLoader::LoadFile( const std::string & textFileName)
    {
        GameLangXMLParser(*this).Parse(textFileName);
    }

    /*
        Using the name of the text_*.str file, this will return the corresponding
        locale string parsed!
    */
    std::string GameLanguageLoader::FindLocaleString( const std::string & textFileName )const
    {
        for( const auto & entry : m_langData )
        {
            if( entry.textStrFName == textFileName )
                return entry.localeStr;
        }
        return string(); //return empty if not found
    }

    /*
        Return the language associated with a given text_*.str file.
    */
    std::string GameLanguageLoader::FindLanguage( const std::string & textFileName )const
    {
        for( const auto & entry : m_langData )
        {
            if( entry.textStrFName == textFileName )
                return entry.language;
        }
        return string(); //return empty if not found
    }

    /*
        Return the name of the text_*.str file associated with the specified language.
    */
    std::string GameLanguageLoader::FindTextStrFName( const std::string & language )const
    {
        for( const auto & entry : m_langData )
        {
            if( entry.language == language )
                return entry.textStrFName;
        }
        return string(); //return empty if not found
    }

    /*
    */
    StringsCatalog GameLanguageLoader::FindAllBlockOffsets(const std::string & textFileName )const
    {
        StringsCatalog cata;
        for( const auto & entry : m_langData )
        {
            if( entry.textStrFName == textFileName )
            {
                cata.SetLocaleString(entry.localeStr);
                for( size_t cntblk = 0; cntblk < StrBlocksNames.size(); ++cntblk )
                {
                    auto found = entry.strBlockOffsets.find( StrBlocksNames[cntblk] );
                    if( found != entry.strBlockOffsets.end() )
                        cata.AddStringBlock( static_cast<eStringBlocks>(cntblk), found->second.first, found->second.second );
                }
            }
        }
        return std::move(cata);
    }


    /*
        For a given string block name inside the text_*.str file, returns whether the 
        block was found, and the index of the first string in that block. The result
        is put into a pair.
    */
    std::pair<bool,std::pair<uint32_t,uint32_t>> GameLanguageLoader::FindStrBlockOffset( const std::string & blockName, const std::string & textFileName )const
    {
        for( const auto & entry : m_langData )
        {
            if( entry.textStrFName == textFileName )
            {
                auto found = entry.strBlockOffsets.find( blockName );
                if( found != entry.strBlockOffsets.end() )
                {
                    return make_pair( true, found->second );
                }
            }
        }
        return make_pair(false,make_pair(uint32_t(0),uint32_t(0)));
    }


    const GameLanguageLoader::glang_t * GameLanguageLoader::GetLanguage(const std::string & langname)const
    {
        for( const auto & entry : m_langData )
        {
            if( entry.language == langname )
                return &entry;
        }
        return nullptr;
    }
#endif
};