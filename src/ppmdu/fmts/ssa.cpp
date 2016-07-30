#include "ssa.hpp"
#include <utils/utility.hpp>
#include <ppmdu/pmd2/pmd2_scripts.hpp>
#include <iostream>
using namespace std;

namespace filetypes
{
    const int16_t ScriptDataPaddingWord = -1;
//=======================================================================================
//  DataFormat
//=======================================================================================

    /*
        LayerEntry
            Entry from the layer list.
    */
    struct LayerEntry
    {
        static const size_t LEN = 20;

        int16_t nblives;
        int16_t offlives;

        int16_t nbobjects;
        int16_t offobjects;

        int16_t nbperformers;
        int16_t offperformers;

        int16_t nbevents;
        int16_t offevents;

        int16_t nbentlistE;
        int16_t offlistE;

        template<class _outit>
            _outit Write(_outit itwriteto)const
        {
            itwriteto = utils::WriteIntToBytes(nblives,         itwriteto);
            itwriteto = utils::WriteIntToBytes(offlives,        itwriteto);
            itwriteto = utils::WriteIntToBytes(nbobjects,       itwriteto);
            itwriteto = utils::WriteIntToBytes(offobjects,      itwriteto);
            itwriteto = utils::WriteIntToBytes(nbperformers,    itwriteto);
            itwriteto = utils::WriteIntToBytes(offperformers,   itwriteto);
            itwriteto = utils::WriteIntToBytes(nbevents,        itwriteto);
            itwriteto = utils::WriteIntToBytes(offevents,       itwriteto);
            itwriteto = utils::WriteIntToBytes(nbentlistE,      itwriteto);
            itwriteto = utils::WriteIntToBytes(offlistE,        itwriteto);
            return itwriteto;
        }

        //
        template<class _init>
            _init Read(_init itReadfrom, _init itpastend)
        {
            itReadfrom = utils::ReadIntFromBytes(nblives,         itReadfrom, itpastend);
            itReadfrom = utils::ReadIntFromBytes(offlives,        itReadfrom, itpastend);
            itReadfrom = utils::ReadIntFromBytes(nbobjects,       itReadfrom, itpastend);
            itReadfrom = utils::ReadIntFromBytes(offobjects,      itReadfrom, itpastend);
            itReadfrom = utils::ReadIntFromBytes(nbperformers,    itReadfrom, itpastend);
            itReadfrom = utils::ReadIntFromBytes(offperformers,   itReadfrom, itpastend);
            itReadfrom = utils::ReadIntFromBytes(nbevents,        itReadfrom, itpastend);
            itReadfrom = utils::ReadIntFromBytes(offevents,       itReadfrom, itpastend);
            itReadfrom = utils::ReadIntFromBytes(nbentlistE,      itReadfrom, itpastend);
            itReadfrom = utils::ReadIntFromBytes(offlistE,        itReadfrom, itpastend);
            return itReadfrom;
        }
    };


    /*
        livesentry
            
    */
    struct livesentry
    {
        static const size_t LEN = 16; //14 if not counting the last padding word!

        int16_t unk0;
        int16_t unk1;   //Is a byte
        int16_t unk2;   //Is a byte
        int16_t unk3;   //Is a byte
        int16_t unk4;   //Is a byte
        int16_t unk5;   //Is a byte
        int16_t unk6;
        // <- Padding word is here

        template<class _outit>
            _outit WriteToContainer(_outit itw)const
        {
            itw = utils::WriteIntToBytes(unk0,   itw);
            itw = utils::WriteIntToBytes(unk1,   itw);
            itw = utils::WriteIntToBytes(unk2,   itw);
            itw = utils::WriteIntToBytes(unk3,   itw);
            itw = utils::WriteIntToBytes(unk4,   itw);
            itw = utils::WriteIntToBytes(unk5,   itw);
            itw = utils::WriteIntToBytes(unk6,   itw);
            //Padding word
            itw = utils::WriteIntToBytes(ScriptDataPaddingWord,   itw);
            return itw;
        }

        //
        template<class _fwdinit>
            _fwdinit Read(_fwdinit itr, _fwdinit itpend)
        {
            itr = utils::ReadIntFromBytes(unk0,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk1,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk2,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk3,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk4,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk5,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk6,   itr, itpend);
            //Padding word
            std::advance(itr, sizeof(int16_t));
            return itr;
        }

        operator pmd2::LivesDataEntry()
        {
            pmd2::LivesDataEntry out;
            out.unk0 = unk0;
            out.unk1 = unk1;
            out.unk2 = unk2;
            out.unk3 = unk3;
            out.unk4 = unk4;
            out.unk5 = unk5;
            out.unk6 = unk6;
            return std::move(out);
        }
    };


    /*
        objectentry
            
    */
    struct objectentry
    {
        static const size_t LEN = 20; //18 if not counting the last padding word!

        int16_t unk0;
        int16_t unk1;   
        int16_t unk2;   
        int16_t unk3;  
        int16_t unk4;  
        int16_t unk5;  
        int16_t unk6;
        int16_t unk7;
        int16_t unk8;
        // <- Padding word is here

        template<class _outit>
            _outit WriteToContainer(_outit itw)const
        {
            itw = utils::WriteIntToBytes(unk0,   itw);
            itw = utils::WriteIntToBytes(unk1,   itw);
            itw = utils::WriteIntToBytes(unk2,   itw);
            itw = utils::WriteIntToBytes(unk3,   itw);
            itw = utils::WriteIntToBytes(unk4,   itw);
            itw = utils::WriteIntToBytes(unk5,   itw);
            itw = utils::WriteIntToBytes(unk6,   itw);
            itw = utils::WriteIntToBytes(unk7,   itw);
            itw = utils::WriteIntToBytes(unk8,   itw);
            //Padding word
            itw = utils::WriteIntToBytes(ScriptDataPaddingWord,   itw);
            return itw;
        }

        //
        template<class _fwdinit>
            _fwdinit Read(_fwdinit itr, _fwdinit itpend)
        {
            itr = utils::ReadIntFromBytes(unk0,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk1,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk2,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk3,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk4,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk5,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk6,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk7,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk8,   itr, itpend);
            //Padding word
            std::advance(itr, sizeof(int16_t));
            return itr;
        }

        operator pmd2::ObjectDataEntry()
        {
            pmd2::ObjectDataEntry out;
            out.unk0 = unk0;
            out.unk1 = unk1;
            out.unk2 = unk2;
            out.unk3 = unk3;
            out.unk4 = unk4;
            out.unk5 = unk5;
            out.unk6 = unk6;
            out.unk7 = unk7;
            out.unk8 = unk8;
            return std::move(out);
        }
    };


    /*
        performerentry
            
    */
    struct performerentry
    {
        static const size_t LEN = 20;

        int16_t unk0;
        int16_t unk1;   
        int16_t unk2;   
        int16_t unk3;  
        int16_t unk4;  
        int16_t unk5;  
        int16_t unk6;
        int16_t unk7;
        int16_t unk8;
        int16_t unk9;

        template<class _outit>
            _outit WriteToContainer(_outit itw)const
        {
            itw = utils::WriteIntToBytes(unk0,   itw);
            itw = utils::WriteIntToBytes(unk1,   itw);
            itw = utils::WriteIntToBytes(unk2,   itw);
            itw = utils::WriteIntToBytes(unk3,   itw);
            itw = utils::WriteIntToBytes(unk4,   itw);
            itw = utils::WriteIntToBytes(unk5,   itw);
            itw = utils::WriteIntToBytes(unk6,   itw);
            itw = utils::WriteIntToBytes(unk7,   itw);
            itw = utils::WriteIntToBytes(unk8,   itw);
            itw = utils::WriteIntToBytes(unk9,   itw);
            return itw;
        }

        //
        template<class _fwdinit>
            _fwdinit Read(_fwdinit itr, _fwdinit itpend)
        {
            itr = utils::ReadIntFromBytes(unk0,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk1,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk2,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk3,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk4,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk5,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk6,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk7,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk8,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk9,   itr, itpend);
            return itr;
        }

        operator pmd2::PerformerDataEntry()
        {
            pmd2::PerformerDataEntry out;
            out.unk0 = unk0;
            out.unk1 = unk1;
            out.unk2 = unk2;
            out.unk3 = unk3;
            out.unk4 = unk4;
            out.unk5 = unk5;
            out.unk6 = unk6;
            out.unk7 = unk7;
            out.unk8 = unk8;
            out.unk9 = unk9;
            return std::move(out);
        }
    };


    /*
        evententry
            
    */
    struct evententry
    {
        static const size_t LEN = 16; //14 if not counting the padding word

        int16_t unk0;
        int16_t unk1;   
        int16_t unk2;   
        int16_t unk3;  
        int16_t unk4;  
        int16_t unk5;  
        int16_t unk6;
        // <- Padding word is here

        template<class _outit>
            _outit WriteToContainer(_outit itw)const
        {
            itw = utils::WriteIntToBytes(unk0,   itw);
            itw = utils::WriteIntToBytes(unk1,   itw);
            itw = utils::WriteIntToBytes(unk2,   itw);
            itw = utils::WriteIntToBytes(unk3,   itw);
            itw = utils::WriteIntToBytes(unk4,   itw);
            itw = utils::WriteIntToBytes(unk5,   itw);
            itw = utils::WriteIntToBytes(unk6,   itw);

            //Padding word
            itw = utils::WriteIntToBytes(ScriptDataPaddingWord,   itw);
            return itw;
        }

        //
        template<class _fwdinit>
            _fwdinit Read(_fwdinit itr, _fwdinit itpend)
        {
            itr = utils::ReadIntFromBytes(unk0,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk1,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk2,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk3,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk4,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk5,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk6,   itr, itpend);

            //Padding word
            std::advance(itr, sizeof(int16_t));
            return itr;
        }

        operator pmd2::EventDataEntry()
        {
            pmd2::EventDataEntry out;
            out.unk0 = unk0;
            out.unk1 = unk1;
            out.unk2 = unk2;
            out.unk3 = unk3;
            out.unk4 = unk4;
            out.unk5 = unk5;
            out.unk6 = unk6;
            return std::move(out);
        }
    };

    /*
        unkentry
            
    */
    struct unkentry
    {
        static const size_t LEN = 8;

        int16_t unk0;
        int16_t unk1;   
        int16_t unk2;   
        int16_t unk3;  

        template<class _outit>
            _outit WriteToContainer(_outit itw)const
        {
            itw = utils::WriteIntToBytes(unk0,   itw);
            itw = utils::WriteIntToBytes(unk1,   itw);
            itw = utils::WriteIntToBytes(unk2,   itw);
            itw = utils::WriteIntToBytes(unk3,   itw);
            return itw;
        }

        //
        template<class _fwdinit>
            _fwdinit Read(_fwdinit itr, _fwdinit itpend)
        {
            itr = utils::ReadIntFromBytes(unk0,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk1,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk2,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk3,   itr, itpend);
            return itr;
        }

        operator pmd2::UnkScriptDataEntry()
        {
            pmd2::UnkScriptDataEntry out;
            out.unk0 = unk0;
            out.unk1 = unk1;
            out.unk2 = unk2;
            out.unk3 = unk3;
            return std::move(out);
        }
    };


//=======================================================================================
//  SSDataParser
//=======================================================================================
    template<typename _init>
    class SSDataParser
    {
    public:
        typedef _init initer_t;

        SSDataParser( _init itbeg, _init itend, const std::string & origfname )
            :m_itbeg(itbeg), m_itcur(itbeg), m_itend(itend), m_origfname(origfname)
        {}


        pmd2::ScriptData Parse()
        {
            ParseHeader();

            return std::move(m_out);
        }

    private:

        inline void ParseHeader()
        {
            m_itcur = m_header.Read(m_itcur, m_itend);
        }

        void ParseLayers()
        {
            initer_t itlayersbeg = std::next(m_itbeg, m_header.ptrlayertbl * 2);

            for( size_t i = 0; i < m_header.nblayers; ++i )
                itlayersbeg = ParseALayer(itlayersbeg);
        }

        initer_t ParseALayer( initer_t itlayerbeg )
        {
            LayerEntry        tmplent;
            pmd2::ScriptLayer outlay;
            itlayerbeg = tmplent.Read(itlayerbeg);
            ParseEntriesList<livesentry>    ( tmplent.nblives,      tmplent.offlives,       std::back_inserter(outlay.lives) );
            ParseEntriesList<objectentry>   ( tmplent.nbobjects,    tmplent.offobjects,     std::back_inserter(outlay.objects) );
            ParseEntriesList<performerentry>( tmplent.nbperformers, tmplent.offperformers,  std::back_inserter(outlay.performers) );
            ParseEntriesList<evententry>    ( tmplent.nbevents,     tmplent.offevents,      std::back_inserter(outlay.events) );
            ParseEntriesList<unkentry>      ( tmplent.nbentlistE,   tmplent.offlistE,       std::back_inserter(outlay.unkentries) );
            m_out.Layers().push_back(std::move(outlay));
            return itlayerbeg;
        }

        template<class _entryty, typename _backinsit>
            void ParseEntriesList( int16_t nbentries, int16_t entrybeg, _backinsit itout )
        {
            initer_t itentriesbeg = std::next(m_itbeg, entrybeg * 2);
            for( size_t i = 0; i < nbentries; ++i )
            {
                _entryty lv;
                itentriesbeg = lv.Read(itentriesbeg, m_itend);
                *itout = lv;
            }
        }

    private:
        initer_t                m_itbeg;
        initer_t                m_itcur;
        initer_t                m_itend;

        std::string             m_origfname;
        pmd2::ScriptData        m_out;

        ssa_header              m_header;
    };

//=======================================================================================
//  SSDataWriter
//=======================================================================================

//=======================================================================================
//  Functions
//=======================================================================================
    pmd2::ScriptData ParseScriptData( const std::string & fpath )
    {
        vector<uint8_t> fdata( std::move(utils::io::ReadFileToByteVector(fpath)) );
        return std::move( SSDataParser<vector<uint8_t>::const_iterator>(fdata.begin(), fdata.end(), utils::GetFilename(fpath) ).Parse() );
    }

    void WriteScriptData( const std::string & fpath, const pmd2::ScriptData & scrdat )
    {
    }
};