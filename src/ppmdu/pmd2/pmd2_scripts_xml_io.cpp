#include "pmd2_scripts.hpp"
#include <string>
#include <cassert>
#include <deque>
#include <utils/utility.hpp>
#include <ppmdu/pmd2/pmd2_scripts.hpp>
#include <ppmdu/pmd2/pmd2_scripts_opcodes.hpp>
using namespace std;

namespace pmd2
{
//==============================================================================
//  Constants
//==============================================================================
    namespace scriptXML
    {
        const string ROOT_ScripDir      = "ScriptDir";      
        const string ATTR_GVersion      = "gameversion"; 
        const string ATTR_GRegion       = "gameregion";
        const string ATTR_DirName       = "eventname";           //Event name/filename with no extension
        
        const string NODE_ScriptData    = "ScriptData";
        const string ATTR_ScriptType    = "type";           // ssa, sse, sss
        const string NODE_;
        const string ATTR_Name          = "name";

        const string NODE_Script        = "Script";
        const string NODE_Constants     = "Constants";
        const string NODE_Constant      = "Constant";

        const string NODE_Strings       = "Strings";
        const string NODE_String        = "String";
        const string ATTR_Value         = "value";

        const string NODE_Code          = "Code";
        const string NODE_Group         = "Group";
        const string ATTR_Type          = "type";
        const string NODE_Instruction   = "Instruction";
        const string ATTR_Param1        = "param1";
        const string ATTR_Param2        = "param2";
        const string ATTR_Param3        = "param3";
        const string ATTR_Param4        = "param4";
        const string ATTR_Param5        = "param5";
        const string ATTR_Param6        = "param6";


    };

//==============================================================================
//  GameScriptsXMLParser
//==============================================================================
    class GameScriptsXMLParser
    {
    public:
        GameScriptsXMLParser(eGameRegion & out_reg, eGameVersion & out_gver)
            :m_out_reg(out_reg), m_out_gver(out_gver)
        {}

        ScriptSet Parse(const std::string & srcdir)
        {
            return ScriptSet( utils::GetBaseNameOnly(srcdir), 
                             std::forward<ScriptSet::scriptgrps_t>(ParseGroups()),
                             std::forward<ScriptSet::lsdtbl_t>(ParseLSD()) );
        }

    private:

        ScriptSet::scriptgrps_t ParseGroups()
        {
            ScriptSet::scriptgrps_t groups;

            assert(false);

            return std::move(groups);
        }

        ScriptSet::lsdtbl_t ParseLSD()
        {
            ScriptSet::lsdtbl_t table;

            assert(false);

            return std::move(table);
        }

    private:
        eGameRegion  & m_out_reg;
        eGameVersion & m_out_gver;
    };


//==============================================================================
//  GameScriptsXMLWriter
//==============================================================================
    class GameScriptsXMLWriter
    {
    public:
        GameScriptsXMLWriter(const ScriptSet & set, eGameRegion gloc, eGameVersion gver)
        {
        }

        void Write(const std::string & destdir)
        {
            assert(false);
        }

    private:
    };

//==============================================================================
//  GameScripts
//==============================================================================

    void ScriptSetToXML( const ScriptSet & set, eGameRegion gloc, eGameVersion gver, const std::string & destdir )
    {
        GameScriptsXMLWriter(set, gloc, gver).Write(destdir);
    }

    ScriptSet XMLToScriptSet( const std::string & srcdir, eGameRegion & out_reg, eGameVersion & out_gver )
    {
        return std::move( GameScriptsXMLParser(out_reg, out_gver).Parse(srcdir) );
    }

};