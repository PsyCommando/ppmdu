#include "waza_p.hpp"
#include <ppmdu/fmts/sir0.hpp>
#include <ppmdu/fmts/integer_encoding.hpp>
#include <ppmdu/pmd2/pmd2_filetypes.hpp>
#include <utils/poco_wrapper.hpp>
#include <utils/library_wide.hpp>
#include <cstdint>
#include <vector>
#include <array>
#include <iostream>
#include <iomanip>
using namespace std;
using namespace pmd2::stats;
using utils::AppendPaddingBytes;
using namespace filetypes;

namespace pmd2 { namespace filetypes 
{

    /*
        A little struct to contain the 2 pointers to the two parts of the file.
    */
    struct WazaPtrs
    {
        uint32_t ptrMovesData = 0;
        uint32_t ptrPLSTbl    = 0;

        template<class _outit>
            _outit WriteToContainer( _outit itWhere )
        {
            itWhere = utils::WriteIntToByteVector( ptrMovesData, itWhere );
            itWhere = utils::WriteIntToByteVector( ptrPLSTbl,    itWhere );
            return itWhere;
        }

        template<class _init>
            _init ReadFromContainer( _init itWhere )
        {
            ptrMovesData = utils::ReadIntFromByteVector<decltype(ptrMovesData)>(itWhere); 
            ptrPLSTbl    = utils::ReadIntFromByteVector<decltype(ptrPLSTbl)>   (itWhere); 
            return itWhere;
        }
    };

//===========================================================================================
//  WazaParser
//===========================================================================================
    class WazaParser
    {
    public:
        WazaParser( const vector<uint8_t> & data )
            :m_rawdata(data)
        {}

        std::pair<MoveDB, std::vector<stats::PokeMoveSet>> Parse()
        {
            return std::make_pair( ParseMoves(), ParseLearnset() );
        }

        MoveDB ParseMoves()
        {
            MoveDB db;

            //Read header if neccessary
            if( !wasHeaderParsed() )
            {
                ParseHeader();
                ParseWazaPtrs();
            }

            auto itRead = (m_rawdata.begin() + m_wazaptrs.ptrMovesData);
            auto itEnd  = (m_rawdata.begin() + m_wazaptrs.ptrPLSTbl);

            //Read all 26 bytes entry, and stop when reaching the beginning of the ptr table or if has read a padding byte !
            while( itRead != itEnd )
            {
                uint8_t sample = *itRead;

                if( sample == filetypes::COMMON_PADDING_BYTE )  //Stop if we hit padding bytes
                {
                    unsigned int dist = 0; //= std::distance( itRead, itEnd );
                    auto itTest = itRead;
                    for( ; dist < 16 && itTest != itEnd; ++itTest ) ++dist;

                    //Get out of the loop if we got as much padding bytes as the number of bytes we're from the end!
                    if( dist < 16 && std::all_of(itRead, itEnd, [](uint8_t by){ return by == filetypes::COMMON_PADDING_BYTE; } ) )
                        break;
                }

                db.push_back( ParseAMove( itRead ) );
            }

            return std::move(db);
        }

        vector<stats::PokeMoveSet> ParseLearnset()
        {
            vector<stats::PokeMoveSet> ls;

            //Read header if neccessary
            if( !wasHeaderParsed() )
            {
                ParseHeader();
                ParseWazaPtrs();
            }

            auto itEnd  = (m_rawdata.begin() + m_wazaptrs.ptrPLSTbl);
            auto ptrtbl = ParsePtrTable();
            ls.resize( ptrtbl.size() );

            clog<<"Parsing learnset data..\n";
            for( unsigned int i = 1; i < ptrtbl.size(); ++i ) //Skip first null entry
            {
                if( utils::LibWide().isLogOn() )
                    clog <<" -- Pokemon #" <<setfill(' ') <<setw(3) <<dec <<i <<" --\n";

                ParseLevelUpMovesList( ls[i],(m_rawdata.begin() + ptrtbl[i][0]), itEnd );
                ParseHMTMMovesList   ( ls[i],(m_rawdata.begin() + ptrtbl[i][1]), itEnd );
                ParseEggMovesList    ( ls[i],(m_rawdata.begin() + ptrtbl[i][2]), itEnd );
            }

            return std::move(ls);
        }

    private:
        /*
            Used to group each individual pokemon's pointers together.
        */
        typedef array<uint32_t,3> pkmnPtrs;

        void ParseHeader()
        {
            m_header.ReadFromContainer( m_rawdata.begin() );
        }

        void ParseWazaPtrs()
        {
            m_wazaptrs.ReadFromContainer( (m_rawdata.begin() + m_header.subheaderptr) );
        }

        vector<pkmnPtrs> ParsePtrTable()
        {
            static const uint32_t SkipEmptyEntries = ( 3 * sizeof(pkmnPtrs::value_type) ); //Skip the first 3 null ptrs!
            static const uint32_t PaddedPointer    = 0xAAAAAAAA;

            vector<pkmnPtrs>      pointers; 
            auto                  itRead        = (m_rawdata.begin() + ( m_wazaptrs.ptrPLSTbl + SkipEmptyEntries ) );
            auto                  itEnd         = (m_rawdata.begin() + m_header.subheaderptr);
            bool                  bHitPadding   = false;

            //std::count_if( itRead, itEnd, [](  ){  } );
            pointers.push_back(pkmnPtrs()); //Push the null first entry

            while( itRead != itEnd )
            {
                pkmnPtrs ptrsonepoke;
                //Read a Poke's 3 pointers
                for( unsigned int cntptr = 0; cntptr < 3; ++cntptr )
                {
                    uint32_t curptr = utils::ReadIntFromByteVector<uint32_t>(itRead);

                    //Break if we hit padding bytes
                    if( curptr == PaddedPointer )
                    {
                        bHitPadding = true;
                        itRead = itEnd; //Stop the loop here
                        break;
                    }
                    else
                        ptrsonepoke[cntptr] = curptr;
                }

                //If we hit padding before the end, quit.
                if( !bHitPadding )
                    pointers.push_back( ptrsonepoke );
                else
                    break;
            }

            return std::move(pointers);
        }

        void ParseLevelUpMovesList( PokeMoveSet              & out_ls,
                                    vector<uint8_t>::const_iterator itRead, 
                                    vector<uint8_t>::const_iterator itEnd )
        {
            try
            {
                vector<uint16_t> decodedvals;
                if( utils::LibWide().isLogOn() )
                {
                    clog<<"Decoding level-up moves..\n";
                    decodedvals = utils::DecodeIntegers<uint16_t>( itRead, itEnd );
                    clog<<"Decoded "<<dec<<decodedvals.size()<<" values!\n";
                }
                else
                    decodedvals = utils::DecodeIntegers<uint16_t>( itRead, itEnd );

                if( (decodedvals.size() % 2) != 0 )
                    throw exception("Missing level + moveID pair(s)!");

                for( unsigned int i = 0; i < decodedvals.size(); i+=2 )
                {
                    if( utils::LibWide().isLogOn() )
                    {
                        clog<<"moveid: 0x" <<setfill('0') <<setw(4) <<hex <<uppercase <<decodedvals[i] <<nouppercase
                            <<", lvl:" <<dec<<decodedvals[i+1] <<"\n";
                    }
                    out_ls.lvlUpMoveSet.emplace( make_pair( decodedvals[i+1], decodedvals[i] ) );
                }
            }
            catch( exception & e )
            {
                stringstream strs;
                strs << "ERROR: something is wrong with an encoded pokemon level-up move learnset! : "
                     << e.what();
                string errorstr = strs.str();
                clog << errorstr <<"\n";
                throw runtime_error(errorstr);
            }
        }

        void ParseHMTMMovesList( PokeMoveSet              & out_ls,
                                 vector<uint8_t>::const_iterator itRead, 
                                 vector<uint8_t>::const_iterator itEnd )
        {
            try
            {
                if( utils::LibWide().isLogOn() )
                {
                    clog<<"Decoding learnable TM/HMs moves..\n";
                    out_ls.teachableHMTMs = utils::DecodeIntegers<moveid_t>( itRead, itEnd );
                    clog<<"Decoded "<<dec<<out_ls.teachableHMTMs.size()<<" values!\n";

                    for( const auto & mv : out_ls.teachableHMTMs )
                        clog<<"moveid: 0x" <<setfill('0') <<setw(4) <<hex <<uppercase <<mv <<nouppercase <<"\n";
                }
                else
                    out_ls.teachableHMTMs = utils::DecodeIntegers<moveid_t>( itRead, itEnd );
            }
            catch( exception & e )
            {
                stringstream strs;
                strs << "ERROR: something is wrong with an encoded pokemon TM/HMs moves list! : "
                     << e.what();
                string errorstr = strs.str();
                clog << errorstr <<"\n";
                throw runtime_error(errorstr);
            }
        }

        void ParseEggMovesList( PokeMoveSet              & out_ls,
                                vector<uint8_t>::const_iterator itRead, 
                                vector<uint8_t>::const_iterator itEnd )
        {
            try
            {
                if( utils::LibWide().isLogOn() )
                {
                    clog<<"Decoding egg moves..\n";
                    out_ls.eggmoves = utils::DecodeIntegers<moveid_t>( itRead, itEnd );
                    clog<<"Decoded "<<dec<<out_ls.eggmoves.size()<<" values!\n";
                    for( const auto & mv : out_ls.eggmoves )
                        clog<<"moveid: 0x" <<setfill('0') <<setw(4) <<hex <<uppercase <<mv <<nouppercase <<"\n";
                }
                else
                    out_ls.eggmoves = utils::DecodeIntegers<moveid_t>( itRead, itEnd );
            }
            catch( exception & e )
            {
                stringstream strs;
                strs << "ERROR: something is wrong with an encoded pokemon egg moves list! : "
                     << e.what();
                string errorstr = strs.str();
                clog << errorstr <<"\n";
                throw runtime_error(errorstr);
            }
        }


        MoveData ParseAMove( vector<uint8_t>::const_iterator & itRead )
        {
            MoveData md;

            ReadValue( md.basePower, itRead );
            ReadValue( md.type,      itRead );
            ReadValue( md.category,  itRead );
            ReadValue( md.unk4,      itRead );
            ReadValue( md.unk5,      itRead );
            ReadValue( md.basePP,    itRead );
            ReadValue( md.unk6,      itRead );
            ReadValue( md.unk7,      itRead );
            ReadValue( md.accuracy,  itRead );
            ReadValue( md.unk9,      itRead );
            ReadValue( md.unk10,     itRead );
            ReadValue( md.unk11,     itRead );
            ReadValue( md.unk12,     itRead );
            ReadValue( md.unk13,     itRead );
            ReadValue( md.unk14,     itRead );
            ReadValue( md.unk15,     itRead );
            ReadValue( md.unk16,     itRead );
            ReadValue( md.unk17,     itRead );
            ReadValue( md.unk18,     itRead );
            ReadValue( md.moveID,    itRead );
            ReadValue( md.unk19,     itRead );

            return std::move(md);
        }

        template<class T>
            inline void ReadValue( T & out_val, vector<uint8_t>::const_iterator & itRead )
        {
            out_val = utils::ReadIntFromByteVector<T>(itRead);
        }

        inline bool wasHeaderParsed()const { return (m_header.magic != 0); }

    private:
        const vector<uint8_t>           & m_rawdata;
        sir0_header                       m_header;
        WazaPtrs                          m_wazaptrs;
    };


//===========================================================================================
//  WazaWriter
//===========================================================================================
    class WazaWriter
    {
    public:
        WazaWriter(const std::vector<stats::PokeMoveSet> & movesets, const stats::MoveDB & movesdata )
            :m_pkmnmoves(movesets), m_movesdata(movesdata), m_itWrite(m_outBuff)
        {}

        vector<uint8_t> Write()
        {
            m_outBuff = vector<uint8_t>( sir0_header::HEADER_LEN ); //Reserve space for the header
            m_itWrite = back_inserter(m_outBuff);

            WritePkmnMoveLists();
            AppendPaddingBytes( m_itWrite, m_outBuff.size(), 16, COMMON_PADDING_BYTE );
            WriteMoveList();
            AppendPaddingBytes( m_itWrite, m_outBuff.size(), 16, COMMON_PADDING_BYTE );
            WritePtrTable();
            AppendPaddingBytes( m_itWrite, m_outBuff.size(), 16, COMMON_PADDING_BYTE );
            uint32_t offsetWazaPtrs = m_outBuff.size();
            WriteWazaPtrs();
            AppendPaddingBytes( m_itWrite, m_outBuff.size(), 16, COMMON_PADDING_BYTE );

            //Append SIR0 ptr offset list
            auto sir0dat = MakeSIR0ForData( m_ptrofflist, (offsetWazaPtrs - sir0_header::HEADER_LEN), (m_outBuff.size() - sir0_header::HEADER_LEN) );
            for( const auto & offset : sir0dat.ptroffsetslst ) 
                WriteValue(offset);

            //Add EoF padding
            AppendPaddingBytes( m_itWrite, m_outBuff.size(), 16, COMMON_PADDING_BYTE );
            
            //Write SIR0 header in the reserved slot
            sir0dat.hdr.WriteToContainer( m_outBuff.begin() );

            return std::move(m_outBuff);
        }

    private:

        uint32_t WritePtr( uint32_t ptr )
        {
            if( ptr != 0 )
            {
                //if( ! m_ptrofflist.empty() ) 
                //{
                //    m_ptrofflist.push_back(m_outBuff.size() - m_ptrofflist.back());
                //}
                //else
                m_ptrofflist.push_back( m_outBuff.size() );
            }
            m_itWrite = utils::WriteIntToByteVector( ptr, m_itWrite );
            return ptr;
        }

        void WritePkmnMoveLists()
        {
            //#FIXME: Not sure if should hardcode this ?!
            //Write the first 3 null move lists
            //WriteValue(uint8_t(0));
            //WriteValue(uint8_t(0));
            //WriteValue(uint8_t(0));

            //Skip first null entry
            for( auto itpkmove = m_pkmnmoves.begin(); itpkmove != m_pkmnmoves.end(); ++itpkmove )
            {
                WriteAPkmnMoveLists(*itpkmove);
            }
        }

        void WriteAPkmnMoveLists( const stats::PokeMoveSet & pkmnmvs )
        {
            //Make a re-usable shared buffer for encoding
            vector<uint16_t> encodebuff;

            WriteAPkmnLvlUpLists( pkmnmvs, encodebuff );
            WriteAPkmnTMHMLists ( pkmnmvs, encodebuff );
            WriteAPkmnEggLists  ( pkmnmvs, encodebuff );
        }

        void WriteAPkmnLvlUpLists( const stats::PokeMoveSet & pkmnmvs, vector<uint16_t> & encodebuff )
        {
            //Write down offsets to each entries 
            m_ptrPkmnMvTbl.push_back(m_outBuff.size());

            //Ensure the shared buffer is empty, and reserve some memory
            encodebuff.reserve( pkmnmvs.lvlUpMoveSet.size() * 2 );
            encodebuff.resize(0);

            for( const auto & alvlupmv : pkmnmvs.lvlUpMoveSet )
            {
                //Move ID
                encodebuff.push_back( alvlupmv.second );
                //Level
                encodebuff.push_back( alvlupmv.first );
            }
            utils::EncodeIntegers( encodebuff.begin(), encodebuff.end(), m_itWrite );
        }

        void WriteAPkmnTMHMLists( const stats::PokeMoveSet & pkmnmvs, vector<uint16_t> & encodebuff )
        {
            //Write down offsets to each entries 
            m_ptrPkmnMvTbl.push_back(m_outBuff.size());

            //Ensure the shared buffer is empty, and reserve
            encodebuff.reserve( pkmnmvs.teachableHMTMs.size() );
            encodebuff.resize(0);

            for( const auto & atmhmv : pkmnmvs.teachableHMTMs )
                encodebuff.push_back( atmhmv );
            utils::EncodeIntegers( encodebuff.begin(), encodebuff.end(), m_itWrite );
        }

        void WriteAPkmnEggLists( const stats::PokeMoveSet & pkmnmvs, vector<uint16_t> & encodebuff )
        {
            //Write down offsets to each entries 
            m_ptrPkmnMvTbl.push_back(m_outBuff.size());

            //Ensure the shared buffer is empty, and reserve
            encodebuff.reserve( pkmnmvs.eggmoves.size() );
            encodebuff.resize(0);

            for( const auto & aeggmv : pkmnmvs.eggmoves )
                encodebuff.push_back( aeggmv );
            utils::EncodeIntegers( encodebuff.begin(), encodebuff.end(), m_itWrite );
        }

        void WriteMoveList()
        {
            //Write down the move data block start offset!
            m_wazptrs.ptrMovesData = m_outBuff.size();

            const size_t nbmoves = m_movesdata.size();

            for( size_t i = 0; i < nbmoves; ++i )
                WriteAMove( m_movesdata[i] );
        }

        void WriteAMove( const MoveData & md )
        {
            WriteValue( md.basePower );
            WriteValue( md.type );
            WriteValue( md.category );
            WriteValue( md.unk4 );
            WriteValue( md.unk5 );
            WriteValue( md.basePP );
            WriteValue( md.unk6 );
            WriteValue( md.unk7 );
            WriteValue( md.accuracy );
            WriteValue( md.unk9 );
            WriteValue( md.unk10 );
            WriteValue( md.unk11 );
            WriteValue( md.unk12 );
            WriteValue( md.unk13 );
            WriteValue( md.unk14 );
            WriteValue( md.unk15 );
            WriteValue( md.unk16 );
            WriteValue( md.unk17 );
            WriteValue( md.unk18 );
            WriteValue( md.moveID );
            WriteValue( md.unk19 );
        }

        template<class T>
            inline void WriteValue( T val )
        {
            utils::WriteIntToByteVector( val, m_itWrite );
        }

        void WritePtrTable()
        {
            //Write the location of the ptr table
            m_wazptrs.ptrPLSTbl = m_outBuff.size();

            //#FIXME: Not sure if should hardcode this ?!
            //Write the initial 3 null dummy ptrs
            //utils::WriteIntToByteVector( uint32_t(0), m_itWrite );
            //utils::WriteIntToByteVector( uint32_t(0), m_itWrite );
            //utils::WriteIntToByteVector( uint32_t(0), m_itWrite );

            //
            ///uint32_t lastptr = 0;
            for( unsigned int i = 0; i < m_ptrPkmnMvTbl.size(); ++i )
            {
                if( !m_pkmnmoves[i/3].empty() )
                {
                    WritePtr(m_ptrPkmnMvTbl[i]);
                }
                else if( i < 3 )
                {
                    WritePtr(0);
                }
                else
                {
                    stringstream sstrerr;
                    sstrerr << "Found extra empty pokemon moveset data at index " <<i <<"!";
                    throw runtime_error( sstrerr.str() );
                }

            }
        }

        void WriteWazaPtrs()
        {
            WritePtr( m_wazptrs.ptrMovesData );
            WritePtr( m_wazptrs.ptrPLSTbl );
        }

        WazaPtrs                                     m_wazptrs;
        vector<uint32_t>                             m_ptrPkmnMvTbl;
        vector<uint32_t>                             m_ptrofflist;
        vector<uint8_t>                              m_outBuff;
        back_insert_iterator<vector<uint8_t>>        m_itWrite;
        const std::vector<stats::PokeMoveSet> & m_pkmnmoves;
        const stats::MoveDB                        & m_movesdata;
    };

//
//  Functions
//
//

    pair<vector<uint8_t>,vector<uint8_t>> LoadWazaData( const std::string & pathOfBalanceDir )
    {
        stringstream            sstrpathWaza;
        stringstream            sstrpathWaza2;

        sstrpathWaza  << pathOfBalanceDir <<"/" <<WAZA_Fname;
        sstrpathWaza2 << pathOfBalanceDir <<"/" <<WAZA2_Fname;

        string pathWaza  = sstrpathWaza.str();
        string pathWaza2 = sstrpathWaza2.str();

        if( ! utils::isFile( pathWaza ) )
        {
            stringstream sstr;
            sstr << "ERROR: File \"" <<pathWaza <<"\" is missing!";
            string errorstr = sstr.str();
            clog << errorstr <<"\n";
            throw std::runtime_error(errorstr);
        }

        auto wazadata = utils::io::ReadFileToByteVector( pathWaza );

        if( utils::isFile( pathWaza2 ) )
            return make_pair( std::move(wazadata), utils::io::ReadFileToByteVector( pathWaza2 ) );
        else
            return make_pair( std::move(wazadata), vector<uint8_t>() );
    }

    pokeMvSets_t ParsePokemonLearnSets( const std::string & pathOfBalanceDir )
    {
        pokeMvSets_t result;
        clog << "Parsing pokemon learnsets..\n";

        auto wazadata = LoadWazaData(pathOfBalanceDir);

        if( ! (wazadata.second.empty()) )
        {
            clog << "waza_p2.bin is present. Assuming game data is from Explorers of Sky!\n";
            result = ParsePokemonLearnSets( wazadata.first, wazadata.second );
        }
        else
        {
            clog << "waza_p2.bin is NOT present. Assuming game data is from Explorers of Time/Darkness!\n";
            result.first = ParsePokemonLearnSets(wazadata.first);
        }
        clog << "Parsing complete!\n";
        return result;
    }

    std::vector<stats::PokeMoveSet> ParsePokemonLearnSets( const std::vector<uint8_t> & waza_pData )
    {
        return WazaParser(waza_pData).ParseLearnset();
    }
    
    pokeMvSets_t ParsePokemonLearnSets( const std::vector<uint8_t> & waza_pData, 
                                                   const std::vector<uint8_t> & waza_p2Data )
    {
        WazaParser Waza1(waza_pData);
        WazaParser Waza2(waza_p2Data);
        return pokeMvSets_t{ Waza1.ParseLearnset(), Waza2.ParseLearnset() };
    }

    /*
        ParseMoveData
            Will parse all the move data to a MoveDB if EoT/EoD, and two if EoS!
    */
    std::pair<MoveDB,MoveDB> ParseMoveData( const std::string & pathOfBalanceDir )
    {
        std::pair<MoveDB,MoveDB> result;
        if( utils::LibWide().isLogOn() )
            clog << "Parsing move data..\n";

        auto wazadata = LoadWazaData(pathOfBalanceDir);

        if( ! (wazadata.second.empty()) )
        {
            if( utils::LibWide().isLogOn() )
                clog << "waza_p2.bin is present. Assuming game data is from Explorers of Sky!\n";
            result = ParseMoveData( wazadata.first, wazadata.second );
        }
        else
        {
            if( utils::LibWide().isLogOn() )
                clog << "waza_p2.bin is NOT present. Assuming game data is from Explorers of Time/Darkness!\n";
            result.first = ParseMoveData(wazadata.first);
        }
        if( utils::LibWide().isLogOn() )
            clog << "Parsing complete!\n";
        return result;
    }
    
    MoveDB ParseMoveData( const std::vector<uint8_t> & waza_pData )
    {
        return WazaParser(waza_pData).ParseMoves();
    }

    std::pair<MoveDB,MoveDB> ParseMoveData( const std::vector<uint8_t> & waza_pData, 
                                            const std::vector<uint8_t> & waza_p2Data )
    {
        auto waza1 = WazaParser(waza_pData);
        auto waza2 = WazaParser(waza_p2Data);
        return make_pair( waza1.ParseMoves(), waza2.ParseMoves() );
    }

    /*
        ParseMoveAndLearnsets
            Parse both the above at the same time.
    */
    std::pair<combinedmovedat_t,pokeMvSets_t> ParseMoveAndLearnsets( const std::string & pathOfBalanceDir )
    {
        return make_pair( ParseMoveData(pathOfBalanceDir), ParsePokemonLearnSets(pathOfBalanceDir) );
    }

    /*
        WriteMoveAndLearnsets
            Will write at least a "waza_p.bin" file. If all 2 learnsets and moves data lists are there, will 
            output an additional "waza_p2.bin" file!

    */
    void WriteMoveAndLearnsets( const std::string                            & pathOutBalanceDir,
                                const std::pair<stats::MoveDB,stats::MoveDB> & movedata, 
                                const pokeMvSets_t                           & lvlupmvset )
    {
        if( !utils::isFolder(pathOutBalanceDir) )
        {
            ostringstream sstr;
            sstr <<"ERROR: Invalid path \"" <<pathOutBalanceDir <<"\" is not a directory!";
            string errorstr = sstr.str();
            clog <<"WriteMoveAndLearnsets():" <<errorstr <<"\n";
            throw std::runtime_error(errorstr);
        }

        stringstream sstrWaza1Path, 
                     sstrWaza2Path;

        sstrWaza1Path <<pathOutBalanceDir <<"/" <<WAZA_Fname;
        sstrWaza2Path <<pathOutBalanceDir <<"/" <<WAZA2_Fname;

        string waza1Path = sstrWaza1Path.str();
        string waza2Path = sstrWaza2Path.str();


        if( !(lvlupmvset.first.empty()) && !(movedata.first.empty()) )
        {
            WazaWriter wazaw1( lvlupmvset.first, movedata.first );

            if( !(lvlupmvset.second.empty()) && !(movedata.second.empty()) )
            {
                if( utils::LibWide().isLogOn() )
                    clog << "Second learnset list and movedata list are present. Assuming game data is from Explorers of Sky! Writing waza_p2.bin file!\n";
                WazaWriter wazaw2( lvlupmvset.second, movedata.second );
                utils::io::WriteByteVectorToFile( waza1Path, wazaw1.Write() );
                utils::io::WriteByteVectorToFile( waza2Path, wazaw2.Write() );
                if( utils::LibWide().isLogOn() )
                    clog << "Wrote \"" <<waza1Path <<"\" and \"" <<waza2Path <<"\" successfully!\n";
            }
            else
            {
                if( utils::LibWide().isLogOn() )
                    clog << "Second learnset list and movedata list are NOT present. Assuming game data is from Explorers of Time/Darkness! Skipping on waza_p2.bin file!\n";
                utils::io::WriteByteVectorToFile( waza1Path, wazaw1.Write() );
                if( utils::LibWide().isLogOn() )
                    clog << "Wrote \"" <<waza1Path <<"\" successfully!\n";
            }
        }
        else
        {
            stringstream sstr;
            sstr << "ERROR: Not enough data to write a waza_p.bin file!";
            string errorstr = sstr.str();
            clog <<"WriteMoveAndLearnsets():" << errorstr <<"\n";
            throw std::runtime_error(errorstr);
        }
    }

};};