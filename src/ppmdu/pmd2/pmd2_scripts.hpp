#ifndef PMD2_SCRIPTS_HPP
#define PMD2_SCRIPTS_HPP
/*
pmd2_scripts.hpp
2015/09/24
psycommando@gmail.com
Description: This code is used to load/index the game scripts.
*/
#include <ppmdu/pmd2/pmd2.hpp>
#include <ppmdu/pmd2/pmd2_configloader.hpp>
#include <cstdint>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <deque>
#include <memory>
#include <regex>
#include <mutex>

namespace pmd2
{
    const std::string ScriptXMLRoot_SingleScript = "SingleScript"; 
    const std::string ScriptXMLRoot_Level        = "Level"; 
    const std::string ScriptDataXMLRoot_SingleDat= "SingleData"; 
    const size_t      ScriptWordLen              = sizeof(uint16_t);    //Len of a word in the scripts. Since its a commonly used unit


    /**********************************************************************************
        Script Naming Constants
    **********************************************************************************/
    //Unique Files
    //const std::string ScriptNames_enter    = "enter.sse";       //enter.sse
    //const std::string ScriptNames_dus      = "dus.sss";         //dus.sss           //!#TODO: change this. We found hus.sss and mus.sss exist too
    const std::string ScriptNames_unionall = "unionall.ssb";    //unionall.ssb
    const std::string DirNameScriptCommon  = "COMMON";

    //Name lens
    //const size_t      ScriptNameLen_U      = 4;             //The full length of a u prefixed file may not exceed 4!
    //const size_t      ScriptDigitLen       = 2;

    //Script Only Prefixes
    const std::string ScriptPrefix_unionall= "unionall";
    const std::string ScriptPrefix_enter   = "enter";
    //const std::string ScriptPrefix_dus     = "dus";
    //const std::string ScriptPrefix_hus     = "hus";
    //const std::string ScriptPrefix_mus     = "mus";
    //const std::string ScriptPrefix_u       = "u";

    //Common Prefixes
    const std::string ScriptPrefix_A = ResourcePrefix_A;
    const std::string ScriptPrefix_B = ResourcePrefix_B;
    const std::string ScriptPrefix_C = ResourcePrefix_C;
    const std::string ScriptPrefix_D = ResourcePrefix_D;   //Dungeon
    const std::string ScriptPrefix_G = ResourcePrefix_G;   //Guild
    const std::string ScriptPrefix_H = ResourcePrefix_H;   //Home?
    const std::string ScriptPrefix_M = ResourcePrefix_M;
    const std::string ScriptPrefix_N = ResourcePrefix_N;
    const std::string ScriptPrefix_P = ResourcePrefix_P;   //Part?
    const std::string ScriptPrefix_S = ResourcePrefix_S;
    const std::string ScriptPrefix_T = ResourcePrefix_T;   //Town
    const std::string ScriptPrefix_V = ResourcePrefix_V;   //Visual?

    /*
        MatchingRegexes
    */
    const std::regex ResrourceNameRegEx        ("([a-z]\\d\\d){1,2}([a-z])?(\\d\\d)\\b"); //For things that are named "m01a01a.something" The second letter + digit pair is optional and the letter at the end is too.

    //const std::regex ScriptRegExNameSSA        ("(.*)\\."s  + SSA_FileExt);
    //const std::regex ScriptRegExNameSSS        ("(.*)\\."s  + SSS_FileExt);
    //const std::regex ScriptRegExNameEnterSSE   ("enter\\."s + SSE_FileExt);
    //const std::regex ScriptRegExNameLSD        ("(.*)\\."s  + LSD_FileExt);
    extern const std::regex MatchScriptFileTypes; //Matches all ssa,ssb,sss, and lsd files

    extern const std::string ScriptRegExNameNumSSBSuff; //Regex to suffix to the basename to get the pattern for the numbered SSBs
    //const std::regex ScriptRegExNameNumberedSSB("(.*)"s + ScriptRegExNameNumSSBSuff ); //matches the whole filename, and split matches into 3 convenient groups basename, ssbid, fileextension


    /*
        isNumberedSSB
            -subname  : base name. Aka the name of the data file tied to this. "m01a02" for example.
            -filename : the filename to verify. "m01a0201.ssb" for example.
    */ 
    inline bool isNumberedSSB( std::string subname, const std::string & filename )
    {
        return std::regex_match(filename, std::regex(subname.append(ScriptRegExNameNumSSBSuff) ) );
    }

    /**********************************************************************************
        Script Type
    **********************************************************************************/
    enum struct eScriptSetType 
    {
        INVALID,
        UNK_unionall,   //Scripts not in a LSD, found under the "COMMON" directory. Has no other component. 
        UNK_enter,      //Scripts not in a LSD, all components labelled "enter", possibly with a 2 digit numbers suffixed. 
        //UNK_dus,        //Scripts not in a LSD, all components labelled "dus", possibly with a 2 digit numbers suffixed. 
        //UNK_hus,        //Scripts not in a LSD, all components labelled "hus", possibly with a 2 digit numbers suffixed. 
        //UNK_mus,        //Scripts not in a LSD, all components labelled "mus", possibly with a 2 digit numbers suffixed. 
        //UNK_u,          //Scripts not in a LSD, but has u prefixed before another prefix(s,m,n,etc), and one or two 2 digit numbers. 
        UNK_fromlsd,    //Script group whose name is in the LSD (Aka regular scripts)
        //UNK_loneSSS,    //A lone SSS script data file
        UNK_sub,         //Scripts not in a LSD, file extension of data file is SSS, may have matched numbered ssb.
        NbTypes,
    };

    enum struct eScrDataTy
    {
        SSE,
        SSS,
        SSA,
        NbTypes,
        Invalid,
    };

    extern const std::array<std::string, static_cast<size_t>(eScrDataTy::NbTypes)> ScriptDataTypeStrings;

    const std::string & ScriptDataTypeToFileExtension( eScrDataTy scrdatty );
    const std::string & ScriptDataTypeToStr( eScrDataTy scrdatty );
    eScrDataTy          StrToScriptDataType( const std::string & scrdatstr );

//==========================================================================================================
//  Script Data Containers
//==========================================================================================================

    /*
        eInstructionType
            Classifies an instruction, to better help determine how to handle it.
    */
    enum struct eInstructionType : uint8_t
    {
        Command,            //For opcodes
        Data,               //For data word

        //Meta instructions
        MetaLabel,          //A jump label
        MetaCaseLabel,      //A jump label for a conditional case
        MetaSwitch,         //The instruction contains case sub-instructions
        MetaAccessor,       //The instruction contains a sub-instruction to be applied to what the accessor is accessing
        MetaProcSpecRet,    //The instruction contains a "case" sub instruction applied on the return value.
        MetaReturnCases,    //For instructions that can have a set of cases applied to their return value

        NbTypes,
        Invalid,
    };


    /***********************************************************************************************
        ScriptInstruction
            Represents a single instruction from a script.

            Is also used to represent meta-command which do not exist in the game's script engine, 
            but are neccessary here to make processing faster. Such as jump labels. 
    ***********************************************************************************************/
    struct ScriptBaseInstruction
    {
        typedef std::deque<uint16_t>          paramcnt_t;
        uint16_t            value;          //Depends on the value of "type". Can be opcode, data, or ID.
        paramcnt_t          parameters;     //The parameters for the instruction
        eInstructionType    type;           //How to handle the instruction

//#ifdef _DEBUG
        size_t              dbg_origoffset; //Original offset of the instruction in the source file if applicable. For debugging purpose
//#endif
    };
    struct ScriptInstruction : public ScriptBaseInstruction
    {
        //typedef std::deque<uint16_t>             paramcnt_t;
        typedef std::deque<ScriptBaseInstruction> subinst_t;
        //uint16_t            value;          //Depends on the value of "type". Can be opcode, data, or ID.
        //paramcnt_t          parameters;     //The parameters for the instruction
        //eInstructionType    type;           //How to handle the instruction
        subinst_t           subinst;        //Contains sub-instructions linked to this instruction

        ScriptInstruction():ScriptBaseInstruction::ScriptBaseInstruction()
        {}

        ScriptInstruction(ScriptInstruction && mv)
        {this->operator=(std::forward<ScriptInstruction>(mv));}

        ScriptInstruction(const ScriptInstruction & cp)
        {this->operator=(cp);}

        ScriptInstruction(ScriptBaseInstruction && mv)
        {this->operator=(std::forward<ScriptBaseInstruction>(mv));}

        ScriptInstruction(const ScriptBaseInstruction & cp)
        {this->operator=(cp);}

        ScriptInstruction & operator=(const ScriptInstruction & cp)
        {
            subinst     = cp.subinst;
            type        = cp.type;
            parameters  = cp.parameters;
            value       = cp.value;
//#ifdef _DEBUG
            dbg_origoffset = cp.dbg_origoffset; 
//#endif
            return *this;
        }

        ScriptInstruction & operator=(ScriptInstruction && mv)
        {
            subinst     = std::move(mv.subinst);
            type        = mv.type;
            parameters  = std::move(mv.parameters);
            value       = mv.value;
//#ifdef _DEBUG
            dbg_origoffset = mv.dbg_origoffset; 
//#endif
            return *this;
        }

        ScriptInstruction & operator=(const ScriptBaseInstruction & cp)
        {
            type        = cp.type;
            parameters  = cp.parameters;
            value       = cp.value;
//#ifdef _DEBUG
            dbg_origoffset = cp.dbg_origoffset; 
//#endif
            return *this;
        }

        ScriptInstruction & operator=(ScriptBaseInstruction && mv)
        {
            type        = mv.type;
            parameters  = std::move(mv.parameters);
            value       = mv.value;
//#ifdef _DEBUG
            dbg_origoffset = mv.dbg_origoffset; 
//#endif
            return *this;
        }

        operator ScriptBaseInstruction()const
        {
            ScriptBaseInstruction out;
            out.parameters  = parameters;
            out.value       = value;
            out.type        = type;
//#ifdef _DEBUG
            out.dbg_origoffset = dbg_origoffset; 
//#endif
            return std::move(out);
        }
    };

    /***********************************************************************************************
        ScriptInstrGrp
            Contains a groupd of instructions.
    ***********************************************************************************************/
    struct ScriptInstrGrp
    {
        typedef std::deque<ScriptInstruction>   cnt_t;
        typedef cnt_t::iterator                 iterator;
        typedef cnt_t::const_iterator           const_iterator;

        inline iterator       begin() { return instructions.begin(); }
        inline const_iterator begin()const { return instructions.begin(); }

        inline iterator       end() { return instructions.end(); }
        inline const_iterator end()const { return instructions.end(); }

        inline size_t         size()const { return instructions.size(); }
        inline bool           empty()const { return instructions.empty(); }

        inline bool           IsAliasOfPrevGroup()const { return isalias; }

        bool     isalias;       //Whether this group refers to the same instructions as the previous one.
        cnt_t    instructions;
        uint16_t type;
        uint16_t unk2;
    };

//==========================================================================================================
//
//==========================================================================================================
    /***********************************************************************************************
        Script
            The content of a SSB file. Script instructions, strings, and constant names.
    ***********************************************************************************************/
    //!#TODO: Rename to Script
    class Script
    {
    public:
        static const size_t                                     NbLang = static_cast<size_t>(eGameLanguages::NbLang);
        typedef std::vector<std::string>                        strtbl_t;
        typedef std::vector<std::string>                        consttbl_t;
        typedef std::deque<ScriptInstrGrp>                      grptbl_t;
        typedef std::unordered_map<eGameLanguages, strtbl_t>    strtblset_t;

        Script(){}
        Script(const std::string & name)
            :m_name(name)
        {}

        //!#REMOVEME: original filename is pretty much useless!
        //Script(const std::string & name, const std::string & origfname)
        //    :m_name(name), m_originalfname(origfname)
        //{}

        Script(const Script & tocopy);
        Script(Script      && tomove);
        Script & operator=( const Script & tocopy );
        Script & operator=( Script       && tomove );

        inline const std::string                & Name       ()const                        { return m_name; }
        //inline const std::string                & FileName   ()const                        { return m_originalfname; }
        inline void                               SetName    ( const std::string & name )   { m_name = name; }
        //inline void                               SetFileName( const std::string & fname )  { m_originalfname = fname; }

        //!#TODO: Encapsulate those later.      
        inline grptbl_t                         & Groups()         { return m_groups; }
        inline const grptbl_t                   & Groups() const   { return m_groups; }

        //Returns the set of all strings for all languages
        inline strtblset_t                      & StrTblSet()         { return m_strtable; }
        inline const strtblset_t                & StrTblSet() const   { return m_strtable; }

        void                                      InsertStrLanguage( eGameLanguages lang, strtbl_t && strings );
        //Returns all strings for a specific language
        inline strtbl_t                         * StrTbl( eGameLanguages lang ); //     { return m_strtable[static_cast<size_t>(lang)]; }
        //inline strtbl_t                         & StrTbl( size_t   lang )      { return m_strtable[lang]; }
        inline const strtbl_t                   * StrTbl( eGameLanguages lang )const;// { return m_strtable[static_cast<size_t>(lang)]; }
        //inline const strtbl_t                   & StrTbl( size_t   lang )const { return m_strtable[lang]; }

        inline consttbl_t          & ConstTbl()       { return m_contants; }
        inline const consttbl_t    & ConstTbl() const { return m_contants; }

    private:
        std::string m_name;
        //std::string m_originalfname;
        grptbl_t    m_groups;
        strtblset_t m_strtable; //Multiple deques for all languages
        consttbl_t  m_contants;
    };

//==========================================================================================================
//  Script Data Structures
//==========================================================================================================

    struct LivesDataEntry
    {
        int16_t livesid = 0;
        int16_t unk1    = 0;
        int16_t unk2    = 0;
        int16_t unk3    = 0;
        int16_t unk4    = 0;
        int16_t unk5    = 0;
        int16_t unk6    = 0;
    };

    struct ObjectDataEntry
    {
        int16_t objid= 0;
        int16_t unk1 = 0;
        int16_t unk2 = 0;
        int16_t unk3 = 0;
        int16_t unk4 = 0;
        int16_t unk5 = 0;
        int16_t unk6 = 0;
        int16_t unk7 = 0;
        int16_t unk8 = 0;
    };

    struct PerformerDataEntry
    {
        int16_t unk0 = 0;
        int16_t unk1 = 0;
        int16_t unk2 = 0;
        int16_t unk3 = 0;
        int16_t unk4 = 0;
        int16_t unk5 = 0;
        int16_t unk6 = 0;
        int16_t unk7 = 0;
        int16_t unk8 = 0;
        int16_t unk9 = 0;
    };

    struct EventDataEntry
    {
        int16_t unk0 = 0;
        int16_t unk1 = 0;
        int16_t unk2 = 0;
        int16_t unk3 = 0;
        int16_t unk4 = 0;
        int16_t unk5 = 0;
        int16_t unk6 = 0;
    };

    //struct UnkScriptDataEntry
    //{
    //    int16_t unk0;
    //    int16_t unk1;
    //    int16_t unk2;
    //    int16_t unk3;
    //};

    struct PosMarkDataEntry
    {
        int16_t unk0 = 0;
        int16_t unk1 = 0;   
        int16_t unk2 = 0;   
        int16_t unk3 = 0;  
        int16_t unk4 = 0;  
        int16_t unk5 = 0;  
        int16_t unk6 = 0;
        int16_t unk7 = 0;
    };

    struct UnkTbl1DataEntry
    {
        int16_t unk0 = 0;
        int16_t unk1 = 0;   
        int16_t unk2 = 0;   
        int16_t unk3 = 0;  
    };

    struct ScriptLayer
    {
        std::vector<LivesDataEntry>     lives;
        std::vector<ObjectDataEntry>    objects;
        std::vector<PerformerDataEntry> performers;
        std::vector<EventDataEntry>     events;
        //std::vector<UnkScriptDataEntry> unkentries;
    };

    /***********************************************************************************************
        ScriptData
            Position data contained in SSA, SSS, and SSE files.
    ***********************************************************************************************/
    //!TODO: Rename to ScriptData.
    class ScriptData
    {
    public:
        typedef std::vector<ScriptLayer>      layers_t;
        typedef std::vector<PosMarkDataEntry> posmarks_t;
        typedef std::vector<UnkTbl1DataEntry> unktbl1ents_t;

        ScriptData()
            :m_datatype(eScrDataTy::Invalid)
        {}

        ScriptData(const std::string & name, eScrDataTy datatype )
            :m_name(name), m_datatype(datatype)
        {}

        inline const std::string & Name()const                      {return m_name;}
        inline void                Name(const std::string & name)   {m_name = name;}

        inline eScrDataTy Type()const           {return m_datatype;}
        inline void       Type(eScrDataTy ty)   {m_datatype = ty;}

        inline layers_t             & Layers()          {return m_layers;}
        inline const layers_t       & Layers()const     {return m_layers;}

        inline posmarks_t           & PosMarkers()      {return m_posmarkers;}
        inline const posmarks_t     & PosMarkers()const {return m_posmarkers;}

        inline unktbl1ents_t        & UnkTbl1()         {return m_unktbl1;}
        inline const unktbl1ents_t  & UnkTbl1()const    {return m_unktbl1;}

    private:
        std::string     m_name;
        layers_t        m_layers;
        posmarks_t      m_posmarkers;
        unktbl1ents_t   m_unktbl1;
        eScrDataTy      m_datatype;
    };

//==========================================================================================================
//
//==========================================================================================================

    /***********************************************************************************************
        ScriptSet
            A script set is an ensemble of one or no ScriptData, and one or more
            Script, that share a common identifier.
    ***********************************************************************************************/
    //!#TODO: Should definitely rename this to "ScriptSet" eventually, since that's what it is..
    class ScriptSet
    {
    public:
        typedef std::unique_ptr<ScriptData>      dataptr_t;
        typedef std::map<std::string,Script> seqtbl_t;
        typedef seqtbl_t::const_iterator               const_seqtbl_iter_t;
        typedef seqtbl_t::iterator                     seqtbl_iter_t;

        ScriptSet(const std::string & id, eScriptSetType ty = eScriptSetType::INVALID)
            :m_indentifier(id), m_type(ty)
        {}

        ScriptSet( const ScriptSet & other)
            :m_indentifier(other.m_indentifier), m_type(other.m_type), m_sequences(other.m_sequences)
        {
            if( other.m_data != nullptr )
                m_data.reset( new ScriptData(*(other.m_data.get())) );
        }

        ScriptSet( ScriptSet && other)
            :m_indentifier(std::move(other.m_indentifier)), m_type(other.m_type)
        {
            m_sequences = std::move( other.m_sequences );
            m_data.reset(other.m_data.release());
            other.m_data = nullptr;
        }

        ScriptSet& operator=( const ScriptSet & other)
        {
            m_indentifier   = other.m_indentifier;
            m_type          = other.m_type; 
            m_sequences     = other.m_sequences;

            if( other.m_data != nullptr )
                m_data.reset( new ScriptData(*(other.m_data.get())) );
            return *this;
        }

        ScriptSet& operator=( ScriptSet && other)
        {
            m_indentifier   = std::move(other.m_indentifier); 
            m_type          = other.m_type;
            m_sequences     = std::move( other.m_sequences );
            m_data.reset(other.m_data.release());
            other.m_data = nullptr;
            return *this;
        }

        inline std::string       & Identifier()      { return m_indentifier; }
        inline const std::string & Identifier()const { return m_indentifier; }
        
        //TEMP
        inline ScriptData       * Data()            {return m_data.get();}
        inline const ScriptData * Data()const       {return m_data.get();}
        inline void                     SetData( ScriptData && data ) 
        {
            m_data.reset( new ScriptData(std::forward<ScriptData>(data)));
        }

        inline seqtbl_t               & Sequences()       {return m_sequences;}
        inline const seqtbl_t         & Sequences()const  {return m_sequences;}

        const_seqtbl_iter_t SequencesTblBeg()const { return m_sequences.begin(); }
        seqtbl_iter_t       SequencesTblBeg()      { return m_sequences.begin(); }
        const_seqtbl_iter_t SequencesTblEnd()const { return m_sequences.end(); }
        seqtbl_iter_t       SequencesTblEnd()      { return m_sequences.end(); }

        eScriptSetType      Type()const                  { return m_type; }
        void                Type(eScriptSetType newty) { m_type = newty; } 

        //Returns the file extension of the data file for this group if applicable.
        // otherwise, returns an empty string.
        const std::string & GetDataFext()const;

    private:
        std::string     m_indentifier;
        dataptr_t       m_data;          //Contains SSA, SSS, or SSE files. There can be 0 or more.
        seqtbl_t        m_sequences;     //Contains SSB data.
        eScriptSetType  m_type;
    };

    /***********************************************************************************************
        LevelScript
            A group of scripts contained within a single named script folder in the
            PMD2 games. Each of those are tied to a specific level.
    ***********************************************************************************************/
    //!#TODO: Should rename this to LevelScript, or something like that, since there's one per level.
    class LevelScript
    {
    public:
        typedef std::array<char,8>           lsdtblentry_t;
        typedef std::deque<lsdtblentry_t>    lsdtbl_t;
        typedef std::deque<ScriptSet>        scriptgrps_t;
        typedef scriptgrps_t::iterator       iterator;
        typedef scriptgrps_t::const_iterator const_iterator;

        //Constructors
        LevelScript            (const std::string & name);
        LevelScript            (const std::string & name, scriptgrps_t && comp, lsdtbl_t && lsdtbl );
        LevelScript            (const LevelScript   & other);
        LevelScript & operator=(const LevelScript   & other);
        LevelScript            (LevelScript        && other);
        LevelScript & operator=(LevelScript        && other);

        //Accessors
        inline void                  Name(const std::string & name)   { m_name = name; }
        inline const std::string   & Name()const                      { return m_name; }

        inline void                  Components(scriptgrps_t && comp) { m_components = std::move(comp); }
        inline const scriptgrps_t  & Components()const                { return m_components; }
        inline scriptgrps_t        & Components()                     { return m_components; }

        inline lsdtbl_t            & LSDTable()                       {return m_lsdentries;}
        inline const lsdtbl_t      & LSDTable()const                  {return m_lsdentries;}

        //Std methods
        inline iterator              begin()      { return m_components.begin(); }
        inline const_iterator        begin()const { return m_components.begin(); }
        inline iterator              end  ()      { return m_components.end(); }
        inline const_iterator        end  ()const { return m_components.end(); }


    private:
        std::string  m_name;         //The name of the set. Ex: "D01P11A" or "COMMON"
        scriptgrps_t m_components;   //All scripts + data groups (SSA,SSS,SSE + SSB)
        lsdtbl_t     m_lsdentries;   //Entries in the LSD table Stored here for now
        bool         m_bmodified;    //Whether the content of this set was modified
    };

//==========================================================================================================
//  Script Manager/Loader
//==========================================================================================================

    /***********************************************************************************************
        GameScripts
            Indexes all scripts from a PMD2 game by event/map name, for easy retrieval and access.

            #TODO: Actually indexes things and classify them, once we know more about the format!
    ***********************************************************************************************/
    
    class GameScripts
    {
    public:
        friend class GameScriptsHandler;
        friend class ScrSetLoader;
        friend void ImportXMLGameScripts(const std::string & dir, GameScripts & out_dest, bool bprintprogress = true);
        friend void ExportGameScriptsXML(const std::string & dir, const GameScripts & gs, bool bautoescapexml, bool bprintprogress = true );

        typedef std::unordered_map<std::string,ScrSetLoader> evindex_t;
        typedef evindex_t::iterator                          iterator;
        typedef evindex_t::const_iterator                    const_iterator;

        //scrdir : the directory of the game we want to load from/write to.
        //GameScripts(const std::string & scrdir, eGameRegion greg, eGameVersion gver, const LanguageFilesDB & langdat );
        GameScripts(const std::string & scrdir, const ConfigLoader & conf, bool bescapexml = false );
        ~GameScripts();

        //File IO
        void Load(); //Indexes all scripts in the src dir. The actual sets are loaded on demand.
        //void Write(); //Writes all script sets that were modified.
        void ImportXML(const std::string & dir);
        void ExportXML(const std::string & dir);

        std::unordered_map<std::string,LevelScript> LoadAll();
        void                                      WriteAll( const std::unordered_map<std::string,LevelScript> & stuff );

        eGameRegion  Region ()const;
        eGameVersion Version()const;

        //Access
        void      WriteScriptSet( const LevelScript   & set ); //Add new script set or overwrite existing

        //Get the COMMON script set, aka "unionall.ssb". It has its own method, because its unique.
        const LevelScript & GetCommonSet()const;
        LevelScript       & GetCommonSet();

        //Method to get access on the list of all found event directories + their helper loader, besides "COMMON".
        inline iterator       begin();
        inline const_iterator begin()const;

        inline iterator       end();
        inline const_iterator end()const;

        inline size_t size()const;
        inline bool   empty()const;

        inline const std::string & GetScriptDir()const{return m_scriptdir;}

        inline const ConfigLoader & GetConfig()const {return m_gconf;}

    private:
        LevelScript LoadScriptSet ( const std::string & setname );

        std::string                                  m_scriptdir;
        LevelScript                                  m_common;                   //Common script set with unionall.ssb, its always loaded
        evindex_t                                    m_setsindex;                //All sets known to exist
        eGameRegion                                  m_scrRegion;
        eGameVersion                                 m_gameVersion;
        std::unique_ptr<GameScriptsHandler>          m_pHandler;
        std::mutex                                   m_mutex;
        const LanguageFilesDB                      * m_langdat;
        bool                                         m_escapexml;
        const ConfigLoader                         & m_gconf;
    };


    /***********************************************************************************************
        ScrSetLoader
            Helper class by the GameScripts class. 
            Used to load known existing directories from the list of indexed directories.
    ***********************************************************************************************/
    //! #TODO: Is this even necessary??
    class ScrSetLoader
    {
    public:
        ScrSetLoader(GameScripts & parent, const std::string & filepath );

        ScrSetLoader( ScrSetLoader && mv )
            :m_parent(mv.m_parent), m_path(std::move(mv.m_path))
        {}

        ScrSetLoader( const ScrSetLoader & cp )
            :m_parent(cp.m_parent), m_path(cp.m_path)
        {}

        ScrSetLoader & operator=( const ScrSetLoader & cp )
        {
            m_path   = cp.m_path;
            m_parent = cp.m_parent;
            return *this;
        }

        ScrSetLoader & operator=( ScrSetLoader && mv )
        {
            m_path   = std::move(mv.m_path);
            m_parent = mv.m_parent;
            return *this;
        }

        LevelScript operator()()const;
        void      operator()(const LevelScript & set)const;

    private:
        std::string   m_path;
        GameScripts * m_parent;
    };


//====================================================================================
//  Import/Export
//====================================================================================
    //bautoescapexml : if set to true, pugixml will escape any reserved/special characters.
    void      ScriptSetToXML( const LevelScript   & set,  
                              const ConfigLoader  & gconf, 
                              bool                  bautoescapexml, 
                              const std::string   & destdir );

    LevelScript XMLToScriptSet( const std::string   & srcdir, 
                                eGameRegion         & out_greg, 
                                eGameVersion        & out_gver, 
                                const ConfigLoader  & gconf );

    //Script Single File Export
    void ScriptToXML( const Script        & scr, 
                      const ConfigLoader  & gconf, 
                      bool                  bautoescapexml, 
                      const std::string   & destdir );

    Script XMLToScript( const std::string   & srcfile, 
                        eGameRegion         & out_greg, 
                        eGameVersion        & out_gver,
                        const ConfigLoader  & gconf );

    //Script Data Single File Export
    void ScriptDataToXML( const ScriptData    & dat, 
                          const ConfigLoader  & gconf, 
                          bool                  bautoescapexml, 
                          const std::string   & destdir );

    ScriptData XMLToScriptData( const std::string   & srcfile, 
                                eGameRegion         & out_greg, 
                                eGameVersion        & out_gver, 
                                const ConfigLoader  & gconf );

//====================================================================================
//  Test Meta-Operation and etc..
//====================================================================================


};

#endif
