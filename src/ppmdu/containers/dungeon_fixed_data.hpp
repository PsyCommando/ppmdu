#ifndef DUNGEON_FIXED_DATA_HPP
#define DUNGEON_FIXED_DATA_HPP
/*
dungeon_fixed_data.hpp
2020/06/22
psycommando@gmail.com
Description: Objects for storing and editing loaded fixed dungeon floor data from the pmd2 games!
*/
#include <vector>
#include <cstdint>

namespace pmd2 {namespace stats
{
//=====================================================================================
//  Fixed Dungeon Entry
//=====================================================================================

	/*
		FixedDungeonFloorEntry
			Contains details on a single fixed dungeon floor.
			The dimensions of the floor, and what is on each tiles, plus some unknown value.
	*/
	struct FixedDungeonFloorEntry
	{
		enum struct EDungeonTileType : uint8_t
		{
			FLOOR = 0x0,			//Generic room floor											0000 0000
			BREAKABLE_WALL,			//Wall can be broken											0000 0001
			IMPASSABLE_WALL,		//Wall cannot be broken or moved through in any ways			0000 0010
			UNK_WALL,				//Can be moved through, but not destroyed. Might be broken?		0000 0011
			FLOOR_PLAYER_START,		//Spawn player and possibly others								0000 0100


		};

		//Constructions + copy
		FixedDungeonFloorEntry()
			:width(0),height(0),unk1(0)
		{}
		FixedDungeonFloorEntry(uint16_t _width, uint16_t _heigth, uint16_t _unk1, std::vector<uint8_t> && _floormap);
		FixedDungeonFloorEntry(const FixedDungeonFloorEntry & other);
		FixedDungeonFloorEntry(FixedDungeonFloorEntry && moved);
		FixedDungeonFloorEntry & operator=(const FixedDungeonFloorEntry & other);
		FixedDungeonFloorEntry & operator=(FixedDungeonFloorEntry && other);
		void CopyCtor(const FixedDungeonFloorEntry & other);
		void MoveCtor(FixedDungeonFloorEntry && other);

		//Tile access

		//Vars
		uint16_t width;
		uint16_t height;
		uint16_t unk1;

		//Decompressed 2d floor map, as a single array of tiles
		std::vector<uint8_t> floormap;
	};

//=====================================================================================
//  Fixed Dungeon DB
//=====================================================================================

	/*
		FixedDungeonDB
			Contains details on all fixed dungeons in the pmd2 games.
			Can be accessed as a container.
	*/
	class FixedDungeonDB
	{
	public:
		typedef std::vector<FixedDungeonFloorEntry>::iterator       iterator;
		typedef std::vector<FixedDungeonFloorEntry>::const_iterator const_iterator;

		FixedDungeonDB() {}
		FixedDungeonDB(std::size_t reservesize);
		~FixedDungeonDB() {}

		inline std::size_t size()const { return m_floorData.size(); }
		inline bool        empty()const { return m_floorData.empty(); }
		inline void        resize(size_t newsz) { return m_floorData.resize(newsz); }

		//The items are guaranteed to stay allocated as long as the object exists!
		inline const FixedDungeonFloorEntry & Item(uint16_t itemindex)const { return m_floorData[itemindex]; }
		inline       FixedDungeonFloorEntry & Item(uint16_t itemindex) { return m_floorData[itemindex]; }

		inline const FixedDungeonFloorEntry & operator[](uint16_t itemindex)const { return m_floorData[itemindex]; }
		inline       FixedDungeonFloorEntry & operator[](uint16_t itemindex) { return m_floorData[itemindex]; }

		void push_back(FixedDungeonFloorEntry       && item);
		void push_back(const FixedDungeonFloorEntry  & item);

		iterator       begin() { return m_floorData.begin(); }
		iterator       end() { return m_floorData.end(); }
		const_iterator begin()const { return m_floorData.begin(); }
		const_iterator end()const { return m_floorData.end(); }

		//Data manipulation
	public:


	private:
		std::vector<FixedDungeonFloorEntry> m_floorData;
	};



}}
#endif