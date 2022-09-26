#ifndef GAME_STATS_HPP
#define GAME_STATS_HPP 
/*
game_stats.hpp
2015/03/01
psycommando@gmail.com
Description: 
    This file contains generic resources for handling the PMD2 game data.

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P
*/
#include <ppmdu/pmd2/pmd2.hpp>
#include <ppmdu/pmd2/pmd2_filetypes.hpp>
#include <ppmdu/pmd2/pmd2_text.hpp>
#include <string>
#include <vector>
#include <map>

//-------------------------------------------------------------
// For apps that don't need all the stats stuff
//-------------------------------------------------------------
#ifndef PPMDU_NOSTATS
    #include <ppmdu/containers/pokemon_stats.hpp>
    #include <ppmdu/containers/item_data.hpp>
    #include <ppmdu/containers/move_data.hpp>
    #include <ppmdu/containers/dungeons_data.hpp>
#else
    namespace stats
    {
        class PokemonDB;
        class ItemsDB;
        class MoveDB;
        class DungeonDB;
    };
#endif
//-------------------------------------------------------------

namespace pmd2
{ 
    //Game data filenames list
    // -- Pokemon --
    extern const std::string PkmnStatsGrowthFile; //Pokemon stats growth per level, and experience requirement
    extern const std::string PkmnStatsFile;       //Pokemon stats + data
    extern const std::string PkmnMovesFile;       //Pokemon level-up move list + Moves stats 

    // -- Items --
    extern const std::string ItemsStatsFile;      //
    extern const std::string ExcItmsStatsFile;    //

    //Game data location list, from "data" dir!
    //extern const std::string GameStatsFolderPath; // "/BALANCE"



    //Path under the rom root to the game's file system
    //extern const std::string GameDataDir;
    //extern const std::string BalanceDirectory;
    //extern const std::string GameTextDirectory;

//==================================================================================
//  Functions
//==================================================================================

    /*
        Returns whether the directory contains at least one of
        the expected folders for an importall operation!
    */
    bool isImportAllDir( const std::string & directory );

//==================================================================================
//  Classes
//==================================================================================


    /************************************************************************
        GameStats
            This loads all stats from the PMD2 games into itself.
            It allows to import and export the data it has loaded.
    ************************************************************************/
    class GameStats
    {
        struct strbounds_t
        {
            uint32_t beg = 0;
            uint32_t end = 0;
        };

    public:

        static const std::string DefPkmnDir;            //The default dirname the pokemon data will be exported into.
        static const std::string DefMvDir;              //The default dirname the move data will be exported into.
        static const std::string DefStrDir;             //The default dirname the game text will be exported into.
        static const std::string DefStrFExt;            //The default file extension of the strings exported
        static const std::string DefItemsDir;           //The default dirname the item data will be exported into.
        static const std::string DefDungeonDir;         //The default dirname the dungeon data will be exported into.

        /*
            Pass the game language loader that contains all the known locale strings depending on the game's text_*.str file name.
        */
        //GameStats( const std::string & pmd2rootdir, const std::string & gamelangfile );
        GameStats( const std::string & pmd2rootdir, eGameVersion gvers, eGameRegion greg, std::shared_ptr<GameText> && gtext );

        //Accessors Pokemon Data
        std::weak_ptr<const stats::PokemonDB>   Pkmn()const;
        std::weak_ptr<stats::PokemonDB>         Pkmn();
        void                     Pkmn(stats::PokemonDB       && newdata);
        void                     Pkmn(const stats::PokemonDB &  newdata);

        //Accessors 
        std::weak_ptr<const stats::ItemsDB> Items()const;
        std::weak_ptr<stats::ItemsDB>       Items();
        void                         Items(stats::ItemsDB       && newdata);
        void                         Items(const stats::ItemsDB &  newdata);

        //Accessors
        void                setRomRootDir( const std::string & path ); //{ m_romrootdir = path; }
        inline const std::string & getRomRootDir()const;

        //Accessors

        //Input / Output
        /*
            If no path is specified, will use the last path used in either the constructor or in the function below
        */
        void Load           ();
        void LoadStrings    ();
        void LoadPkmn       ();
        void LoadMoves      ();
        void LoadItems      ();
        void LoadDungeons   ();
        void Load           ( const std::string & rootdatafolder );
        void LoadText       ( const std::string & rootdatafolder );
        void LoadPkmn       ( const std::string & rootdatafolder );
        void LoadMoves      ( const std::string & rootdatafolder );
        void LoadItems      ( const std::string & rootdatafolder );
        void LoadDungeons   ( const std::string & rootdatafolder );
        
        void Write          ();
        void WritePkmn      ();
        void WriteMoves     ();
        void WriteText      (); 
        void WriteItems     ();
        void WriteDungeons  ();
        void Write          ( const std::string & rootdatafolder );
        void WritePkmn      ( const std::string & rootdatafolder );
        void WriteMoves     ( const std::string & rootdatafolder );
        void WriteItems     ( const std::string & rootdatafolder );
        void WriteDungeons  ( const std::string & rootdatafolder );

        //Export
        /*
            The Current data directory must be set to the game data
            folder the data is exported from !
            Unless everything was loaded!
        */
        void ExportAll      (const std::string & directory);
        void ExportPkmn     (const std::string & directory);
        void ExportMoves    (const std::string & directory);
        void ExportText     (const std::string & directory);
        void ExportItems    (const std::string & directory);
        void ExportDungeons (const std::string & directory);

        //Import
        /*
            The Current data directory must be set to the game data
            folder where the data will be imported to ! This is
            to allow determining the target game version, nothing will be overwritten!
        */
        void ImportAll      (const std::string & directory);
        void ImportPkmn     (const std::string & directory);
        void ImportMoves    (const std::string & directory);
        void ImportText     (const std::string & directory);
        void ImportItems    (const std::string & directory);
        void ImportDungeons (const std::string & directory);

    private:
        bool CheckStringsLoaded()const;
        void _LoadGameStrings();
        void _LoadPokemonAndMvData(); //Must be written together!
        //void _LoadMoveData   ();
        void _LoadItemData   ();
        void _LoadDungeonData();

        void _WriteGameStrings();
        void _WritePokemonAndMvData(); //Must be written together!
        //void _WriteMoveData   ();
        void _WriteItemData   ();
        void _WriteDungeonData();

        //Call this to do a check whether game strings are loaded, and load them as needed!
        void _EnsureStringsLoaded();

    private:

        std::string                 m_romrootdir;
        std::string                 m_dataFolder;
        eGameVersion                m_gameVersion;
        eGameRegion                 m_gameRegion;

        //#TODO: pimpl this, and use shared pointers instead so apps that only need one of these don't have to include a lot of useless crap!!

        std::shared_ptr<GameText>           m_gameStrings;
        std::shared_ptr<stats::PokemonDB>   m_pokemonStats;
        std::shared_ptr<stats::ItemsDB>     m_itemsData;
        //#TODO: Combine those two. The move DB should abstract game specific details!!
        std::shared_ptr<stats::MoveDB>      m_moveData1;
        std::shared_ptr<stats::MoveDB>      m_moveData2; //For Explorers of Sky only

        //Level Data
        std::shared_ptr<stats::DungeonDB>   m_dungeonsData;

        //Quiz Data

        //Mini-Game Data

        //Shop Data
    };

};

#endif