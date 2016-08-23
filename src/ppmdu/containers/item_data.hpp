#ifndef ITEM_DATA_HPP
#define ITEM_DATA_HPP
/*
item_data.hpp
2015/03/11
psycommando@gmail.com
Description:
    Internal representation of item data for pmd2.
*/
#include <vector>
#include <string>
#include <memory>
#include <ppmdu/pmd2/pmd2_text.hpp>

namespace pmd2 { namespace stats 
{

//=====================================================================================
//  Constants
//=====================================================================================
    static const uint32_t ItemDataNbEntry_EoS  = 1400;
    static const uint32_t ItemDataExBeg_EoS    = 444;    //Index at which exclusive items begins
    static const uint32_t ItemDataExEnd_EoS    = 1351;   //Index at which exclusive items end
    static const uint32_t ItemDataLen_EoS      = 16; //bytes
    static const uint32_t ExclusiveItemDataLen = 4;  //bytes

    static const uint32_t ItemDataNbEntry_EoTD = 1000;
    static const uint32_t ItemDataLen_EoTD     = 24; //bytes

//=====================================================================================
//  Structs
//=====================================================================================
    struct exclusiveitemdata;
    struct itemdata_EoTD;
    struct itemdata_EoS;

    /*
        NOTE: So after trying inheritance, I think it would be much better to try using composition instead to
              represent the various item formats. This is mainly because of the way we can't predict the item format
              at construction, or even at instanciation. And keeping track of the types was also not very easy..
    */

    /*
        itemdata
            Contains item data for all possible PMD2 games.
    */
    class itemdata
    {
    public:
        itemdata();
        itemdata( const itemdata & other );
        itemdata( itemdata      && other );
        itemdata & operator=( const itemdata & other );
        itemdata & operator=( itemdata      && other );

        void CopyCtor( const itemdata & other );
        void MoveCtor( itemdata      && other );

        inline exclusiveitemdata       * GetExclusiveItemData()      { return m_pexdata.get(); }
        inline itemdata_EoS            * Get_EoS_ItemData    ()      { return m_peosdata.get(); }
        inline itemdata_EoTD           * Get_EoTD_ItemData   ()      { return m_peotddata.get(); }

        inline const exclusiveitemdata * GetExclusiveItemData()const { return m_pexdata.get(); }
        inline const itemdata_EoS      * Get_EoS_ItemData    ()const { return m_peosdata.get(); }
        inline const itemdata_EoTD     * Get_EoTD_ItemData   ()const { return m_peotddata.get(); }

        //Use these to set the data for each specific item types. Returns the resulting pointer, as the Getters above would return!
        exclusiveitemdata               * SetExclusiveData    ( const exclusiveitemdata & data );
        itemdata_EoS                    * SetEoSData          ( const itemdata_EoS      & data );
        itemdata_EoTD                   * SetEoTDData         ( const itemdata_EoTD     & data );

        //Allocate a new, default constructed struct of the specified type and return the pointer to it!
        exclusiveitemdata               * MakeExclusiveData    ();
        itemdata_EoS                    * MakeEoSData          ();
        itemdata_EoTD                   * MakeEoTDData         ();

        uint16_t buyPrice;
        uint16_t sellPrice;
        uint8_t  category;
        uint8_t  spriteID;
        uint16_t itemID;
        uint16_t param1;
        uint8_t  param2;
        uint8_t  param3;
        uint8_t  unk1;
        uint8_t  unk2;
        uint8_t  unk3;
        uint8_t  unk4;

    private:
        std::unique_ptr<itemdata_EoTD>     m_peotddata;
        std::unique_ptr<itemdata_EoS>      m_peosdata;
        std::unique_ptr<exclusiveitemdata> m_pexdata;
    };

    /*
        Item data format specific to Explorers of Time/Darkness
    */
    struct itemdata_EoTD
    {
        itemdata_EoTD(){}
    };

    
    /*
        Item data format specific to Explorers of Sky
    */
    struct itemdata_EoS
    {
        itemdata_EoS(){}
    };

    /*
        Additional data for exclusive items
    */
    struct exclusiveitemdata
    {
        exclusiveitemdata( uint16_t ty = 0, uint16_t prm = 0 )
            :type(ty), param(prm)
        {}

        uint16_t type;
        uint16_t param;
    };

//=====================================================================================
//  Classes
//=====================================================================================

    /*
        Used to contain all item data for the entire PMD games.
    */
    class ItemsDB
    {
        friend class ItemsXMLWriter;
        friend class ItemsXMLParser;
    public:
        typedef std::vector<itemdata>::iterator       iterator;
        typedef std::vector<itemdata>::const_iterator const_iterator;

        ItemsDB(){}
        ItemsDB( std::size_t reservesize );
        ~ItemsDB(){}

        inline std::size_t size()const          { return m_itemData.size(); }
        inline bool        empty()const         { return m_itemData.empty(); }
        inline void        resize(size_t newsz) { return m_itemData.resize(newsz);}

        //The items are guaranteed to stay allocated as long as the object exists!
        inline const itemdata & Item( uint16_t itemindex )const { return m_itemData[itemindex]; }
        inline       itemdata & Item( uint16_t itemindex )      { return m_itemData[itemindex]; }

        inline const itemdata & operator[]( uint16_t itemindex )const { return m_itemData[itemindex]; }
        inline       itemdata & operator[]( uint16_t itemindex )      { return m_itemData[itemindex]; }

        void push_back( itemdata       && item );
        void push_back( const itemdata  & item );

        iterator       begin()      { return m_itemData.begin(); }
        iterator       end()        { return m_itemData.end(); }
        const_iterator begin()const { return m_itemData.begin(); }
        const_iterator end()const   { return m_itemData.end(); }

    private:
        std::vector<itemdata> m_itemData;
    };

//
//
//

    /*
        Export item data to XML files.
    */
    //void      ExportItemsToXML     ( const ItemsDB                           & srcitems,
    //                                 std::vector<std::string>::const_iterator  itbegitemnames,
    //                                 std::vector<std::string>::const_iterator  itenditemnames,
    //                                 std::vector<std::string>::const_iterator  itbegitemdesc,
    //                                 std::vector<std::string>::const_iterator  itenditemdesc,
    //                                 std::vector<std::string>::const_iterator  itbegitemlongdesc,
    //                                 std::vector<std::string>::const_iterator  itenditemlongdesc,
    //                                 const std::string                       & destdir );

    void      ExportItemsToXML     ( const ItemsDB                           & srcitems,
                                     const GameText                          * pgametext,
                                     const std::string                       & destdir );

    /*
        Import item data from xml files.
    */
    //ItemsDB   ImportItemsFromXML   ( const std::string                  & srcdir, 
    //                                 std::vector<std::string>::iterator  itbegitemnames,
    //                                 std::vector<std::string>::iterator  itenditemnames,
    //                                 std::vector<std::string>::iterator  itbegitemdesc,
    //                                 std::vector<std::string>::iterator  itenditemdesc,
    //                                 std::vector<std::string>::iterator  itbegitemlongdesc,
    //                                 std::vector<std::string>::iterator  itenditemlongdesc );

    ItemsDB   ImportItemsFromXML   ( const std::string                  & srcdir, 
                                     GameText                           * pgametext );




};};

#endif