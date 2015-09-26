#include "game_strings.hpp"
#include <utils/library_wide.hpp>
#include <utils/uuid_gen_wrapper.hpp>
#include <string>
#include <map>
#include <locale>
#include <cassert>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;

namespace pmd3
{

//============================================================================================
//  GameStringsCategoryImpl
//============================================================================================
    /*
        GameStringsCategoryImpl
            Individual string files are string category. 
    */
    class GameStringsCategory::GameStringsCategoryImpl
    {
    public:

        GameStringsCategoryImpl( const catstr_t & catname/*, std::locale & loc*/ )
            :/*m_loc(loc),*/ m_name(catname)
        {}

        //locale                      m_loc;
        catstr_t                    m_name;
        map<gstruuid_t,GameStrData> m_strs;
    };


//============================================================================================
//  GameStringCatagory
//============================================================================================
    GameStringsCategory::GameStringsCategory( const catstr_t & catname/*, std::locale & loc*/ )
        :m_pimpl(new GameStringsCategoryImpl(catname/*, loc*/))
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

    GameStringsCategory::~GameStringsCategory()
    {}

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

    //const std::locale & GameStringsCategory::Locale()const
    //{
    //    return m_pimpl->m_loc;
    //}

    size_t GameStringsCategory::NbStrings()const
    {
        return m_pimpl->m_strs.size();
    }

    //--- Strings ---
    //Access a string. Return nullptr if doesn't exist!
    const GameStrData * GameStringsCategory::GetString( gstruuid_t str )const
    {
        return const_cast<GameStringsCategory*>(this)->GetString(str);
    }

    GameStrData * GameStringsCategory::GetString( gstruuid_t str )
    {
        auto found = m_pimpl->m_strs.find( str );

        if( found != m_pimpl->m_strs.end() )
            return &(found->second);
        else
            return nullptr;
    }

    //Replace existing string with content
    void GameStringsCategory::SetString( gstruuid_t str, const GameStrData & newstr )
    {
        GameStrData * pstr = GetString(str);

        if( pstr != nullptr )
            (*pstr) = newstr;
        else
            AddString( str, newstr );
    }

    //Add a brand new string. Should be done through the Game string pool to keep uuides unique.
    void GameStringsCategory::AddString( gstruuid_t str, const GameStrData & newstr )
    {
        m_pimpl->m_strs.emplace( str, newstr );
    }

    bool GameStringsCategory::RemString( gstruuid_t uuid )
    {
        GameStringsCategoryImpl & impl = *m_pimpl;
        auto itfound = impl.m_strs.find(uuid);

        if( itfound != impl.m_strs.end() )
        {
            impl.m_strs.erase(itfound);
            return true;
        }
        else
            return false;
    }



//============================================================================================
//  GameStringsPoolImpl
//============================================================================================
    /*
        GameStringsPoolImpl
            A collection of string categories.
            Also handles unique uuid generation, and managment.
    */
    class GameStringsPool::GameStringsPoolImpl
    {
    public:
        GameStringsPoolImpl( /*std::locale && loc*/ )
            /*:m_loc(loc)*/
        {}

        gstruuid_t AddString( const GameStrData & str, const catstr_t & cat )
        {
            GameStringsCategory * pcat = GetCategoryByName(cat);

            //Generate uuid
            gstruuid_t uuid = utils::GenerateUUID();

            if( utils::LibWide().isLogOn() )
            {
               wclog << "Added new string \"";
               for( auto c : str.str )
                   wclog<<"\\x" <<hex <<uppercase <<static_cast<uint16_t>(c) <<dec <<nouppercase;
               wclog <<"\" with UUID:" <<hex <<uppercase <<uuid <<dec <<nouppercase << " !";
            }

            //Assign
            pcat->AddString( uuid, str );
        }


        void RemString( gstruuid_t struuid )
        {
            for( auto & category : m_cats )
            {
                GameStrData * pfound = category.second.GetString(struuid);
                if( pfound != nullptr )
                    category.second.RemString( struuid );
            }
        }

        GameStringsCategory * GetCategoryByName( const catstr_t & name )
        {
            auto itfound = m_cats.find( name );

            if( itfound != m_cats.end() )
                return &(itfound->second);
            else
                return nullptr;
        }

        GameStrData * GetString( gstruuid_t struuid )
        {
            for( auto & category : m_cats )
            {
                GameStrData * pfound = category.second.GetString(struuid);
                if( pfound != nullptr )
                    return pfound;
            }
            return nullptr;
        }

        void AddCategory( GameStringsCategory && newcat )
        {
            //Add category
            catstr_t catname = newcat.Name(); //Make a copy, because its not guaranteed the arguments will be parsed in the desired order
            m_cats.emplace( std::move(catname), std::move(newcat) );
        }

        void RemCategory( const catstr_t & name )
        {
            m_cats.erase( name );
        }

        GameStringsCategory * GetCategoryByStrUUID( gstruuid_t struuid )
        {
            for( auto & category : m_cats )
            {
                GameStrData * pfound = category.second.GetString(struuid);
                if( pfound != nullptr )
                    return &(category.second);
            }

            return nullptr;
        }

        size_t CalcTotalNbStrings()
        {
            size_t total = 0;
            for( auto & category : m_cats )
                total += category.second.NbStrings();

            return total;
        }

        //std::locale                        m_loc;
        map<catstr_t, GameStringsCategory> m_cats;
    };


//============================================================================================
//  GameStringsPool
//============================================================================================
    GameStringsPool::GameStringsPool(/* std::locale loc*/ )
        :m_pimpl(std::make_unique<GameStringsPoolImpl>(/*std::move(loc)*/))
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

    GameStringsPool::~GameStringsPool()
    {}

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
    const GameStrData * GameStringsPool::GetString( gstruuid_t str )const
    {
        //Search through our internal reference table
        return const_cast<GameStringsPool*>(this)->GetString(str);
    }

    GameStrData * GameStringsPool::GetString( gstruuid_t str )
    {
        return m_pimpl->GetString(str);
    }

    const GameStringsCategory * GameStringsPool::GetCategoryByStrUUID( gstruuid_t struuid )const
    {
        return const_cast<GameStringsPool*>(this)->GetCategoryByStrUUID(struuid);
    }

    GameStringsCategory * GameStringsPool::GetCategoryByStrUUID( gstruuid_t struuid )
    {
        return m_pimpl->GetCategoryByStrUUID(struuid);
    }

    size_t GameStringsPool::CalcTotalNbStrings()const
    {
        return m_pimpl->CalcTotalNbStrings();
    }

    size_t GameStringsPool::GetNbCategories()const
    {
        return m_pimpl->m_cats.size();
    }

    //Modify Strings
    gstruuid_t GameStringsPool::AddString  ( const GameStrData & str, const catstr_t & cat )
    {
        return m_pimpl->AddString( str, cat );
    }

    void GameStringsPool::RemString  ( gstruuid_t str )
    {
        m_pimpl->RemString(str);
    }

    //Modify Categories
    void GameStringsPool::AddCategory( const catstr_t & cat )
    {
        //auto & impl = *m_pimpl;
        //return impl.m_cats[ impl.AddCategory( GameStringsCategory( cat, impl.m_loc ) ) ];
        m_pimpl->AddCategory( GameStringsCategory( cat/*, m_pimpl->m_loc*/ ) );
    }

    void GameStringsPool::RemCategory( const catstr_t & cat )
    {
        m_pimpl->RemCategory(cat);
    }

//============================================================================================
//  Input/Output
//============================================================================================

    //Writing
    void GameStringsPool::WriteStringPool( const std::string & messdir )
    {
    }

    //Loading
    void GameStringsPool::LoadStringPool( const std::string & messdir )
    {
    }


};