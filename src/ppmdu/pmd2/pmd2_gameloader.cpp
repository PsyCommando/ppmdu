#include "pmd2_gameloader.hpp"
#include <utils/utility.hpp>
#include <utils/library_wide.hpp>
#include <ppmdu/pmd2/pmd2.hpp>
#include <fstream>
using namespace std;
using utils::logutil::slog;

namespace pmd2
{
//===========================================================================================
// GameDataLoader
//===========================================================================================

    GameDataLoader::GameDataLoader( const std::string & romroot, const std::string & gamelangxml )
        :m_romroot(romroot), m_configfile(gamelangxml),m_bAnalyzed(false)
    {}

    GameDataLoader::~GameDataLoader()
    {
        slog()<<"<!>-GameDataLoader: Deallocating..\n";
        //We delete the unique ptr in order here, to avoid issues with circular ownership
        m_audio   .reset(nullptr);
        m_graphics.reset(nullptr);
        m_stats   .reset(nullptr);
        m_asmmanip.reset(nullptr);

        //Levels depend on script via shared ptr, so delete it before deleting scripts!
        m_levels  .reset(nullptr);

        m_scripts .reset();

        //Text has to be destroyed last after scripts and stats to avoid possible circular ownership lockups
        if( m_text.use_count() > 0 ) 
            slog()<<"<!>- Warning! While destroying the Gameloader object, there were still " <<m_text.use_count() <<" others owner of the GameText pointer!!\n";
        m_text.reset();
    }

    void GameDataLoader::AnalyseGame()
    {
        //Look for arm9.bin, then for the data/MESSAGE and data/BALANCE folder content
        auto filelst       = utils::ListDirContent_FilesAndDirs( m_romroot, true, true );
        bool bfoundarm9    = false;
        bool bfoundoverlay = false;
        bool bfounddata    = false;

        slog()<<"<!>-GameDataLoader: Analyzing game folder..\n";
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
            slog() <<"<!>- Couldn't find data directory under \"" <<m_romroot <<"\". Attempting to search for ROM data directory..\n";

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
                            slog() <<"<*>- ROM data directory seems to be \"" <<fpath <<"\"!\n";
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

        slog()<<"<!>-GameDataLoader: Loading configuration..\n";
        if( m_noarm9 || (!m_noarm9 && !LoadConfigUsingARM9()) )
        {
            slog()<<"<!>-GameDataLoader: Falling back to \"romfs content\" game version detection method..\n";
            //Fallback to old method of finding game version, if we don't have the arm9 handy, or if the arm9 scan didn't work
            stringstream fsroot;
            fsroot << utils::TryAppendSlash(m_romroot) <<DirName_DefData;
            auto result = DetermineGameVersionAndLocale( fsroot.str() );
            
            if( result.first != eGameVersion::Invalid && result.second != eGameRegion::Invalid )
            {
                MainPMD2ConfigWrapper::Instance().InitConfig(result.first, result.second, m_configfile);
            }
            else
                throw std::runtime_error("GameDataLoader::AnalyseGame(): Couldn't determine the version of the pmd2 ROM data!");
        }
        slog()<<"<!>-GameDataLoader: Configuration loaded!\n";

        //Compatibility check
        if( !MainPMD2ConfigWrapper::CfgInstance().GetGameVersion().issupported )
        {
            stringstream ssunsup;
            ssunsup << "<!>- WARNING: " << MainPMD2ConfigWrapper::CfgInstance().GetGameVersion().id <<" is not flagged as a supported version of the game!\n";
            const string strunsup = ssunsup.str();
            cout << strunsup;
            if(utils::LibWide().isLogOn())
                slog() << strunsup;
        }
    }

    bool GameDataLoader::LoadConfigUsingARM9()
    {
        stringstream arm9path;
        slog()<<"<!>-GameDataLoader: Comparing arm9 binary magic value at offset 0xE with XML presets..\n";
        try 
        {
            uint16_t     arm9off14 = 0;
            arm9path << utils::TryAppendSlash(m_romroot) <<FName_ARM9Bin; 
            ifstream arm9f( arm9path.str(), std::ios::in | std::ios::binary );
            arm9f.exceptions(ifstream::badbit);
            arm9f.seekg(14);
            utils::ReadIntFromBytes( arm9off14, std::istreambuf_iterator<char>(arm9f.rdbuf()), std::istreambuf_iterator<char>() );
            
            if( arm9off14 != 0 )
            {
                MainPMD2ConfigWrapper::Instance().InitConfig(arm9off14, m_configfile);
                return true;
            }
            else
            {
                slog() <<"<!>- GameDataLoader::AnalyseGame(): The 14th byte in \"" + FName_ARM9Bin + "\" file didn't match anything. Falling back to other detection method.\n";
                return false;
            }
        }
        catch(const std::exception&)
        {
            stringstream ss;
            ss << "GameDataLoader::LoadConfigUsingARM9(): Error loading arm9 \"" <<arm9path.str() 
               <<"\" and or config file \"" <<m_configfile <<"\"!";
            std::throw_with_nested( std::runtime_error(ss.str()) );
        }
    }

// ======================== Loading ========================
    //void GameDataLoader::Init()
    //{
    //    DoCommonInit();
    //    InitGameText();
    //    InitScripts(DefConfigOptions);
    //    InitLevels(Default_Level_Options);
    //    InitGraphics();
    //    InitStats();
    //    InitAudio();
    //    InitAsm();
    //}

    void GameDataLoader::DoCommonInit( bool bcheckdata )
    {
        if(!m_bAnalyzed)
            AnalyseGame();
        if( bcheckdata && m_nodata )
            throw EX_NoRomDataAvailable( "GameDataLoader::DoCommonInit(): Couldn't open or load the data directory!" );
    }

    GameText * GameDataLoader::InitGameText()
    {
        DoCommonInit();

        if( !m_text )
        {
            slog()<<"<!>-GameDataLoader: Requested loading of text data!\n";
            stringstream gamefsroot;
            gamefsroot << utils::TryAppendSlash(m_romroot) << DirName_DefData;

            if( MainPMD2ConfigWrapper::Instance().GetConfig() )
            {
                m_text.reset( new GameText( gamefsroot.str(), MainPMD2ConfigWrapper::CfgInstance() ) );
                m_text->Load();
            }
            else
                slog() <<"<!>- GameDataLoader::LoadGameText(): No game config data was loaded! Skipping on loading game text!\n";
        }
        return m_text.get();
    }

    GameScripts * GameDataLoader::InitScripts(const scriptprocoptions & options)
    {
        DoCommonInit();

        if( !m_scripts )
        {
            slog()<<"<!>-GameDataLoader: Requested loading of script data!\n";
            stringstream scriptdir;
            scriptdir << utils::TryAppendSlash(m_romroot) << DirName_DefData <<"/" <<DirName_SCRIPT;
            m_scripts.reset( new GameScripts(scriptdir.str(), MainPMD2ConfigWrapper::CfgInstance(), options) );
        }
        return m_scripts.get();
    }

    GameLevels * GameDataLoader::InitLevels(const lvlprocopts & options)
    {
        //!NOTE: Have to do this, so statsutil compiles
#ifndef PPMDU_STATSUTIL
        DoCommonInit();

        if(!m_scripts)
            InitScripts(DefConfigOptions);

        if(!m_levels)
        {
            slog()<<"<!>-GameDataLoader: Requested loading of level data!\n";
            stringstream gamefsroot;
            gamefsroot << utils::TryAppendSlash(m_romroot) << DirName_DefData;
            m_levels.reset( new GameLevels(gamefsroot.str(), MainPMD2ConfigWrapper::CfgInstance(), shared_ptr<GameScripts>(m_scripts), options) );
        }
#endif
        return m_levels.get();
    }


    GameGraphics * GameDataLoader::InitGraphics()
    {
        DoCommonInit();

        //Stuff
        slog()<<"<!>-GameDataLoader: Requested loading of graphics data!\n";
        assert(false);
        return m_graphics.get();
    }

    GameStats * GameDataLoader::InitStats()
    {
        DoCommonInit();

        //Need to load game text for this
        if( !m_text )
            InitGameText();

        if( !m_stats )
        {
            slog()<<"<!>-GameDataLoader: Requested loading of game statistics data!\n";
            m_stats.reset( new GameStats( m_romroot, GetGameVersion(), GetGameRegion(), shared_ptr<GameText>(m_text) ) );
            m_stats->Load();
        }
        return m_stats.get();
    }

    GameAudio * GameDataLoader::InitAudio()
    {
        DoCommonInit();

        //Stuff
        slog()<<"<!>-GameDataLoader: Requested loading of audio data!\n";
        assert(false);
        return m_audio.get();
    }

    PMD2_ASM * GameDataLoader::InitAsm()
    {
        DoCommonInit(false); //Don't check for data, we don't need it!

        if( m_noarm9 && m_nooverlays )
            return nullptr;

        if( !m_asmmanip )
        {
            slog()<<"<!>-GameDataLoader: Requested loading of asm data!\n";
            m_asmmanip.reset( new PMD2_ASM( m_romroot, MainPMD2ConfigWrapper::CfgInstance() ) );
        }
        return m_asmmanip.get();
    }

// ======================== DeInit ========================
    //void GameDataLoader::DeInit()
    //{
    //    DoCommonInit();

    //    if( m_text != nullptr )
    //        DeInitGameText();
    //    if (m_levels != nullptr)
    //        DeInitLevels();
    //    if( m_scripts != nullptr )
    //        DeInitScripts();
    //    if( m_graphics != nullptr )
    //        DeInitGraphics();
    //    if( m_stats != nullptr )
    //        DeInitStats();
    //    if( m_audio != nullptr )
    //        DeInitAudio();
    //    //if( m_asmmanip != nullptr )
    //    //    WriteAsm();
    //}

    void GameDataLoader::DeInitGameText()
    {
        DoCommonInit();

        if( !m_text )
        {
            slog() <<"<!>- GameDataLoader::WriteGameText(): Nothing to write!\n";
            return;
        }

        slog()<<"<!>-GameDataLoader: Requested writing of text data!\n";
        m_text->Write();
    }

    void GameDataLoader::DeInitScripts()
    {
        //NOTHING to do here
        //if(!m_bAnalyzed)
        //    AnalyseGame();

        //if( m_nodata )
        //    return;

        //if( !m_scripts )
        //{
        //    slog() <<"<!>- GameDataLoader::WriteScripts(): Nothing to write!\n";
        //    return;
        //}

        // m_scripts->Write();
    }

    void GameDataLoader::DeInitLevels()
    {
        DoCommonInit();

        slog()<<"<!>-GameDataLoader: Requested writing of level data!\n";
        //Stuff
        assert(false);
    }

    void GameDataLoader::DeInitGraphics()
    {
        DoCommonInit();

        if( !m_graphics )
            return;

        //Stuff
        assert(false);
    }

    void GameDataLoader::DeInitStats()
    {
        DoCommonInit();

        if( !m_stats )
        {
            slog() <<"<!>- GameDataLoader::WriteStats(): Nothing to write!\n";
            return;
        }
        
        slog()<<"<!>-GameDataLoader: Requested writing of game statistics data!\n";
        m_stats->Write();
    }

    void GameDataLoader::DeInitAudio()
    {
        DoCommonInit();

        if( !m_audio )
            return;

        //Stuff
        assert(false);
    }

    //void GameDataLoader::WriteAsm()
    //{
    //    if(!m_bAnalyzed)
    //        AnalyseGame();

    //    if( m_noarm9 && m_nooverlays )
    //        return;

    //    if( !m_asmmanip )
    //    {
    //        slog() <<"<!>- GameDataLoader::WriteAsm(): Nothing to write!\n";
    //        return;
    //    }

    //    //m_asmmanip->Write();
    //}

// ======================== Data Access ========================
    void                      GameDataLoader::SetRomRoot(const std::string & romroot)   { m_romroot = romroot; }
    const std::string       & GameDataLoader::GetRomRoot() const                        { return m_romroot; }

    GameText                * GameDataLoader::GetGameText()                             { return m_text.get(); }
    const GameText          * GameDataLoader::GetGameText() const                       { return m_text.get(); }

    GameScripts             * GameDataLoader::GetScripts()                              { return m_scripts.get(); }
    const GameScripts       * GameDataLoader::GetScripts() const                        { return m_scripts.get(); }

    GameLevels              * GameDataLoader::GetLevels()                               { return m_levels.get(); }
    const GameLevels        * GameDataLoader::GetLevels() const                         { return m_levels.get(); }

    GameGraphics            * GameDataLoader::GetGraphics()                             { return m_graphics.get(); }
    const GameGraphics      * GameDataLoader::GetGraphics() const                       { return m_graphics.get(); }

    GameStats               * GameDataLoader::GetStats()                                { return m_stats.get(); }
    const GameStats         * GameDataLoader::GetStats() const                          { return m_stats.get(); }

    GameAudio               * GameDataLoader::GetAudio()                                { return m_audio.get(); }
    const GameAudio         * GameDataLoader::GetAudio() const                          { return m_audio.get(); }

    PMD2_ASM                * GameDataLoader::GetAsm()                                  { return m_asmmanip.get(); }
    const PMD2_ASM          * GameDataLoader::GetAsm() const                            { return m_asmmanip.get(); }

    //const ConfigLoader * GameDataLoader::GetConfig() const
    //{
    //    //!#TODO: It would be nice to make use of weak_ptr here, as it was intended!!
    //    return MainPMD2ConfigWrapper.get();
    //}

};
