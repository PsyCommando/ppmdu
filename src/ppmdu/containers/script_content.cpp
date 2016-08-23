#include "script_content.hpp"
#include <ppmdu/pmd2/pmd2_scripts_opcodes.hpp>
#include <ppmdu/fmts/lsd.hpp>
#include <ppmdu/fmts/ssa.hpp>
#include <ppmdu/fmts/ssb.hpp>
using namespace std;


namespace pmd2
{
//==========================================================================================================
//  Constants
//==========================================================================================================
    const std::array<std::string, static_cast<size_t>(eScrDataTy::NbTypes)> ScriptDataTypeStrings = 
    {
        filetypes::SSE_FileExt,
        filetypes::SSS_FileExt,
        filetypes::SSA_FileExt,
    };

    const std::string & ScriptDataTypeToFileExtension( eScrDataTy scrdatty )
    {
        switch(scrdatty)
        {
            case eScrDataTy::SSA:
            {
                return filetypes::SSA_FileExt;
            }
            case eScrDataTy::SSE:
            {
                return filetypes::SSE_FileExt;
            }
            case eScrDataTy::SSS:
            {
                return filetypes::SSS_FileExt;
            }
            default:
            {
                throw std::runtime_error("ScriptDataTypeToFileExtension() : Invalid script data type!");
            }
        };
    }

    const std::string & ScriptDataTypeToStr(eScrDataTy scrdatty)
    {
        if( scrdatty < eScrDataTy::NbTypes )
            return ScriptDataTypeStrings[static_cast<size_t>(scrdatty)];
        else
            return Generic_Invalid;
    }

    eScrDataTy StrToScriptDataType(const std::string & scrdatstr)
    {
        for( size_t cnt = 0; cnt < ScriptDataTypeStrings.size(); ++cnt )
            if( scrdatstr == ScriptDataTypeStrings[cnt] ) return static_cast<eScrDataTy>(cnt);
        return eScrDataTy::Invalid;
    }

//==============================================================================
//  Script
//==============================================================================
    Script::Script(const Script & tocopy)
        :m_name(tocopy.m_name), /*m_originalfname(tocopy.m_originalfname),*/
         m_groups(tocopy.m_groups), m_strtable(tocopy.m_strtable),
         m_contants(tocopy.m_contants)
    {}
    
    Script::Script(Script      && tomove)
        :m_name(std::move(tomove.m_name)), /*m_originalfname(std::move(tomove.m_originalfname)),*/
         m_groups(std::move(tomove.m_groups)), m_strtable(std::move(tomove.m_strtable)),
         m_contants(std::move(tomove.m_contants))
    {}

    Script & Script::operator=( const Script & tocopy )
    {
        m_name          = tocopy.m_name;
        //m_originalfname = tocopy.m_originalfname;
        m_groups        = tocopy.m_groups;
        m_strtable      = tocopy.m_strtable;
        m_contants      = tocopy.m_contants;
        return *this;
    }

    Script & Script::operator=( Script && tomove )
    {
        m_name          = std::move(tomove.m_name);
        //m_originalfname = std::move(tomove.m_originalfname);
        m_groups        = std::move(tomove.m_groups);
        m_strtable      = std::move(tomove.m_strtable);
        m_contants      = std::move(tomove.m_contants);
        return *this;
    }

    void Script::InsertStrLanguage(eGameLanguages lang, strtbl_t && strings)
    {
        m_strtable.insert_or_assign( lang, std::forward<strtbl_t>(strings) );
    }

    inline Script::strtbl_t * Script::StrTbl(eGameLanguages lang)
    {
        auto itfound = m_strtable.find(lang);

        if( itfound !=  m_strtable.end() )
            return &(itfound->second);
        else
            return nullptr;
    }

    inline const Script::strtbl_t * Script::StrTbl(eGameLanguages lang) const
    {
        return const_cast<Script*>(this)->StrTbl(lang);
    }

/***********************************************************************************************
    ScriptSet
        A script group is an ensemble of one or more ScriptData, and one or more
        Script, that share a common identifier.
***********************************************************************************************/

    const std::string & ScriptSet::GetDataFext() const
    {
        static const string EMPTYStr;
        switch(Type())
        {
            case eScriptSetType::UNK_enter:
            {
                return filetypes::SSE_FileExt;
            }
            case eScriptSetType::UNK_station:
            {
                return filetypes::SSS_FileExt;
            }
            case eScriptSetType::UNK_acting:
            {
                return filetypes::SSA_FileExt;
            }
            default:
            {
                return EMPTYStr;
            }
        };
    }

//==============================================================================
//  LevelScript
//==============================================================================
    LevelScript::LevelScript(const std::string & name)
        :m_name(name), m_bmodified(false)
    {}

    LevelScript::LevelScript(const std::string & name, scriptgrps_t && comp, lsdtbl_t && lsdtbl)
        :m_name(name), m_lsdentries(std::move(lsdtbl)), m_components(std::move(comp)), m_bmodified(false)
    {}

    LevelScript::LevelScript(const LevelScript & other)
        :m_name(other.m_name), m_components(other.m_components), m_lsdentries(other.m_lsdentries), m_bmodified(other.m_bmodified)
    {}

    LevelScript & LevelScript::operator=(const LevelScript & other)
    {
        m_name          = other.m_name;
        m_components    = other.m_components; 
        m_lsdentries    = other.m_lsdentries;
        m_bmodified     = other.m_bmodified;
        return *this;
    }

    LevelScript::LevelScript(LevelScript && other)
    {
        m_name          = std::move(other.m_name);
        m_components    = std::move(other.m_components); 
        m_lsdentries    = std::move(other.m_lsdentries);
        m_bmodified     = other.m_bmodified;
    }

    LevelScript & LevelScript::operator=(LevelScript && other)
    {
        m_name          = std::move(other.m_name);
        m_components    = std::move(other.m_components); 
        m_lsdentries    = std::move(other.m_lsdentries);
        m_bmodified     = other.m_bmodified;
        return *this;
    }

};