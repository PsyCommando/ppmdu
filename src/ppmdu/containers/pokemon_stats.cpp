#include "pokemon_stats.hpp"
#include <utils/utility.hpp>
#include <utils/poco_wrapper.hpp>
#include <utils/library_wide.hpp>
#include <ppmdu/fmts/monster_data.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
using namespace std;

namespace pmd2{ namespace stats
{
    /*
        Utility function to help prepare the exported pokemon data
    */
    uint32_t PrepareExport( const PokemonDB              & pdb,
                            std::vector<PokeMonsterData> & out_md,
                            pokeMvSets_t                 & out_mvsets,
                            std::vector<PokeStatsGrowth> & out_growth)
    {
        //#2 - Estimate and Allocate
        const uint32_t nbRegulars = pdb.CountNbRegularPokemons();
        const uint32_t nbSpecials = pdb.size() - nbRegulars;
        const uint32_t nbUniques = pdb.size();
        const uint32_t totalnb = (nbRegulars * 2) + nbSpecials;

        if (utils::LibWide().isLogOn())
        {
            clog << "Splitting PokemonDB..\n"
                << "\tNb of regular entries : " << dec << nbRegulars << "\n"
                << "\tNb of special entries : " << dec << nbSpecials << "\n"
                << "\tNb of unique pokemons : " << dec << nbUniques << "\n"
                << "\tTotal                 : " << dec << totalnb << "\n";
        }
        if (nbRegulars != filetypes::MonsterMD_DefNBRegulars)
            clog << "WARNING: The amount of regular Pokemon  differs from PMD2:EoS's default value. Continuing happily.\n";
        if (nbSpecials != filetypes::MonsterMD_DefNBSpecials)
            clog << "WARNING: The amount of special Pokemon differs from PMD2:EoS's default value. Continuing happily.\n";

        out_md.resize(totalnb);
        out_mvsets.first.reserve(nbUniques);
        out_mvsets.second.reserve(nbUniques);
        out_growth.reserve(nbUniques);

        return nbRegulars;
    }

//==========================================================================================
//
//==========================================================================================

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
            throw runtime_error("ERROR: Secondary gender entries not found in parsed Pokemon monster.md data!");


        const uint32_t nbRegularPk = md.size() - offsetDups;       //The amount of regular pokes is the length of the secondary gender block
        const uint32_t nbSpecialPk = md.size() - (2 * nbRegularPk); //The amount of special pokes is the remainder of the total size minus the size of the two genders blocks added together

        if( utils::LibWide().isLogOn() )
        {
            clog << "Building PokemonDB..\n"
                 << "\tNb of regular entries : " <<dec <<nbRegularPk <<"\n"
                 << "\tNb of special entries : " <<dec <<nbSpecialPk <<"\n";
        }
        if( nbRegularPk != filetypes::MonsterMD_DefNBRegulars )
            clog << "WARNING: The amount of regular Pokemon parsed differs from PMD2:EoS's default value. Continuing happily.\n";
        if( nbSpecialPk != filetypes::MonsterMD_DefNBSpecials )
            clog << "WARNING: The amount of special Pokemon parsed differs from PMD2:EoS's default value. Continuing happily.\n";

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

    pokeMvSets_t PokemonDB::ExportMovesest()const
    {
        uint32_t nbregularpokemons = CountNbRegularPokemons();
        pokeMvSets_t mvsets;

        //Reserve the destination
        mvsets.first.reserve(size());
        mvsets.second.reserve(size());

        for (unsigned int i = 0; i < m_pkmn.size(); ++i)
        {
            if (!m_pkmn[i].MoveSet1().empty() || i == 0) //First entry is special
                mvsets.first.push_back(m_pkmn[i].MoveSet1());
            if (!m_pkmn[i].MoveSet2().empty() || i == 0) //First entry is special
                mvsets.second.push_back(m_pkmn[i].MoveSet2());
        }
        return mvsets;
    }

    std::vector<PokeMonsterData> PokemonDB::ExportMonsterData()const
    {
        const uint32_t nbreg = CountNbRegularPokemons(); //Nb of regular pokemons
        std::vector<PokeMonsterData> md;
        md.resize(size());
        for (unsigned int i = 0; i < m_pkmn.size(); ++i)
        {
            md[i] = m_pkmn[i].MonsterDataGender1();
            if (i < nbreg)
            {
                //Handle secondary gender for regulars
                md[i + m_pkmn.size()] = m_pkmn[i].MonsterDataGender2(); //Put the second gender entry in the secondary gender section!
            }
        }
        return md;
    }

    std::vector<PokeStatsGrowth> PokemonDB::ExportStatsGrowth()const
    {
        std::vector<PokeStatsGrowth> growth;
        growth.reserve(size());
        for (unsigned int i = 0; i < m_pkmn.size(); ++i)
        {
            growth.push_back(m_pkmn[i].StatsGrowth());
        }
        return growth;
    }

    uint32_t PokemonDB::CountNbRegularPokemons() const
    {
        uint32_t offsetSpecialBeg = 0;
        //Find the offset where the special monster entries begin
        for (; offsetSpecialBeg < size() && (*this)[offsetSpecialBeg].Has2GenderEntries(); ++offsetSpecialBeg);
        return offsetSpecialBeg;
    }

    uint32_t PokemonDB::CountNbSpecialPokemons() const
    {
        return size() - CountNbRegularPokemons();
    }

    uint32_t PokemonDB::CountNbUniquePokemons() const
    {
        return size();
    }

    uint32_t PokemonDB::CountNbUniqueMonsterDataEntries() const
    {
        const uint32_t nbreg = CountNbRegularPokemons();
        const uint32_t nbspecials = size() - nbreg;
        return (nbreg * 2) + nbspecials;
    }

};};