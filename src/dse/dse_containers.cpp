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
             << "TPQN        : " <<m_meta.tpqn       <<"\n\n";

        size_t cnttrk = 0;
        for( const auto & trk : m_tracks )
        {
            if( !trk.empty() )
                sstr << "\t- Track " <<cnttrk << ": Chan " << static_cast<uint16_t>(trk.GetMidiChannel()) << ", constains " << trk.size() <<" event(s).\n";
            ++cnttrk;
        }

        return sstr.str();
    }
};