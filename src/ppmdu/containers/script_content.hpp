#ifndef SCRIPT_CONTENT_HPP
#define SCRIPT_CONTENT_HPP
/*
script_content.hpp
2016/08/03
psycommando@gmail.com
    Description: Data container for processing and handling scripts from PMD2.
*/
#include <ppmdu/pmd2/pmd2.hpp>
#include <ppmdu/pmd2/pmd2_configloader.hpp>
#include <cstdint>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <deque>

namespace pmd2
{
//==========================================================================================================
//  Constants
//==========================================================================================================
    /**********************************************************************************
        Script Type
    **********************************************************************************/
    enum struct eScriptSetType 
    {
        INVALID,
        UNK_unionall,   //Scripts not in a LSD, found under the "COMMON" directory. Has no other component. 
        UNK_enter,      //Scripts for accessible levels.
        UNK_acting,     //Scripts mainly for cutscenes
        UNK_station,    //Scripts not in a LSD, file extension of data file is SSS, may have matched numbered ssb.
        NbTypes,
    };

    /*
        Script Data Type
            For identifying the purpose of a script data file.
    */
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

    /*
        eInstructionType
            This is meant to help determine how to process and format some instructions,
            and differentiate between instructions we've added solely to serves our needs, 
            and actual instructions to be placed in the target file.
    */
    enum struct eInstructionType : uint8_t
    {
        Command,            //For most instructions

        //Meta Labels
        MetaLabel,          //A jump label
        MetaCaseLabel,      //A jump label for a conditional case //#REMOVEME

        //Meta instructions
        MetaSwitch,         //The instruction contains case sub-instructions
        MetaAccessor,       //The instruction contains a sub-instruction to be applied to what the accessor is accessing
        MetaReturnCases,    //For instructions that can have a set of cases applied to their return value


        //Must be last
        NbTypes,
        Invalid,
    };

//==========================================================================================================
//  Script Instructions
//==========================================================================================================
    //! #TODO: Make this a single class, and get rid of inheritance. We could just make a wrapper around the instruction that contains both and instruction and a list of sub instruction, to remedy the problem.
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
        size_t              dbg_origoffset; //Original offset of the instruction in the source file if applicable. For debugging purpose
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
            dbg_origoffset = cp.dbg_origoffset; 
            return *this;
        }

        ScriptInstruction & operator=(ScriptInstruction && mv)
        {
            subinst     = std::move(mv.subinst);
            type        = mv.type;
            parameters  = std::move(mv.parameters);
            value       = mv.value;
            dbg_origoffset = mv.dbg_origoffset; 
            return *this;
        }

        ScriptInstruction & operator=(const ScriptBaseInstruction & cp)
        {
            type        = cp.type;
            parameters  = cp.parameters;
            value       = cp.value;
            dbg_origoffset = cp.dbg_origoffset; 
            return *this;
        }

        ScriptInstruction & operator=(ScriptBaseInstruction && mv)
        {
            type        = mv.type;
            parameters  = std::move(mv.parameters);
            value       = mv.value;
            dbg_origoffset = mv.dbg_origoffset; 
            return *this;
        }

        operator ScriptBaseInstruction()const
        {
            ScriptBaseInstruction out;
            out.parameters  = parameters;
            out.value       = value;
            out.type        = type;
            out.dbg_origoffset = dbg_origoffset; 
            return std::move(out);
        }
    };


//==========================================================================================================
//  Script Routine
//==========================================================================================================

    /***********************************************************************************************
        ScriptRoutine
            Contains a groupd of instructions.
    ***********************************************************************************************/
    struct ScriptRoutine
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
        uint16_t parameter;
    };

//==========================================================================================================
//  Script Container
//==========================================================================================================
    /***********************************************************************************************
        Script
            The content of a SSB file. Script instructions, strings, and constant names.
    ***********************************************************************************************/
    class Script
    {
    public:
        static const size_t                                     NbLang = static_cast<size_t>(eGameLanguages::NbLang);
        typedef std::vector<std::string>                        strtbl_t;
        typedef std::vector<std::string>                        consttbl_t;
        typedef std::deque<ScriptRoutine>                       grptbl_t;
        typedef std::unordered_map<eGameLanguages, strtbl_t>    strtblset_t;

        Script(){}
        Script(const std::string & name)
            :m_name(name)
        {}

        Script(const Script & tocopy);
        Script(Script      && tomove);
        Script & operator=( const Script & tocopy );
        Script & operator=( Script       && tomove );

        inline const std::string                & Name       ()const                        { return m_name; }
        inline void                               SetName    ( const std::string & name )   { m_name = name; }

        //!#TODO: Encapsulate those later.      
        inline grptbl_t                         & Routines()         { return m_groups; }
        inline const grptbl_t                   & Routines() const   { return m_groups; }

        //Returns the set of all strings for all languages
        inline strtblset_t                      & StrTblSet()         { return m_strtable; }
        inline const strtblset_t                & StrTblSet() const   { return m_strtable; }

        void                                      InsertStrLanguage( eGameLanguages lang, strtbl_t && strings );
        //Returns all strings for a specific language
        inline strtbl_t                         * StrTbl( eGameLanguages lang );
        inline const strtbl_t                   * StrTbl( eGameLanguages lang )const;

        inline consttbl_t          & ConstTbl()       { return m_contants; }
        inline const consttbl_t    & ConstTbl() const { return m_contants; }

    private:
        std::string m_name;
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
        int16_t facing  = 0;
        int16_t xoff    = 0;
        int16_t yoff    = 0;
        int16_t unk4    = 0;
        int16_t unk5    = 0;
        int16_t scrid   = 0;
    };

    struct ObjectDataEntry
    {
        int16_t objid   = 0;
        int16_t facing  = 0;
        int16_t width   = 0;
        int16_t height  = 0;
        int16_t xoff    = 0;
        int16_t yoff    = 0;
        int16_t unk6    = 0;
        int16_t unk7    = 0;
        int16_t scrid   = 0;
    };

    struct PerformerDataEntry
    {
        int16_t type    = 0; //Value from 0 to 5
        int16_t facing  = 0; 
        int16_t unk2    = 0;
        int16_t unk3    = 0;
        int16_t xoff    = 0;
        int16_t yoff    = 0;
        int16_t unk6    = 0;
        int16_t unk7    = 0;
        //int16_t unk8    = 0;
        //int16_t unk9    = 0;
    };

    struct EventDataEntry
    {
        int16_t width       = 0;
        int16_t height      = 0;
        int16_t xoff        = 0;
        int16_t yoff        = 0;
        int16_t unk4        = 0;
        int16_t unk5        = 0;
        int16_t actionidx   = 0;
    };

    struct PosMarkDataEntry
    {
        int16_t xoff = 0;
        int16_t yoff = 0;   
        int16_t unk2 = 0;   
        int16_t unk3 = 0;  
        int16_t unk4 = 0;  
        int16_t unk5 = 0;  
        int16_t unk6 = 0;
        int16_t unk7 = 0;
    };

    struct ActionDataEntry
    {
        int16_t croutineid = 0;
        int16_t unk1 = 0;   
        int16_t unk2 = 0;   
        int16_t scrid = 0;  
    };

    struct ScriptLayer
    {
        std::vector<LivesDataEntry>     lives;
        std::vector<ObjectDataEntry>    objects;
        std::vector<PerformerDataEntry> performers;
        std::vector<EventDataEntry>     events;
    };

    /***********************************************************************************************
        ScriptData
            Data contained in SSA, SSS, and SSE files.
    ***********************************************************************************************/
    class ScriptData
    {
    public:
        typedef std::vector<ScriptLayer>      layers_t;
        typedef std::vector<PosMarkDataEntry> posmarks_t;
        typedef std::vector<ActionDataEntry> trgentry_t;

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

        inline trgentry_t        & ActionTable()         {return m_trgtbl;}
        inline const trgentry_t  & ActionTable()const    {return m_trgtbl;}

    private:
        std::string     m_name;
        layers_t        m_layers;
        posmarks_t      m_posmarkers;
        trgentry_t      m_trgtbl;
        eScrDataTy      m_datatype;
    };

//==========================================================================================================
//  Script Set
//==========================================================================================================

    /***********************************************************************************************
        ScriptSet
            A script set is an ensemble of one or no ScriptData, and none or more
            Scripts, that share a common identifier.
    ***********************************************************************************************/
    class ScriptSet
    {
    public:
        typedef std::unique_ptr<ScriptData>  dataptr_t;
        typedef std::map<std::string,Script> seqtbl_t;
        typedef seqtbl_t::const_iterator     const_seqtbl_iter_t;
        typedef seqtbl_t::iterator           seqtbl_iter_t;

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
        void                Type(eScriptSetType newty)   { m_type = newty; } 

        //Returns the file extension of the data file for this group if applicable.
        // otherwise, returns an empty string.
        const std::string & GetDataFext()const;

    private:
        std::string     m_indentifier;  //The identifier for this set. AKA the filename prefix all its components share.
        dataptr_t       m_data;         //Contains SSA, SSS, or SSE files. There can be 0 or more.
        seqtbl_t        m_sequences;    //Contains SSB data.
        eScriptSetType  m_type;         //Type of the set. Is usually tied to the data file, but not always.
    };

//==========================================================================================================
//  Script Level Data
//==========================================================================================================

    /***********************************************************************************************
        LevelScript
            A group of scripts contained within a single named script folder in the
            PMD2 games. Each of those are tied to a specific level.
    ***********************************************************************************************/
    class LevelScript
    {
    public:
        typedef std::array<char,8>           lsdtblentry_t;
        typedef std::deque<lsdtblentry_t>    lsdtbl_t;
        typedef std::deque<ScriptSet>        scriptsets_t;
        typedef scriptsets_t::iterator       iterator;
        typedef scriptsets_t::const_iterator const_iterator;

        //Constructors
        LevelScript            (const std::string & name);
        LevelScript            (const std::string & name, scriptsets_t && sets, lsdtbl_t && lsdtbl );
        LevelScript            (const LevelScript   & other);
        LevelScript & operator=(const LevelScript   & other);
        LevelScript            (LevelScript        && other);
        LevelScript & operator=(LevelScript        && other);

        //Accessors
        inline void                  Name(const std::string & name)   { m_name = name; }
        inline const std::string   & Name()const                      { return m_name; }

        inline void                  Components(scriptsets_t && comp) { m_components = std::move(comp); }
        inline const scriptsets_t  & Components()const                { return m_components; }
        inline scriptsets_t        & Components()                     { return m_components; }

        inline lsdtbl_t            & LSDTable()                       {return m_lsdentries;}
        inline const lsdtbl_t      & LSDTable()const                  {return m_lsdentries;}

        //Std methods
        inline iterator              begin()      { return m_components.begin(); }
        inline const_iterator        begin()const { return m_components.begin(); }
        inline iterator              end  ()      { return m_components.end(); }
        inline const_iterator        end  ()const { return m_components.end(); }


    private:
        std::string  m_name;         //The name of the set. Ex: "D01P11A" or "COMMON"
        scriptsets_t m_components;   //All scripts + data groups (SSA,SSS,SSE + SSB)
        lsdtbl_t     m_lsdentries;   //Entries in the LSD table Stored here for now
        bool         m_bmodified;    //Whether the content of this set was modified
    };
};

#endif
