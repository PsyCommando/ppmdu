#include "game_strings.hpp"
#include <utils/library_wide.hpp>
#include <string>
#include <map>
#include <locale>
#include <cassert>
#include <vector>

using namespace std;

namespace pmd3
{


//============================================================================================
//  GameStringCatagory
//============================================================================================
    /*
        GameStringsCategoryImpl
            Individual string files are string category. 
    */
    class GameStringsCategory::GameStringsCategoryImpl
    {
    public:

        GameStringsCategoryImpl( const catstr_t & catname, std::locale & loc )
            :m_loc(loc), m_name(catname)
        {}

        locale                      m_loc;
        catstr_t                    m_name;
        map<gstrhash_t,GameStrData> m_strs;
    };



    GameStringsCategory::GameStringsCategory( const catstr_t & catname, std::locale & loc )
        :m_pimpl(new GameStringsCategoryImpl(catname, loc))
    {}

    //Constructors
    GameStringsCategory::GameStringsCategory( GameStringsCategory && mv )
    {
        m_pimpl = move(mv.m_pimpl);
    }

    GameStringsCategory::GameStringsCategory( const GameStringsCategory & cp )
    {
        m_pimpl.reset( new GameStringsCategoryImpl(*cp.m_pimpl) );
    }

    GameStringsCategory & GameStringsCategory::operator=( GameStringsCategory && mv )
    {
        m_pimpl = move(mv.m_pimpl);
        return *this;
    }

    GameStringsCategory & GameStringsCategory::operator=( const GameStringsCategory & cp )
    {
        m_pimpl.reset( new GameStringsCategoryImpl(*cp.m_pimpl) );
        return *this;
    }

    //--- Category ---
    const catstr_t GameStringsCategory::Name()const
    {
        return m_pimpl->m_name;
    }

    const std::locale & GameStringsCategory::Locale()const
    {
        return m_pimpl->m_loc;
    }

    size_t GameStringsCategory::NbStrings()const
    {
        return m_pimpl->m_strs.size();
    }

    //--- Strings ---
    //Access a string. Return nullptr if doesn't exist!
    const GameStrData * GameStringsCategory::GetString( gstrhash_t str )const
    {
        return const_cast<GameStringsCategory*>(this)->GetString(str);
    }

    GameStrData * GameStringsCategory::GetString( gstrhash_t str )
    {
        auto found = m_pimpl->m_strs.find( str );

        if( found != m_pimpl->m_strs.end() )
            return &(found->second);
        else
            return nullptr;
    }

        //Provide a way to access strings via index directly
    const std::pair<const gstrhash_t, GameStrData> & GameStringsCategory::GetStringByIndex( size_t index )const
    {
        return const_cast<GameStringsCategory*>(this)->GetStringByIndex(index);
    }

    std::pair<const gstrhash_t, GameStrData> & GameStringsCategory::GetStringByIndex( size_t index )
    {
        if( index >= m_pimpl->m_strs.size()  )
            throw out_of_range("GameStringsCategory::GetStringByIndex(" + to_string(index) + "): Index out of range!");

        auto itfetch = (m_pimpl->m_strs.begin());
        advance( itfetch, index );
        return (*itfetch);
    }

    //Replace existing string with content
    void GameStringsCategory::SetString( gstrhash_t str, const GameStrData & newstr )
    {
        GameStrData * pstr = GetString(str);

        if( pstr != nullptr )
            (*pstr) = newstr;
        else
            AddString( str, newstr );
    }

    //Add a brand new string. Should be done through the Game string pool to keep hashes unique.
    void GameStringsCategory::AddString( gstrhash_t str, const GameStrData & newstr )
    {
        m_pimpl->m_strs.emplace( str, newstr );
    }


//============================================================================================
//  GameStringsPool
//============================================================================================
    /*
        GameStringsPoolImpl
            A collection of string categories.
            Also handles unique hash generation, and managment.
    */
    class GameStringsPool::GameStringsPoolImpl
    {
    public:
        GameStringsPoolImpl( std::locale && loc )
            :m_loc(loc), m_totalstrs(0)
        {}

        gstrhash_t AddString( const GameStrData & str, const catstr_t & cat )
        {
            GameStringsCategory * pcat = GetCategoryByName(cat);

            //Generate hash
            gstrhash_t hash = 0;
            assert(false); //implement me!

            //Assign
            pcat->AddString( hash, str );

            //Increment string count 
            ++m_totalstrs;
        }

        GameStringsCategory * GetCategoryByName( const catstr_t & name )
        {
            const size_t nbstrings = m_cats.size();
            auto       & cats      = m_cats;

            for( size_t i = 0; i < nbstrings; ++i )
                if( cats[i].Name() == name ) return &(cats[i]);

            return nullptr;
        }

        GameStrData * GetString( gstrhash_t strh )
        {
            auto itfound = m_uidtocat.find(strh);

            if( itfound != m_uidtocat.end() )
            {
                return m_cats[itfound->second].GetString(strh);
            }
            else
                return nullptr;
        }

        size_t AddCategory( GameStringsCategory && newcat )
        {
            //Add category
            m_cats.push_back(newcat);

            //Add category's strings
            const size_t nbstrings = m_cats.back().NbStrings();
            const size_t indexcat  = m_cats.size()-1;
            
            for( size_t i = 0; i < nbstrings; ++i )
                m_uidtocat.emplace( m_cats.back().GetStringByIndex(i).first, indexcat );

            m_totalstrs += nbstrings;

            return indexcat; //We return the index of the new category
        }

        void RemCategory( size_t catind )
        {
            if( catind >= m_cats.size() )
            {
                utils::LogError("GameStringsPool::GameStringsPoolImpl::RemCategory():Category index out of range!");
            //    throw out_of_range("GameStringsPool::GameStringsPoolImpl::RemCategory():Category index out of range!");
                assert(false);
            }

            //Remove category's strings
            auto & curcat = m_cats[catind];
            
            for( size_t i = 0; i < curcat.NbStrings(); ++i )
                m_uidtocat.erase( curcat.GetStringByIndex(i).first );

            m_totalstrs -= curcat.NbStrings();

            //Remove category
            m_cats.erase( m_cats.begin() + catind );
        }


        std::locale                  m_loc;
        map<gstrhash_t, size_t>      m_uidtocat; //List of all known string UID, and the category they belong to!
        vector<GameStringsCategory>  m_cats;     //This is where categories are stored!
        size_t                       m_totalstrs;
    };

    GameStringsPool::GameStringsPool( std::locale loc )
        :m_pimpl(std::make_unique<GameStringsPoolImpl>(std::move(loc)))
    {}

    GameStringsPool::GameStringsPool( const GameStringsPool & cp )
    {
        m_pimpl.reset( new GameStringsPoolImpl(*cp.m_pimpl) );
    }

    GameStringsPool::GameStringsPool( GameStringsPool && mv )
    {
        m_pimpl.reset(mv.m_pimpl.release());
        mv.m_pimpl = nullptr;
    }

    GameStringsPool & GameStringsPool::operator=( const GameStringsPool & cp )
    {
        m_pimpl.reset( new GameStringsPoolImpl(*cp.m_pimpl) );
        return *this;
    }

    GameStringsPool & GameStringsPool::operator=( GameStringsPool && mv )
    {
        m_pimpl.reset(mv.m_pimpl.release());
        mv.m_pimpl = nullptr;
        return *this;
    }

    //Access
    const GameStrData * GameStringsPool::GetString( gstrhash_t str )const
    {
        //Search through our internal reference table
        return const_cast<GameStringsPool*>(this)->GetString(str);
    }

    GameStrData * GameStringsPool::GetString( gstrhash_t str )
    {
        return m_pimpl->GetString(str);
    }

    const GameStringsCategory & GameStringsPool::GetCategory( size_t catind )const
    {
        return const_cast<GameStringsPool*>(this)->GetCategory(catind);
    }

    GameStringsCategory & GameStringsPool::GetCategory( size_t catind )
    {
        return m_pimpl->m_cats[catind];
    }

    //#FIXME: The pointer returned is a bit unsafe. Considering the category is contained in a vector!
    //const std::pair<bool, const GameStringsCategory&> GameStringsPool::GetCategory( const catstr_t & name )const
    //{
    //}

    ////#FIXME: The pointer returned is a bit unsafe. Considering the category is contained in a vector!
    //std::pair<bool, GameStringsCategory&> GameStringsPool::GetCategory( const catstr_t & name )
    //{
    //    for( auto & acat : m_pimpl->m_cats )
    //    {
    //        if( acat.Name() == name )
    //            return make_pair( true, acat );
    //    }
    //    return make_pair( false,  );
    //}

    const std::pair<bool,size_t> GameStringsPool::GetCategoryIndex( const catstr_t & name )const
    {
        return const_cast<GameStringsPool*>(this)->GetCategoryIndex(name);
    }

    std::pair<bool,size_t> GameStringsPool::GetCategoryIndex( const catstr_t & name )
    {
        const size_t nbstrings = m_pimpl->m_cats.size();
        auto       & cats      = m_pimpl->m_cats;

        for( size_t i = 0; i < nbstrings; ++i )
            if( cats[i].Name() == name ) return std::move( make_pair( true, i ) );

        return std::move( make_pair( false, 0 ) );
    }

    const GameStringsCategory * GameStringsPool::GetCategoryByStrHash( gstrhash_t strhash )const
    {
        return const_cast<GameStringsPool*>(this)->GetCategoryByStrHash(strhash);
    }

    GameStringsCategory * GameStringsPool::GetCategoryByStrHash( gstrhash_t strhash )
    {
        auto itfound = m_pimpl->m_uidtocat.find(strhash);

        if( itfound != m_pimpl->m_uidtocat.end() )
            return &(m_pimpl->m_cats[itfound->second]);
        else
            return nullptr;
    }

    size_t GameStringsPool::GetTotalNbStrings()const
    {
        return m_pimpl->m_totalstrs;
    }

    size_t GameStringsPool::GetNbCategories  ()const
    {
        return m_pimpl->m_cats.size();
    }

    //Modify Strings
    gstrhash_t GameStringsPool::AddString  ( const GameStrData & str, const catstr_t & cat )
    {
    }

    bool GameStringsPool::RemString  ( gstrhash_t str )
    {
    }

    //Modify Categories
    GameStringsCategory & GameStringsPool::AddCategory( const catstr_t & cat )
    {
    }

    bool GameStringsPool::RemCategory( const catstr_t & cat )
    {
    }

    bool GameStringsPool::RemCategory( size_t catindex )
    {
    }

//============================================================================================
//  Input/Output
//============================================================================================

    //Export
    void GameStringsPool::ExportXML ( const std::string & filepath )
    {
    }

    //Import
    void GameStringsPool::ImportXML ( const std::string & filepath )
    {
    }

    //Writing
    void GameStringsPool::WriteStringPool( const std::string & messdir )
    {
    }

    //Loading
    void GameStringsPool::LoadStringPool( const std::string & messdir )
    {
    }


};