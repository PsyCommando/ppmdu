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
        uint16_t nbgrp;
        uint16_t grpslen;
        uint16_t unkdb1ptr;
        uint16_t entdbptr;
        uint16_t propdbptr;
        uint16_t bgdbptr;
        uint16_t unkdb2ptr;
        uint16_t actdbptr;
        uint16_t unk3ptr;


        //
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( nbgrp,     itwriteto );
            itwriteto = utils::WriteIntToBytes( grpslen,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unkdb1ptr, itwriteto );
            itwriteto = utils::WriteIntToBytes( entdbptr,  itwriteto );
            itwriteto = utils::WriteIntToBytes( propdbptr, itwriteto );
            itwriteto = utils::WriteIntToBytes( bgdbptr,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unkdb2ptr, itwriteto );
            itwriteto = utils::WriteIntToBytes( actdbptr,  itwriteto );
            itwriteto = utils::WriteIntToBytes( unk3ptr,   itwriteto );
            return itwriteto;
        }

        //
        template<class _init>
            _init ReadFromContainer(  _init itReadfrom )
        {
            itReadfrom = utils::ReadIntFromBytes( nbgrp,     itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( grpslen,   itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unkdb1ptr, itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( entdbptr,  itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( propdbptr, itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( bgdbptr,   itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unkdb2ptr, itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( actdbptr,  itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk3ptr,   itReadfrom );
            return itReadfrom;
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