#ifndef PMD2_CONFIG_LOADER_HPP
#define PMD2_CONFIG_LOADER_HPP
/*
pmd2_configloader.hpp
Description: Loads on demand certain values from the config file!
*/
#include <ppmdu/pmd2/pmd2.hpp>
//#include <ppmdu/pmd2/pmd2_langconf.hpp>
#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <tuple>


namespace pmd2
{
    const std::string DefConfigFileName = "pmd2data.xml";

    enum struct eBinaryLocations : unsigned int
    {
        Invalid = 0,        
        Entities,           //Entities table
        Events,             //Events table
        ScriptOpCodes,      //Script opcodes table
        StartersHeroIds,    //Table of Hero ids for each natures
        StartersPartnerIds, //Table of partner ids
        StartersStrings,    //Table of string ids for the result
        QuizzQuestionStrs,  //Table of string ids for the Quizz Questions
        QuizzAnswerStrs,    //Table of string ids for the Quizz answers

        NbLocations,        //Must be last
    };
    extern const std::array<std::string, static_cast<uint32_t>(eBinaryLocations::NbLocations)> BinaryLocationNames;

    enum struct eGameConstants :unsigned int
    {
        NbPossibleHeros= 0,
        NbPossiblePartners,
        NbUniqueEntities,
        NbTotalEntities,

        NbEntries,
        Invalid,
    };
    extern const std::array<std::string, static_cast<uint32_t>(eGameConstants::NbEntries)> GameConstantNames;

    /*******************************************************************************
        eStringBlocks
        Enum for associating the values of the StrBlocksNames array below.

        **NEED TO KEEP THIS IN SYNC WITH StrBlocksNames**
    *******************************************************************************/
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

    extern const std::array<std::string, static_cast<uint32_t>(eStringBlocks::NBEntries)> StringBlocksNames;

//
//
//
    inline eStringBlocks StrToStringBlock( const std::string & blkname )
    {
        for( size_t i = 0; i < StringBlocksNames.size(); ++i )
        {
            if( blkname == StringBlocksNames[i] )
                return static_cast<eStringBlocks>(i);
        }
        return eStringBlocks::Invalid;
    }


    inline eBinaryLocations StrToBinaryLocation( const std::string & locname )
    {
        for( size_t i = 0; i < BinaryLocationNames.size(); ++i )
        {
            if( locname == BinaryLocationNames[i] )
                return static_cast<eBinaryLocations>(i);
        }
        return eBinaryLocations::Invalid;
    }

    inline eGameConstants StrToGameConstant( const std::string & constname )
    {
        for( size_t i = 0; i < GameConstantNames.size(); ++i )
        {
            if( constname == GameConstantNames[i] )
                return static_cast<eGameConstants>(i);
        }
        return eGameConstants::Invalid;
    }

//========================================================================================
//
//========================================================================================
    struct GameVersionInfo
    {
        std::string     id;
        std::string     code;
        eGameVersion    version;
        eGameRegion     region;
        uint16_t        arm9off14; //Value at offse 0xE in ARM9 bin
        eGameLanguages  defaultlang;
        bool            issupported;
    };

    struct GameBinaryOffsetInfo
    {
        std::string fpath;  //The path to the file containing the data
        uint32_t    beg;
        uint32_t    end;
    };


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
        typedef blkcnt_t::iterator                            iterator;
        typedef blkcnt_t::const_iterator                      const_iterator;

        StringsCatalog(){}

        StringsCatalog( std::string && strfname, std::string && locstr, eGameLanguages lang, blkcnt_t && blocks )
            :m_localeStr(std::move(locstr)), m_strfname(std::move(strfname)), m_lang(lang), m_offsets(std::move(blocks))
        {}

        StringsCatalog( std::string strfname, std::string locstr, eGameLanguages lang, blkcnt_t blocks )
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

        inline const strbounds_t & operator[]( eStringBlocks blkty )const
        {
            auto found = m_offsets.find( blkty );
            if( found == m_offsets.end() )
                throw std::out_of_range("StringCatalog::operator[](): Unknown string block!");
            return found->second;
        }

        inline const strbounds_t & operator[]( size_t blkindex )const
        {
            if( blkindex >= static_cast<unsigned int>(eStringBlocks::NBEntries) )
                throw std::out_of_range("StringCatalog::operator[](): String block index out of range!");
            return operator[]( static_cast<eStringBlocks>(blkindex) );
        }


        inline iterator find(eStringBlocks blk)
        {
            return m_offsets.find(blk);
        }

        inline const_iterator find(eStringBlocks blk)const
        {
            return m_offsets.find(blk);
        }

        inline bool   empty()const { return m_offsets.empty(); }
        inline size_t size ()const { return m_offsets.size();  }
        inline iterator begin() {return m_offsets.begin();}
        inline const_iterator begin()const {return m_offsets.begin();}
        inline iterator end() {return m_offsets.end();}
        inline const_iterator end()const {return m_offsets.end();}

    private:
        blkcnt_t        m_offsets;
        std::string     m_localeStr;
        std::string     m_strfname;
        eGameLanguages  m_lang;
    };

//========================================================================================
//  GameStrings
//========================================================================================
    /*
    */
    class LanguageFilesDB
    {
    public:
        typedef std::string                                 gameid_t;      // <-
        typedef std::string                                 strfname_t;    // <- Made those to make it a bit more intuitive
        typedef std::string                                 blockname_t;   // <-
        typedef StringsCatalog                              blocks_t;      
        typedef std::unordered_map<strfname_t, blocks_t>    strfiles_t;    

        LanguageFilesDB()
        {}

        LanguageFilesDB( strfiles_t && strdata )
            :m_strindex(std::forward<strfiles_t>(strdata))
        {}

        LanguageFilesDB( LanguageFilesDB && mv )
        {
            this->operator=(std::forward<LanguageFilesDB>(mv));
        }

        const LanguageFilesDB & operator=( LanguageFilesDB && mv )
        {
            this->m_strindex = std::move(mv.m_strindex);
            return *this;
        }

        /*
            returns null if block not found!
        */
        const blocks_t * GetByTextFName( const strfname_t & strfname )const
        {
            auto foundstrf = m_strindex.find(strfname);
            if( foundstrf != m_strindex.end() )
                return &(foundstrf->second);
            else
                return nullptr;
        }

        const blocks_t * GetByLanguage( eGameLanguages lang )const
        {
            for( const auto & entry : m_strindex )
            {
                if( entry.second.GetLanguage() == lang )
                    return &(entry.second);
            }
            return nullptr;
        }

        inline const strfiles_t & Languages()const
        {
            return m_strindex;
        }

    private:
        strfiles_t m_strindex;
    };


//========================================================================================
//  ConfigLoader
//========================================================================================

    /************************************************************************************
        ConfigLoader
            Load constants as needed from the XML file.
    ************************************************************************************/
    class ConfigLoader
    {
        friend class ConfigXMLParser;
        typedef std::unordered_map<eBinaryLocations,GameBinaryOffsetInfo> bincnt_t;
        typedef std::unordered_map<eGameConstants,  std::string>          constcnt_t;
    public:

        /*
            - arm9off14: The short stored at offset 14 in the ARM9 bin
        */
        ConfigLoader( uint16_t arm9off14, const std::string & configfile = DefConfigFileName );

        /*
            Allows to get a result from the config file knowing only the region and game version!
        */
        ConfigLoader( eGameVersion version, eGameRegion region, const std::string & configfile = DefConfigFileName );

        inline const GameVersionInfo & GetGameVersion         ()const {return m_versioninfo;}
        inline const LanguageFilesDB & GetLanguageFilesDB     ()const {return m_langdb;}
        inline const std::string     & GetGameConstantAsString( eGameConstants gconst )const{return m_constants.at(gconst);}

        int                            GetGameConstantAsInt   ( eGameConstants gconst )const;
        unsigned int                   GetGameConstantAsUInt  ( eGameConstants gconst )const;
        const GameBinaryOffsetInfo   & GetGameBinaryOffset    ( eBinaryLocations loc )const { return m_binoffsets.at(loc); }

    private:
        void Parse(uint16_t arm9off14);
        void Parse(eGameVersion version, eGameRegion region);

    private:
        
        std::string         m_conffile;
        uint16_t            m_arm9off14;

        GameVersionInfo     m_versioninfo;
        constcnt_t          m_constants;
        bincnt_t            m_binoffsets;
        LanguageFilesDB     m_langdb;
    };

};

#endif
