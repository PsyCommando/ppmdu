#include "m_level.hpp"
#include <ppmdu/fmts/pack_file.hpp>
#include <ppmdu/fmts/pkdpx.hpp>
#include <ppmdu/fmts/at4px.hpp>
#include <ppmdu/fmts/sir0.hpp>
#include <iostream>

using namespace std;
using namespace pmd2::stats;

namespace filetypes 
{
//======================================================================================
//  Constants
//======================================================================================
    static const uint32_t MLevel_ForcedOffset_EoS = 0x1200; //The forced first file offset used in m_level.bin 

//======================================================================================
//  MLevelParser
//======================================================================================
    class MLevelParser
    {
    public:
        MLevelParser( const std::vector<uint8_t> & srcdata )
            :m_rawdata(srcdata)
        {}
        
        vector<PokeStatsGrowth> Parse()
        {
            CPack mypack;
            mypack.LoadPack( m_rawdata.begin(), m_rawdata.end() );
            vector<PokeStatsGrowth> pkmngrowth(mypack.getNbSubFiles());
            m_itcurpoke = pkmngrowth.begin();

            vector<uint8_t> decompBuf( PokeStatsGrowth::PkmnEntryLen );

            for( const auto & pokesubf : mypack.SubFiles() )
            {
                sir0_header hdr;
                hdr.ReadFromContainer( pokesubf.begin(), pokesubf.end() );

                //In Explorers of Time/Darkness some pokemon's level data is compressed
                // as AT4PX!
                array<uint8_t,5> magicn = {0};
                copy_n( pokesubf.begin() + hdr.subheaderptr, 5, magicn.begin() );

                //Check what the magic number is
                if( equal( magicn.begin(), magicn.end(), MagicNumber_AT4PX.begin() ) )
                {
                    DecompressAT4PX( (pokesubf.begin() + hdr.subheaderptr), 
                                     (pokesubf.begin() + hdr.ptrPtrOffsetLst), 
                                     decompBuf);
                }
                else
                {
                    DecompressPKDPX( (pokesubf.begin() + hdr.subheaderptr), 
                                     (pokesubf.begin() + hdr.ptrPtrOffsetLst), 
                                     decompBuf );
                }

                ParsePkmnEntry(decompBuf);
            }

            return std::move(pkmngrowth);
        }

    private:

        void ParsePkmnEntry( const vector<uint8_t> & data )
        {
            PokeStatsGrowth & curpoke = *m_itcurpoke;
            curpoke.statsgrowth.resize(PokeStatsGrowth::NbEntriesPerPkmn);
            auto itread = data.begin();


            for( auto & entry : curpoke.statsgrowth )
            {
                //Read Exp
                itread = utils::ReadIntFromBytes( entry.first,      itread, data.end() );
                //Read stats
                itread = utils::ReadIntFromBytes( entry.second.HP,  itread, data.end() );
                itread = utils::ReadIntFromBytes( entry.second.Atk, itread, data.end() );
                itread = utils::ReadIntFromBytes( entry.second.SpA, itread, data.end() );
                itread = utils::ReadIntFromBytes( entry.second.Def, itread, data.end() );
                itread = utils::ReadIntFromBytes( entry.second.SpD, itread, data.end() );
                std::advance( itread, sizeof(uint16_t) ); //Skip ending null bytes
            }

            ++m_itcurpoke;
        }

        const std::vector<uint8_t>      & m_rawdata;
        vector<PokeStatsGrowth>::iterator m_itcurpoke;
    };

//======================================================================================
//  MLevelWriter
//======================================================================================
    class MLevelWriter
    {
    public:
        MLevelWriter( const vector<PokeStatsGrowth> & data )
            :m_Pkmns(data)
        {}

        vector<uint8_t> Write()
        {
            if( m_Pkmns.empty() )
                throw std::runtime_error( "MLevelWriter::Write(): There are no entries to write! This should never happen!" );

            //
            vector<vector<uint8_t>> outBuff(m_Pkmns.size());

            //
            vector<uint8_t> swapbuff;
            vector<uint8_t> pokedatbuff(PokeStatsGrowth::PkmnEntryLen);

            //Make all entries
            for( unsigned int i = 0; i < m_Pkmns.size(); ++i )
            {
                WriteEntry( m_Pkmns[i], pokedatbuff );
                CompressToPKDPX( pokedatbuff.begin(), pokedatbuff.end(), swapbuff );
                outBuff[i] = std::move( MakeSIR0Wrap( swapbuff, 0xAA ) );
            }

            
            CPack myPack( std::move(outBuff) );
            //#FIXME: this might require a different offset for EoT/EoD
            myPack.setForceFirstFilePosition( MLevel_ForcedOffset_EoS );
            return myPack.OutputPack();
        }

    private:

        void WriteEntry( const PokeStatsGrowth & pkmnentry, vector<uint8_t> & buffer )
        {
            auto itat = buffer.begin();

            for( const auto & entry : pkmnentry.statsgrowth )
            {
                WriteVal( entry.first,      itat );
                WriteVal( entry.second.HP,  itat );
                WriteVal( entry.second.Atk, itat );
                WriteVal( entry.second.SpA, itat );
                WriteVal( entry.second.Def, itat );
                WriteVal( entry.second.SpD, itat );
                WriteVal( uint16_t(0),      itat );      //Entry ends with null uint16
            }
        }

        template<class T>
            inline void WriteVal( T val, vector<uint8_t>::iterator & itat )
        {
            itat = utils::WriteIntToBytes( val, itat );
        }

        const vector<PokeStatsGrowth> & m_Pkmns;
    };

//======================================================================================
//  Functions
//======================================================================================
    /*
    */
    std::vector<PokeStatsGrowth> & ParseLevelGrowthData( const std::string            & inpath, 
                                                         std::vector<PokeStatsGrowth> & out_pkmdat )
    {
        out_pkmdat = MLevelParser( utils::io::ReadFileToByteVector(inpath) ).Parse();
        return out_pkmdat;
    }

    std::vector<PokeStatsGrowth> ParseLevelGrowthData( const std::string & inpath )
    {
        return MLevelParser( utils::io::ReadFileToByteVector(inpath) ).Parse();
    }

    /*
    */
    void WriteLevelGrowthData( const std::vector<PokeStatsGrowth> & pkmdat, 
                               const std::string                  & outpath )
    {
        utils::io::WriteByteVectorToFile( outpath, MLevelWriter(pkmdat).Write() );
    }

};