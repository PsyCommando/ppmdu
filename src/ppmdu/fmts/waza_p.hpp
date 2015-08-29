#ifndef WAZA_P_HPP
#define WAZA_P_HPP
/*
waza_p.hpp
2015/03/07
psycommando@gmail.com
Description:
    Utilities for parsing the content of the waza_p.bin and waza_p2.bin files !
*/
#include <vector>
#include <string>
#include <ppmdu/containers/pokemon_stats.hpp>
#include <ppmdu/containers/move_data.hpp>

namespace pmd2 { namespace filetypes
{
    typedef std::pair<stats::MoveDB,stats::MoveDB> combinedmovedat_t;

    //static const ContentTy CnTy_WAZA_P  {"WAZA_P"};
    //static const ContentTy CnTy_WAZA_P2 {"WAZA_P2"};

//
//  Constants
//
    static const std::string WAZA_Fname  = "waza_p.bin";
    static const std::string WAZA2_Fname = "waza_p2.bin";

//
//  Class
//
    //class Waza_p
    //{
    //public:
    //    Waza_p();

    //    void Load ( const std::string & srcBalanceDir  );
    //    void Write( const std::string & destBalanceDir );

    //    //Returns whether the directory is from Explorers of Sky, or not.
    //    bool isEoS()const;

    //private:
    //    std::vector<stats::PokeMoveSet> m_learnsets;
    //};

//
//  Functions
//
    /*
        ParsePokemonLearnSets
            Read the pokemon level-up move list.
    */
    stats::pokeMvSets_t              ParsePokemonLearnSets( const std::string          & pathOfBalanceDir );
    std::vector<stats::PokeMoveSet>      ParsePokemonLearnSets( const std::vector<uint8_t> & waza_pData );
    stats::pokeMvSets_t              ParsePokemonLearnSets( const std::vector<uint8_t> & waza_pData, 
                                                                const std::vector<uint8_t> & waza_p2Data );

    /*
        ParseMoveData
            Will parse all the move data to a MoveDB if EoT/EoD, and two if EoS!
    */
    std::pair<stats::MoveDB,stats::MoveDB> ParseMoveData( const std::string          & pathOfBalanceDir );
    stats::MoveDB                          ParseMoveData( const std::vector<uint8_t> & waza_pData );
    std::pair<stats::MoveDB,stats::MoveDB> ParseMoveData( const std::vector<uint8_t> & waza_pData, 
                                                          const std::vector<uint8_t> & waza_p2Data );

    /*
        ParseMoveAndLearnsets
            Parse both the above at the same time.
    */
    std::pair<combinedmovedat_t,stats::pokeMvSets_t> ParseMoveAndLearnsets( const std::string & pathOfBalanceDir );

    /*
        WriteMoveAndLearnsets
            Will write at least a "waza_p.bin" file. If both the second learnsets and moves data lists are not empty, will 
            output an additional "waza_p2.bin" file!

    */
    void WriteMoveAndLearnsets( const std::string                            & pathOutBalanceDir,
                                const std::pair<stats::MoveDB,stats::MoveDB> & movedata, 
                                const stats::pokeMvSets_t                & lvlupmvset );

};};

#endif