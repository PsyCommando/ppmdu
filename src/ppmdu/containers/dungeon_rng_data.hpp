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
#include <array>
#include <unordered_map>
#include <map>

namespace pmd2 { namespace stats
{
//========================================================================================
//  Constants
//========================================================================================
    enum struct eMappaID : char //Identifier of the possible mappa files, mappa_g files only exist in EoS
    {
        NONE = 0,   //bad filenme or something else
        S = 's',    //mappa_s / mappa_gs the mappa_s file is not in the EoT/D versions
        T = 't',    //mappa_t / mappa_gt
        Y = 'y',    //mappa_y / mappa_gy
    };

    //Mappa file per version
    const int NB_MAPPA_PAIRS_EOS = 3;
    const int NB_MAPPA_EOTD = 2;
    const int NB_TRAPS_TYPES_EOS = 25;
    const int NB_TRAPS_TYPES_EOTD = 23;
    extern const std::unordered_map<eGameVersion, std::vector<eMappaID>> MAPPA_FILES_VERSIONS_FOR_VERSIONS; //Mappa files id for each versions of the game

    //Bit flag for the terrain bit for a given floor
    const uint16_t MASK_TERRAIN_HAZARDS     = 0b0000'0001;  //Is on if there are water/lava/void tiles on the floor
    const uint16_t MASK_TERRAIN_NON_SQUARE  = 0b0000'0100;  //Is on if the rooms shouldn't be perfect squares

//========================================================================================
//  trap_floor_spawn_entry
//========================================================================================
	/*
		Contains the spawn chance for all trap types
	*/
	template<class _gameVer_t>
	    struct trap_floor_spawn_entry
	{
        static const eGameVersion GAMEVERSION = _gameVer_t::value;
        typedef typename std::conditional<GAMEVERSION == eGameVersion::EoS,
			std::integral_constant<size_t, NB_TRAPS_TYPES_EOS>,
			typename std::conditional<GAMEVERSION == eGameVersion::EoT || GAMEVERSION == eGameVersion::EoD,
				std::integral_constant<size_t, NB_TRAPS_TYPES_EOTD>,
				std::nullptr_t>::type
		>::type NB_TRAPS_TYPES_T;
        static const size_t NB_TRAPS = NB_TRAPS_TYPES_T::value;
		std::array<uint16_t, NB_TRAPS> traps_probs;

        /*
            Read the traps probability list for the floor.
        */
        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itEnd )
        {
            //Go through all the traps entries
            for (uint16_t & trapprob : traps_probs)
                itReadfrom = utils::ReadIntFromBytes(trapprob, itReadfrom, itEnd);
            return itReadfrom;
        }

		/*
			Write the traps probability list for the floor.
		*/
		template<class _outit>
			_outit WriteToContainer(_outit itWrite) const
		{
            for (uint16_t trapprob : traps_probs)
                itWrite = utils::WriteIntToBytes(trapprob, itWrite);
			return itWrite;
		}
	};


//========================================================================================
//  dungeon_items_lst
//========================================================================================
	/*
		Spawn probability for a given list of items
	*/
	struct dungeon_items_lst
	{
		typedef std::map<uint16_t, uint16_t> probabilitylist_t;
		probabilitylist_t item_probabilities; //First value is item id, second value is probability as a fixed point 2 digits precision value stored in an integer. The value is multiplied by 100 to have it all fit in the integer.

		static const uint16_t MASK_INCR_ITEM_ID		= 0x7500; //When this mask is detected, the we subtract the mask value and 0x30 (Or just 30,000 decimal from the whole thing)  0111 0101 0000 0000
		static const uint16_t ITEM_SKIP_SUBTRACTOR	= 0x7530; //We apparently need to subtract 30,000 from the value when we hit one of the skip command, then add the result to the item id.
		static const uint16_t ITEM_GUARANTEED		= 0xFFFF; //If get this as spawn chance bit, we have a guaranteed spawn

        //Iterator Access
        typedef typename probabilitylist_t::iterator        iterator;
        typedef typename probabilitylist_t::const_iterator  const_iterator;
        iterator        begin()     {return item_probabilities.begin(); }
        const_iterator  begin()const{return item_probabilities.begin(); }
        iterator        end()       {return item_probabilities.end();}
        const_iterator  end()const  {return item_probabilities.end();}
        uint16_t        size()const { return static_cast<uint16_t>(item_probabilities.size()); }
        bool            empty()const { return item_probabilities.empty(); }


        /*
            Read and decode the item probablity list!
        */
        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itEnd )
        {
			uint16_t curitemid = 0; //Index of a category (0x0-0xF) or an item (0x10 to 0x180)

            //Parse the entries
			while (itReadfrom != itEnd)
			{
				//Get command half-word
				uint16_t cmd = 0;
				itReadfrom = utils::ReadIntFromBytes(cmd, itReadfrom, itEnd);

				if (cmd != ITEM_GUARANTEED && cmd & MASK_INCR_ITEM_ID) //Filter out the special guaranteed spawn mask
				{
					//We gotta add to the item id
					curitemid += (cmd - ITEM_SKIP_SUBTRACTOR);
				}
				else
				{
					//We set the probabiliy for the current item id, and then increment the current item id!
					item_probabilities.emplace(curitemid, cmd);
                    ++curitemid;
				}
			}
            return itReadfrom;
        }


		/*
			Write and encode the item probability list!
		*/
		template<class _outit>
			_outit WriteToContainer(_outit itWrite) const
		{
			uint32_t lastitemid = 0; //The id of the item/category that was last processed

			for (const auto & item : item_probabilities)
			{
				
				assert((item.first - lastitemid) > 0); //negative diff would be bad...
				uint16_t itemiddiff = item.first - lastitemid; 
				if (itemiddiff > 1)
				{
					//If the current item is further away from the last one than 1 step, we wanna make a skip byte first!
					itWrite = utils::WriteIntToBytes(static_cast<uint16_t>(ITEM_SKIP_SUBTRACTOR + itemiddiff), itWrite);
				}

				//Then write the actual probability
				itWrite = utils::WriteIntToBytes(item.second, itWrite);
			}

			return itWrite;
		}
	};

//========================================================================================
//  pkmn_spawn_entry
//========================================================================================
	/*
		Represents a single entry in the list of pokemon to spawn on a given level.
	*/
	struct pkmn_spawn_entry
	{
		static const size_t SIZE = 8;
		uint16_t lvlmult  = 0;	//Shift right by 7 and multiply by 2 to get level of spawned pokemon
		uint16_t spwnrte  = 0;	//Spawn weight
		uint16_t spwnrte2 = 0;	//Spawn weight 2
		uint16_t pkmnid   = 0;	//Entity id of pokemon to spawn

        template<class _outit>
			_outit WriteToContainer(_outit itwriteto)const
		{
			itwriteto = utils::WriteIntToBytes(lvlmult, itwriteto);
			itwriteto = utils::WriteIntToBytes(spwnrte, itwriteto);
			itwriteto = utils::WriteIntToBytes(spwnrte2, itwriteto);
			itwriteto = utils::WriteIntToBytes(pkmnid, itwriteto);
			return itwriteto;
		}

        template<class _init>
			_init ReadFromContainer(_init itReadfrom, _init itbegpkmnLUT)
		{
			itReadfrom = utils::ReadIntFromBytes(lvlmult, itReadfrom, itbegpkmnLUT);
			itReadfrom = utils::ReadIntFromBytes(spwnrte, itReadfrom, itbegpkmnLUT);
			itReadfrom = utils::ReadIntFromBytes(spwnrte2, itReadfrom, itbegpkmnLUT);
			itReadfrom = utils::ReadIntFromBytes(pkmnid, itReadfrom, itbegpkmnLUT);
			return itReadfrom;
		}

		inline bool isnull()const
		{
			return lvlmult == 0 && spwnrte == 0 && spwnrte2 == 0 && pkmnid == 0;
		}
	};

//========================================================================================
// pkmn_spawn_lst
//========================================================================================
	/*
		Holds the list of pokemons to spawn on a single floor!
	*/
    struct pkmn_floor_spawn_lst
    {
		std::vector<pkmn_spawn_entry> pkm_spawns; //Pokemons that spawn on this floor!

        //Iterator Access
        typedef typename std::vector<pkmn_spawn_entry>::iterator        iterator;
        typedef typename std::vector<pkmn_spawn_entry>::const_iterator  const_iterator;
        iterator        begin() { return pkm_spawns.begin(); }
        const_iterator  begin()const { return pkm_spawns.begin(); }
        iterator        end() { return pkm_spawns.end(); }
        const_iterator  end()const { return pkm_spawns.end(); }
        uint16_t        size()const { return static_cast<uint16_t>(pkm_spawns.size()); }
        bool            empty()const { return pkm_spawns.empty(); }

        //Write the structure using an iterator to a byte container
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            for( const auto & afloor : pkm_spawns )
                itwriteto = afloor.WriteToContainer(itwriteto);

			//Write the last nulled out entry
            return std::fill_n(itwriteto, pkmn_spawn_entry::SIZE, 0);
        }

        /*
            Read the structure from an iterator on a byte container
        */
        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itbegpkmnLUT )
        {
            //Parse the entries
            while( itReadfrom != itbegpkmnLUT )
            {
                auto			 itsav = itReadfrom;
                pkmn_spawn_entry info;
                itReadfrom = info.ReadFromContainer(itReadfrom, itbegpkmnLUT);

                if( info.isnull() )
                {
                    itReadfrom = itsav;
                    break;
                }
                else
                    pkm_spawns.push_back( std::move(info) );
            }

            return itReadfrom;
        }
    };

//========================================================================================
//  floor_data_entry
//========================================================================================
 	/*
		Represents data for a single floor.
	*/
	struct floor_data_entry
	{
		static const size_t SIZE = 32; //bytes

		uint8_t floorshape			= 0;
		uint8_t roomdensity			= 0;
		uint8_t tilesetid			= 0;
		uint8_t musicid				= 0;
		uint8_t defaultweather		= 0; //(0: Clear, 1: Sunny, 2: Sandstorm, 3: Cloudy, 4: Rainy, 5: Hail , 6: Fog, 7: Snow, 8: Random)
		uint8_t minconnections		= 0;
		uint8_t pkmndensity			= 0;
		uint8_t shopratio			= 0;
		uint8_t mhouseratio			= 0;
		uint8_t unk6				= 0;
		uint8_t stickychance		= 0;
		uint8_t deadendsallowed		= 0;
		uint8_t hazardtiles			= 0;
		uint8_t terrainbits			= 0;
		uint8_t unk11				= 0;
		uint8_t itemdensity			= 0;
		uint8_t trapdensity			= 0;
		uint8_t floornb				= 0;
		uint8_t fixedfloorid		= 0;
		uint8_t extrahallwdensity	= 0;
		uint8_t burieditemdensity	= 0;
		uint8_t waterdensity		= 0;
		uint8_t darklvl				= 0; //(0: No darkness, 1: Heavy darkness, 2: Light darkness)
		uint8_t maxcoin				= 0; //Maximum amount of coins divided by 5
		uint8_t keclitempos			= 0;
		uint8_t emptymhchance		= 0;
		uint8_t unk16				= 0;
		uint8_t hiddenstrschnc		= 0;
		uint16_t enemyiq			= 0;
		uint8_t allowiqboostr		= 0;
		uint8_t unk19				= 0;


        //Write the structure using an iterator to a byte container
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( floorshape,			itwriteto );
            itwriteto = utils::WriteIntToBytes( roomdensity,		itwriteto );
            itwriteto = utils::WriteIntToBytes( tilesetid,			itwriteto );
            itwriteto = utils::WriteIntToBytes( musicid,			itwriteto );
            itwriteto = utils::WriteIntToBytes( defaultweather,		itwriteto );
            itwriteto = utils::WriteIntToBytes( minconnections,		itwriteto );
            itwriteto = utils::WriteIntToBytes( pkmndensity,		itwriteto );
            itwriteto = utils::WriteIntToBytes( shopratio,			itwriteto );
            itwriteto = utils::WriteIntToBytes( mhouseratio,		itwriteto );
			itwriteto = utils::WriteIntToBytes( unk6,				itwriteto );
			itwriteto = utils::WriteIntToBytes( stickychance,		itwriteto );
			itwriteto = utils::WriteIntToBytes( deadendsallowed,	itwriteto );
			itwriteto = utils::WriteIntToBytes( hazardtiles,		itwriteto );
			itwriteto = utils::WriteIntToBytes( terrainbits,		itwriteto );
			itwriteto = utils::WriteIntToBytes( unk11,				itwriteto );
			itwriteto = utils::WriteIntToBytes( itemdensity,		itwriteto );
			itwriteto = utils::WriteIntToBytes( trapdensity,		itwriteto );
			itwriteto = utils::WriteIntToBytes( floornb,			itwriteto );
			itwriteto = utils::WriteIntToBytes( fixedfloorid,		itwriteto );
			itwriteto = utils::WriteIntToBytes( extrahallwdensity,	itwriteto );
			itwriteto = utils::WriteIntToBytes( burieditemdensity,	itwriteto );
			itwriteto = utils::WriteIntToBytes( waterdensity,		itwriteto );
			itwriteto = utils::WriteIntToBytes( darklvl,			itwriteto );
			itwriteto = utils::WriteIntToBytes( maxcoin,			itwriteto );
			itwriteto = utils::WriteIntToBytes( keclitempos,		itwriteto );
			itwriteto = utils::WriteIntToBytes( emptymhchance,		itwriteto );
			itwriteto = utils::WriteIntToBytes( unk16,				itwriteto );
			itwriteto = utils::WriteIntToBytes( hiddenstrschnc,		itwriteto );
			itwriteto = utils::WriteIntToBytes( enemyiq,			itwriteto );
			itwriteto = utils::WriteIntToBytes( allowiqboostr,		itwriteto );
			itwriteto = utils::WriteIntToBytes( unk19,				itwriteto );
            return itwriteto;
        }

        //Read the structure from an iterator on a byte container
        template<class _init>
            _init ReadFromContainer( _init itreadfrom, _init itend )
        {
            itreadfrom = utils::ReadIntFromBytes( floorshape,		itreadfrom, itend );
            itreadfrom = utils::ReadIntFromBytes( roomdensity,      itreadfrom, itend );
            itreadfrom = utils::ReadIntFromBytes( tilesetid,		itreadfrom, itend );
            itreadfrom = utils::ReadIntFromBytes( musicid,			itreadfrom, itend );
            itreadfrom = utils::ReadIntFromBytes( defaultweather,   itreadfrom, itend );
            itreadfrom = utils::ReadIntFromBytes( minconnections,   itreadfrom, itend );
            itreadfrom = utils::ReadIntFromBytes( pkmndensity,      itreadfrom, itend );
            itreadfrom = utils::ReadIntFromBytes( shopratio,		itreadfrom, itend );
            itreadfrom = utils::ReadIntFromBytes( mhouseratio,      itreadfrom, itend );
			itreadfrom = utils::ReadIntFromBytes( unk6,				itreadfrom, itend );
			itreadfrom = utils::ReadIntFromBytes( stickychance,		itreadfrom, itend );
			itreadfrom = utils::ReadIntFromBytes( deadendsallowed,	itreadfrom, itend );
			itreadfrom = utils::ReadIntFromBytes( hazardtiles,		itreadfrom, itend );
			itreadfrom = utils::ReadIntFromBytes( terrainbits,		itreadfrom, itend );
			itreadfrom = utils::ReadIntFromBytes( unk11,			itreadfrom, itend );
			itreadfrom = utils::ReadIntFromBytes( itemdensity,		itreadfrom, itend );
			itreadfrom = utils::ReadIntFromBytes( trapdensity,		itreadfrom, itend );
			itreadfrom = utils::ReadIntFromBytes( floornb,			itreadfrom, itend );
			itreadfrom = utils::ReadIntFromBytes( fixedfloorid,		itreadfrom, itend );
			itreadfrom = utils::ReadIntFromBytes( extrahallwdensity,itreadfrom, itend );
			itreadfrom = utils::ReadIntFromBytes( burieditemdensity,itreadfrom, itend );
			itreadfrom = utils::ReadIntFromBytes( waterdensity,		itreadfrom, itend );
			itreadfrom = utils::ReadIntFromBytes( darklvl,			itreadfrom, itend );
			itreadfrom = utils::ReadIntFromBytes( maxcoin,			itreadfrom, itend );
			itreadfrom = utils::ReadIntFromBytes( keclitempos,		itreadfrom, itend );
			itreadfrom = utils::ReadIntFromBytes( emptymhchance,	itreadfrom, itend );
			itreadfrom = utils::ReadIntFromBytes( unk16,			itreadfrom, itend );
			itreadfrom = utils::ReadIntFromBytes( hiddenstrschnc,	itreadfrom, itend );
			itreadfrom = utils::ReadIntFromBytes( enemyiq,			itreadfrom, itend );
			itreadfrom = utils::ReadIntFromBytes( allowiqboostr,	itreadfrom, itend );
			itreadfrom = utils::ReadIntFromBytes( unk19,			itreadfrom, itend );
            return itreadfrom;
        }
	};

//========================================================================================
//  DungeonFloor
//========================================================================================

    /***********************************************************************
        dungeon_floor_info
            18 bytes entry that contains indices to the data lists that 
            applies to this particular floor!
    ***********************************************************************/
    struct dungeon_floor_info
    {
        static const size_t SIZE = 18; //bytes
        uint16_t idfloordata      = 0;
        uint16_t idpkspwn         = 0;
        uint16_t idlutc           = 0;
        uint16_t iditemspwn       = 0;
        uint16_t idshopdat        = 0;
        uint16_t idmhouseitem     = 0;
        uint16_t idburieditemsdat = 0;
        uint16_t unk2             = 0;  //indice to an item list of unknown purpose
        uint16_t unk3             = 0;  //indice to an item list of unknown purpose

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

//========================================================================================
//  Dungeon
//========================================================================================
    /***********************************************************************  
        dungeon_entry
            Contains several 18 bytes entries(dungeon_floor_info) containing indices to the 
            data lists that applies to this dungeon floor.
    ***********************************************************************/
    struct dungeon_entry
    {
        std::vector<dungeon_floor_info> floors;

        //Write the structure using an iterator to a byte container
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            //first write the leading dummy null entry
            itwriteto = std::fill_n( itwriteto, dungeon_floor_info::SIZE, 0 );

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
            std::advance( itReadfrom, dungeon_floor_info::SIZE );

            //Parse the entries
            while( itReadfrom != itbegdungeonLUT )
            {
                auto             itsav = itReadfrom;
                dungeon_floor_info info;
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


    /*
        dungeon_floor_override
            Contains data from the mappa_g file with overrides for some of the floors in the matching mappa file.
    */
    struct dungeon_floor_override
    {
        static const size_t SIZE = 4;
        uint8_t tilesetid       = 0;
        uint8_t fixedfloorid    = 0;
        uint8_t unk0            = 0;
        uint8_t unk1            = 0;

        //Write the structure using an iterator to a byte container
        template<class _outit>
        _outit WriteToContainer(_outit itwriteto)const
        {
            //Write the whole thing in one go
            uint32_t buf = static_cast<uint32_t>((tilesetid << 24) | (fixedfloorid << 16) | (unk0 << 8) | unk1);
            itwriteto = utils::WriteIntToBytes(buf, itwriteto);
            return itwriteto;
        }

        template<class _init>
        _init ReadFromContainer(_init itReadfrom, _init itEnd)
        {
            //Read the whole thing in one go
            uint32_t buf = 0;
            itReadfrom = utils::ReadIntFromBytes(buf, itReadfrom, itEnd);
            tilesetid       = (buf >> 24) & 0xFF;
            fixedfloorid    = (buf >> 16) & 0xFF;
            unk0            = (buf >> 8) & 0xFF;
            unk1            = buf & 0xFF;
            return itReadfrom;
        }
    };


    /*
        dungeon_overrides
            Represents a list of dungeon floors indices for a given dungeon.
    */
    struct dungeon_override
    {
        std::vector<uint32_t> floor_indices;    //List of floor data indices for this dungeon

        //Write the structure using an iterator to a byte container
        template<class _outit>
            _outit WriteToContainer(_outit itwriteto)const
        {
            //Insert first null entry
            itwriteto = utils::WriteIntToBytes(static_cast<uint32_t>(0), itwriteto);

            //Write the indices
            for (uint32_t v : floor_indices)
                itwriteto = utils::WriteIntToBytes(v, itwriteto);
            return itwriteto;
        }


        /*
            Read the structure from an iterator on a byte container
        */
        template<class _init>
            _init ReadFromContainer(_init itReadfrom, _init itNextEntry)
        {
            //Skip the first null value of the list
            uint32_t curval = 0;
            itReadfrom = utils::ReadIntFromBytes(curval, itReadfrom, itNextEntry);
            assert(curval == 0);

            if (itReadfrom == itNextEntry)
                assert(false);

            //Read the floors indics 
            do
            {
                itReadfrom = utils::ReadIntFromBytes(curval, itReadfrom, itNextEntry);
                if (curval != 0)
                    floor_indices.push_back(curval);
            }while (itReadfrom != itNextEntry);

            return itReadfrom;
        }
    };
//========================================================================================
//  DungeonFloor
//========================================================================================
//    /*
//		Contains the data for a single dungeon floor.
//    */
//    class DungeonFloor
//    {
//    public:
//
//        DungeonFloor()
//        {}
//
//        template<class _init>
//            DungeonFloor(
//                dungeon_items_lst            &&  itemsp,
//                dungeon_items_lst            &&  itemshop,
//                dungeon_items_lst            &&  itemsmhouse,
//                dungeon_items_lst            &&  items,
//                floor_data_entry                &&  fldat, 
//                std::vector<pkmn_spawn_entry>   &&  pkmnspwn, 
//                _init                               ittrapsbeg, 
//                _init                               ittrapsend, 
//                uint8_t                             fixedfloorovr = 0, 
//                uint8_t                             tilesetovr = 0,
//                uint8_t                             unk0 = 0,
//                uint8_t                             unk1 = 0)
//                :m_items_probs(itemsp), 
//                m_floor_dat(fldat), 
//                m_pkmn_spawns(pkmnspwn), 
//                m_traps_prob(ittrapsbeg, ittrapsend),
//                m_fixedflooridoverride(fixedfloorovr),
//                m_tilesetidoverride(tilesetovr),
//                m_unk0(unk0),
//                m_unk1(unk1)
//        {}
//
//        DungeonFloor(const DungeonFloor &) = default;
//        DungeonFloor(DungeonFloor &&) = default;
//        DungeonFloor& operator=(const DungeonFloor &) = default;
//        DungeonFloor& operator=(DungeonFloor &&) = default;
//
//        bool    isFixedFloor()const { return m_fixedflooridoverride > 0 || m_floor_dat.fixedfloorid > 0; }
//
//        uint8_t get_overriden_fixed_floor()const { return m_fixedflooridoverride; }
//        void    set_overriden_fixed_floor(uint8_t fixedid) { m_fixedflooridoverride = fixedid; }
//
//        uint8_t get_overriden_tileset()const { return m_tilesetidoverride; }
//        void    set_overriden_tileset(uint8_t tilesetid) { m_tilesetidoverride = tilesetid; }
//
//        uint8_t get_unk0()const { return m_unk0; }
//        void    set_unk0(uint8_t val) { m_unk0 = val; }
//
//        uint8_t get_unk1()const { return m_unk1; }
//        void    set_unk1(uint8_t val) { m_unk1 = val; }
//
//        const floor_data_entry  & data()const  { return m_floor_dat; }
//        floor_data_entry        & data()       { return m_floor_dat; }
//
//        const std::vector<pkmn_spawn_entry> & pokemons()const   { return m_pkmn_spawns; }
//        std::vector<pkmn_spawn_entry>       & pokemons()        { return m_pkmn_spawns; }
//
//        const std::vector<uint16_t> & traps()const   { return m_traps_prob; }
//        std::vector<uint16_t>       & traps()        { return m_traps_prob; }
//
//        //Item lists:
//        const dungeon_items_lst  & items_spawn()const  { return m_items_probs; }
//        dungeon_items_lst        & items_spawn()       { return m_items_probs; }
//
//        const dungeon_items_lst  & items_shop()const  { return m_items_shop_probs; }
//        dungeon_items_lst        & items_shop()       { return m_items_shop_probs; }
//
//        const dungeon_items_lst  & items_monster_house()const  { return m_items_mhouse_probs; }
//        dungeon_items_lst        & items_monster_house()       { return m_items_mhouse_probs; }
//
//        const dungeon_items_lst  & items_buried()const  { return m_items_buried_probs; }
//        dungeon_items_lst        & items_buried()       { return m_items_buried_probs; }
//
//        const dungeon_items_lst  & items_unk2()const  { return m_items_unk2_probs; }
//        dungeon_items_lst        & items_unk2()       { return m_items_unk2_probs; }
//
//        const dungeon_items_lst  & items_unk3()const  { return m_items_unk3_probs; }
//        dungeon_items_lst        & items_unk3()       { return m_items_unk3_probs; }
//
//    private:
//        //Item lists associated with this floor
//        dungeon_items_lst            m_items_probs;
//        dungeon_items_lst            m_items_shop_probs;
//        dungeon_items_lst            m_items_mhouse_probs;
//        dungeon_items_lst            m_items_buried_probs;
//        dungeon_items_lst            m_items_unk2_probs;
//        dungeon_items_lst            m_items_unk3_probs;
//
//        floor_data_entry                m_floor_dat;
//        std::vector<pkmn_spawn_entry>   m_pkmn_spawns;
//        std::vector<uint16_t>           m_traps_prob;
//        uint8_t                         m_fixedflooridoverride = 0;
//        uint8_t                         m_tilesetidoverride = 0;
//        uint8_t                         m_unk0 = 0;
//        uint8_t                         m_unk1 = 0;
//    };
//
////========================================================================================
////  DungeonDataEntry
////========================================================================================
//	/*
//		Contains all the data for a given dungeon.
//	*/
//    struct DungeonDataEntry
//    {
//        typedef std::vector<DungeonFloor>      floors_t;
//        typedef floors_t::iterator             iterator;
//        typedef floors_t::const_iterator       const_iterator;
//
//        size_t          size()const     { return m_floors.size();}
//        bool            empty()const    { return m_floors.empty(); }
//        iterator        begin()         { return m_floors.begin(); }
//        const_iterator  begin()const    { return m_floors.begin(); }
//        iterator        end()           { return m_floors.end(); }
//        const_iterator  end()const      { return m_floors.end(); }
//
//        void            add(const DungeonFloor & fl) { m_floors.push_back(fl); }
//        void            add(DungeonFloor && fl) { m_floors.push_back(fl); }
//
//        void            remove(size_t index) { m_floors.erase(utils::advAsMuchAsPossible(m_floors.begin(), m_floors.end(), index)); }
//        void            remove(iterator itrem) { m_floors.erase(itrem); }
//        floors_t        m_floors;
//    };

//========================================================================================
//  DungeonRNGDataSet
//========================================================================================
	/*
		Contains all the data for all dungeons in a set of mappa files.
	*/
    class DungeonRNGDataSet
    {
        friend class MappaParser;
        friend class MappaGParser;
    public:

        DungeonRNGDataSet()
            :m_origmappa(eMappaID::NONE)
        {}

        /*
            DungeonRNGDataSet
                -fnamemappa  : name of the original mappa_*.bin file
                -fnamemappag : name of the original mappa_g*.bin file
        */
        DungeonRNGDataSet(eMappaID id)
            :m_origmappa(id)
        {}

        DungeonRNGDataSet(DungeonRNGDataSet&&) = default;
        DungeonRNGDataSet(const DungeonRNGDataSet&) = default;
        DungeonRNGDataSet & operator=(const DungeonRNGDataSet &) = default;
        DungeonRNGDataSet & operator=(DungeonRNGDataSet&&) = default;

        std::string OriginalMappaName()const      {return (std::stringstream()<<"mappa_" <<static_cast<char>(m_origmappa) <<".bin" ).str();}
        std::string OriginalMappaGName()const     {return (std::stringstream()<<"mappa_g" <<static_cast<char>(m_origmappa) <<".bin" ).str();}
        eMappaID    OriginalMappaID()const        {return m_origmappa;}
        void        OriginalMappaID(eMappaID id)  {m_origmappa = id;}

        //
        //  Base Dungeon Handling
        //

        //*** Dungeon data ***
        const dungeon_entry & Dungeon(uint16_t id)const  { return m_dungeonstbl[id]; }
        dungeon_entry       & Dungeon(uint16_t id)       { return m_dungeonstbl[id]; }
        uint16_t              NbDungeons()const          { return static_cast<uint16_t>(m_dungeonstbl.size()); }

        const std::vector<dungeon_entry> & Dungeons()const  { return m_dungeonstbl; }
        std::vector<dungeon_entry>       & Dungeons()       { return m_dungeonstbl; }

        //*** Floor data ***
        const floor_data_entry & Floor(uint16_t id)const  { return m_floordatatbl[id]; }
        floor_data_entry       & Floor(uint16_t id)       { return m_floordatatbl[id]; }
        uint16_t                 NbFloors()const          { return static_cast<uint16_t>(m_floordatatbl.size()); }

        const std::vector<floor_data_entry> & Floors()const  { return m_floordatatbl; }
        std::vector<floor_data_entry>       & Floors()       { return m_floordatatbl; }

        //*** Pokemon spawn lists ***
        const pkmn_floor_spawn_lst & PokemonSpawnList(uint16_t id)const  { return m_pkmnspwnlststbl[id]; }
        pkmn_floor_spawn_lst       & PokemonSpawnList(uint16_t id)       { return m_pkmnspwnlststbl[id]; }
        uint16_t                     NbPokemonSpawnLists()const          { return static_cast<uint16_t>(m_pkmnspwnlststbl.size()); }

        const std::vector<pkmn_floor_spawn_lst> & PokemonSpawnLists()const  { return m_pkmnspwnlststbl; }
        std::vector<pkmn_floor_spawn_lst>       & PokemonSpawnLists()       { return m_pkmnspwnlststbl; }

        //*** Items lists ***
        const dungeon_items_lst & ItemsList(uint16_t id)const  { return m_itemslststbl[id]; }
        dungeon_items_lst       & ItemsList(uint16_t id)       { return m_itemslststbl[id]; }
        uint16_t                  NbItemsLists()const          { return static_cast<uint16_t>(m_itemslststbl.size()); }

        const std::vector<dungeon_items_lst> & ItemsLists()const  { return m_itemslststbl; }
        std::vector<dungeon_items_lst>       & ItemsLists()       { return m_itemslststbl; }

        //*** Traps lists ***
        const std::vector<uint16_t> & TrapsList(uint16_t id)const  { return m_trapslsts[id]; }
        std::vector<uint16_t>       & TrapsList(uint16_t id)       { return m_trapslsts[id]; }
        uint16_t                      NbTrapsLists()const          { return static_cast<uint16_t>(m_trapslsts.size()); }

        const std::vector<std::vector<uint16_t>> & TrapsLists()const  { return m_trapslsts; }
        std::vector<std::vector<uint16_t>>       & TrapsLists()       { return m_trapslsts; }


        //
        //  Dungeon Floor Overrides Handling
        //

        //*** Overridden Dungeon floor indices ***
        const dungeon_override & OverridenDungeon(uint16_t id)const  { return m_dungeons_ovr[id]; }
        dungeon_override       & OverridenDungeon(uint16_t id)       { return m_dungeons_ovr[id]; }
        uint16_t                 NbOverridenDungeons()const          { return static_cast<uint16_t>(m_dungeons_ovr.size()); }

        const std::vector<dungeon_override>  & OverridenDungeons()const  { return m_dungeons_ovr; }
        std::vector<dungeon_override>        & OverridenDungeons()       { return m_dungeons_ovr; }

        //*** Overridden Dungeon floor data ***
        const dungeon_floor_override & OverridenFloor(uint16_t id)const  { return m_dungeonFloorData[id]; }
        dungeon_floor_override       & OverridenFloor(uint16_t id)       { return m_dungeonFloorData[id]; }
        uint16_t                       NbOverridenFloors()const          { return static_cast<uint16_t>(m_dungeonFloorData.size()); }

        const std::vector<dungeon_floor_override>  & OverridenFloors()const  { return m_dungeonFloorData; }
        std::vector<dungeon_floor_override>        & OverridenFloors()       { return m_dungeonFloorData; }


        eMappaID                                        m_origmappa;            //The id of the mappa file this data came from. Used to keep track of what mappa to repack this into

        //Base dungeon data loaded from mappa
        std::vector<dungeon_entry>					    m_dungeonstbl;			//All dungeons data, containing floor entries which refers to lists in the tables below
        std::vector<floor_data_entry>				    m_floordatatbl;			//All floor data for all floors
        std::vector<pkmn_floor_spawn_lst>				m_pkmnspwnlststbl;		//All pokemon spawn lists for all floors
        std::vector<dungeon_items_lst>				    m_itemslststbl;		    //All items lists for all floors
        std::vector<std::vector<uint16_t>>	            m_trapslsts;			//All trap spawn lists for all floors

        //Overrides stuff loaded from mappa_g, unique to Explorers of sky
        std::vector<dungeon_override>                   m_dungeons_ovr;        //List of structs containing the indices in the m_dungeonFloorData for each of their floors
        std::vector<dungeon_floor_override>             m_dungeonFloorData;     //List of override data for every single dungeon floors referred to in the indice lists
    };


//========================================================================================
//
//========================================================================================
}};

#endif // !dungeon_rng_data_hpp
