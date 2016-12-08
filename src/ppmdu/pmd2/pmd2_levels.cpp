#include "pmd2_levels.hpp"
#include <utils/utility.hpp>
using namespace std;


namespace pmd2
{

//
//  GameLevelsHandler
//

    class GameLevelsHandler
    {
    public:
        GameLevelsHandler(const string & fsrootpath, const ConfigLoader & conf, GameScripts & gs, lvlprocopts options )
            :m_gconf(conf), m_options(options), m_gscript(gs)
        {
            m_pgscriptdat = &(m_gconf.GetGameScriptData());
            m_mapbgdir = utils::TryAppendSlash(fsrootpath) + DirName_MAP_BG;
        }

        inline void SetGameScriptData(const GameScriptData & pgsdata)
        { 
            if( m_pgscriptdat != nullptr )
                m_pgscriptdat = &pgsdata; 
            else
                m_pgscriptdat = &(m_gconf.GetGameScriptData()); //Set the default one back
        }

        //
        //  Export
        //
        void ExportLevels(const std::string & destdir)
        {
            //1. Export tilesets
            ExportTilesets(destdir);

            //2. Do scripts if needed
            if(m_options.bexportscripts)
                ExportScripts(destdir);
        }

        void ExportScripts(const std::string & destdir)
        {
        }

        void ExportTilesets(const std::string & destdir)
        {
            //1. Obtain level list
            if( m_options.lvllistsrc == eHCDataLoadSrc::ParseFromXML )
            {
                //1a. Try using the specified list of levels
                assert(false);
            }
            else if( m_options.lvllistsrc == eHCDataLoadSrc::ParseFromModdedFile )
            {
                //1b. Try loading modified files if modified
                assert(false);
            }
            else if( m_options.lvllistsrc == eHCDataLoadSrc::ParseFromBin )
            {
                //1c. Try dumping binaries + checking if modified

            }


            //2. Load bg_list.dat

            //3. Iterate level list, export each levels matching the index of the name in the BG list


        }


        //
        //  Import
        //
        void ImportLevels(const std::string & srcdir)
        {
        }

        void ImportScripts(const std::string & srcdir)
        {
        }

        void ImportTilesets(const std::string & srcdir)
        {
        }


        const ConfigLoader    & m_gconf;
        GameScripts           & m_gscript;      //Ref on gamescript handler
        const GameScriptData  * m_pgscriptdat;  //Pointer to the gamescript data to use to load the levels
        lvlprocopts             m_options;
        string                  m_mapbgdir;
    };



//
//  GameLevels
//
    GameLevels::GameLevels(const std::string & fsrootdir, const ConfigLoader & conf, GameScripts & gs, const lvlprocopts & options)
        :m_pimpl(new GameLevelsHandler(fsrootdir,conf,gs,options))
    {}

    GameLevels::~GameLevels()
    {}

    void GameLevels::ExportLevels(const std::string & destdir)
    {
        m_pimpl->ExportLevels(destdir);
    }

    void GameLevels::ImportLevels(const std::string & srcdir)
    {
        m_pimpl->ImportLevels(srcdir);
    }

    /**/
    void GameLevels::SetGameScriptData(const GameScriptData & pgsdata)
    {
        m_pimpl->SetGameScriptData(pgsdata);
    }

    const std::string & GameLevels::GetMapBgDir() const
    {
        return m_pimpl->m_mapbgdir;
    }

    const ConfigLoader & GameLevels::GetConfig() const
    {
        return m_pimpl->m_gconf;
    }

    const lvlprocopts & GameLevels::GetOptions() const
    {
        return m_pimpl->m_options;
    }
};
