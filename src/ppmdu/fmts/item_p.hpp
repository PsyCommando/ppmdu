#ifndef ITEM_P_HPP
#define ITEM_P_HPP
/*
item_p.hpp
2015/03/11
psycommando@gmail.com
Description:
    Utilities for dealing with the "item_p.bin" and "item_s_p.bin" files of pokemon mystery dungeon 2.

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P
*/
#include <ppmdu/containers/item_data.hpp>
#include <cstdint>
#include <string>

namespace pmd2 {namespace filetypes 
{

//=========================================================================================
//  Constants
//=========================================================================================
    static const std::string ItemData_FName          = "item_p.bin";
    //EoS only
    static const std::string ExclusiveItemData_FName = "item_s_p.bin";

    //Offset Exclusive items begin
    extern const uint32_t ExclusiveItemBaseDataIndex; //Index at which exclusive items have their base data in the item_p file(EoS only)

//=========================================================================================
//  Functions
//=========================================================================================
    /*
        ParseItemsDataEoS
            * pathItemsdat: The path to the directory containing either a single item_p.bin or both item_p.bin and item_s_p.bin !
    */
    stats::ItemsDB ParseItemsDataEoS( const std::string & pathBalanceDir );

    /*
        WriteItemsDataEoS
            * pathItemsdat: The directory where the itemdata will be outputed to, in the form of at least a item_p.bin 
                            and, if there are any PMD:EoS items, possibly also an item_s_p.bin.
            * itemdata    : The item data to write the output files from.
    */
    void WriteItemsDataEoS( const std::string & pathBalanceDir, const stats::ItemsDB & itemdata );

//=========================================================================================
//  Functions
//=========================================================================================
    /*
        ParseItemsDataEoTD
            Parse the item data files from Explorers of Time/Darkness.

            * pathItemsdat: The path to the directory containing the item data files.
    */
    stats::ItemsDB ParseItemsDataEoTD( const std::string & pathBalanceDir );

    /*
        WriteItemsDataEoTD
            Write the item data files for Explorers of Time/Darkness.

            * pathItemsdat: The directory where the itemdata will be outputed to.
            * itemdata    : The item data to write the output files from.
    */
    void WriteItemsDataEoTD( const std::string & pathBalanceDir, const stats::ItemsDB & itemdata );
};};

#endif