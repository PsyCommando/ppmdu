#ifndef DUNGEONS_DATA_HPP
#define DUNGEONS_DATA_HPP
/*
dungeons_data.hpp

Class handling sorting dungeon information for easier use in the program.
*/
#include <ppmdu/pmd2/pmd2.hpp>
#include <ppmdu/pmd2/pmd2_configloader.hpp>
#include <ppmdu/containers/dungeon_fixed_data.hpp>
#include <ppmdu/containers/dungeon_rng_data.hpp>
#include <ppmdu/pmd2/pmd2_text.hpp>
#include <cstdint>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>


namespace pmd2{ namespace stats
{
    typedef uint16_t globdungeon_id_t;          //Identifier for a global dungeon id, as used in the game's scripts
    typedef uint16_t dungeon_idx_t;             //Identifier for a DungeonSet dungeon Index
    typedef uint16_t dungeonfloor_id_t;         //Identifier for a DungeonSet dungeon floor entry
    typedef uint16_t dungeonpklst_id_t;         //Identifier for a DungeonSet pokemon spawn data list
    typedef uint16_t dungeontraplst_id_t;       //Identifier for a DungeonSet trap probabilities list
    typedef uint16_t dungeonitemlst_id_t;       //Identifier for a DungeonSet item spawn probability list

//==============================================
// DungeonDB
//==============================================
	/*
		DungeonDB
			Organize and regroup all the loaded dungeon data for easy retrieval and editing.
	*/
	class DungeonDB
	{
        typedef std::unordered_map<eMappaID, DungeonRNGDataSet> mappa_container_t;
	public:
        DungeonDB();
        //DungeonDB(const std::string & fsrootdir, const ConfigLoader & conf, std::shared_ptr<GameText> && gtext);
        ~DungeonDB();

        //Copying/moving
        DungeonDB(const DungeonDB&) = default;
        DungeonDB(DungeonDB&&) = default;
        DungeonDB & operator=(const DungeonDB&) = default;
        DungeonDB & operator=(DungeonDB&&) = default;

		/*
            File IO
        */
		//void LoadDungeonData();
		//void WriteDungeonData()const;

        ///*
        //    Get a dungeon's data using its global id, as used in the script files
        //*/
        //const DungeonRNGDataSet   & GetDungeon(globdungeon_id_t id)const;
        //DungeonRNGDataSet         & GetDungeon(globdungeon_id_t id);

        ///*
        //    Get a dungeon using it mappa file local id
        //*/
        //const DungeonRNGDataSet   & GetDungeon(dungeon_idx_t idx, eMappaID mappaid)const;
        //DungeonRNGDataSet         & GetDungeon(dungeon_idx_t idx, eMappaID mappaid);

        /*
            Container access
        */
        //Get a specific mappa dataset from this db! Does not create anything if the mappa set doesn't exist, unlike operator[]
        const DungeonRNGDataSet & GetDungeonDataSet(eMappaID mappaid)const;
        DungeonRNGDataSet       & GetDungeonDataSet(eMappaID mappaid);

        //Returns the amount of dungeons in a given mappa dataset contained in this DB!
        size_t GetNbDungeons(eMappaID mappaid)const;

        //Returns the number of mappa data sets in this DB!
        size_t getNbDataSets()const;

        //Returns the mappa file that this game uses as its main list of dungeon for the main story, and not for rescue compatibility.
        eMappaID GetDefaultMappaID(eGameVersion version)const;

        //Obligatory typedefs for collections
        typedef mappa_container_t::iterator iterator;
        typedef mappa_container_t::const_iterator const_iterator;
        typedef DungeonRNGDataSet value_type;
        typedef eMappaID key_type;
        iterator begin();
        const_iterator begin()const;
        iterator end();
        const_iterator end()const;
        //Returns true if the DB contains no data sets!
        bool   empty()const;
        value_type & operator[](const key_type & key);

        void AddMappa(eMappaID id, DungeonRNGDataSet && dset)
        {
            m_dungeons_mappa.emplace(id, dset);
        }

        stats::FixedDungeonDB &         FixedFloorData() { return m_fixedData; }
        const stats::FixedDungeonDB &   FixedFloorData()const { return m_fixedData; }
        void FixedFloorData(const stats::FixedDungeonDB & fixed) { m_fixedData = fixed; }
        void FixedFloorData(stats::FixedDungeonDB && fixed) { m_fixedData = fixed; }

	private:
        //const ConfigLoader &        m_conf;
        //std::shared_ptr<GameText>   m_gametext;
        //std::string                 m_rootpath;
		stats::FixedDungeonDB       m_fixedData;

        //Dungeon data from the mappa files
        //Not all dungeon data is used in all games..
        mappa_container_t m_dungeons_mappa;
	};
}}
#endif