#include "pmd2_gameloader.hpp"
#include <utils/utility.hpp>
#include <ppmdu/pmd2/pmd2.hpp>
using namespace std;

namespace pmd2
{



//===========================================================================================
// GameDataLoader
//===========================================================================================

    GameDataLoader::GameDataLoader( const std::string & romroot )
        :m_romroot(romroot), m_gamelocale(eGameLocale::Invalid), m_gameversion(eGameVersion::Invalid)
    {}

    GameDataLoader::~GameDataLoader()
    {}

    void GameDataLoader::AnalyseGame()
    {
        //Look for arm9.bin, then for the data/MESSAGE and data/BALANCE folder content
        auto filelst       = utils::ListDirContent_FilesAndDirs( m_romroot, true );
        bool bfoundarm9    = false;
        bool bfoundoverlay = false;
        bool bfounddata    = false;

        for( const auto & fname : filelst )
        {
            if( fname == DirName_DefData )
                bfounddata = true;
            else if( fname == DirName_DefOverlay )
                bfoundoverlay = true;
            else if( fname == FName_ARM9Bin )
                bfoundarm9 = true;

            if( bfoundarm9 && bfoundoverlay && bfounddata )
                break;
        }

        if( !bfounddata )
        {
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
                            m_datadiroverride = fpath.substr( lastslashpos+1 );
                        break;
                    }
                }
            }
        }

        //
        assert(false);
    }

// ======================== Loading ========================
    void GameDataLoader::Load()
    {
        AnalyseGame();
        //Stuff
        assert(false);
    }

    GameText * GameDataLoader::LoadGameText()
    {
        AnalyseGame();
        if( m_text == nullptr )
            m_text.reset( new GameText );

        //Stuff
        assert(false);
        return m_text.get();
    }

    GameScripts * GameDataLoader::LoadScripts()
    {
        AnalyseGame();
        if( m_scripts == nullptr )
            m_scripts.reset( new GameScripts( m_romroot, m_gamelocale, m_gameversion ) );

        //Stuff
        assert(false);
        return m_scripts.get();
    }

    GameGraphics * GameDataLoader::LoadGraphics()
    {
        AnalyseGame();
        //Stuff
        assert(false);
        return m_graphics.get();
    }

    stats::GameStats * GameDataLoader::LoadStats()
    {
        AnalyseGame();
        //Stuff
        assert(false);
        return m_stats.get();
    }

    GameAudio * GameDataLoader::LoadAudio()
    {
        AnalyseGame();
        //Stuff
        assert(false);
        return m_audio.get();
    }

    PMD2_ASM_Manip * GameDataLoader::LoadAsm()
    {
        AnalyseGame();
        //Stuff
        assert(false);
        return m_asmmanip.get();
    }

// ======================== Writing ========================
    void GameDataLoader::Write()
    {
        AnalyseGame();
        //Stuff
        assert(false);
    }

    void GameDataLoader::WriteGameText()
    {
        AnalyseGame();
        if( m_text == nullptr )
            throw std::runtime_error("GameDataLoader::WriteGameText() : No game text to write!!");

        //Stuff
        assert(false);
    }

    void GameDataLoader::WriteScripts()
    {
        AnalyseGame();
        if( m_text == nullptr )
            throw std::runtime_error("GameDataLoader::WriteScripts() : No game scripts to write!!");

        //Stuff
        assert(false);
    }

    void GameDataLoader::WriteGraphics()
    {
        AnalyseGame();
        if( m_text == nullptr )
            throw std::runtime_error("GameDataLoader::WriteGraphics() : No game graphics to write!!");

        //Stuff
        assert(false);
    }

    void GameDataLoader::WriteStats()
    {
        AnalyseGame();
        if( m_text == nullptr )
            throw std::runtime_error("GameDataLoader::WriteStats() : No game stats to write!!");

        //Stuff
        assert(false);
    }

    void GameDataLoader::WriteAudio()
    {
        AnalyseGame();
        if( m_text == nullptr )
            throw std::runtime_error("GameDataLoader::WriteAudio() : No game audio to write!!");

        //Stuff
        assert(false);
    }

    void GameDataLoader::WriteAsm()
    {
        AnalyseGame();
        if( m_asmmanip == nullptr )
            throw std::runtime_error("GameDataLoader::WriteAsm() : No asm data to write!!");
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

    stats::GameStats        * GameDataLoader::GetStats()                                { return m_stats.get(); }
    const stats::GameStats  * GameDataLoader::GetStats() const                          { return m_stats.get(); }

    GameAudio               * GameDataLoader::GetAudio()                                { return m_audio.get(); }
    const GameAudio         * GameDataLoader::GetAudio() const                          { return m_audio.get(); }

    PMD2_ASM_Manip          * GameDataLoader::GetAsmManip()                             { return m_asmmanip.get(); }
    const PMD2_ASM_Manip    * GameDataLoader::GetAsmManip() const                       { return m_asmmanip.get(); }

};
