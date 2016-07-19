#include "pmd2_gameloader.hpp"
#include <utils/utility.hpp>
#include <ppmdu/pmd2/pmd2.hpp>
#include <fstream>
using namespace std;

namespace pmd2
{



//===========================================================================================
// GameDataLoader
//===========================================================================================

    GameDataLoader::GameDataLoader( const std::string & romroot, const std::string & gamelangxml )
        :m_romroot(romroot), /*m_gameregion(eGameRegion::Invalid), m_gameversion(eGameVersion::Invalid),*/ m_configfile(gamelangxml),
        m_bAnalyzed(false)
    {}

    GameDataLoader::~GameDataLoader()
    {
        //We delete the unique ptr in order here, to avoid issues with circular ownership
        m_audio   .reset(nullptr);
        m_graphics.reset(nullptr);
        m_stats   .reset(nullptr);
        m_asmmanip.reset(nullptr);
        m_scripts .reset(nullptr);

        //Text has to be destroyed last to avoid possible circular ownership lockups
        if( !m_text.unique() ) 
            clog<<"<!>- Warning! While destroying the Gameloader object, there were still " <<m_text.use_count() <<" others owner of the GameText pointer!!\n";
        m_text.reset();
    }

    void GameDataLoader::AnalyseGame()
    {
        //Look for arm9.bin, then for the data/MESSAGE and data/BALANCE folder content
        auto filelst       = utils::ListDirContent_FilesAndDirs( m_romroot, true, true );
        bool bfoundarm9    = false;
        bool bfoundoverlay = false;
        bool bfounddata    = false;

        for( const auto & fname : filelst )
        {
            if( fname == DirName_DefData )
                bfounddata = true;
            else if( fname == DirName_DefOverlay )
            {
                stringstream sstoverlays;
                sstoverlays << utils::TryAppendSlash(m_romroot) <<DirName_DefOverlay; 
                auto overlaycnt = utils::ListDirContent_FilesAndDirs( sstoverlays.str(), true );
                bfoundoverlay = !overlaycnt.empty();
            }
            else if( fname == FName_ARM9Bin )
                bfoundarm9 = true;

            if( bfoundarm9 && bfoundoverlay && bfounddata )
                break;
        }

#if 0
        //If we can't find a directory named data, try to find one that contains the typical PMD2 files
        if( !bfounddata )
        {
            clog <<"<!>- Couldn't find data directory under \"" <<m_romroot <<"\". Attempting to search for ROM data directory..\n";

            //Found the directory that contains the PMD2 filetree
            auto pathlst = utils::ListDirContent_FilesAndDirs( m_romroot, false );
            for( const auto & fpath : pathlst )
            {
                if( utils::isFolder(fpath) )
                {
                    if( pmd2::AnalyzeDirForPMD2Dirs(fpath) != pmd2::eGameVersion::Invalid )
                    {
                        size_t lastslashpos = string::npos;
                        for( size_t i = 0; i < fpath.size(); ++i )
                        {
                            if( (i != (fpath.size()-1)) && fpath[i] == '/' || fpath[i] == '\\'  )
                                lastslashpos = i;
                        }

                        if( lastslashpos != string::npos )
                        {
                            clog <<"<*>- ROM data directory seems to be \"" <<fpath <<"\"!\n";
                            bfounddata = true;
                            m_datadiroverride = fpath.substr( lastslashpos+1 );
                        }
                        break;
                    }
                }
            }
        }
#endif

        //Save the result of our analysis.
        m_nodata     = !bfounddata;
        m_noarm9     = !bfoundarm9;
        m_nooverlays = !bfoundoverlay;
        m_bAnalyzed  = true;

        if( m_noarm9 || (!m_noarm9 && !LoadConfig()) )
        {
            //Fallback to old method of finding game version, if we don't have the arm9 handy, or if the arm9 scan didn't work
            stringstream fsroot;
            fsroot << utils::TryAppendSlash(m_romroot) <<DirName_DefData;
            auto result = DetermineGameVersionAndLocale( fsroot.str() );
            
            if( result.first != eGameVersion::Invalid && result.second != eGameRegion::Invalid )
                m_conf.reset( new ConfigLoader(result.first, result.second, m_configfile) );
            else
                throw std::runtime_error("GameDataLoader::AnalyseGame(): Couldn't determine the version of the pmd2 ROM data!");
        }
    }

    bool GameDataLoader::LoadConfig()
    {
        try 
        {
            stringstream arm9path;
            uint16_t     arm9off14 = 0;
            arm9path << utils::TryAppendSlash(m_romroot) <<FName_ARM9Bin; 
            ifstream arm9f( arm9path.str(), std::ios::in | std::ios::binary );
            arm9f.seekg(14);
            utils::ReadIntFromBytes( arm9off14, std::istreambuf_iterator<char>(arm9f.rdbuf()), std::istreambuf_iterator<char>() );
            
            if( arm9off14 != 0 )
            {
                m_conf.reset( new ConfigLoader(arm9off14, m_configfile) );
                return true;
            }
            else
            {
                clog <<"<!>- GameDataLoader::AnalyseGame(): The 14th short in \"" + FName_ARM9Bin + "\" file didn't match anything. Falling back to other detection method.\n";
                return false;
            }
        }
        catch(...)
        {
            //Normally, do something here
            //Maybe load default config or something?
            std::rethrow_exception( std::current_exception() );
        }
    }

// ======================== Loading ========================
    void GameDataLoader::Load()
    {
        if(!m_bAnalyzed)
            AnalyseGame();

        LoadGameText();
        LoadScripts();
        LoadGraphics();
        LoadStats();
        LoadAudio();
        LoadAsm();
    }

    GameText * GameDataLoader::LoadGameText()
    {
        if(!m_bAnalyzed)
            AnalyseGame();

        if( m_nodata )
            return nullptr;

        if( !m_text )
        {
            stringstream gamefsroot;
            gamefsroot << utils::TryAppendSlash(m_romroot) << DirName_DefData;

            if( m_conf )
            {
                m_text.reset( new GameText( gamefsroot.str(), *m_conf ) );
                m_text->Load();
            }
            else
                clog <<"<!>- GameDataLoader::LoadGameText(): No game config data was loaded! Skipping on loading game text!\n";
        }
        return m_text.get();
    }

    GameScripts * GameDataLoader::LoadScripts()
    {
        if(!m_bAnalyzed)
            AnalyseGame();

        if( m_nodata )
            return nullptr;

        if( !m_scripts )
        {
            stringstream scriptdir;
            scriptdir << utils::TryAppendSlash(m_romroot) << DirName_DefData <<"/" <<DirName_SCRIPT;
            m_scripts.reset( new GameScripts( scriptdir.str(), GetGameRegion(), GetGameVersion() ) );
        }
        return m_scripts.get();
    }

    GameGraphics * GameDataLoader::LoadGraphics()
    {
        if(!m_bAnalyzed)
            AnalyseGame();

        if( m_nodata )
            return nullptr;

        //Stuff
        assert(false);
        return m_graphics.get();
    }

    GameStats * GameDataLoader::LoadStats()
    {
        if(!m_bAnalyzed)
            AnalyseGame();

        if( m_nodata )
            return nullptr;

        //Need to load game text for this
        if( !m_text )
            LoadGameText();

        if( !m_stats )
        {
            m_stats.reset( new GameStats( m_romroot, GetGameVersion(), GetGameRegion(), shared_ptr<GameText>(m_text) ) );
            m_stats->Load();
        }
        return m_stats.get();
    }

    GameAudio * GameDataLoader::LoadAudio()
    {
        if(!m_bAnalyzed)
            AnalyseGame();

        if( m_nodata )
            return nullptr;

        //Stuff
        assert(false);
        return m_audio.get();
    }

    PMD2_ASM_Manip * GameDataLoader::LoadAsm()
    {
        if(!m_bAnalyzed)
            AnalyseGame();

        if( m_noarm9 && m_nooverlays )
            return nullptr;

        //if( !m_asmmanip )
        //{
        //    m_asmmanip.reset( new PMD2_ASM_Manip( ASM_Data_Loader(m_romroot), GetGameVersion(), GetGameRegion() ) );
        //    m_asmmanip->Load();
        //}
        return m_asmmanip.get();
    }

// ======================== Writing ========================
    void GameDataLoader::Write()
    {
        if(!m_bAnalyzed)
            AnalyseGame();

        if( m_text != nullptr )
            WriteGameText();
        if( m_scripts != nullptr )
            WriteScripts();
        if( m_graphics != nullptr )
            WriteGraphics();
        if( m_stats != nullptr )
            WriteStats();
        if( m_audio != nullptr )
            WriteAudio();
        if( m_asmmanip != nullptr )
            WriteAsm();
    }

    void GameDataLoader::WriteGameText()
    {
        if(!m_bAnalyzed)
            AnalyseGame();

        if( m_nodata )
            return;

        if( !m_text )
        {
            clog <<"<!>- GameDataLoader::WriteGameText(): Nothing to write!\n";
            return;
        }

        m_text->Write();
    }

    void GameDataLoader::WriteScripts()
    {
        //NOTHING to do here
        //if(!m_bAnalyzed)
        //    AnalyseGame();

        //if( m_nodata )
        //    return;

        //if( !m_scripts )
        //{
        //    clog <<"<!>- GameDataLoader::WriteScripts(): Nothing to write!\n";
        //    return;
        //}

        // m_scripts->Write();
    }

    void GameDataLoader::WriteGraphics()
    {
        if(!m_bAnalyzed)
            AnalyseGame();

        if( m_nodata )
            return;

        if( !m_graphics )
            return;

        //Stuff
        assert(false);
    }

    void GameDataLoader::WriteStats()
    {
        if(!m_bAnalyzed)
            AnalyseGame();

        if( m_nodata )
            return;

        if( !m_stats )
        {
            clog <<"<!>- GameDataLoader::WriteStats(): Nothing to write!\n";
            return;
        }
        
        m_stats->Write();
    }

    void GameDataLoader::WriteAudio()
    {
        if(!m_bAnalyzed)
            AnalyseGame();

        if( m_nodata )
            return;

        if( !m_audio )
            return;

        //Stuff
        assert(false);
    }

    void GameDataLoader::WriteAsm()
    {
        if(!m_bAnalyzed)
            AnalyseGame();

        if( m_noarm9 && m_nooverlays )
            return;

        if( !m_asmmanip )
        {
            clog <<"<!>- GameDataLoader::WriteAsm(): Nothing to write!\n";
            return;
        }

        //m_asmmanip->Write();
    }

// ======================== Data Access ========================
    void                      GameDataLoader::SetRomRoot(const std::string & romroot)   { m_romroot = romroot; }
    const std::string       & GameDataLoader::GetRomRoot() const                        { return m_romroot; }

    GameText                * GameDataLoader::GetGameText()                             { return m_text.get(); }
    const GameText          * GameDataLoader::GetGameText() const                       { return m_text.get(); }

    GameScripts             * GameDataLoader::GetScripts()                              { return m_scripts.get(); }
    const GameScripts       * GameDataLoader::GetScripts() const                        { return m_scripts.get(); }

    GameGraphics            * GameDataLoader::GetGraphics()                             { return m_graphics.get(); }
    const GameGraphics      * GameDataLoader::GetGraphics() const                       { return m_graphics.get(); }

    GameStats               * GameDataLoader::GetStats()                                { return m_stats.get(); }
    const GameStats         * GameDataLoader::GetStats() const                          { return m_stats.get(); }

    GameAudio               * GameDataLoader::GetAudio()                                { return m_audio.get(); }
    const GameAudio         * GameDataLoader::GetAudio() const                          { return m_audio.get(); }

    PMD2_ASM_Manip          * GameDataLoader::GetAsmManip()                             { return m_asmmanip.get(); }
    const PMD2_ASM_Manip    * GameDataLoader::GetAsmManip() const                       { return m_asmmanip.get(); }

    const ConfigLoader * GameDataLoader::GetConfig() const
    {
        return nullptr;
    }

};
