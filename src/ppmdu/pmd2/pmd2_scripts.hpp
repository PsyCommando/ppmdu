#ifndef PMD2_SCRIPTS_HPP
#define PMD2_SCRIPTS_HPP
/*
pmd2_scripts.hpp
2015/09/24
psycommando@gmail.com
Description: This code is used to load/index the game scripts.
*/
#include <ppmdu/pmd2/pmd2.hpp>
#include <cstdint>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <deque>
#include <memory>
#include <regex>


namespace pmd2
{
    /**********************************************************************************
        Script Naming Constants
    **********************************************************************************/
    //Unique Files
    const std::string ScriptNames_enter    = "enter.sse";       //enter.sse
    //const std::string ScriptNames_dus      = "dus.sss";         //dus.sss           //!#TODO: change this. We found hus.sss and mus.sss exist too
    const std::string ScriptNames_unionall = "unionall.ssb";    //unionall.ssb
    const std::string DirNameScriptCommon  = "COMMON";

    //Name lens
    const size_t      ScriptNameLen_U      = 4;             //The full length of a u prefixed file may not exceed 4!
    const size_t      ScriptDigitLen       = 2;

    //Script Only Prefixes
    const std::string ScriptPrefix_unionall= "unionall";
    const std::string ScriptPrefix_enter   = "enter";
    const std::string ScriptPrefix_dus     = "dus";
    const std::string ScriptPrefix_hus     = "hus";
    const std::string ScriptPrefix_mus     = "mus";
    const std::string ScriptPrefix_u       = "u";

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
    enum struct eScriptGroupType 
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

    /***********************************************************************************************
        ScriptInstruction
            Contains a single instruction from a script, from either EoS or EoTD.
    ***********************************************************************************************/
    struct ScriptInstruction
    {
        uint16_t             opcode;            //Either the opcode, or a data word, depending on whether its isdata or not
        std::deque<uint16_t> parameters;        //The parameters for the instruction
        bool                 isdata;            //Whether the instruction is actually a data word
    };

    /***********************************************************************************************
        ScriptInstrGrp
            Contains a groupd of instructions.

    ***********************************************************************************************/
    struct ScriptInstrGrp
    {
        typedef std::deque<ScriptInstruction>::iterator       iterator;
        typedef std::deque<ScriptInstruction>::const_iterator const_iterator;

        iterator       begin() { return instructions.begin(); }
        const_iterator begin()const { return instructions.begin(); }

        iterator       end() { return instructions.end(); }
        const_iterator end()const { return instructions.end(); }

        size_t         size()const { return instructions.size(); }
        bool           empty()const { return instructions.empty(); }

        //std::string    Print()const;

        std::deque<ScriptInstruction> instructions;
        uint16_t                      type;
        uint16_t                      unk2;
    };

    /***********************************************************************************************
        ScriptedSequence
            The content of a SSB file. Script instructions, strings, and constant names.
    ***********************************************************************************************/
    class ScriptedSequence
    {
    public:
        static const size_t                  NbLang = static_cast<size_t>(eGameLanguages::NbLang);
        typedef std::vector<std::string>      strtbl_t;
        typedef std::vector<std::string>      consttbl_t;
        typedef std::deque<ScriptInstrGrp>   grptbl_t;
        typedef std::unordered_map<eGameLanguages, strtbl_t> strtblset_t;

        ScriptedSequence(){}
        ScriptedSequence(const std::string & name, const std::string & origfname)
            :m_name(name), m_originalfname(origfname)
        {}

        ScriptedSequence(const ScriptedSequence & tocopy);
        ScriptedSequence(ScriptedSequence      && tomove);
        ScriptedSequence & operator=( const ScriptedSequence & tocopy );
        ScriptedSequence & operator=( ScriptedSequence       && tomove );

        inline const std::string                & Name       ()const                        { return m_name; }
        inline const std::string                & FileName   ()const                        { return m_originalfname; }
        inline void                               SetName    ( const std::string & name )   { m_name = name; }
        inline void                               SetFileName( const std::string & fname )  { m_originalfname = fname; }

        //!#TODO: Encapsulate those later.      
        inline grptbl_t                         & Groups()         { return m_groups; }
        inline const grptbl_t                   & Groups() const   { return m_groups; }

        //Returns the set of all strings for all languages
        inline strtblset_t                      & StrTblSet()         { return m_strtable; }
        inline const strtblset_t                & StrTblSet() const   { return m_strtable; }

        void               InsertStrLanguage( eGameLanguages lang, strtbl_t && strings );
        //Returns all strings for a specific language
        inline strtbl_t                         * StrTbl( eGameLanguages lang ); //     { return m_strtable[static_cast<size_t>(lang)]; }
        //inline strtbl_t                         & StrTbl( size_t   lang )      { return m_strtable[lang]; }
        inline const strtbl_t                   * StrTbl( eGameLanguages lang )const;// { return m_strtable[static_cast<size_t>(lang)]; }
        //inline const strtbl_t                   & StrTbl( size_t   lang )const { return m_strtable[lang]; }

        inline consttbl_t          & ConstTbl()       { return m_contants; }
        inline const consttbl_t    & ConstTbl() const { return m_contants; }

    private:
        std::string m_name;
        std::string m_originalfname;
        grptbl_t    m_groups;
        strtblset_t m_strtable; //Multiple deques for all languages
        consttbl_t  m_contants;
    };

    /***********************************************************************************************
        ScriptEntityData
            Position data contained in SSA, SSS, and SSE files.
    ***********************************************************************************************/
    class ScriptEntityData
    {
    public:


        ScriptEntityData()
        {}

        ScriptEntityData(const std::string & name, const std::string & origfname, eScrDataTy datatype )
            :m_name(name), m_originalfname(origfname), m_datatype(datatype)
        {}

        inline const std::string & Name()const                      {return m_name;}
        inline void                Name(const std::string & name)   {m_name = name;}

        inline const std::string & FileName()const                      {return m_originalfname;}
        inline void                FileName(const std::string & name)   {m_originalfname = name;}

        inline eScrDataTy Type()const{return m_datatype;}

    private:
        std::string m_name;
        std::string m_originalfname;
        eScrDataTy  m_datatype;
    };

    /***********************************************************************************************
        ScriptGroup
            A script group is an ensemble of one or no ScriptEntityData, and one or more
            ScriptedSequence, that share a common identifier.
    ***********************************************************************************************/
    class ScriptGroup
    {
    public:
        typedef std::unique_ptr<ScriptEntityData>      dataptr_t;
        typedef std::map<std::string,ScriptedSequence> seqtbl_t;
        typedef seqtbl_t::const_iterator               const_seqtbl_iter_t;
        typedef seqtbl_t::iterator                     seqtbl_iter_t;

        ScriptGroup(const std::string & id, eScriptGroupType ty = eScriptGroupType::INVALID)
            :m_indentifier(id), m_type(ty)
        {}

        ScriptGroup( const ScriptGroup & other)
            :m_indentifier(other.m_indentifier), m_type(other.m_type), m_sequences(other.m_sequences)
        {
            if( other.m_data != nullptr )
                m_data.reset( new ScriptEntityData(*(other.m_data.get())) );
        }

        ScriptGroup( ScriptGroup && other)
            :m_indentifier(std::move(other.m_indentifier)), m_type(other.m_type)
        {
            m_sequences = std::move( other.m_sequences );
            m_data.reset(other.m_data.release());
            other.m_data = nullptr;
        }

        ScriptGroup& operator=( const ScriptGroup & other)
        {
            m_indentifier   = other.m_indentifier;
            m_type          = other.m_type; 
            m_sequences     = other.m_sequences;

            if( other.m_data != nullptr )
                m_data.reset( new ScriptEntityData(*(other.m_data.get())) );
            return *this;
        }

        ScriptGroup& operator=( ScriptGroup && other)
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
        inline ScriptEntityData       * Data()            {return m_data.get();}
        inline const ScriptEntityData * Data()const       {return m_data.get();}
        inline void                     SetData( ScriptEntityData && data ) 
        {
            m_data.reset( new ScriptEntityData(std::forward<ScriptEntityData>(data)));
        }

        inline seqtbl_t               & Sequences()       {return m_sequences;}
        inline const seqtbl_t         & Sequences()const  {return m_sequences;}

        const_seqtbl_iter_t SequencesTblBeg()const { return m_sequences.begin(); }
        seqtbl_iter_t       SequencesTblBeg()      { return m_sequences.begin(); }
        const_seqtbl_iter_t SequencesTblEnd()const { return m_sequences.end(); }
        seqtbl_iter_t       SequencesTblEnd()      { return m_sequences.end(); }

        eScriptGroupType    Type()const                  { return m_type; }
        void                Type(eScriptGroupType newty) { m_type = newty; }

    private:
        std::string      m_indentifier;
        dataptr_t        m_data;          //Contains SSA, SSS, or SSE files. There can be 0 or more.
        seqtbl_t         m_sequences;     //Contains SSB data.
        eScriptGroupType m_type;
    };

    /***********************************************************************************************
        ScriptSet
            A scriptset is a group of scripts contained within a single named script folder in the
            PMD2 games.
    ***********************************************************************************************/
    class ScriptSet
    {
    public:
        typedef std::array<char,8>           lsdtblentry_t;
        typedef std::deque<lsdtblentry_t>    lsdtbl_t;
        typedef std::deque<ScriptGroup>      scriptgrps_t;
        typedef scriptgrps_t::iterator       iterator;
        typedef scriptgrps_t::const_iterator const_iterator;

        //Constructors
        ScriptSet            (const std::string & name);
        ScriptSet            (const std::string & name, scriptgrps_t && comp, lsdtbl_t && lsdtbl );
        ScriptSet            (const ScriptSet   & other);
        ScriptSet & operator=(const ScriptSet   & other);
        ScriptSet            (ScriptSet        && other);
        ScriptSet & operator=(ScriptSet        && other);

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

        //
        //iterator       FindByIdentifier(const std::string & id);
        //const_iterator FindByIdentifier(const std::string & id)const;

        //


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
        friend void ImportXMLGameScripts(const std::string & dir, GameScripts & out_dest);
        friend void ExportGameScriptsXML(const std::string & dir, const GameScripts & gs );

        typedef std::unordered_map<std::string,ScrSetLoader> evindex_t;
        typedef evindex_t::iterator                          iterator;
        typedef evindex_t::const_iterator                    const_iterator;

        //scrdir : the directory of the game we want to load from/write to.
        GameScripts(const std::string & scrdir, eGameRegion greg, eGameVersion gver );
        ~GameScripts();

        //File IO
        void Load(); //Indexes all scripts in the src dir. The actual sets are loaded on demand.
        //void Write(); //Writes all script sets that were modified.
        void ImportXML(const std::string & dir);
        void ExportXML(const std::string & dir);

        std::unordered_map<std::string,ScriptSet> LoadAll();
        void                                      WriteAll( const std::unordered_map<std::string,ScriptSet> & stuff );

        eGameRegion  Region()const;
        eGameVersion Version()const;

        //Access
        void      WriteScriptSet( const ScriptSet   & set ); //Add new script set or overwrite existing

        //Get the COMMON set. It has its own method, because its unique.
        const ScriptSet & GetCommonSet()const;
        ScriptSet       & GetCommonSet();

        //Method to get access on the list of all found event directories + their helper loader, besides "COMMON".
        inline iterator       begin();
        inline const_iterator begin()const;

        inline iterator       end();
        inline const_iterator end()const;

        inline size_t size()const;
        inline bool   empty()const;

    private:
        ScriptSet LoadScriptSet ( const std::string & setname );

        std::string                                  m_scriptdir;
        ScriptSet                                    m_common;                   //Common script set with unionall.ssb, its always loaded
        evindex_t                                    m_setsindex;                //All sets known to exist
        eGameRegion                                  m_scrRegion;
        eGameVersion                                 m_gameVersion;
        std::unique_ptr<GameScriptsHandler>          m_pHandler;
    };

    /*
        ScrSetLoader
            Helper class used to load known existing directories from the list of indexed directories.
    */
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

        ScriptSet operator()()const;
        void      operator()(const ScriptSet & set)const;

    private:
        std::string   m_path;
        GameScripts * m_parent;
    };


//
//  Import/Export
//

    //void ImportXMLGameScripts(const std::string & dir, GameScripts & out_dest);
    //void ExportGameScriptsXML(const std::string & dir, const GameScripts & gs );

    void      ScriptSetToXML( const ScriptSet   & set,    eGameRegion greg,       eGameVersion gver, const std::string & destdir );
    ScriptSet XMLToScriptSet( const std::string & srcdir, eGameRegion & out_greg, eGameVersion & out_gver );

};

#endif
