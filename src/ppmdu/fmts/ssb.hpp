#ifndef SSB_HPP
#define SSB_HPP
/*
ssb.hpp
*/
#include <cstdint>
#include <vector>


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
        uint16_t ptrconsts;
        uint16_t consttbllen; //In uint32
        uint16_t strtbllen;   //In uint32
        uint16_t unk1;

        //
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToByteContainer( nbconst,     itwriteto );
            itwriteto = utils::WriteIntToByteContainer( nbstrs,      itwriteto );
            itwriteto = utils::WriteIntToByteContainer( ptrconsts,   itwriteto );
            itwriteto = utils::WriteIntToByteContainer( consttbllen, itwriteto );
            itwriteto = utils::WriteIntToByteContainer( strtbllen,   itwriteto );
            itwriteto = utils::WriteIntToByteContainer( unk1,        itwriteto );
            return itwriteto;
        }

        //
        template<class _init>
            _init ReadFromContainer(  _init itReadfrom )
        {
            itReadfrom = utils::ReadIntFromByteContainer( nbconst,     itReadfrom );
            itReadfrom = utils::ReadIntFromByteContainer( nbstrs,      itReadfrom );
            itReadfrom = utils::ReadIntFromByteContainer( ptrconsts,   itReadfrom );
            itReadfrom = utils::ReadIntFromByteContainer( consttbllen, itReadfrom );
            itReadfrom = utils::ReadIntFromByteContainer( strtbllen,   itReadfrom );
            itReadfrom = utils::ReadIntFromByteContainer( unk1,        itReadfrom );
            return itReadfrom;
        }

    };

};

#endif 