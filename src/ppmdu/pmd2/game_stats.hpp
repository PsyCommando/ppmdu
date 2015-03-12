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
#include <string>
#include <vector>

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

    /************************************************************************
        CGameStatsLoader
            This loads all stats from the PMD2 games into itself.
            It allows to import and export the data it has loaded.
    ************************************************************************/
    class CGameStatsLoader
    {
    public:
        CGameStatsLoader( const std::string & pmd2rootdir );

        //Accessors Pokemon Data
        //const std::vector<CPokemon> & Pkmn()const                                    { return m_pokemonStats; }
        //std::vector<CPokemon>       & Pkmn()                                         { return m_pokemonStats; }
        //void                          Pkmn( std::vector<CPokemon>       && newdata ) { m_pokemonStats = newdata; }
        //void                          Pkmn( const std::vector<CPokemon> &  newdata ) { m_pokemonStats = newdata; }

        //Accessors 
        const ItemsDictionary       & Items()const                                   { return m_itemsData; }
        ItemsDictionary             & Items()                                        { return m_itemsData; }
        void                          Items( ItemsDictionary       && newdata )      { m_itemsData = newdata; }
        void                          Items( const ItemsDictionary &  newdata )      { m_itemsData = newdata; }

        //Accessors

        //Accessors

        //Input / Output
        void Load ();
        void Load ( const std::string & newpmd2rootdir );
        void Write();
        void Write( const std::string & rootdatafolder );

    private:
        void LoadPokemonData();
        void LoadPokemonStatsGrowth();
        void LoadPokemonMoves();
        void LoadItemData();
        void LoadDungeonData();

    private:
        std::string           m_dataFolder;
        //std::vector<CPokemon> m_pokemonStats;
        ItemsDictionary       m_itemsData;
    };

};};

#endif