#include "ppmdu3.hpp"
#include <ppmdu/pmd3/game_strings.hpp>
#include <iostream>
#include <iomanip>
#include <map>
#include <set>
#include <memory>
#include <cassert>
using namespace std;

namespace ppmdu3
{
    static const size_t MaxStrLen = 512;

    int c16strcpy( char16_t * dest, const char16_t * src, size_t maxlen )
    {
        //Count source string length
        size_t cnt = 0;
        for( ;cnt < maxlen; ++cnt )
        {
            char16_t c = *(src+cnt);
            *(dest+cnt) = c;

            if( c == 0 ) 
                break;
        }

        if(cnt >= maxlen)
            return -1;
        else
            return cnt;
    }

//
//  GameStrings
//
    /*
        This handles allocating game strings pool objects.
    */
    class GameStringsStateManager
    {
    public:
        static GameStringsStateManager & Instance()
        {
            static GameStringsStateManager s_instance;
            return s_instance;
        }


        GameStringsStateManager()
            :m_lasthndl(1) //start at 1, not 0
        {
        }

        ~GameStringsStateManager()
        {
        }

        std::locale GetMyLocale()
        {
            return std::locale::classic();
        }

        gametext::ppmdu3_gstrplhndl CreateGStrPool()
        {
            gametext::ppmdu3_gstrplhndl newhndl = 0;
            
            if( !m_releasedhndl.empty() )
            {
                newhndl = *m_releasedhndl.end();
                m_releasedhndl.erase( m_releasedhndl.end() );
            }
            else
            {
                ++m_lasthndl;
                newhndl = m_lasthndl;
            }

            m_strpools.emplace( newhndl, std::unique_ptr<pmd3::GameStringsPool>( new pmd3::GameStringsPool( std::move(GetMyLocale()) ) ) );
        }

        pmd3::GameStringsPool * GetGStrPool( gametext::ppmdu3_gstrplhndl hndl )
        {
            auto result = m_strpools.find( hndl );
            
            if( result != m_strpools.end() )
                return result->second.get();
            else 
                return nullptr;
        }

        void RemoveGStrPool( gametext::ppmdu3_gstrplhndl hndl )
        {
            m_strpools.erase( hndl );
        }

    private:
        gametext::ppmdu3_gstrplhndl                                         m_lasthndl;

        map<gametext::ppmdu3_gstrplhndl, unique_ptr<pmd3::GameStringsPool>> m_strpools;
        set<gametext::ppmdu3_gstrplhndl>                                    m_releasedhndl;
    };



    namespace gametext
    {
        bool AcquireStringPool( ppmdu3_gstrplhndl hndl, pmd3::GameStringsPool * ptr )
        {
            ptr = GameStringsStateManager::Instance().GetGStrPool(hndl);
                
            if( ptr == nullptr )
                return false;
            else
                return true;
        }

        //------------------ Alloc ------------------
        /*
            ppmdu3_CreateGameStringsPool
                Instantiate a GameStringsPool object.

                GameStringsPool objects are automatically destroyed when the library is unloaded, 
                but you can control its lifetime more accurately by usingg the 
                ppmdu3_ReleaseGameStringsPool function to delete the object.
        */
        PPMDU_API int ppmdu3_CreateGameStringsPool( ppmdu3_gstrplhndl & out_hndl )
        {
            try
            {
                out_hndl = GameStringsStateManager::Instance().CreateGStrPool();
                return 0;
            }
            catch(std::exception & e)
            {
                cerr <<"Exception caught (ppmdu3_CreateGameStringsPool): " << e.what() <<"\n";
                return -1;
            }
        }

        /*
            ppmdu3_ReleaseGameStringsPool
                Delete a GameStringsPool object.
        */
        PPMDU_API int ppmdu3_ReleaseGameStringsPool( ppmdu3_gstrplhndl hndl )
        {
            try
            {
                GameStringsStateManager::Instance().RemoveGStrPool(hndl);
                return 0;
            }
            catch(std::exception & e)
            {
                cerr <<"Exception caught (ppmdu3_ReleaseGameStringsPool): " << e.what() <<"\n";
                return -1;
            }
        }

        //------------------ IO ------------------
        /*
            ppmdu3_LoadGameStrings
                If returns 0, success, otherwise fail..

                - hndl : The GameStringsPool object to use.
                - path : Path to the "Message" directory where all game strings are stored.
        */
        PPMDU_API int ppmdu3_LoadGameStrings( ppmdu3_gstrplhndl hndl, const char * path, size_t strl )
        {
            try
            {
                pmd3::GameStringsPool * ptr = nullptr;
                if( !AcquireStringPool( hndl, ptr ) )
                    return -1;

                if( strl < MaxStrLen )
                    ptr->LoadStringPool( string(path, path + strl) );
                else
                    return -1;
            }
            catch(std::exception & e)
            {
                cerr <<"Exception caught (ppmdu3_LoadGameStrings): " << e.what() <<"\n";
                return -1;
            }
            return 0;
        }

        /*
            ppmdu3_WriteGameStrings
                If returns 0, success, otherwise fail..

                - hndl : The GameStringsPool object to use.
                - path : Path to the "Message" directory where all game strings are stored.
        */
        PPMDU_API int ppmdu3_WriteGameStrings( ppmdu3_gstrplhndl hndl, const char * path, size_t strl )
        {
            try
            {
                pmd3::GameStringsPool * ptr = nullptr;
                if( !AcquireStringPool( hndl, ptr ) )
                    return -1;

                if( strl < MaxStrLen )
                    ptr->WriteStringPool( string(path, path + strl) );
                else
                    return -1;
            }
            catch(std::exception & e)
            {
                cerr <<"Exception caught (ppmdu3_WriteGameStrings): " << e.what() <<"\n";
                return -1;
            }
            return 0;
        }


        //------------------ Access ------------------
        /*
            ppmdu3_GetStringByUUID
                Returns a loaded string from the specified GameStringsPool object.

                - hndl : The GamesStringsPool object to load from.
                - strh : The unique uuid of the string.
        */
        PPMDU_API int ppmdu3_GetStringByUUID( ppmdu3_gstrplhndl hndl, ppmdu3_struuid strh, char16_t * out_str, size_t strmaxlen )
        {
            try
            {
                pmd3::GameStringsPool * ptr = nullptr;
                if( !AcquireStringPool( hndl, ptr ) )
                    return -1;

                if( strmaxlen < MaxStrLen )
                {
                    const pmd3::GameStrData * pstrdat = ptr->GetString( strh );
                    if( pstrdat != nullptr && (pstrdat->str.length() + 1) < strmaxlen )
                    {
                        if( !c16strcpy( out_str, pstrdat->str.c_str(), strmaxlen ) )
                            return -1;

                        //for( auto achar : pstrdat->str )
                        //{
                        //    (*out_str) = achar;
                        //    ++out_str;
                        //}

                        ////Append terminating 0
                        //(*out_str) = '\0';
                        //++out_str;
                    }
                    else
                    {
                        //The strmaxlen is probably too small
                        return -1;
                    }
                }
                else
                    return -1;
            }
            catch(std::exception & e)
            {
                cerr <<"Exception caught (ppmdu3_GetStringByuuid): " << e.what() <<"\n";
                return -1;
            }
            return 0;
        }

        /*
            ppmdu3_GetStringByCategoryAndIndex
                Returns a loaded string from the specified GameStringsPool object, 
                from the specified category(name of the file it was loaded from), and at the given index within said category!

                - hndl     : The handle to the GameStringsPool.
                - catindex : The index of the category to look into.
                - strindex : The index of the string within the category.
        */
        PPMDU_API int ppmdu3_GetStringByCategoryAndIndex( ppmdu3_gstrplhndl hndl, ppmdu3_index catindex, ppmdu3_index strindex, char16_t * out_str, size_t strmaxlen )
        {
            try
            {
                pmd3::GameStringsPool * ptr = nullptr;
                if( !AcquireStringPool( hndl, ptr ) )
                    return -1;

                if( strmaxlen < MaxStrLen )
                {
                    const auto & strdat = ptr->GetCategory(catindex).GetStringByIndex(strindex);

                    if( strdat.second.str.length() < strmaxlen )
                    {
                        if( !c16strcpy( out_str, strdat.second.str.c_str(), strmaxlen ) )
                            return -1;
                        //wcscpy( out_str, strdat.second.str.c_str() );
                    }
                    else
                        return -1;
                }
                else
                    return -1;
            }
            catch(std::exception & e)
            {
                cerr <<"Exception caught (ppmdu3_GetStringByCategoryAndIndex): " << e.what() <<"\n";
                return -1;
            }
            return 0;
        }

        /*
        */
        PPMDU_API int GetCategoryName( ppmdu3_gstrplhndl hndl, ppmdu3_index catindex, char * out_str, size_t strmaxlen )
        {
            try
            {
                pmd3::GameStringsPool * ptr = nullptr;
                if( !AcquireStringPool( hndl, ptr ) )
                    return -1;

                if( strmaxlen < MaxStrLen )
                {
                    const pmd3::GameStringsCategory & mycat = ptr->GetCategory(catindex);
                    if( mycat.Name().length() < strmaxlen )
                    {
                        strcpy_s( out_str, mycat.Name().length(), mycat.Name().c_str() );
                    }
                    else
                        return -1;
                }
                else
                    return -1;
            }
            catch(std::exception & e)
            {
                cerr <<"Exception caught (GetCategoryName): " << e.what() <<"\n";
                return -1;
            }
            return 0;
        }

        /*
        */
        PPMDU_API int FindCategoryByName( ppmdu3_gstrplhndl hndl, const char * in_str, size_t strl, ppmdu3_index & out_catindex )
        {
            try
            {
                pmd3::GameStringsPool * ptr = nullptr;
                if( !AcquireStringPool( hndl, ptr ) )
                    return -1;

                if( strl < MaxStrLen )
                {
                    auto res = ptr->GetCategoryIndex( string(in_str, in_str + strl) );
                    if( res.first )
                    {
                        out_catindex = res.second;
                    }
                    else
                        return -1;
                }
                else
                    return -1;
            }
            catch(std::exception & e)
            {
                cerr <<"Exception caught (FindCategoryByName): " << e.what() <<"\n";
                return -1;
            }
            return 0;
        }

        /*
        */
        PPMDU_API size_t GetNbCategoriesLoaded( ppmdu3_gstrplhndl hndl )
        {
            try
            {
                pmd3::GameStringsPool * ptr = nullptr;
                if( !AcquireStringPool( hndl, ptr ) )
                    return 0;
                else
                    return ptr->GetNbCategories();
            }
            catch(std::exception & e)
            {
                cerr <<"Exception caught (GetNbCategoriesLoaded): " << e.what() <<"\n";
                return 0;
            }
        }

        /*
        */
        PPMDU_API size_t GetNbStringsLoaded( ppmdu3_gstrplhndl hndl, ppmdu3_index catindex )
        {
            try
            {
                pmd3::GameStringsPool * ptr = nullptr;

                if( AcquireStringPool( hndl, ptr ) && catindex < ptr->GetNbCategories() )
                    return ptr->GetCategory(catindex).NbStrings();
                else
                    return 0;
            }
            catch(std::exception & e)
            {
                cerr <<"Exception caught (GetNbStringsLoaded): " << e.what() <<"\n";
                return 0;
            }
        }


        //------------------ Modification ------------------


        PPMDU_API int ppmdu3_SetStringByUUID( ppmdu3_gstrplhndl hndl, ppmdu3_struuid strh, const char16_t * in_str, size_t strl )
        {
            assert(false);
            return -1;
        }

        PPMDU_API int ppmdu3_SetStringByIndex( ppmdu3_gstrplhndl hndl,  ppmdu3_index catindex,  ppmdu3_index strindex, const char16_t * in_str, size_t strl )
        {
            assert(false);
            return -1;
        }

        /*
            ppmdu3_AddString
                Returns uuid of the new string, or 0 if it fails.
        */
        PPMDU_API ppmdu3_struuid ppmdu3_AddString( ppmdu3_gstrplhndl hndl, ppmdu3_index catindex,  const char16_t * in_str, size_t strl )
        {
            assert(false);
            return 0;
        }

        /*
            ppmdu3_RemString
                Returns 0 if it fails.
        */
        PPMDU_API int ppmdu3_RemString( ppmdu3_gstrplhndl hndl, ppmdu3_struuid strh )
        {
            assert(false);
            return -1;
        }

        /*
            ppmdu3_AddString
                Returns index of the new category + 1, or 0 if it fails.
        */
        PPMDU_API size_t ppmdu3_AddCategory( ppmdu3_gstrplhndl hndl,  const char * in_str, size_t strl )
        {
            assert(false);
            return 0;
        }

        /*
            ppmdu3_RemString
                Returns uuid of the new string, or 0 if it fails.
        */
        PPMDU_API int ppmdu3_RemCategory( ppmdu3_gstrplhndl hndl, ppmdu3_index catindex )
        {
            assert(false);
            return -1;
        }
    };


};