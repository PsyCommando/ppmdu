#include "pmd2_scripts.hpp"
#include <ppmdu/pmd2/pmd2_scripts_opcodes.hpp>
#include <ppmdu/fmts/lsd.hpp>
#include <ppmdu/fmts/ssa.hpp>
#include <ppmdu/fmts/ssb.hpp>
#include <utils/poco_wrapper.hpp>
#include <utils/utility.hpp>
#include <utils/library_wide.hpp>
#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
#include <cassert>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <functional>
using namespace std;


namespace pmd2
{
//==============================================================================
//  
//==============================================================================
    //Used to identify all the files of the script engine
    const std::regex MatchScriptFileTypes( ".*\\.(("s + filetypes::SSS_FileExt + 
                                           ")|("s + filetypes::SSA_FileExt + 
                                           ")|("s + filetypes::SSE_FileExt + 
                                           ")|("s + filetypes::SSB_FileExt + 
                                           ")|("s + filetypes::LSD_FileExt + "))"s );

    const std::string ScriptRegExNameNumSSBSuff = "(\\d\\d)\\."s + filetypes::SSB_FileExt;


    /*
        isNumberedSSB
            -subname  : base name. Aka the name of the data file tied to this. "m01a02" for example.
            -filename : the filename to verify. "m01a0201.ssb" for example.
    */ 
    inline bool isNumberedSSB( std::string subname, const std::string & filename )
    {
        return std::regex_match(filename, std::regex(subname.append(ScriptRegExNameNumSSBSuff) ) );
    }

//==============================================================================
//  GameScriptsHandler
//==============================================================================
    /*
    */
    class GameScriptsHandler
    {
    public:

        typedef std::unordered_map<std::string,LevelScript> settbl_t;

        GameScriptsHandler( GameScripts & parent )
            :m_parent(parent)
        {}

        //IO
        settbl_t Load(bool escapeasxml = false)
        {
            using namespace utils;
            auto filelist = ListDirContent_FilesAndDirs( m_parent.m_scriptdir, false, true );
            settbl_t dest;

            for( const auto & dir : filelist )
            {
                if( isFolder(dir) )
                    dest.emplace( std::forward<string>(utils::GetBaseNameOnly(dir)), std::forward<LevelScript>(LoadDirectory(dir)) );
            }
            return std::move(dest);
        }
        
        template<typename _init>
            void Write( _init itbeg, _init itend )
        {
            for( ; itbeg != itend; ++itbeg )
                WriteDirectory(*itbeg);
        }

        //Load and write a single "Event"
        LevelScript LoadDirectory (const std::string & path);
        void      WriteDirectory(const LevelScript   & set, const std::string & path );

    private:
        void LoadGrpEnter     ( std::deque<Poco::Path> & fqueue, LevelScript & out_scrset );
        void LoadSub          ( const Poco::Path & datafpath, std::deque<Poco::Path> & fqueue, LevelScript & out_scrset );
        void LoadGrpLSDContent( const Poco::Path & curdir, std::deque<Poco::Path> & fqueue, LevelScript & out_scrset );
        void LoadLSD   ( LevelScript   & curset, const std::string & fpath );
        void LoadSSB   ( ScriptSet & tgtgrp, const std::string & fpath );
        void LoadSSData( ScriptSet & tgtgrp, const std::string & fpath );
        void LoadScrDataAndMatchedNumberedSSBs( const std::string & prefix, const std::string & fext, eScriptSetType grpty, std::deque<Poco::Path> & fqueue, LevelScript & out_scrset  );
        void LoadNumberedSSBForPrefix( std::deque<Poco::Path> & fqueue, const std::string & prefix, ScriptSet & tgtgrp );

        void WriteLSD   ( const LevelScript & curset, const std::string & fpath );
        void WriteGroups( const LevelScript & curset, const std::string & dirpath );

    private:
        GameScripts & m_parent;
    };

// GameScriptsHandler implementation!
//==============================================================================

    void GameScriptsHandler::LoadLSD( LevelScript & curset, const std::string & fpath)
    {
        curset.LSDTable() = filetypes::ParseLSD(fpath);
    }

    void GameScriptsHandler::LoadSSB(ScriptSet & tgtgrp, const std::string & fpath )
    {
        try 
        {
            string basename = Poco::Path(fpath).getBaseName();
            auto script = std::move( filetypes::ParseScript(fpath, 
                                                            m_parent.Region(), 
                                                            m_parent.Version(), 
                                                            m_parent.GetConfig().GetLanguageFilesDB(), 
                                                            false,                                      //! #TODO: Get rid of this option eventually
                                                            m_parent.GetOptions().bscriptdebug) );
            script.SetName(basename);
            tgtgrp.Sequences().emplace(std::move(std::make_pair(basename, std::move(script) )));
        }
        catch(const std::exception &)
        {
            stringstream sstrer;
            sstrer << "GameScriptsHandler::LoadSSB(): Encountered error loading SSB file \"" <<fpath <<"\"!";
            throw_with_nested(std::runtime_error(sstrer.str()));
        }
    }


    void GameScriptsHandler::LoadSSData(ScriptSet& tgtgrp, const std::string & fpath)
    {
        try 
        {
            string basename = Poco::Path(fpath).getBaseName();
            tgtgrp.SetData( std::forward<ScriptData>(filetypes::ParseScriptData(fpath)) );
        }
        catch(const std::exception &)
        {
            stringstream sstrer;
            sstrer << "GameScriptsHandler::LoadSSData(): Encountered error loading script data file \"" <<fpath <<"\"!";
            throw_with_nested(std::runtime_error(sstrer.str()));
        }
    }


    void GameScriptsHandler::LoadNumberedSSBForPrefix( std::deque<Poco::Path> & fqueue, const std::string & prefix, ScriptSet & tgtgrp )
    {
        auto itfound = fqueue.end();

        do
        {
            itfound = std::find_if( fqueue.begin(), fqueue.end(), [&]( const Poco::Path & ap )->bool
            {
                return isNumberedSSB( prefix, ap.getFileName() );
            } ); 

            if( itfound != fqueue.end() )
            {
                string curbasename = itfound->toString();
                clog << "\n\tFound matching ssb, " <<curbasename <<", for prefix " <<prefix  <<"\n";
                LoadSSB( tgtgrp, curbasename );
                fqueue.erase(itfound);
                itfound = fqueue.begin(); //Re-assign here, because the iterator got invalidated
            }

        }while( itfound != fqueue.end() );
    }




    void GameScriptsHandler::LoadScrDataAndMatchedNumberedSSBs( const std::string      & prefix, 
                                                                const std::string      & fext, 
                                                                eScriptSetType         grpty, 
                                                                std::deque<Poco::Path> & fqueue,
                                                                LevelScript              & out_scrset )
    {
        auto itfounddata = fqueue.end();
        //#1 - Find if we have a data file!

        for( auto itqueue = fqueue.begin(); itqueue != fqueue.end(); ++itqueue )
        {
            if( itqueue->getExtension() == fext && itqueue->getBaseName() == prefix )
            {
                itfounddata = itqueue; 
                break;
            }
        }

        //#2 - If we don't return early!
        if( itfounddata == fqueue.end() )
        {
            if( utils::LibWide().isLogOn() )
                clog << "\n\tNo " <<prefix <<" data found!\n";
            return;
        }
        else
        {
            clog <<"\n\tLoading " <<prefix <<"." <<fext <<" and its dependencies..";
        }

        //#3 - Otherwise keep going and load the data first ###
        ScriptSet grp( prefix, grpty );
        LoadSSData( grp, itfounddata->toString() );
        fqueue.erase(itfounddata); //Remove it from the queue so we don't process it again

        //#4 - Then load the matched ssb
        LoadNumberedSSBForPrefix( fqueue, prefix, grp );

        //#5 - Add the group to the set
        out_scrset.Components().push_back(std::move(grp));
    }

    void GameScriptsHandler::LoadGrpEnter(  std::deque<Poco::Path> & fqueue, LevelScript & out_scrset )
    {
        if( fqueue.empty() ) 
            return;

        const static auto lambdafindsse = [](const Poco::Path & p)->bool{return p.getExtension() == filetypes::SSE_FileExt && p.getBaseName() == ScriptPrefix_enter;};
        auto itcur         = fqueue.begin();
        while( (itcur = std::find_if( fqueue.begin(), fqueue.end(), lambdafindsse)) != fqueue.end() )
        {
            Poco::Path p = *itcur;
            fqueue.erase(itcur); //Delete here, because we invalidate the iterator in the LoadSub method

            string basename( std::move(p.getBaseName()));
            ScriptSet grp( basename, eScriptSetType::UNK_enter );
            LoadSSData( grp, p.toString() );
            LoadNumberedSSBForPrefix( fqueue, basename, grp );
            out_scrset.Components().push_back(std::move(grp));
        }
        /*LoadScrDataAndMatchedNumberedSSBs( ScriptPrefix_enter, filetypes::SSE_FileExt, eScriptSetType::UNK_enter, fqueue, out_scrset );*/
    }

    void GameScriptsHandler::LoadSub( const Poco::Path & datafpath, std::deque<Poco::Path> & fqueue, LevelScript & out_scrset )
    {
        string basename( std::move(datafpath.getBaseName()));
        ScriptSet grp( basename, eScriptSetType::UNK_station );
        LoadSSData( grp, datafpath.toString() );
        LoadNumberedSSBForPrefix( fqueue, basename, grp );
        out_scrset.Components().push_back(std::move(grp));
    }

    void GameScriptsHandler::LoadGrpLSDContent( const Poco::Path & curdir, std::deque<Poco::Path> & fqueue, LevelScript & out_scrset )
    {

        //Load files by name in the lsd table!

        for( const auto & afile : out_scrset.LSDTable() )
        {
            //#1 - Build the expected filenames
            string fname;
            fname.reserve(8);
            for( const char & c : afile )
            {
                if( c != 0 ) //Ignore the terminating 0
                    fname.push_back( std::tolower( c, std::locale::classic() ) );
            }

            Poco::Path ssbpath(curdir);
            Poco::Path ssapath(curdir);
            ssbpath.append(fname).setExtension(filetypes::SSB_FileExt);
            ssapath.append(fname).setExtension(filetypes::SSA_FileExt);

            //#2 - Pop those out of the queue + verify if they exist at the same time
            auto itfoundssb = std::find_if( fqueue.begin(), fqueue.end(), [&ssbpath]( const Poco::Path & path )->bool
            {
                return (ssbpath.toString() == path.toString());
            } );

            if( itfoundssb != fqueue.end() )
                fqueue.erase(itfoundssb);
            else
                clog<< "GameScriptsHandler::LoadGrpLSDContent(): Expected SSB file named" << ssbpath.toString() << ", but couldn't find it.. Possibly a duplicate entry!\n";

            auto itfoundssa = std::find_if( fqueue.begin(), fqueue.end(), [&ssapath]( const Poco::Path & path )->bool
            {
                return (ssapath.toString() == path.toString());
            } );

            if( itfoundssa != fqueue.end() )
                fqueue.erase(itfoundssa);
            else
                clog<<"GameScriptsHandler::LoadGrpLSDContent(): Expected SSA file named" << ssapath.toString() << ", but couldn't find it.. Possibly a duplicate entry!\n";

            //#3 - Handle the files
            ScriptSet scrpair( string( std::begin(afile), std::end(afile) ), eScriptSetType::UNK_acting );
            LoadSSB   ( scrpair, ssbpath.toString() );
            LoadSSData( scrpair, ssapath.toString() );

            //#4 - Push the pair into the set!
            out_scrset.Components().push_back( std::move(scrpair) );
        }

    }


    LevelScript GameScriptsHandler::LoadDirectory(const std::string & path)
    {
        //m_escapeasxml = escapeasxml;
        using namespace Poco;
        Path curdir(path);

        if( utils::LibWide().isLogOn() )
            clog << "Loading Directory " <<curdir.parent().getBaseName() <<"/" <<curdir.getBaseName() <<".. ";

        //#0 Fetch file list
        DirectoryIterator  itdir(curdir);
        DirectoryIterator  itdirend;
        deque<Path>        processqueue;
        LevelScript          curset(curdir.getBaseName());
        

        while( itdir != itdirend )
        {
            if(itdir->isFile() && !itdir->isHidden() && std::regex_match( itdir->path(), MatchScriptFileTypes ) )
                processqueue.push_back(itdir.path());
            ++itdir;
        }

        //#1 Check for unionall.ssb
        {
            auto itfoundunion= std::find_if( processqueue.begin(), processqueue.end(), [&](const Path & entry)->bool
            {
                return utils::CompareStrIgnoreCase(entry.getFileName(),  ScriptNames_unionall );
            });
            if( itfoundunion != processqueue.end() )
            {
                clog << "\n\tParsing unionall.ssb..";
                ScriptSet unionall( "unionall", eScriptSetType::UNK_unionall );
                LoadSSB( unionall, itfoundunion->toString() );
                curset.Components().push_back( std::move(unionall) );
                processqueue.erase(itfoundunion);
                clog <<"\n";
            }
        }

        //#2 Load LSD, if its there
        {
            auto itfoundlsd = std::find_if( processqueue.begin(), processqueue.end(), [&](const Path & entry)->bool
            {
                return utils::CompareStrIgnoreCase(entry.getExtension(), filetypes::LSD_FileExt);
            });
            if( itfoundlsd != processqueue.end() )
            {
                clog <<"\n\tParsing LSD file..";
                LoadLSD(curset, itfoundlsd->toString());
                processqueue.erase(itfoundlsd);
                clog <<"\n";
            }
        }

        //#3 Check for enter.sse/enterXX.ssb
        LoadGrpEnter( processqueue, curset );

        //#4 Load script files from LSD
        if( !curset.LSDTable().empty() )
        {
            clog <<"\n\tLoading LSD references..";
            LoadGrpLSDContent( curdir, processqueue, curset );
            clog <<"\n";
        }

        //#5 Load the SSS subs
        auto lambdafindsss = [](const Poco::Path & p)->bool{return p.getExtension() == filetypes::SSS_FileExt;};
        auto itcur         = processqueue.begin();
        while( (itcur = std::find_if( processqueue.begin(), processqueue.end(), lambdafindsss)) != processqueue.end() )
        {
            Poco::Path p = *itcur;
            processqueue.erase(itcur); //Delete here, because we invalidate the iterator in the LoadSub method
            LoadSub(p, processqueue, curset);
        }

        //#8 - List files remaining in the queue
        if( !processqueue.empty() )
        {
            clog <<"\n\t" <<processqueue.size() <<" files were ignored:\n";
                 
            while( !processqueue.empty() )
            {
                clog <<"\t\t" <<processqueue.front().getFileName() <<"\n";
                processqueue.pop_front();
            }
        }

        clog <<"\n";
        return std::move(curset);
    }

    void GameScriptsHandler::WriteDirectory(const LevelScript & set, const std::string & path)
    {
        //Create it first if needed
        Poco::File tgtdir(path);
        if( !tgtdir.exists() )
            tgtdir.createDirectory();
        else if( tgtdir.exists() && !tgtdir.isDirectory() )
            throw std::runtime_error("GameScriptsHandler::WriteDirectory(): Output dir exist already as a file! Can't overwrite with a directory!");

        //Write LSD table if needed
        Poco::Path lsdpath(path);
        string     lsdname = set.Name();

        //Make lsd name lower case
        std::transform( lsdname.begin(), lsdname.end(), lsdname.begin(), std::bind( std::tolower<string::value_type>, placeholders::_1, std::ref(locale::classic()) ) );
        lsdpath.append(lsdname).makeFile().setExtension(filetypes::LSD_FileExt);

        WriteLSD(set, lsdpath.toString());

        //Write all our content
        WriteGroups(set, path);
    }


    void GameScriptsHandler::WriteLSD(const LevelScript & set, const std::string & fpath)
    {
        if( ! set.LSDTable().empty() )
        {
            if( utils::LibWide().isLogOn() )
                clog<<"\tWriting LSD file...\n";
            filetypes::WriteLSD( set.LSDTable(), fpath );
        }
    }

    void GameScriptsHandler::WriteGroups(const LevelScript & set, const std::string & dirpath)
    {
        if( utils::LibWide().isLogOn() )
            clog << "\tWriting groups to " <<dirpath <<"\n";
        for( const auto & grp : set ) 
        {
            if( utils::LibWide().isLogOn() )
                clog << "\t\tWriting " <<grp.Identifier() <<"...";

            //Write data file
            if( grp.Data() )
            {
                filetypes::WriteScriptData( Poco::Path(dirpath).append(grp.Data()->Name()).makeFile().setExtension(grp.GetDataFext()).toString(),
                                            *grp.Data() );
            }

            //Write SSBs
            for( const auto & seq : grp.Sequences() )
            {
                filetypes::WriteScript( Poco::Path(dirpath).append(seq.first).makeFile().setExtension(filetypes::SSB_FileExt).toString(), 
                                        seq.second,
                                        m_parent.Region(), 
                                        m_parent.Version(),
                                        m_parent.GetConfig().GetLanguageFilesDB() );
            }
            
            if( utils::LibWide().isLogOn() )
                clog <<"\n";
        }
    }



//==============================================================================
//  GameScripts
//==============================================================================

    ScrSetLoader::ScrSetLoader(GameScripts & parent, const std::string & filepath )
        :m_parent( std::addressof(parent) ), m_path(filepath)
    {}

    LevelScript ScrSetLoader::operator()()const
    {
        return std::move( m_parent->m_pHandler->LoadDirectory(m_path) );
    }

    void ScrSetLoader::operator()(const LevelScript & set)const 
    {
        m_parent->m_pHandler->WriteDirectory(set, m_path);
    }

    GameScripts::GameScripts(const std::string & scrdir, const ConfigLoader & gconf, const scriptprocoptions & options )
        :m_scriptdir(scrdir), 
         //m_scrRegion(gconf.GetGameVersion().region), 
         //m_gameVersion(gconf.GetGameVersion().version),
         m_pHandler(new GameScriptsHandler(*this)), 
         m_common(DirNameScriptCommon),
         //m_langdat(std::addressof(gconf.GetLanguageFilesDB())),
         //m_escapexml(bescapexml),
         m_gconf(gconf),
         //m_bbscriptdebug(bscriptdebug)
         m_options(options)
    {
        Load();
    }

    GameScripts::~GameScripts()
    {}

    //File IO
    void GameScripts::Load()
    {
        //Build directory index
        Poco::DirectoryIterator itdir(m_scriptdir);
        Poco::DirectoryIterator itdirend;

        for( ; itdir != itdirend; ++itdir )
        {
            if( itdir->isDirectory() )
            {
                string basename = std::move( Poco::Path::transcode(itdir.path().getBaseName()) );
                if( basename == DirNameScriptCommon )
                    m_common = std::move( m_pHandler->LoadDirectory(Poco::Path::transcode(itdir->path()) ) );
                else
                    m_setsindex.emplace( std::forward<string>(basename), std::forward<ScrSetLoader>(ScrSetLoader(*this, Poco::Path::transcode(itdir->path()))) );
            }
        }
    }

    void GameScripts::ImportXML(const std::string & dir)
    {
        ImportXMLGameScripts(dir,*this, m_options);
    }

    void GameScripts::ExportXML(const std::string & dir)
    {
        ExportGameScriptsXML(dir,*this, m_options); //We don't want pugixml to escape characters if we already did!!
    }

    std::unordered_map<std::string, LevelScript> GameScripts::LoadAll()
    {
        std::unordered_map<std::string, LevelScript> out;

        for( const auto & entry : m_setsindex )
            out.emplace( entry.first, std::forward<LevelScript>(entry.second()) );

        return std::move(out);
    }

    void GameScripts::WriteAll(const std::unordered_map<std::string, LevelScript>& stuff)
    {
        WriteScriptSet(m_common);

        for( const auto & entry : stuff )
            m_setsindex.at(entry.first)(entry.second);
    }

    LevelScript GameScripts::LoadScriptSet(const std::string & setname)
    {
        Poco::File dir( Poco::Path(m_scriptdir).append(setname) );
        if( dir.exists() && dir.isDirectory() )
            return std::move( m_pHandler->LoadDirectory(dir.path()) );
        else
            throw std::runtime_error("GameScripts::LoadScriptSet(): "+setname+" doesn't exists.");
        return LevelScript("");
    }

    eGameRegion GameScripts::Region() const
    {
        return GetConfig().GetGameVersion().region;
    }

    eGameVersion GameScripts::Version() const
    {
        return GetConfig().GetGameVersion().version;
    }

    void GameScripts::WriteScriptSet(const LevelScript & set)
    {
        const string tgtdir = Poco::Path(m_scriptdir).append(set.Name()).toString();
        if( set.Name() != DirNameScriptCommon )
            m_setsindex.insert_or_assign(set.Name(), std::forward<ScrSetLoader>(ScrSetLoader(*this, tgtdir)) ); //Add to index if doesn't exists
        m_pHandler->WriteDirectory(set, tgtdir);
    }

    const LevelScript & GameScripts::GetCommonSet() const
    {
        return m_common;
    }

    LevelScript & GameScripts::GetCommonSet()
    {
        return m_common;
    }

    inline GameScripts::iterator       GameScripts::begin()      {return m_setsindex.begin();}
    inline GameScripts::const_iterator GameScripts::begin()const {return m_setsindex.begin();}

    inline GameScripts::iterator       GameScripts::end()      {return m_setsindex.end();}
    inline GameScripts::const_iterator GameScripts::end()const {return m_setsindex.end();}

    inline size_t GameScripts::size()const {return m_setsindex.size();}
    inline bool   GameScripts::empty()const {return m_setsindex.empty();}




};
