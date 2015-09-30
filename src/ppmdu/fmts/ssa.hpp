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

namespace filetypes
{

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
            itwriteto = utils::WriteIntToByteContainer( nbgrp,     itwriteto );
            itwriteto = utils::WriteIntToByteContainer( grpslen,   itwriteto );
            itwriteto = utils::WriteIntToByteContainer( unkdb1ptr, itwriteto );
            itwriteto = utils::WriteIntToByteContainer( entdbptr,  itwriteto );
            itwriteto = utils::WriteIntToByteContainer( propdbptr, itwriteto );
            itwriteto = utils::WriteIntToByteContainer( bgdbptr,   itwriteto );
            itwriteto = utils::WriteIntToByteContainer( unkdb2ptr, itwriteto );
            itwriteto = utils::WriteIntToByteContainer( actdbptr,  itwriteto );
            itwriteto = utils::WriteIntToByteContainer( unk3ptr,   itwriteto );
            return itwriteto;
        }

        //
        template<class _init>
            _init ReadFromContainer(  _init itReadfrom )
        {
            itReadfrom = utils::ReadIntFromByteContainer( nbgrp,     itReadfrom );
            itReadfrom = utils::ReadIntFromByteContainer( grpslen,   itReadfrom );
            itReadfrom = utils::ReadIntFromByteContainer( unkdb1ptr, itReadfrom );
            itReadfrom = utils::ReadIntFromByteContainer( entdbptr,  itReadfrom );
            itReadfrom = utils::ReadIntFromByteContainer( propdbptr, itReadfrom );
            itReadfrom = utils::ReadIntFromByteContainer( bgdbptr,   itReadfrom );
            itReadfrom = utils::ReadIntFromByteContainer( unkdb2ptr, itReadfrom );
            itReadfrom = utils::ReadIntFromByteContainer( actdbptr,  itReadfrom );
            itReadfrom = utils::ReadIntFromByteContainer( unk3ptr,   itReadfrom );
            return itReadfrom;
        }
    };

    /*
    */


};

#endif