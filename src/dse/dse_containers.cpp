#include "dse_containers.hpp"
#include <iostream>
#include <sstream>
using namespace std;

namespace DSE
{
    std::string MusicSequence::tostr()const
    {
        stringstream sstr;
        //int cnt = 0;
        //for( const auto & track : m_tracks )
        //{
        //    sstr <<"==== Track" <<cnt << " ====\n\n";
        //    TrackPlaybackState st;
        //    for( const auto & ev : track )
        //    {
        //        sstr << st.printevent( ev ) << "\n";
        //    }
        //    ++cnt;
        //}
        assert(false); //Deprecated!!
        return move(sstr.str());
    }

    std::string MusicSequence::printinfo()const
    {
        stringstream sstr;
        sstr << " ==== " <<m_meta.fname <<" ==== \n"
             << "CREATE ITME : " <<m_meta.createtime <<"\n"
             << "NB TRACKS   : " <<m_tracks.size()   <<"\n"
             << "TPQN        : " <<m_meta.tpqn       <<"\n";

        return sstr.str();
    }
};