#ifndef PMD2_CONFIG_LOADER_HPP
#define PMD2_CONFIG_LOADER_HPP
/*
pmd2_configloader.hpp
Description: Loads on demand certain values from the config file!
*/
#include <ppmdu/pmd2/pmd2.hpp>
#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <tuple>
#include <sstream>
#include <iomanip>

namespace pmd2
{
    const std::string DefConfigFileName = "pmd2data.xml";

//========================================================================================
//  Entries in the configuration file
//========================================================================================

    /*******************************************************************************
        eBinaryLocations
            Possible binary locations supported by the program.
    *******************************************************************************/
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

    /*******************************************************************************
        eGameConstants
            List of constants the program can import from the data xml file.
    *******************************************************************************/
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

//========================================================================================
//  Parsing utilities
//========================================================================================

    /*******************************************************************************
        StrToStringBlock
            Gets the string block's enum value from a string containing its name.
    *******************************************************************************/
    inline eStringBlocks StrToStringBlock( const std::string & blkname )
    {
        for( size_t i = 0; i < StringBlocksNames.size(); ++i )
        {
            if( blkname == StringBlocksNames[i] )
                return static_cast<eStringBlocks>(i);
        }
        return eStringBlocks::Invalid;
    }

    /*******************************************************************************
        StrToBinaryLocation
            Gets the binary block's enum value from a string containing its name.
    *******************************************************************************/
    inline eBinaryLocations StrToBinaryLocation( const std::string & locname )
    {
        for( size_t i = 0; i < BinaryLocationNames.size(); ++i )
        {
            if( locname == BinaryLocationNames[i] )
                return static_cast<eBinaryLocations>(i);
        }
        return eBinaryLocations::Invalid;
    }

    /*******************************************************************************
        StrToGameConstant
        Gets the constant's enum value from a string containing its name.
    *******************************************************************************/
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
//  Structs
//========================================================================================

    /*******************************************************************************
        GameVersionInfo
            Details about a game's version as stored in the data xml file.
    *******************************************************************************/
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

    /*******************************************************************************
        GameBinaryOffsetInfo
            Offsets in the binaries, and their name.
    *******************************************************************************/
    //struct GameBinaryOffsetInfo
    //{
    //    std::string fpath;  //The path to the file containing the data
    //    uint32_t    beg;
    //    uint32_t    end;
    //};


    /*******************************************************************************
        strbounds_t
            Boundaries for string blocks in the text files.
    *******************************************************************************/
    struct strbounds_t
    {
        uint32_t beg;
        uint32_t end;
    };

//==================================================================================
//   StringsCatalog
//==================================================================================

    /*******************************************************************************
        StringsCatalog
            Contains the list of the offsets of all the string 
            blocks that could be found.
    *******************************************************************************/
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

    /*******************************************************************************
        LanguageFilesDB
            Contains the string catalogs for several languages.
    *******************************************************************************/
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
//  GameConstants
//========================================================================================

    /*
            Wrap access to constants, and to binary locations.
    */
    class GameConstants
    {
    public:
        typedef std::unordered_map<eGameConstants,  std::string> constcnt_t;

        GameConstants()
        {}

        //**This expects "consts" to have all elements in "eGameConstants", with at least an empty string**
        GameConstants( constcnt_t && consts )
            :m_constants(std::forward<constcnt_t>(consts))
        {}

        inline void SetConstants( constcnt_t && consts )
        {
            m_constants = std::move(consts);
        }

        inline const std::string & GetConstAsString(eGameConstants cnst)const { return m_constants.at(cnst); }
        
        template<typename _IntegerTy>
            _IntegerTy GetConstAsInt(eGameConstants cnst)const
        {
            static_assert( std::is_integral<_IntegerTy>::value, "GameConstants::GetConstAsInt(): _IntegerTy is not an integer!!" );
            _IntegerTy val = 0;
            std::stringstream sstr;
            sstr << GetConstAsString(cnst);
            sstr >> val;
            return val;
        }

        bool GetConstAsBool(eGameConstants cnst)const
        {
            using namespace std;
            bool val = false;
            std::stringstream sstr;
            sstr <<std::boolalpha <<GetConstAsString(cnst); 
            sstr >> val;
            return val;
        }

    private:
        constcnt_t m_constants;
    };

//========================================================================================
//  GameBinariesInfo
//========================================================================================
    struct binlocation
    {
        size_t beg;
        size_t end;
    };
    struct binaryinfo
    {
        typedef std::unordered_map<eBinaryLocations,binlocation> binoffsets_t;
        uint32_t     loadaddress;   //The binary's load address in memory
        binoffsets_t blocks;        //The blocks in the binary
    };

    //Used to retrieve info via eBinaryLocations
    struct binarylocatioinfo
    {
        std::string fpath;
        binlocation location;
        uint32_t    loadaddress;

        /*
            Returns whether the entry is valid or not
        */
        operator bool()const
        {
            return !fpath.empty();
        }
    };

    /************************************************************************************ 
        GameBinariesInfo
            Contains info on where interesting data is stored in the stock binaries.
    ************************************************************************************/
    class GameBinariesInfo
    {
    public:
        typedef std::unordered_map<std::string, binaryinfo> binfilesinf_t;

        inline void AddBinary( std::string && binpath, binaryinfo && binfo )
        {
            m_info.emplace( std::make_pair( std::forward<std::string>(binpath), std::forward<binaryinfo>(binfo) ) );
        }

        const binaryinfo * FindInfo(const std::string & binpath)const
        {
            auto itf = m_info.find(binpath);
            if( itf != m_info.end() )
            {
                return std::addressof(itf->second);
            }
            else
                return nullptr;
        }

        binarylocatioinfo FindInfoByLocation(eBinaryLocations location)const
        {
            binarylocatioinfo inf;
            for( const auto & entry : m_info )
            {
                auto itf = entry.second.blocks.find(location);
                if( itf != entry.second.blocks.end() )
                {
                    inf.fpath       = entry.first;
                    inf.loadaddress = entry.second.loadaddress;
                    inf.location    = itf->second;
                    break;
                }
            }
            return std::move(inf);
        }

    private:
        binfilesinf_t m_info;
    };

//
//  CustomFormat
//
    /*
        This is for parsing the content of any custom data formats in the config files.
    */
    class FlexibleConfigData
    {
    public:
        struct Entry
        {
            typedef std::unordered_multimap<std::string, std::string> attrs_t;
            typedef std::unordered_multimap<std::string, Entry>       subentt_t;

            Entry();
            Entry(attrs_t && attr, subentt_t && sub);
            Entry(Entry&&) = default;
            Entry(const Entry&) = default;
            Entry & operator=(const Entry&) = default;
            Entry & operator=(Entry&&) = default;

            inline void AddAttribute( std::string && name, std::string && value ) 
            { 
                rawattributes.emplace( std::forward<std::string>(name), std::forward<std::string>(value) ); 
            }

            inline void AddSubentry( std::string && name, Entry && sub ) 
            { 
                subentries.emplace( std::forward<std::string>(name), std::forward<Entry>(sub) ); 
            }

            attrs_t     rawattributes;
            subentt_t   subentries;
        };
        typedef Entry data_t;
        



    private:
        data_t m_data;
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
        //typedef std::unordered_map<eBinaryLocations,GameBinaryOffsetInfo> bincnt_t;
        
        typedef std::unordered_map<eGameConstants,  std::string> constcnt_t;
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

        inline const std::string     & GetGameConstantAsString( eGameConstants gconst )const {return m_constants.GetConstAsString(gconst);}
        inline bool                    GetGameConstantAsBool  ( eGameConstants gconst )const {return m_constants.GetConstAsBool(gconst);}
        int                            GetGameConstantAsInt   ( eGameConstants gconst )const;
        unsigned int                   GetGameConstantAsUInt  ( eGameConstants gconst )const;
        inline const binarylocatioinfo GetGameBinaryOffset    ( eBinaryLocations loc )const 
        { 
            return std::move(m_binblocks.FindInfoByLocation(loc)); 
        }

    private:
        void Parse(uint16_t arm9off14);
        void Parse(eGameVersion version, eGameRegion region);

    private:
        std::string         m_conffile;
        uint16_t            m_arm9off14;

        GameVersionInfo     m_versioninfo;
        GameConstants       m_constants;
        GameBinariesInfo    m_binblocks;
        LanguageFilesDB     m_langdb;
    };

};

#endif
