#include "bg_list_data.hpp"
#include <iostream>
#include <fstream>
#include <utils/utility.hpp>
using namespace std;

namespace filetypes
{
    const std::string FName_BGListFile = "bg_list.dat";

//============================================================================================
//  Loading
//============================================================================================
    template<class _infwdit>
        void LoadLevelFNameString(_infwdit & itwhere, _infwdit itend, LevelBgEntry::lvlstr_t & out_str )
    {
        if(itwhere == itend)
            throw std::out_of_range("LoadLevelFNameString(): Level backgrounds file ended unexpectedly!");
        std::copy_n( itwhere, LevelBgEntry::LevelFnameMaxLen, out_str.begin() );
    }


    template<class _infwdit>
        LevelBgEntry LoadLevelEntry( _infwdit & itwhere, _infwdit itend )
    {
        LevelBgEntry curentry;

        //Load the 3 obligatory ones
        LoadLevelFNameString(itwhere, itend, curentry.bplname);
        LoadLevelFNameString(itwhere, itend, curentry.bpcname);
        LoadLevelFNameString(itwhere, itend, curentry.bmaname);

        //Load the extra names
        for( size_t cntname = 0; cntname < LevelBgEntry::MaxNbExtraNames; ++cntname )
        {
            if(itwhere == itend)
                throw std::out_of_range("LoadLevelEntry(): Level backgrounds file ended unexpectedly!");

            char c = *itwhere;
            if(c != 0) //If the string is not empty
            {
                LevelBgEntry::lvlstr_t lvlextraname;
                ++itwhere;
                if(itwhere == itend)
                    throw std::out_of_range("LoadLevelEntry(): Level backgrounds file ended unexpectedly!");
                lvlextraname.front() = c;
                std::copy_n( itwhere, (LevelBgEntry::LevelFnameMaxLen-1), lvlextraname.begin() + 1 );
                curentry.extranames.push_back(std::move(lvlextraname));
            }
            else //If the string is empty
            {
                std::advance( itwhere, (LevelBgEntry::EntryLen - ((cntname+3) * LevelBgEntry::LevelFnameMaxLen) ) ); //Skip over the rest of the bytes from this entry
                break; //If that entry is null, all the others will be too
            }
        }
        return std::move(curentry);
    }

//============================================================================================
//  Writing
//============================================================================================
    
    template<class _backinsit>
        void WriteLevelEntry( _backinsit itwhere, const LevelBgEntry & lvlentry )
    {
        //Write the 3 obligatory names first
        itwhere = std::copy(lvlentry.bplname.begin(), lvlentry.bplname.end(), itwhere);
        itwhere = std::copy(lvlentry.bpcname.begin(), lvlentry.bpcname.end(), itwhere);
        itwhere = std::copy(lvlentry.bmaname.begin(), lvlentry.bmaname.end(), itwhere);

        //Write extra names!
        for( size_t cntname = 0; cntname < LevelBgEntry::MaxNbExtraNames; ++cntname )
        {
            if( cntname < lvlentry.extranames.size() )
                itwhere = std::copy(lvlentry.extranames[cntname].begin(), lvlentry.extranames[cntname].end(), itwhere);
            else
                itwhere = std::fill_n(itwhere, LevelBgEntry::LevelFnameMaxLen, 0); //Fill the other extra slots that we don't have with 0 bytes!
        }
    }

//============================================================================================
//  Functions
//============================================================================================
    /*
        LoadLevelList
    */
    lvlbglist_t LoadLevelList ( const std::string & bgfilefpath )
    {
        lvlbglist_t destlst;
        ifstream infile(bgfilefpath, std::ios::in | std::ios::binary);
        infile.exceptions(std::ios::badbit);
        istreambuf_iterator<char> itf(infile);
        istreambuf_iterator<char> itend;

        while( itf != itend )
            destlst.push_back( LoadLevelEntry(itf, itend) );

        return std::move(destlst);
    }
    
    /*
        WriteLevelList
    */
    void WriteLevelList( const std::string & bgfilefpath, const lvlbglist_t & bglist )
    {
        ofstream outfile(bgfilefpath, std::ios::out | std::ios::binary);
        outfile.exceptions(std::ios::badbit);
        ostreambuf_iterator<char> itout(outfile);

        for( const auto & lvl : bglist )
            WriteLevelEntry(itout, lvl);
    }
};