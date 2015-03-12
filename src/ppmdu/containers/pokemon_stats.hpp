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
#include <ppmdu/utils/utility.hpp>
#include <array>
#include <vector>
#include <map>
#include <string>

namespace pmd2 { namespace stats 
{

//======================================================================================================
//  Constants
//======================================================================================================
    
    static const uint8_t PkmnMaxLevel = 100;

    /*
        #TODO: this should be in an external resource file.
    */
    //enum struct ePkmnType : uint8_t
    //{
    //    None     = 0x00,
    //    Normal   = 0x01,
    //    Fire     = 0x02,
    //    Water    = 0x03,
    //    Grass    = 0x04,
    //    Electric = 0x05,
    //    Ice      = 0x06,
    //    Fighting = 0x07,
    //    Poison   = 0x08,
    //    Ground   = 0x09,
    //    Flying   = 0x0A,
    //    Psychic  = 0x0B,
    //    Bug      = 0x0C,
    //    Rock     = 0x0D,
    //    Ghost    = 0x0E,
    //    Dragon   = 0x0F,
    //    Dark     = 0x10,
    //    Steel    = 0x11,
    //};

    /*
        #TODO: this should be in an external resource file.
    */
    //enum struct ePkmnIQGrp : uint8_t
    //{
    //    A = 0x00,
    //    B = 0x01,
    //    C = 0x02,
    //    D = 0x03,
    //    E = 0x04,
    //    F = 0x05,
    //    G = 0x06,
    //    H = 0x07,
    //    //2 unused values here
    //    I = 0x0A,
    //    J = 0x0B,
    //};


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
        uint8_t  SpA;
        uint8_t  Def;
        uint8_t  SpD;

        inline bool isNull()const
        { return ( HP == 0 && Atk == 0 && SpA == 0 && Def == 0 && SpD == 0 ); }

        static const unsigned int DataLen = 6; //bytes

        //#TODO: this is implementation specific, gotta move this out.
        ////
        //template<class _outit>
        //    _outit WriteToContainer( _outit itwriteto )const
        //{
        //    itwriteto = utils::WriteIntToByteVector( HP,  itwriteto ); 
        //    //Attack first
        //    itwriteto = utils::WriteIntToByteVector( Atk, itwriteto );
        //    itwriteto = utils::WriteIntToByteVector( SpA, itwriteto );
        //    //Defense last
        //    itwriteto = utils::WriteIntToByteVector( Def, itwriteto );
        //    itwriteto = utils::WriteIntToByteVector( SpD, itwriteto ); 
        //    return itwriteto;
        //}

        ////
        //template<class _init>
        //    _init ReadFromContainer(  _init itReadfrom )
        //{
        //    HP  = utils::ReadIntFromByteVector<decltype(HP)> (itReadfrom); //iterator is incremented
        //    //Attack first
        //    Atk = utils::ReadIntFromByteVector<decltype(Atk)>(itReadfrom); //iterator is incremented
        //    SpA = utils::ReadIntFromByteVector<decltype(SpA)>(itReadfrom); //iterator is incremented
        //    //Defense last
        //    Def = utils::ReadIntFromByteVector<decltype(Def)>(itReadfrom); //iterator is incremented
        //    SpD = utils::ReadIntFromByteVector<decltype(SpD)>(itReadfrom); //iterator is incremented
        //    return itReadfrom;
        //}
    };

    /*
        Represents stats growth
    */
    struct PokeStatsGrowth
    {
        //The integer is the experience requirement, the object is the stats increase for that level, and the vector index is the level.
        typedef std::pair<uint32_t, PokeStats> growthlvl_t;
        std::vector<growthlvl_t> statsgrowth;

        static const unsigned int EntryLen         = (PokeStats::DataLen + sizeof(uint32_t) + sizeof(uint16_t));   //Length of a stats entry for a single pokemon, for a single level (with the 2 ending null bytes!)
        static const unsigned int NbEntriesPerPkmn = PkmnMaxLevel;                                                 //Nb of stats entry per pokemon
        static const unsigned int PkmnEntryLen     = EntryLen * NbEntriesPerPkmn;                                  //Length of an entry for a single pokemon
    };

    /*
        Evolution related data
    */
    struct PokeEvolution
    {
        PokeEvolution( uint16_t preevoid = 0, uint16_t evomethod = 0, uint16_t evoparam1 = 0, uint16_t evoparam2 = 0 )
            :preEvoIndex(preevoid), evoMethod(evomethod), evoParam1(evoparam1), evoParam2(evoparam2)
        {}

        uint16_t preEvoIndex;
        uint16_t evoMethod;
        uint16_t evoParam1;
        uint16_t evoParam2;

        inline bool isNull()const
        { return ( preEvoIndex == 0 && evoMethod == 0 && evoParam1 == 0 && evoParam2 == 0 ); }


        ////#TODO: this is implementation specific, gotta move this out.
        //template<class _outit>
        //    _outit WriteToContainer( _outit itwriteto )const
        //{
        //    itwriteto = utils::WriteIntToByteVector( preEvo_ID, itwriteto ); //Force this, to avoid bad surprises
        //    itwriteto = utils::WriteIntToByteVector( evoMethod, itwriteto );
        //    itwriteto = utils::WriteIntToByteVector( evoParam1, itwriteto );
        //    itwriteto = utils::WriteIntToByteVector( evoParam2, itwriteto ); //Force this, to avoid bad surprises
        //    return itwriteto;
        //}

        ////Reading the magic number, and endzero value is solely for validating on read.
        //template<class _init>
        //    _init ReadFromContainer(  _init itReadfrom )
        //{
        //    preEvo_ID = utils::ReadIntFromByteVector<decltype(preEvo_ID)>(itReadfrom); //iterator is incremented
        //    evoMethod = utils::ReadIntFromByteVector<decltype(evoMethod)>(itReadfrom); //iterator is incremented
        //    evoParam1 = utils::ReadIntFromByteVector<decltype(evoParam1)>(itReadfrom); //iterator is incremented
        //    evoParam2 = utils::ReadIntFromByteVector<decltype(evoParam2)>(itReadfrom); //iterator is incremented
        //    return itReadfrom;
        //}
    };

    /*
        The data specific to "monster.md"!
    */
    struct PokeMonsterData
    {
        uint16_t      pokeID          = 0;
        uint16_t      categoryIndex   = 0;
        uint16_t      natPkdexNb2     = 0;
        uint16_t      unk1            = 0;
        PokeEvolution evoData;
        uint16_t      spriteIndex     = 0;
        uint8_t       genderType      = 0;
        uint8_t       bodySize        = 0;
        uint8_t       primaryTy       = 0;
        uint8_t       secondaryTy     = 0;
        uint8_t       unk8            = 0;
        uint8_t       IQGrp           = 0;
        uint8_t       primAbility     = 0;
        uint8_t       secAbility      = 0;
        uint16_t      unk11           = 0;
        uint16_t      unk12           = 0;
        uint16_t      unk13           = 0;
        uint16_t      baseHP          = 0;
        uint16_t      unk14           = 0;
        uint8_t       baseAtk         = 0;
        uint8_t       baseSpAtk       = 0;
        uint8_t       baseDef         = 0;
        uint8_t       baseSpDef       = 0;
        uint16_t      unk15           = 0;
        uint16_t      unk16           = 0;
        uint8_t       unk17           = 0;
        uint8_t       unk18           = 0;
        uint8_t       unk19           = 0;
        uint8_t       unk20           = 0;
        uint16_t      unk21           = 0;
        uint16_t      unk22           = 0;
        uint16_t      unk23           = 0;
        uint16_t      unk24           = 0;
        uint16_t      unk25           = 0;
        uint16_t      unk26           = 0;
        uint16_t      unk27           = 0;
        uint16_t      unk28           = 0;
        uint16_t      unk29           = 0;
        uint16_t      unk30           = 0;

        static const unsigned int DataLen = 68;//bytes (0x44)
    };

    struct PokeMoveSet
    {
        //Self-documention
        typedef uint16_t level_t;
        typedef uint16_t moveid_t;

        PokeMoveSet()
        {}

        std::multimap<level_t, moveid_t> lvlUpMoveSet1;    //Pokemons have 2 level-up movesets for some reasons( in EoS only, only the first is used in EoT/EoD!)
        std::multimap<level_t, moveid_t> lvlUpMoveSet2;    //

        //TM moves

        //Egg moves
    };

//======================================================================================================
//  Classes
//======================================================================================================

    /*************************************************************************************
        CStatsResLoader
            Loads the data for naming the pokemons, types, IQ groups, abilities, etc..

        #TODO
    *************************************************************************************/
    class CStatsResLoader
    {
    public:
        static CStatsResLoader GetInstance();

        inline const std::vector<std::string> & PkmnTypes()const { return m_pkmnTypes; }
        inline const std::vector<std::string> & IQGroups ()const { return m_iqgrps;    }
        inline const std::vector<std::string> & Abilities()const { return m_abilities; }
        inline const std::vector<std::string> & Moves    ()const { return m_moves;     }   
        inline const std::vector<std::string> & Items    ()const { return m_items;     }    

    private:
        CStatsResLoader();
        CStatsResLoader(const CStatsResLoader&);
        CStatsResLoader(CStatsResLoader&&);
        CStatsResLoader & operator=(const CStatsResLoader&);
        CStatsResLoader & operator=(CStatsResLoader&&);

        //Parsing
        void ParseData();

        void ParsePkmnTypes();
        void ParseIQGrps();
        void ParseAbilities();
        void ParseMoves();
        void ParseItems();

        //Variable
        std::vector<std::string> m_pkmnTypes;
        std::vector<std::string> m_iqgrps;
        std::vector<std::string> m_abilities;
        std::vector<std::string> m_moves;
        std::vector<std::string> m_items;
    };


    /*************************************************************************************
        CPokemon
            Storage for pokemon statistics.
    *************************************************************************************/
    //class CPokemon
    //{
    //public:
    //    static const uint8_t MaxLevel = 100;
    //    static const uint8_t MinLevel = 1;

    //    typedef std::vector<PokeStats> pkstcntner_t;
    //    typedef std::vector<uint32_t>  expcurve_t;

    //public:
    //    CPokemon()
    //        :m_statsGrowth(MaxLevel),m_expCurve(MaxLevel,0)
    //    {}

    //    //
    //    uint32_t GetReqExp( uint8_t forlevel )const        { return m_expCurve.at(forlevel); }

    //    //
    //    const PokeStats & GetStatsGrowth( uint8_t forlevel )const  { return m_statsGrowth.at(forlevel); }

    //    //Accessors
    //    inline const std::string     & Name()const         { return m_name;        }
    //    inline std::string           & Name()              { return m_name;        }

    //    //inline const PokeMonsterData & MonsterData()const  { return m_monsterdata; }
    //    //inline PokeMonsterData       & MonsterData()       { return m_monsterdata; }

    //    inline const pkstcntner_t    & StatsGrowth()const  { return m_statsGrowth; }
    //    inline pkstcntner_t          & StatsGrowth()       { return m_statsGrowth; }

    //    inline const expcurve_t      & ExpCurve()const     { return m_expCurve;    }
    //    inline expcurve_t            & ExpCurve()          { return m_expCurve;    }

    //    inline const PokeMoveSet     & MoveSet()const      { return m_moveset;     }
    //    inline PokeMoveSet           & MoveSet()           { return m_moveset;     }

    //public:
    //    //DEBUG
    //    void DumpExpCurve  ( const std::string & filepath );
    //    void DumpStatGrowth( const std::string & filepath );
    //    void DumpMonData   ( const std::string & filepath );
    //    void DumpMoveset   ( const std::string & filepath );

    //private:
    //    //Internal data
    //    std::string   m_name;

    //private:

    //    //PokeMonsterData m_monsterdata;    //#TODO: use the pokemon's ID to lookup in a separate vector, given several pokes could share data!
    //    pkstcntner_t    m_statsGrowth;
    //    expcurve_t      m_expCurve;
    //    PokeMoveSet     m_moveset;
    //};


//======================================================================================================
//  Functions
//======================================================================================================

    ///*
    //    Export pokemon data to XML
    //*/
    //void ExportToXML( const CPokemon & src, const std::string & destfile );

    ///*
    //    Export pokemon data to text file
    //*/
    //void ExportToText( const CPokemon & src, const std::string & destfile );

    ///*
    //    Import pokemon data from XML file
    //*/
    //void ImportFromXML( const std::string & srcfile, const CPokemon & dest );

    ///*
    //    Import pokemon data from text file
    //*/
    //void ImportFromText( const std::string & srcfile, const CPokemon & dest );

};};

#endif