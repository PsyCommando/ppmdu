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
    */
    enum struct eStrBNames : unsigned int
    {
        PkmnNames,
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
//  GameLanguageLoader
//==================================================================================
    /*
        Loads the list of game language and the locale strings that go with it.
    */
    class GameLanguageLoader
    {
        friend class GameLangXMLParser;
    public:
        typedef std::pair<uint32_t,uint32_t> strbounds_t;


        GameLanguageLoader();
        GameLanguageLoader( const std::string & textFileName, eGameVersion version );

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
            For a given string block name inside the text_*.str file, returns whether the 
            block was found, and the bounds of that block. The result is put into a pair.
            First is whether it was found or not, second are the bounds, beginning and end.
        */
        std::pair<bool,strbounds_t> FindStrBlockOffset( const std::string & blockName, const std::string & textFileName )const;

    private:
        void LoadFile( const std::string & textFileName );

        typedef std::map<std::string,strbounds_t>::value_type blockoffs_t;

        struct glang_t
        {
            std::string                    language;
            std::string                    textStrFName;
            std::string                    localeStr;
            std::map<std::string,std::pair<uint32_t,uint32_t>> strBlockOffsets;   //Name + offsets of all the sections containing specific strings
        };

        std::vector<glang_t> m_langData;
        eGameVersion         m_gameVersion;
    };

};

#endif