#include "pmd2_asm.hpp"
#include <utils/utility.hpp>
#include <utils/gbyteutils.hpp>
#include <ppmdu/pmd2/pmd2_hcdata.hpp>
#include <ppmdu/fmts/sir0.hpp>
#include <sstream>
#include <vector>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
using namespace std;
using namespace utils;

namespace pmd2
{
    const std::string FName_ARM9Bin             = "arm9.bin";
    const std::string ASM_ModdedTag             = "PATCH";
    const int         ASM_ModdedTaDescgMaxLen   = 4096; 

//=======================================================================================
//  Exceptions
//=======================================================================================

    ExBinaryIsModded::ExBinaryIsModded(const std::string & msg, const std::string & binpath, uint32_t binoff)
        :std::runtime_error(msg),m_binoff(binoff),m_binpath(binpath)
    {}

//=======================================================================================
//  Implementation
//=======================================================================================
    class PMD2_ASM_Impl
    {
    public:
        /*
            PMD2_ASM_Impl
        */
        PMD2_ASM_Impl(const std::string & romroot, ConfigLoader & conf)
            :m_conf(conf), m_romroot(romroot)
        {}

        /*
            MakeBinPathString
                Assemble the path to a binary within the current rom's directory structure.
        */
        std::string MakeBinPathString(const binarylocatioinfo & info)const
        {
            stringstream binpath;
            binpath <<utils::TryAppendSlash(m_romroot) <<info.fpath;
            return std::move(binpath.str());
        }

        /*
            MakeLooseBinFileOutPath
                Assemble the path to a binary loose file within the rom's directory structure.
        */
        std::string MakeLooseBinFileOutPath( eBinaryLocations loc )
        {
            auto found = m_conf.GetASMPatchData().lfentry.find(loc);

            if(found ==  m_conf.GetASMPatchData().lfentry.end())
                throw std::runtime_error("PMD2_ASM_Impl::MakeLooseBinFileOutPath(): Couldn't find the entity file output path for the actor list!");

            stringstream outfpath;
            outfpath <<TryAppendSlash(m_romroot) <<DirName_DefData <<"/" <<found->second.path ;
            return std::move(outfpath.str());
        }

        PMD2_ASM::modinfo CheckBlockModdedTag(const binarylocatioinfo & locinfo)
        {
            ifstream binf;
            OpenBinFile(binf,locinfo);
            return CheckBlockModdedTag(binf, locinfo);
        }

        /*
            CheckBlockModdedTag
                Verifies if the PATCH tag is at an expected bin location offset within a binary.
        */
        PMD2_ASM::modinfo CheckBlockModdedTag(istream & binf, const binarylocatioinfo & locinfo)
        {
            PMD2_ASM::modinfo minfo;
            //stringstream binpath;
            //binpath <<utils::TryAppendSlash(m_romroot) <<binppathrel;
            try
            {
                //ifstream binf( binpath.str(), ios::in | ios::binary );
                binf.seekg(locinfo.location.beg);
                std::istreambuf_iterator<char> init(binf);
                std::istreambuf_iterator<char> initend;
                if( std::equal( ASM_ModdedTag.begin(), ASM_ModdedTag.end(), init ) )
                {
                    //Read the mod tag for at least 4096 bytes or until we hit a terminating 0 or reach the end of the file!
                    for( size_t cnt = 0; cnt < ASM_ModdedTaDescgMaxLen && init != initend; ++cnt )
                    {
                        char ch = *init;
                        ++init;
                        if(ch == 0)
                            break;
                        minfo.modstring.push_back(ch);
                    }
                }
            }
            catch(const std::exception&)
            {
                throw_with_nested( std::runtime_error("PMD2_ASM_Impl::CheckBlockModdedTag() : Failure while reading file " + locinfo.fpath + "!") );
            }
            
            return std::move(minfo);
        }

        /*
            LoadData
        */
        template<class _DataTy, class _TransType>
            void LoadData( eBinaryLocations binloc, _DataTy & out_data)
        {
            const binarylocatioinfo bininfo = m_conf.GetGameBinaryOffset(binloc);
            ifstream                binf;
            OpenBinFile(binf, bininfo);

            //Check if modded tag is there, then load from the correct source!
            PMD2_ASM::modinfo modinfo = CheckBlockModdedTag( binf, bininfo );
            binf.seekg(0);
            binf.clear();

            if(modinfo.ismodded())
            {
                ifstream loosebin;
                OpenLooseBinFile(loosebin,binloc);
                LoadDataFromLooseBin<_DataTy,_TransType>(out_data, loosebin); //Open lose file
            }
            else
                LoadDataFromBin<_DataTy,_TransType>(out_data, bininfo, binf);
        }

        /*
            LoadDataFromLooseBin
        */
        template<class _DataTy, class _TransType>
            void LoadDataFromLooseBin( _DataTy & out_data, std::ifstream & binstrm )
        {
            filetypes::sir0_header hdr;
            hdr.ReadFromContainer( istreambuf_iterator<char>(binstrm), istreambuf_iterator<char>() );

            if(hdr.magic != filetypes::MagicNumber_SIR0)
                throw std::runtime_error( "PMD2_ASM_Impl::LoadDataFromLooseBin(): File is not a SIR0 container!" );
            
            //Move to the beginning of the list
            binstrm.seekg(hdr.subheaderptr);
            uint32_t ptrtbl    = utils::ReadIntFromBytes<uint32_t>( istreambuf_iterator<char>(binstrm), istreambuf_iterator<char>() );
            uint32_t nbentries = utils::ReadIntFromBytes<uint32_t>( istreambuf_iterator<char>(binstrm), istreambuf_iterator<char>() );
            binstrm.seekg(ptrtbl);

            //Read each entries
            for( size_t cntentry = 0; cntentry < nbentries; ++cntentry )
            {
                _TransType entry;
                entry.Read(binstrm, 0); //No offset applied to what is loaded from loose binaries!!
                out_data.PushEntryPair(entry.name, entry);
            }
        }

        /*
            LoadDataFromBin
        */
        template<class _DataTy, class _TransType>
            void LoadDataFromBin( _DataTy & out_data, const binarylocatioinfo & bininfo, std::ifstream & binstrm )
        {
            //2. If no modded tag, we go ahead and dump the list
            binstrm.seekg(bininfo.location.beg);

            while(static_cast<std::streamoff>(binstrm.tellg()) < bininfo.location.end)
            {
                _TransType entry;
                entry.Read(binstrm, bininfo.loadaddress);
                out_data.PushEntryPair(entry.name, entry);
            }
        }

        /*
            DumpStringAndList
                Generic function for writing a SIR0 file containing a consecutive list of entries, and a consecutive 
                block of null terminated c strings referred to by each entry.
        */
        template<class _EntryType, class _TransType, class _infwdit, class _outfwdit>
            _outfwdit DumpStringAndList( _infwdit itbeg, _infwdit itend, _outfwdit itw, bool bputsubheader = true )const
        {
            using std::vector;
            using std::back_inserter;
            static_assert( std::is_same<_EntryType&, typename decltype(*itbeg)>::value ||
                           std::is_same<const _EntryType&, typename decltype(*itbeg)>::value, 
                           "PMD2_ASM_Impl::DumpStringAndList(): Iterators weren't iterators on expected type!!" );

            static const size_t entrysz = _TransType::Size; //Size of an entry 

            // -----------------------------------------
            // === 1st pass pile up strings in order ===
            // -----------------------------------------
            ::filetypes::FixedSIR0DataWrapper<std::vector<uint8_t>> sir0anddat;
            const size_t                     nbentries = std::distance(itbeg, itend);
            vector<uint8_t>                & outdata   = sir0anddat.Data();
            vector<uint32_t>                 stroffsets;
            stroffsets.reserve(nbentries);
            sir0anddat.Data().reserve((nbentries * 8) + (nbentries * entrysz));

            auto itbackins = std::back_inserter(sir0anddat.Data());

            for( auto itstr = itbeg; itstr != itend; ++itstr)
            {
                const _EntryType & inf = *itstr;
                stroffsets.push_back(outdata.size());
                std::copy( inf.name.begin(), inf.name.end(), itbackins );
                outdata.push_back(0);
                utils::AppendPaddingBytes( itbackins, outdata.size(), 4, 0 );
            }
            utils::AppendPaddingBytes(itbackins, outdata.size(), 16, 0); //Pad the strings, because I'm a perfectionist

            // -----------------------------------------
            // === 2nd pass, write the table entries ===
            // -----------------------------------------
            uint32_t ptrdatatbl = outdata.size();

            size_t cntptr = 0;
            for( auto itentry = itbeg; itentry != itend; ++itentry, ++cntptr )
            {
                //WriteEntryType_impl<_EntryType>::WriteEntry(*itentry, stroffsets[cntptr], sir0anddat, itbackins);
                _TransType trans;
                trans = (*itentry);
                trans.WriteEntry(stroffsets[cntptr], sir0anddat, itbackins);
            }

            utils::AppendPaddingBytes(itbackins, outdata.size(), 16, 0xAA);

            if(bputsubheader)
            {
                sir0anddat.SetDataPointerOffset(outdata.size());
                //Append a subheader
                sir0anddat.pushpointer(ptrdatatbl);
                itbackins = utils::WriteIntToBytes( static_cast<uint32_t>(nbentries),  itbackins );
                utils::AppendPaddingBytes(itbackins, outdata.size(), 16, 0xAA);
            }
            else
                sir0anddat.SetDataPointerOffset(ptrdatatbl);

            //Then write out!
            sir0anddat.Write(itw);
            return itw; 
        }

        /*
            LoadLevelList
        */
        GameScriptData::lvlinf_t LoadLevelList()
        {
            GameScriptData::lvlinf_t data;
            LoadData<GameScriptData::lvlinf_t, LevelEntry>(eBinaryLocations::Events, data);
            return std::move(data);
        }

        /*
            LoadActorList
        */
        GameScriptData::livesent_t LoadActorList()
        {
            GameScriptData::livesent_t data;
            LoadData<GameScriptData::livesent_t, EntitySymbolListEntry>(eBinaryLocations::Entities, data);
            return std::move(data);
        }

        /*
            LoadObjectList
        */
        GameScriptData::objinf_t LoadObjectList()
        {
            GameScriptData::objinf_t data;
            LoadData<GameScriptData::objinf_t, ObjectFileListEntry>(eBinaryLocations::Objects, data);
            return std::move(data);
        }

        /*
            LoadGameVariableList
        */
        GameScriptData::gvar_t LoadGameVariableList()
        {
            GameScriptData::gvar_t data;
            LoadData<GameScriptData::gvar_t, ScriptVariablesEntry>(eBinaryLocations::ScriptVariables, data);
            return std::move(data);
        }

        /*
            Write the data to a loose file at the locations indicated in the configuration files, since there's no way to guarantee any changes would fit into the initial binaries.

        */
        void WriteLevelList( const GameScriptData::lvlinf_t & src )
        {
            const string outfpath = MakeLooseBinFileOutPath(eBinaryLocations::Events);
            try
            {
                std::ofstream out(outfpath, std::ios::out|std::ios::binary);
                out.exceptions(std::ios::badbit);
                DumpStringAndList<pmd2::level_info,LevelEntry>(src.begin(), src.end(), std::ostreambuf_iterator<char>(out) );
            }
            catch(const std::exception &)
            {
                std::throw_with_nested( std::runtime_error("PMD2_ASM_Impl::WriteLevelList(): IO error writing to file " + outfpath) );
            }
        }

        /*
        */
        void WriteActorList( const GameScriptData::livesent_t & src )
        {
            const string outfpath = MakeLooseBinFileOutPath(eBinaryLocations::Entities);
            try
            {
                std::ofstream out(outfpath, std::ios::out|std::ios::binary);
                out.exceptions(std::ios::badbit);
                DumpStringAndList<pmd2::livesent_info, EntitySymbolListEntry>(src.begin(), src.end(), std::ostreambuf_iterator<char>(out) );
            }
            catch(const std::exception &)
            {
                std::throw_with_nested( std::runtime_error("PMD2_ASM_Impl::WriteActorList(): IO error writing to file " + outfpath) );
            }
            
        }

        /*
        */
        void WriteObjectList( const GameScriptData::objinf_t & src )
        {
            const string outfpath = MakeLooseBinFileOutPath(eBinaryLocations::Objects);
            try
            {
                std::ofstream out(outfpath, std::ios::out|std::ios::binary);
                out.exceptions(std::ios::badbit);
                DumpStringAndList<pmd2::object_info, ObjectFileListEntry>(src.begin(), src.end(), std::ostreambuf_iterator<char>(out) );
            }
            catch(const std::exception &)
            {
                std::throw_with_nested( std::runtime_error("PMD2_ASM_Impl::WriteObjectList(): IO error writing to file " + outfpath) );
            }
        }

        /*
        */
        void WriteGameVariableList( const GameScriptData::gvar_t & src )
        {
            const string outfpath = MakeLooseBinFileOutPath(eBinaryLocations::ScriptVariables);
            try
            {
                std::ofstream out(outfpath, std::ios::out|std::ios::binary);
                out.exceptions(std::ios::badbit);
                DumpStringAndList<pmd2::gamevariable_info, ScriptVariablesEntry>(src.begin(), src.end(), std::ostreambuf_iterator<char>(out) );
            }
            catch(const std::exception &)
            {
                std::throw_with_nested( std::runtime_error("PMD2_ASM_Impl::WriteGameVariableList(): IO error writing to file " + outfpath) );
            }
        }

        /*
            LoadAllToConfig
                Replaces what was currently loaded as script data with what was loaded from the 
                binaries.
        */
        void LoadAllToConfig()
        {
            m_conf.GetGameScriptData().LevelInfo()      = LoadLevelList();
            m_conf.GetGameScriptData().LivesEnt()       = LoadActorList();
            m_conf.GetGameScriptData().ObjectsInfo()    = LoadObjectList();
            m_conf.GetGameScriptData().GameVariables()  = LoadGameVariableList();
        }

        /*
            WriteAllFromConfig
                Writes the hardcoded data parsed from the config file, into loose files.
        */
        void WriteAllFromConfig()
        {
            WriteLevelList(m_conf.GetGameScriptData().LevelInfo());
            WriteActorList(m_conf.GetGameScriptData().LivesEnt());
            WriteObjectList(m_conf.GetGameScriptData().ObjectsInfo());
            WriteGameVariableList(m_conf.GetGameScriptData().GameVariables());
        }


    private:

        /*
        */
        void OpenBinFile( std::ifstream & out_strm, const binarylocatioinfo & locinfo)
        {
            const std::string binpath = MakeBinPathString(locinfo);
            out_strm.open(binpath, ios::in | ios::binary );
            if( out_strm.bad() || !out_strm.is_open() )
                throw std::runtime_error("PMD2_ASM_Impl::OpenBinFile(): Couldn't open file " + binpath + "!");
            out_strm.exceptions(ios::badbit);
        }

        /*
        */
        void OpenLooseBinFile( std::ifstream & out_strm, eBinaryLocations loc )
        {
            const string binpath = MakeLooseBinFileOutPath(loc);
            out_strm.open(binpath, ios::in | ios::binary );
            if( out_strm.bad() || !out_strm.is_open() )
                throw std::runtime_error("PMD2_ASM_Impl::OpenLooseBinFile(): Couldn't open file " + binpath + " !");
            out_strm.exceptions(ios::badbit);
        }

//
//        std::string FetchString( size_t strpos, size_t fsize, ifstream & ifstr )
//        {
//            string    str;
//            streampos posbef = ifstr.tellg();
//            if( strpos < fsize )
//            {
//                ifstr.seekg( strpos, ios::beg );
//                str = std::move(utils::ReadCStrFromBytes( std::istreambuf_iterator<char>(ifstr), std::istreambuf_iterator<char>() ));
//                ifstr.seekg( posbef, ios::beg );
//            }
//            else
//            {
//                clog << "<!>- PMD2_ASM_Impl::FetchString(): Warning: Got invalid string pointer!!!\n";
//#ifdef _DEBUG
//                assert(false);
//#endif
//            }
//
//            return std::move(str);
//        }
//
//        void LoadEntityData()
//        {
//            binarylocatioinfo locinfo = m_bininfo.FindInfoByLocation( eBinaryLocations::Entities );
//            size_t            fsize   = 0;
//
//            if(!locinfo)
//                throw std::runtime_error("PMD2_ASM_Impl::LoadEntityData(): Invalid entity data in the config file!");
//
//            ifstream indata = std::move(OpenFile(locinfo,fsize));
//            m_entdata.reserve( (locinfo.location.end - locinfo.location.beg) / RawEntityDataEntry::LEN );
//
//            auto itentry = std::istreambuf_iterator<char>(indata);
//            indata.seekg(locinfo.location.beg);
//            for( auto & entry : m_entdata )
//            {
//                RawEntityDataEntry rawent;
//                rawent.Read(std::istreambuf_iterator<char>(indata), std::istreambuf_iterator<char>());
//                entry.entityid = rawent.entityid;
//                entry.type     = rawent.type;
//                entry.unk3     = rawent.unk3;
//                entry.unk4     = rawent.unk4;
//                entry.name     = FetchString( (rawent.ptrstring - locinfo.loadaddress), fsize, indata );
//            }
//        }
//
//        void LoadStarters()
//        {
//            binarylocatioinfo heroids    = m_bininfo.FindInfoByLocation( eBinaryLocations::StartersHeroIds );
//            binarylocatioinfo partnerids = m_bininfo.FindInfoByLocation( eBinaryLocations::StartersPartnerIds );
//            binarylocatioinfo startstr   = m_bininfo.FindInfoByLocation( eBinaryLocations::StartersStrings );
//            if( heroids && partnerids && startstr )
//            {
//                size_t            flen = 0;
//                ifstream          inf  = std::move(OpenFile( heroids, flen ));
//
//                //Load hero
//                const size_t nbheroentries = (heroids.location.end  - heroids.location.beg)  / sizeof(uint16_t);
//                const size_t nbstr         = (startstr.location.end - startstr.location.beg) / sizeof(uint16_t);
//
//                if(nbstr != nbheroentries || nbheroentries % 2 != 0 || nbheroentries != static_cast<size_t>(eStarterNatures::NbNatures) )
//                    throw std::runtime_error("PMD2_ASM_Impl::LoadStarters(): mismatch between number of starter strings id and starters OR nb of starters not divisible by 2!!");
//
//                m_startertable.HeroEntries.reserve( nbheroentries );
//                inf.seekg(heroids.location.beg);
//
//                //Load Heroes
//                for( size_t cntentry = 0; cntentry < nbheroentries; cntentry+=2 )
//                {
//                    StarterPKmList::StarterPkData cur;
//                    cur.pokemon1 = utils::ReadIntFromBytes<uint16_t>( std::istreambuf_iterator<char>(inf), std::istreambuf_iterator<char>() );
//                    cur.pokemon2 = utils::ReadIntFromBytes<uint16_t>( std::istreambuf_iterator<char>(inf), std::istreambuf_iterator<char>() );
//                    streampos befpos = inf.tellg();
//
//                    //Get string id
//                    //size_t stroff = startstr.location.beg + (cntentry * 2);
//                    //inf.seekg(stroff,ios::beg);
//                    //cur.textidpkm1 = utils::ReadIntFromBytes<uint16_t>( std::istreambuf_iterator<char>(inf), std::istreambuf_iterator<char>() );
//                    //cur.textidpkm2 = utils::ReadIntFromBytes<uint16_t>( std::istreambuf_iterator<char>(inf), std::istreambuf_iterator<char>() );
//                    //inf.seekg(befpos,ios::beg);
//                    m_startertable.HeroEntries.emplace( std::forward<eStarterNatures>(static_cast<eStarterNatures>(cntentry)), std::forward<StarterPKmList::StarterPkData>(cur) );
//                }
//
//                //Load Hero strings id
//                inf.seekg(startstr.location.beg,ios::beg);
//                for( size_t cntstr = 0; cntstr < nbstr; cntstr+=2 )
//                {
//                    StarterPKmList::StarterPkData & cur = m_startertable.HeroEntries.at( static_cast<eStarterNatures>(cntstr) );
//                    cur.textidpkm1 = utils::ReadIntFromBytes<uint16_t>( std::istreambuf_iterator<char>(inf), std::istreambuf_iterator<char>() );
//                    cur.textidpkm2 = utils::ReadIntFromBytes<uint16_t>( std::istreambuf_iterator<char>(inf), std::istreambuf_iterator<char>() );
//                }
//
//                //Load partners
//                const size_t nbpartnerentries = (partnerids.location.end - partnerids.location.beg) / sizeof(uint16_t);
//                m_startertable.PartnerEntries.resize(nbpartnerentries);
//                inf.seekg(partnerids.location.beg,ios::beg);
//
//                for( size_t cntptner = 0; cntptner < nbpartnerentries; ++cntptner )
//                {
//                    m_startertable.PartnerEntries[cntptner] = utils::ReadIntFromBytes<uint16_t>( std::istreambuf_iterator<char>(inf), std::istreambuf_iterator<char>() );
//                }
//
//            }
//            else
//                throw std::runtime_error("PMD2_ASM_Impl::LoadStarters(): Invalid starters data in the config file!");
//        }



    private:
        ConfigLoader & m_conf;
        eGameVersion   m_version;
        eGameRegion    m_region;
        std::string    m_romroot;
    };


//=======================================================================================
//  PMD2_ASM_Manip
//=======================================================================================
    PMD2_ASM::PMD2_ASM( const std::string & romroot, ConfigLoader & conf )
        :m_pimpl(new PMD2_ASM_Impl(romroot, conf))
    {}

    PMD2_ASM::~PMD2_ASM()
    {
    }

    GameScriptData::lvlinf_t PMD2_ASM::LoadLevelList()
    {
        return m_pimpl->LoadLevelList();
    }

    GameScriptData::livesent_t PMD2_ASM::LoadActorList()
    {
        //assert(false);
        return m_pimpl->LoadActorList();
    }

    GameScriptData::objinf_t PMD2_ASM::LoadObjectList()
    {
        //assert(false);
        return m_pimpl->LoadObjectList();
    }

    GameScriptData::gvar_t PMD2_ASM::LoadGameVariableList()
    {
        //assert(false);
        return m_pimpl->LoadGameVariableList();
    }

    void PMD2_ASM::WriteLevelList(const GameScriptData::lvlinf_t & src)
    {
        m_pimpl->WriteLevelList(src);
    }

    void PMD2_ASM::WriteActorList(const GameScriptData::livesent_t & src)
    {
        m_pimpl->WriteActorList(src);
    }

    void PMD2_ASM::WriteObjectList(const GameScriptData::objinf_t & src)
    {
        m_pimpl->WriteObjectList(src);
    }

    void PMD2_ASM::WriteGameVariableList(const GameScriptData::gvar_t & src)
    {
        m_pimpl->WriteGameVariableList(src);
    }

    void PMD2_ASM::LoadAllToConfig()
    {
        m_pimpl->LoadAllToConfig();
    }

    void PMD2_ASM::WriteAllFromConfig()
    {
        m_pimpl->WriteAllFromConfig();
    }

    PMD2_ASM::modinfo PMD2_ASM::CheckBlockModdedTag(const binarylocatioinfo & locinfo)
    {
        return m_pimpl->CheckBlockModdedTag(locinfo);
    }
};