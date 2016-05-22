#ifndef MAPPA_HPP
#define MAPPA_HPP
/*
mappa.hpp
2016/05/18
psycommando@gmail.com
Description: Utilities for parsing the mappa files from PMD2.
*/
#include <ppmdu/fmts/sir0.hpp>
#include <ppmdu/pmd2/dungeon_rng_data.hpp>

namespace filetypes
{

//========================================================================================
//  
//========================================================================================




//========================================================================================
//  Functions
//========================================================================================
    
    /*
    */
    pmd2::DungeonRNGDataSet LoadMappaSet ( const std::string & fnamemappa, const std::string & fnamemappag );

    /*
    */
    void                    WriteMappaSet( const std::string             & fnamemappa, const std::string & fnamemappag, 
                                           const pmd2::DungeonRNGDataSet & mappaset );

};
#endif
