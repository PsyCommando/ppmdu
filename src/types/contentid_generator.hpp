#ifndef CONTENT_ID_GENERATOR_HPP
#define CONTENT_ID_GENERATOR_HPP
/*
contentid_generator.hpp
2015/08/28
psycommando@gmail.com
Description: This class handles the creation of unique content ID handles for the content identification system. 
             So no more need of a huge shared enum!
*/
#include <string>
#include <set>
#include <deque>
#include <map>

namespace filetypes
{

//================================================================================================
//  Typedef
//================================================================================================
    //Type for the uid assigned to the handles.
    typedef unsigned int cnt_t;

//================================================================================================
//  ContentTy
//================================================================================================
    /*
        ContentTy
            This object represent a content type. Content types are static.

            It acquires a unique content ID number from the content ID manager at construction.
    */
    class ContentTy
    {
    public:
        ContentTy( const std::string  & extension  );
        ContentTy( ContentTy         && other );
        ~ContentTy();
        ContentTy & operator=( ContentTy && other );

        //         operator unsigned int      ()const { return m_id;   }
        //explicit operator unsigned long long()const { return m_id;   }
                 operator cnt_t             ()const { return m_id;   }
        explicit operator std::string       ()const { return m_ext; }

        cnt_t    id  ()const{ return m_id; }
        std::string name()const{ return m_ext; }

    private:
        cnt_t        m_id;
        std::string  m_ext;

        //Never copy this
        ContentTy( const ContentTy &  )            = delete;
        ContentTy & operator=( const ContentTy & ) = delete;
    };

//================================================================================================
//  ContentIDManager
//================================================================================================
    /*
        ContentIDManager
            Manages the attribution of unique ids for the "ContentTy" handles.
    */
    class ContentIDManager
    {
    public:
        static ContentIDManager & GetInstance();
        cnt_t AcquireID( const ContentTy & owner );
        void  ReleaseID( const ContentTy & owner, cnt_t id );

        const ContentTy * FindMatchingCnt( cnt_t id )const;
        const ContentTy * FindMatchingCnt( const std::string & extension )const;

    private:
        ContentIDManager():m_lastgenid(0){}
        ContentIDManager(const ContentIDManager&)               = delete;
        ContentIDManager(ContentIDManager&&)                    = delete;
        ContentIDManager & operator=( const ContentIDManager& ) = delete;
        ContentIDManager & operator=( ContentIDManager && )     = delete;

        cnt_t                            m_lastgenid;
        std::set<cnt_t>                  m_released;
        std::map<cnt_t,const ContentTy*> m_active;
    };

//================================================================================================
//  This content type is always defined first :
//================================================================================================
    extern const ContentTy CnTy_Invalid;

};

#endif
