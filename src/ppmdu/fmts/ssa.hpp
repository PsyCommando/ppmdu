#ifndef SSA_HPP
#define SSA_HPP
/*
ssa.hpp

Description: Utilities for handling SSA/SSE/SSS files.
*/
#include <cstdint>
#include <vector>
#include <string>
#include <utils/utility.hpp>
#include <ppmdu/pmd2/pmd2_scripts.hpp>

namespace filetypes
{
    const std::string SSA_FileExt = "ssa";
    const std::string SSE_FileExt = "sse";
    const std::string SSS_FileExt = "sss";

    /*
        ssa_header
            Header for the SSA/SSE/SSS files.
    */
    struct ssa_header
    {
        static const size_t LEN = 18;
        uint16_t nblayers;
        uint16_t ptrlayertbl;
        uint16_t actionptr;
        uint16_t actorsptr;
        uint16_t objectsptr;
        uint16_t performersptr;
        uint16_t eventsptr;
        uint16_t posmarksptr;
        uint16_t unk3ptr;

        
        // 
        template<class _outit>
            _outit Write( _outit itw )const
        {
            itw = utils::WriteIntToBytes( nblayers,         itw );
            itw = utils::WriteIntToBytes( ptrlayertbl,      itw );
            itw = utils::WriteIntToBytes( actionptr,        itw );
            itw = utils::WriteIntToBytes( actorsptr,        itw );
            itw = utils::WriteIntToBytes( objectsptr,       itw );
            itw = utils::WriteIntToBytes( performersptr,    itw );
            itw = utils::WriteIntToBytes( eventsptr,        itw );
            itw = utils::WriteIntToBytes( posmarksptr,      itw );
            itw = utils::WriteIntToBytes( unk3ptr,          itw );
            return itw;
        }

        //
        template<class _fwdinit>
            _fwdinit Read( _fwdinit itr, _fwdinit itpend )
        {
            itr = utils::ReadIntFromBytes( nblayers,        itr, itpend );
            itr = utils::ReadIntFromBytes( ptrlayertbl,     itr, itpend );
            itr = utils::ReadIntFromBytes( actionptr,       itr, itpend );
            itr = utils::ReadIntFromBytes( actorsptr,       itr, itpend );
            itr = utils::ReadIntFromBytes( objectsptr,      itr, itpend );
            itr = utils::ReadIntFromBytes( performersptr,   itr, itpend );
            itr = utils::ReadIntFromBytes( eventsptr,       itr, itpend );
            itr = utils::ReadIntFromBytes( posmarksptr,     itr, itpend );
            itr = utils::ReadIntFromBytes( unk3ptr,         itr, itpend );
            return itr;
        }
    };

//=======================================================================================
//  Functions
//=======================================================================================
    /*
    */
    pmd2::ScriptData ParseScriptData( const std::string & fpath );
    void             WriteScriptData( const std::string & fpath, const pmd2::ScriptData & scrdat );
};

#endif