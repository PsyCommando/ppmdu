#include "game_stats.hpp"
#include <ppmdu/utils/poco_wrapper.hpp>
#include <ppmdu/fmts/waza_p.hpp>
#include <ppmdu/fmts/monster_data.hpp>
#include <ppmdu/fmts/m_level.hpp>
#include <ppmdu/fmts/text_str.hpp>
#include <ppmdu/fmts/item_p.hpp>
#include <ppmdu/utils/config_io.hpp>
#include <pugixml.hpp>
#include <ppmdu/utils/parse_utils.hpp>
#include <ppmdu/utils/library_wide.hpp>
#include <Poco/Path.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cassert>
#include <map>
using namespace std;

namespace pmd2{ namespace stats
{
//==========================================================================================
//
//==========================================================================================
    static const string BalanceDirectory  = "BALANCE";
    static const string GameTextDirectory = "MESSAGE";

    std::array<std::string, static_cast<size_t>(eGameVersion::NBGameVers)> GameVersionNames =
    {
        "",
        "EoS",
        "EoTD",
    };



    /*
        Lookup table for finding what language the game is in.
        - The first value is the name of the game string file for the specific language.
        - The second value is the enum value for the language.
    */
    //static const std::vector<std::pair<std::string, CGameStats::eGameLanguage>> TextStr_Lookup =
    //{
    //    { filetypes::TextStr_EngFName, CGameStats::eGameLanguage::English  },
    //    { filetypes::TextStr_JapFName, CGameStats::eGameLanguage::Japanese },
    //};

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

        //static const string PokeNamesBlockName    = "Pokemon Names";
        //static const string PokeCatBlockName      = "Pokemon Categories";

        //static const string MoveNamesBlockName    = "Move Names";
        //static const string MoveDescBlockName     = "Move Descriptions";

        //static const string ItemNamesBlockName    = "Item Names";
        //static const string ItemSDescBlockName    = "Item Short Descriptions";
        //static const string ItemLDescBlockName    = "Item Long Descriptions";

        //static const string AbilityNamesBlockName = "Ability Names";
        //static const string AbilityDescBlockName  = "Ability Descriptions";

        //static const string TypeNamesBlockName    = "Type Names";
    };

    static const array<string, static_cast<uint32_t>(CGameStats::eStrBNames::NBEntries)> StrBlocksNames = 
    {
        "Pokemon Names",
        "Pokemon Categories",

        "Move Names",
        "Move Descriptions",

        "Item Names",
        "Item Short Descriptions",
        "Item Long Descriptions",

        "Ability Names",
        "Ability Descriptions",

        "Type Names",
    };


    class GameLangXMLParser
    {
        typedef GameLanguageLoader::blockoffs_t strblock_t;
    public:
        GameLangXMLParser(GameLanguageLoader& target)
            :m_target(target)
        {
        }

        void Parse(const std::string & confFilePath )
        {
            using namespace pugi;
            using namespace GameLangXMLStr;
            std::ifstream ingamelang( confFilePath );
            xml_document  langconf;

            if( ingamelang.bad() )
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

            /*
                iterate through all game definitions
            */
            for( auto & gamedef : langconf.child(ROOT_StrIndexes.c_str()).children( NODE_GameDef.c_str() ) )
            {
                try
                {
                    if( !gamedef.empty() )
                        ParseGameDef(gamedef);
                    else
                        cerr << "An empty \"" <<NODE_GameDef <<"\" node was ignored!\n";
                }
                catch( exception & )
                {
                    cerr << "An invalid \"" <<NODE_GameDef <<"\" node was ignored!\n";
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
                        cerr << "Empty \"" <<NODE_Language <<"\" node was ignored!\n";
                }
                catch( exception & )
                {
                    cerr << "An invalid \"" <<NODE_Language <<"\" node was ignored!\n";
                }
            }
        }

        GameLanguageLoader::glang_t ParseLanguage(const pugi::xml_node & lang, const std::string & gamename )
        {
            using namespace pugi;
            using namespace GameLangXMLStr;
            //std::string langname;
            //std::string strfilename;
            //std::string localestr;
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
                    cerr << "Empty \"" <<NODE_StrBlock <<"\" node was ignored!\n";
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


//==========================================================================================
//  CGameStats
//==========================================================================================
    //CGameStats::CGameStats( const std::string & pmd2rootdir, GameLanguageLoader && langList )
    //    :m_dataFolder(pmd2rootdir), m_gameVersion(eGameVersion::Invalid)/*, m_possibleLang(langList)*/
    //{
    //}

    CGameStats::CGameStats( const std::string & pmd2rootdir, const std::string & gamelangfile )
        :m_dataFolder(pmd2rootdir), m_gameVersion(eGameVersion::Invalid), m_gamelangfile(gamelangfile)
    {
    }

    void CGameStats::IdentifyGameVersion()
    {
        //To identify whether its Explorers of Sky or Explorers of Time/Darkness
        // check if we have a waza_p2.bin file under the /BALANCE/ directory.
        stringstream sstr;
        sstr << utils::AppendTraillingSlashIfNotThere(m_dataFolder) << BalanceDirectory;

        if( utils::isFolder( sstr.str() ) )
        {
            sstr << "/" << filetypes::WAZA2_Fname;
            if( utils::isFile( sstr.str() ) )
                m_gameVersion = eGameVersion::EoS;
            else
                m_gameVersion = eGameVersion::EoTEoD;
        }
        else
            m_gameVersion = eGameVersion::Invalid;

        if( utils::LibWide().isLogOn() )
        {
            clog << "Detected game version : " 
                 << ( (m_gameVersion == eGameVersion::EoS )? "EoS" : ( m_gameVersion == eGameVersion::EoTEoD )? "EoT/D" : "Invalid" )
                 << "\n";
        }
    }

    void CGameStats::IdentifyGameLocaleStr()
    {
        /*
            Load game language file
        */
        m_possibleLang = GameLanguageLoader( m_gamelangfile, m_gameVersion );

        /*
            To identify the game language, find the text_*.str file, and compare the name to the lookup table.
        */
        static const string TextStr_FnameBeg = "text_";
        stringstream sstr;
        sstr << utils::AppendTraillingSlashIfNotThere(m_dataFolder) << GameTextDirectory;
        const string MessageDirPath =  sstr.str();

        if( utils::isFolder( MessageDirPath ) )
        {
            const vector<string> filelist = utils::ListDirContent_FilesAndDirs( MessageDirPath, true );
            for( const auto & fname : filelist )
            {
                //if the begining of the text_*.str filename matches, do a comparison, and return the result
                if( fname.size() > TextStr_FnameBeg.size() &&
                   fname.substr( 0, (TextStr_FnameBeg.size()) ) == TextStr_FnameBeg )
                {
                    m_gameTextFName  = fname;
                    m_gameLangLocale = m_possibleLang.FindLocaleString( fname );

                    if( utils::LibWide().isLogOn() )
                        clog << "Got locale string : \"" <<m_gameLangLocale <<"\" for text file \"" <<m_gameTextFName <<"\" !\n";
                    return;
                }
            }
        }
 
        //We end up here if we have no data on that file name, or if the file was not found at all!!

        m_gameLangLocale = ""; //Use default locale if nothing match

        if( utils::LibWide().isLogOn() )
            clog << "WARNING: No data found for game text file : \"" <<m_gameTextFName <<"\" ! Falling back to default locale!\n";
    }

    void CGameStats::BuildListOfStringOffsets()
    {
        using namespace GameLangXMLStr;
        m_strOffsets.resize(StrBlocksNames.size());
        bool         bencounteredError = false;
        stringstream sstr;

        for( unsigned int i = 0; i < StrBlocksNames.size(); ++i )
        {
            auto result = m_possibleLang.FindStrBlockOffset( StrBlocksNames[i], m_gameTextFName );
            if( result.first )
            {
                m_strOffsets[i].beg = result.second.first;
                m_strOffsets[i].end = result.second.second;
            }
            else
            {
                sstr <<"ERROR: The beginning or the end of the " <<StrBlocksNames[i] 
                     <<" string block(s) was not found in the game string location configuration file!\n";
                bencounteredError = true;
            }
        }

        //Print all the missing entries
        if(bencounteredError)
        {
            string strerr = sstr.str();
            clog << strerr <<"\n";
            throw runtime_error( strerr );
        }
    }

    void CGameStats::Load()
    {
        //First identify what we're dealing with
        IdentifyGameVersion();
        IdentifyGameLocaleStr();
        BuildListOfStringOffsets();

        if( m_gameVersion != eGameVersion::Invalid )
        {
            LoadGameStrings();
            LoadPokemonData();
            //LoadItemData();
            //LoadDungeonData();
        }
        else
            throw std::runtime_error( "ERROR: Couldn't identify the game's version. Some files might be missing..\n" );
    }

    void CGameStats::Load( const std::string & newpmd2rootdir )
    {
        m_dataFolder = newpmd2rootdir;
        Load();
    }

    void CGameStats::LoadStringsOnly()
    {
        IdentifyGameVersion();
        IdentifyGameLocaleStr();
        BuildListOfStringOffsets();
        LoadGameStrings();
    }

    void CGameStats::LoadStringsOnly( const std::string & newpmd2rootdir )
    {
        m_dataFolder = newpmd2rootdir;
        LoadStringsOnly();
    }

    void CGameStats::LoadPkmn()
    {
        IdentifyGameVersion();
        IdentifyGameLocaleStr();
        BuildListOfStringOffsets();

        if( m_gameVersion != eGameVersion::Invalid )
        {
            LoadGameStrings();
            LoadPokemonData();
        }
        else
            throw std::runtime_error( "ERROR: Couldn't identify the game's version. Some files might be missing..\n" );
    }

    void CGameStats::LoadGameStrings()
    {
        stringstream sstr;
        sstr << utils::AppendTraillingSlashIfNotThere(m_dataFolder) << GameTextDirectory << "/" << m_gameTextFName;
        m_gameStrings = filetypes::ParseTextStrFile( sstr.str(), std::locale( m_gameLangLocale ) );
    }

    void CGameStats::LoadPokemonData()
    {
        using namespace filetypes;
        stringstream sstrMd;
        sstrMd << utils::AppendTraillingSlashIfNotThere(m_dataFolder) << BalanceDirectory << "/" << filetypes::MonsterMD_FName;
        stringstream sstrMovedat;
        sstrMovedat << utils::AppendTraillingSlashIfNotThere(m_dataFolder) << BalanceDirectory;
        stringstream sstrGrowth;
        sstrGrowth << utils::AppendTraillingSlashIfNotThere(m_dataFolder) << BalanceDirectory << "/"  << MLevel_FName;

        cout << "Loading Pokemon Data..\n";

        //Load all the move and move set data at the same time first
        cout << "\t<*>- Loading move data and Pokemon movesets..\n";
        auto allmovedat = ParseMoveAndLearnsets(sstrMovedat.str());

        //Build all pokemon entries
        cout << "\t<*>- Building Pokemon database..\n";
        m_pokemonStats = PokemonDB::BuildDB( filetypes::ParsePokemonBaseData(sstrMd.str()),
                                             std::move(allmovedat.second),
                                             filetypes::ParseLevelGrowthData(sstrGrowth.str()) );

        //Set move data
        m_moveData1 = std::move(allmovedat.first.first);
        m_moveData2 = std::move(allmovedat.first.second);
        cout << "Done!\n";
    }

    void CGameStats::LoadItemData()
    {
        assert(false);  //#TODO
        //stringstream sstr;
        //sstr << utils::AppendTraillingSlashIfNotThere(m_dataFolder) << BalanceDirectory;
        //m_itemsData = filetypes::ParseItemsData( sstr.str() );
    }

    void CGameStats::LoadDungeonData()
    {
        assert(false); //Not implemented yet !
    }


    void CGameStats::Write()
    {
        assert(false); //Not implemented yet !
    }

    void CGameStats::WritePkmn( const std::string & rootdatafolder )
    {
        using namespace filetypes;
        //utils::MrChronometer chronopoke("Writing Pokemon");
        stringstream sstrMd;
        sstrMd << utils::AppendTraillingSlashIfNotThere(rootdatafolder) << BalanceDirectory << "/" << filetypes::MonsterMD_FName;
        stringstream sstrMovedat;
        sstrMovedat << utils::AppendTraillingSlashIfNotThere(rootdatafolder) << BalanceDirectory;
        stringstream sstrGrowth;
        sstrGrowth << utils::AppendTraillingSlashIfNotThere(rootdatafolder) << BalanceDirectory << "/"  << MLevel_FName;
        stringstream sstrstrings;
        sstrstrings << utils::AppendTraillingSlashIfNotThere(m_dataFolder) << GameTextDirectory << "/" << m_gameTextFName;

        const string fStatsGrowth = sstrGrowth.str();
        const string fPokeData    = sstrMd.str();
        const string fMoveData    = sstrMovedat.str();
        const string fStrings     = sstrstrings.str();

        cout << "Writing Pokemon Data..\n";

        //Split the pokemon data into its 3 parts
        vector<PokeMonsterData> md;
        pokeMvSets_t            mvset;
        vector<PokeStatsGrowth> sgrowth;
        cout << "\t<*>-Building move, Pokemon data, and level-up data lists..\n";
        m_pokemonStats.ExportComponents( md, mvset, sgrowth );

        //Write stats growth
        cout << "\t<*>-Writing Pokemon stats growth file \"" <<fStatsGrowth <<"\"..\n";
        filetypes::WriteLevelGrowthData( sgrowth, fStatsGrowth );

        //Given the waza file contains both moves and learnsets, we have to load the move data and rewrite it as we modify the pokemon
        // movesets!
        cout << "\t<*>-Loading and writing move data file(s) to directory \"" <<fMoveData <<"\"..\n";
        auto movedat = ParseMoveData(fMoveData);
        filetypes::WriteMoveAndLearnsets( fMoveData, movedat, mvset );

        //Write monster data
        cout << "\t<*>-Writing Pokemon data file \"" <<fPokeData <<"\"..\n";
        filetypes::WritePokemonBaseData( md, fPokeData );

        //Write game strings
        cout << "\t<*>-Writing game strings file \"" <<fStrings <<"\"..\n";
        filetypes::WriteTextStrFile( fStrings, m_gameStrings, std::locale( m_gameLangLocale ) );

        cout << "Done exporting Pokemon data!\n";
    }

    void CGameStats::Write( const std::string & rootdatafolder )
    {
        m_dataFolder = rootdatafolder;
        Write();
    }

    void CGameStats::WriteGameStrings()
    {
        stringstream sstr;
        sstr << utils::AppendTraillingSlashIfNotThere(m_dataFolder) << GameTextDirectory << "/" << m_gameTextFName;
        const string ftxtfile = sstr.str();
        cout << "Writing game strings file \"" <<ftxtfile <<"\"..\n";
        filetypes::WriteTextStrFile( ftxtfile, m_gameStrings, std::locale( m_gameLangLocale ) );
        cout << "Done!\n";
    }
    
    void CGameStats::WritePokemonData()
    {
        assert(false); //Not implemented yet !
    }

    void CGameStats::WriteItemData()
    {
        assert(false); //Not implemented yet !
    }

    void CGameStats::WriteDungeonData()
    {
        assert(false); //Not implemented yet !
    }

    void CGameStats::ExportPkmn( const std::string & directory )
    {
        cout<<"Exporting all Pokemon data to XML data..\n";
        if( m_gameStrings.empty() || m_pokemonStats.empty() )
        {
            throw runtime_error("ERROR: Tried to export an empty list of Pokemon ! Or with an empty string list!");
        }

        cout<<"\t<*>- Writing XML data..\n";
        stats::ExportPokemonsToXML( m_pokemonStats, GetPokemonNameBeg(), GetPokemonCatBeg(), directory );
        cout<<"Done!\n";
    }

    void CGameStats::ImportPkmn( const std::string & directory )
    {
        cout<<"Importing all Pokemon from XML data..\n";
        //Need game strings loaded for this !
        if( m_gameStrings.empty() )
        {
            cout<<"\t<*>- Need to load target game strings file!\nLoading..\n";
            LoadStringsOnly();
        }
        cout<<"\t<*>- Parsing XML data..\n";
        stats::ImportPokemonsFromXML( directory, m_pokemonStats, GetPokemonNameBeg(), GetPokemonNameEnd(), GetPokemonCatBeg(), GetPokemonCatEnd() );
        cout<<"Done!\n";
    }

    /*
        Text Strings Access
            Use those to get the correct string depending on the current game version.
    */

    std::vector<std::string>::const_iterator CGameStats::GetPokemonNameBeg()const
    {
        return const_cast<CGameStats*>(this)->GetPokemonNameBeg();
    }

    std::vector<std::string>::const_iterator CGameStats::GetPokemonNameEnd()const
    {
        return const_cast<CGameStats*>(this)->GetPokemonNameEnd();
    }

    std::vector<std::string>::iterator CGameStats::GetPokemonNameBeg()
    {
        return (m_gameStrings.begin() + strBounds(eStrBNames::PkmnNames).beg );
    }

    std::vector<std::string>::iterator CGameStats::GetPokemonNameEnd()
    {
        return (m_gameStrings.begin() + strBounds(eStrBNames::PkmnNames).end );
    }

    std::vector<std::string>::const_iterator CGameStats::GetPokemonCatBeg()const
    {
        return const_cast<CGameStats*>(this)->GetPokemonCatBeg();
    }
    std::vector<std::string>::const_iterator CGameStats::GetPokemonCatEnd()const
    {
        return const_cast<CGameStats*>(this)->GetPokemonCatEnd();
    }
    std::vector<std::string>::iterator CGameStats::GetPokemonCatBeg()
    {
        return (m_gameStrings.begin() +strBounds(eStrBNames::PkmnCats).beg );
    }
    std::vector<std::string>::iterator CGameStats::GetPokemonCatEnd()
    {
        return (m_gameStrings.begin() +strBounds(eStrBNames::PkmnCats).end );
    }

    std::string & CGameStats::GetPokemonNameStr( uint16_t pkmnindex )
    {
        return m_gameStrings[strBounds(eStrBNames::PkmnNames).beg + pkmnindex];
    }

    std::string & CGameStats::GetPkmnCatNameStr( uint16_t pkmnindex )
    {
        return m_gameStrings[strBounds(eStrBNames::PkmnCats).beg + pkmnindex];
    }

    std::string & CGameStats::GetMoveNameStr( uint16_t moveindex )
    {
        return m_gameStrings[strBounds(eStrBNames::MvNames).beg + moveindex];
    }

    std::string & CGameStats::GetMoveDexcStr( uint16_t moveindex )
    {
        return m_gameStrings[strBounds(eStrBNames::MvDesc).beg + moveindex];
    }

    std::string & CGameStats::GetAbilityNameStr( uint8_t abilityindex )
    {
        return m_gameStrings[strBounds(eStrBNames::AbilityNames).beg + abilityindex];
    }

    std::string & CGameStats::GetAbilityDescStr( uint8_t abilityindex )
    {
        return m_gameStrings[strBounds(eStrBNames::AbilityDesc).beg + abilityindex];
    }

    std::string & CGameStats::GetTypeNameStr( uint8_t type )
    {
        return m_gameStrings[strBounds(eStrBNames::TypeNames).beg + type];
    }

    std::string & CGameStats::GetItemNameStr( uint16_t itemindex )
    {
        return m_gameStrings[strBounds(eStrBNames::ItemNames).beg + itemindex];
    }

    std::string & CGameStats::GetItemSDescStr( uint16_t itemindex )
    {
        return m_gameStrings[strBounds(eStrBNames::ItemDescS).beg + itemindex];
    }

    std::string & CGameStats::GetItemLDescStr( uint16_t itemindex )
    {
        return m_gameStrings[strBounds(eStrBNames::ItemDescL).beg + itemindex];
    }

};};