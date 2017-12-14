#include "game_stats.hpp"
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
using namespace std;
using namespace pmd2::filetypes;
using namespace pmd2::stats;
using namespace filetypes;

namespace pmd2
{
//==========================================================================================
//
//==========================================================================================
    const string GameStats::DefPkmnDir    = "pokemon_data";
    const string GameStats::DefMvDir      = "move_data";
    
    const string GameStats::DefStrDir     = "game_text";
    const string GameStats::DefStrFExt    = ".txt";
    const string GameStats::DefItemsDir   = "item_data";
    const string GameStats::DefDungeonDir = "dungeon_data";

//==========================================================================================
//  Utilities
//==========================================================================================
    /*
        Test whether the directory contains XML data that
        can be imported.
    */
    bool isImportAllDir( const std::string & directory )
    {
        //Check for one of the required directories
        const string pkmndir = Poco::Path(directory).append(GameStats::DefPkmnDir ).makeDirectory().toString();
        const string mvdir   = Poco::Path(directory).append(GameStats::DefItemsDir).makeDirectory().toString();
        const string itemdir = Poco::Path(directory).append(GameStats::DefMvDir   ).makeDirectory().toString();
        return ( utils::isFolder(pkmndir) || utils::isFolder(mvdir) || utils::isFolder(itemdir) );
    }




//==========================================================================================
//  GameStats
//==========================================================================================

    GameStats::GameStats( const std::string & pmd2rootdir, eGameVersion gvers, eGameRegion gloc, std::shared_ptr<GameText> && gtext )
        :m_gameStrings(gtext), m_romrootdir(pmd2rootdir), m_gameVersion(gvers), m_gameRegion(gloc)
    {
        m_dataFolder = Poco::Path(m_romrootdir).append(DirName_DefData).toString();
    }

    void GameStats::setRomRootDir( const std::string & path )
    {
        m_romrootdir = path;
        m_dataFolder = Poco::Path(path).append(DirName_DefData).toString();
    }

//--------------------------------------------------------------
//  Loading
//--------------------------------------------------------------

    void GameStats::Load()
    {
        //First identify what we're dealing with
        _LoadGameStrings();
        _LoadPokemonAndMvData();
        _LoadItemData();
    }

    void GameStats::Load( const std::string & rootdatafolder )
    {
        if( !rootdatafolder.empty() )
            setRomRootDir(rootdatafolder);

        Load();
    }

    void GameStats::LoadStrings()
    {
        _LoadGameStrings();
    }

    void GameStats::LoadText( const std::string & rootdatafolder )
    {
        if( !rootdatafolder.empty() )
            setRomRootDir(rootdatafolder);

        LoadStrings();
    }

    void GameStats::_LoadGameStrings()
    {
        if( m_gameStrings && !m_gameStrings->AreStringsLoaded() )
            m_gameStrings->Load();
        else if( !m_gameStrings )
        {
            assert(false);
            throw std::runtime_error("GameStats::_LoadGameStrings(): m_gameStrings is nullptr!!");
        }
    }

    void GameStats::_EnsureStringsLoaded()
    {
        if( m_gameStrings && !m_gameStrings->AreStringsLoaded() )
        {
            cout<<"  <!>- Need to load target game strings file! Loading..\n";
            LoadStrings();
        }
        else if( !m_gameStrings )
        {    
            assert(false);
            throw std::runtime_error("GameStats::_EnsureStringsLoaded(): m_gameStrings is nullptr!!");
        }
    }

    void GameStats::LoadPkmn()
    {
        _LoadGameStrings();
        _LoadPokemonAndMvData();
    }

    void GameStats::LoadPkmn( const std::string & rootdatafolder )
    {
        if( !rootdatafolder.empty() )
            setRomRootDir(rootdatafolder);

        LoadPkmn();
    }

    void GameStats::_LoadPokemonAndMvData()
    {
        using namespace ::filetypes;
        stringstream sstrMd;
        sstrMd << utils::TryAppendSlash(m_dataFolder) << DirName_BALANCE << "/" << MonsterMD_FName;
        stringstream sstrMovedat;
        sstrMovedat << utils::TryAppendSlash(m_dataFolder) << DirName_BALANCE;
        stringstream sstrGrowth;
        sstrGrowth << utils::TryAppendSlash(m_dataFolder) << DirName_BALANCE << "/"  << MLevel_FName;

        cout << "Loading Pokemon Data..\n";

        //Load all the move and move set data at the same time first
        cout << " <*>-Loading move data and Pokemon movesets..\n";
        auto allmovedat = ParseMoveAndLearnsets(sstrMovedat.str());

        //Build all pokemon entries
        cout << " <*>-Building Pokemon database..\n";
        m_pokemonStats = PokemonDB::BuildDB( ParsePokemonBaseData(sstrMd.str()),
                                             std::move(allmovedat.second),
                                             ParseLevelGrowthData(sstrGrowth.str()) );

        //Set move data
        m_moveData1 = std::move(allmovedat.first.first);
        m_moveData2 = std::move(allmovedat.first.second);
        cout << "Done!\n";
    }

    void GameStats::LoadMoves( const std::string & rootdatafolder )
    {
        if( !rootdatafolder.empty() )
            setRomRootDir(rootdatafolder);

        LoadMoves();
    }

    void GameStats::LoadMoves()
    {
        using namespace filetypes;

        if( m_gameVersion != eGameVersion::Invalid )
        {
            _LoadGameStrings();
            stringstream sstrMovedat;
            sstrMovedat << utils::TryAppendSlash(m_dataFolder) << DirName_BALANCE;

            cout << "Loading moves data..";
            auto allmovedat = ParseMoveAndLearnsets(sstrMovedat.str());

            //Set move data
            m_moveData1 = std::move(allmovedat.first.first);
            m_moveData2 = std::move(allmovedat.first.second);

            cout << " Done!\n";
        }
        else
            throw std::runtime_error( "ERROR: Couldn't identify the game's version. Some files might be missing..\n" );
    }

    void GameStats::LoadItems( const std::string & rootdatafolder )
    {
        if( !rootdatafolder.empty() )
            setRomRootDir(rootdatafolder);

        LoadItems();
    }

    void GameStats::LoadItems()
    {
        if( m_gameVersion != eGameVersion::Invalid )
        {
            _LoadGameStrings();
            _LoadItemData();
            cout << " Done!\n";
        }
        else
            throw std::runtime_error( "Couldn't identify the game's version. Some files might be missing..\n" );
    }

    void GameStats::_LoadItemData()
    {
        stringstream sstrItemDat;
        sstrItemDat << utils::TryAppendSlash(m_dataFolder) << DirName_BALANCE;

        cout << "Loading items data..\n";

        if( m_gameVersion == eGameVersion::EoS )
            m_itemsData = std::move( ParseItemsDataEoS( sstrItemDat.str() ) );
        else if( m_gameVersion == eGameVersion::EoT || m_gameVersion == eGameVersion::EoD )
            m_itemsData = std::move( ParseItemsDataEoTD( sstrItemDat.str() ) );
        else
            throw runtime_error("GameStats::LoadItems(): Unknown game version!");
    }

    void GameStats::_LoadDungeonData()
    {
        cout<<"GameStats::_LoadDungeonData() : Not implemented yet !\n";
        throw exception("Not Implemented!"); 
    }

//--------------------------------------------------------------
//  Writing
//--------------------------------------------------------------
    void GameStats::Write()
    {
        //First identify what we're dealing with
        
        _WritePokemonAndMvData();
        _WriteItemData();
        _WriteGameStrings();
    }

    void GameStats::Write( const std::string & rootdatafolder )
    {
        if( !rootdatafolder.empty() )
            setRomRootDir(rootdatafolder);

        Write();
    }

    void GameStats::WritePkmn()
    {
        using namespace filetypes;
        //First identify what we're dealing with
        
        _WritePokemonAndMvData();
        _WriteGameStrings();
    }

    void GameStats::WritePkmn( const std::string & rootdatafolder )
    {
        using namespace filetypes;
        if( !rootdatafolder.empty() )
            setRomRootDir(rootdatafolder);

        WritePkmn();
    }

    void GameStats::_WritePokemonAndMvData()
    {
        using namespace ::filetypes;

        stringstream sstrMd;
        sstrMd << utils::TryAppendSlash(m_dataFolder) << DirName_BALANCE << "/" << MonsterMD_FName;
        stringstream sstrMovedat;
        sstrMovedat << utils::TryAppendSlash(m_dataFolder) << DirName_BALANCE;
        stringstream sstrGrowth;
        sstrGrowth << utils::TryAppendSlash(m_dataFolder) << DirName_BALANCE << "/"  << MLevel_FName;

        const string fStatsGrowth = sstrGrowth.str();
        const string fPokeData    = sstrMd.str();
        const string fMoveData    = sstrMovedat.str();

        cout << "Writing Pokemon Data..\n";

        //Split the pokemon data into its 3 parts
        vector<PokeMonsterData> md;
        pokeMvSets_t            mvset;
        vector<PokeStatsGrowth> sgrowth;
        cout << " <*>-Building move, Pokemon data, and level-up data lists..\n";
        m_pokemonStats.ExportComponents( md, mvset, sgrowth );

        //Write stats growth
        cout << " <*>-Writing Pokemon stats growth file \"" <<fStatsGrowth <<"\"..\n";
        WriteLevelGrowthData( sgrowth, fStatsGrowth );

        //Given the waza file contains both moves and learnsets, we have to load the move data and rewrite it as we modify the pokemon
        // movesets!
        cout << " <*>-Writing move data file(s) to directory \"" <<fMoveData <<"\"..\n";
        if( m_moveData1.empty() )
        {
            cout << "  <!>-Loading move data, as MoveDB had not been loaded..\n";
            auto movedat = ParseMoveData(fMoveData);
            cout << "  <!>-Load complete!\n";
            filetypes::WriteMoveAndLearnsets( fMoveData, movedat, mvset );
        }
        else
        {
            filetypes::WriteMoveAndLearnsets( fMoveData, make_pair( m_moveData1, m_moveData2 ), mvset );
        }

        //Write monster data
        cout << " <*>-Writing Pokemon data file \"" <<fPokeData <<"\"..\n";
        WritePokemonBaseData( md, fPokeData );

        cout << "Done importing Pokemon data!\n";
    }

    void GameStats::WriteMoves()
    {
        using namespace filetypes;
        stringstream sstrMovedat;
        sstrMovedat << utils::TryAppendSlash(m_dataFolder) << DirName_BALANCE;
        //stringstream sstrstrings;
        //sstrstrings << utils::TryAppendSlash(m_dataFolder) << GameTextDirectory << "/" << m_gameTextFName;

        const string fMoveData    = sstrMovedat.str();
        //const string fStrings     = sstrstrings.str();

        cout << "Writing moves Data..\n";

        //Given the waza file contains both moves and learnsets, we have to load the move data and rewrite it as we modify the pokemon
        // movesets!
        cout << " <*>-Need to load partially Pokemon moveset data file(s) from directory \"" <<fMoveData <<"\"..\n";
        auto mvset  = ParsePokemonLearnSets(fMoveData);

        cout << " <*>-Writing data to \"" <<fMoveData <<"\"..\n";
        WriteMoveAndLearnsets( fMoveData, make_pair( MoveDB(m_moveData1), MoveDB(m_moveData2) ), mvset );

        cout << " <*>-Writing game strings..\n";
        _WriteGameStrings();

        cout << "Done writing moves data!\n";
    }

    void GameStats::WriteMoves( const std::string & rootdatafolder )
    {
        if( !rootdatafolder.empty() )
            setRomRootDir(rootdatafolder);

        WriteMoves();
    }

    void GameStats::WriteText()
    {
        using namespace filetypes;
        auto prevGameVer = m_gameVersion;

        //Identify target game if we have no info
        

        //Warn about game mismatch
        if( prevGameVer != eGameVersion::Invalid && prevGameVer != m_gameVersion )
        {
            clog <<"WARNING: Game version mismatch. The target game data directory is from a different game than the current text data!\n"
                 <<"This will result in unforceen consequences. Continuing..\n";
        }

        _WriteGameStrings();

    }

    bool GameStats::CheckStringsLoaded()const
    {
        return m_gameStrings && m_gameStrings->AreStringsLoaded();
    }

    //void GameStats::WriteStrings( const std::string & rootdatafolder )
    //{
    //    if( !rootdatafolder.empty() )
    //        setRomRootDir(rootdatafolder);

    //    WriteStrings();
    //}

    void GameStats::_WriteGameStrings()
    {
        if( CheckStringsLoaded() )
            m_gameStrings->Write();
        else
        {
#ifdef _DEBUG
            assert(false); //this is not good
#endif
            throw std::runtime_error("GameStats::_WriteGameStrings(): The GameText object is null! The text never was loaded properly!");
        }
        //using namespace filetypes;
        //stringstream sstr;
        //sstr << utils::TryAppendSlash(m_dataFolder) << GameTextDirectory << "/" << m_gameTextFName;
        //filetypes::WriteTextStrFile( sstr.str(), m_gameStrings, std::locale( m_gameLangLocale ) );
    }
    
    void GameStats::WriteItems()
    {
        using namespace filetypes;
        auto prevGameVer = m_gameVersion;

        //Identify target game if we have no info
        _EnsureStringsLoaded();

        //Warn about game mismatch
        if( prevGameVer != eGameVersion::Invalid && prevGameVer != m_gameVersion )
        {
            clog <<"WARNING: Game version mismatch. The target game data directory is from a different game than the current item data!\n"
                 <<"This will result in unforceen consequences! Continuing..\n";
        }

        _WriteItemData();
        _WriteGameStrings();
    }

    void GameStats::WriteItems( const std::string & rootdatafolder )
    {
        if( !rootdatafolder.empty() )
            setRomRootDir(rootdatafolder);

        WriteItems();
    }

    void GameStats::_WriteItemData()
    {
        using namespace filetypes;

        stringstream sstrItemdat;
        sstrItemdat << utils::TryAppendSlash(m_dataFolder) << DirName_BALANCE;
        const string balancedirpath = sstrItemdat.str();

        cout << " <*>- Writing item data to \"" <<balancedirpath <<"\"..\n";

        if( m_gameVersion == eGameVersion::EoS )
            WriteItemsDataEoS( balancedirpath, m_itemsData );
        else if( m_gameVersion == eGameVersion::EoT || m_gameVersion == eGameVersion::EoD )
            WriteItemsDataEoTD( balancedirpath, m_itemsData );
        else
        {
            throw runtime_error( "GameStats::_WriteItemData(): Unsuported game version !" );
        }
    }

    void GameStats::_WriteDungeonData()
    {
        throw exception("Not Implemented!"); //Not implemented yet !
    }


//--------------------------------------------------------------
//  Export/Import
//--------------------------------------------------------------
    void GameStats::ExportPkmn( const std::string & directory )
    {
        //cout<<"-- Exporting all Pokemon data to XML data --\n";
        if( !CheckStringsLoaded() || m_pokemonStats.empty() )
            throw runtime_error("ERROR: Tried to export an empty list of Pokemon ! Or with an empty string list!");

        cout<<" <*>- Writing Pokemon XML data..";
        stats::ExportPokemonsToXML( m_pokemonStats, m_gameStrings.get(), directory );
        cout<<" Done!\n";
    }

    void GameStats::ImportPkmn( const std::string & directory )
    {
        //cout<<"-- Importing all Pokemon from XML data --\n";
        //Need game strings loaded for this !
        _EnsureStringsLoaded();

        cout<<" <*>- Parsing Pokemon XML data..";
        stats::ImportPokemonsFromXML( directory, m_pokemonStats, m_gameStrings.get() );
        cout<<" Done!\n";
    }

    void GameStats::ExportMoves( const std::string & directory )
    {
        //cout<<"-- Exporting all moves data to XML data --\n";
        if( !CheckStringsLoaded() || m_moveData1.empty() )
            throw runtime_error("Move list(s) is/are empty. Or the game strings are not loaded!");

        cout << " <*>- Writing moves to XML data.. ";
        if( m_gameVersion == eGameVersion::EoS )
            stats::ExportMovesToXML( m_moveData1, &m_moveData2, m_gameStrings.get(), directory );
        else if( m_gameVersion == eGameVersion::EoT || m_gameVersion == eGameVersion::EoD )
            stats::ExportMovesToXML( m_moveData1, nullptr, m_gameStrings.get(), directory );
        cout << " Done!\n";
    }

    void GameStats::ImportMoves( const std::string & directory )
    {
        //cout<<"-- Importing all moves data from XML data --\n";
        //Need game strings loaded for this !
        _EnsureStringsLoaded();

        cout<<" <*>- Parsing moves XML data..";
        if( m_gameVersion == eGameVersion::Invalid )
            throw runtime_error("Game version is invalid, or was not determined. Cannot import move data and format it!");
        else if( m_gameVersion == eGameVersion::EoS )
            stats::ImportMovesFromXML( directory, m_moveData1, &m_moveData2, m_gameStrings.get() );
        else if( m_gameVersion == eGameVersion::EoT || m_gameVersion == eGameVersion::EoD )
            stats::ImportMovesFromXML( directory, m_moveData1, nullptr, m_gameStrings.get() );
        cout<<" Done!\n";
    }

    void GameStats::ExportText( const std::string & directory )
    {
        //cout<<"-- Exporting all game strings to text file \"" <<file <<"\" --\n";
        if( !CheckStringsLoaded() )
            throw runtime_error( "GameStats::ExportText(): No string data to export !" );

        cout<<" <*>- Writing game text to \"" <<directory <<"\" ..";
        m_gameStrings->ExportText(directory);
        cout<<" Done!\n";
    }

    void GameStats::ImportText( const std::string & directory )
    {

        cout<<" <*>- Importing game text from \"" <<directory <<"\" ..";
        m_gameStrings->ImportText(directory);
        cout<<"Done importing strings!\n";
    }

    void GameStats::ExportItems( const std::string & directory )
    {
        if( !CheckStringsLoaded() ||  m_itemsData.empty() )
            throw runtime_error( "No item data to export, or the strings weren't loaded!!" );

        cout<<" <*>- Exporting items to XML..";
        stats::ExportItemsToXML( m_itemsData, m_gameStrings.get(), directory );
        cout << " Done!\n";
    }

    void GameStats::ImportItems( const std::string & directory )
    {
        //Need game strings loaded for this !
        _EnsureStringsLoaded();

        if( m_gameVersion == eGameVersion::Invalid )
            throw runtime_error("Game version is invalid, or was not determined. Cannot import item data and format it!");

        cout<<" <*>- Parsing items XML data..";
        m_itemsData = std::move( stats::ImportItemsFromXML( directory, m_gameStrings.get() ) );
        cout<<" Done!\n";
    }

    void GameStats::ExportAll( const std::string & directory )
    {
        cout<<"-- Exporting everything to XML --\n";
        const string pkmndir = Poco::Path(directory).makeAbsolute().append(DefPkmnDir ).makeDirectory().toString();
        const string mvdir   = Poco::Path(directory).makeAbsolute().append(DefMvDir   ).makeDirectory().toString();
        const string itemdir = Poco::Path(directory).makeAbsolute().append(DefItemsDir).makeDirectory().toString();

        if( m_pokemonStats.empty() && m_moveData1.empty() && m_itemsData.empty() )
            throw runtime_error( "No data to export!" );

        _EnsureStringsLoaded();

        if( !m_pokemonStats.empty() )
        {
            utils::DoCreateDirectory(pkmndir);

            cout<<" <*>- Has Pokemon data to export. Exporting to \"" <<pkmndir <<"\"..\n  ";
            ExportPkmn( pkmndir );
        }
        else
            cout<<" <!>- No item Pokemon data to export, skipping..\n";

        if( !m_moveData1.empty() )
        {
            utils::DoCreateDirectory(mvdir);

            cout<<" <*>- Has moves data to export. Exporting to \"" <<mvdir <<"\"..\n  ";
            ExportMoves( mvdir );
        }
        else
            cout<<" <!>- No move data to export, skipping..\n";

        if( !m_itemsData.empty() )
        {
            utils::DoCreateDirectory(itemdir);

            cout<<" <*>- Has items data to export. Exporting to \"" <<itemdir <<"\"..\n  ";
            ExportItems( itemdir );
        }
        else
            cout<<" <!>- No item data to export, skipping..\n";

        cout<<"-- Export complete! --\n";
    }

    void GameStats::ImportAll( const std::string & directory )
    {
        using namespace utils;
        cout<<"-- Importing everything from XML --\n";
        const string pkmndir = Poco::Path(directory).makeAbsolute().append(DefPkmnDir ).makeDirectory().toString();
        const string mvdir   = Poco::Path(directory).makeAbsolute().append(DefMvDir   ).makeDirectory().toString();
        const string itemdir = Poco::Path(directory).makeAbsolute().append(DefItemsDir).makeDirectory().toString();

        //Check what we can import
        bool importPokes = pathExists(pkmndir);
        bool importMoves = pathExists(mvdir  );
        bool importItems = pathExists(itemdir);

        if( !importPokes && !importMoves && !importItems )
        {
            stringstream sstrerr;
            sstrerr << "Couldn't find any expected data directories in specified directory!:\n"
                    << " - Expected \"" <<pkmndir <<"\", but no such directory exists!\n" 
                    << " - Expected \"" <<mvdir <<"\", but no such directory exists!\n" 
                    << " - Expected \"" <<itemdir <<"\", but no such directory exists!\n";
            const string strerr = sstrerr.str();
            clog <<strerr <<"\n";
            throw runtime_error( strerr );
        }

        if( !m_pokemonStats.empty() || !m_moveData1.empty() || !m_itemsData.empty() )
            cout <<"  <!>- WARNING: The data already loaded will be overwritten! Continuing happily..\n";

        //Need game strings loaded for this !
        _EnsureStringsLoaded();

        //Check after loading strings
        if( m_gameVersion == eGameVersion::Invalid )
            throw runtime_error("Game version is invalid, or could not be determined!");

        //Run all the import methods
        if(importPokes)
            ImportPkmn(pkmndir);
        if(importMoves)
            ImportMoves(mvdir);
        if(importItems)
            ImportItems(itemdir);

        cout<<"-- Import complete! --\n";
    }

//--------------------------------------------------------------
//  Text Strings Access
//--------------------------------------------------------------

    //std::vector<std::string>::const_iterator GameStats::GetPokemonNameBeg()const
    //{
    //    return const_cast<GameStats*>(this)->GetPokemonNameBeg();
    //}

    //std::vector<std::string>::const_iterator GameStats::GetPokemonNameEnd()const
    //{
    //    return const_cast<GameStats*>(this)->GetPokemonNameEnd();
    //}

    //std::vector<std::string>::iterator GameStats::GetPokemonNameBeg()
    //{
    //    return (m_gameStrings.begin() + strBounds(eStringBlocks::PkmnNames).beg );
    //}

    //std::vector<std::string>::iterator GameStats::GetPokemonNameEnd()
    //{
    //    return (m_gameStrings.begin() + strBounds(eStringBlocks::PkmnNames).end );
    //}

    //std::vector<std::string>::const_iterator GameStats::GetPokemonCatBeg()const
    //{
    //    return const_cast<GameStats*>(this)->GetPokemonCatBeg();
    //}
    //std::vector<std::string>::const_iterator GameStats::GetPokemonCatEnd()const
    //{
    //    return const_cast<GameStats*>(this)->GetPokemonCatEnd();
    //}
    //std::vector<std::string>::iterator GameStats::GetPokemonCatBeg()
    //{
    //    return (m_gameStrings.begin() +strBounds(eStringBlocks::PkmnCats).beg );
    //}
    //std::vector<std::string>::iterator GameStats::GetPokemonCatEnd()
    //{
    //    return (m_gameStrings.begin() +strBounds(eStringBlocks::PkmnCats).end );
    //}


    //std::vector<std::string>::const_iterator GameStats::GetMoveNamesBeg()const
    //{
    //    return const_cast<GameStats*>(this)->GetMoveNamesBeg();
    //}

    //std::vector<std::string>::const_iterator GameStats::GetMoveNamesEnd()const
    //{
    //    return const_cast<GameStats*>(this)->GetMoveNamesEnd();
    //}

    //std::vector<std::string>::iterator       GameStats::GetMoveNamesBeg()
    //{
    //    return (m_gameStrings.begin() +strBounds(eStringBlocks::MvNames).beg );
    //}

    //std::vector<std::string>::iterator       GameStats::GetMoveNamesEnd()
    //{
    //    return (m_gameStrings.begin() +strBounds(eStringBlocks::MvNames).end );
    //}

    //std::vector<std::string>::const_iterator GameStats::GetMoveDescBeg()const
    //{
    //    return const_cast<GameStats*>(this)->GetMoveDescBeg();
    //}

    //std::vector<std::string>::const_iterator GameStats::GetMoveDescEnd()const
    //{
    //    return const_cast<GameStats*>(this)->GetMoveDescEnd();
    //}

    //std::vector<std::string>::iterator       GameStats::GetMoveDescBeg()
    //{
    //    return (m_gameStrings.begin() +strBounds(eStringBlocks::MvDesc).beg );
    //}

    //std::vector<std::string>::iterator       GameStats::GetMoveDescEnd()
    //{
    //    return (m_gameStrings.begin() +strBounds(eStringBlocks::MvDesc).end );
    //}

    //std::vector<std::string>::const_iterator GameStats::GetItemNamesBeg()const
    //{
    //    return const_cast<GameStats*>(this)->GetItemNamesBeg();
    //}

    //std::vector<std::string>::const_iterator GameStats::GetItemNamesEnd()const
    //{
    //    return const_cast<GameStats*>(this)->GetItemNamesEnd();
    //}

    //std::vector<std::string>::iterator       GameStats::GetItemNamesBeg()
    //{
    //    return (m_gameStrings.begin() +strBounds(eStringBlocks::ItemNames).beg );
    //}

    //std::vector<std::string>::iterator       GameStats::GetItemNamesEnd()
    //{
    //    return (m_gameStrings.begin() +strBounds(eStringBlocks::ItemNames).end );
    //}

    //std::vector<std::string>::const_iterator GameStats::GetItemShortDescBeg()const
    //{
    //    return const_cast<GameStats*>(this)->GetItemShortDescBeg();
    //}

    //std::vector<std::string>::const_iterator GameStats::GetItemShortDescEnd()const
    //{
    //    return const_cast<GameStats*>(this)->GetItemShortDescEnd();
    //}

    //std::vector<std::string>::iterator       GameStats::GetItemShortDescBeg()
    //{
    //    return (m_gameStrings.begin() +strBounds(eStringBlocks::ItemDescS).beg );
    //}

    //std::vector<std::string>::iterator       GameStats::GetItemShortDescEnd()
    //{
    //    return (m_gameStrings.begin() +strBounds(eStringBlocks::ItemDescS).end );
    //}

    //std::vector<std::string>::const_iterator GameStats::GetItemLongDescBeg()const
    //{
    //    return const_cast<GameStats*>(this)->GetItemLongDescBeg();
    //}

    //std::vector<std::string>::const_iterator GameStats::GetItemLongDescEnd()const
    //{
    //    return const_cast<GameStats*>(this)->GetItemLongDescEnd();
    //}

    ////Portraits
    //std::vector<std::string>::const_iterator GameStats::GetPortraitNamesBeg()const
    //{
    //    return const_cast<GameStats*>(this)->GetPortraitNamesBeg();
    //}

    //std::vector<std::string>::const_iterator GameStats::GetPortraitNamesEnd()const
    //{
    //    return const_cast<GameStats*>(this)->GetPortraitNamesEnd();
    //}

    //std::vector<std::string>::iterator       GameStats::GetPortraitNamesBeg()
    //{
    //    return (m_gameStrings.begin() +strBounds(eStringBlocks::PortraitNames).beg );
    //}

    //std::vector<std::string>::iterator       GameStats::GetPortraitNamesEnd()
    //{
    //    return (m_gameStrings.begin() +strBounds(eStringBlocks::PortraitNames).end );
    //}

    //std::vector<std::string>::iterator       GameStats::GetItemLongDescBeg()
    //{
    //    return (m_gameStrings.begin() +strBounds(eStringBlocks::ItemDescL).beg );
    //}

    //std::vector<std::string>::iterator       GameStats::GetItemLongDescEnd()
    //{
    //    return (m_gameStrings.begin() +strBounds(eStringBlocks::ItemDescL).end );
    //}

    //std::string & GameStats::GetPokemonNameStr( uint16_t pkmnindex )
    //{
    //    return m_gameStrings[strBounds(eStringBlocks::PkmnNames).beg + pkmnindex];
    //}

    //std::string & GameStats::GetPkmnCatNameStr( uint16_t pkmnindex )
    //{
    //    return m_gameStrings[strBounds(eStringBlocks::PkmnCats).beg + pkmnindex];
    //}

    //std::string & GameStats::GetMoveNameStr( uint16_t moveindex )
    //{
    //    return m_gameStrings[strBounds(eStringBlocks::MvNames).beg + moveindex];
    //}

    //std::string & GameStats::GetMoveDexcStr( uint16_t moveindex )
    //{
    //    return m_gameStrings[strBounds(eStringBlocks::MvDesc).beg + moveindex];
    //}

    //std::string & GameStats::GetAbilityNameStr( uint8_t abilityindex )
    //{
    //    return m_gameStrings[strBounds(eStringBlocks::AbilityNames).beg + abilityindex];
    //}

    //std::string & GameStats::GetAbilityDescStr( uint8_t abilityindex )
    //{
    //    return m_gameStrings[strBounds(eStringBlocks::AbilityDesc).beg + abilityindex];
    //}

    //std::string & GameStats::GetTypeNameStr( uint8_t type )
    //{
    //    return m_gameStrings[strBounds(eStringBlocks::TypeNames).beg + type];
    //}

    //std::string & GameStats::GetItemNameStr( uint16_t itemindex )
    //{
    //    return m_gameStrings[strBounds(eStringBlocks::ItemNames).beg + itemindex];
    //}

    //std::string & GameStats::GetItemSDescStr( uint16_t itemindex )
    //{
    //    return m_gameStrings[strBounds(eStringBlocks::ItemDescS).beg + itemindex];
    //}

    //std::string & GameStats::GetItemLDescStr( uint16_t itemindex )
    //{
    //    return m_gameStrings[strBounds(eStringBlocks::ItemDescL).beg + itemindex];
    //}



//--------------------------------------------------------------
//  Misc
//--------------------------------------------------------------


};