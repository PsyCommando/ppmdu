#include "pmd2_levels.hpp"
#include <utils/utility.hpp>
#include <ppmdu/fmts/bpc.hpp>
#include <ppmdu/fmts/bpa.hpp>
#include <ppmdu/fmts/bpl.hpp>
#include <ppmdu/fmts/bma.hpp>
#include <ppmdu/fmts/bg_list_data.hpp>
#include <ppmdu/containers/level_tileset.hpp>
using namespace std;


namespace pmd2
{

    const std::string LevelData_LevelDefFname = "leveldef.xml";
    const std::string LevelDataXMLRoot        = "LevelDefinition";

//=========================================================================================================================================
//  GameLevelsHandler
//=========================================================================================================================================

    class GameLevels::GameLevelsHandler
    {
    public:
        GameLevelsHandler(const string & fsrootpath, const ConfigLoader & conf, shared_ptr<GameScripts> && gs, lvlprocopts options )
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
        void ExportAllLevels(const std::string & destdir)
        {
            //1. Export tilesets
            ExportTilesets(destdir);

            //2. Do scripts if needed
            //if(m_options.bexportscripts)
                //ExportScripts(destdir);
        }

        void ExportLevel(const std::string & destdir, const std::string & levelname)
        {
            assert(false);
        }

        void ExportScripts(const std::string & destdir)
        {
            assert(false);
        }

        void ExportTilesets(const std::string & destdir)
        {
            //1. Obtain level list
            const GameScriptData::lvlinf_t & lvlinf = m_gconf.GetGameScriptData().LevelInfo();

            //2. Load bg_list.dat
            stringstream sstrbglist;
            sstrbglist << utils::TryAppendSlash(m_mapbgdir) <<filetypes::FName_BGListFile;
            filetypes::lvlbglist_t bglist(filetypes::LoadLevelList( sstrbglist.str() ));
            cout <<"Loaded " <<filetypes::FName_BGListFile <<"file..\n";

            //3. Iterate level list, export each levels matching the index of the name in the BG list
            for( const pmd2::level_info & lvl : lvlinf )
            {
                if( lvl.mapid < bglist.size() )
                {
                    stringstream sstrtsetpath;
                    sstrtsetpath << utils::TryAppendSlash(destdir) <<lvl.name;
                    string tsetpath = sstrtsetpath.str();

                    cout <<"\rExporting " <<left <<setw(10) <<setfill(' ') <<lvl.name <<"..";
                    utils::DoCreateDirectory(tsetpath);
                    ExportATileset( lvl, bglist[lvl.mapid], tsetpath );
                }
                else
                {
                    //The map id refers to a map out of bound!
                    stringstream sstrer;
                    sstrer << "GameLevelHandler::ExportTilesets(): Error processing level \"" <<lvl.name <<"\"! Map id " 
                           <<lvl.mapid <<" is outside the \"" <<filetypes::FName_BGListFile <<"\" level list file's range!!";
                    assert(false);
                    throw std::runtime_error(sstrer.str());
                }
            }

        }

        void ExportATileset( const pmd2::level_info & lvlinf, const filetypes::LevelBgEntry & entry, const std::string & destdir )
        {
            Tileset tset( LoadTileset(m_mapbgdir, entry, lvlinf) );
            ExportTilesetToRaw(destdir, lvlinf.name, tset);
            //PrintAssembledTilesetPreviewToPNG(utils::TryAppendSlash(destdir) + "preview", tset );
            DumpCellsToPNG( destdir, tset );
        }


        //
        //  Import
        //
        void ImportAllLevels(const std::string & srcdir)
        {
            assert(false);
        }

        void ImportLevel(const std::string & srclvldir)
        {
            assert(false);
        }

        void ImportScripts(const std::string & srcdir)
        {
            assert(false);
        }

        void ImportTilesets(const std::string & srcdir)
        {
            assert(false);
        }


        bool DirContainsXMLLevelDef(const std::string & srclvldir)
        {
            stringstream sstr; 
            sstr << utils::TryAppendSlash(srclvldir) <<LevelData_LevelDefFname;

            //!#TODO: make a more elaborate check!
            if( utils::pathExists(sstr.str()) )
                return true;

            return false;
        }

        //
        inline const std::string & GetMapBgDir() const
        {
            return m_mapbgdir;
        }

        inline const ConfigLoader & GetConfig() const
        {
            return m_gconf;
        }

        inline const lvlprocopts & GetOptions() const
        {
            return m_options;
        }

        const ConfigLoader    & m_gconf;
        shared_ptr<GameScripts> m_gscript;      //Ref on gamescript handler
        const GameScriptData  * m_pgscriptdat;  //Pointer to the gamescript data to use to load the levels
        lvlprocopts             m_options;
        string                  m_mapbgdir;
    };



//=========================================================================================================================================
//  GameLevels
//=========================================================================================================================================
    GameLevels::GameLevels(const std::string & fsrootdir, const ConfigLoader & conf, std::shared_ptr<GameScripts> && gs, const lvlprocopts & options)
        :m_pimpl(new GameLevelsHandler(fsrootdir,conf,std::forward<std::shared_ptr<GameScripts>>(gs),options))
    {}

    GameLevels::~GameLevels()
    {}

    void GameLevels::ExportAllLevels(const std::string & destdir)
    {
        m_pimpl->ExportAllLevels(destdir);
    }

    void GameLevels::ExportLevel(const std::string & destdir, const std::string & levelname)
    {
        m_pimpl->ExportLevel(destdir, levelname);
    }

    void GameLevels::ImportAllLevels(const std::string & srcdir)
    {
        m_pimpl->ImportAllLevels(srcdir);
    }

    void GameLevels::ImportLevel(const std::string & srclvldir)
    {
        m_pimpl->ImportLevel(srclvldir);
    }

    bool GameLevels::DirContainsXMLLevelDef(const std::string & srclvldir)
    {
        return m_pimpl->DirContainsXMLLevelDef(srclvldir);
    }

    void GameLevels::SetGameScriptData(const GameScriptData & pgsdata)
    {
        m_pimpl->SetGameScriptData(pgsdata);
    }

    const std::string & GameLevels::GetMapBgDir() const
    {
        return m_pimpl->GetMapBgDir();
    }

    const ConfigLoader & GameLevels::GetConfig() const
    {
        return m_pimpl->GetConfig();
    }

    const lvlprocopts & GameLevels::GetOptions() const
    {
        return m_pimpl->GetOptions();
    }
};
