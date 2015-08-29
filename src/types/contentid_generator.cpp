#include "contentid_generator.hpp"
#include <iostream>
#include <utils/library_wide.hpp>
using namespace std;

namespace filetypes
{
    const ContentTy CnTy_Invalid{"INVALID"};

//================================================================================================
//  ContentTy
//================================================================================================
    ContentTy::ContentTy( const string & extension )
        :m_ext(extension)
    {
        //Acquire id
        m_id = ContentIDManager::GetInstance().AcquireID(*this);

        if( utils::LibWide().isLogOn() )
            clog <<"Acquired Content ID " <<m_id <<" for " <<m_ext <<"\n";
    }

    ContentTy::ContentTy( ContentTy && other )
    {
        m_id   = other.m_id;
        m_ext = std::move(other.m_ext);
        //invalidate other id
        other.m_id = std::numeric_limits<cnt_t>::max();
    }

    ContentTy::~ContentTy()
    {
        if( m_id == std::numeric_limits<cnt_t>::max() )
            return;

        //release id
        ContentIDManager::GetInstance().ReleaseID(*this, m_id);

        if( utils::LibWide().isLogOn() )
            clog <<"Released Content ID " <<m_id <<" for " <<m_ext <<"\n";
    }

    ContentTy & ContentTy::operator=( ContentTy && other )
    {
        m_id   = other.m_id;
        m_ext  = std::move(other.m_ext);
        //invalidate other id
        other.m_id = std::numeric_limits<cnt_t>::max();
        return *this;
    }

//================================================================================================
//  ContentIDManager
//================================================================================================

    ContentIDManager & ContentIDManager::GetInstance()
    {
        static ContentIDManager s_instance;
        return s_instance;
    }

    cnt_t ContentIDManager::AcquireID( const ContentTy & owner )
    {
        cnt_t curid = -1;
        if( !m_released.empty() )
        {
            curid = *m_released.end();
            m_released.erase( m_released.end() );
        }
        else if( m_lastgenid < std::numeric_limits<cnt_t>::max() )
        {
            curid = m_lastgenid;
            m_lastgenid++;
        }
        else
            throw std::overflow_error("ContentIDManager::AcquireID(): Out of possible values!");

        m_active.emplace( curid, std::addressof(owner) ); 

        return curid;
    }

    const ContentTy * ContentIDManager::FindMatchingCnt( cnt_t id )const
    {
        auto found = m_active.find(id);

        if( found != m_active.end() )
            return found->second;
        else
            return nullptr;
    }
    
    const ContentTy * ContentIDManager::FindMatchingCnt( const std::string & extension )const
    {
        for( const auto cnt : m_active )
        {
            if( cnt.second->name() == extension )
                return cnt.second;
        }
        return nullptr;
    }

    void ContentIDManager::ReleaseID( const ContentTy & owner, cnt_t id )
    {
        if( id > m_lastgenid )
            throw std::runtime_error("ContentIDManager::ReleaseID(): Tried to release an ID we didn't assign yet!");

        m_released.emplace(id);
        m_active.erase(id);
    }

};