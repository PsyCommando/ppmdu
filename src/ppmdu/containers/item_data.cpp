#include "item_data.hpp"
using namespace std;

namespace pmd2{ namespace stats 
{

    void ItemsDB::CopyCtor( const ItemsDB & other )
    {
        m_itemData.resize( other.m_itemData.size() );

        for( unsigned int i = 0; i < other.m_itemData.size(); ++i )
        {

            if( other.m_itemData[i]->Get_EoS_ItemData() != nullptr )
            {
                if( other.m_itemData[i]->GetExclusiveItemData() == nullptr )
                {
                    m_itemData[i].reset( new itemdata_EoS( *(other.m_itemData[i]->Get_EoS_ItemData()) ) );
                }
                else
                {
                    m_itemData[i].reset( new exclusiveitemdata( *(other.m_itemData[i]->GetExclusiveItemData() ) ) );
                }
            }
            else if( other.m_itemData[i]->Get_EoTD_ItemData() != nullptr )
            {
                m_itemData[i].reset( new itemdata_EoTD( *(other.m_itemData[i]->Get_EoTD_ItemData()) ) );
            }
            else //If all else fails just copy as is
            {
                m_itemData[i].reset( new itemdata( *(other.m_itemData[i]) ) );
            }
        }
    }

    void ItemsDB::push_back( itemdata && item )
    {
        m_itemData.push_back( make_unique<itemdata>(item) );
    }

    void ItemsDB::push_back( itemdata_EoTD && item )
    {
        m_itemData.push_back( make_unique<itemdata_EoTD>(item) );
    }
    
    void ItemsDB::push_back( itemdata_EoS && item )
    {
        m_itemData.push_back( make_unique<itemdata_EoS>(item) );
    }
    
    void ItemsDB::push_back( exclusiveitemdata && item )
    {
        m_itemData.push_back( make_unique<exclusiveitemdata>(item) );
    }

};};