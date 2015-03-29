#ifndef GAME_STATS_HPP
#define GAME_STATS_HPP 
/*
game_stats.hpp
2015/03/01
psycommando@gmail.com
Description: 
    This file contains generic resources for handling the PMD2 game data.
*/
#include <ppmdu/containers/pokemon_stats.hpp>
#include <ppmdu/containers/item_data.hpp>
#include <ppmdu/containers/move_data.hpp>
#include <string>
#include <vector>
#include <map>

namespace pmd2{ namespace stats
{ 
    //Game data filenames list
    // -- Pokemon --
    static const std::string PkmnStatsGrowthFile; //Pokemon stats growth per level, and experience requirement
    static const std::string PkmnStatsFile;       //Pokemon stats + data
    static const std::string PkmnMovesFile;       //Pokemon level-up move list + Moves stats 

    // -- Items --
    static const std::string ItemsStatsFile;      //
    static const std::string ExcItmsStatsFile;    //

    //Game data location list, from rom root!
    static const std::string GameStatsFolderPath; // "/BALANCE"

    //Game data type list
    //enum struct eGameDataTy 
    //{
    //    Invalid,
    //    PkmnStatsGrowth,
    //    PkmnStats,
    //    MovesStats,
    //    ItemsStats,
    //};

    /*
    */
    enum struct eGameVersion
    {
        Invalid,
        EoS,    //Explorers of Sky
        EoTEoD, //Explorers of Time/Darkness
        NBGameVers,
    };

//==================================================================================
//  Functions
//==================================================================================
    
    /************************************************************************
        GetGameDataTyForFile
            Return what this PMD 2 file is for. If its a valid statistics 
            file!
    ************************************************************************/
    //eGameDataTy GetGameDataTyForFile( const std::string & path );

    /************************************************************************
        LoadPkmnStatGrowthFromFile
            Use this to only load stats growth and overwrite the stats 
            growth data for the pokemon data in the "inout_pokemondata"
            vector!

            infile is the stats growth file, "m_level.bin".

    ************************************************************************/
    //void LoadPkmnStatGrowthFromFile( const std::string & infile, std::vector<CPokemon> & inout_pokemondata );

//==================================================================================
//  Classes
//==================================================================================

    /*
        Loads the list of game language and the locale strings that go with it.
    */
    class GameLanguageLoader
    {
        friend class GameLangXMLParser;
    public:
        //static const GameLanguageLoader & GetInstance( const std::string & langFilePath );
        GameLanguageLoader();
        GameLanguageLoader( const std::string & textFileName, eGameVersion version );

        /*
            Using the name of the text_*.str file, this will return the corresponding
            locale string parsed!
        */
        std::string              FindLocaleString  ( const std::string & textFileName )const;

        /*
            Return the language associated with a given text_*.str file.
        */
        std::string              FindLanguage      ( const std::string & textFileName )const;

        /*
            Return the name of the text_*.str file associated with the specified language.
        */
        std::string              FindTextStrFName  ( const std::string & language     )const;

        /*
            For a given string block name inside the text_*.str file, returns whether the 
            block was found, and the bounds of that block. The result is put into a pair.
            First is whether it was found or not, second are the bounds, beginning and end.
        */
        std::pair<bool,std::pair<uint32_t,uint32_t>> FindStrBlockOffset( const std::string & blockName, const std::string & textFileName )const;

    private:
        void LoadFile( const std::string & textFileName );

        typedef std::map<std::string,std::pair<uint32_t,uint32_t>>::value_type blockoffs_t;

        struct glang_t
        {
            std::string                    language;
            std::string                    textStrFName;
            std::string                    localeStr;
            std::map<std::string,std::pair<uint32_t,uint32_t>> strBlockOffsets;   //Name + offsets of all the sections containing specific strings
        };

        //typedef std::pair< std::string, std::string> glang_t;
        std::vector<glang_t> m_langData;
        eGameVersion         m_gameVersion;
    };


    /************************************************************************
        CGameStats
            This loads all stats from the PMD2 games into itself.
            It allows to import and export the data it has loaded.
    ************************************************************************/
    class CGameStats
    {

        struct strbounds_t
        {
            uint32_t beg = 0;
            uint32_t end = 0;
        };

    public:
        /*
            Pass the game language loader that contains all the known locale strings depending on the game's text_*.str file name.
        */
        //CGameStats( const std::string & pmd2rootdir, GameLanguageLoader && langList );
        CGameStats( const std::string & pmd2rootdir, const std::string & gamelangfile );

        //Accessors Pokemon Data
        inline const PokemonDB & Pkmn()const                        { return m_pokemonStats; }
        inline PokemonDB       & Pkmn()                             { return m_pokemonStats; }
        inline void              Pkmn( PokemonDB       && newdata ) { m_pokemonStats = newdata; }
        inline void              Pkmn( const PokemonDB &  newdata ) { m_pokemonStats = newdata; }

        inline const std::vector<std::string> Strings()const        { return m_gameStrings; }
        inline       std::vector<std::string> Strings()             { return m_gameStrings; }

        //Accessors 
        //const ItemsDB       & Items()const                      { return m_itemsData;    }
        //ItemsDB             & Items()                           { return m_itemsData;    }
        //void                  Items( ItemsDB       && newdata ) { m_itemsData = newdata; }
        //void                  Items( const ItemsDB &  newdata ) { m_itemsData = newdata; }

        //Accessors


        //Accessors

        //Input / Output
        void Load ();
        void Load ( const std::string & newpmd2rootdir );

        void LoadStringsOnly();
        void LoadStringsOnly( const std::string & newpmd2rootdir );

        void LoadPkmn();

        void Write();

        void WritePkmn( const std::string & rootdatafolder );

        void Write( const std::string & rootdatafolder );

        //Export
        void ExportPkmn( const std::string & directory );

        //Import
        void ImportPkmn( const std::string & directory );
        
    public:
        /*
            Text Strings Access
                Use those to get the correct string depending on the current game version.
        */

        std::vector<std::string>::const_iterator GetPokemonNameBeg()const;
        std::vector<std::string>::const_iterator GetPokemonNameEnd()const;
        std::vector<std::string>::iterator       GetPokemonNameBeg();
        std::vector<std::string>::iterator       GetPokemonNameEnd();

        std::vector<std::string>::const_iterator GetPokemonCatBeg()const;
        std::vector<std::string>::const_iterator GetPokemonCatEnd()const;
        std::vector<std::string>::iterator       GetPokemonCatBeg();
        std::vector<std::string>::iterator       GetPokemonCatEnd();

        std::string              & GetPokemonNameStr( uint16_t pkmnindex );
        inline const std::string & GetPokemonNameStr( uint16_t pkmnindex )const  { return const_cast<CGameStats*>(this)->GetPokemonNameStr(pkmnindex); }
        std::string              & GetPkmnCatNameStr( uint16_t pkmnindex );
        inline const std::string & GetPkmnCatNameStr( uint16_t pkmnindex )const  { return const_cast<CGameStats*>(this)->GetPkmnCatNameStr(pkmnindex); }

        std::string              & GetMoveNameStr   ( uint16_t moveindex );
        inline const std::string & GetMoveNameStr   ( uint16_t moveindex )const  { return const_cast<CGameStats*>(this)->GetMoveNameStr(moveindex); }
        std::string              & GetMoveDexcStr   ( uint16_t moveindex );
        inline const std::string & GetMoveDexcStr   ( uint16_t moveindex )const  { return const_cast<CGameStats*>(this)->GetMoveDexcStr(moveindex); }

        std::string              & GetAbilityNameStr( uint8_t abilityindex );
        inline const std::string & GetAbilityNameStr( uint8_t abilityindex )const{ return const_cast<CGameStats*>(this)->GetAbilityNameStr(abilityindex); }
        std::string              & GetAbilityDescStr( uint8_t abilityindex );
        inline const std::string & GetAbilityDescStr( uint8_t abilityindex )const{ return const_cast<CGameStats*>(this)->GetAbilityDescStr(abilityindex); }

        std::string              & GetTypeNameStr   ( uint8_t type );
        inline const std::string & GetTypeNameStr   ( uint8_t type )const        { return const_cast<CGameStats*>(this)->GetTypeNameStr(type); }

        std::string              & GetItemNameStr   ( uint16_t itemindex );
        inline const std::string & GetItemNameStr   ( uint16_t itemindex )const  { return const_cast<CGameStats*>(this)->GetItemNameStr(itemindex); }
        std::string              & GetItemSDescStr  ( uint16_t itemindex );      //Short Description
        inline const std::string & GetItemSDescStr  ( uint16_t itemindex )const  { return const_cast<CGameStats*>(this)->GetItemSDescStr(itemindex); } //Short Description
        std::string              & GetItemLDescStr  ( uint16_t itemindex );      //Long Description
        inline const std::string & GetItemLDescStr  ( uint16_t itemindex )const  { return const_cast<CGameStats*>(this)->GetItemLDescStr(itemindex); } //Long Description

        /*
            Enum for associating the values of the StrBlocksNames array below.
        */
        enum struct eStrBNames : unsigned int
        {
            PkmnNames,
            PkmnCats,
            MvNames,
            MvDesc,
            ItemNames,
            ItemDescS,
            ItemDescL,
            AbilityNames,
            AbilityDesc,
            TypeNames,

            //Add new string types above!
            NBEntries,
        };
    
    private:
        void   IdentifyGameVersion();
        void          IdentifyGameLocaleStr();
        void          BuildListOfStringOffsets(); //Make a list of all the offsets to the interesting game strings to avoid searching for them in the getstring methods below

        void LoadGameStrings();
        void LoadPokemonData();
        void LoadItemData();
        void LoadDungeonData();

        void WriteGameStrings();
        void WritePokemonData();
        void WriteItemData();
        void WriteDungeonData();

        inline strbounds_t strBounds( eStrBNames what )const
        {
            return m_strOffsets[static_cast<uint32_t>(what)];
        }

    private:

        std::string         m_dataFolder;
        std::string         m_gamelangfile;
        GameLanguageLoader  m_possibleLang;
        eGameVersion        m_gameVersion;
        std::string         m_gameLangLocale;
        std::string         m_gameTextFName;
        


        //See enum eStrBNames for what each index is for !
        std::vector<strbounds_t>    m_strOffsets;

        //Game Text
        std::vector<std::string> m_gameStrings;

        //Gameplay Data
        PokemonDB           m_pokemonStats;
        //ItemsDB             m_itemsData;

        //#TODO: Combine those two. The move DB should abstract game specific details!!
        MoveDB              m_moveData1;
        MoveDB              m_moveData2; //For Explorers of Sky only

        //Level Data

        //Quiz Data

        //Mini-Game Data

        //Shop Data
    };

};};

#endif