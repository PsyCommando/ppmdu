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
            ParseVal( md.categoryIndex );
            ParseVal( md.natPkdexNb2 );
            ParseVal( md.unk1 );
            ParseVal( md.evoData.preEvoIndex );
            ParseVal( md.evoData.evoMethod );
            ParseVal( md.evoData.evoParam1 );
            ParseVal( md.evoData.evoParam2 );
            ParseVal( md.spriteIndex );
            ParseVal( md.genderType );
            ParseVal( md.bodySize );
            ParseVal( md.primaryTy );
            ParseVal( md.secondaryTy );
            ParseVal( md.unk8 );
            ParseVal( md.IQGrp );
            ParseVal( md.primAbility );
            ParseVal( md.secAbility );
            ParseVal( md.unk11 );
            ParseVal( md.unk12 );
            ParseVal( md.unk13 );
            ParseVal( md.baseHP );
            ParseVal( md.unk14 );
            ParseVal( md.baseAtk );
            ParseVal( md.baseSpAtk );
            ParseVal( md.baseDef );
            ParseVal( md.baseSpDef );
            ParseVal( md.unk15 );
            ParseVal( md.unk16 );
            ParseVal( md.unk17 );
            ParseVal( md.unk18 );
            ParseVal( md.unk19 );
            ParseVal( md.unk20 );
            ParseVal( md.unk21 );
            ParseVal( md.pkmnIndex );
            ParseVal( md.unk23 );
            ParseVal( md.unk24 );
            ParseVal( md.unk25 );
            ParseVal( md.unk26 );
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

        void WriteEntry( const stats::PokeMonsterData & entry )
        {
            WriteVal( entry.pokeID );
            WriteVal( entry.categoryIndex );
            WriteVal( entry.natPkdexNb2 );
            WriteVal( entry.unk1 );
            WriteVal( entry.evoData.preEvoIndex );
            WriteVal( entry.evoData.evoMethod );
            WriteVal( entry.evoData.evoParam1 );
            WriteVal( entry.evoData.evoParam2 );
            WriteVal( entry.spriteIndex );
            WriteVal( entry.genderType );
            WriteVal( entry.bodySize );
            WriteVal( entry.primaryTy );
            WriteVal( entry.secondaryTy );
            WriteVal( entry.unk8 );
            WriteVal( entry.IQGrp );
            WriteVal( entry.primAbility );
            WriteVal( entry.secAbility );
            WriteVal( entry.unk11 );
            WriteVal( entry.unk12 );
            WriteVal( entry.unk13 );
            WriteVal( entry.baseHP );
            WriteVal( entry.unk14 );
            WriteVal( entry.baseAtk );
            WriteVal( entry.baseSpAtk );
            WriteVal( entry.baseDef );
            WriteVal( entry.baseSpDef );
            WriteVal( entry.unk15 );
            WriteVal( entry.unk16 );
            WriteVal( entry.unk17 );
            WriteVal( entry.unk18 );
            WriteVal( entry.unk19 );
            WriteVal( entry.unk20 );
            WriteVal( entry.unk21 );
            WriteVal( entry.pkmnIndex );
            WriteVal( entry.unk23 );
            WriteVal( entry.unk24 );
            WriteVal( entry.unk25 );
            WriteVal( entry.unk26 );
            WriteVal( entry.unk27 );
            WriteVal( entry.unk28 );
            WriteVal( entry.unk29 );
            WriteVal( entry.unk30 );
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

    /*
    */
    void WritePokemonBaseData( const std::vector<stats::PokeMonsterData> & pkmdat, 
                               const std::string                         & outpath )
    {
        utils::io::WriteByteVectorToFile( outpath, MonsterMdWriter(pkmdat).Write() );
    }


};};