#include "pmd2_scripts.hpp"
#include <ppmdu/pmd2/pmd2_scripts_opcodes.hpp>
#include <ppmdu/fmts/lsd.hpp>
#include <ppmdu/fmts/ssa.hpp>
#include <ppmdu/fmts/ssb.hpp>
#include <cassert>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
#include <utils/poco_wrapper.hpp>
#include <utils/utility.hpp>
#include <utils/library_wide.hpp>
#include <functional>
using namespace std;


namespace pmd2
{
//==============================================================================
//  
//==============================================================================
    const std::regex MatchScriptFileTypes( ".*\\.(("s + filetypes::SSS_FileExt + 
                                           ")|("s + filetypes::SSA_FileExt + 
                                           ")|("s + filetypes::SSB_FileExt + 
                                           ")|("s + filetypes::LSD_FileExt + "))"s );

    const std::string ScriptRegExNameNumSSBSuff = "(\\d\\d)\\."s + filetypes::SSB_FileExt;

    const std::array<std::string, static_cast<size_t>(eScrDataTy::NbTypes)> ScriptDataTypeStrings = 
    {
        filetypes::SSE_FileExt,
        filetypes::SSS_FileExt,
        filetypes::SSA_FileExt,
    };


    const std::string & ScriptDataTypeToFileExtension( eScrDataTy scrdatty )
    {
        switch(scrdatty)
        {
            case eScrDataTy::SSA:
            {
                return filetypes::SSA_FileExt;
            }
            case eScrDataTy::SSE:
            {
                return filetypes::SSE_FileExt;
            }
            case eScrDataTy::SSS:
            {
                return filetypes::SSS_FileExt;
            }
            default:
            {
                throw std::runtime_error("ScriptDataTypeToFileExtension() : Invalid script data type!");
            }
        };
    }

    const std::string & ScriptDataTypeToStr(eScrDataTy scrdatty)
    {
        if( scrdatty < eScrDataTy::NbTypes )
            return ScriptDataTypeStrings[static_cast<size_t>(scrdatty)];
        else
            return Generic_Invalid;
    }

    eScrDataTy StrToScriptDataType(const std::string & scrdatstr)
    {
        for( size_t cnt = 0; cnt < ScriptDataTypeStrings.size(); ++cnt )
            if( scrdatstr == ScriptDataTypeStrings[cnt] ) return static_cast<eScrDataTy>(cnt);
        return eScrDataTy::Invalid;
    }


//==============================================================================
//  ScriptedSequence
//==============================================================================



    ScriptedSequence::ScriptedSequence(const ScriptedSequence & tocopy)
        :m_name(tocopy.m_name), m_originalfname(tocopy.m_originalfname),
         m_groups(tocopy.m_groups), m_strtable(tocopy.m_strtable),
         m_contants(tocopy.m_contants)
    {}
    
    ScriptedSequence::ScriptedSequence(ScriptedSequence      && tomove)
        :m_name(std::move(tomove.m_name)), m_originalfname(std::move(tomove.m_originalfname)),
         m_groups(std::move(tomove.m_groups)), m_strtable(std::move(tomove.m_strtable)),
         m_contants(std::move(tomove.m_contants))
    {}

    ScriptedSequence & ScriptedSequence::operator=( const ScriptedSequence & tocopy )
    {
        m_name          = tocopy.m_name;
        m_originalfname = tocopy.m_originalfname;
        m_groups        = tocopy.m_groups;
        m_strtable      = tocopy.m_strtable;
        m_contants      = tocopy.m_contants;
        return *this;
    }

    ScriptedSequence & ScriptedSequence::operator=( ScriptedSequence && tomove )
    {
        m_name          = std::move(tomove.m_name);
        m_originalfname = std::move(tomove.m_originalfname);
        m_groups        = std::move(tomove.m_groups);
        m_strtable      = std::move(tomove.m_strtable);
        m_contants      = std::move(tomove.m_contants);
        return *this;
    }

    void ScriptedSequence::InsertStrLanguage(eGameLanguages lang, strtbl_t && strings)
    {
        m_strtable.insert_or_assign( lang, std::forward<strtbl_t>(strings) );
    }

    inline ScriptedSequence::strtbl_t * ScriptedSequence::StrTbl(eGameLanguages lang)
    {
        auto itfound = m_strtable.find(lang);

        if( itfound !=  m_strtable.end() )
            return &(itfound->second);
        else
            return nullptr;
    }

    inline const ScriptedSequence::strtbl_t * ScriptedSequence::StrTbl(eGameLanguages lang) const
    {
        return const_cast<ScriptedSequence*>(this)->StrTbl(lang);
    }

/***********************************************************************************************
    ScriptGroup
        A script group is an ensemble of one or more ScriptEntityData, and one or more
        ScriptedSequence, that share a common identifier.
***********************************************************************************************/


//==============================================================================
//  ScriptSet
//==============================================================================
    ScriptSet::ScriptSet(const std::string & name)
        :m_name(name), m_bmodified(false)
    {}

    ScriptSet::ScriptSet(const std::string & name, scriptgrps_t && comp, lsdtbl_t && lsdtbl)
        :m_name(name), m_lsdentries(std::move(lsdtbl)), m_components(std::move(comp)), m_bmodified(false)
    {}

    ScriptSet::ScriptSet(const ScriptSet & other)
        :m_name(other.m_name), m_components(other.m_components), m_lsdentries(other.m_lsdentries), m_bmodified(other.m_bmodified)
    {}

    ScriptSet & ScriptSet::operator=(const ScriptSet & other)
    {
        m_name          = other.m_name;
        m_components    = other.m_components; 
        m_lsdentries    = other.m_lsdentries;
        m_bmodified     = other.m_bmodified;
        return *this;
    }

    ScriptSet::ScriptSet(ScriptSet && other)
    {
        m_name          = std::move(other.m_name);
        m_components    = std::move(other.m_components); 
        m_lsdentries    = std::move(other.m_lsdentries);
        m_bmodified     = other.m_bmodified;
    }

    ScriptSet & ScriptSet::operator=(ScriptSet && other)
    {
        m_name          = std::move(other.m_name);
        m_components    = std::move(other.m_components); 
        m_lsdentries    = std::move(other.m_lsdentries);
        m_bmodified     = other.m_bmodified;
        return *this;
    }

//==============================================================================
//  GameScriptsHandler
//==============================================================================
    /*
    */
    class GameScriptsHandler
    {
    public:

        typedef std::unordered_map<std::string,ScriptSet> settbl_t;

        GameScriptsHandler( GameScripts & parent )
            :m_parent(parent)
        {}

        //IO
        settbl_t Load()
        {
            using namespace utils;
            auto filelist = ListDirContent_FilesAndDirs( m_parent.m_scriptdir, false, true );
            settbl_t dest;

            for( const auto & dir : filelist )
            {
                if( isFolder(dir) )
                    dest.emplace( std::forward<string>(utils::GetBaseNameOnly(dir)), std::forward<ScriptSet>(LoadDirectory(dir)) );
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
        ScriptSet LoadDirectory (const std::string & path);
        void      WriteDirectory(const ScriptSet   & set, const std::string & path );

    private:
        void LoadGrpEnter     ( std::deque<Poco::Path> & fqueue, ScriptSet & out_scrset );
        void LoadSub          ( const Poco::Path & datafpath, std::deque<Poco::Path> & fqueue, ScriptSet & out_scrset );
        void LoadGrpLSDContent( const Poco::Path & curdir, std::deque<Poco::Path> & fqueue, ScriptSet & out_scrset );
        void LoadLSD   ( ScriptSet   & curset, const std::string & fpath );
        void LoadSSB   ( ScriptGroup & tgtgrp, const std::string & fpath );
        void LoadSSData( ScriptGroup & tgtgrp, const std::string & fpath );
        void LoadScrDataAndMatchedNumberedSSBs( const std::string & prefix, const std::string & fext, eScriptGroupType grpty, std::deque<Poco::Path> & fqueue, ScriptSet & out_scrset  );
        void LoadNumberedSSBForPrefix( std::deque<Poco::Path> & fqueue, const std::string & prefix, ScriptGroup & tgtgrp );

        void WriteLSD   ( const ScriptSet & curset, const std::string & fpath );
        void WriteGroups( const ScriptSet & curset, const std::string & dirpath );

    private:
        GameScripts & m_parent;
    };

// GameScriptsHandler implementation!
//==============================================================================

    void GameScriptsHandler::LoadLSD( ScriptSet & curset, const std::string & fpath)
    {
        curset.LSDTable() = filetypes::ParseLSD(fpath);
    }

    void GameScriptsHandler::LoadSSB(ScriptGroup & tgtgrp, const std::string & fpath)
    {
        string basename = Poco::Path(fpath).getBaseName();
        auto script = std::move( filetypes::ParseScript(fpath, m_parent.m_scrRegion, m_parent.m_gameVersion) );
        script.SetFileName(Poco::Path(fpath).getFileName());
        script.SetName(basename);
        tgtgrp.Sequences().emplace(std::move(std::make_pair(basename, std::move(script) )));

    }


    void GameScriptsHandler::LoadSSData(ScriptGroup& tgtgrp, const std::string & fpath)
    {
        string basename = Poco::Path(fpath).getBaseName();
        tgtgrp.SetData( std::forward<ScriptEntityData>(filetypes::ParseScriptData(fpath)) );
    }


    void GameScriptsHandler::LoadNumberedSSBForPrefix( std::deque<Poco::Path> & fqueue, const std::string & prefix, ScriptGroup & tgtgrp )
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
                                                                eScriptGroupType         grpty, 
                                                                std::deque<Poco::Path> & fqueue,
                                                                ScriptSet              & out_scrset )
    {
        auto itfounddata = fqueue.end();
        //#1 - Find if we have a data file!

        for( auto itqueue = fqueue.begin(); itqueue != fqueue.end(); ++itqueue )
        {
            if( itqueue->getBaseName() == prefix )
            {
                itfounddata = itqueue; break;
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
        ScriptGroup grp( prefix, grpty );
        LoadSSData( grp, itfounddata->toString() );
        fqueue.erase(itfounddata); //Remove it from the queue so we don't process it again

        //#4 - Then load the matched ssb
        LoadNumberedSSBForPrefix( fqueue, prefix, grp );

        //#5 - Add the group to the set
        out_scrset.Components().push_back(std::move(grp));
    }

    void GameScriptsHandler::LoadGrpEnter( std::deque<Poco::Path> & fqueue, ScriptSet & out_scrset )
    {
        if( fqueue.empty() ) 
            return;

        LoadScrDataAndMatchedNumberedSSBs( ScriptPrefix_enter, filetypes::SSE_FileExt, eScriptGroupType::UNK_enter, fqueue, out_scrset );
    }

    void GameScriptsHandler::LoadSub( const Poco::Path & datafpath, std::deque<Poco::Path> & fqueue, ScriptSet & out_scrset )
    {
        string basename( std::move(datafpath.getBaseName()));
        ScriptGroup grp( basename, eScriptGroupType::UNK_sub );
        LoadSSData( grp, datafpath.toString() );
        LoadNumberedSSBForPrefix( fqueue, basename, grp );
        out_scrset.Components().push_back(std::move(grp));
    }

    void GameScriptsHandler::LoadGrpLSDContent( const Poco::Path & curdir, std::deque<Poco::Path> & fqueue, ScriptSet & out_scrset )
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
            ScriptGroup scrpair( string( std::begin(afile), std::end(afile) ), eScriptGroupType::UNK_fromlsd );
            LoadSSB   ( scrpair, ssbpath.toString() );
            LoadSSData( scrpair, ssapath.toString() );

            //#4 - Push the pair into the set!
            out_scrset.Components().push_back( std::move(scrpair) );
        }

    }


    ScriptSet GameScriptsHandler::LoadDirectory(const std::string & path)
    {
        using namespace Poco;
        Path curdir(path);

        if( utils::LibWide().isLogOn() )
            clog << "Loading Directory " <<curdir.parent().getBaseName() <<"/" <<curdir.getBaseName() <<".. ";

        //#0 Fetch file list
        DirectoryIterator  itdir(curdir);
        DirectoryIterator  itdirend;
        deque<Path>        processqueue;
        ScriptSet          curset(curdir.getBaseName());
        

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
                ScriptGroup unionall( "unionall", eScriptGroupType::UNK_unionall );
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

    void GameScriptsHandler::WriteDirectory(const ScriptSet & set, const std::string & path)
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
        lsdpath.append( lsdname );

        WriteLSD(set, lsdpath.toString());

        //Write all our content
        WriteGroups(set, path);
    }


    void GameScriptsHandler::WriteLSD(const ScriptSet & set, const std::string & fpath)
    {
        if( ! set.LSDTable().empty() )
        {
            if( utils::LibWide().isLogOn() )
                clog<<"\tWriting LSD file...\n";
            filetypes::WriteLSD( set.LSDTable(), fpath );
        }
    }

    void GameScriptsHandler::WriteGroups(const ScriptSet & set, const std::string & dirpath)
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
                filetypes::WriteScriptData( Poco::Path(dirpath).setFileName(grp.Data()->Name()).setExtension(ScriptDataTypeToFileExtension(grp.Data()->Type())).toString(),
                                            *grp.Data() );
            }

            //Write SSBs
            for( const auto & seq : grp.Sequences() )
                filetypes::WriteScript(Poco::Path(dirpath).setFileName(seq.first).setExtension(filetypes::SSB_FileExt).toString(), seq.second);
            
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

    ScriptSet ScrSetLoader::operator()()const
    {
        return std::move( m_parent->m_pHandler->LoadDirectory(m_path) );
    }

    void ScrSetLoader::operator()(const ScriptSet & set)const
    {
        m_parent->m_pHandler->WriteDirectory(set, m_path);
    }

    GameScripts::GameScripts(const std::string & scrdir, eGameRegion greg, eGameVersion gver)
        :m_scriptdir(scrdir), m_scrRegion(greg), m_gameVersion(gver), m_pHandler(new GameScriptsHandler(*this)), m_common(DirNameScriptCommon)
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
            string basename = std::move( itdir.path().getBaseName() );
            if( basename == DirNameScriptCommon )
                m_common = std::move( m_pHandler->LoadDirectory(itdir->path()) );
            else
                m_setsindex.emplace( std::forward<string>(basename), std::forward<ScrSetLoader>(ScrSetLoader(*this, itdir->path())) );
        }
    }

    void GameScripts::ImportXML(const std::string & dir)
    {
        ImportXMLGameScripts(dir,*this);
    }

    void GameScripts::ExportXML(const std::string & dir)
    {
        ExportGameScriptsXML(dir,*this);
    }

    std::unordered_map<std::string, ScriptSet> GameScripts::LoadAll()
    {
        std::unordered_map<std::string, ScriptSet> out;

        for( const auto & entry : m_setsindex )
            out.emplace( entry.first, std::forward<ScriptSet>(entry.second()) );

        return std::move(out);
    }

    void GameScripts::WriteAll(const std::unordered_map<std::string, ScriptSet>& stuff)
    {
        WriteScriptSet(m_common);

        for( const auto & entry : stuff )
            m_setsindex.at(entry.first)(entry.second);
    }

    ScriptSet GameScripts::LoadScriptSet(const std::string & setname)
    {
        Poco::File dir( Poco::Path(m_scriptdir).append(setname) );
        if( dir.exists() && dir.isDirectory() )
            return std::move( m_pHandler->LoadDirectory(dir.path()) );
        else
            throw std::runtime_error("GameScripts::LoadScriptSet(): "+setname+" doesn't exists.");
        return ScriptSet("");
    }

    eGameRegion GameScripts::Region() const
    {
        return m_scrRegion;
    }

    eGameVersion GameScripts::Version() const
    {
        return m_gameVersion;
    }

    void GameScripts::WriteScriptSet(const ScriptSet & set)
    {
        const string tgtdir = Poco::Path(m_scriptdir).append(set.Name()).toString();
        if( set.Name() != DirNameScriptCommon )
            m_setsindex.insert_or_assign(set.Name(), std::forward<ScrSetLoader>(ScrSetLoader(*this, tgtdir)) ); //Add to index if doesn't exists
        m_pHandler->WriteDirectory(set, tgtdir);
    }

    const ScriptSet & GameScripts::GetCommonSet() const
    {
        return m_common;
    }

    ScriptSet & GameScripts::GetCommonSet()
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
