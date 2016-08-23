#ifndef DUNGEON_RNG_DATA_HPP
#define DUNGEON_RNG_DATA_HPP
/*
dungeon_rng_data.hpp
2016/05/19
psycommando@gmail.com
Description: Utilities for handling and manipulating dungeon data used for generating dungeons floors in PMD2.
*/
#include <ppmdu/pmd2/pmd2.hpp>
#include <vector>
#include <deque>
#include <string>

namespace pmd2
{

//========================================================================================
//  ItemSpawnEntry
//========================================================================================
    class ItemSpawnList
    {
    public:

    private:
    };

//========================================================================================
//  PokemonSpawnEntry
//========================================================================================
    class PokemonSpawnList
    {
    public:

    private:
    };

//========================================================================================
//  DungeonFloorDataEntry
//========================================================================================
    /*
        DungeonFloorDataEntry
            32 bytes entry containing the data for a floor.
    */
    struct DungeonFloorDataEntry
    {
        static const size_t SIZE = 32;
        uint8_t  floorshape         = 0;
        uint8_t  unk4               = 0;
        uint8_t  tilesetid          = 0;
        uint8_t  musicid            = 0;
        uint8_t  defaultweather     = 0;
        uint8_t  unk5               = 0;
        uint8_t  pkmndensity        = 0;
        uint8_t  shopratio          = 0;
        uint8_t  mhouseratio        = 0;
        uint8_t  unk6               = 0;
        uint8_t  unk7               = 0;
        uint8_t  unk8               = 0;
        uint8_t  unk9               = 0;
        uint8_t  unk10              = 0;
        uint8_t  unk11              = 0;
        uint8_t  itemdensity        = 0;
        uint8_t  trapdensity        = 0;
        uint8_t  floorcnt           = 0;
        uint8_t  unk13              = 0;
        uint8_t  unk12              = 0;
        uint8_t  burieditemdensity  = 0;
        uint8_t  waterratio         = 0;
        uint8_t  darklvl            = 0;
        uint8_t  maxcoin            = 0;
        uint8_t  unk14              = 0;
        uint8_t  unk15              = 0;
        uint8_t  unk16              = 0;
        uint8_t  unk17              = 0;
        uint16_t enemyiq            = 0;
        uint8_t  unk18              = 0;
        uint8_t  unk19              = 0;


        //Write the structure using an iterator to a byte container
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( floorshape,        itwriteto );
            itwriteto = utils::WriteIntToBytes( unk4,              itwriteto );
            itwriteto = utils::WriteIntToBytes( tilesetid,         itwriteto );
            itwriteto = utils::WriteIntToBytes( musicid,           itwriteto );
            itwriteto = utils::WriteIntToBytes( defaultweather,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk5,              itwriteto );
            itwriteto = utils::WriteIntToBytes( pkmndensity,       itwriteto );
            itwriteto = utils::WriteIntToBytes( shopratio,         itwriteto );
            itwriteto = utils::WriteIntToBytes( mhouseratio,       itwriteto );
            itwriteto = utils::WriteIntToBytes( unk6,              itwriteto );
            itwriteto = utils::WriteIntToBytes( unk7,              itwriteto );
            itwriteto = utils::WriteIntToBytes( unk8,              itwriteto );
            itwriteto = utils::WriteIntToBytes( unk9,              itwriteto );
            itwriteto = utils::WriteIntToBytes( unk10,             itwriteto );
            itwriteto = utils::WriteIntToBytes( unk11,             itwriteto );
            itwriteto = utils::WriteIntToBytes( itemdensity,       itwriteto );
            itwriteto = utils::WriteIntToBytes( trapdensity,       itwriteto );
            itwriteto = utils::WriteIntToBytes( floorcnt,          itwriteto );
            itwriteto = utils::WriteIntToBytes( unk13,             itwriteto );
            itwriteto = utils::WriteIntToBytes( unk12,             itwriteto );
            itwriteto = utils::WriteIntToBytes( burieditemdensity, itwriteto );
            itwriteto = utils::WriteIntToBytes( waterratio,        itwriteto );
            itwriteto = utils::WriteIntToBytes( darklvl,           itwriteto );
            itwriteto = utils::WriteIntToBytes( maxcoin,           itwriteto );
            itwriteto = utils::WriteIntToBytes( unk14,             itwriteto );
            itwriteto = utils::WriteIntToBytes( unk15,             itwriteto );
            itwriteto = utils::WriteIntToBytes( unk16,             itwriteto );
            itwriteto = utils::WriteIntToBytes( unk17,             itwriteto );
            itwriteto = utils::WriteIntToBytes( enemyiq,           itwriteto );
            itwriteto = utils::WriteIntToBytes( unk18,             itwriteto );
            itwriteto = utils::WriteIntToBytes( unk19,             itwriteto );
            return itwriteto;
        }

        //Read the structure from an iterator on a byte container
        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itend )
        {
            itReadfrom = utils::ReadIntFromBytes( floorshape,        itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk4,              itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( tilesetid,         itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( musicid,           itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( defaultweather,    itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk5,              itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( pkmndensity,       itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( shopratio,         itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( mhouseratio,       itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk6,              itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk7,              itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk8,              itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk9,              itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk10,             itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk11,             itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( itemdensity,       itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( trapdensity,       itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( floorcnt,          itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk13,             itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk12,             itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( burieditemdensity, itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( waterratio,        itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( darklvl,           itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( maxcoin,           itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk14,             itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk15,             itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk16,             itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk17,             itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( enemyiq,           itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk18,             itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk19,             itReadfrom, itend );
            return itReadfrom;
        }
    };

    /*
    */
    class DungeonFloor
    {
    public:

    private:
    };

//========================================================================================
//  DungeonDataEntry
//========================================================================================
    class DungeonDataEntry
    {
    public:
        typedef std::vector<DungeonFloor>      floors_t;
        typedef std::deque<PokemonSpawnList>   pkmnspwnlst_t;
        typedef std::deque<ItemSpawnList>      itemspwnlst_t;
        typedef floors_t::iterator             iterator;
        typedef floors_t::const_iterator       const_iterator;

        size_t          size()const     { return m_floors.size();}
        bool            empty()const    { return m_floors.empty(); }
        iterator        begin()         { return m_floors.begin(); }
        const_iterator  begin()const    { return m_floors.begin(); }
        iterator        end()           { return m_floors.end(); }
        const_iterator  end()const      { return m_floors.end(); }

        floors_t        m_floors;
    };

//========================================================================================
//  DungeonRNGDataSet
//========================================================================================
    class DungeonRNGDataSet
    {
    public:
        typedef std::vector<DungeonDataEntry>       dungeonentries_t;
        typedef dungeonentries_t::iterator          iterator;
        typedef dungeonentries_t::const_iterator    const_iterator;

        DungeonRNGDataSet()
        {}

        /*
            DungeonRNGDataSet
                -fnamemappa  : name of the original mappa_*.bin file
                -fnamemappag : name of the original mappa_g*.bin file
        */
        DungeonRNGDataSet( const std::string & fnamemappa, const std::string & fnamemappag )
            :m_origmappafn(fnamemappa),m_origmappagfn(fnamemappag)
        {}

        size_t          size()const     { return m_dungeondata.size();}
        bool            empty()const    { return m_dungeondata.empty(); }
        iterator        begin()         { return m_dungeondata.begin(); }
        const_iterator  begin()const    { return m_dungeondata.begin(); }
        iterator        end()           { return m_dungeondata.end(); }
        const_iterator  end()const      { return m_dungeondata.end(); }

        dungeonentries_t        & DungeonData()         { return m_dungeondata; }
        const dungeonentries_t  & DungeonData()const    { return m_dungeondata; }

        const std::string       & OriginalMappaName()const    {return m_origmappafn;}
        const std::string       & OriginalMappaGName()const   {return m_origmappagfn;}

    private:
        dungeonentries_t    m_dungeondata;
        std::string         m_origmappafn;
        std::string         m_origmappagfn;
    };


//========================================================================================
//
//========================================================================================
};

#endif // !dungeon_rng_data_hpp
