#ifndef PMD2_HCDATA_DUMPER_HPP
#define PMD2_HCDATA_DUMPER_HPP
/*
pmd2_hcdata_dumper.hpp
2016/08/22
psycommando@gmail.com
Description:    Utilities for dumping hard-coded data from the game into loose files.
                The data to be dumped may come from various sources, such as the utility's config data!

                This should be referred to by the ASM patcher.
*/
#include <ppmdu/pmd2/pmd2.hpp>
#include <ppmdu/pmd2/pmd2_configloader.hpp>
#include <ppmdu/fmts/sir0.hpp>
#include <utils/utility.hpp>
#include <vector>
#include <iterator>
#include <fstream>
#include <cstdint>

namespace pmd2
{

    /*
        HardCodedDataDumper
            This interface is meant to be used for dumping various hard-coded data tables from the PMD2 games.
            It can do so using several sources, such as the XML data file, or just raw data.
    */
    class HardCodedDataDumper
    {
    public:
        HardCodedDataDumper( const ConfigLoader & cfg )
            :m_conf(&cfg)
        {}

        template<class _EntryTy>
            struct WriteEntryType_impl
        {};

        template<>
            struct WriteEntryType_impl<pmd2::level_info>
        {
            template<class _outfwdit, class _sir0wrapper>
                static _outfwdit WriteEntry( const pmd2::level_info & inf, uint32_t strpointer, _sir0wrapper & wrap, _outfwdit itw )
            {
                //4 shorts, 1 string pointer
                itw = utils::WriteIntToBytes( inf.unk1,  itw );
                itw = utils::WriteIntToBytes( inf.unk2,  itw );
                itw = utils::WriteIntToBytes( inf.mapid, itw );
                itw = utils::WriteIntToBytes( inf.unk4,  itw );
                //Write string pointer
                wrap.pushpointer(strpointer);
                return itw;
            }
        };

        template<>
            struct WriteEntryType_impl<pmd2::livesent_info>
        {
            template<class _outfwdit, class _sir0wrapper>
                static _outfwdit WriteEntry( const pmd2::livesent_info & inf, uint32_t strpointer, _sir0wrapper & wrap, _outfwdit itw )
            {
                //2 shorts, 1 pointer, 2 shorts.
                itw = utils::WriteIntToBytes( inf.type,  itw );
                itw = utils::WriteIntToBytes( inf.entid, itw );
                //Write string pointer
                wrap.pushpointer(strpointer);
                itw = utils::WriteIntToBytes( inf.unk3,  itw );
                itw = utils::WriteIntToBytes( inf.unk4,  itw );
                return itw;
            }
        };


        /*
        */
        template<unsigned int _EntryLen, class _EntryType, class _infwdit, class _outfwdit>
            _outfwdit DumpStringAndList( _infwdit itbeg, _infwdit itend, _outfwdit itw, bool bputsubheader = false )const
        {
            using std::vector;
            using std::back_inserter;
            static_assert( std::is_same<_EntryType&, typename decltype(*itbeg)>::value ||
                           std::is_same<const _EntryType&, typename decltype(*itbeg)>::value, 
                           "HardCodedDataDumper::DumpStringAndList(): Iterators weren't iterators on expected type!!" );

            static const size_t entrysz = _EntryLen; //Size of an entry 

            // === 1st pass pile up strings in order ===
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

            // === 2nd pass, write the table entries ===
            uint32_t ptrdatatbl = outdata.size();

            size_t cntptr = 0;
            for( auto itentry = itbeg; itentry != itend; ++itentry, ++cntptr )
                WriteEntryType_impl<_EntryType>::WriteEntry(*itentry, stroffsets[cntptr], sir0anddat, itbackins);

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
        */
        template<class _infwdit, class _outfwdit>
            inline _outfwdit DumpActorList( _infwdit itbeg, _infwdit itend, _outfwdit itw )const
        {
            static const size_t entrysz = 12;
            return DumpStringAndList<entrysz, pmd2::livesent_info>(itbeg, itend, itw, true);
        }

        /*
        */

        void DumpActorDataFromConf( const std::string & outfpath )const 
        {
            try
            {
                std::ofstream out(outfpath, std::ios::out|std::ios::binary);
                out.exceptions(std::ios::badbit);
                DumpActorList( m_conf->GetGameScriptData().LivesEnt().begin(), 
                               m_conf->GetGameScriptData().LivesEnt().end(), 
                               std::ostreambuf_iterator<char>(out) );
            }
            catch(const std::exception &)
            {
                std::throw_with_nested( std::runtime_error("HardCodedDataDumper::DumpActorDataFromConf(): IO error writing to file " + outfpath) );
            }
        }


        /*
            DumpLevelList
                This dumps a range containing "level_info" structs into a sir0 wrapped representation of the data.
                    -itbeg : The beginning of the range.
                    -itend : The end of the range.
                    -itw   : The output iterator on values accepting assignement from bytes.
                Returns the new output position.
        */
        template<class _infwdit, class _outfwdit>
            inline _outfwdit DumpLevelList( _infwdit itbeg, _infwdit itend, _outfwdit itw )const
        {
            static const size_t entrysz = 12;
            return DumpStringAndList<entrysz, pmd2::level_info>(itbeg, itend, itw);
        }

        /*
        */
        void DumpLevelDataFromConf( const std::string & fpath )const 
        {
            try
            {
                std::ofstream out(fpath, std::ios::out|std::ios::binary);
                out.exceptions(std::ios::badbit);
                DumpLevelList( m_conf->GetGameScriptData().LevelInfo().begin(), 
                               m_conf->GetGameScriptData().LevelInfo().end(), 
                               std::ostreambuf_iterator<char>(out) );
            }
            catch(const std::exception &)
            {
                std::throw_with_nested( std::runtime_error("HardCodedDataDumper::DumpLevelDataFromConf(): IO error writing to file " + fpath) );
            }
        }

    private:

    private:
        const ConfigLoader * m_conf;
    };

};

#endif