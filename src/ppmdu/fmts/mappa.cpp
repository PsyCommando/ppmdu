#include "mappa.hpp"
#include <ppmdu/containers/dungeon_rng_data.hpp>
#include <utils/poco_wrapper.hpp>
#include <utils/utility.hpp>
#include <algorithm>
#include <type_traits>
using namespace std;
using namespace pmd2;
//#pragma optimize( "", off )
namespace filetypes
{
//==========================================================================================
// Constants
//==========================================================================================
    const uint32_t MappaPaddingMarker  = 0xAAAAAAAA; //Padding bytes used in the mappa files
	//const size_t   NB_TRAPS_TYPES_EOS  = 25;			//There's 25 different types of traps in explorers of sky!
	//const size_t   NB_TRAPS_TYPES_EOTD = 23;			//There's 23 different types of traps in explorers of time/darkness

//==========================================================================================
// Utilities
//==========================================================================================
    /*
        ParseLUTAndData
            - lutbeg    : Offset of the beginning of the look up table
            - lutend    : Offset of one past the end of the look up table
            - cntainer  : Container where to put the data. The "value_type" must implement "template<class _init> _init ReadFromContainer(_init, _init)".
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
            itlutbeg = utils::ReadIntFromBytes( offsetentry, itlutbeg, itlutend);

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
        floor_data_entries
    ***********************************************************************/

	/*
		Parser for the block of floor data entries.
	*/
	struct floor_data_entries
	{
		std::vector<pmd2::stats::floor_data_entry> floor_entries;

        //Write the structure using an iterator to a byte container
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            //Then write the structs
            for( const auto & afloor : floor_entries )
                itwriteto = afloor.WriteToContainer(itwriteto);
            return itwriteto;
        }

        /*
            Read the structure from an iterator on a byte container
        */
        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itbegdungeonLUT )
        {
            //Skip the leading null entry
            std::advance( itReadfrom, floor_data_entry::SIZE );

            //Parse the entries
            while( itReadfrom != itbegdungeonLUT )
            {
                auto             itsav = itReadfrom;
                floor_data_entry info;
                itReadfrom = info.ReadFromContainer(itReadfrom, itbegdungeonLUT);
                floor_entries.push_back( std::move(info) );
            }

            return itReadfrom;
        }
	};





//==========================================================================================
// MappaParser
//==========================================================================================

    /***********************************************************************
        MappaParser
            Parses a mappa_*.bin file.
    ***********************************************************************/
    template<typename _init, typename _gameVer_t>
        class MappaParser
    {
    public:
        typedef _init initer_t;
		static const eGameVersion GAME_VERSION = _gameVer_t::value;
        typedef pmd2::stats::trap_floor_spawn_entry<typename _gameVer_t> trap_list_t;

        MappaParser(_init itbeg, _init itend )
            :m_itbegin(itbeg), m_itend(itend)
        {}

        //mappaid : Characte code identifying the mappa_#.bin file. usually s, t, y
        pmd2::stats::DungeonRNGDataSet & Parse(pmd2::stats::eMappaID mappaid, pmd2::stats::DungeonRNGDataSet & dataset)
        {
            m_tgt = &dataset;
            m_tgt->OriginalMappaID(mappaid);
            ParseHeader();
            ParseSubHdr();
            FillTables();   //Fill all our tables, to make lookup much faster!
            ProcessData();  //Move the data to the storage object, and re-organize
            return *m_tgt;
        }

    private:

        void ParseHeader()
        {
            m_sir0hdr.ReadFromContainer(m_itbegin, m_itend);
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
            ParseLUTAndData( m_subhdr.ptrDungeonLUT, m_subhdr.ptrFloorDataBlock, m_dungeonstbl, m_itbegin );

            //Fill item spawn lists table
            uint32_t begitem  = ParseLUTAndData( m_subhdr.ptrItemSpawnListLUT, m_sir0hdr.subheaderptr, m_itemslststbl, m_itbegin );

            //Fill Traps LUT
            uint32_t begtrapdata = ParseLUTAndData( m_subhdr.ptrTrapsLUT, begitem, m_trapslsts, m_itbegin );
            
            //Fill pkmn spawn lists table
            uint32_t begpkspwndat = ParseLUTAndData( m_subhdr.ptrItemSpawnListLUT, begtrapdata, m_pkmnspwnlststbl, m_itbegin );

            auto itfloordataend = m_itbegin + begpkspwndat;
            //Fill floor data table
            {
                const size_t nbfloors = (begpkspwndat - m_subhdr.ptrFloorDataBlock) / pmd2::stats::floor_data_entry::SIZE;
                auto         itfloor  = m_itbegin + m_subhdr.ptrFloorDataBlock;
                m_floordatatbl.reserve(nbfloors);

                for( size_t cntfloor = 0; cntfloor < nbfloors && itfloor != itfloordataend; ++cntfloor )
                {
                    pmd2::stats::floor_data_entry curfloor;
                    itfloor = curfloor.ReadFromContainer( itfloor, itfloordataend );
                    m_floordatatbl.push_back(curfloor);
                }
            }
        }

        //We'll try to make sense of the data and regroup everything per dungeon.
        void ProcessData()
        {
            pmd2::stats::DungeonRNGDataSet & dest = *m_tgt;

            //Move all the crap directly
            dest.m_dungeonstbl      = std::move(m_dungeonstbl);
            dest.m_floordatatbl     = std::move(m_floordatatbl);
            dest.m_pkmnspwnlststbl  = std::move(m_pkmnspwnlststbl);
            dest.m_itemslststbl     = std::move(m_itemslststbl);

            //Convert traps list to vector for editing in the container
            for (const trap_list_t & entry : m_trapslsts)
                dest.m_trapslsts.push_back(std::vector<uint16_t>(entry.traps_probs.begin(), entry.traps_probs.end()));
        }


    private:
        sir0_header						m_sir0hdr;
        mappa_subhdr					m_subhdr;

        initer_t						m_itbegin;
        initer_t						m_itend;

        pmd2::stats::DungeonRNGDataSet *				m_tgt;			        //Destination storage class for the parsed data!

        vector<pmd2::stats::dungeon_entry>			m_dungeonstbl;			//All dungeons data, containing the details for their floors
        vector<pmd2::stats::floor_data_entry>	    m_floordatatbl;			//All floor data for all floors
        vector<pmd2::stats::pkmn_floor_spawn_lst>	m_pkmnspwnlststbl;		//All pokemon spawn lists for all floors
        vector<pmd2::stats::dungeon_items_lst>		m_itemslststbl;		    //All items lists for all floors
        vector<trap_list_t>	            m_trapslsts;			//All trap spawn lists for all floors
    };

//==========================================================================================
// MappaGParser
//==========================================================================================

    /***********************************************************************
        MappaGParser
            Parses a mappa_g*.bin file.
    ***********************************************************************/
    template<typename _init, typename _gameVer_t>
        class MappaGParser
    {
    public:
        typedef _init initer_t;
        static const eGameVersion GAME_VERSION = _gameVer_t::value;

        MappaGParser(initer_t itbeg, initer_t itend)
            :m_itbeg(itbeg), m_itend(itend)
        {}

        pmd2::stats::DungeonRNGDataSet & Parse(pmd2::stats::eMappaID mappagid, pmd2::stats::DungeonRNGDataSet & dataset)
        {
            m_ptrcurdataset = &dataset;
            m_ptrcurdataset->OriginalMappaID(mappagid);

            ParseHeaders();
            ParseData();
            return *m_ptrcurdataset;
        }

    private:
        void ParseHeaders()
        {
            m_sir0hdr.ReadFromContainer(m_itbeg, m_itend);
            initer_t itsubhdr = m_itbeg;
            std::advance(itsubhdr, m_sir0hdr.subheaderptr);
            m_hdr.ReadFromContainer(itsubhdr, m_itend);
        }

        void ParseData()
        {
            //Read the lookup table, and let the dungeon_override object fill itself from the raw data at the lut offsets
            ParseLUTAndData(m_hdr.ptrdungeonslut, m_hdr.ptrfloordata, m_dungeonsFlIdx, m_itbeg);

            initer_t itfloorDat = m_itbeg;
            initer_t itfloorDatEnd = m_itbeg; //Does not accounts for the padding at the end of the list before the sub-header!! So you'll likely hit padding bytes before hitting the end iterator!!
            std::advance(itfloorDat, m_hdr.ptrfloordata);
            std::advance(itfloorDat, m_sir0hdr.subheaderptr);

            uint32_t paddingcheck = 0;
            while (itfloorDat != itfloorDat && paddingcheck != MappaPaddingMarker)
            {
                //Peek at the current bytes to be sure we didn't hit the padding, without incrementing the iterators!
                std::array<uint8_t, pmd2::stats::dungeon_floor_override::SIZE> buffer;
                std::copy_n(itfloorDat, pmd2::stats::dungeon_floor_override::SIZE, buffer.begin());
                std::advance(itfloorDat, pmd2::stats::dungeon_floor_override::SIZE);
                utils::ReadIntFromBytes(paddingcheck, buffer.begin(), buffer.end());

                //#NOTE: I'm buffering ahead here since I don't want to assume we have a random access iterator.. 
                //       So if its a one way only iterator, that you can't use to keep positions, it won't break anything.

                if (paddingcheck != MappaPaddingMarker)
                {
                    //Grab the entry if its not padding!
                    pmd2::stats::dungeon_floor_override ovr;
                    ovr.ReadFromContainer(buffer.begin(), buffer.end());
                    m_dungeonFloorData.push_back(ovr);
                }
            }
        }

    private:
        initer_t m_itbeg;   //Beginning of the data to parse from
        initer_t m_itend;   //End of the data to parse from

        pmd2::stats::DungeonRNGDataSet *                 m_ptrcurdataset;        //Target dataset to place the override data into
        std::vector<pmd2::stats::dungeon_override>       m_dungeonsFlIdx;        //List of structs containing the indices in the m_dungeonFloorData for each of their floors
        std::vector<pmd2::stats::dungeon_floor_override> m_dungeonFloorData;     //List of override data for every single dungeon floors referred to in the indice lists

        sir0_header         m_sir0hdr;
        mappa_g_subhdr      m_hdr;
    };

//==========================================================================================
// MappaWriter
//==========================================================================================
    /***********************************************************************
        MappaWriter
            Writes dungeon dataset to a buffer
    ***********************************************************************/
    template<typename _gameVer_t>
        class MappaWriter
    {
    public:
        static const eGameVersion GAME_VERSION = _gameVer_t::value;

        MappaWriter(const pmd2::stats::DungeonRNGDataSet & src)
            :m_src(src)
        {}

        template<class _outit>
            _outit Write(_outit itwrite)
        {
            WriteDungeonData();
            WriteFloorData();
            WritePkmnSpawns();
            WriteTrapsSpawns();
            WriteItemsLists();
            m_wbuffer.Write(itwrite);
            return itwrite;
        }

    private:
        void WriteDungeonData()
        {
            std::vector<uint32_t> offsets;

            //write data + save offset to lut
            for(const auto & dungeon : m_src.)

            //write LUT
            m_hdr.ptrDungeonLUT = m_wbuffer.Data().size();
            WriteLUT(offsets);

        }

        void WriteFloorData()
        {
            std::vector<uint32_t> offsets;
            //write floor data + save offsets to lut
            //padding
            //write lut
            //padding
        }

        void WritePkmnSpawns()
        {
            std::vector<uint32_t> offsets;
            //write pkmn data + save offsets to lut
            //padding
            //write lut
            //padding
        }


        void WriteTrapsSpawns()
        {
            std::vector<uint32_t> offsets;
            //write trap data + save offsets to lut
            //padding
            //write lut
            //padding
        }

        void WriteItemsLists()
        {
            std::vector<uint32_t> offsets;
            //write item spawns
            //write shop items
            //write monster house items
            //write buried items
            //write unk2 items
            //write unk3 items

            //padding
            //write lut
            //padding
        }

        inline void WriteLUT(const std::vector<uint32_t> & lut)
        {
            for (uint32_t p : lut)
                m_wbuffer.pushpointer(p);
        }

    private:
        typedef std::vector<uint8_t> data_t;
        const pmd2::stats::DungeonRNGDataSet &       m_src;
        FixedSIR0DataWrapper<data_t>    m_wbuffer;
        mappa_subhdr                    m_hdr;
    };


//==========================================================================================
// MappaGWriter
//==========================================================================================
    class MappaGWriter
    {

    };

//==========================================================================================
// Functions
//==========================================================================================

    pmd2::stats::DungeonRNGDataSet LoadMappaSetEoS(const std::string & fnamemappa, const std::string & fnamemappag)
    {
        using namespace pmd2::stats;
        DungeonRNGDataSet dataset;
        //Handle Mappa
        {
            std::string fnameonly = utils::GetFilename(fnamemappa);
            std::string::iterator itunderscore = std::find(fnameonly.begin(), fnameonly.end(), '_');
            ++itunderscore; //Get the letter after the '_'
            eMappaID mappaid = static_cast<eMappaID>(*itunderscore);

            std::vector<uint8_t> mappafiledata = utils::io::ReadFileToByteVector(fnamemappa);
            MappaParser<std::vector<uint8_t>::const_iterator, std::integral_constant<pmd2::eGameVersion, pmd2::eGameVersion::EoS>>(mappafiledata.begin(), mappafiledata.end()).Parse(mappaid, dataset);
        }

        //Handle MappaG
        {
            std::string fnameonly = utils::GetFilename(fnamemappag);
            std::string::iterator itg = std::find(fnameonly.begin(), fnameonly.end(), 'g');
            ++itg; //Get the letter after the 'g'
            eMappaID mappagid = static_cast<eMappaID>(*itg);
            std::vector<uint8_t> mappagfiledata = utils::io::ReadFileToByteVector(fnamemappag);
            MappaGParser<std::vector<uint8_t>::const_iterator, std::integral_constant<pmd2::eGameVersion, pmd2::eGameVersion::EoS>>(mappagfiledata.begin(), mappagfiledata.end()).Parse(mappagid, dataset);
        }
        return dataset;
    }

    pmd2::stats::DungeonRNGDataSet LoadMappaSetEoTD(const std::string & fnamemappa)
    {
        using namespace pmd2::stats;
        DungeonRNGDataSet dataset;
        std::string fnameonly = utils::GetFilename(fnamemappa);
        std::string::iterator itunderscore = std::find(fnameonly.begin(), fnameonly.end(), '_');
        ++itunderscore; //Get the letter after the '_'
        eMappaID mappaid = static_cast<eMappaID>(*itunderscore);

        std::vector<uint8_t> mappafiledata = utils::io::ReadFileToByteVector(fnamemappa);
        MappaParser<std::vector<uint8_t>::const_iterator, std::integral_constant<pmd2::eGameVersion, pmd2::eGameVersion::EoT>>(mappafiledata.begin(), mappafiledata.end()).Parse(mappaid, dataset);
        return dataset;
    }

    void WriteMappaSetEoS(const std::string & fnamemappa, const std::string & fnamemappag, const pmd2::stats::DungeonRNGDataSet & mappaset)
    {
    }

    void WriteMappaSetEoTD(const std::string & fnamemappa, const pmd2::stats::DungeonRNGDataSet & mappaset)
    {
    }

};
//#pragma optimize( "", on )