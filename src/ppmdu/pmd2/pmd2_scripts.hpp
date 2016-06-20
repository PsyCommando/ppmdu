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
#include <vector>
#include <deque>
#include <memory>

namespace pmd2
{
    /**********************************************************************************
        Script Naming Constants
    **********************************************************************************/
    //Unique Files
    const std::string ScriptNames_enter    = "enter.sse";       //enter.sse
    const std::string ScriptNames_dus      = "dus.sss";         //dus.sss           //!#TODO: change this. We found hus.sss and mus.sss exist too
    const std::string ScriptNames_unionall = "unionall.ssb";    //unionall.ssb

    //Name lens
    const size_t      ScriptNameLen_U      = 4;             //The full length of a u prefixed file may not exceed 4!

    //Script Only Prefixes
    const std::string ScriptPrefix_enter   = "enter";
    const std::string ScriptPrefix_dus     = "dus";
    const std::string ScriptPrefix_u       = "u";

    //Common Prefixes
    const std::string ScriptPrefix_A = ResourcePrefix_A;
    const std::string ScriptPrefix_B = ResourcePrefix_B;
    const std::string ScriptPrefix_D = ResourcePrefix_D;   //Dungeon
    const std::string ScriptPrefix_G = ResourcePrefix_G;   //Guild
    const std::string ScriptPrefix_H = ResourcePrefix_H;   //Home?
    const std::string ScriptPrefix_M = ResourcePrefix_M;
    const std::string ScriptPrefix_N = ResourcePrefix_N;
    const std::string ScriptPrefix_P = ResourcePrefix_P;   //Part?
    const std::string ScriptPrefix_S = ResourcePrefix_S;
    const std::string ScriptPrefix_T = ResourcePrefix_T;   //Town
    const std::string ScriptPrefix_V = ResourcePrefix_V;   //Visual?

    /**********************************************************************************
        Script Type
    **********************************************************************************/
    enum struct eScriptGroupType 
    {
        INVALID,
        UNK_unionall,   //Scripts not in a LSD, found under the "COMMON" directory. Has no other component. 
        UNK_enter,      //Scripts not in a LSD, all components labelled "enter", possibly with a 2 digit numbers suffixed. 
        UNK_dus,        //Scripts not in a LSD, all components labelled "dus", possibly with a 2 digit numbers suffixed. 
        UNK_u,          //Scripts not in a LSD, but has u prefixed before another prefix(s,m,n,etc), and one or two 2 digit numbers. 
        UNK_fromlsd,    //Script group whose name is in the LSD (Aka regular scripts)
        UNK_loneSSS,    //A lone SSS script data file
        NbTypes,
    };

//==========================================================================================================
//  Script Data Containers
//==========================================================================================================


    /*
    */
    //struct [[nodiscard]] ScriptIdentifier
    //{
    //    enum struct ePrefixes : char
    //    {
    //        //ResourceIDs
    //        A = 'a',
    //        B = 'b',
    //        D = 'd',
    //        G = 'g',
    //        H = 'h',
    //        M = 'm',
    //        N = 'n',
    //        P = 'p',
    //        S = 's',
    //        T = 't',
    //        V = 'v',

    //        none,
    //        NbPrefixes,
    //    };


    //    struct IdToken
    //    {
    //        ePrefixes pf;
    //        uint8_t   digit;
    //        bool      bhasdigit;
    //        bool      bhasprefix;
    //    };

    //    //union IdComponent
    //    //{
    //    //    ePrefixes prfx;
    //    //    uint8_t   number;
    //    //    uint16_t  b15hasnbb14hasprfx; //Byte 15 idicates whether is has a number, Byte 14 indicates
    //    //};

    //    static ScriptIdentifier ParseScriptIdentifier( const std::string & scrid );

    //    eScriptGroupType    type;
    //    std::deque<IdToken> tokens;
    //    std::string         name;
    //};


    /***********************************************************************************************
        ScriptInstruction
            Contains a single instruction from a script, from either EoS or EoTD.
    ***********************************************************************************************/
    struct ScriptInstruction
    {
        uint16_t             opcode;
        std::deque<uint16_t> parameters;
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
        enum struct eStrLang : size_t
        {
            english = 0,
            french  = 1,
            german  = 2,
            italian = 3,
            spanish = 4,
            NbLang,
        };
        static const size_t NbLang = static_cast<size_t>(eStrLang::NbLang);
        typedef std::deque<std::string>      strtbl_t;
        typedef std::array<strtbl_t, NbLang> strtblset_t;

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
        inline std::deque<ScriptInstrGrp>       & Groups()         { return m_groups; }
        inline const std::deque<ScriptInstrGrp> & Groups() const   { return m_groups; }

        //Returns the set of all strings for all languages
        inline strtblset_t                      & StrTblSet()         { return m_strtable; }
        inline const strtblset_t                & StrTblSet() const   { return m_strtable; }

        //Returns all strings for a specific language
        inline strtbl_t                         & StrTbl( eStrLang lang )      { return m_strtable[static_cast<size_t>(lang)]; }
        inline strtbl_t                         & StrTbl( size_t   lang )      { return m_strtable[lang]; }
        inline const strtbl_t                   & StrTbl( eStrLang lang )const { return m_strtable[static_cast<size_t>(lang)]; }
        inline const strtbl_t                   & StrTbl( size_t   lang )const { return m_strtable[lang]; }

        inline std::deque<std::string>          & ConstTbl()       { return m_contants; }
        inline const std::deque<std::string>    & ConstTbl() const { return m_contants; }



    private:
        std::string                         m_name;
        std::string                         m_originalfname;

        std::deque<ScriptInstrGrp>          m_groups;
        strtblset_t                         m_strtable; //Multiple deques for all languages
        std::deque<std::string>             m_contants;
    };

    /***********************************************************************************************
        ScriptEntityData
            Position data contained in SSA, SSS, and SSE files.
    ***********************************************************************************************/
    class ScriptEntityData
    {
    public:
        enum struct eScrDataTy
        {
            SSS,
            SSE,
            SSA,
            Invalid,
        };

        ScriptEntityData()
        {}

        ScriptEntityData(const std::string & name, const std::string & origfname, eScrDataTy datatype )
            :m_name(name), m_originalfname(origfname), m_datatype(datatype)
        {}



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

        ScriptGroup(const std::string & id, eScriptGroupType ty)
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
            m_data      = std::move(other.m_data);
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
            m_data          = std::move(other.m_data);
            return *this;
        }

        inline std::string       & Identifier()      { return m_indentifier; }
        inline const std::string & Identifier()const { return m_indentifier; }
        
        //TEMP
        inline ScriptEntityData       * Data()            {return m_data.get();}
        inline const ScriptEntityData * Data()const       {return m_data.get();}
        inline void                     SetData( ScriptEntityData* data ) {m_data.reset(data);}

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
        //scrdir : the directory of the game we want to load from/write to.
        GameScripts(const std::string & scrdir, eGameLocale loc, eGameVersion gver );
        ~GameScripts();

        //File IO
        void Load (); //Indexes all scripts. The actual sets are loaded on demand.
        void Write(); //Writes all script sets that were modified.

        //Access
        ScriptSet * AccessScriptSet( const std::string & setname );

    private:
        std::string                         m_scriptdir;
        std::map<std::string,ScriptSet>     m_sets;
        eGameLocale                         m_scrloc;
        eGameVersion                        m_gameversion;
        std::unique_ptr<GameScriptsHandler> m_pHandler;
    };
};

#endif
