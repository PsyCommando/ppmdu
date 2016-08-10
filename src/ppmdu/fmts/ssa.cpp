#include "ssa.hpp"
#include <utils/utility.hpp>
#include <ppmdu/pmd2/pmd2_scripts.hpp>
#include <iostream>
#include <fstream>
#include <iomanip>
using namespace std;
using namespace pmd2;

namespace filetypes
{
    const int16_t ScriptDataPaddingWord = -1;
//=======================================================================================
//  DataFormat
//=======================================================================================

    /*
        LayerTableEntry
    */
    struct LayerTableEntry
    {
        int16_t nbentries = 0;
        int16_t offset    = 0;

        template<class _outit>
            _outit Write(_outit itw)const
        {
            itw = utils::WriteIntToBytes(nbentries, itw);
            itw = utils::WriteIntToBytes(offset,    itw);
            return itw;
        }

        template<class _init>
            _init Read(_init itr, _init itpend)
        {
            itr = utils::ReadIntFromBytes(nbentries,    itr, itpend );
            itr = utils::ReadIntFromBytes(offset,       itr, itpend );
            return itr;
        }
    };


    /*
        LayerEntry
            Entry from the layer list.
    */
    struct LayerEntry
    {
        static const size_t LEN = 20;

        enum struct eTableTypes : size_t
        {
            Actors  = 0,
            Objects,
            Performers,
            Events,
            Unk3,

            NbTables,
        };

        std::array<LayerTableEntry, static_cast<size_t>(eTableTypes::NbTables)> tables;

        inline LayerTableEntry       & actors()             {return tables[static_cast<size_t>(eTableTypes::Actors)];}
        inline LayerTableEntry       & objects()            {return tables[static_cast<size_t>(eTableTypes::Objects)];}
        inline LayerTableEntry       & performers()         {return tables[static_cast<size_t>(eTableTypes::Performers)];}
        inline LayerTableEntry       & events()             {return tables[static_cast<size_t>(eTableTypes::Events)];}
        inline LayerTableEntry       & unk3lst()            {return tables[static_cast<size_t>(eTableTypes::Unk3)];}

        inline const LayerTableEntry & actors()const        {return tables[static_cast<size_t>(eTableTypes::Actors)];}
        inline const LayerTableEntry & objects()const       {return tables[static_cast<size_t>(eTableTypes::Objects)];}
        inline const LayerTableEntry & performers()const    {return tables[static_cast<size_t>(eTableTypes::Performers)];}
        inline const LayerTableEntry & events()const        {return tables[static_cast<size_t>(eTableTypes::Events)];}
        inline const LayerTableEntry & unk3lst()const       {return tables[static_cast<size_t>(eTableTypes::Unk3)];}

        template<class _outit>
            _outit Write(_outit itw)const
        {
            for(const auto & entry : tables )
                itw = entry.Write(itw);
            return itw;
        }

        //
        template<class _init>
            _init Read(_init itr, _init itpend)
        {
            for(auto & entry : tables )
                itr = entry.Read(itr, itpend);
            return itr;
        }
    };


    /*
        livesentry
            
    */
    struct livesentry
    {
        static const size_t LEN = 16; //14 if not counting the last padding word!

        int16_t livesid;
        int16_t unk1;   //Is a byte
        int16_t xoff;   //Is a byte
        int16_t yoff;   //Is a byte
        int16_t unk4;   //Is a byte
        int16_t unk5;   //Is a byte
        int16_t scrid;
        // <- Padding word is here

        livesentry()
            :livesid(0), unk1(0), xoff(0), yoff(0), unk4(0),
             unk5(0), scrid(0)
        {}

        livesentry(const livesentry & other)
            :livesid(other.livesid), unk1(other.unk1), xoff(other.xoff), yoff(other.yoff), unk4(other.unk4),
             unk5(other.unk5), scrid(other.scrid)
        {}

        livesentry(const pmd2::LivesDataEntry & other)
            :livesid(other.livesid), unk1(other.unk1), xoff(other.xoff), yoff(other.yoff), unk4(other.unk4),
             unk5(other.unk5), scrid(other.scrid)
        {}

        template<class _outit>
            _outit Write(_outit itw)const
        {
            itw = utils::WriteIntToBytes(livesid,   itw);
            itw = utils::WriteIntToBytes(unk1,   itw);
            itw = utils::WriteIntToBytes(xoff,   itw);
            itw = utils::WriteIntToBytes(yoff,   itw);
            itw = utils::WriteIntToBytes(unk4,   itw);
            itw = utils::WriteIntToBytes(unk5,   itw);
            itw = utils::WriteIntToBytes(scrid,   itw);
            //Padding word
            itw = utils::WriteIntToBytes(ScriptDataPaddingWord,   itw);
            return itw;
        }

        //
        template<class _fwdinit>
            _fwdinit Read(_fwdinit itr, _fwdinit itpend)
        {
            itr = utils::ReadIntFromBytes(livesid,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk1,   itr, itpend);
            itr = utils::ReadIntFromBytes(xoff,   itr, itpend);
            itr = utils::ReadIntFromBytes(yoff,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk4,   itr, itpend);
            itr = utils::ReadIntFromBytes(unk5,   itr, itpend);
            itr = utils::ReadIntFromBytes(scrid,   itr, itpend);
            //Padding word
            std::advance(itr, sizeof(int16_t));
            return itr;
        }

        operator pmd2::LivesDataEntry()
        {
            pmd2::LivesDataEntry out;
            out.livesid = livesid;
            out.unk1 = unk1;
            out.xoff = xoff;
            out.yoff = yoff;
            out.unk4 = unk4;
            out.unk5 = unk5;
            out.scrid = scrid;
            return std::move(out);
        }

        livesentry & operator=(const pmd2::LivesDataEntry & other)
        {
            livesid  = other.livesid;
            unk1     = other.unk1;
            xoff     = other.xoff;
            yoff     = other.yoff;
            unk4     = other.unk4;
            unk5     = other.unk5;
            scrid     = other.scrid;
            return *this;
        }
    };


    /*
        objectentry
            
    */
    struct objectentry
    {
        static const size_t LEN = 20; //18 if not counting the last padding word!

        int16_t objid;
        int16_t unk1;   
        int16_t unk2;   
        int16_t unk3;  
        int16_t unk4;  
        int16_t unk5;  
        int16_t unk6;
        int16_t unk7;
        int16_t unk8;
        // <- Padding word is here

        objectentry()
            :objid(0), unk1(0), unk2(0), unk3(0), unk4(0),
             unk5(0), unk6(0), unk7(0), unk8(0)
        {}

        objectentry(const objectentry & other)
            :objid(other.objid), unk1(other.unk1), unk2(other.unk2), unk3(other.unk3), unk4(other.unk4),
             unk5(other.unk5), unk6(other.unk6), unk7(other.unk7), unk8(other.unk8)
        {}

        objectentry(const pmd2::ObjectDataEntry & other)
            :objid(other.objid), unk1(other.unk1), unk2(other.unk2), unk3(other.unk3), unk4(other.unk4),
             unk5(other.unk5), unk6(other.unk6), unk7(other.unk7), unk8(other.unk8)
        {}

        template<class _outit>
            _outit Write(_outit itw)const
        {
            itw = utils::WriteIntToBytes(objid,   itw);
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
            itr = utils::ReadIntFromBytes(objid,   itr, itpend);
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
            out.objid = objid;
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

        objectentry & operator=(const pmd2::ObjectDataEntry & other)
        {
            objid   = other.objid;
            unk1    = other.unk1;
            unk2    = other.unk2;
            unk3    = other.unk3;
            unk4    = other.unk4;
            unk5    = other.unk5;
            unk6    = other.unk6;
            unk7    = other.unk7;
            unk8    = other.unk8;
            return *this;
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

        performerentry()
            :unk0(0), unk1(0), unk2(0), unk3(0), unk4(0),
             unk5(0), unk6(0), unk7(0), unk8(0), unk9(0)
        {}

        performerentry(const performerentry & other)
            :unk0(other.unk0), unk1(other.unk1), unk2(other.unk2), unk3(other.unk3), unk4(other.unk4),
             unk5(other.unk5), unk6(other.unk6), unk7(other.unk7), unk8(other.unk8), unk9(other.unk9)
        {}

        performerentry(const pmd2::PerformerDataEntry & other)
            :unk0(other.unk0), unk1(other.unk1), unk2(other.unk2), unk3(other.unk3), unk4(other.unk4),
             unk5(other.unk5), unk6(other.unk6), unk7(other.unk7), unk8(other.unk8), unk9(other.unk9)
        {}

        template<class _outit>
            _outit Write(_outit itw)const
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

        performerentry & operator=(const pmd2::PerformerDataEntry & other)
        {
            unk0    = other.unk0;
            unk1    = other.unk1;
            unk2    = other.unk2;
            unk3    = other.unk3;
            unk4    = other.unk4;
            unk5    = other.unk5;
            unk6    = other.unk6;
            unk7    = other.unk7;
            unk8    = other.unk8;
            unk9    = other.unk9;
            return *this;
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

        evententry()
            :unk0(0), unk1(0), unk2(0), unk3(0), unk4(0),
             unk5(0), unk6(0)
        {}

        evententry(const evententry & other)
            :unk0(other.unk0), unk1(other.unk1), unk2(other.unk2), unk3(other.unk3), unk4(other.unk4),
             unk5(other.unk5), unk6(other.unk6)
        {}

        evententry(const pmd2::EventDataEntry & other)
            :unk0(other.unk0), unk1(other.unk1), unk2(other.unk2), unk3(other.unk3), unk4(other.unk4),
             unk5(other.unk5), unk6(other.unk6)
        {}

        template<class _outit>
            _outit Write(_outit itw)const
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

        evententry & operator=(const pmd2::EventDataEntry & other)
        {
            unk0    = other.unk0;
            unk1    = other.unk1;
            unk2    = other.unk2;
            unk3    = other.unk3;
            unk4    = other.unk4;
            unk5    = other.unk5;
            unk6    = other.unk6;
            return *this;
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

        unkentry(int16_t u0=(ssa_header::LEN / ScriptWordLen), int16_t u1=0xFFFF, int16_t u2=0xFFFF, int16_t u3=0xFFFF)
            :unk0(u0), unk1(u1), unk2(u2), unk3(u3)
        {}

        unkentry(const unkentry & other)
            :unk0(other.unk0), unk1(other.unk1), unk2(other.unk2), unk3(other.unk3)
        {}

        //unkentry(const pmd2::UnkScriptDataEntry & other)
        //    :unk0(other.unk0), unk1(other.unk1), unk2(other.unk2), unk3(other.unk3)
        //{}

        template<class _outit>
            _outit Write(_outit itw)const
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

        //operator pmd2::UnkScriptDataEntry()
        //{
        //    pmd2::UnkScriptDataEntry out;
        //    out.unk0 = unk0;
        //    out.unk1 = unk1;
        //    out.unk2 = unk2;
        //    out.unk3 = unk3;
        //    return std::move(out);
        //}

        //unkentry & operator=(const pmd2::UnkScriptDataEntry & other)
        //{
        //    unk0    = other.unk0;
        //    unk1    = other.unk1;
        //    unk2    = other.unk2;
        //    unk3    = other.unk3;
        //    return *this;
        //}
    };

    const unkentry DefaultUnk3TblEntry;

    /*
        posmarkentry
    */
    struct posmarkentry
    {
        static const size_t LEN = 16; 

        int16_t unk0;
        int16_t unk1;   
        int16_t unk2;   
        int16_t unk3;  
        int16_t unk4;  
        int16_t unk5;  
        int16_t unk6;
        int16_t unk7;

        posmarkentry()
            :unk0(0), unk1(0), unk2(0), unk3(0), unk4(0),
             unk5(0), unk6(0), unk7(0)
        {}

        posmarkentry(const posmarkentry & other)
            :unk0(other.unk0), unk1(other.unk1), unk2(other.unk2), unk3(other.unk3), unk4(other.unk4),
             unk5(other.unk5), unk6(other.unk6), unk7(other.unk7)
        {}

        posmarkentry(const pmd2::PosMarkDataEntry & other)
            :unk0(other.unk0), unk1(other.unk1), unk2(other.unk2), unk3(other.unk3), unk4(other.unk4),
             unk5(other.unk5), unk6(other.unk6), unk7(other.unk7)
        {}

        template<class _outit>
            _outit Write(_outit itw)const
        {
            itw = utils::WriteIntToBytes(unk0,   itw);
            itw = utils::WriteIntToBytes(unk1,   itw);
            itw = utils::WriteIntToBytes(unk2,   itw);
            itw = utils::WriteIntToBytes(unk3,   itw);
            itw = utils::WriteIntToBytes(unk4,   itw);
            itw = utils::WriteIntToBytes(unk5,   itw);
            itw = utils::WriteIntToBytes(unk6,   itw);
            itw = utils::WriteIntToBytes(unk7,   itw);
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
            return itr;
        }

        operator pmd2::PosMarkDataEntry()
        {
            pmd2::PosMarkDataEntry out;
            out.unk0 = unk0;
            out.unk1 = unk1;
            out.unk2 = unk2;
            out.unk3 = unk3;
            out.unk4 = unk4;
            out.unk5 = unk5;
            out.unk6 = unk6;
            out.unk7 = unk7;
            return std::move(out);
        }

        posmarkentry & operator=(const pmd2::PosMarkDataEntry & other)
        {
            unk0    = other.unk0;
            unk1    = other.unk1;
            unk2    = other.unk2;
            unk3    = other.unk3;
            unk4    = other.unk4;
            unk5    = other.unk5;
            unk6    = other.unk6;
            unk7    = other.unk7;
            return *this;
        }
    };

    /*
        unktbl1entry
    */
    struct unktbl1entry
    {
        static const size_t LEN = 8; 
        int16_t unk0;
        int16_t unk1;   
        int16_t unk2;   
        int16_t unk3; 

        unktbl1entry()
            :unk0(0), unk1(0), unk2(0), unk3(0)
        {}

        unktbl1entry(const unktbl1entry & other)
            :unk0(other.unk0), unk1(other.unk1), unk2(other.unk2), unk3(other.unk3)
        {}

        unktbl1entry(const pmd2::UnkTbl1DataEntry & other)
            :unk0(other.unk0), unk1(other.unk1), unk2(other.unk2), unk3(other.unk3)
        {}

        template<class _outit>
            _outit Write(_outit itw)const
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

        operator pmd2::UnkTbl1DataEntry()
        {
            pmd2::UnkTbl1DataEntry out;
            out.unk0 = unk0;
            out.unk1 = unk1;
            out.unk2 = unk2;
            out.unk3 = unk3;
            return std::move(out);
        }

        unktbl1entry & operator=(const pmd2::UnkTbl1DataEntry & other)
        {
            unk0    = other.unk0;
            unk1    = other.unk1;
            unk2    = other.unk2;
            unk3    = other.unk3;
            return *this;
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
            try
            {
                string fext = utils::GetFileExtension(m_origfname);
                if(fext.empty())
                    throw std::runtime_error("SSDataParser::Parse(): File has no file extension!! " + m_origfname);

                m_out.Name(utils::GetBaseNameOnly(m_origfname));
                m_out.Type( pmd2::StrToScriptDataType(fext) );
                ParseHeader();
                ParseUnkTbl1();
                ParseLayers();
                ParsePositionMarks();
            }
            catch(const std::exception&)
            {
                std::throw_with_nested(std::runtime_error("SSDataParser::Parse(): Couldn't parse " + m_origfname));
            }
            return std::move(m_out);
        }

    private:

        inline void ParseHeader()
        {
            m_itcur = m_header.Read(m_itcur, m_itend);
        }

        void ParseUnkTbl1()
        {
            int nbentries = ((m_header.actorsptr - m_header.unkdb1ptr) * ScriptWordLen) / unktbl1entry::LEN;
            assert(nbentries > 0);
            if(nbentries > 0)
            {
                m_out.UnkTbl1().resize(nbentries);
                for( auto & entry : m_out.UnkTbl1() )
                {
                    unktbl1entry unk1ent;
                    m_itcur = unk1ent.Read( m_itcur, m_itend );
                    entry = unk1ent;
                }
            }
            else
                throw std::runtime_error("SSDataParser::ParseUnkTbl1(): No entries in UnkTbl1!!");
        }

        void ParseLayers()
        {
            initer_t itlayersbeg = std::next(m_itbeg, m_header.ptrlayertbl * ScriptWordLen);

            for( size_t i = 0; i < m_header.nblayers; ++i )
            {
                itlayersbeg = ParseALayer(itlayersbeg);
                
                //if(!m_out.Layers().back().unkentries.empty())
                //{
                //    clog << "Layer#" <<i <<", has unk3tbl\n";
                //}
            }
        }

        initer_t ParseALayer( initer_t itlayerbeg )
        {
            LayerEntry        tmplent;
            pmd2::ScriptLayer outlay;
            itlayerbeg = tmplent.Read(itlayerbeg, m_itend);
            ParseEntriesList<livesentry>    ( tmplent.actors(),     std::back_inserter(outlay.lives) );
            ParseEntriesList<objectentry>   ( tmplent.objects(),    std::back_inserter(outlay.objects) );
            ParseEntriesList<performerentry>( tmplent.performers(), std::back_inserter(outlay.performers) );
            ParseEntriesList<evententry>    ( tmplent.events(),     std::back_inserter(outlay.events) );
            //ParseEntriesList<unkentry>      ( tmplent.unk3lst(),    std::back_inserter(outlay.unkentries) ); //The last group is not really a group
            m_out.Layers().push_back(std::move(outlay));
            return itlayerbeg;
        }

        template<class _entryty, typename _backinsit>
            inline void ParseEntriesList( LayerTableEntry & ent, _backinsit itout )
        {
            if(ent.nbentries <= 0) return;
            initer_t itentriesbeg = std::next(m_itbeg, ent.offset * ScriptWordLen);
            for( int16_t i = 0; i < ent.nbentries; ++i )
            {
                _entryty lv;
                itentriesbeg = lv.Read(itentriesbeg, m_itend);
                *itout = lv;
            }
        }

        void ParsePositionMarks()
        {
            int nbentries = ((m_header.unk3ptr - m_header.posmarksptr) * ScriptWordLen) / posmarkentry::LEN;
            if( nbentries <= 0 )
                return;
            initer_t itposmarkbeg = std::next(m_itbeg, (m_header.posmarksptr * ScriptWordLen));
            m_out.PosMarkers().resize(nbentries);
            for( auto & entry : m_out.PosMarkers() )
            {
                posmarkentry pmark;
                itposmarkbeg = pmark.Read(itposmarkbeg, m_itend);
                entry = pmark;
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
    

    class SSDataWriter
    {
        typedef std::ostreambuf_iterator<char> outit_t;
    public:
        SSDataWriter(const pmd2::ScriptData & src)
            :m_src(src)
        {}

        inline void Write(const std::string & outfile)
        {
            try
            {
                _Write(outfile);
            }
            catch(const std::exception &)
            {
                std::throw_with_nested(std::runtime_error("SSDataWriter::Write(): Encountered issue writing " + outfile ));
            }
        }

    private:
        /*
            PrepareForWritingHeader
                Returns an iterator to the header's position + does any required processing.
                Meant to separate file stream ops from anything that's more generic.
        */
        inline outit_t PrepareForWritingHeader()
        {
            m_outf.seekp(0);
            return outit_t(m_outf);
        }

        /*
            PrepareForWritingContent
                Init the target container for writing and returns an iterator to write at!
                Meant to separate file stream ops from anything that's more generic.
        */
        inline outit_t PrepareForWritingContent(const std::string & outfname)
        {
            m_outf.open(outfname, ios::binary | ios::out);
            m_outf.exceptions(ofstream::badbit);
            if( !m_outf )
                throw std::runtime_error("SSDataWriter::Write(): Couldn't create file " + outfname);
            return outit_t(m_outf);
        }

        /*
            GetCurFOff
                Returns the current file offset/write position divided by the length of a 16 bits word.
                Meant to separate file stream ops from anything that's more generic.
                Also validates the current offset to find possible overflows!
        */
        inline uint16_t GetCurFOff() 
        { 
            const auto offset = (m_outf.tellp() / ScriptWordLen);
            if( offset < std::numeric_limits<uint16_t>::max() )
                return static_cast<uint16_t>(offset); 
            else
                throw std::overflow_error("SSDataWriter::GetCurFOff(): Current offset is too large to be stored in a 16 bits word!");
        }

        /*
            ResizeLayerTable
                Validates and resizes the layer list to find possible overflows!
        */
        inline void ResizeLayerTable()
        {
            const size_t sz = m_src.Layers().size();
            if( sz < std::numeric_limits<uint16_t>::max() )
                m_layertbl.resize(sz);
            else
                throw std::overflow_error("SSDataWriter::ResizeLayerTable(): Current layer list size is larger than a 16 bits word!");
        }

        /*
        */
        inline uint16_t GetBlockBeg(LayerEntry::eTableTypes ty)
        {
            switch (ty)
            {
                case LayerEntry::eTableTypes::Actors:
                {
                    return m_hdr.actorsptr;
                }
                case LayerEntry::eTableTypes::Events:
                {
                    return m_hdr.eventsptr;
                }
                case LayerEntry::eTableTypes::Objects:
                {
                    return m_hdr.objectsptr;
                }
                case LayerEntry::eTableTypes::Performers:
                {
                    return m_hdr.performersptr;
                }
                case LayerEntry::eTableTypes::Unk3:
                {
                    return m_hdr.unk3ptr;
                }
                default:
                    break;
            }
            assert(false);
            throw std::runtime_error("SSDataWriter::GetBlockBeg(): Tried to write an unknown layer??? Program logic error.");
        }

        /*
            WriteATable
                Writes for a single layer, the content of one of its lists of assigned entries!

                IntermediateTy == The low-level struct that will handle writing the actual data for the entry
                _TableTy       == The container that contains the processed entries for the entries in the list
        */
        template<class IntermediateTy, class _TableTy>
            void WriteATable(outit_t & itw, LayerEntry::eTableTypes ty, const _TableTy & list, size_t cntlayer )
        {
            //**Note**: If no entries, point to the last word of the last entry/datablock!!!

            //#1 - Write the offset the specified entries for the current layer begins at, and how many there are.
            auto & target = m_layertbl[cntlayer].tables[static_cast<size_t>(ty)];
            target.nbentries = list.size();
            target.offset    = (target.nbentries == 0)? (GetBlockBeg(ty) - 1) : GetCurFOff(); //If empty, set pointer 1 word behind current block beg!

            //#2 - Then append all entries for the current layer to the current file position
            for(const auto & entry : list)
                itw = IntermediateTy(entry).Write(itw); 
        }

        /*
            _Write
                Actual implementation of the Write() method!
        */
        void _Write(const std::string & outfname)
        {
            if(m_src.Layers().empty()) //Should never happen to be honest..
                throw std::runtime_error("SSDataWriter::_Write(): We got a data file with no layers??\n");

            outit_t itw(PrepareForWritingContent(outfname));
            //#1 - Reserve Header + Allocate Layers
            itw = std::fill_n( itw, ssa_header::LEN, 0 );
            ResizeLayerTable();

            //#2 - Write Unk1Table
            WriteUnk1Tbl(itw);

            //#3 - Write the rest
            WriteActors(itw);
            WriteObjects(itw);
            WritePerformers(itw);
            WriteEvents(itw);
            WritePosMarkers(itw);
            WriteUnkTbl3(itw);

            //#4 - Write the layer list
            WriteLayerList(itw);
            
            //#5 -Write the completed header!
            WriteHeader(PrepareForWritingHeader());
        }

       inline void WriteUnk1Tbl(outit_t & itw)
        {
            m_hdr.unkdb1ptr = GetCurFOff(); //Mark the file offset
            for( const auto & entry : m_src.UnkTbl1() )
                itw = unktbl1entry(entry).Write(itw);
        }

        inline void WriteActors(outit_t & itw)
        {
            m_hdr.actorsptr = GetCurFOff(); //Mark the file offset
            assert( !m_src.Layers().empty() );
            for( size_t cntlay = 0; cntlay < m_src.Layers().size(); ++cntlay )
                WriteATable<livesentry>(itw, LayerEntry::eTableTypes::Actors,  m_src.Layers()[cntlay].lives, cntlay);
        }

        inline void WriteObjects(outit_t & itw)
        {
            m_hdr.objectsptr = GetCurFOff(); //Mark the file offset
            for( size_t cntlay = 0; cntlay < m_src.Layers().size(); ++cntlay )
                WriteATable<objectentry>(itw, LayerEntry::eTableTypes::Objects,  m_src.Layers()[cntlay].objects, cntlay);
        }

        inline void WritePerformers(outit_t & itw)
        {
            m_hdr.performersptr = GetCurFOff(); //Mark the file offset
            for( size_t cntlay = 0; cntlay < m_src.Layers().size(); ++cntlay )
                WriteATable<performerentry>(itw, LayerEntry::eTableTypes::Performers,  m_src.Layers()[cntlay].performers, cntlay);
        }

        inline void WriteEvents(outit_t & itw)
        {
            m_hdr.eventsptr = GetCurFOff(); //Mark the file offset
            for( size_t cntlay = 0; cntlay < m_src.Layers().size(); ++cntlay )
                WriteATable<evententry>(itw, LayerEntry::eTableTypes::Events,  m_src.Layers()[cntlay].events, cntlay);
        }

        inline void WritePosMarkers(outit_t & itw)
        {
            m_hdr.posmarksptr = GetCurFOff(); //Mark the file offset
            for( const auto & entry : m_src.PosMarkers() )
                itw = posmarkentry(entry).Write(itw);
        }

        //This is more like a end marker than anything!
        inline void WriteUnkTbl3(outit_t & itw)
        {
            m_hdr.unk3ptr = GetCurFOff(); //Mark the file offset
            
            //Only the first layer contains a single entry
            auto & target = m_layertbl.front().tables[static_cast<size_t>(LayerEntry::eTableTypes::Unk3)];
            target.nbentries = 1;
            target.offset    = m_hdr.unk3ptr;
            itw = DefaultUnk3TblEntry.Write(itw); //Its always the same value.

            //Every single other layers has 0 entry for that table!
            for( size_t cntlay = 1; cntlay < m_src.Layers().size(); ++cntlay )
            {
                auto & current = m_layertbl[cntlay].tables[static_cast<size_t>(LayerEntry::eTableTypes::Unk3)];
                current.nbentries = 0;
                current.offset    = (m_hdr.unk3ptr - 1);
            }
        }

        inline void WriteHeader(outit_t & itw)
        {
            itw = m_hdr.Write(itw);
        }

        inline void WriteLayerList(outit_t & itw)
        {
            m_hdr.nblayers    = static_cast<uint16_t>(m_layertbl.size());
            m_hdr.ptrlayertbl = GetCurFOff();

            //Write list
            for( const auto & entry : m_layertbl )
                itw = entry.Write(itw);
        }

    private:
        const pmd2::ScriptData & m_src;
        std::ofstream            m_outf;
        ssa_header               m_hdr;
        std::vector<LayerEntry>  m_layertbl;
    };


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
        SSDataWriter(scrdat).Write(fpath);
    }
};