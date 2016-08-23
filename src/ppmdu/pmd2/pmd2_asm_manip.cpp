#include "pmd2_asm_manip.hpp"
#include <vector>
#include <fstream>
#include <iomanip>
#include <iostream>
using namespace std;

namespace pmd2
{

    /*
        RawEntityDataEntry
    */
    struct RawEntityDataEntry
    {
        static const size_t LEN = 12;
        int16_t  type       = 0;
        int16_t  entityid   = 0;
        uint32_t ptrstring  = 0;
        uint16_t unk3       = 0;
        uint16_t unk4       = 0;

        template<typename _init>
            _init Read( _init itbeg, _init itend )
        {
            itbeg = utils::ReadIntFromBytes( type,         itbeg, itend );
            itbeg = utils::ReadIntFromBytes( entityid,     itbeg, itend );
            itbeg = utils::ReadIntFromBytes( ptrstring,    itbeg, itend );
            itbeg = utils::ReadIntFromBytes( unk3,         itbeg, itend );
            itbeg = utils::ReadIntFromBytes( unk4,         itbeg, itend );
            return itbeg;
        }

        template<typename _outit>
            _outit Write( _outit itwhere )
        {
            itwhere = utils::WriteIntToBytes( type,      itwhere );
            itwhere = utils::WriteIntToBytes( entityid,  itwhere );
            itwhere = utils::WriteIntToBytes( ptrstring, itwhere );
            itwhere = utils::WriteIntToBytes( unk3,      itwhere );
            itwhere = utils::WriteIntToBytes( unk4,      itwhere );
            return itwhere;
        }
    };

//=======================================================================================
//  
//=======================================================================================
    class PMD2_ASM_Impl
    {
    public:
        typedef std::vector<EntityDataEntry> entitydata_t;

        PMD2_ASM_Impl(const std::string & romroot, const GameBinariesInfo & info, eGameVersion gv, eGameRegion gr)
            :m_bininfo(info), m_romroot(romroot)
        {
        }

        void LoadTables()
        {
        }

    private:

        std::ifstream OpenFile( const binarylocatioinfo & locinfo, size_t & out_flen )
        {
            stringstream sstrpath;
            size_t fsize = 0;
            sstrpath << utils::TryAppendSlash(m_romroot) <<locinfo.fpath;

            ifstream indata(sstrpath.str(), ios::in | ios::binary | ios::ate );
            if( indata.bad() || !indata.is_open() )
                throw std::runtime_error("PMD2_ASM_Impl::LoadEntityData(): Couldn't open file !");

            fsize = indata.tellg();

            indata.seekg(0);
            indata.clear();
            if( indata.bad() )
                throw std::runtime_error("PMD2_ASM_Impl::LoadEntityData(): Can't read file !");

            return std::move(indata);
        }

        std::string FetchString( size_t strpos, size_t fsize, ifstream & ifstr )
        {
            string    str;
            streampos posbef = ifstr.tellg();
            if( strpos < fsize )
            {
                ifstr.seekg( strpos, ios::beg );
                str = std::move(utils::ReadCStrFromBytes( std::istreambuf_iterator<char>(ifstr), std::istreambuf_iterator<char>() ));
                ifstr.seekg( posbef, ios::beg );
            }
            else
            {
                clog << "<!>- PMD2_ASM_Impl::FetchString(): Warning: Got invalid string pointer!!!\n";
#ifdef _DEBUG
                assert(false);
#endif
            }

            return std::move(str);
        }

        void LoadEntityData()
        {
            binarylocatioinfo locinfo = m_bininfo.FindInfoByLocation( eBinaryLocations::Entities );
            size_t            fsize   = 0;

            if(!locinfo)
                throw std::runtime_error("PMD2_ASM_Impl::LoadEntityData(): Invalid entity data in the config file!");

            ifstream indata = std::move(OpenFile(locinfo,fsize));
            m_entdata.reserve( (locinfo.location.end - locinfo.location.beg) / RawEntityDataEntry::LEN );

            auto itentry = std::istreambuf_iterator<char>(indata);
            indata.seekg(locinfo.location.beg);
            for( auto & entry : m_entdata )
            {
                RawEntityDataEntry rawent;
                rawent.Read(std::istreambuf_iterator<char>(indata), std::istreambuf_iterator<char>());
                entry.entityid = rawent.entityid;
                entry.type     = rawent.type;
                entry.unk3     = rawent.unk3;
                entry.unk4     = rawent.unk4;
                entry.name     = FetchString( (rawent.ptrstring - locinfo.loadaddress), fsize, indata );
            }
        }

        void LoadStarters()
        {
            binarylocatioinfo heroids    = m_bininfo.FindInfoByLocation( eBinaryLocations::StartersHeroIds );
            binarylocatioinfo partnerids = m_bininfo.FindInfoByLocation( eBinaryLocations::StartersPartnerIds );
            binarylocatioinfo startstr   = m_bininfo.FindInfoByLocation( eBinaryLocations::StartersStrings );
            if( heroids && partnerids && startstr )
            {
                size_t            flen = 0;
                ifstream          inf  = std::move(OpenFile( heroids, flen ));

                //Load hero
                const size_t nbheroentries = (heroids.location.end  - heroids.location.beg)  / sizeof(uint16_t);
                const size_t nbstr         = (startstr.location.end - startstr.location.beg) / sizeof(uint16_t);

                if(nbstr != nbheroentries || nbheroentries % 2 != 0 || nbheroentries != static_cast<size_t>(eStarterNatures::NbNatures) )
                    throw std::runtime_error("PMD2_ASM_Impl::LoadStarters(): mismatch between number of starter strings id and starters OR nb of starters not divisible by 2!!");

                m_startertable.HeroEntries.reserve( nbheroentries );
                inf.seekg(heroids.location.beg);

                //Load Heroes
                for( size_t cntentry = 0; cntentry < nbheroentries; cntentry+=2 )
                {
                    StarterPKmList::StarterPkData cur;
                    cur.pokemon1 = utils::ReadIntFromBytes<uint16_t>( std::istreambuf_iterator<char>(inf), std::istreambuf_iterator<char>() );
                    cur.pokemon2 = utils::ReadIntFromBytes<uint16_t>( std::istreambuf_iterator<char>(inf), std::istreambuf_iterator<char>() );
                    streampos befpos = inf.tellg();

                    //Get string id
                    //size_t stroff = startstr.location.beg + (cntentry * 2);
                    //inf.seekg(stroff,ios::beg);
                    //cur.textidpkm1 = utils::ReadIntFromBytes<uint16_t>( std::istreambuf_iterator<char>(inf), std::istreambuf_iterator<char>() );
                    //cur.textidpkm2 = utils::ReadIntFromBytes<uint16_t>( std::istreambuf_iterator<char>(inf), std::istreambuf_iterator<char>() );
                    //inf.seekg(befpos,ios::beg);
                    m_startertable.HeroEntries.emplace( std::forward<eStarterNatures>(static_cast<eStarterNatures>(cntentry)), std::forward<StarterPKmList::StarterPkData>(cur) );
                }

                //Load Hero strings id
                inf.seekg(startstr.location.beg,ios::beg);
                for( size_t cntstr = 0; cntstr < nbstr; cntstr+=2 )
                {
                    StarterPKmList::StarterPkData & cur = m_startertable.HeroEntries.at( static_cast<eStarterNatures>(cntstr) );
                    cur.textidpkm1 = utils::ReadIntFromBytes<uint16_t>( std::istreambuf_iterator<char>(inf), std::istreambuf_iterator<char>() );
                    cur.textidpkm2 = utils::ReadIntFromBytes<uint16_t>( std::istreambuf_iterator<char>(inf), std::istreambuf_iterator<char>() );
                }

                //Load partners
                const size_t nbpartnerentries = (partnerids.location.end - partnerids.location.beg) / sizeof(uint16_t);
                m_startertable.PartnerEntries.resize(nbpartnerentries);
                inf.seekg(partnerids.location.beg,ios::beg);

                for( size_t cntptner = 0; cntptner < nbpartnerentries; ++cntptner )
                {
                    m_startertable.PartnerEntries[cntptner] = utils::ReadIntFromBytes<uint16_t>( std::istreambuf_iterator<char>(inf), std::istreambuf_iterator<char>() );
                }

            }
            else
                throw std::runtime_error("PMD2_ASM_Impl::LoadStarters(): Invalid starters data in the config file!");
        }

    private:
        StarterPKmList                  m_startertable;
        entitydata_t                    m_entdata;
        const GameBinariesInfo        & m_bininfo;
        eGameVersion                    m_version;
        eGameRegion                     m_region;
        std::string                     m_romroot;
    };


//=======================================================================================
//  PMD2_ASM_Manip
//=======================================================================================
    PMD2_ASM_Manip::PMD2_ASM_Manip( const std::string & romroot, const GameBinariesInfo & info, eGameVersion gv, eGameRegion gr )
        :m_pimpl(new PMD2_ASM_Impl(romroot, info, gv, gr))
    {}

    PMD2_ASM_Manip::~PMD2_ASM_Manip()
    {
    }

    PMD2_ASM_Manip::starterdata_t PMD2_ASM_Manip::FetchStartersTable()
    {
        assert(false);
        return starterdata_t();
    }

    void PMD2_ASM_Manip::ReplaceStartersTable(const starterdata_t & newstart)
    {
        assert(false);
    }

    void PMD2_ASM_Manip::Load()
    {

    }

    void PMD2_ASM_Manip::Write()
    {

    }
};