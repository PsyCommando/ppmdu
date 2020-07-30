#include "dungeons_data.hpp"
#include <ppmdu/fmts/mappa.hpp>

namespace pmd2{ namespace stats
{

//==============================================
// DungeonDB
//==============================================
    DungeonDB::DungeonDB()
    {
    }

 //   DungeonDB::DungeonDB(const std::string & fsrootdir, const ConfigLoader & conf, std::shared_ptr<GameText>&& gtext)
 //       :m_conf(conf), m_gametext(gtext), m_rootpath(fsrootdir)
	//{
 //       
	//}

	DungeonDB::~DungeonDB()
	{
	}



    //const pmd2::stats::dungeon_entry & DungeonDB::GetDungeon(globdungeon_id_t id) const
    //{
    //    const DungeonRNGDataSet & ds = GetDungeonDataSet(GetDefaultMappaID());
    //    return ds.Dungeon(id);
    //    // TODO: insert return statement here
    //}

    //pmd2::stats::dungeon_entry & DungeonDB::GetDungeon(globdungeon_id_t id)
    //{
    //    // TODO: insert return statement here
    //}

    //const pmd2::stats::dungeon_entry & DungeonDB::GetDungeon(dungeon_idx_t idx, eMappaID mappaid) const
    //{
    //    // TODO: insert return statement here
    //}

    //pmd2::stats::dungeon_entry & DungeonDB::GetDungeon(dungeon_idx_t idx, eMappaID mappaid)
    //{
    //    // TODO: insert return statement here
    //}

    const DungeonRNGDataSet & DungeonDB::GetDungeonDataSet(eMappaID mappaid) const
    {
        return m_dungeons_mappa.at(mappaid);
    }

    DungeonRNGDataSet & DungeonDB::GetDungeonDataSet(eMappaID mappaid)
    {
        return m_dungeons_mappa.at(mappaid);
    }

    size_t DungeonDB::GetNbDungeons(eMappaID mappaid) const
    {
        return m_dungeons_mappa.at(mappaid).NbDungeons();
    }

    eMappaID DungeonDB::GetDefaultMappaID(eGameVersion version) const
    {
        switch (version)
        {
        case eGameVersion::EoS: return eMappaID::S;
        case eGameVersion::EoT: return eMappaID::T;
        case eGameVersion::EoD: return eMappaID::Y;
        }
    }

    size_t DungeonDB::getNbDataSets() const
    {
        return m_dungeons_mappa.size();
    }

    DungeonDB::iterator DungeonDB::begin()
    {
        return m_dungeons_mappa.begin();
    }

    DungeonDB::const_iterator DungeonDB::begin() const
    {
        return m_dungeons_mappa.begin();
    }

    DungeonDB::iterator DungeonDB::end()
    {
        return m_dungeons_mappa.end();
    }

    DungeonDB::const_iterator DungeonDB::end() const
    {
        return m_dungeons_mappa.end();
    }

    DungeonDB::value_type & DungeonDB::operator[](const key_type & key)
    {
        return m_dungeons_mappa[key];
    }

    bool DungeonDB::empty() const
    {
        return m_dungeons_mappa.empty();
    }

}}