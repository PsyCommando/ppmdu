#ifndef POKEMON_STATS_HPP
#define POKEMON_STATS_HPP
/*
pokemon_stats.hpp
2015/03/01
psycommando@gmail.com
Description: 
    Generic storage classes for containing the pokemon data, and abstracting the lower-level storage format used by the game!
*/
#include <ppmdu/pmd2/pmd2_text.hpp>
#include <utils/utility.hpp>
#include <array>
#include <vector>
#include <map>
#include <string>
#include <memory>

namespace pmd2 { namespace stats 
{

//======================================================================================================
//  Typedefs
//======================================================================================================
    //Self-documention
    typedef uint16_t level_t;
    typedef uint16_t moveid_t;

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
        PokeStats
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
    };

    /*
        PokeStatsGrowth
            Represents stats growth over 100 levels.
    */
    struct PokeStatsGrowth
    {
        //The integer is the experience requirement, the object is the stats increase for that level, and the vector index is the level.
        typedef std::pair<uint32_t, PokeStats> growthlvl_t;
        std::vector<growthlvl_t> statsgrowth;

        inline bool empty()const { return statsgrowth.empty(); }
        inline size_t size()const { return statsgrowth.size(); }

        inline const growthlvl_t & operator[](size_t index)const { return statsgrowth[index]; }
        inline       growthlvl_t & operator[](size_t index)      { return statsgrowth[index]; }

        static const unsigned int EntryLen         = (PokeStats::DataLen + sizeof(uint32_t) + sizeof(uint16_t));   //Length of a stats entry for a single pokemon, for a single level (counting the 2 ending null bytes!)
        static const unsigned int NbEntriesPerPkmn = PkmnMaxLevel;                                                 //Nb of stats entry per pokemon
        static const unsigned int PkmnEntryLen     = EntryLen * NbEntriesPerPkmn;                                  //Length of an entry for a single pokemon
    };

    /*
        PokeEvolution
            Evolution data specific.
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
    };

    /*
        PokeMonsterData
            The data from "monster.md"!
    */
    struct PokeMonsterData
    {
        uint16_t      pokeID       = 0;
        uint16_t      mdunk31      = 0;
        uint16_t      natPkdexNb   = 0;
        uint16_t      mdunk1       = 0;

        PokeEvolution evoData;

        int16_t       spriteIndex  = 0;
        uint8_t       gender       = 0;
        uint8_t       bodySize     = 0;
        uint8_t       primaryTy    = 0;
        uint8_t       secondaryTy  = 0;
        uint8_t       moveTy       = 0;
        uint8_t       IQGrp        = 0;
        uint8_t       primAbility  = 0;
        uint8_t       secAbility   = 0;
        uint16_t      bitflags1    = 0;
        uint16_t      expYield     = 0;
        int16_t       recruitRate1 = 0;
        uint16_t      baseHP       = 0;
        int16_t       recruitRate2 = 0;
        uint8_t       baseAtk      = 0;
        uint8_t       baseSpAtk    = 0;
        uint8_t       baseDef      = 0;
        uint8_t       baseSpDef    = 0;
        uint16_t      weight       = 0;
        uint16_t      size         = 0;
        uint8_t       mdunk17      = 0;
        uint8_t       mdunk18      = 0;
        uint8_t       mdunk19      = 0;
        uint8_t       mdunk20      = 0;
        uint16_t      mdunk21      = 0;
        uint16_t      BasePkmn     = 0;  //The evolution line's base pokemon entity index. Always 0 to 600. Refers to the first gender entry.

        std::array<uint16_t,4> exclusiveItems;

        uint16_t      unk27        = 0;
        uint16_t      unk28        = 0;
        uint16_t      unk29        = 0;
        uint16_t      unk30        = 0;

        static const unsigned int DataLen = 68;//bytes (0x44)
    };



    /*
    */
    struct PokeMoveSet
    {
        typedef std::multimap<level_t, moveid_t> lvlUpMoveSet_t;

        PokeMoveSet()
        {}

        inline bool empty()const { return lvlUpMoveSet.empty() && teachableHMTMs.empty() && eggmoves.empty(); }

        lvlUpMoveSet_t        lvlUpMoveSet; // Used a multimap, because several moves can be learned at the same level!
        std::vector<moveid_t> teachableHMTMs;
        std::vector<moveid_t> eggmoves;
    };

    /*
        The two movesets for all pokemon
    */
    typedef std::pair< std::vector<stats::PokeMoveSet>, std::vector<stats::PokeMoveSet> > pokeMvSets_t;

//======================================================================================================
//  Classes
//======================================================================================================

    /*************************************************************************************
        CStatsResLoader
            Loads the data for naming the pokemons, types, IQ groups, abilities, etc..

        #TODO
    *************************************************************************************/
    //class CStatsResLoader
    //{
    //public:
    //    static CStatsResLoader & GetInstance();

    //    inline const std::vector<std::string> & PkmnNames     ()const { return m_pkmnnames; }
    //    inline const std::vector<std::string> & PkmnCategories()const { return m_pkmnCat; }
    //    inline const std::vector<std::string> & PkmnTypes     ()const { return m_pkmnTypes; }
    //    inline const std::vector<std::string> & IQGroups      ()const { return m_iqgrps;    }
    //    inline const std::vector<std::string> & Abilities     ()const { return m_abilities; }
    //    inline const std::vector<std::string> & Moves         ()const { return m_moves;     }
    //    inline const std::vector<std::string> & Items         ()const { return m_items;     }

    //    /*
    //        Call this to load the data from a directory.
    //    */
    //    void Parse( const std::string & pathDataDir );

    //private:
    //    CStatsResLoader();
    //    CStatsResLoader(const CStatsResLoader&);
    //    CStatsResLoader(CStatsResLoader&&);
    //    CStatsResLoader & operator=(const CStatsResLoader&);
    //    CStatsResLoader & operator=(CStatsResLoader&&);

    //    //Parsing
    //    void ParseData(std::string pathDataDir);

    //    void ParsePkmnNames( const std::string & pkmnNamesPath );
    //    void ParsePkmnCat  ( const std::string & pkmnNamesPath );
    //    void ParsePkmnTypes( const std::string & pkmnTypesPath );
    //    void ParseIQGrps   ( const std::string & pkmnIQPath );
    //    void ParseAbilities( const std::string & pkmnAbilitiesPath );
    //    void ParseMoves    ( const std::string & pkmnMovesPath );
    //    void ParseItems    ( const std::string & ItemsPath );

    //    //Variable
    //    std::vector<std::string> m_pkmnnames;
    //    std::vector<std::string> m_pkmnCat;
    //    std::vector<std::string> m_pkmnTypes;
    //    std::vector<std::string> m_iqgrps;
    //    std::vector<std::string> m_abilities;
    //    std::vector<std::string> m_moves;
    //    std::vector<std::string> m_items;
    //};


    /*************************************************************************************
        CPokemon
            Storage for pokemon statistics.

            Explorers of Sky:
            -----------------
            MoveSets    : 553
            StatsGrowth : 571
            MonsterMD   : 1,155
            Kaomado     : 1,155

            Explorers of Time/Darkness:
            ---------------------------
            MoveSets    : 
            StatsGrowth : 1,192
            MonsterMD   : 1,155
            Kaomado     : 1,155

    *************************************************************************************/
    class CPokemon
    {
    public:
        static const uint8_t MaxLevel = PkmnMaxLevel;
        static const uint8_t MinLevel = 1;

    public:
        CPokemon()
            :m_bHas2GenderEntries(false)
        {}

        /*
            Constructor for pokemons that appear twice in the monster.md file
        */
        CPokemon( PokeMonsterData   && entryGender1, 
                  PokeMonsterData   && entryGender2, 
                  PokeStatsGrowth   && growth, 
                  PokeMoveSet       && mvs1,
                  PokeMoveSet       && mvs2)
            :m_mDataGender1(entryGender1), 
             m_mDataGender2(entryGender2), 
             m_statsGrowth(growth), 
             m_moveset_1(mvs1), 
             m_moveset_2(mvs2), 
             m_bHas2GenderEntries(true)
        {}

        /*
            Constructor for pokemons that only appear once in the monster.md file
        */
        CPokemon( PokeMonsterData   && entryGender1, 
                  PokeStatsGrowth   && growth, 
                  PokeMoveSet       && mvs1,
                  PokeMoveSet       && mvs2)
            :m_mDataGender1(entryGender1), 
             m_statsGrowth(growth), 
             m_moveset_1(mvs1), 
             m_moveset_2(mvs2), 
             m_bHas2GenderEntries(false)
        {}


        //
        uint32_t GetReqExp( uint8_t forlevel )const                { return m_statsGrowth.statsgrowth.at(forlevel).first; }

        //
        const PokeStats & GetStatsGrowth( uint8_t forlevel )const  { return m_statsGrowth.statsgrowth.at(forlevel).second; }

        //Accessors
        inline const PokeMonsterData & MonsterDataGender1()const   { return m_mDataGender1; }
        inline PokeMonsterData       & MonsterDataGender1()        { return m_mDataGender1; }

        inline const PokeMonsterData & MonsterDataGender2()const   { return m_mDataGender2; }
        inline PokeMonsterData       & MonsterDataGender2()        { return m_mDataGender2; }

        inline const PokeStatsGrowth & StatsGrowth()const          { return m_statsGrowth; }
        inline PokeStatsGrowth       & StatsGrowth()               { return m_statsGrowth; }

        inline const PokeMoveSet     & MoveSet1()const             { return m_moveset_1;   }
        inline PokeMoveSet           & MoveSet1()                  { return m_moveset_1;   }

        inline const PokeMoveSet     & MoveSet2()const             { return m_moveset_2;   }
        inline PokeMoveSet           & MoveSet2()                  { return m_moveset_2;   }

        inline bool                    Has2GenderEntries()const    { return m_bHas2GenderEntries; }
        inline bool                    Has2GenderEntries(bool val) { m_bHas2GenderEntries = val; }

    public:
        ////DEBUG
        //void DumpExpCurve  ( const std::string & filepath );
        //void DumpStatGrowth( const std::string & filepath );
        //void DumpMonData   ( const std::string & filepath );
        //void DumpMoveset   ( const std::string & filepath );

    private:
        bool            m_bHas2GenderEntries;
        PokeMonsterData m_mDataGender1;
        PokeMonsterData m_mDataGender2;
        PokeMoveSet     m_moveset_1;
        PokeMoveSet     m_moveset_2;
        PokeStatsGrowth m_statsGrowth;
    };


    /*************************************************************************************
        PokemonDB
            A storage class for containing all pokemon's data.

    *************************************************************************************/
    class PokemonDB
    {
    public:

        /*
            From the 3 containers builds a list of pokemons!
                - md : the entire content of the monster.md file. It will be split into the two genders.
                - movesets : content of waza_p.bin and and waza_p2.bin. The later is an empty vector if waza_p2.bin is not present.
                - growth: content of m_level.bin
        */
        static PokemonDB BuildDB( std::vector<PokeMonsterData>       && md, 
                                  pokeMvSets_t                       && movesets, 
                                  std::vector<PokeStatsGrowth>       && growth );

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
        static void      SplitDB( PokemonDB                          && pdb,
                                  std::vector<PokeMonsterData>       & out_md, 
                                  pokeMvSets_t                       & out_mvsets, 
                                  std::vector<PokeStatsGrowth>       & out_growth );

        /*
            Access a pokemon's name string.
        */
        //inline const std::string & PkmnName( uint16_t index )const;
        //inline       std::string & PkmnName( uint16_t index );
        /*
            Access a pokemon's category string.
        */
        //inline const std::string & PkmnCat( uint16_t index )const;
        //inline       std::string & PkmnCat( uint16_t index );

        /*
            Copy this PokemonDB's data, and place it into the 3 components that makes it up.
                - out_md     : The list of PokeMonsterData that will receive the appropriate data from the pdb!
                - out_mvsets : The 2 lists of pokemon learnsets that will receive the appropriate data from the pdb!
                - out_growth : The list of PokeStatsGrowth that will receive the appropriate data from the pdb!
        */
        void ExportComponents( std::vector<PokeMonsterData>       & out_md, 
                               pokeMvSets_t                       & out_mvsets, 
                               std::vector<PokeStatsGrowth>       & out_growth )const;

        inline const CPokemon & operator[]( uint16_t index )const { return m_pkmn[index]; }
        inline       CPokemon & operator[]( uint16_t index )      { return m_pkmn[index]; }
        inline std::size_t      size()const                       { return m_pkmn.size(); }
        inline bool             empty()const                      { return m_pkmn.empty(); }

        inline const std::vector<CPokemon> & Pkmn()const          { return m_pkmn; }
        inline       std::vector<CPokemon> & Pkmn()               { return m_pkmn; }

        inline bool             isEoSData()const                  { return m_isEoSData; } 
        inline void             isEoSData( bool val )             { m_isEoSData = val; } 

    private:
        std::vector<CPokemon> m_pkmn;
        bool                  m_isEoSData;
    };


//======================================================================================================
//  Functions
//======================================================================================================

    /*
        Write pokemon data to a directory, into several xml files. Takes 2 iterators to the beginning of the pokemon names and 
        pokemon category strings respectively! 
        
        The code expercts the amount of name strings matches the amount of pokemon in the PokemonDB object !
    */
    //void      ExportPokemonsToXML  ( const PokemonDB                         & src,
    //                                 std::vector<std::string>::const_iterator  itbegnames,
    //                                 std::vector<std::string>::const_iterator  itbegcat,
    //                                 const std::string                       & destdir );

    void      ExportPokemonsToXML  ( const PokemonDB                         & src,
                                     const GameText                          * gtext,
                                     const std::string                       & destdir );

    /*
        Read pokemon data from several xml files in a directory, into a PokemonDB.
        Also import string data from the xml files into the ranges specified by the 4 iterators.
    */
    //void      ImportPokemonsFromXML( const std::string                  & srcdir, 
    //                                 PokemonDB                          & out_pkdb,
    //                                 std::vector<std::string>::iterator   itbegnames,
    //                                 std::vector<std::string>::iterator   itendnames,
    //                                 std::vector<std::string>::iterator   itbegcat,
    //                                 std::vector<std::string>::iterator   itendcat );

    void      ImportPokemonsFromXML ( const std::string                 & srcdir, 
                                      PokemonDB                         & out_pkdb,
                                      GameText                          * inout_gtext );

    /*
        Export pokemon data to XML
    */
    //void ExportPokemonToXML( const CPokemon & src, const std::string & destfile );

    /*
        Export pokemon data to text file
    */
    //void ExportPokemonToText( const CPokemon & src, const std::string & destfile );

    /*
        Import pokemon data from XML file
    */
    //CPokemon ImportPokemonFromXML( const std::string & srcfile );

    /*
        Import pokemon data from text file
    */
    //CPokemon ImportPokemonFromText( const std::string & srcfile );

};};

#endif