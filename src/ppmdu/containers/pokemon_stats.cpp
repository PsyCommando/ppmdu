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
    //static const std::string PKMN_NamesFile     = "pkmn_names.txt";
    //static const std::string PKMN_AbilitiesFile = "pkmn_abilities.txt";
    //static const std::string PKMN_IQGrpsFile    = "pkmn_iq.txt";
    //static const std::string PKMN_TypesFile     = "pkmn_types.txt";
    //static const std::string PKMN_MovesFile     = "pkmn_moves.txt";
    //static const std::string ItemsFile          = "items.txt";

//==========================================================================================
//
//==========================================================================================
    /*
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

        //#2 - Allocate pokemon slots in the db
        mydb.m_pkmn.resize(offsetDups);

        uint32_t nbValidMvSet2 = 0;

        //#3 - Fill up each pokemon entries 
        for( unsigned int i = 0; i < mydb.m_pkmn.size(); ++i )
        {
            PokeMoveSet mvset2 = ( i < movesets.second.size() )? std::move(movesets.second[i]) : PokeMoveSet();

            if( !mvset2.empty() )
                ++nbValidMvSet2;

            //#1 - Check if we have a secondary gender entry for this poke
            if( i < nbRegularPk )
            {
                //If we do, Handle regulars
                mydb[i] =CPokemon( std::move(md[i]), 
                                    std::move(md[i+offsetDups]), 
                                    (i < growth.size() )? std::move(growth[i]) : PokeStatsGrowth(),
                                    ( i < movesets.first.size() )? std::move(movesets.first[i]) : PokeMoveSet(),
                                    std::move(mvset2) );
            }
            else
            {
                //If we don't, Handle uniques
                mydb[i] = CPokemon( std::move(md[i]), 
                                    (i < growth.size() )? std::move(growth[i]) : PokeStatsGrowth(),
                                    ( i < movesets.first.size() )? std::move(movesets.first[i]) : PokeMoveSet(),
                                    std::move(mvset2) );
            }
        }

        //If we got more than a single entry its safe to say its not a possible input error
        if( nbValidMvSet2 > 1 )
            mydb.isEoSData(true);   //Uses the waza_p2.bin file as well !
        else
             mydb.isEoSData(false);  

        return std::move(mydb);
    }

    /*
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