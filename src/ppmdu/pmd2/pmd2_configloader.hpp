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
        Entities= 0,        //Entities table
        Events,             //Events table
        ScriptOpCodes,      //Script opcodes table
        StartersHeroIds,    //Table of Hero ids for each natures
        StartersPartnerIds, //Table of partner ids
        StartersStrings,    //Table of string ids for the result
        QuizzQuestionStrs,  //Table of string ids for the Quizz Questions
        QuizzAnswerStrs,    //Table of string ids for the Quizz answers
        ScriptVariables,    //Table of informations on the script variables 
        ScriptVarsLocals,   //Table of informations on the local script variables 
        Objects,            //Table containing info on all object sprite files the game can load.
        CommonRoutines,     //Table containing info on commmon routines

        NbLocations,        //Must be last valid entry
        Invalid,   
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

//========================================================================================
//  GameScriptData
//========================================================================================

    /************************************************************************************
        NamedDataEntry
            This is meant to be used for lists of data entries that must be looked
            up by either name or index. It contains an unordered map hashed container
            with the indices to the strings they correspond to in order to 
            speed up lookup significantly when parsing.
    ************************************************************************************/
    template<class _DataEntryTy>
        class NamedDataEntry
    {
    public:
        typedef _DataEntryTy                                        dataentry_t;
        typedef typename std::vector<dataentry_t>::iterator         iterator;
        typedef typename std::vector<dataentry_t>::const_iterator   const_iterator;

        /*
            PushEntryPair
                Push a single entry pair
        */
        void PushEntryPair( const std::string & name, const dataentry_t & data )
        {
            m_datastrlut.emplace(name, m_data.size() );
            m_data.push_back(data);
        }


        /*
            PushEntryPair
                Push a single entry pair
        */
        //template<typename _infwdit>
        //    void PushEntryPair( std::string && name, dataentry_t && data )
        //{
        //    m_datastrlut.emplace(name, m_data.size() );
        //    m_data.push_back(data);
        //}

        /*
            PushEntriesPairs
                Adds entries to the end of the current list.
                The entries are expected to be std::pair<std::string,dataentry_t>
        */
        template<typename _infwdit>
            void PushEntriesPairs( _infwdit itbeg, _infwdit itend )
        {
            for( ;itbeg != itend; ++itbeg )
            {
                m_datastrlut.emplace(itbeg->first, m_data.size() );
                m_data.push_back(itbeg->second);
            }
        }

        /*
            FindByName
                Returns nullptr if entry doesn't exists.
        */
        inline const dataentry_t * FindByName( const std::string & name )const
        {
            auto itf = m_datastrlut.find(name);
            if( itf != m_datastrlut.end() )
                return std::addressof(m_data[itf->second]);
            else
                return nullptr;
        }

        /*
            FindIndexByName
                Returns std::numeric_limits<size_t>::max() if entry doesn't exists.
        */
        inline const size_t FindIndexByName( const std::string & name )const
        {
            auto itf = m_datastrlut.find(name);
            if( itf != m_datastrlut.end() )
                return itf->second;
            else
                return std::numeric_limits<size_t>::max();
        }

        /*
            FindByIndex
                returns nullptr if entry doesn't exists.
        */
        inline const dataentry_t * FindByIndex( size_t idx )const
        {
            if( idx < m_data.size() )
                return std::addressof(m_data[idx]);
            else
                return nullptr;
        }

        inline size_t           size()const {return m_data.size();}
        //inline iterator         begin()     {return m_data.begin();}
        inline const_iterator   begin()const{return m_data.begin();}
        //inline iterator         end()       {return m_data.end();}
        inline const_iterator   end()const  {return m_data.end();}

        inline const dataentry_t& operator[](size_t idx)const
        {
            return m_data[idx];
        }

    private:
        std::unordered_map<std::string, size_t> m_datastrlut;
        std::vector<dataentry_t>                m_data;
    };

    /*
        gamevariable_info
    */
    struct gamevariable_info
    {
        int16_t type;
        int16_t unk1;
        int16_t memoffset;
        int16_t bitshift;
        int16_t unk3;
        int16_t unk4;
        std::string name;
    };

    /*
        livesent_info
    */
    struct livesent_info
    {
        std::string name;
        int16_t     type;
        int16_t     entid;
        int16_t     unk3;
        int16_t     unk4;
    };

    /*
        level_info
    */
    struct level_info
    {
        std::string name;
        int16_t     mapty;
        int16_t     unk2;
        int16_t     mapid;
        int16_t     unk4;
    };

    /*
        commonroutine_info
    */
    struct commonroutine_info
    {
        int16_t     id;
        int16_t     unk1;
        std::string name;
    };

    /*
        object_info
    */
    struct object_info
    {

        int16_t unk1;
        int16_t unk2;
        int16_t unk3;
        std::string name;
    };

    /************************************************************************************
        GameScriptData
            Loader/wrapper for XML script data.
    ************************************************************************************/
    class GameScriptData
    {
        friend class ConfigXMLParser;
    public:
        typedef NamedDataEntry<gamevariable_info>   gvar_t;
        typedef NamedDataEntry<livesent_info>       livesent_t;
        typedef NamedDataEntry<level_info>          lvlinf_t;
        typedef NamedDataEntry<commonroutine_info>  commonroutines_t;
        typedef NamedDataEntry<std::string>         stringlut_t;
        typedef NamedDataEntry<object_info>         objinf_t;
        
        inline const gvar_t             & GameVariables()const      {return m_gvars;}
        inline const gvar_t             & ExGameVariables()const    {return m_gvarsex;}
        inline const livesent_t         & LivesEnt()const           {return m_livesent;}
        inline const lvlinf_t           & LevelInfo()const          {return m_levels;}
        inline const commonroutines_t   & CommonRoutineInfo()const  {return m_commonroutines;}
        inline const stringlut_t        & FaceNames()const          {return m_facenames;}
        inline const stringlut_t        & FacePosModes()const       {return m_faceposmodes;}
        inline const stringlut_t        & Directions()const         {return m_directions;}
        inline const objinf_t           & ObjectsInfo()const        {return m_objectsinfo;}

        inline gvar_t                   & GameVariables()           {return m_gvars;}
        inline gvar_t                   & ExGameVariables()         {return m_gvarsex;}
        inline livesent_t               & LivesEnt()                {return m_livesent;}
        inline lvlinf_t                 & LevelInfo()               {return m_levels;}
        inline commonroutines_t         & CommonRoutineInfo()       {return m_commonroutines;}
        inline stringlut_t              & FaceNames()               {return m_facenames;}
        inline stringlut_t              & FacePosModes()            {return m_faceposmodes;}
        inline stringlut_t              & Directions()              {return m_directions;}
        inline objinf_t                 & ObjectsInfo()             {return m_objectsinfo;}

    private:
        gvar_t              m_gvars;
        gvar_t              m_gvarsex; //Extended game vars 0x400 range!
        livesent_t          m_livesent;
        lvlinf_t            m_levels;
        commonroutines_t    m_commonroutines;
        stringlut_t         m_facenames;
        stringlut_t         m_faceposmodes;
        stringlut_t         m_directions;
        objinf_t            m_objectsinfo;
    };

//
//
//
    struct asmpatchentry
    {
        enum struct eAsmPatchStep
        {
            IncludeFile,
            OpenBin,
            CloseBin,
            Invalid,
        };

        struct asmpatchstep
        {
            eAsmPatchStep   op;
            std::string     param;
        };

        std::string                 id;
        std::vector<asmpatchstep>   steps;
    };

    /*
        patchloosebinfile
    */
    struct patchloosebinfile
    {
        eBinaryLocations src;
        std::string      path;
    };

    struct GameASMPatchData
    {
        std::string                                             asmpatchdir;   //Path to source patch files for current game version
        std::unordered_map<std::string,asmpatchentry>           patches;       //<ID,entrydata>
        std::unordered_map<eBinaryLocations,patchloosebinfile>  lfentry;       //<ID,entrydata> loose binary file path entry for the given data type
    };


//
//
//


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

        typedef std::unordered_map<eGameConstants,  std::string> constcnt_t;
    public:

        /*
            - arm9off14: The short stored at offset 14 in the ARM9 bin
        */
        ConfigLoader( uint16_t arm9off14, const std::string & configfile );

        /*
            Allows to get a result from the config file knowing only the region and game version!
        */
        ConfigLoader( eGameVersion version, eGameRegion region, const std::string & configfile );

        void ReloadConfig();

        inline const GameVersionInfo    & GetGameVersion         ()const {return m_versioninfo;}
        inline const LanguageFilesDB    & GetLanguageFilesDB     ()const {return m_langdb;}
        inline const GameASMPatchData   & GetASMPatchData()const        {return m_asmpatchdata;}
        inline const std::string        & GetGameConstantAsString( eGameConstants gconst )const {return m_constants.GetConstAsString(gconst);}
        inline bool                       GetGameConstantAsBool  ( eGameConstants gconst )const {return m_constants.GetConstAsBool(gconst);}
        int                               GetGameConstantAsInt   ( eGameConstants gconst )const;
        unsigned int                      GetGameConstantAsUInt  ( eGameConstants gconst )const;
        inline const binarylocatioinfo    GetGameBinaryOffset    ( eBinaryLocations loc )const 
        { 
            return std::move(m_binblocks.FindInfoByLocation(loc)); 
        }

        inline patchloosebinfile GetLooseBinFileEntry( eBinaryLocations loc )const    
        {
            auto found = m_asmpatchdata.lfentry.find(loc);
            if(found != m_asmpatchdata.lfentry.end())
                return found->second;
            else
                throw std::runtime_error("ConfigLoader::GetLooseBinFileData(): This kind of data doesn't have a a file path entry in the configuration!");
        }

        //Editable data
        inline const GameScriptData         & GetGameScriptData()const      {return m_gscriptdata;}
        inline GameScriptData               & GetGameScriptData()           {return m_gscriptdata;}

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
        GameScriptData      m_gscriptdata;
        GameASMPatchData    m_asmpatchdata;
    };


//
//  MainPMD2ConfigWrapper
//
    /*
        MainPMD2ConfigWrapper
            The main instance of the configuration data for all pmd2 classes.
            Its a singleton that must be initialised at one point. 

            The GameLoader object does it.
    */
    class MainPMD2ConfigWrapper
    {
    public:
        static MainPMD2ConfigWrapper& Instance();     //Gets the instance of the wrapper
        static ConfigLoader         & CfgInstance();  //Gets the configuration loader object

        void InitConfig( uint16_t arm9off14,   const std::string & configfile );
        void InitConfig( eGameVersion version, eGameRegion region, const std::string & configfile );
        inline void ReloadConfig();

        inline ConfigLoader       * GetConfig()      {return m_loaderinstance.get();}
        inline const ConfigLoader * GetConfig()const {return m_loaderinstance.get();}

    private:
        std::unique_ptr<ConfigLoader> m_loaderinstance;
        static MainPMD2ConfigWrapper  s_instance;
    };

};

#endif
