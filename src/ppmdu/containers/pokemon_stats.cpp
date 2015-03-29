#include "pokemon_stats.hpp"
#include <ppmdu/utils/utility.hpp>
#include <ppmdu/utils/poco_wrapper.hpp>
#include <ppmdu/utils/library_wide.hpp>
#include <ppmdu/fmts/monster_data.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
using namespace std;

namespace pmd2{ namespace stats
{
    static const std::string PKMN_NamesFile     = "pkmn_names.txt";
    static const std::string PKMN_AbilitiesFile = "pkmn_abilities.txt";
    static const std::string PKMN_IQGrpsFile    = "pkmn_iq.txt";
    static const std::string PKMN_TypesFile     = "pkmn_types.txt";
    static const std::string PKMN_MovesFile     = "pkmn_moves.txt";
    static const std::string ItemsFile          = "items.txt";

//
//
//
    //CStatsResLoader & CStatsResLoader::GetInstance()
    //{
    //    static CStatsResLoader s_instance;
    //    return s_instance;
    //}

    //CStatsResLoader::CStatsResLoader()
    //{
    //    Parse( utils::getCWD() );
    //}

    //void CStatsResLoader::Parse( const std::string & pathDataDir )
    //{
    //    ParseData(pathDataDir);
    //}

    ////Parsing
    //void CStatsResLoader::ParseData( std::string pathDataDir )
    //{
    //    try
    //    {
    //        pathDataDir = utils::AppendTraillingSlashIfNotThere( pathDataDir );
    //        
    //        clog<<"Parsing " <<PKMN_NamesFile <<"...\n";
    //        ParsePkmnNames(pathDataDir + PKMN_NamesFile);
    //        
    //        clog<<"Parsing " <<PKMN_TypesFile <<"...\n";
    //        ParsePkmnTypes(pathDataDir + PKMN_TypesFile);
    //        
    //        clog<<"Parsing " <<PKMN_IQGrpsFile <<"...\n";
    //        ParseIQGrps   (pathDataDir + PKMN_IQGrpsFile);
    //        
    //        clog<<"Parsing " <<PKMN_AbilitiesFile <<"...\n";
    //        ParseAbilities(pathDataDir + PKMN_AbilitiesFile);
    //        
    //        clog<<"Parsing " <<PKMN_MovesFile <<"...\n";
    //        ParseMoves    (pathDataDir + PKMN_MovesFile);
    //        
    //        //clog<<"Parsing " <<ItemsFile <<"...\n";
    //        //ParseItems    (pathDataDir + ItemsFile );
    //        clog<<"All resources loaded!\n";
    //    }
    //    catch( exception & e )
    //    {
    //        stringstream sstr;
    //        sstr<<"ERROR: Unable to parse the pkmn_*.txt string files! " <<e.what();
    //        string errorstr = sstr.str();
    //        clog<<errorstr<<"\n";
    //        throw std::runtime_error(errorstr);
    //    }
    //}

    //void CStatsResLoader::ParsePkmnNames( const std::string & pkmnNamesPath )
    //{
    //    m_pkmnnames = utils::io::ReadTextFileLineByLine( pkmnNamesPath );
    //}

    //void CStatsResLoader::ParsePkmnTypes( const std::string & pkmnTypesPath )
    //{
    //    m_pkmnTypes = utils::io::ReadTextFileLineByLine( pkmnTypesPath );
    //}
    //
    //void CStatsResLoader::ParseIQGrps   ( const std::string & pkmnIQPath )
    //{
    //    m_iqgrps = utils::io::ReadTextFileLineByLine( pkmnIQPath );
    //}

    //void CStatsResLoader::ParseAbilities( const std::string & pkmnAbilitiesPath )
    //{
    //    m_abilities = utils::io::ReadTextFileLineByLine( pkmnAbilitiesPath );
    //}

    //void CStatsResLoader::ParseMoves    ( const std::string & pkmnMovesPath )
    //{
    //    m_moves = utils::io::ReadTextFileLineByLine( pkmnMovesPath );
    //}

    //void CStatsResLoader::ParseItems    ( const std::string & ItemsPath )
    //{
    //    m_items = utils::io::ReadTextFileLineByLine( ItemsPath );
    //}

//
//  Functions
//
    //void ExportPokemonsToXML  ( const PokemonDB   & src, const std::string & destfile )
    //{
    //}
    //
    //PokemonDB ImportPokemonsFromXML( const std::string & srcfile )
    //{
    //}

    ///*
    //    Export pokemon data to XML
    //*/
    //void ExportPokemonToXML( const CPokemon & src, const std::string & destfile )
    //{
    //}

    ///*
    //    Import pokemon data from XML file
    //*/
    //CPokemon ImportPokemonFromXML( const std::string & srcfile )
    //{
    //}

//
//
//

    /*
        Function used both by SplitDB and ExportComponents to prepare for export.
        Its basically the operations both methods have a common of allocating space and etc..
    */
    uint32_t PrepareExport( const PokemonDB              & pdb, 
                            std::vector<PokeMonsterData> & out_md, 
                            pokeMvSets_t                 & out_mvsets, 
                            std::vector<PokeStatsGrowth> & out_growth )
    {
        uint32_t offsetSpecialBeg = 0;

        //#1 - Find the offset where the special entries begin
        for( ; offsetSpecialBeg < pdb.size(); ++offsetSpecialBeg )
        {
            if( !pdb[offsetSpecialBeg].Has2GenderEntries() )
                break;
        }

        //#2 - Estimate and Allocate
        const uint32_t nbRegulars = offsetSpecialBeg;
        const uint32_t nbSpecials = pdb.size() - offsetSpecialBeg;
        const uint32_t totalnb    = (nbRegulars * 2) + nbSpecials;

        if( utils::LibWide().isLogOn() )
        {
            clog << "Splitting PokemonDB..\n"
                 << "\tNb of regular entries : " <<dec <<nbRegulars <<"\n"
                 << "\tNb of special entries : " <<dec <<nbSpecials <<"\n"
                 << "\tTotal                 : " <<dec <<totalnb    <<"\n";
        }
        if( nbRegulars != filetypes::MonsterMD_DefNBRegulars )
            clog << "WARNING: The amount of regular Pokemon  differs from the default value. Continuing happily.\n";
        if( nbSpecials != filetypes::MonsterMD_DefNBSpecials )
            clog << "WARNING: The amount of special Pokemon differs from the default value. Continuing happily.\n";

        out_md.resize( totalnb );
        out_mvsets.first.reserve( nbRegulars + nbSpecials );
        out_mvsets.second.reserve( nbRegulars + nbSpecials );
        out_growth.reserve( nbRegulars + nbSpecials );

        return nbRegulars;
    }


    /*
        From the 3 containers builds a list of pokemons!
            - md : the entire content of the monster.md file. It will be split into the two genders.
            - movesets : content of waza_p.bin and and waza_p2.bin. The later is an empty vector if waza_p2.bin is not present.
            - growth: content of m_level.bin
    */
    PokemonDB PokemonDB::BuildDB( std::vector<PokeMonsterData>       && md, 
                                  pokeMvSets_t                       && movesets, 
                                  std::vector<PokeStatsGrowth>       && growth )
    {
        //Typedef to made things readable!
        typedef vector<PokeMonsterData>::const_iterator itin_t;

        PokemonDB mydb;

        //#1 - Find where the data for the secondary genders begins at.
        uint32_t offsetDups = 1; //Skip first dummy entry
        for( ; offsetDups < md.size(); ++offsetDups ) 
        {
            if( md[offsetDups].pokeID == 0 )    //Break once we hit the duplicate dummy entry
                break;
        }

        if( offsetDups == md.size() )
            throw runtime_error("ERROR: Secondary gender entries not found in parsed Pokemon monster.md data! Possibly an error in the code!");


        const uint32_t nbRegularPk = md.size() - offsetDups;       //The amount of regular pokes is the length of the secondary gender block
        const uint32_t nbSpecialPk = md.size() - (2 * nbRegularPk); //The amount of special pokes is the remainder of the total size minus the size of the two genders blocks added together

        if( utils::LibWide().isLogOn() )
        {
            clog << "Building PokemonDB..\n"
                 << "\tNb of regular entries : " <<dec <<nbRegularPk <<"\n"
                 << "\tNb of special entries : " <<dec <<nbSpecialPk <<"\n";
        }
        if( nbRegularPk != filetypes::MonsterMD_DefNBRegulars )
            clog << "WARNING: The amount of regular Pokemon parsed differs from the default value. Continuing happily.\n";
        if( nbSpecialPk != filetypes::MonsterMD_DefNBSpecials )
            clog << "WARNING: The amount of special Pokemon parsed differs from the default value. Continuing happily.\n";

        //#2 - Allocate
        mydb.m_pkmn.resize(offsetDups);

        //#3 - Fill up each entries 
        for( unsigned int i = 0; i < mydb.m_pkmn.size(); ++i )
        {
            //#1 - Check if we have a secondary gender entry for this poke
            if( i < nbRegularPk )
            {
                //If we do, Handle regulars
                mydb[i] =CPokemon( std::move(md[i]), 
                                   std::move(md[i+offsetDups]), 
                                   (i < growth.size() )? std::move(growth[i]) : PokeStatsGrowth(),
                                   ( i < movesets.first.size() )? std::move(movesets.first[i]) : PokeMoveSet(),
                                   ( i < movesets.second.size() )? std::move(movesets.second[i]) : PokeMoveSet() );
            }
            else
            {
                //If we don't, Handle uniques
                mydb[i] = CPokemon( std::move(md[i]), 
                                    (i < growth.size() )? std::move(growth[i]) : PokeStatsGrowth(),
                                    ( i < movesets.first.size() )? std::move(movesets.first[i]) : PokeMoveSet(),
                                    ( i < movesets.second.size() )? std::move(movesets.second[i]) : PokeMoveSet() );
            }
        }

        return std::move(mydb);
    }

    /*
        This takes a PokemonDB and split it off into the 3 components used to build it.
            - pdb        : The pokemon database to split into lists.
            - out_md     : The list of PokeMonsterData that will receive the appropriate data from the pdb!
            - out_mvsets : The 2 lists of pokemon learnsets that will receive the appropriate data from the pdb!
            - out_growth : The list of PokeStatsGrowth that will receive the appropriate data from the pdb!

        NOTE: The Pokemon DB is destroyed in the process, to allow using move assignements instead of using
                copie assignements, resulting in much faster code. 
                Use the ExportComponents instance method instead to output copies, if you'd like to preserve the object!
    */
    void PokemonDB::SplitDB( PokemonDB                          && pdb,
                             std::vector<PokeMonsterData>       & out_md, 
                             pokeMvSets_t                       & out_mvsets, 
                             std::vector<PokeStatsGrowth>       & out_growth )
    {
        uint32_t nbRegulars = PrepareExport( pdb, out_md, out_mvsets, out_growth );

        //#3 - Fill up
        for( unsigned int i = 0; i < pdb.size(); ++i )
        {
            out_md[i] = std::move( pdb[i].MonsterDataGender1() );
            if( i < nbRegulars )
            {
                //Handle secondary gender for regulars
                out_md[i + pdb.size()] = std::move( pdb[i].MonsterDataGender2() ); //Put the second gender entry in the secondary gender section!
            }
            if( !pdb[i].MoveSet1().empty() || i == 0 ) //First entry is special
                out_mvsets.first.push_back( std::move( pdb[i].MoveSet1() ) );
            if( !pdb[i].MoveSet2().empty() || i == 0 ) //First entry is special
                out_mvsets.second.push_back( std::move( pdb[i].MoveSet2() ) );
            if( !pdb[i].StatsGrowth().empty() )
                out_growth.push_back( std::move( pdb[i].StatsGrowth() ) );
        }
    }

    /*
        Copy this PokemonDB's data, and place it into the 3 components that makes it up.
            - out_md     : The list of PokeMonsterData that will receive the appropriate data from the pdb!
            - out_mvsets : The 2 lists of pokemon learnsets that will receive the appropriate data from the pdb!
            - out_growth : The list of PokeStatsGrowth that will receive the appropriate data from the pdb!
    */
    void PokemonDB::ExportComponents( std::vector<PokeMonsterData>       & out_md, 
                                      pokeMvSets_t                       & out_mvsets, 
                                      std::vector<PokeStatsGrowth>       & out_growth )const
    {
        uint32_t nbRegulars = PrepareExport( *this, out_md, out_mvsets, out_growth );

        //#3 - Fill up
        for( unsigned int i = 0; i < m_pkmn.size(); ++i )
        {
            out_md[i] = m_pkmn[i].MonsterDataGender1();
            if( i < nbRegulars )
            {
                //Handle secondary gender for regulars
                out_md[i + m_pkmn.size()] = m_pkmn[i].MonsterDataGender2(); //Put the second gender entry in the secondary gender section!
            }

            if( !m_pkmn[i].MoveSet1().empty() || i == 0 ) //First entry is special
                out_mvsets.first.push_back(  m_pkmn[i].MoveSet1() );
            if( !m_pkmn[i].MoveSet2().empty() || i == 0 ) //First entry is special
                out_mvsets.second.push_back( m_pkmn[i].MoveSet2() );
            if( !m_pkmn[i].StatsGrowth().empty() )
                out_growth.push_back( m_pkmn[i].StatsGrowth() );
        }
    }

};};