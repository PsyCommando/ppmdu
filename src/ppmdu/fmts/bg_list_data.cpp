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
        auto itstr = out_str.begin();
        for( size_t i = 0; i < LevelBgEntry::LevelFnameMaxLen; ++i, ++itstr, ++itwhere )
            *itstr = *itwhere;
        
        //std::copy_n( itwhere, LevelBgEntry::LevelFnameMaxLen, out_str.begin() );

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
                LoadLevelFNameString(itwhere, itend,lvlextraname);
                curentry.extranames.push_back(lvlextraname);
            }
            else
                std::advance( itwhere, LevelBgEntry::LevelFnameMaxLen );
            //else //If the string is empty
            //{
            //    std::advance( itwhere, (LevelBgEntry::EntryLen - ((cntname+3) * LevelBgEntry::LevelFnameMaxLen) ) ); //Skip over the rest of the bytes from this entry
            //    break; //If that entry is null, all the others will be too
            //}
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
        lvlbglist_t     destlst;
        //ifstream        infile(bgfilefpath, std::ios::in | std::ios::binary | std::ios::ate);
        //std::streamoff  fsize = infile.tellg();
        //infile.seekg(0, ios::beg);
        //infile.exceptions(std::ios::badbit);


        //
        std::vector<uint8_t> fdata = utils::io::ReadFileToByteVector(bgfilefpath);

        //!FIXME : THOSE ENDED UP INSERTING RANDOM ZEROS IN THE STUFF WE READ!
        //istreambuf_iterator<char> itf(infile);
        //istreambuf_iterator<char> itend;

        //Safety check
        const size_t    nbentries = fdata.size() / LevelBgEntry::EntryLen;
        auto            itbeg     = fdata.begin();

        for( size_t cntentry = 0; cntentry < nbentries; ++cntentry )
            destlst.push_back( LoadLevelEntry(itbeg, fdata.end()) );

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