#ifndef PMD2_CONFIG_LOADER_HPP
#define PMD2_CONFIG_LOADER_HPP
/*
pmd2_configloader.hpp
Description: Loads on demand certain values from the config file!
*/
#include <ppmdu/pmd2/pmd2.hpp>
#include <ppmdu/pmd2/pmd2_langconf.hpp>
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


    //enum struct eStringBlocks : unsigned int
    //{
    //    PkmnNames = 0,
    //    PkmnCats,
    //    MvNames,
    //    MvDesc,
    //    ItemNames,
    //    ItemDescS,
    //    ItemDescL,
    //    AbilityNames,
    //    AbilityDesc,
    //    TypeNames,
    //    PortraitNames,

    //    //Add new string types above!
    //    NBEntries,
    //    Invalid,
    //};
    //extern const std::array<std::string, static_cast<uint32_t>(eStringBlocks::NBEntries)> StringBlocksNames;


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

//
//  GameStrings
//
    class LanguageFilesDB
    {
    public:
        //struct strbounds
        //{
        //    uint16_t beg;
        //    uint16_t end;
        //};

        /************************************************************************************
            StringBlocks
                Represent a single language/text_*.str file.
        ************************************************************************************/
        //class StringBlocks
        //{
        //public:
        //    typedef std::unordered_map<eStringBlocks, strbounds_t>  blkcnt_t;
        //    typedef blkcnt_t::iterator                              iterator;
        //    typedef blkcnt_t::const_iterator                        const_iterator;

        //    StringBlocks( std::string && strfname, std::string && strloc, eGameLanguages lang, blkcnt_t && blocks )
        //        :m_strfname(std::forward<std::string>(strfname)),
        //         m_strlocale(std::forward<std::string>(strloc)),
        //         m_lang(lang),
        //         m_blocks(std::forward<blkcnt_t>(blocks))
        //    {}

        //    const StringBlocks & operator=( StringBlocks && mv )
        //    {
        //        this->m_blocks    = std::move(mv.m_blocks);
        //        this->m_strfname  = std::move(mv.m_strfname);
        //        this->m_strlocale = std::move(mv.m_strlocale);
        //        return *this;
        //    }

        //    StringBlocks( StringBlocks && mv )
        //    {
        //        this->operator=(std::move(mv));
        //    }

        //    inline const_iterator GetStrBlock(eStringBlocks blk)const {return m_blocks.find(blk);}

        //    inline const std::string & GetLocaleStr()const  {return m_strlocale;}
        //    inline const std::string & GetStrFName()const   {return m_strfname;}
        //    inline const eGameLanguages GetLanguage()const {return m_lang;}

        //private:
        //    blkcnt_t        m_blocks;
        //    std::string     m_strfname;
        //    std::string     m_strlocale;
        //    eGameLanguages  m_lang;
        //};


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

//
//  ConfigInstance
//
    /*
        Helper for making a single instance of the config loader available to what uses it accross the program.
    */
    //class ConfigInstance
    //{
    //public:
    //    static ConfigInstance & Instance()
    //    {
    //        return s_instance;
    //    }

    //    inline ConfigLoader * CreateLoader( uint16_t arm9off14, const std::string & configfile = DefConfigFileName )
    //    {
    //        m_ploader.reset(new ConfigLoader( arm9off14, configfile ));
    //    }

    //    inline std::weak_ptr<ConfigLoader> GetLoaderWeakptr()
    //    {
    //        return m_ploader;
    //    }

    //    inline ConfigLoader* GetLoader()
    //    {
    //        return m_ploader.get();
    //    }

    //    inline bool WasInitialised()const
    //    {
    //        return m_ploader != nullptr;
    //    }

    //private:
    //    std::shared_ptr<ConfigLoader> m_ploader;
    //    static ConfigInstance         s_instance;
    //};

};

#endif
