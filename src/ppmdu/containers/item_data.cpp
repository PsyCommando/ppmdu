#include "item_data.hpp"
using namespace std;

namespace pmd2{ namespace stats 
{

//
//
//
    itemdata::itemdata()
    {
        buyPrice    = 0;
        sellPrice   = 0;
        category    = 0;
        spriteID    = 0;
        itemID      = 0;
        param1      = 0;
        param2      = 0;
        param3      = 0;
        unk1        = 0;
        unk2        = 0;
        unk3        = 0;
        unk4        = 0;
    }

    void itemdata::CopyCtor( const itemdata & other )
    {
        buyPrice    = other.buyPrice;
        sellPrice   = other.sellPrice;
        category    = other.category;
        spriteID    = other.spriteID;
        itemID      = other.itemID;
        param1      = other.param1;
        param2      = other.param2;
        param3      = other.param3;
        unk1        = other.unk1;
        unk2        = other.unk2;
        unk3        = other.unk3;
        unk4        = other.unk4;

        //Call all the other copy constructors if applicable
        if( other.m_peotddata != nullptr )
            m_peotddata.reset( new itemdata_EoTD( *(other.m_peotddata) ) );
        else
            m_peotddata.reset(nullptr);

        if( other.m_peosdata != nullptr )
            m_peosdata.reset( new itemdata_EoS( *(other.m_peosdata) ) );
        else
            m_peosdata.reset(nullptr);

        if( other.m_pexdata != nullptr )
            m_pexdata.reset( new exclusiveitemdata( *(other.m_pexdata) ) );
        else
            m_pexdata.reset(nullptr);
    }

    void itemdata::MoveCtor( itemdata && other )
    {
        buyPrice    = other.buyPrice;
        sellPrice   = other.sellPrice;
        category    = other.category;
        spriteID    = other.spriteID;
        itemID      = other.itemID;
        param1      = other.param1;
        param2      = other.param2;
        param3      = other.param3;
        unk1        = other.unk1;
        unk2        = other.unk2;
        unk3        = other.unk3;
        unk4        = other.unk4;

        //Move the pointers
        m_peotddata = move(other.m_peotddata);

        m_peosdata = move(other.m_peosdata);

        m_pexdata = move(other.m_pexdata);
    }

    itemdata::itemdata( const itemdata & other )
    {
        CopyCtor(other);
    }

    itemdata::itemdata( itemdata && other )
    {
        MoveCtor( std::move(other) );
    }

    itemdata & itemdata::operator=( const itemdata & other )
    {
        CopyCtor(other);
        return *this;
    }

    itemdata & itemdata::operator=( itemdata && other )
    {
        MoveCtor( std::move(other) );
        return *this;
    }

    //Use these to set the data for each specific item types. Returns the resulting pointer, as the Getters above would return!
    exclusiveitemdata * itemdata::SetExclusiveData( const exclusiveitemdata & data )
    {
        m_pexdata.reset( new exclusiveitemdata(data) );
        return m_pexdata.get();
    }

    itemdata_EoS * itemdata::SetEoSData( const itemdata_EoS & data )
    {
        m_peosdata.reset( new itemdata_EoS(data) );
        return m_peosdata.get();
    }

    itemdata_EoTD * itemdata::SetEoTDData( const itemdata_EoTD & data )
    {
        m_peotddata.reset( new itemdata_EoTD(data) );
        return m_peotddata.get();
    }

    exclusiveitemdata * itemdata::MakeExclusiveData()
    {
        return SetExclusiveData( exclusiveitemdata() );
    }

    itemdata_EoS * itemdata::MakeEoSData()
    {
        return SetEoSData( itemdata_EoS() );
    }

    itemdata_EoTD * itemdata::MakeEoTDData()
    {
        return SetEoTDData( itemdata_EoTD() );
    }

//
//
//
    //void ItemsDB::CopyCtor( const ItemsDB & other )
    //{
    //    m_itemData.resize( other.m_itemData.size() );

    //    for( unsigned int i = 0; i < other.m_itemData.size(); ++i )
    //    {

    //        if( other.m_itemData[i]->Get_EoS_ItemData() != nullptr )
    //        {
    //            if( other.m_itemData[i]->GetExclusiveItemData() == nullptr )
    //            {
    //                m_itemData[i].reset( new itemdata_EoS( *(other.m_itemData[i]->Get_EoS_ItemData()) ) );
    //            }
    //            else
    //            {
    //                m_itemData[i].reset( new exclusiveitemdata( *(other.m_itemData[i]->GetExclusiveItemData() ) ) );
    //            }
    //        }
    //        else if( other.m_itemData[i]->Get_EoTD_ItemData() != nullptr )
    //        {
    //            m_itemData[i].reset( new itemdata_EoTD( *(other.m_itemData[i]->Get_EoTD_ItemData()) ) );
    //        }
    //        else //If all else fails just copy as is
    //        {
    //            m_itemData[i].reset( new itemdata( *(other.m_itemData[i]) ) );
    //        }
    //    }
    //}

    void ItemsDB::push_back( itemdata && item )
    {
        m_itemData.push_back( item ); //make_unique<itemdata>(item) );
    }

    void ItemsDB::push_back( const itemdata & item )
    {
        m_itemData.push_back( item );
    }

    //void ItemsDB::push_back( itemdata_EoTD && item )
    //{
    //    m_itemData.push_back( make_unique<itemdata_EoTD>(item) );
    //}
    //
    //void ItemsDB::push_back( itemdata_EoS && item )
    //{
    //    m_itemData.push_back( make_unique<itemdata_EoS>(item) );
    //}
    //
    //void ItemsDB::push_back( exclusiveitemdata && item )
    //{
    //    m_itemData.push_back( make_unique<exclusiveitemdata>(item) );
    //}

};};