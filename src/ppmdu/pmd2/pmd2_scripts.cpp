#include "pmd2_scripts.hpp"
#include <cassert>

using namespace std;

namespace pmd2
{

//==============================================================================
//  GameScripts
//==============================================================================
    GameScripts::GameScripts( const std::string & scrdir )
        :m_scriptdir(scrdir)
    {}

    //Writing/loading a single script set
    ScriptSet GameScripts::LoadSet ( const std::string & setname )
    {
        assert(false);
        return ScriptSet("");
    }

    void GameScripts::WriteSet( const ScriptSet & src )
    {
        assert(false);
    }

    //Conversion
    //void GameScripts::ImportScriptsFromXML( const std::string & importdir );
    //void GameScripts::ExportScriptsToXML  ( const std::string & exportdir );

};