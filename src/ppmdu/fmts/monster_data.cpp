#include "monster_data.hpp"
#include <ppmdu/containers/pokemon_stats.hpp>
#include <vector>
#include <cstdint>
#include <string>
#include <iostream>
#include <sstream>
using namespace std;
using namespace pmd2::stats;

namespace pmd2 {namespace filetypes 
{
    static const unsigned int MonsterMd_PkmEntryLen = 0x44; //68 bytes

//==========================================================================================
//  MonsterMdParser
//==========================================================================================

    class MonsterMdParser
    {
    public:
        MonsterMdParser( vector<uint8_t> && rawdata )
            :m_pPkmns(nullptr), m_rawdata(rawdata)
        {}

        void Parse( std::vector<stats::PokeMonsterData> & out_pkmns )
        {
            m_itReadPos = m_rawdata.begin();
            m_pPkmns    = &out_pkmns;
            //
            ParseHeader();
            //
            ParseEntries();
            m_pPkmns = nullptr;
        }

    private:

        void ParseHeader()
        {
            m_itReadPos = m_header.ReadFromContainer( m_itReadPos );

            if( !m_header.isValid() )
            {
                stringstream sstr;
                sstr << "ERROR: The monster.md file to parse has an incorrect header!";
                throw std::runtime_error(sstr.str());
            }
        }

        void ParseEntries()
        {
            const uint32_t NbEntries = m_header.nbentries;
            m_pPkmns->reserve(NbEntries);

            for( unsigned int i = 0; (i < NbEntries) && (m_itReadPos != m_rawdata.end()); ++i )
                m_pPkmns->push_back( ParseAnEntry() );
        }

        stats::PokeMonsterData ParseAnEntry()
        {
            PokeMonsterData md;

            ParseVal( md.pokeID );
            ParseVal( md.mdunk31 );
            ParseVal( md.natPkdexNb );
            ParseVal( md.mdunk1 );
            ParseVal( md.evoData.preEvoIndex );
            ParseVal( md.evoData.evoMethod );
            ParseVal( md.evoData.evoParam1 );
            ParseVal( md.evoData.evoParam2 );
            ParseVal( md.spriteIndex );
            ParseVal( md.gender );
            ParseVal( md.bodySize );
            ParseVal( md.primaryTy );
            ParseVal( md.secondaryTy );
            ParseVal( md.moveTy );
            ParseVal( md.IQGrp );
            ParseVal( md.primAbility );
            ParseVal( md.secAbility );
            ParseVal( md.bitflags1 );
            ParseVal( md.expYield );
            ParseVal( md.recruitRate1 );
            ParseVal( md.baseHP );
            ParseVal( md.recruitRate2 );
            ParseVal( md.baseAtk );
            ParseVal( md.baseSpAtk );
            ParseVal( md.baseDef );
            ParseVal( md.baseSpDef );
            ParseVal( md.weight );
            ParseVal( md.size );
            ParseVal( md.mdunk17 );
            ParseVal( md.mdunk18 );
            ParseVal( md.mdunk19 );
            ParseVal( md.mdunk20 );
            ParseVal( md.mdunk21 );
            ParseVal( md.BasePkmn );

            ParseVal( md.exclusiveItems[0] );
            ParseVal( md.exclusiveItems[1] );
            ParseVal( md.exclusiveItems[2] );
            ParseVal( md.exclusiveItems[3] );

            ParseVal( md.unk27 );
            ParseVal( md.unk28 );
            ParseVal( md.unk29 );
            ParseVal( md.unk30 );

            return std::move(md);
        }

        template<class T>
            inline void ParseVal( T & var )
        {
            var = utils::ReadIntFromByteVector<T>(m_itReadPos);
        }

    private:
        vector<uint8_t>::const_iterator m_itReadPos;
        monstermd_header                m_header;
        vector<uint8_t>                 m_rawdata;
        std::vector<stats::PokeMonsterData>  * m_pPkmns;
    };

//==========================================================================================
//  MonsterMdWriter
//==========================================================================================

    class MonsterMdWriter
    {
    public:
        MonsterMdWriter( const std::vector<stats::PokeMonsterData> & data )
            :m_Pkmns(data)
        {}

        vector<uint8_t> Write()
        {
            uint32_t filelength = ( monstermd_header::HEADER_LEN + (m_Pkmns.size() * PokeMonsterData::DataLen) );
            vector<uint8_t> outbuff( filelength );
            m_itWritePos = outbuff.begin();

            WriteHeader();
            WriteEntries();

            return std::move(outbuff);
        }

    private:
        
        void WriteHeader()
        {
            monstermd_header hdr;
            hdr.magicn    = monstermd_header::MAGIC_NUMBER;
            hdr.nbentries = m_Pkmns.size();
            m_itWritePos  = hdr.WriteToContainer( m_itWritePos );
        }

        void WriteEntries()
        {
            for( const auto & entry : m_Pkmns )
                WriteEntry(entry);
        }

        void WriteEntry( const stats::PokeMonsterData & md )
        {
            WriteVal( md.pokeID );
            WriteVal( md.mdunk31 );
            WriteVal( md.natPkdexNb );
            WriteVal( md.mdunk1 );
            WriteVal( md.evoData.preEvoIndex );
            WriteVal( md.evoData.evoMethod );
            WriteVal( md.evoData.evoParam1 );
            WriteVal( md.evoData.evoParam2 );
            WriteVal( md.spriteIndex );
            WriteVal( md.gender );
            WriteVal( md.bodySize );
            WriteVal( md.primaryTy );
            WriteVal( md.secondaryTy );
            WriteVal( md.moveTy );
            WriteVal( md.IQGrp );
            WriteVal( md.primAbility );
            WriteVal( md.secAbility );
            WriteVal( md.bitflags1 );
            WriteVal( md.expYield );
            WriteVal( md.recruitRate1 );
            WriteVal( md.baseHP );
            WriteVal( md.recruitRate2 );
            WriteVal( md.baseAtk );
            WriteVal( md.baseSpAtk );
            WriteVal( md.baseDef );
            WriteVal( md.baseSpDef );
            WriteVal( md.weight );
            WriteVal( md.size );
            WriteVal( md.mdunk17 );
            WriteVal( md.mdunk18 );
            WriteVal( md.mdunk19 );
            WriteVal( md.mdunk20 );
            WriteVal( md.mdunk21 );
            WriteVal( md.BasePkmn );

            WriteVal( md.exclusiveItems[0] );
            WriteVal( md.exclusiveItems[1] );
            WriteVal( md.exclusiveItems[2] );
            WriteVal( md.exclusiveItems[3] );

            WriteVal( md.unk27 );
            WriteVal( md.unk28 );
            WriteVal( md.unk29 );
            WriteVal( md.unk30 );
        }

        template<class T>
            inline void WriteVal( T var )
        {
            m_itWritePos = utils::WriteIntToByteVector( var, m_itWritePos );
        }

    private:
        const std::vector<stats::PokeMonsterData> & m_Pkmns;
        //vector<uint8_t>                             m_outbuff;
        vector<uint8_t>::iterator                   m_itWritePos;
    };

//==========================================================================================
//  Function
//==========================================================================================

    /*
        Returns a reference to the output vector passed as parameter!
    */
    std::vector<stats::PokeMonsterData> & ParsePokemonBaseData( const std::string            & inpath, 
                                                         std::vector<stats::PokeMonsterData> & out_pkmdat )
    {
        MonsterMdParser( utils::io::ReadFileToByteVector(inpath) ).Parse(out_pkmdat);
        return out_pkmdat;
    }

    std::vector<stats::PokeMonsterData> ParsePokemonBaseData( const std::string & inpath )
    {
        vector<stats::PokeMonsterData> out;
        MonsterMdParser( utils::io::ReadFileToByteVector(inpath) ).Parse(out);
        return std::move(out);
    }

    /*
    */
    void WritePokemonBaseData( const std::vector<stats::PokeMonsterData> & pkmdat, 
                               const std::string                         & outpath )
    {
        utils::io::WriteByteVectorToFile( outpath, MonsterMdWriter(pkmdat).Write() );
    }


};};