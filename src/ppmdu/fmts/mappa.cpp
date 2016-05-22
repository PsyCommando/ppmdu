#include "mappa.hpp"
#include <utils/poco_wrapper.hpp>
#include <utils/utility.hpp>
using namespace std;
using namespace pmd2;
#pragma optimize( "", off )
namespace filetypes
{
//==========================================================================================
// Constants
//==========================================================================================
    const uint32_t MappaPaddingMarker = 0xAAAAAAAA;

//==========================================================================================
// Utilities
//==========================================================================================
    /*
        ParseLUTAndData
            - lutbeg    : Offset of the beginning of the look up table
            - lutend    : Offset of one past the end of the look up table
            - cntainer  : Container where to put the data. The "value_type" must implement "ReadFromContainer".
            - itbeg     : Beginning of the container the offsets are relative to!
            Returns beginning of the data chunk for the current lut, 
            so the end of the previous lut can be figured out from that offset.
    */
    template<typename _tgtcontainer, typename _init>
        uint32_t ParseLUTAndData( uint32_t lutbeg, uint32_t lutend, _tgtcontainer & cntainer, _init itbeg )
    {
        uint32_t     begdata   = 0;   //The lowest pointer of the lookup table
        const size_t nblutptrs = (lutend - lutbeg) / sizeof(uint32_t);
        auto         itlutbeg  = itbeg + lutbeg;
        auto         itlutend  = itbeg + lutend;
        cntainer.reserve(nblutptrs);

        for( size_t cntlut = 0; cntlut < nblutptrs && itlutbeg != itlutend; ++cntlut )
        {
            uint32_t offsetentry = 0;
            itlutbeg = utils::ReadIntFromBytes( offsetentry, itlutbeg );

            if( offsetentry != MappaPaddingMarker )
            {
                if( offsetentry < begdata )
                    begdata = offsetentry;

                typename _tgtcontainer::value_type dat;
                auto itdat = itbeg + offsetentry;
                itdat = dat.ReadFromContainer( itdat, itlutend );
                cntainer.push_back( std::move(dat) );
            }
            else
                break;
        }

        return begdata;
    }


//==========================================================================================
// Data Structures
//==========================================================================================

    /***********************************************************************
        mappa_subhdr
            This is what the first pointer in the SIR0 header points to.
            5, 32 bits pointer to various places in the data.
    ***********************************************************************/
    struct mappa_subhdr
    {
        static const size_t SIZE = 20;  //bytes
        uint32_t ptrDungeonLUT       = 0;
        uint32_t ptrFloorDataBlock   = 0;
        uint32_t ptrItemSpawnListLUT = 0;
        uint32_t ptrPkmnSpwnLUT      = 0;
        uint32_t ptrLUTC             = 0;


        //Write the structure using an iterator to a byte container
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( ptrDungeonLUT,          itwriteto );
            itwriteto = utils::WriteIntToBytes( ptrFloorDataBlock,      itwriteto );
            itwriteto = utils::WriteIntToBytes( ptrItemSpawnListLUT,    itwriteto );
            itwriteto = utils::WriteIntToBytes( ptrPkmnSpwnLUT,         itwriteto );
            itwriteto = utils::WriteIntToBytes( ptrLUTC,                itwriteto );
            return itwriteto;
        }

        //Read the structure from an iterator on a byte container
        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itend )
        {
            itReadfrom = utils::ReadIntFromBytes( ptrDungeonLUT,          itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( ptrFloorDataBlock,      itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( ptrItemSpawnListLUT,    itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( ptrPkmnSpwnLUT,         itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( ptrLUTC,                itReadfrom, itend );
            return itReadfrom;
        }
    };





    /***********************************************************************
        raw_dungeon_floor_info
            18 bytes entry that contains indices to the data lists that 
            applies to this particular floor!
    ***********************************************************************/
    struct raw_dungeon_floor_info
    {
        static const size_t SIZE = 18;
        uint16_t idfloordata      = 0;
        uint16_t idpkspwn         = 0;
        uint16_t idlutc           = 0;
        uint16_t iditemspwn       = 0;
        uint16_t idshopdat        = 0;
        uint16_t idmhouseitem     = 0;
        uint16_t idburieditemsdat = 0;
        uint16_t unk2             = 0;
        uint16_t unk3             = 0;

        bool isnull()const 
        {
            return idfloordata == 0 && idpkspwn     == 0 && idlutc           == 0 && iditemspwn == 0 &&
                   idshopdat   == 0 && idmhouseitem == 0 && idburieditemsdat == 0 && unk2       == 0 &&
                   unk3        == 0;
        }

        //Write the structure using an iterator to a byte container
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( idfloordata,      itwriteto );
            itwriteto = utils::WriteIntToBytes( idpkspwn,         itwriteto );
            itwriteto = utils::WriteIntToBytes( idlutc,           itwriteto );
            itwriteto = utils::WriteIntToBytes( iditemspwn,       itwriteto );
            itwriteto = utils::WriteIntToBytes( idshopdat,        itwriteto );
            itwriteto = utils::WriteIntToBytes( idmhouseitem,     itwriteto );
            itwriteto = utils::WriteIntToBytes( idburieditemsdat, itwriteto );
            itwriteto = utils::WriteIntToBytes( unk2,             itwriteto );
            itwriteto = utils::WriteIntToBytes( unk3,             itwriteto );
            return itwriteto;
        }

        //Read the structure from an iterator on a byte container
        template<class _init>
            _init ReadFromContainer( _init itreadfrom, _init itend )
        {
            itreadfrom = utils::ReadIntFromBytes( idfloordata,      itreadfrom, itend );
            itreadfrom = utils::ReadIntFromBytes( idpkspwn,         itreadfrom, itend );
            itreadfrom = utils::ReadIntFromBytes( idlutc,           itreadfrom, itend );
            itreadfrom = utils::ReadIntFromBytes( iditemspwn,       itreadfrom, itend );
            itreadfrom = utils::ReadIntFromBytes( idshopdat,        itreadfrom, itend );
            itreadfrom = utils::ReadIntFromBytes( idmhouseitem,     itreadfrom, itend );
            itreadfrom = utils::ReadIntFromBytes( idburieditemsdat, itreadfrom, itend );
            itreadfrom = utils::ReadIntFromBytes( unk2,             itreadfrom, itend );
            itreadfrom = utils::ReadIntFromBytes( unk3,             itreadfrom, itend );
            return itreadfrom;
        }
    };

    /***********************************************************************  
        raw_dungeon_entry
            Contains several 18 bytes entries containing indices to the 
            data lists that applies to this dungeon floor.
    ***********************************************************************/
    struct raw_dungeon_entry
    {
        std::vector<raw_dungeon_floor_info> floors;

        //Write the structure using an iterator to a byte container
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            //first write the leading dummy null entry
            itwriteto = std::fill_n( itwriteto, raw_dungeon_floor_info::SIZE, 0 );

            //Then write the structs
            for( const auto & afloor : floors )
                itwriteto = afloor.WriteToContainer(itwriteto);

            return itwriteto;
        }

        /*
            Read the structure from an iterator on a byte container
            - itReadfrom        : Iterator from the first dungeon entry.
            - itbegdungeonLUT   : Iterator on the beginning of the dungeon look up table, which is right after the last dungeon entry.
                                  its needed to stop properly when parsing the last entry, since there is no null entry to mark the start
                                  of the next entry afterward.
        */
        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itbegdungeonLUT )
        {
            //Skip the leading null entry
            std::advance( itReadfrom, raw_dungeon_floor_info::SIZE );

            //Parse the entries
            while( itReadfrom != itbegdungeonLUT )
            {
                auto             itsav = itReadfrom;
                raw_dungeon_floor_info info;
                itReadfrom = info.ReadFromContainer(itReadfrom, itbegdungeonLUT);

                if( info.isnull() )
                {
                    itReadfrom = itsav;
                    break;
                }
                else
                    floors.push_back( std::move(info) );
            }

            return itReadfrom;
        }
    };

    /***********************************************************************
        pkmn_spawn_lst
    ***********************************************************************/
    struct pkmn_spawn_lst
    {
    };

    /***********************************************************************
        item_spawn_lst
    ***********************************************************************/
    struct item_spawn_lst
    {
    };

    /***********************************************************************
        lutc_data
    ***********************************************************************/
    struct lutc_data
    {
    };

//==========================================================================================
// MappaParser
//==========================================================================================

    /***********************************************************************
        MappaParser
            Parses a mappa_*.bin file.
    ***********************************************************************/
    template<typename _init>
        class MappaParser
    {
    public:
        typedef _init initer_t;

        MappaParser( const string & fpath, _init itbeg, _init itend )
            :m_fpath(fpath), m_itbegin(itbeg), m_itcur(itbeg), m_itend(itend)
        {}

        DungeonRNGDataSet Parse()
        {
            //!TODO: Fill up the original names of the 2 mappa files?
            m_tgt = std::move( DungeonRNGDataSet() );
            ParseHeader();
            ParseSubHdr();
            FillTables();    //Fill all our tables, to make lookup much faster!
            ParseData();
            return std::move(m_tgt);
        }

    private:

        void ParseHeader()
        {
            m_itcur = m_sir0hdr.ReadFromContainer(m_itcur);
        }

        void ParseSubHdr()
        {
            auto itsubhdr = m_itbegin;
            std::advance( itsubhdr, m_sir0hdr.subheaderptr );
            m_subhdr.ReadFromContainer(itsubhdr, m_itend);
        }

        void FillTables()
        {
            //Fill dungeon table
            //{
                ParseLUTAndData( m_subhdr.ptrDungeonLUT, m_subhdr.ptrFloorDataBlock, m_dungeonstbl, m_itbegin );

            //    const size_t nbdungeonptrs = (m_subhdr.ptrFloorDataBlock - m_subhdr.ptrDungeonLUT) / sizeof(uint32_t);
            //    auto         itDungLutBeg  = m_itbegin;
            //    std::advance( itDungLutBeg, m_subhdr.ptrDungeonLUT );
            //    m_itcur = itDungLutBeg;

            //    for( size_t cntdun = 0; cntdun < nbdungeonptrs; ++cntdun )
            //    {
            //        uint32_t offsetentry = 0;
            //        m_itcur = utils::ReadIntFromBytes( offsetentry, m_itcur );
            //        raw_dungeon_entry dung;
            //        if( offsetentry != 0 )
            //        {
            //            auto itdunentry = m_itbegin;
            //            std::advance( itdunentry, offsetentry );
            //            itdunentry = dung.ReadFromContainer( itdunentry, itDungLutBeg );
            //        }
            //        else
            //        {
            //            //IDK what we should do here tbh..
            //            clog << "Got null pointer in dungeon LUT!\n";
            //        }
            //        m_dungeonstbl.push_back( std::move(dung) );
            //    }
            //}

            //Fill item spawn lists table
            uint32_t begitem  = ParseLUTAndData( m_subhdr.ptrItemSpawnListLUT, m_sir0hdr.subheaderptr, m_itemspwnlststbl, m_itbegin );

            //Fill C LUT
            uint32_t begcdata = ParseLUTAndData( m_subhdr.ptrLUTC, begitem, m_lutcdatatbl, m_itbegin );
            
            //Fill pkmn spawn lists table
            uint32_t begpkspwndat = ParseLUTAndData( m_subhdr.ptrItemSpawnListLUT, begcdata, m_pkmnspwnlststbl, m_itbegin );

            auto itfloordataend = m_itbegin + begpkspwndat;
            //Fill floor data table
            {
                const size_t nbfloors = (begpkspwndat - m_subhdr.ptrFloorDataBlock) / DungeonFloorDataEntry::SIZE;
                auto         itfloor  = m_itbegin + m_subhdr.ptrFloorDataBlock;
                m_floordatatbl.reserve(nbfloors);

                for( size_t cntfloor = 0; cntfloor < nbfloors && itfloor != itfloordataend; ++cntfloor )
                {
                    DungeonFloorDataEntry curfloor;
                    itfloor = curfloor.ReadFromContainer( itfloor, itfloordataend );
                    m_floordatatbl.push_back(curfloor);
                }
            }
        }

        //We'll try to make sense of the data and regroup everything per dungeon.
        void ParseData()
        {

        }


    private:
        string                         m_fpath;
        sir0_header                    m_sir0hdr;
        mappa_subhdr                   m_subhdr;

        initer_t                       m_itbegin;
        initer_t                       m_itcur;
        initer_t                       m_itend;

        DungeonRNGDataSet              m_tgt;

        vector<raw_dungeon_entry>      m_dungeonstbl;
        vector<DungeonFloorDataEntry>  m_floordatatbl;
        vector<pkmn_spawn_lst>         m_pkmnspwnlststbl;
        vector<item_spawn_lst>         m_itemspwnlststbl;
        vector<lutc_data>              m_lutcdatatbl;
    };


//==========================================================================================
// MappaGParser
//==========================================================================================

    /***********************************************************************
        MappaGParser
            Parses a mappa_g*.bin file.
    ***********************************************************************/
    class MappaGParser
    {
    public:


    private:

    };


//==========================================================================================
// Functions
//==========================================================================================
    DungeonRNGDataSet LoadMappaSet(const std::string & fnamemappa, const std::string & fnamemappag)
    {
        return DungeonRNGDataSet();
    }

    void WriteMappaSet(const std::string & fnamemappa, const std::string & fnamemappag, const DungeonRNGDataSet & mappaset)
    {
    }



};
#pragma optimize( "", on )