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
#include <ppmdu/containers/script_content.hpp>
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
    const std::string ScriptCompilerReportFname  = "CompilerReportFilename"; //KeyName of the compiler report filename in the libwide data!
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
    extern const std::regex MatchScriptFileTypes;       //Matches all ssa,ssb,sss, and lsd files
    extern const std::string ScriptRegExNameNumSSBSuff; //Regex to suffix to the basename to get the pattern for the numbered SSBs

    /*
        scriptprocoptions
            Set of options to be passed to script handling functions.
    */
    struct scriptprocoptions
    {
        bool bescapepcdata;     //Whether we allow the xml writer to escape characters to its discretion.
        bool bnodeisinst;       //Whether the node name should be set to the instruction's name during export
        bool bmarkoffsets;      //Whether the offsets of each instructions should be marked by comments
        bool bscriptdebug;      //Whether the debug_branch instructions should be tweaked to work as if debug mode was on
        bool basdir;            //Whether the scripts' XML data is exported/imported to/from a directory containing sub-files if true, or a single XML file if false.
    };
    const scriptprocoptions DefConfigOptions{true, true, false, false, false};

//==========================================================================================================
//  Script Manager/Loader
//==========================================================================================================

    //! #TODO: The purpose  of this should be reviewed and insisted on. Its for transparently accesing
    //!        indexed script files in the rom's directory, and triggering export and
    //!        importing over some of these.
    //!        It also acts as a way to access relevant script data at runtime. Such as the COMMON
    //!        script.
    //!        We definitely do not want this to load all the scripts in memory.
    /***********************************************************************************************
        GameScripts
            Indexes all scripts from a PMD2 game by event/map name, for easy retrieval and access.

            #TODO: Actually indexes things and classify them, once we know more about the format!
    ***********************************************************************************************/
    class GameScripts
    {
        friend class GameScriptsHandler;
    public:
        friend class ScrSetLoader;
        friend void ImportXMLGameScripts(const std::string & dir, GameScripts & out_dest, const scriptprocoptions & options );
        friend void ExportGameScriptsXML(const std::string & dir, const GameScripts & gs, const scriptprocoptions & options );

        typedef std::unordered_map<std::string,ScrSetLoader> evindex_t;
        typedef evindex_t::iterator                          iterator;
        typedef evindex_t::const_iterator                    const_iterator;

        //scrdir : the directory of the game we want to load from/write to.
        GameScripts(const std::string & scrdir, const ConfigLoader & conf, const scriptprocoptions & options );
        ~GameScripts();

        //File IO
        void Load(); //Indexes all scripts in the src dir. The actual sets are loaded on demand.
        void ImportXML(const std::string & dir);
        void ExportXML(const std::string & dir); 

        std::unordered_map<std::string,LevelScript> LoadAll();
        void                                      WriteAll( const std::unordered_map<std::string,LevelScript> & stuff );

        eGameRegion  Region ()const;
        eGameVersion Version()const;

        //Access
        void      WriteScriptSet( const LevelScript & set ); //Add new script set or overwrite existing

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

        inline const std::string       & GetScriptDir()const{return m_scriptdir;}
        inline const ConfigLoader      & GetConfig()const   {return m_gconf;}
        inline const scriptprocoptions & GetOptions()const  {return m_options;}

    private:
        LevelScript LoadScriptSet ( const std::string & setname );

        std::string                                  m_scriptdir;
        LevelScript                                  m_common;           //We probably should make this its own type!!
        evindex_t                                    m_setsindex;                //All sets known to exist
        //eGameRegion                                  m_scrRegion;
        //eGameVersion                                 m_gameVersion;
        std::unique_ptr<GameScriptsHandler>          m_pHandler;
        //std::mutex                                   m_mutex;
        //const LanguageFilesDB                      * m_langdat;
        scriptprocoptions                            m_options;
        //bool                                         m_escapexml;
        //bool                                         m_bbscriptdebug;
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

        inline const std::string & path()const {return m_path;}

    private:
        std::string   m_path;
        GameScripts * m_parent;
    };


//====================================================================================
//  Import/Export
//====================================================================================

    /*
        ScriptXML
            Interface for handling conversion to and from XML for script data.
    */
    //! #TODO: finish designing this.
#if 0
    class ScriptXML
    {
    public:
        ScriptXML( const ConfigLoader & gconf, config & exportcfg );

        void Export( const LevelScript & lvlscript, const std::string & destdir );
        void Export( const Script      & scr,       const std::string & destdir );
        void Export( const ScriptData  & dat,       const std::string & destdir );

        LevelScript ImportLevelScript(const std::string & srcfile);
        Script      ImportScript     (const std::string & srcfile);
        ScriptData  ImportScriptData (const std::string & srcfile);
    };
#endif



    //bautoescapexml : if set to true, pugixml will escape any reserved/special characters.
    void      ScriptSetToXML( const LevelScript         & set,  
                              const ConfigLoader        & gconf, 
                              const scriptprocoptions   & options, 
                              const std::string         & destdir );

    LevelScript XMLToScriptSet( const std::string   & srcdir, 
                                eGameRegion         & out_greg, 
                                eGameVersion        & out_gver, 
                                const ConfigLoader  & gconf );

    //Script Single File Export
    void ScriptToXML( const Script              & scr, 
                      const ConfigLoader        & gconf, 
                      const scriptprocoptions   & options, 
                      const std::string         & destdir );

    Script XMLToScript( const std::string   & srcfile, 
                        eGameRegion         & out_greg, 
                        eGameVersion        & out_gver,
                        const ConfigLoader  & gconf );

    //Script Data Single File Export
    void ScriptDataToXML( const ScriptData          & dat, 
                          const ConfigLoader        & gconf, 
                          const scriptprocoptions   & options,
                          const std::string         & destdir );

    ScriptData XMLToScriptData( const std::string   & srcfile, 
                                eGameRegion         & out_greg, 
                                eGameVersion        & out_gver, 
                                const ConfigLoader  & gconf );


};

#endif
