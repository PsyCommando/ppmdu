#include "dse_common.hpp"

#include <ppmdu/fmts/smdl.hpp> //#TODO #MOVEME

#include <sstream>
#include <iomanip>
using namespace std;

namespace DSE
{

    const std::array<eDSEChunks, NB_DSEChunks> DSEChunksList 
    {{
        eDSEChunks::wavi, 
        eDSEChunks::prgi, 
        eDSEChunks::kgrp, 
        eDSEChunks::pcmd,
        eDSEChunks::trk,
        eDSEChunks::seq,
        eDSEChunks::bnkl,
        eDSEChunks::mcrl,
        eDSEChunks::eoc,
        eDSEChunks::eod,
    }};

    inline eDSEChunks IntToChunkID( uint32_t value )
    {
        eDSEChunks valcompare = static_cast<eDSEChunks>(value);
        
        for( auto cid : DSEChunksList )
        {
            if( valcompare == cid )
                return valcompare;
        }

        return eDSEChunks::invalid;
    }
    
    inline uint32_t ChunkIDToInt( eDSEChunks id )
    {
        return static_cast<uint32_t>(id);
    }

    //Global stream operator
    std::ostream & operator<<(std::ostream &os, const DateTime &obj )
    {
        os << static_cast<unsigned long>(obj.year) <<"/" 
           <<static_cast<unsigned long>(obj.month) <<"/" 
           <<static_cast<unsigned long>(obj.day) <<"-" 
           <<static_cast<unsigned long>(obj.hour) <<"h" 
           <<static_cast<unsigned long>(obj.minute) <<"m" 
           <<static_cast<unsigned long>(obj.second) <<"s";
        return os;
    }

};