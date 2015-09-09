#ifndef GAME_STRINGS_HPP
#define GAME_STRINGS_HPP
/*
game_strings.hpp
2015/08/28
psycommando@gmail.com
Description: Utilities for reading/writing the game strings fomat from SIR0 files.
*/
#include <cstdint>
#include <string>
#include <memory>
#include <locale>

namespace pmd3
{
//============================================================================================
//  Typedefs
//============================================================================================
    typedef uint32_t     gstrhash_t; //Represents a unique string hash
    typedef std::string  catstr_t;
    typedef std::wstring gstr_t;


//
//
//
    struct GameStrData
    {
        gstr_t   str;
        uint16_t unk1;
        uint16_t unk2;
    };

//============================================================================================
//  GameStringCatagory
//============================================================================================
    /*
        GameStringsCatagory
            Individual string files are string category. 
    */
    class GameStringsCategory
    {
    public:
        GameStringsCategory( const catstr_t & catname, std::locale & loc );
        //Constructors
        GameStringsCategory( GameStringsCategory && mv );
        GameStringsCategory( const GameStringsCategory & cp );

        GameStringsCategory & operator=( GameStringsCategory && mv );
        GameStringsCategory & operator=( const GameStringsCategory & cp );

        //--- Category ---
        const catstr_t      Name     ()const;
        const std::locale & Locale  ()const;
              size_t        NbStrings()const;

        //--- Strings ---
        //Access a string. Return nullptr if doesn't exist!
        const GameStrData * GetString( gstrhash_t str )const;
              GameStrData * GetString( gstrhash_t str );

         //Provide a way to access strings via index directly
         const std::pair<const gstrhash_t, GameStrData> & GetStringByIndex( size_t index )const;
               std::pair<const gstrhash_t, GameStrData> & GetStringByIndex( size_t index );

        //Replace existing string with content
        void                SetString( gstrhash_t str, const GameStrData & newstr );

        //Add a brand new string. Should be done through the Game string pool to keep hashes unique.
        void                AddString( gstrhash_t str, const GameStrData & newstr );

    private:
        class GameStringsCategoryImpl;
        std::unique_ptr<GameStringsCategoryImpl> m_pimpl;
    };


//============================================================================================
//  GameStringsPool
//============================================================================================
    /*
        GameStringsPool
            A collection of string categories.
            Also handles unique hash generation, and managment.
    */
    class GameStringsPool
    {
    public:
        GameStringsPool( std::locale loc );

        //Constructors
        GameStringsPool( const GameStringsPool & cp );
        GameStringsPool( GameStringsPool      && mv );

        GameStringsPool & operator=( const GameStringsPool & cp );
        GameStringsPool & operator=( GameStringsPool      && mv );

        //Writing
        void WriteStringPool( const std::string & messdir );

        //Loading
        void LoadStringPool( const std::string & messdir );

        //Access
        const GameStrData * GetString( gstrhash_t str )const;
              GameStrData * GetString( gstrhash_t str );

        const GameStringsCategory & GetCategory( size_t catind )const;
              GameStringsCategory & GetCategory( size_t catind );

        const std::pair<bool, const GameStringsCategory&> GetCategory( const catstr_t & name )const;
              std::pair<bool, GameStringsCategory&>       GetCategory( const catstr_t & name );

        const std::pair<bool,size_t> GetCategoryIndex( const catstr_t & name )const;
              std::pair<bool,size_t> GetCategoryIndex( const catstr_t & name );

        const GameStringsCategory * GetCategoryByStrHash( gstrhash_t strhash )const;
              GameStringsCategory * GetCategoryByStrHash( gstrhash_t strhash );

        size_t GetTotalNbStrings()const;
        size_t GetNbCategories  ()const;

        //Modify Strings
        gstrhash_t            AddString  ( const GameStrData & str, const catstr_t & cat );
        bool                  RemString  ( gstrhash_t str );

        //Modify Categories
        GameStringsCategory & AddCategory( const catstr_t & cat );
        bool                  RemCategory( const catstr_t & cat );
        bool                  RemCategory( size_t           catindex );

        //Export
        void ExportXML ( const std::string & filepath );

        //Import
        void ImportXML ( const std::string & filepath );

    private:
        class GameStringsPoolImpl;
        std::unique_ptr<GameStringsPoolImpl> m_pimpl;
    };

};

#endif