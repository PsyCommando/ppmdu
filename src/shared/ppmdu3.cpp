#include "ppmdu3.hpp"
#include <ppmdu/pmd3/game_strings.hpp>
#include <iostream>
#include <iomanip>
#include <map>
#include <set>
#include <memory>
using namespace std;

namespace ppmdu3
{
    static const size_t MaxStrLen = 512;
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
            ppmdu3_GetStringByHash
                Returns a loaded string from the specified GameStringsPool object.

                - hndl : The GamesStringsPool object to load from.
                - strh : The unique Hash of the string.
        */
        PPMDU_API int ppmdu3_GetStringByHash( ppmdu3_gstrplhndl hndl, ppmdu3_strhash strh, wchar_t * out_str, size_t strmaxlen )
        {
            try
            {
                pmd3::GameStringsPool * ptr = nullptr;
                if( !AcquireStringPool( hndl, ptr ) )
                    return -1;

                if( strmaxlen < MaxStrLen )
                {
                    const pmd3::gstr_t * fstr = ptr->GetString( strh );
                    if( fstr != nullptr && fstr->length() < strmaxlen )
                    {
                        wcscpy( out_str, fstr->c_str() );
                    }
                    else
                        return -1;
                }
                else
                    return -1;
            }
            catch(std::exception & e)
            {
                cerr <<"Exception caught (ppmdu3_GetStringByHash): " << e.what() <<"\n";
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
        PPMDU_API int ppmdu3_GetStringByCategoryAndIndex( ppmdu3_gstrplhndl hndl, ppmdu3_index catindex, ppmdu3_index strindex, wchar_t * out_str, size_t strmaxlen )
        {
            try
            {
                pmd3::GameStringsPool * ptr = nullptr;
                if( !AcquireStringPool( hndl, ptr ) )
                    return -1;

                if( strmaxlen < MaxStrLen )
                {
                    const pmd3::gstr_t & fstr = ptr->GetCategory(catindex).GetStringByIndex(strindex);
                    if( fstr.length() < strmaxlen )
                    {
                        wcscpy( out_str, fstr.c_str() );
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
        PPMDU_API size_t GetNbCategoriesLoaded( ppmdu3_gstrplhndl hndl );

        /*
        */
        PPMDU_API size_t GetNbStringsLoaded( ppmdu3_gstrplhndl hndl, ppmdu3_index catindex );


        //------------------ Modification ------------------


        PPMDU_API int ppmdu3_SetStringByHash( ppmdu3_gstrplhndl hndl, ppmdu3_strhash strh, const wchar_t * in_str, size_t strl );

        PPMDU_API int ppmdu3_SetStringByIndex( ppmdu3_gstrplhndl hndl,  ppmdu3_index catindex,  ppmdu3_index strindex, const wchar_t * in_str, size_t strl );

        /*
            ppmdu3_AddString
                Returns hash of the new string, or 0 if it fails.
        */
        PPMDU_API ppmdu3_strhash ppmdu3_AddString( ppmdu3_gstrplhndl hndl, ppmdu3_index catindex,  const wchar_t * in_str, size_t strl );

        /*
            ppmdu3_RemString
                Returns 0 if it fails.
        */
        PPMDU_API int ppmdu3_RemString( ppmdu3_gstrplhndl hndl, ppmdu3_strhash strh );

        /*
            ppmdu3_AddString
                Returns index of the new category + 1, or 0 if it fails.
        */
        PPMDU_API size_t ppmdu3_AddCategory( ppmdu3_gstrplhndl hndl,  const wchar_t * in_str, size_t strl );

        /*
            ppmdu3_RemString
                Returns hash of the new string, or 0 if it fails.
        */
        PPMDU_API int ppmdu3_RemCategory( ppmdu3_gstrplhndl hndl, ppmdu3_index catindex );
    };


};