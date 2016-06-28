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


namespace pmd2
{

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


    enum struct eGameConstants :unsigned int
    {
        Invalid = 0,
        NbPossibleHeros,
        NbPossiblePartners,
        NbUniqueEntities,
        NbTotalEntities,

        NbEntries,
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


    struct GameStringData
    {
        std::string     filename;
        eGameLanguages  lang;
        std::string     localestr;
    };

    

//========================================================================================
//  GameVersionsData
//========================================================================================
    /*
        Base class for the various data accessor. Just to save on rewriting those every times.
    */
    //template<typename _VALT>
    //    class BaseGameKeyValData
    //{
    //public:
    //    typedef _VALT                                   value_t;
    //    typedef std::unordered_map<std::string,value_t> gvcnt_t;
    //    typedef gvcnt_t::iterator                       iterator;
    //    typedef gvcnt_t::const_iterator                 const_iterator;

    //    BaseGameKeyValData(){}

    //    BaseGameKeyValData(gvcnt_t&& data)
    //        :m_kvdata(std::forward<gvcnt_t>(data))
    //    {}

    //    BaseGameKeyValData(BaseGameKeyValData&& mv) {this->operator=(std::forward<BaseGameKeyValData>(mv));}

    //    const BaseGameKeyValData & operator=(BaseGameKeyValData&& mv)
    //    { 
    //        this->m_kvdata = std::move(mv.m_kvdata);
    //        return *this;
    //    }

    //    inline iterator         begin()      { return m_kvdata.begin(); }
    //    inline const_iterator   begin()const { return m_kvdata.begin(); }
    //    inline iterator         end  ()      { return m_kvdata.end(); }
    //    inline const_iterator   end  ()const { return m_kvdata.end(); }
    //    inline size_t           size ()const { return m_kvdata.size(); }
    //    inline bool             empty()const { return m_kvdata.empty(); }

    //protected:
    //    gvcnt_t m_kvdata;
    //};


//========================================================================================
//  GameVersionsData
//========================================================================================
    //class GameVersionsData : public BaseGameKeyValData<GameVersionInfo>
    //{
    //public:
    //    using BaseGameKeyValData::BaseGameKeyValData;

    //    const_iterator GetVersionForArm9Off14( uint16_t arm9off14 )const;
    //    const_iterator GetVersionForId       ( const std::string & id )const;
    //};

//
//  GameConstantsData
//
    //class GameConstantsData 
    //{
    //public:
    //    typedef std::unordered_map<,> cnt_t;

    //    GameConstantsData(){}
    //    GameConstantsData( gvcnt_t && gvinf );


    //    const_iterator GetConstant( eGameVersion vers, const std::string & name )const;
    //};

//
//  GameBinaryOffsets
//

    /*
    */
    //class GameBinaryOffsets
    //{
    //public:
    //    typedef std::unordered_map<eBinaryLocations,GameBinaryOffsetInfo> cnt_t;
    //    typedef cnt_t::iterator                                           iterator;
    //    typedef cnt_t::const_iterator                                     const_iterator;

    //    GameBinaryOffsets( cnt_t && locations )
    //        :m_locations(std::forward<cnt_t>(locations))
    //    {}

    //    GameBinaryOffsets( GameBinaryOffsets && mv )
    //    {
    //        this->operator=(std::forward<GameBinaryOffsets>(mv));
    //    }

    //    const GameBinaryOffsets& operator=(GameBinaryOffsets && mv)
    //    {
    //        this->m_locations = std::move(m_locations);
    //        return *this;
    //    }

    //    const const_iterator GetLocationInfo( eBinaryLocations loc )const
    //    {
    //        return m_locations.find(loc);
    //    }

    //    inline iterator       begin()      { return m_locations.begin(); }
    //    inline const_iterator begin()const { return m_locations.begin(); }
    //    inline iterator       end()      { return m_locations.end(); }
    //    inline const_iterator end()const { return m_locations.end(); }

    //    inline bool empty()const {return m_locations.empty();}
    //    inline size_t size()const {return m_locations.size();}

    //private:
    //    cnt_t       m_locations;
    //};

//
//  GameStrings
//
    class GameStringIndex
    {
    public:
        struct strbounds
        {
            uint16_t beg;
            uint16_t end;
        };

        /*
        */
        class BlocksIndex
        {
        public:
            typedef std::unordered_map<eStringBlocks, strbounds> blkcnt_t;
            typedef blkcnt_t::iterator                           iterator;
            typedef blkcnt_t::const_iterator                     const_iterator;

            BlocksIndex( std::string && strfname, std::string && strloc, eGameLanguages lang, blkcnt_t && blocks )
                :m_strfname(std::forward<std::string>(strfname)),
                 m_strlocale(std::forward<std::string>(strloc)),
                 m_lang(lang),
                 m_blocks(std::forward<blkcnt_t>(blocks))
            {}

            const BlocksIndex & operator=( BlocksIndex && mv )
            {
                this->m_blocks    = std::move(mv.m_blocks);
                this->m_strfname  = std::move(mv.m_strfname);
                this->m_strlocale = std::move(mv.m_strlocale);
                return *this;
            }

            BlocksIndex( BlocksIndex && mv )
            {
                this->operator=(std::move(mv));
            }

            inline const_iterator GetStrBlock(eStringBlocks blk)const {return m_blocks.find(blk);}

            inline const std::string & GetLocaleStr()const  {return m_strlocale;}
            inline const std::string & GetStrFName()const   {return m_strfname;}
            inline const eGameLanguages GetLanguage()const {return m_lang;}

        private:
            blkcnt_t        m_blocks;
            std::string     m_strfname;
            std::string     m_strlocale;
            eGameLanguages  m_lang;
        };


        typedef std::string                                 gameid_t;      // <-
        typedef std::string                                 strfname_t;    // <- Made those to make it a bit more intuitive
        typedef std::string                                 blockname_t;   // <-
        typedef BlocksIndex                                 blocks_t;      
        typedef std::unordered_map<strfname_t, blocks_t>    strfiles_t;    


        GameStringIndex()
        {}

        GameStringIndex( strfiles_t && strdata )
            :m_strindex(std::forward<strfiles_t>(strdata))
        {}

        GameStringIndex( GameStringIndex && mv )
        {
            this->operator=(std::forward<GameStringIndex>(mv));
        }

        const GameStringIndex & operator=( GameStringIndex && mv )
        {
            this->m_strindex = std::move(mv.m_strindex);
            return *this;
        }

        /*
            returns null if block not found!
        */
        const blocks_t * GetStrBlocks( const strfname_t & strfname )const
        {
            auto foundstrf = m_strindex.find(strfname);
            if( foundstrf != m_strindex.end() )
                return &(foundstrf->second);
            else
                return nullptr;
        }

    private:
        strfiles_t m_strindex;
    };


//========================================================================================
//  ConfigLoader
//========================================================================================

    /*
        Load constants as needed from the XML file.
    */
    //class ConfigLoaderImpl;
    class ConfigLoader
    {
        friend class ConfigXMLParser;
        typedef std::unordered_map<eBinaryLocations,GameBinaryOffsetInfo> bincnt_t;
        typedef std::unordered_map<eGameConstants,  std::string>          constcnt_t;
    public:
        /*
            - arm9off14: The short stored at offset 14 in the ARM9 bin
        */
        ConfigLoader( uint16_t arm9off14, const std::string & configfile = "pmd2data.xml" );

        inline const GameVersionInfo & GetGameVersion         ()const {return m_versioninfo;}
        inline const GameStringIndex & GetGameStringIndex     ()const {return m_strindex;}
        inline const std::string     & GetGameConstantAsString( eGameConstants gconst )const{return m_constants.at(gconst);}

        int                            GetGameConstantAsInt   ( eGameConstants gconst )const;
        unsigned int                   GetGameConstantAsUInt  ( eGameConstants gconst )const;
        const GameBinaryOffsetInfo   & GetGameBinaryOffset    ( eBinaryLocations loc )const { return m_binoffsets.at(loc); }

    private:
        void Parse();

    private:
        
        std::string         m_conffile;
        uint16_t            m_arm9off14;

        GameVersionInfo     m_versioninfo;
        constcnt_t          m_constants;
        bincnt_t            m_binoffsets;
        GameStringIndex     m_strindex;

        //ConfigLoader(const ConfigLoader &) = delete;
        //ConfigLoader(ConfigLoader &&) = delete;
        //const ConfigLoader& operator=(const ConfigLoader &) = delete;
        //ConfigLoader& operator=(ConfigLoader &&) = delete;

        //std::unique_ptr<ConfigLoaderImpl> m_pimpl;
    };
};

#endif
