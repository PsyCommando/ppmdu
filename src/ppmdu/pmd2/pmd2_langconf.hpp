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
#include <unordered_map>
#include <array>
#include <ppmdu/pmd2/pmd2.hpp>

namespace pmd2
{

    /*
        Enum for associating the values of the StrBlocksNames array below.

        **NEED TO KEEP THIS IN SYNC WITH StrBlocksNames**
    */
    enum struct eStringBlocks : unsigned int
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
        Invalid,
    };

    extern const std::array<std::string, static_cast<uint32_t>(eStringBlocks::NBEntries)> StrBlocksNames;

    /*
        Boundaries for strings
    */
    struct strbounds_t
    {
        uint32_t beg;
        uint32_t end;
    };

//==================================================================================
//   StringsCatalog
//==================================================================================
    /*
        Contains the list of the offsets of all the string blocks that could be found.
    */
    class StringsCatalog
    {
    public:
        typedef std::unordered_map<eStringBlocks,strbounds_t> blkcnt_t;

        StringsCatalog(){}

        StringsCatalog( std::string && strfname, std::string && locstr, eGameLanguages lang, blkcnt_t && blocks )
            :m_localeStr(std::move(locstr)), m_strfname(std::move(strfname)), m_lang(lang), m_offsets(std::move(blocks))
        {}

        StringsCatalog( StringsCatalog && mv )
        {
            this->operator=(std::forward<StringsCatalog>(mv));
        }

        StringsCatalog( const StringsCatalog & cp )
        {
            this->operator=(cp);
        }

        const StringsCatalog & operator=( StringsCatalog && mv )
        {
            this->m_localeStr = std::move(mv.m_localeStr);
            this->m_offsets   = std::move(mv.m_offsets);
            this->m_strfname  = std::move(mv.m_strfname);
            this->m_lang      = std::move(mv.m_lang);
            return *this;
        }

        const StringsCatalog & operator=( const StringsCatalog & cp )
        {
            this->m_localeStr = cp.m_localeStr;
            this->m_offsets   = cp.m_offsets;
            this->m_strfname  = cp.m_strfname;
            this->m_lang      = cp.m_lang;
            return *this;
        }

        //Accessor
        inline const std::string & GetLocaleString()const                         { return m_localeStr; }
        inline void                SetLocaleString( const std::string & locstr )  { m_localeStr = locstr; }

        inline const std::string & GetStrFName()const                           { return m_strfname; }
        inline void                SetStrFName( const std::string & strfname )  { m_strfname = strfname; }

        inline eGameLanguages      GetLanguage()const                           { return m_lang; }
        inline void                SetLanguage( eGameLanguages lang )           { m_lang = lang; }

        void AddStringBlock ( eStringBlocks blkty, uint32_t beg, uint32_t end )    { m_offsets.emplace( std::make_pair( blkty, strbounds_t{beg,end} ) ); }
        void AddStringBlocks( blkcnt_t && blocks )                              { m_offsets = std::move(blocks); }

        inline const strbounds_t * operator[]( eStringBlocks blkty )const
        {
            auto found = m_offsets.find( blkty );

            if( found != m_offsets.end() )
                return &(found->second);
            else
                return nullptr;
        }

        inline const strbounds_t * operator[]( size_t blkindex )const
        {
            if( blkindex < static_cast<unsigned int>(eStringBlocks::NBEntries) )
                return operator[]( static_cast<eStringBlocks>(blkindex) );
            else
                return nullptr;
        }

        inline bool   empty()const { return m_offsets.empty(); }
        inline size_t size ()const { return m_offsets.size();  }

    private:
        blkcnt_t        m_offsets;
        std::string     m_localeStr;
        std::string     m_strfname;
        eGameLanguages  m_lang;
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