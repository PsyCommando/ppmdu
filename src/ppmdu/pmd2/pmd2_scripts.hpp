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

namespace pmd2
{

//==========================================================================================================
//  Script Data Containers
//==========================================================================================================

    /***********************************************************************************************
    ***********************************************************************************************/
    struct ScriptInstruction
    {
    };

    /***********************************************************************************************
        ScriptedSequence
            The content of a SSB file. Script instructions, strings, and constant names.
    ***********************************************************************************************/
    class ScriptedSequence
    {
    public:

    private:
        std::vector<ScriptInstruction> m_instructions;
    };


    /***********************************************************************************************
        ScriptEntityData
            Position data contained in SSA, SSS, and SSE files.
    ***********************************************************************************************/
    class ScriptEntityData
    {
    public:

    private:
    };




    /***********************************************************************************************
        ScriptGroup
            A script group is an ensemble of one or more ScriptEntityData, and one or more 
            ScriptedSequence, that share a common identifier.
    ***********************************************************************************************/
    class ScriptGroup
    {
    public:
        ScriptGroup( const std::string & id );

        inline std::string       & Identifier()      { return m_indentifier; }
        inline const std::string & Identifier()const { return m_indentifier; }

    private:
        std::string m_indentifier;
    };


    /***********************************************************************************************
        ScriptSet
            A scriptset is a group of scripts contained within a single named script folder in the 
            PMD2 games.
    ***********************************************************************************************/
    class ScriptSet
    {
    public:
        typedef std::vector<ScriptGroup>::iterator       iterator;
        typedef std::vector<ScriptGroup>::const_iterator const_iterator;

        //Constructors
        ScriptSet            ( const std::string & name );
        ScriptSet            ( const std::string & name, std::vector<ScriptGroup> && comp );
        ScriptSet            ( const ScriptSet   & other );
        ScriptSet & operator=( const ScriptSet   & other );
        ScriptSet            ( ScriptSet        && other );
        ScriptSet & operator=( ScriptSet        && other );

        //Accessors
        inline void                             Name( const std::string & name )                { m_name = name; }
        inline const std::string              & Name()const                                     { return m_name; }

        inline void                             Components( std::vector<ScriptGroup> && comp )  { m_components = std::move(comp); }
        inline const std::vector<ScriptGroup> & Components()const                               { return m_components; }

        //Std methods
        inline iterator       begin()      { return m_components.begin(); }
        inline const_iterator begin()const { return m_components.begin(); }
        inline iterator       end()        { return m_components.end();   }
        inline const_iterator end()const   { return m_components.end();   }

        //
        iterator       FindByIdentifier( const std::string & id );
        const_iterator FindByIdentifier( const std::string & id )const;

    private:
        std::string                 m_name;         //The name of the set. Ex: "D01P11A" or "COMMON"
        std::vector<ScriptGroup>    m_components;   //All scripts + data groups
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
        //scrdir : the directory of the game we want to load from/write to.
        GameScripts( const std::string & scrdir );

        //Writing/loading a single script set
        ScriptSet LoadSet ( const std::string & setname );
        void      WriteSet( const ScriptSet   & src );

        //Conversion
        void ImportScriptsFromXML( const std::string & importdir );
        void ExportScriptsToXML  ( const std::string & exportdir );

        //

    private:
        std::string m_scriptdir;
    };

};

#endif