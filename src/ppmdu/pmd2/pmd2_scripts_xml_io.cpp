#include "pmd2_scripts.hpp"
#include <string>
#include <cassert>

using namespace std;

namespace pmd2
{
//==============================================================================
//  Constants
//==============================================================================
    namespace scriptXML
    {
        const string ROOT_ScripDir      = "ScriptDir";      

        const string ATTR_FileName      = "name";           //Event name/filename with no extension
        
        const string NODE_ScriptData    = "ScriptData";
        const string ATTR_ScriptType    = "type";           // ssa, sse, sss
        const string NODE_;

        const string NODE_Script        = "Script";
        const string NODE_Constants     = "Constants";
        const string NODE_Strings       = "Strings";
        const string NODE_Instructions  = "Instructions";
        const string NODE_Code          = "Instruction";


    };

//==============================================================================
//  GameScriptsXMLParser
//==============================================================================

//==============================================================================
//  GameScriptsXMLWriter
//==============================================================================
    class GameScriptsXMLWriter
    {
    public:
        GameScriptsXMLWriter()
        {
        }

    private:
    };

//==============================================================================
//  GameScripts
//==============================================================================

    void ScriptSetToXML( const ScriptSet & set, eGameRegion gloc, eGameVersion gver, const std::string & destdir )
    {

    }

    ScriptSet XMLToScriptSet( const std::string & srcdir, eGameRegion & out_reg, eGameVersion & out_gver )
    {
    }

};