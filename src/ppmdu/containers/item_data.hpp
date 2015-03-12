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

//
//
//

//
//
//
    struct exclusiveitemdata;
    /*
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
    */
    struct exclusiveitemdata : public itemdata
    {
        exclusiveitemdata()
        {
            exlusiveType   = 0;
            exclusiveParam = 0;
        }
        virtual ~exclusiveitemdata(){}

        virtual exclusiveitemdata * GetExclusiveItemData() { return this; }

        uint16_t exlusiveType;
        uint16_t exclusiveParam;
    };


    /*
        Used to contain all item data for the entire PMD games.
    */
    class ItemsDictionary
    {
        friend class ItemsXMLWriter;
        friend class ItemsXMLParser;
    public:
        ItemsDictionary(){}
        ItemsDictionary( std::size_t reservesize );
        ~ItemsDictionary();

        ItemsDictionary( const ItemsDictionary & other )
        {
            CopyCtor(other);
        }

        ItemsDictionary& operator=( const ItemsDictionary & other )
        {
            CopyCtor(other);
            return *this;
        }

        void CopyCtor( const ItemsDictionary & other )
        {
            m_itemData.resize( other.m_itemData.size() );

            for( unsigned int i = 0; i < other.m_itemData.size(); ++i )
            {
                if( other.m_itemData[i]->GetExclusiveItemData() == nullptr )
                {
                    m_itemData[i].reset( new itemdata( *(other.m_itemData[i]) ) );
                }
                else
                {
                    m_itemData[i].reset( new exclusiveitemdata( *(other.m_itemData[i]->GetExclusiveItemData() ) ) );
                }
            }
        }

        inline std::size_t size()const { return m_itemData.size(); }

        //The items are guaranteed to stay allocated as long as the object exists!
        inline const itemdata & Item( uint16_t itemindex )const { return *(m_itemData[itemindex].get()); }
        inline       itemdata & Item( uint16_t itemindex )      { return *(m_itemData[itemindex].get()); }

        void push_back( itemdata          && item );
        void push_back( exclusiveitemdata && item );

    private:


        std::vector<std::unique_ptr<itemdata>> m_itemData;
    };


//
//
//



};};

#endif