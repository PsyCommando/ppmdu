#ifndef SSB_HPP
#define SSB_HPP
/*
ssb.hpp
2015/09/11
psycommando@gmail.com
Description:
    Handles the parsing of script data from SSB files.
*/
#include <cstdint>
#include <vector>
#include <utils/utility.hpp>
#include <ppmdu/pmd2/pmd2.hpp>
#include <ppmdu/pmd2/pmd2_scripts.hpp>

namespace filetypes
{
    const std::string SSB_FileExt = "ssb";


    /***********************************************************************************
        ssb_header
            Represent the header of a ssb file.
    ***********************************************************************************/
    struct ssb_header
    {
        static const unsigned int LEN = 12; //bytes
        static unsigned int   size() { return LEN; }

        uint16_t nbconst      = 0;
        uint16_t nbstrs       = 0;
        uint16_t scriptdatlen = 0; //Counted in 16bits words
        uint16_t consttbllen  = 0; //Counted in 16bits words
        uint16_t strtbllen    = 0; //Counted in 16bits words
        uint16_t unk1         = 0;

        //
        template<class _outit>
        _outit WriteToContainer(_outit itwriteto)const
        {
            itwriteto = utils::WriteIntToBytes(nbconst,      itwriteto);
            itwriteto = utils::WriteIntToBytes(nbstrs,       itwriteto);
            itwriteto = utils::WriteIntToBytes(scriptdatlen, itwriteto);
            itwriteto = utils::WriteIntToBytes(consttbllen,  itwriteto);
            itwriteto = utils::WriteIntToBytes(strtbllen,    itwriteto);
            itwriteto = utils::WriteIntToBytes(unk1,         itwriteto);
            return itwriteto;
        }

        //
        template<class _init>
        _init ReadFromContainer(_init itReadfrom, _init itpastend)
        {
            itReadfrom = utils::ReadIntFromBytes(nbconst,       itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes(nbstrs,        itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes(scriptdatlen,  itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes(consttbllen,   itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes(strtbllen,     itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes(unk1,          itReadfrom, itpastend );
            return itReadfrom;
        }
    };

    /***********************************************************************************
        ssb_header_pal
            Represent the header of a PAL ssb file.
    ***********************************************************************************/
    struct ssb_header_pal
    {
        static const unsigned int LEN = 18; //bytes
        static unsigned int   size() { return LEN; }

        uint16_t nbconst      = 0;
        uint16_t nbstrs       = 0;
        uint16_t scriptdatlen = 0; //Counted in 16bits words
        uint16_t consttbllen  = 0; //Counted in 16bits words
        uint16_t strenglen    = 0; //Counted in 16bits words
        uint16_t strfrelen    = 0; //Counted in 16bits words
        uint16_t strgerlen    = 0; //Counted in 16bits words
        uint16_t stritalen    = 0; //Counted in 16bits words
        uint16_t strspalen    = 0; //Counted in 16bits words

        //
        template<class _outit>
        _outit WriteToContainer(_outit itwriteto)const
        {
            itwriteto = utils::WriteIntToBytes(nbconst,      itwriteto);
            itwriteto = utils::WriteIntToBytes(nbstrs,       itwriteto);
            itwriteto = utils::WriteIntToBytes(scriptdatlen, itwriteto);
            itwriteto = utils::WriteIntToBytes(consttbllen,  itwriteto);
            itwriteto = utils::WriteIntToBytes(strenglen,    itwriteto);
            itwriteto = utils::WriteIntToBytes(strfrelen,    itwriteto);
            itwriteto = utils::WriteIntToBytes(strgerlen,    itwriteto);
            itwriteto = utils::WriteIntToBytes(stritalen,    itwriteto);
            itwriteto = utils::WriteIntToBytes(strspalen,    itwriteto);
            return itwriteto;
        }

        //
        template<class _init>
        _init ReadFromContainer(_init itReadfrom, _init itpastend)
        {
            itReadfrom = utils::ReadIntFromBytes(nbconst,       itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes(nbstrs,        itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes(scriptdatlen,  itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes(consttbllen,   itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes(strenglen,     itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes(strfrelen,     itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes(strgerlen,     itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes(stritalen,     itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes(strspalen,     itReadfrom, itpastend );
            return itReadfrom;
        }
    };


    /***********************************************************************************
        ssb_data_hdr
            Header of the data block of a script.
    ***********************************************************************************/
    struct ssb_data_hdr
    {
        static const size_t LEN     = 4; //bytes
        uint16_t            datalen = 0;   //Lenght of the data block
        uint16_t            nbgrps  = 0;   //Nb of groups in the data block

        template<class _outit>
            _outit WriteToContainer(_outit itwriteto)const
        {
            itwriteto = utils::WriteIntToBytes(datalen,      itwriteto);
            itwriteto = utils::WriteIntToBytes(nbgrps,       itwriteto);
            return itwriteto;
        }

        //
        template<class _init>
            _init ReadFromContainer(_init itReadfrom, _init itpastend)
        {
            itReadfrom = utils::ReadIntFromBytes(datalen, itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes(nbgrps,  itReadfrom, itpastend );
            return itReadfrom;
        }
    };


//=======================================================================================
//  Functions
//=======================================================================================

    /***********************************************************************************
        ParseScript
    ***********************************************************************************/
    pmd2::ScriptedSequence ParseScript( const std::string & scriptfile, pmd2::eGameLocale gloc, pmd2::eGameVersion gvers );
    //pmd2::ScriptedSequence ParseScriptEoS    (const std::string & scriptfile);
    //pmd2::ScriptedSequence ParseScriptEoSPal (const std::string & scriptfile);
    //pmd2::ScriptedSequence ParseScriptEoTD   (const std::string & scriptfile);
    //pmd2::ScriptedSequence ParseScriptEoTDPal(const std::string & scriptfile);

    /***********************************************************************************
        WriteScript
    ***********************************************************************************/
    void                   WriteScript(const std::string & scriptfile, const pmd2::ScriptedSequence & scrdat);
    void                   WriteScriptPal(const std::string & scriptfile, const pmd2::ScriptedSequence & scrdat);
};

#endif 