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
using namespace std;


namespace pmd2
{
//==============================================================================
//  
//==============================================================================
 
//
//
//
    //template<typename init_t>
    //    ScriptIdentifier::IdToken ParseToken( init_t & itbeg, init_t itend )
    //{

    //}


    //ScriptIdentifier ScriptIdentifier::ParseScriptIdentifier( const std::string & scrid )
    //{
    //    ScriptIdentifier outscrid;
    //    static const char EnterFirstLetter = 'e';
    //    static const char DusFirstLetter   = 'd';
    //    static const char UFirstLetter     = 'u';

    //    if(scrid.size() < 2)
    //    {
    //        assert(false);
    //    }

    //    outscrid.type = eScriptGroupType::INVALID;

    //    switch( scrid.front() )
    //    {
    //        case EnterFirstLetter:
    //        {
    //            auto itfound = std::search( ScriptPrefix_enter.begin(), ScriptPrefix_enter.end(), scrid.begin(), scrid.end() );
    //            if( itfound != scrid.end() )
    //            {
    //                outscrid.type = eScriptGroupType::UNK_enter;

    //                //If the filename has a number appended, parse that!
    //                if( ScriptPrefix_enter.size() < scrid.size() ) 
    //                    outscrid.tokens.push_back( ParseToken( (scrid.begin() + ScriptPrefix_enter.size()), scrid.end() ) );
    //            }
    //            break;
    //        }
    //        case DusFirstLetter:
    //        {
    //            auto itfound = std::search( ScriptPrefix_enter.begin(), ScriptPrefix_enter.end(), scrid.begin(), scrid.end() );
    //            if( itfound != scrid.end() )
    //            {
    //                outscrid.type = eScriptGroupType::UNK_dus;
    //            }
    //            break;
    //        }
    //        case UFirstLetter:
    //        {
    //            ///validate that next character is a letter, and the next after that is a number
    //            if( !std::isalpha( scrid[1], locale::classic() ) || std::isalpha( scrid[2], locale::classic() )  )
    //                break;
    //            
    //            outscrid.type = eScriptGroupType::UNK_u;
    //            break;
    //        }
    //    };

    //    //If we end up here with no script type, its not a static name or 'u' prefix!!
    //    if( outscrid.type == eScriptGroupType::INVALID )
    //    {
    //        outscrid.type = eScriptGroupType::UNK_fromlsd;


    //    }

    //    return outscrid;
    //}

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

        GameScriptsHandler( GameScripts & parent )
            :m_parent(parent)
        {}

        //IO
        void Load();
        void Write();

    private:
        void LoadDirectory    (const std::string       & path);
        void LoadGrpEnter     ( std::deque<Poco::Path> & fqueue, ScriptSet & out_scrset );
        void LoadGrpDus       ( std::deque<Poco::Path> & fqueue, ScriptSet & out_scrset );
        void LoadLoneSSS      ( std::deque<Poco::Path> & fqueue, ScriptSet & out_scrset );
        void LoadGrpU         ( std::deque<Poco::Path> & fqueue, ScriptSet & out_scrset );
        void LoadGrpLSDContent( const Poco::Path & curdir, std::deque<Poco::Path> & fqueue, ScriptSet & out_scrset );


        void LoadLSD   ( ScriptSet   & curset, const std::string & fpath );
        void LoadSSB   ( ScriptGroup & tgtgrp, const std::string & fpath );
        void LoadSSData( ScriptGroup & tgtgrp, const std::string & fpath );

        void LoadAUGrp( std::deque<Poco::Path> & fqueue, ScriptSet & out_scrset, std::deque<Poco::Path>::iterator itfound);
        void LoadScrDataAndMatchedNumberedSSBs( const std::string & prefix, const std::string & fext, eScriptGroupType grpty, std::deque<Poco::Path> & fqueue, ScriptSet & out_scrset  );
        void LoadNumberedSSBForPrefix( std::deque<Poco::Path> & fqueue, const std::string & prefix, ScriptGroup & tgtgrp );

    private:
        GameScripts & m_parent;
    };

// GameScriptsHandler implementation!
//==============================================================================
    void GameScriptsHandler::Load ()
    {
        using namespace utils;
        auto filelist = ListDirContent_FilesAndDirs( m_parent.m_scriptdir );

        for( const auto & dir : filelist )
        {
            if( isFolder(dir) )
                LoadDirectory(dir);
        }
    }

    void GameScriptsHandler::Write()
    {
        assert(false);
    }

    void GameScriptsHandler::LoadLSD( ScriptSet & curset, const std::string & fpath)
    {
        curset.LSDTable() = filetypes::ParseLSD(fpath);
    }

    void GameScriptsHandler::LoadSSB(ScriptGroup & tgtgrp, const std::string & fpath)
    {
        string basename = Poco::Path(fpath).getBaseName();

        tgtgrp.Sequences().emplace(std::move(std::make_pair(basename, filetypes::ParseScript( fpath, m_parent.m_scrloc, m_parent.m_gameversion ))));
        //if( m_parent.m_gameversion == eGameVersion::EoS )
        //{
        //    if( m_parent.m_scrloc == eGameLocale::NorthAmerica )
        //        tgtgrp.Sequences().emplace(std::move(std::make_pair(basename, filetypes::ParseScriptEoS( fpath ))));
        //    else if( m_parent.m_scrloc == eGameLocale::Europe )
        //        tgtgrp.Sequences().emplace(std::move(std::make_pair(basename, filetypes::ParseScriptEoSPal( fpath ))));
        //}
        //else if(m_parent.m_gameversion == eGameVersion::EoTEoD)
        //{
        //    if( m_parent.m_scrloc == eGameLocale::NorthAmerica )
        //        tgtgrp.Sequences().emplace(std::move(std::make_pair(basename, filetypes::ParseScriptEoTD( fpath ))));
        //    else if( m_parent.m_scrloc == eGameLocale::Europe )
        //        tgtgrp.Sequences().emplace(std::move(std::make_pair(basename, filetypes::ParseScriptEoTDPal( fpath ))));
        //}
        //else
        //{
        //    cerr <<"GameScripts::LoadSSB(): Unknown game version!!\n";
        //    assert(false);
        //}
    }


    void GameScriptsHandler::LoadSSData(ScriptGroup& tgtgrp, const std::string & fpath)
    {
        string basename = Poco::Path(fpath).getBaseName();
        tgtgrp.SetData( new ScriptEntityData(filetypes::ParseScriptData(fpath)) );
    }


    void GameScriptsHandler::LoadNumberedSSBForPrefix( std::deque<Poco::Path> & fqueue, const std::string & prefix, ScriptGroup & tgtgrp )
    {
        auto itfound = fqueue.end();

        do
        {
            itfound = std::find_if( fqueue.begin(), fqueue.end(), [&]( const Poco::Path & ap )->bool
            {
                if( utils::CompareStrIgnoreCase(ap.getExtension(), filetypes::SSB_FileExt) ) //Check if we got the right extension
                {
                    string fname = ap.getBaseName();
                    if( (fname.size() == prefix.size() + 2) &&                              //Check if the length is even valid
                        std::isdigit( fname[prefix.size()],   std::locale::classic() ) && 
                        std::isdigit( fname[prefix.size()+1], std::locale::classic() ))     //Check if it ends with 2 digits
                    {
                        auto   itfoundprfx = std::search( fname.begin(), fname.end(), prefix.begin(), prefix.end() );
                        return (itfoundprfx == fname.begin()); //Check if the filename contains the prefix at the beginning of it
                    }
                }
                return false;
            } ); 

            if( itfound != fqueue.end() )
            {
                string curbasename = itfound->toString();
                clog << "\n\tFound matching ssb, " <<curbasename <<", for prefix " <<prefix  <<"\n";
                LoadSSB( tgtgrp, curbasename );
                fqueue.erase(itfound);
            }

        }while( itfound != fqueue.end() );


        //for( auto itqueue = loopq.begin(); itqueue != loopq.end(); ++itqueue )
        //{
        //    string fname   = itqueue->getBaseName();
        //    auto   itfound = std::search( fname.begin(), fname.end(), prefix.begin(), prefix.end() );

        //    if( itfound != fname.end() && 
        //        fname.size() == (prefix.size() + 2) &&     // That size is that of "enter" + a 2 digit number!
        //        std::isdigit( fname[prefix.size()],   std::locale::classic() ) &&
        //        std::isdigit( fname[prefix.size()+1], std::locale::classic() ))  //Check if the last 2 characters are digits!
        //    {
        //        string fext = itqueue->getExtension();
        //        if( fext == filetypes::SSB_FileExt )
        //        {
        //            LoadSSB( tgtgrp, itqueue->toString() );
        //        }
        //        else if( fext == filetypes::SSS_FileExt )
        //        {
        //            clog << "Found an extra " <<prefix <<".sss file!! Not supposed to happen!!\n";
        //            assert(false);
        //        }
        //        else if( fext == filetypes::SSE_FileExt )
        //        {
        //            clog << "Found an extra " <<prefix <<".sse file!! Not supposed to happen!!\n";
        //            assert(false);
        //        }
        //        else
        //        {
        //            clog << "Found " <<prefix <<" component with unexpected file extension! " <<itqueue->getFileName() <<"\n";
        //            assert(false);
        //        }
        //        fqueue.erase(itqueue);
        //    }

        //}
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
                itfounddata = itqueue;
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
        LoadScrDataAndMatchedNumberedSSBs( ScriptNames_enter, filetypes::SSE_FileExt, eScriptGroupType::UNK_enter, fqueue, out_scrset );


        ////Find if we have a enter.sse file first.
        //auto itfoundenter = fqueue.end();

        ////### Find if we have a enter.sse file! ###
        //for( auto itqueue = fqueue.begin(); itqueue != fqueue.end(); ++itqueue )
        //{
        //    if( itqueue->getBaseName() == ScriptNames_enter )
        //        itfoundenter = itqueue;
        //}

        ////If we don't return early!
        //if( itfoundenter == fqueue.end() )
        //{
        //    if( utils::LibWide().isLogOn() )
        //        clog << "\n\tNo " <<ScriptNames_enter <<" data found!\n";
        //    return;
        //}
        //else
        //{
        //    clog <<"\n\tLoading " <<ScriptPrefix_enter <<"." <<filetypes::SSE_FileExt <<" and its dependencies..";
        //}

        ////### Otherwise keep going and load the sse ###
        //ScriptGroup entergrp( ScriptPrefix_enter, eScriptGroupType::UNK_enter );
        //LoadSSData( entergrp, itfoundenter->toString() );
        ////Pop it from the queue
        //fqueue.erase(itfoundenter);

        //LoadNumberedSSBForPrefix( fqueue, ScriptPrefix_enter, entergrp );
        
        //### Then find the matching ssb ###
        //for( auto itqueue = fqueue.begin(); itqueue != fqueue.end(); ++itqueue )
        //{
        //    string fname   = itqueue->getBaseName();
        //    auto   itfound = std::search( ScriptPrefix_enter.begin(), ScriptPrefix_enter.end(), fname.begin(), fname.end() );

        //    if( itfound != fname.end() && 
        //        fname.size() == (ScriptPrefix_enter.size() + 2) &&     // That size is that of "enter" + a 2 digit number!
        //       std::isdigit( fname.back(), std::locale::classic() ) )  //Check if the last character is a digit!
        //    {
        //        string fext = itqueue->getExtension();
        //        if( fext == filetypes::SSB_FileExt )
        //        {
        //            LoadSSB( entergrp, itqueue->toString() );
        //        }
        //        else if( fext == filetypes::SSE_FileExt )
        //        {
        //            clog << "Found an extra enter.sse file!! Not supposed to happen!!\n";
        //            assert(false);
        //        }
        //        else
        //        {
        //            clog << "Found enter component with unexpected file extension! " <<itqueue->getFileName() <<"\n";
        //            assert(false);
        //        }
        //        fqueue.erase(itqueue);
        //    }

        //}
        //out_scrset.Components().push_back(std::move(entergrp));
    }

    void GameScriptsHandler::LoadGrpDus( std::deque<Poco::Path> & fqueue, ScriptSet & out_scrset )
    {
        LoadScrDataAndMatchedNumberedSSBs( ScriptNames_dus, filetypes::SSS_FileExt, eScriptGroupType::UNK_dus, fqueue, out_scrset );

        ////Find if we have a dus.sss file first.
        //auto itfounddus = fqueue.end();

        ////### Find if we have a dus.sss file! ###
        //for( auto itqueue = fqueue.begin(); itqueue != fqueue.end(); ++itqueue )
        //{
        //    if( itqueue->getBaseName() == ScriptNames_dus )
        //        itfounddus = itqueue;
        //}

        ////If we don't return early!
        //if( itfounddus == fqueue.end() )
        //{
        //    if( utils::LibWide().isLogOn() )
        //        clog << "\t- No " <<ScriptNames_dus <<" data found!\n";
        //    return;
        //}
        //else
        //{
        //    clog <<"\n\tLoading " <<ScriptNames_dus <<"." <<filetypes::SSS_FileExt <<" and its dependencies..";
        //}

        ////### Otherwise keep going and load the sse ###
        //ScriptGroup dusgrp( ScriptPrefix_dus, eScriptGroupType::UNK_dus );
        //LoadSSData( dusgrp, itfounddus->toString() );
        ////Pop it from the queue
        //fqueue.erase(itfounddus);

        //LoadNumberedSSBForPrefix( fqueue, ScriptPrefix_dus, dusgrp );

        //for( auto itqueue = fqueue.begin(); itqueue != fqueue.end(); ++itqueue )
        //{
        //    string fname   = itqueue->getBaseName();
        //    auto   itfound = std::search( ScriptPrefix_dus.begin(), ScriptPrefix_dus.end(), fname.begin(), fname.end() );

        //    if( itfound != fname.end() && 
        //        fname.size() == (ScriptPrefix_dus.size() + 2) &&     // That size is that of "dus" + a 2 digit number!
        //       std::isdigit( fname.back(), std::locale::classic() ) )  //Check if the last character is a digit! )
        //    {
        //        string fext = itqueue->getExtension();
        //        if( fext == filetypes::SSB_FileExt )
        //        {
        //            LoadSSB( dusgrp, itqueue->toString() );
        //        }
        //        else if( fext == filetypes::SSS_FileExt )
        //        {
        //            clog << "Found an extra dus.sss file!! Not supposed to happen!!\n";
        //            assert(false);
        //        }
        //        else
        //        {
        //            clog << "Found dus component with unexpected file extension! " <<itqueue->getFileName() <<"\n";
        //            assert(false);
        //        }
        //        fqueue.erase(itqueue);
        //    }
        //}
        //out_scrset.Components().push_back(std::move(dusgrp));

        //If we do, parse the dusXX.ssb files too.
    }

    void GameScriptsHandler::LoadLoneSSS( std::deque<Poco::Path> & fqueue, ScriptSet & out_scrset )
    {
        auto itfoundlsd = std::find_if( fqueue.begin(), fqueue.end(), [&](const Poco::Path & entry)->bool
        {
            return utils::CompareStrIgnoreCase(entry.getExtension(),  filetypes::SSS_FileExt );
        });
        if( itfoundlsd != fqueue.end() )
        {
            clog << "\n\tParsing lone sss files..";
            ScriptGroup lonesss( itfoundlsd->getBaseName(), eScriptGroupType::UNK_loneSSS );
            LoadSSData( lonesss, itfoundlsd->toString() );
            out_scrset.Components().push_back( std::move(lonesss) );
            clog <<"\n";
        }
    }

    void GameScriptsHandler::LoadAUGrp( std::deque<Poco::Path> & fqueue, ScriptSet & out_scrset, std::deque<Poco::Path>::iterator itfound )
    {
        Poco::Path  curfile     = *itfound;
        string      curbasename = curfile.getBaseName();
        ScriptGroup ugrp( curbasename, eScriptGroupType::UNK_u );
        fqueue.erase(itfound);

        clog << "\n\tFound " <<curbasename <<" u prefixed script data file..\n";

        //Load the uLXX.sss file first.
        LoadSSData( ugrp, curfile.toString() );

        //Then the matching uLXXYY.ssb files.
        LoadNumberedSSBForPrefix( fqueue, curfile.getBaseName(), ugrp );

        //Add the group to the list
        out_scrset.Components().push_back( std::move(ugrp) );
    }

    void GameScriptsHandler::LoadGrpU( std::deque<Poco::Path> & fqueue, ScriptSet & out_scrset )
    {
        static auto lambdafindu = []( const Poco::Path & ap )->bool
        {
            if( utils::CompareStrIgnoreCase(ap.getExtension(), filetypes::SSS_FileExt) ) //Check if we got the right extension
            {
                string fname = ap.getBaseName();
                if( (fname.size() == ScriptNameLen_U ) &&                 //Check if the length is even valid
                    std::isalpha( fname[1], std::locale::classic() ) &&   //Check if it has a letter after the u prefix
                    std::isdigit( fname[2], std::locale::classic() ) && 
                    std::isdigit( fname[3], std::locale::classic() ))     //Check if it ends with 2 digits
                {
                    return (fname.front() == ScriptPrefix_u.front()); //Check if the filename contains the prefix at the beginning of it
                }
            }
            return false;
        };

        //Find if we have some uLXX.sss files first.
        auto itfound = fqueue.end();

        do
        {
            itfound = std::find_if( fqueue.begin(), fqueue.end(), lambdafindu ); 

            if( itfound != fqueue.end() )
                LoadAUGrp( fqueue, out_scrset, itfound );

        }while( itfound != fqueue.end() );
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
                fname.push_back( std::tolower( c, std::locale::classic() ) );

            Poco::Path ssbpath(curdir);
            Poco::Path ssapath(curdir);
            ssbpath.append(fname).setExtension(filetypes::SSB_FileExt);
            ssapath.append(fname).setExtension(filetypes::SSA_FileExt);

            //#2 - Pop those out of the queue + verify if they exist at the same time
            auto itfoundssb = std::find_if( std::begin(fqueue), std::end(fqueue), [&ssbpath]( const Poco::Path & path )->bool
            {
                return (ssbpath.toString() == path.toString());
            } );
            auto itfoundssa = std::find_if( std::begin(fqueue), std::end(fqueue), [&ssapath]( const Poco::Path & path )->bool
            {
                return (ssapath.toString() == path.toString());
            } );

            if( itfoundssb != std::end(fqueue) )
                fqueue.erase(itfoundssb);
            else
                throw std::runtime_error("GameScriptsHandler::LoadGrpLSDContent(): Expected SSB file named" + ssbpath.toString() + ", but couldn't find it..");

            if( itfoundssa != std::end(fqueue) )
                fqueue.erase(itfoundssa);
            else
                throw std::runtime_error("GameScriptsHandler::LoadGrpLSDContent(): Expected SSA file named" + ssapath.toString() + ", but couldn't find it..");

            //#3 - Handle the files
            ScriptGroup scrpair( string( std::begin(afile), std::end(afile) ), eScriptGroupType::UNK_fromlsd );
            LoadSSB   ( scrpair, ssbpath.toString() );
            LoadSSData( scrpair, ssapath.toString() );

            //#4 - Push the pair into the set!
            out_scrset.Components().push_back( std::move(scrpair) );
        }

    }


    void GameScriptsHandler::LoadDirectory(const std::string & path)
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
            if(itdir->isFile() && !itdir->isHidden())
                processqueue.push_back(itdir.path());
            ++itdir;
        }

        //#1 Check for unionall.ssb
        {
            auto itfoundlsd = std::find_if( processqueue.begin(), processqueue.end(), [&](const Path & entry)->bool
            {
                return utils::CompareStrIgnoreCase(entry.getFileName(),  ScriptNames_unionall );
            });
            if( itfoundlsd != processqueue.end() )
            {
                clog << "\n\tParsing unionall.ssb..";
                ScriptGroup unionall( "unionall", eScriptGroupType::UNK_unionall );
                LoadSSB( unionall, itfoundlsd->toString() );
                curset.Components().push_back( std::move(unionall) );
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

        //#4 Check for dus.sss/dusXX.ssb
        LoadGrpDus(processqueue, curset);

        //#5 Load script files from LSD
        if( !curset.LSDTable().empty() )
        {
            clog <<"\n\tLoading LSD references..";
            LoadGrpLSDContent( curdir, processqueue, curset );
            clog <<"\n";
        }

        //#6 Load 'u' prefixed files
        LoadGrpU( processqueue, curset );

        //#7 Load lone sss files.
        LoadLoneSSS(processqueue, curset);

        //#8 - Add the set to the list
        m_parent.m_sets.emplace( std::make_pair( curdir.getBaseName(), std::move(curset) ) );


        //#9 - List files remaining in the queue
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

    }


//==============================================================================
//  GameScripts
//==============================================================================


    GameScripts::GameScripts(const std::string & scrdir, eGameLocale loc, eGameVersion gver )
        :m_scriptdir(scrdir), m_scrloc(loc), m_gameversion(gver), m_pHandler(new GameScriptsHandler(*this))
    {}

    GameScripts::~GameScripts()
    {
    }

    //File IO
    void GameScripts::Load ()
    {
        m_pHandler->Load();
    }

    void GameScripts::Write()
    {
        m_pHandler->Write();
    }

    ScriptSet * GameScripts::AccessScriptSet(const std::string & setname)
    {
        return nullptr;
    }

    //Access


};
