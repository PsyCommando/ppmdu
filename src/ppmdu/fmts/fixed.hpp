#ifndef FIXED_HPP
#define FIXED_HPP
/*
fixed.hpp
2020/06/22
psycommando@gmail.com
Description: Helpers for parsing the fixed.bin file used for fixed dungeons.
*/
#include <ppmdu/fmts/sir0.hpp>
#include <ppmdu/containers/dungeon_fixed_data.hpp>
#include <string>

namespace pmd2 { namespace filetypes{
//=========================================================================================
//  Constants
//=========================================================================================
    static const std::string FixedDungeonData_FName = "fixed.bin";

//=========================================================================================
//  Functions
//=========================================================================================
    /*
        ParseFixedDungeonFloorData
    */
    stats::FixedDungeonDB ParseFixedDungeonFloorData( const std::string & pathBalanceDir );

    /*
        WriteFixedDungeonFloorDataEoS
    */
    void WriteFixedDungeonFloorDataEoS( const std::string & pathBalanceDir, const stats::FixedDungeonDB & floordat );
}}

#endif