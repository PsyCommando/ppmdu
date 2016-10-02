#ifndef BG_LIST_DATA_HPP
#define BG_LIST_DATA_HPP
/*
bg_list_data.hpp
2016/09/15
psycommando@gmail.com
Description: Utility to read from the bg_list.dat file from pmd2!
*/
#include <ppmdu/pmd2/pmd2.hpp>
#include <cstdint>
#include <string>
#include <vector>
#include <array>

namespace filetypes
{
//============================================================================================
//  Constants
//============================================================================================
    extern const std::string FName_BGListFile;

//============================================================================================
//  Level Entry Struct
//============================================================================================
    /*
    */
    struct LevelBgEntry
    {
        static const size_t EntryLen         = 88; //The raw total length of an entry in the bg_list file!
        static const size_t LevelFnameMaxLen = 8;
        static const size_t MaxNbExtraNames  = 8;
        typedef std::array<char, LevelFnameMaxLen> lvlstr_t;
        lvlstr_t                bplname;
        lvlstr_t                bpcname;
        lvlstr_t                bmaname;
        std::vector<lvlstr_t>   extranames;
    };
    typedef std::vector<LevelBgEntry> lvlbglist_t;

//============================================================================================
//  Functions
//============================================================================================

    /*
        LoadLevelList
    */
    lvlbglist_t LoadLevelList ( const std::string & bgfilefpath );
    
    /*
        WriteLevelList
    */
    void        WriteLevelList( const std::string & bgfilefpath, const lvlbglist_t & bglist );

};
#endif
