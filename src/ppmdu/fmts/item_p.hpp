#ifndef ITEM_P_HPP
#define ITEM_P_HPP
/*
item_p.hpp
2015/03/11
psycommando@gmail.com
Description:
    Utilities for dealing with the "item_p.bin" and "item_s_p.bin" files of pokemon mystery dungeon 2.
*/
#include <ppmdu/containers/item_data.hpp>
#include <cstdint>
#include <string>

namespace pmd2 {namespace filetypes 
{

//
//  Constants
//
    static const std::string ItemData_FName          = "item_p.bin";
    static const std::string ExclusiveItemData_FName = "item_s_p.bin";

//
//  Functions
//
    /*
        * pathItemsdat: The path to the directory containing either a single item_p.bin or both item_p.bin and item_s_p.bin !
    */
    stats::ItemsDB ParseItemsData( const std::string & pathBalanceDir );

    /*
        * pathItemsdat: The directory where the itemdata will be outputed to, in the form of at least a item_p.bin 
                        and, if there are any PMD:EoS items, possibly also an item_s_p.bin.
        * itemdata    : The item data to write the output files from.
    */
    void WriteItemsData( const std::string & pathBalanceDir, const stats::ItemsDB & itemdata );

};};

#endif