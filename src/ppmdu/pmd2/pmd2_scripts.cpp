#include "pmd2_scripts.hpp"
#include <cassert>

using namespace std;

namespace pmd2
{

//==============================================================================
//  ScriptSet
//==============================================================================
    ScriptSet::ScriptSet( const std::string & name )
    {
        //#TODO
        assert(false);
    }
    ScriptSet::ScriptSet( const std::string & name, std::vector<ScriptGroup> && comp )
    {
        //#TODO
        assert(false);
    }

    ScriptSet::ScriptSet( const ScriptSet & other )
    {
        //#TODO
        assert(false);
    }

    ScriptSet & ScriptSet::operator=( const ScriptSet & other )
    {
        //#TODO
        assert(false);
        return ScriptSet("");
    }

    ScriptSet::ScriptSet( ScriptSet && other )
    {
        //#TODO
        assert(false);
    }

    ScriptSet & ScriptSet::operator=( ScriptSet && other )
    {
        //#TODO
        assert(false);
        return ScriptSet("");
    }

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