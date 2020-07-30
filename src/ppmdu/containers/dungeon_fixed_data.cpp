#include "dungeon_fixed_data.hpp"

namespace pmd2 {namespace stats
{
	//==============================================================
	//FixedDungeonFloorEntry
	//==============================================================
    void FixedDungeonFloorEntry::CopyCtor( const FixedDungeonFloorEntry & other )
    {
		width		= other.width;
		height		= other.height;
		unk1		= other.unk1;
		floormap	= other.floormap;
    }

    void FixedDungeonFloorEntry::MoveCtor( FixedDungeonFloorEntry && other )
    {
		width		= other.width;
		height		= other.height;
		unk1		= other.unk1;
		floormap	= std::move(other.floormap);
    }

	FixedDungeonFloorEntry::FixedDungeonFloorEntry(uint16_t _width, uint16_t _heigth, uint16_t _unk1, std::vector<uint8_t> && _floormap)
	{
		width = _width;
		height = _heigth;
		unk1 = _unk1;
		floormap = std::move(_floormap);
	}

    FixedDungeonFloorEntry::FixedDungeonFloorEntry( const FixedDungeonFloorEntry & other )
    {
        CopyCtor(other);
    }

    FixedDungeonFloorEntry::FixedDungeonFloorEntry( FixedDungeonFloorEntry && other )
    {
        MoveCtor( std::move(other) );
    }

    FixedDungeonFloorEntry & FixedDungeonFloorEntry::operator=( const FixedDungeonFloorEntry & other )
    {
        CopyCtor(other);
        return *this;
    }

    FixedDungeonFloorEntry & FixedDungeonFloorEntry::operator=( FixedDungeonFloorEntry && other )
    {
        MoveCtor( std::move(other) );
        return *this;
    }

	//==============================================================
	//FixedDungeonDB
	//==============================================================
	FixedDungeonDB::FixedDungeonDB(std::size_t reservesize)
	{
		m_floorData.reserve(reservesize);
	}

	void FixedDungeonDB::push_back(FixedDungeonFloorEntry && item)
	{
		m_floorData.push_back(item);
	}

	void FixedDungeonDB::push_back(const FixedDungeonFloorEntry & item)
	{
		m_floorData.push_back(item);
	}
}}