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
        Base item
    */
    struct itemdata
    {
        itemdata()
        {
            buyPrice    = 0;
            sellPrice   = 0;
            category    = 0;
            spriteID    = 0;
            itemID      = 0;
            itemParam1  = 0;
            itemParam2  = 0;
            itemParam3  = 0;
            unk1        = 0;
            unk2        = 0;
            unk3        = 0;
            unk4        = 0;
        }
        virtual ~itemdata(){}
        virtual exclusiveitemdata * GetExclusiveItemData() { return nullptr; }
        virtual itemdata_EoS      * Get_EoS_ItemData()     { return nullptr; }
        virtual itemdata_EoTD     * Get_EoTD_ItemData()    { return nullptr; }

        uint16_t buyPrice;
        uint16_t sellPrice;
        uint8_t  category;
        uint8_t  spriteID;
        uint16_t itemID;
        uint16_t itemParam1;
        uint8_t  itemParam2;
        uint8_t  itemParam3;
        uint8_t  unk1;
        uint8_t  unk2;
        uint8_t  unk3;
        uint8_t  unk4;
    };

    /*
        Item data format for Explorers of Time/Darkness
    */
    struct itemdata_EoTD : public itemdata
    {
        itemdata_EoTD()
            :itemdata()
        {
        }

        virtual itemdata_EoTD * Get_EoTD_ItemData() { return this; }
    };

    
    /*
        Item data format for Explorers of Sky
    */
    struct itemdata_EoS : public itemdata
    {
        itemdata_EoS()
            :itemdata()
        {
            //buyPrice    = 0;
            //sellPrice   = 0;
            //category    = 0;
            //spriteID    = 0;
            //itemID      = 0;
            //itemParam1  = 0;
            //itemParam2  = 0;
            //itemParam3  = 0;
            //unk1        = 0;
            //unk2        = 0;
            //unk3        = 0;
            //unk4        = 0;
        }
        virtual ~itemdata_EoS(){}
        virtual exclusiveitemdata * GetExclusiveItemData() { return nullptr; }
        virtual itemdata_EoS      * Get_EoS_ItemData()     { return this; }

        //uint16_t buyPrice;
        //uint16_t sellPrice;
        //uint8_t  category;
        //uint8_t  spriteID;
        //uint16_t itemID;
        //uint16_t itemParam1;
        //uint8_t  itemParam2;
        //uint8_t  itemParam3;
        //uint8_t  unk1;
        //uint8_t  unk2;
        //uint8_t  unk3;
        //uint8_t  unk4;
    };

    /*
        Additional data for exclusive items
    */
    struct exclusiveitemdata : public itemdata_EoS
    {
        exclusiveitemdata( uint16_t extype = 0, uint16_t exparam = 0 )
            :itemdata_EoS()
        {
            exlusiveType   = extype;
            exclusiveParam = exparam;
        }
        virtual ~exclusiveitemdata(){}

        virtual exclusiveitemdata * GetExclusiveItemData() { return this; }

        uint16_t exlusiveType;
        uint16_t exclusiveParam;
    };

//=====================================================================================
//  Classes
//=====================================================================================

    /*
        Used to contain all item data for the entire PMD games.
    */
    class ItemsDB
    {
        typedef std::vector<std::unique_ptr<itemdata>> itemptr_t;
        friend class ItemsXMLWriter;
        friend class ItemsXMLParser;
    public:
        ItemsDB(){}
        ItemsDB( std::size_t reservesize );
        ~ItemsDB(){}

        ItemsDB( const ItemsDB & other )
        {
            CopyCtor(other);
        }

        ItemsDB& operator=( const ItemsDB & other )
        {
            CopyCtor(other);
            return *this;
        }

        /*
            Handle copying polymorphic item type.
        */
        void CopyCtor( const ItemsDB & other );

        inline std::size_t size()const          { return m_itemData.size(); }
        inline bool        empty()const         { return m_itemData.empty(); }
        inline void        resize(size_t newsz) { return m_itemData.resize(newsz);}

        //The items are guaranteed to stay allocated as long as the object exists!
        inline const itemdata & Item( uint16_t itemindex )const { return *(m_itemData[itemindex]); }
        inline       itemdata & Item( uint16_t itemindex )      { return *(m_itemData[itemindex]); }

        inline const itemdata & operator[]( uint16_t itemindex )const { return *(m_itemData[itemindex]); }
        inline       itemdata & operator[]( uint16_t itemindex )      { return *(m_itemData[itemindex]); }

        void push_back( itemdata          && item );
        void push_back( itemdata_EoTD     && item );
        void push_back( itemdata_EoS      && item );
        void push_back( exclusiveitemdata && item );

    private:
        itemptr_t m_itemData;
    };

};};

#endif