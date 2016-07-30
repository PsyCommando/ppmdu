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
        uint16_t nblayers;
        uint16_t ptrlayertbl;
        uint16_t unkdb1ptr;
        uint16_t entdbptr;
        uint16_t propdbptr;
        uint16_t bgdbptr;
        uint16_t unkdb2ptr;
        uint16_t actdbptr;
        uint16_t unk3ptr;

        
        // 
        template<class _outit>
            _outit Write( _outit itw )const
        {
            itw = utils::WriteIntToBytes( nblayers,     itw );
            itw = utils::WriteIntToBytes( ptrlayertbl,  itw );
            itw = utils::WriteIntToBytes( unkdb1ptr,    itw );
            itw = utils::WriteIntToBytes( entdbptr,     itw );
            itw = utils::WriteIntToBytes( propdbptr,    itw );
            itw = utils::WriteIntToBytes( bgdbptr,      itw );
            itw = utils::WriteIntToBytes( unkdb2ptr,    itw );
            itw = utils::WriteIntToBytes( actdbptr,     itw );
            itw = utils::WriteIntToBytes( unk3ptr,      itw );
            return itw;
        }

        //
        template<class _fwdinit>
            _fwdinit Read( _fwdinit itr, _fwdinit itpend )
        {
            itr = utils::ReadIntFromBytes( nblayers,        itr, itpend );
            itr = utils::ReadIntFromBytes( ptrlayertbl,     itr, itpend );
            itr = utils::ReadIntFromBytes( unkdb1ptr,       itr, itpend );
            itr = utils::ReadIntFromBytes( entdbptr,        itr, itpend );
            itr = utils::ReadIntFromBytes( propdbptr,       itr, itpend );
            itr = utils::ReadIntFromBytes( bgdbptr,         itr, itpend );
            itr = utils::ReadIntFromBytes( unkdb2ptr,       itr, itpend );
            itr = utils::ReadIntFromBytes( actdbptr,        itr, itpend );
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