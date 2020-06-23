#include "fixed.hpp"
#include <ppmdu/fmts/sir0.hpp>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <iostream>
using namespace std;


namespace pmd2 { namespace filetypes 
{
//=======================================================================
//Parser
//=======================================================================
	//Parse the content of a fixed.bin file to a FixedDungeonDB object
    template<typename _init>
		class FixedBinParser
	{
	public:
		typedef _init input_iterator;

        FixedBinParser( input_iterator itbeg, input_iterator itend )
            :m_pathBalanceDir(pathBalanceDir), m_itbeg(itbeg), m_itcur(itbeg), m_itend(itend)
        {
			m_lookuptable.reserve(512); //Pre-alloc the vector
		}

		pmd2::stats::FixedDungeonDB Parse()
		{
			m_db = pmd2::stats::FixedDungeonDB(); //Make sure we got an object in a valid state if ran again after another use

			ParseHeader();
			ParseLuT();
			ParseEntries();
			return std::move(m_db);
		}

	private:

		void ParseHeader()
		{
			m_sir0hdr.ReadFromContainer(m_itbeg, m_itend);
		}

		void ParseLuT()
		{
			m_lookuptable.resize(0);
			uint32_t entryptr = 0;
			auto itcur = std::advance(m_itbeg, m_sir0hdr.subheaderptr);
			auto itLutEnd = std::advance(m_itbeg, m_sir0hdr.ptrPtrOffsetLst);

			while (itcur != itLutEnd && entryptr != 0xAAAAAAAA)
			{
				entryptr = utils::ReadIntFromBytes<uint32_t>(itcur, itLutEnd);
				if (entryptr != 0xAAAAAAAA)
					m_lookuptable.push_back(entryptr);
			}
		}

		void ParseEntries()
		{
			auto itFileBeg = m_itbeg;
			auto itLutBeg = std::advance(m_itbeg, m_sir0hdr.subheaderptr);

			for (size_t i = 0; i < m_lookuptable.size(); ++i)
			{
				size_t entryOffset = m_lookuptable[i];
				size_t endOfEntryOffset = (i + 1 < m_lookuptable.size()) ? m_lookuptable[i + 1] : m_sir0hdr.subheaderptr;
				ParseEntry(itFileBeg, itLutBeg, entryOffset, endOfEntryOffset)
			}
		}

		template<class _init>
			void ParseEntry(_init itFileBeg, _init itLutBeg, size_t entryOffset, size_t endOfEntryOffset)
		{
			auto itEntry = std::advance(itFileBeg, entryOffset);
			uint16_t width = utils::ReadIntFromBytes<uint16_t>(itEntry, itLutBeg);
			uint16_t height = utils::ReadIntFromBytes<uint16_t>(itEntry, itLutBeg);
			uint16_t unk1 = utils::ReadIntFromBytes<uint16_t>(itEntry, itLutBeg);
			m_db.AddEntry( width, height, unk1, DecodeLevelMap( itEntry, std::advance(itFileBeg, endOfEntryOffset), width, height) );
		}

		template<class _init>
			vector<uint8_t> DecodeLevelMap(_init itbeg, _init itend, uint16_t width, uint16_t height)
		{
			vector<uint8_t> result;
			auto itInsert = std::back_inserter(result);
			result.reserve(width * height);

			while (itbeg != itend)
			{
				uint8_t tileType = utils::ReadIntFromBytes<uint16_t>(itbeg, itend); //The type of tile for this sequence
				uint8_t nbtiles = utils::ReadIntFromBytes<uint16_t>(itbeg, itend); //Nb of tiles of this type in this sequence
				fill_n(itInsert, nbtiles, tileType);
			}
			return result;
		}

	private:
		_init m_itbeg;
		_init m_itend;
		string m_pathBalanceDir;
		vector<uint32_t> m_lookuptable;
		filetypes::sir0_header m_sir0hdr;
		pmd2::stats::FixedDungeonDB m_db;
	};


//=======================================================================
// Writer
//=======================================================================
	class FixedBinWriter
	{
	public:

		FixedBinWriter(const pmd2::stats::FixedDungeonDB & db)
			:m_db(db)
		{}

		template<class _outit>
			void Write(_outit & outit)
		{


			m_lut.resize(0);
			m_writebuffer.resize(0);
			auto itWrite = std::back_inserter(m_writebuffer);
			m_sir0wrap.data() = m_writebuffer;


			WriteEntries(itWrite);
			WriteLuT(itWrite);
			m_sir0wrap.Write(outit);
			return outit
		}

	private:
		template<class _outit>
			void WriteEntries(_outit & itwrite)
		{
			for (auto entry : m_db)
			{
				WriteEntry(itwrite, entry.width, entry.height, 0, entry.m_floormap);
			}
		}

		template<class _outit>
			void WriteEntry(_outit & itwrite, uint16_t width, uint16_t height, uint16_t unk1, const vector<uint8_t> & map)
		{
			m_lut.push_back(m_writebuffer.size());
			itwrite = utils::WriteIntToBytes(width, itwrite);
			itwrite = utils::WriteIntToBytes(height, itwrite);
			itwrite = utils::WriteIntToBytes(unk1, itwrite);
			WriteEncodeMap(itwrite, map);
		}

		void WriteLuT()
		{
			m_sir0wrap.SetDataPointerOffset(m_writebuffer.size()); //Make the header point at the right place
			for (uint32_t ptr : m_lut)
			{
				m_sir0wrap.pushpointer(ptr);
			}
		}

		template<class _outit>
			void WriteEncodeMap(_outit & itwrite, const vector<uint8_t> & map)
		{

			auto itRead = map.begin();
			auto itEnd = map.end();
			while (itRead != itEnd)
			{
				uint8_t curval = *itRead;
				size_t nbelems = std::count(itRead, itEnd, curval);
				itRead = std::advance(itRead, nbelems);
				itwrite = utils::WriteIntToBytes(curval, itwrite);
				itwrite = utils::WriteIntToBytes((uint8_t)nbelems, itwrite);
			}
		}

	private:
		string m_pathBalanceDir;
		vector<uint32_t> m_lut;
		vector<uint8_t> m_writebuffer;
		::filetypes::FixedSIR0DataWrapper<vector<uint8_t>> m_sir0wrap;
		const pmd2::stats::FixedDungeonDB & m_db;
	};

//
// Function Defnition
//
	pmd2::stats::FixedDungeonDB ParseFixedDungeonFloorData(const std::string & pathBalanceDir)
	{
		stringstream sspathfixed;
		sspathfixed << utils::TryAppendSlash(pathBalanceDir) << FixedDungeonData_FName;
		const string path = sspathfixed.str();

		if (!utils::isFile(path))
		{
			ostringstream sstrerr;
			sstrerr << "FixedBinParser::Parse(): Couldn't find the \"" << path << "\" file!";
			string strerr = sstrerr.str();
			clog << strerr << "\n";
			throw runtime_error(strerr);
		}
		vector<uint8_t> fdat = utils::io::ReadFileToByteVector(path);
		return FixedBinParser<vector<uint8_t>::const_iterator>(fdat.begin(), fdat.end()).Parse();
	}

	void WriteFixedDungeonFloorDataEoS(const std::string & pathBalanceDir, const stats::FixedDungeonDB & floordat)
	{
		stringstream sspathfixed;
		sspathfixed << utils::TryAppendSlash(pathBalanceDir) << FixedDungeonData_FName;
		const string path = sspathfixed.str();

		ofstream fixedbin(path, std::ios::binary);
		FixedBinWriter(floordat).Write(ostream_iterator<uint8_t>(fixedbin));
	}
}}