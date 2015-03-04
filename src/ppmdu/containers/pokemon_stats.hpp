#ifndef POKEMON_STATS_HPP
#define POKEMON_STATS_HPP
/*
pokemon_stats.hpp
2015/03/01
psycommando@gmail.com
Description: 
    Generic storage classes for containing the pokemon data, and abstracting the lower-level storage format used by the game!
*/
#include <ppmdu/basetypes.hpp>
#include <array>
#include <vector>
#include <string>

namespace pmd2 { namespace stats 
{

//======================================================================================================
//  Data Structures
//======================================================================================================
    /*
        Represents the stats growth for a single level/step.
    */
    struct PokeStats
    {
        PokeStats( uint16_t hp = 0, uint8_t atk = 0, uint8_t def = 0, uint8_t spa = 0, uint8_t spd = 0 )
            :HP(hp), Atk(atk), Def(def), SpA(spa), SpD(spd)
        {}

        uint16_t HP;
        uint8_t  Atk;
        uint8_t  Def;
        uint8_t  SpA;
        uint8_t  SpD;
    };

//======================================================================================================
//  Classes
//======================================================================================================
    class CPokemon
    {
    public:
        static const uint8_t MaxLevel = 100;
    public:
        CPokemon();

        //
        uint32_t GetReqExp( uint8_t atlevel )const;

        //
        uint32_t GetStatsGrowth( uint8_t atlevel )const;

    private:
        PokeStats                      m_BaseStats;
        std::vector<PokeStats>         m_statsGrowth;
        std::array<uint32_t,MaxLevel>  m_expCurve;
    };

//======================================================================================================
//  Functions
//======================================================================================================

};};

#endif