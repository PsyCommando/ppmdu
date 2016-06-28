#ifndef PMD2_LANGCONF_HPP
#define PMD2_LANGCONF_HPP
/*
pmd2_langconf.hpp
2015/09/11
psycommando@gmail.com
Description:
    Loads the language configuration file. 
    Basically, an XML file containing important details on how to handle localized data in a 
    specific version + language game.
*/
#include <string>
#include <cstdint>
#include <vector>
#include <map>
#include <array>
#include <ppmdu/pmd2/pmd2.hpp>

namespace pmd2
{

    /*
        Enum for associating the values of the StrBlocksNames array below.

        **NEED TO KEEP THIS IN SYNC WITH StrBlocksNames**
    */
    enum struct eStrBNames : unsigned int
    {
        PkmnNames = 0,
        PkmnCats,
        MvNames,
        MvDesc,
        ItemNames,
        ItemDescS,
        ItemDescL,
        AbilityNames,
        AbilityDesc,
        TypeNames,
        PortraitNames,

        //Add new string types above!
        NBEntries,
    };

    extern const std::array<std::string, static_cast<uint32_t>(eStrBNames::NBEntries)> StrBlocksNames;

//==================================================================================
//   StringsCatalog
//==================================================================================
    /*
        Contains the list of the offsets of all the string blocks that could be found.
    */
    class StringsCatalog
    {
    public:

        /*
            Boundaries for strings
        */
        struct strbounds_t
        {
            uint32_t beg;
            uint32_t end;
        };

    public:

        const std::string GetLocaleString()const
        {
            return m_localeStr;
        }

        void SetLocaleString( const std::string & locstr )
        {
            m_localeStr = locstr;
        }

        void AddStringBlock( eStrBNames blkty, uint32_t beg, uint32_t end )
        {
            m_offsets.emplace( std::make_pair( blkty, strbounds_t{beg,end} ) );
        }

        inline const strbounds_t * operator[]( eStrBNames blkty )const
        {
            auto found = m_offsets.find( blkty );

            if( found != m_offsets.end() )
                return &(found->second);
            else
                return nullptr;
        }

        inline const strbounds_t * operator[]( size_t blkindex )const
        {
            if( blkindex < static_cast<unsigned int>(eStrBNames::NBEntries) )
                return operator[]( static_cast<eStrBNames>(blkindex) );
            else
                return nullptr;
        }

        inline bool   empty()const { return m_offsets.empty(); }
        inline size_t size ()const { return m_offsets.size();  }

    private:
        std::map<eStrBNames,strbounds_t> m_offsets;
        std::string                      m_localeStr;
    };

//==================================================================================
//  GameLanguageLoader
//==================================================================================
    /*
        Loads the list of game language and the locale strings that go with it.
    */
    class GameLanguageLoader
    {
        friend class GameLangXMLParser;
    public:
        struct glang_t
        {
            std::string                    language;
            std::string                    textStrFName;
            std::string                    localeStr;
            std::map<std::string,std::pair<uint32_t,uint32_t>> strBlockOffsets;   //Name + offsets of all the sections containing specific strings
        };

        typedef std::pair<uint32_t,uint32_t> strbounds_t;


        GameLanguageLoader();
        GameLanguageLoader( const std::string & langxmlfname, eGameVersion version );

        /*
            Using the name of the text_*.str file, this will return the corresponding
            locale string parsed!
        */
        std::string              FindLocaleString  ( const std::string & textFileName )const;

        /*
            Return the language associated with a given text_*.str file.
        */
        std::string              FindLanguage      ( const std::string & textFileName )const;

        /*
            Return the name of the text_*.str file associated with the specified language.
        */
        std::string              FindTextStrFName  ( const std::string & language     )const;

        /*
            Returns all the valid string blocks bounds obtained for a given text file name.
        */
        StringsCatalog FindAllBlockOffsets( const std::string & textFileName )const;

        /*
            For a given string block name inside the text_*.str file, returns whether the 
            block was found, and the bounds of that block. The result is put into a pair.
            First is whether it was found or not, second are the bounds, beginning and end.
        */
        std::pair<bool,strbounds_t> FindStrBlockOffset( const std::string & blockName, const std::string & textFileName )const;

        /*
            Returns the complete language struct by language name. Or nullptr if the language doesn't exist.
            **NOTE: The pointer will be invalidated as soon as the object gets modified, so better use the other
            methods for querying details!.
        */
        const glang_t * GetLanguage( const std::string & langname )const;

    private:
        void LoadFile( const std::string & textFileName );

        typedef std::map<std::string,strbounds_t>::value_type blockoffs_t;



        std::vector<glang_t> m_langData;
        eGameVersion         m_gameVersion;
    };

};

#endif