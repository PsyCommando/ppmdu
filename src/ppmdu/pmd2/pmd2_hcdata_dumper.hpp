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

        /*
            DumpLevelList
                This dumps a range containing "level_info" structs into a sir0 wrapped representation of the data.
                    -itbeg : The beginning of the range.
                    -itend : The end of the range.
                    -itw   : The output iterator on values accepting assignement from bytes.
                Returns the new output position.
        */
        template<class _infwdit, class _outfwdit>
            _outfwdit DumpLevelList( _infwdit itbeg, _infwdit itend, _outfwdit itw )const
        {
            using std::vector;
            using std::back_inserter;
            static_assert( std::is_same<pmd2::level_info&, typename decltype(*itbeg)>::value ||
                           std::is_same<const pmd2::level_info&, typename decltype(*itbeg)>::value, 
                           "HardCodedDataDumper::DumpLevelList(): Iterators weren't iterators on level_info!!" );

            static const size_t entrysz = 12; //Size of an entry 

            //1st pass pile up strings in order
            ::filetypes::FixedSIR0DataWrapper<std::vector<uint8_t>> sir0anddat;
            const size_t                     nbentries = std::distance(itbeg, itend);
            vector<uint8_t>                & outdata   = sir0anddat.Data();
            vector<uint32_t>                 stroffsets;
            stroffsets.reserve(nbentries);
            sir0anddat.Data().reserve((nbentries * 8) + (nbentries * entrysz));

            auto itbackins = std::back_inserter(sir0anddat.Data());

            for( auto itstr = itbeg; itstr != itend; ++itstr)
            {
                const pmd2::level_info & inf = *itstr;
                stroffsets.push_back(outdata.size());
                std::copy( inf.name.begin(), inf.name.end(), itbackins );
                outdata.push_back(0);
                utils::AppendPaddingBytes( itbackins, outdata.size(), 4, 0 );
            }
            utils::AppendPaddingBytes(itbackins, outdata.size(), 16, 0); //Pad the strings, because I'm a perfectionist

            //2nd pass, write the table entries
            sir0anddat.SetDataPointerOffset(outdata.size());

            size_t cntptr = 0;
            for( auto itentry = itbeg; itentry != itend; ++itentry, ++cntptr )
            {
                const pmd2::level_info & inf = *itentry;
                //4 shorts, 1 string pointer
                utils::WriteIntToBytes( inf.unk1,  itbackins );
                utils::WriteIntToBytes( inf.unk2,  itbackins );
                utils::WriteIntToBytes( inf.mapid, itbackins );
                utils::WriteIntToBytes( inf.unk4,  itbackins );

                //Write string pointer
                sir0anddat.pushpointer(stroffsets[cntptr]);
            }

            utils::AppendPaddingBytes(itbackins, outdata.size(), 16, 0xAA);

            //Then write out!
            //ofstream out("level_list.bin",ios::out|ios::binary);
            sir0anddat.Write(itw);
            return itw;
        }

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