#ifndef SSB_HPP
#define SSB_HPP
/*
ssb.hpp
*/
#include <cstdint>
#include <vector>
#include <utils/utility.hpp>

namespace filetypes
{

    /*
        ssb_header
            Represent the header of a ssb file.
    */
    struct ssb_header
    {
        static const unsigned int Length = 12; //bytes
        static unsigned int   size() { return Length; }

        uint16_t nbconst;
        uint16_t nbstrs;
        uint16_t scriptdatlen;
        uint16_t consttbllen; //In uint16
        uint16_t strtbllen;   //In uint16
        uint16_t unk1;

        //
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( nbconst,     itwriteto );
            itwriteto = utils::WriteIntToBytes( nbstrs,      itwriteto );
            itwriteto = utils::WriteIntToBytes( scriptdatlen,itwriteto );
            itwriteto = utils::WriteIntToBytes( consttbllen, itwriteto );
            itwriteto = utils::WriteIntToBytes( strtbllen,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unk1,        itwriteto );
            return itwriteto;
        }

        //
        template<class _init>
            _init ReadFromContainer(  _init itReadfrom )
        {
            itReadfrom = utils::ReadIntFromBytes( nbconst,     itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( nbstrs,      itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( scriptdatlen,itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( consttbllen, itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( strtbllen,   itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk1,        itReadfrom );
            return itReadfrom;
        }

    };


//
//
//

};

#endif 