#include "ssb.hpp"
#include <ppmdu/pmd2/pmd2_scripts.hpp>
#include <ppmdu/pmd2/pmd2_scripts_opcodes.hpp>
#include <utils/utility.hpp>
#include <utils/library_wide.hpp>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
using namespace pmd2;
using namespace std;

namespace filetypes
{
    /*
        group_entry
            Script instruction group entry
    */
    struct group_entry
    {
        static const size_t LEN = 3 * sizeof(uint16_t);
        uint16_t begoffset = 0;
        uint16_t type      = 0;
        uint16_t unk2      = 0;

        template<class _outit>
            _outit WriteToContainer(_outit itwriteto)const
        {
            itwriteto = utils::WriteIntToBytes(begoffset,  itwriteto);
            itwriteto = utils::WriteIntToBytes(type,       itwriteto);
            itwriteto = utils::WriteIntToBytes(unk2,       itwriteto);
            return itwriteto;
        }

        //
        template<class _init>
            _init ReadFromContainer(_init itReadfrom, _init itpastend)
        {
            itReadfrom = utils::ReadIntFromBytes(begoffset, itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes(type,      itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes(unk2,      itReadfrom, itpastend );
            return itReadfrom;
        }
    };

    /*
        OpCodeFinderPicker
            Retrieve opcode info for the correct version of the game.
    */
    template<eOpCodeVersion>
        struct OpCodeFinderPicker;

    template<>
        struct OpCodeFinderPicker<eOpCodeVersion::EoS>
    {
        inline const OpCodeInfoEoS * operator()( uint16_t opcode )
        {
            return FindOpCodeInfo_EoS(opcode);
        }
    };

    template<>
        struct OpCodeFinderPicker<eOpCodeVersion::EoTD>
    {
        inline const OpCodeInfoEoTD * operator()( uint16_t opcode )
        {
            return FindOpCodeInfo_EoTD(opcode);
        }
    };


//=======================================================================================
//  SSB Parser
//=======================================================================================
    /*
        SSB_Parser
            Parse SSB files.
    */
    template<typename _Init>
        class SSB_Parser
    {

    public:
        typedef _Init                           initer;

        SSB_Parser( _Init beg, _Init end, eOpCodeVersion scrver, eGameRegion scrloc )
            :m_beg(beg), m_end(end), m_cur(beg), m_scrversion(scrver), m_scrRegion(scrloc)
        {}

        ScriptedSequence Parse()
        {
            m_out = std::move(ScriptedSequence());
            ParseHeader   ();
            ParseGroups   ();
            ParseCode     ();
            ParseConstants();
            ParseStrings  ();
            return std::move(m_out);
        }

    private:

        void ParseHeader()
        {
            uint16_t scriptdatalen = 0;
            uint16_t constdatalen  = 0;

            if( m_scrRegion == eGameRegion::NorthAmerica )
            {
                ssb_header hdr;
                m_hdrlen = ssb_header::LEN;
                m_cur = hdr.ReadFromContainer( m_cur, m_end );

                m_nbconsts     = hdr.nbconst;
                m_nbstrs       = hdr.nbstrs;
                scriptdatalen  = hdr.scriptdatlen;
                constdatalen   = hdr.consttbllen;
                m_stringblksSizes.push_back( hdr.strtbllen * ScriptWordLen );
            }
            else if( m_scrRegion == eGameRegion::Europe )
            {
                ssb_header_pal hdr;
                m_hdrlen = ssb_header_pal::LEN;
                m_cur = hdr.ReadFromContainer( m_cur, m_end );

                m_nbconsts    = hdr.nbconst;
                m_nbstrs      = hdr.nbstrs;
                scriptdatalen = hdr.scriptdatlen;
                constdatalen  = hdr.consttbllen;
                m_stringblksSizes.push_back( hdr.strenglen * ScriptWordLen );
                m_stringblksSizes.push_back( hdr.strfrelen * ScriptWordLen );
                m_stringblksSizes.push_back( hdr.strgerlen * ScriptWordLen );
                m_stringblksSizes.push_back( hdr.stritalen * ScriptWordLen );
                m_stringblksSizes.push_back( hdr.strspalen * ScriptWordLen );
            }
            else if( m_scrRegion == eGameRegion::Japan )
            {
                ssb_header hdr;
                m_hdrlen = ssb_header::LEN;
                m_cur = hdr.ReadFromContainer( m_cur, m_end );

                m_nbconsts     = hdr.nbconst;
                m_nbstrs       = hdr.nbstrs;
                scriptdatalen  = hdr.scriptdatlen;
                constdatalen   = hdr.consttbllen;
                m_stringblksSizes.push_back( hdr.strtbllen * ScriptWordLen );
                //throw std::runtime_error("SSBParser::ParseHeader(): Japanese ssb header not yet supported. Aborting script parsing!");
            }
            else
            {
                cout<<"SSB_Parser::ParseHeader(): Unknown script region!!\n";
                assert(false);
            }

            //Parse SSB Header
            
            //Parse Data header
            m_cur = m_dathdr.ReadFromContainer( m_cur, m_end );

            //Compute offsets
            m_datalen        = (m_dathdr.datalen * ScriptWordLen);
            m_constoffset    = m_hdrlen + m_datalen;         //Group table is included into the datalen
            m_stringblockbeg = m_hdrlen + (scriptdatalen * ScriptWordLen) + (constdatalen*2);
        }

        void ParseGroups()
        {
            m_grps.resize( m_dathdr.nbgrps );
            //Grab all groups
            for( size_t cntgrp = 0; cntgrp < m_dathdr.nbgrps; ++cntgrp )
            {
                m_cur = m_grps[cntgrp].ReadFromContainer( m_cur, m_end );
            }

            //!#TODO: Do some validation I guess?
        }

        void ParseCode()
        {
            if( m_scrversion == eOpCodeVersion::EoS ) 
                ParseCodeWithOpCodeFinder(OpCodeFinderPicker<eOpCodeVersion::EoS>(), OpCodeNumberPicker<eOpCodeVersion::EoS>());
            else if( m_scrversion == eOpCodeVersion::EoTD )
                ParseCodeWithOpCodeFinder(OpCodeFinderPicker<eOpCodeVersion::EoTD>(), OpCodeNumberPicker<eOpCodeVersion::EoTD>() );
            else
            {
                clog << "\n<!>- SSB_Parser::ParseCode() : INVALID SCRIPT VERSION!!\n";
                assert(false);
            }
        }

        template<typename _InstFinder, typename _InstNumber>
            void ParseCodeWithOpCodeFinder(_InstFinder & opcodefinder, _InstNumber & opcodenumber)
        {
            //Iterate through all group and grab their instructions.
            for( size_t cntgrp = 0; cntgrp < m_dathdr.nbgrps; ++cntgrp )
            {
                ScriptInstrGrp grp;
                size_t absgrpbeg = (m_grps[cntgrp].begoffset * ScriptWordLen) + m_hdrlen;
                size_t absgrpend = ((cntgrp +1) < m_dathdr.nbgrps)?
                                    (m_grps[cntgrp+1].begoffset * ScriptWordLen) + m_hdrlen :   //If we have another group after, this is the end
                                    m_datalen + m_hdrlen;                                       //If we have no group after, end of script data is end


                grp.instructions = std::move( ParseInstructionSequence( absgrpbeg, 
                                                                        absgrpend,
                                                                        opcodefinder,
                                                                        opcodenumber ) );
                grp.type         = m_grps[cntgrp].type;
                grp.unk2         = m_grps[cntgrp].unk2;
                m_out.Groups().push_back( std::move(grp) );
            }
        }


        template<typename _InstFinder, typename _InstNumber>
            deque<ScriptInstruction> ParseInstructionSequence( size_t foffset, size_t endoffset, _InstFinder & opcodefinder, _InstNumber & opcodenumber )
        {
            deque<ScriptInstruction> sequence;
            m_cur = m_beg;
            std::advance( m_cur, foffset ); 
            auto itendseq= m_beg;
            std::advance( itendseq, endoffset);


            while( m_cur != itendseq )
            {
                uint16_t curop = utils::ReadIntFromBytes<uint16_t>( m_cur, itendseq );

                //if( curop != NullOpCode )
                if( curop < opcodenumber() )
                {
                    auto opcodedata = opcodefinder(curop); //THis should never happen though..
                    
                    if( opcodedata == nullptr )
                    {
#ifdef _DEBUG
                        assert(false);
#endif
                        throw std::runtime_error("SSB_Parser::ParseInstructionSequence() : Unknown Opcode!");
                    }

                    ScriptInstruction inst;
                    inst.isdata = false;
                    inst.opcode = curop;

                    if( opcodedata->nbparams >= 0 && m_cur != itendseq )
                    {
                        const uint16_t nbparams = static_cast<uint16_t>(opcodedata->nbparams);
                        size_t cntparam = 0;
                        for( ; cntparam < nbparams && m_cur != itendseq; ++cntparam )
                        {
                            inst.parameters.push_back( utils::ReadIntFromBytes<uint16_t>(m_cur, itendseq) );
                        }
                        foffset += cntparam * ScriptWordLen;

                        if( cntparam != nbparams )
                            clog << "\n<!>- Found instruction with not enough bytes left to assemble all its parameters at offset 0x" <<hex <<uppercase <<foffset <<dec <<nouppercase <<"\n";
                        else
                            sequence.push_back(std::move(inst));
                    }
                    else
                    {
                        clog << "\n<!>- Found instruction with -1 parameter number in this script! Offset 0x" <<hex <<uppercase <<foffset <<dec <<nouppercase <<"\n";
                        sequence.push_back(std::move(inst));
                    }
                }
                else
                {
                    if( utils::LibWide().isLogOn() )
                        clog<<"0x" <<hex <<uppercase <<foffset  <<" - Got data word 0x" <<curop <<" \n" <<nouppercase <<dec;

                    //The instruction is actually a data word
                    ScriptInstruction inst;
                    inst.isdata = true;
                    inst.opcode = curop;
                    sequence.push_back(std::move(inst));
                }
                foffset += ScriptWordLen;
            }

            return std::move(sequence);
        }


        void ParseConstants()
        {
            if( !m_nbconsts )
                return;

            const size_t strlutlen = (m_nbstrs * 2); // In the file, the offset for each constants in the constant table includes the 
                                                     // length of the string lookup table(string pointers). Here, to compensate
                                                     // we subtract the length of the string LUT from each pointer read.
            m_out.ConstTbl() = std::move(ParseOffsetTblAndStrings<ScriptedSequence::consttbl_t>( m_constoffset, m_constoffset, m_nbconsts, strlutlen ));
        }

        void ParseStrings()
        {
            if( !m_nbstrs )
                return;

            //Parse the strings for any languages we have
            size_t strparseoffset = m_stringblockbeg;
            size_t begoffset      = ( m_nbconsts != 0 )? m_constoffset : m_stringblockbeg;

            for( size_t i = 0; i < m_stringblksSizes.size(); ++i )
            {
                m_out.InsertStrLanguage( static_cast<eGameLanguages>(i), std::move(ParseOffsetTblAndStrings<ScriptedSequence::strtbl_t>( strparseoffset, begoffset, m_nbstrs )) );
                //m_out.StrTbl( static_cast<eGameLanguages>(i) ) = ;
                strparseoffset += m_stringblksSizes[i]; //Add the size of the last block, so we have the offset of the next table
            }
        }

        /*
            relptroff == The position in the file against which the offsets in the table are added to.
            offsetdiff == this value will be subtracted from every ptr read in the table.
        */
        template<class _ContainerT>
            _ContainerT ParseOffsetTblAndStrings( size_t foffset, uint16_t relptroff, uint16_t nbtoparse, long offsetdiff=0 )
        {
            _ContainerT strings;

                //Parse regular strings here
                initer itoreltblbeg = m_beg;
                std::advance( itoreltblbeg, relptroff);
                //initer itstrtbl = m_beg;
                //std::advance( itstrtbl, foffset );
                initer itluttable = m_beg;
                std::advance(itluttable, foffset);
            
                assert( itoreltblbeg != m_end );

                //Parse string table
                for( size_t cntstr = 0; cntstr < nbtoparse && itluttable != m_end; ++cntstr )
                {
                    uint16_t stroffset = utils::ReadIntFromBytes<uint16_t>( itluttable, m_end ) - offsetdiff; //Offset is in bytes this time!
                    initer   itstr     = itoreltblbeg;
                    std::advance(itstr,stroffset);
                    strings.push_back( std::move(utils::ReadCStrFromBytes( itstr, m_end )) );
                }

            return std::move(strings);
        }

    private:
        initer              m_beg;
        initer              m_cur;
        initer              m_end;
        ScriptedSequence    m_out;

        size_t              m_hdrlen;
        ssb_data_hdr        m_dathdr;

        uint16_t            m_nbstrs;
        uint16_t            m_nbconsts;
        vector<uint16_t>    m_stringblksSizes;     //in bytes //The lenghts of all strings blocks for each languages

        size_t              m_datalen;             //in bytes //Length of the Data block in bytes
        size_t              m_constoffset;         //in bytes //Start of the constant block
        size_t              m_stringblockbeg;      //in bytes //Start of strings blocks

        vector<group_entry> m_grps;

        eOpCodeVersion       m_scrversion; 
        eGameRegion          m_scrRegion;
    };

//=======================================================================================
//  SSB Writer
//=======================================================================================

    class SSBWriterTofile
    {
        typedef ostreambuf_iterator<char> outit_t;
    public:
        SSBWriterTofile(const pmd2::ScriptedSequence & scrdat, eGameRegion gloc, eOpCodeVersion opver)
            :m_scrdat(scrdat), m_scrRegion(gloc), m_opversion(opver)
        {
            if( m_scrRegion == eGameRegion::NorthAmerica || m_scrRegion == eGameRegion::NorthAmerica )
                m_stringblksSizes.resize(1,0);
            else if( m_scrRegion == eGameRegion::Europe )
                m_stringblksSizes.resize(5,0);
        }

        void Write(const std::string & scriptfile)
        {
            m_outf.open(scriptfile, ios::binary | ios::out);
            if( m_outf.bad() || !m_outf.is_open() )
                throw std::runtime_error("SSBWriterTofile::Write(): Couldn't open file " + scriptfile);

            m_hdrlen         = 0;
            m_datalen        = 0; 
            m_nbstrings      = 0;
            m_constoffset    = 0;
            m_constblksize   = 0;
            m_stringblockbeg = 0;

            if( m_scrRegion == eGameRegion::NorthAmerica || m_scrRegion == eGameRegion::Japan )
                m_hdrlen = ssb_header::LEN;
            else if( m_scrRegion == eGameRegion::Europe )
                m_hdrlen = ssb_header_pal::LEN;

            outit_t oit(m_outf);
            //#1 - Reserve data header 
            std::fill_n( oit, m_hdrlen + ssb_data_hdr::LEN, 0 );
            m_datalen += ssb_data_hdr::LEN; //Add to the total length immediately

            //#2 - Reserve group table
            std::fill_n( oit, m_scrdat.Groups().size() * group_entry::LEN, 0 );

            //#3 - Pre-Alloc/pre-calc stuff
            CalcAndVerifyNbStrings();
            m_grps.reserve( m_scrdat.Groups().size() );

            //#4 - Write code for each groups, constants, strings
            WriteCode(oit);
            WriteConstants(oit);
            WriteStrings(oit);

            //#5 - Header and group table written last, since the offsets and sizes are calculated as we go.
            m_outf.seekp(0, ios::beg);
            outit_t ithdr(m_outf);
            WriteHeader    (ithdr);
            WriteGroupTable(ithdr);
        }

    private:

        //Since we may have several string blocks to deal with, we want to make sure they're all the same size.
        void CalcAndVerifyNbStrings()
        {
            size_t siz = 0;
            for( auto & cur : m_scrdat.StrTblSet() )
            {
                if( siz == 0 )
                    siz = cur.second.size();
                else if( cur.second.size() != siz )
                    throw std::runtime_error("SSBWriterTofile::CalcAndVerifyNbStrings(): Size mismatch in one of the languages' string table!");
            }
            m_nbstrings = siz;
        }

        void WriteHeader( outit_t & itw )
        {
#ifdef _DEBUG
                assert(m_stringblksSizes.size()>= 1);
#endif // _DEBUG
            if( m_scrRegion == eGameRegion::NorthAmerica )
            {
                ssb_header hdr;
                hdr.nbconst      = m_scrdat.ConstTbl().size();
                hdr.nbstrs       = m_nbstrings;
                hdr.scriptdatlen = TryConvertToScriptLen(m_datalen);
                hdr.consttbllen  = TryConvertToScriptLen(m_constblksize);
                hdr.strtbllen    = TryConvertToScriptLen(m_stringblksSizes.front());
                hdr.unk1         = 0; //Unk1 seems to be completely useless, so we're putting in random junk
                itw = hdr.WriteToContainer(itw);
            }
            else if( m_scrRegion == eGameRegion::Europe )
            {
#ifdef _DEBUG
                assert(m_stringblksSizes.size()== 5);
#endif // _DEBUG
                ssb_header_pal hdr;
                hdr.nbconst      = m_scrdat.ConstTbl().size();
                hdr.nbstrs       = m_nbstrings;
                hdr.scriptdatlen = TryConvertToScriptLen(m_datalen);
                hdr.consttbllen  = TryConvertToScriptLen(m_constblksize);
                if( m_nbstrings != 0 )
                {
                    hdr.strenglen = TryConvertToScriptLen(m_stringblksSizes[0]);
                    hdr.strfrelen = TryConvertToScriptLen(m_stringblksSizes[1]);
                    hdr.strgerlen = TryConvertToScriptLen(m_stringblksSizes[2]);
                    hdr.stritalen = TryConvertToScriptLen(m_stringblksSizes[3]);
                    hdr.strspalen = TryConvertToScriptLen(m_stringblksSizes[4]);
                }
                else
                {
                    hdr.strenglen = 0;
                    hdr.strfrelen = 0;
                    hdr.strgerlen = 0;
                    hdr.stritalen = 0;
                    hdr.strspalen = 0;
                }
                itw = hdr.WriteToContainer(itw);
            }
            else if( m_scrRegion == eGameRegion::Japan )
            {
                //The japanese game makes no distinction between strings and constants, and just places everything in the constant slot
                ssb_header hdr;
                hdr.nbconst      = m_scrdat.ConstTbl().size() + m_nbstrings;
                hdr.nbstrs       = 0;
                hdr.scriptdatlen = TryConvertToScriptLen(m_datalen);
                hdr.consttbllen  = TryConvertToScriptLen(m_constblksize);
                hdr.strtbllen    = 0;
                hdr.unk1         = 0; //Unk1 seems to be completely useless, so we're putting in random junk
                itw = hdr.WriteToContainer(itw);
            }

            ssb_data_hdr dathdr;
            dathdr.nbgrps  = m_scrdat.Groups().size();

            if( m_constoffset > 0 )
                dathdr.datalen = TryConvertToScriptLen(m_constoffset - m_hdrlen); //Const offset table isn't counted in this value, so we can't use m_datalen
            else 
                dathdr.datalen = TryConvertToScriptLen(m_datalen); //If no const table, we can set this to m_datalen
            itw = dathdr.WriteToContainer(itw);
        }

        //Write the table after the data header listing all the instruction groups
        void WriteGroupTable( outit_t & itw )
        {
#ifdef _DEBUG   //!#REMOVEME: For testing
            assert(!m_grps.empty()); //There is always at least one group!
#endif
            for( const auto & entry : m_grps )
            {
                itw = entry.WriteToContainer(itw);
            }
        }


        void WriteCode( outit_t & itw )
        {
            for( const auto & grp : m_scrdat.Groups() )
            {
                //Add a group entry for the current instruction group
                group_entry grent;
                grent.begoffset = TryConvertToScriptLen( (static_cast<uint16_t>(m_outf.tellp()) -  m_hdrlen) );
                grent.type      = grp.type;
                grent.unk2      = grp.unk2;
                m_grps.push_back(grent);
                m_datalen += group_entry::LEN;

                //Write the content of the group
                for( const auto & inst : grp )
                    WriteInstruction(itw,inst);
            }
        }

        void WriteInstruction( outit_t & itw, const ScriptInstruction & inst )
        {
            itw = utils::WriteIntToBytes( inst.opcode, itw );
            m_datalen += ScriptWordLen;

            //!#TODO: We might want to add something here to handle references to file offsets used as parameters
            for( const auto & param : inst.parameters )
            {
                itw = utils::WriteIntToBytes( param, itw );
                m_datalen += ScriptWordLen;
            }
        }

        void WriteConstants( outit_t & itw )
        {
            if( m_scrdat.ConstTbl().empty() )
                return;
            //**The constant pointer table counts as part of the script data, but not the constant strings it points to for some weird reasons!!**
            //**Also, the offsets in the tables include the length of the string ptr table!**
            const streampos befconsttbl = m_outf.tellp();
            m_constoffset = static_cast<size_t>(befconsttbl);   //Save the location where we'll write the constant ptr table at, for the data header
            
            const uint16_t  sizcptrtbl     = m_scrdat.ConstTbl().size() * ScriptWordLen;
            const uint16_t  szstringptrtbl = m_nbstrings * ScriptWordLen;
            m_datalen += sizcptrtbl;    //Add the length of the table to the scriptdata length value for the header
            m_constblksize = WriteTableAndStrings( itw, m_scrdat.ConstTbl(),szstringptrtbl); //The constant strings data is not counted in datalen!




            //
            //size_t cntconst = 0;
            //const streampos befconsttbl = m_outf.tellp();
            //m_constoffset = static_cast<size_t>(befconsttbl);   //Save the location where we'll write the constant ptr table

            ////reserve table, so we can write the offsets as we go
            //const uint16_t  sizcptrtbl  = m_scrdat.ConstTbl().size() * ScriptWordLen;
            //std::fill_n( itw, sizcptrtbl, 0 );
            //m_datalen += sizcptrtbl;    //Add the length of the table to the scriptdata length value for the header

            ////Write constant strings
            //const streampos befconstdata = m_outf.tellp();
            //for( const auto & constant : m_scrdat.ConstTbl() )
            //{
            //    //Write offset in table 
            //    streampos curpos = m_outf.tellp();
            //    uint16_t curstroffset = (curpos - befconsttbl) / ScriptWordLen;

            //    m_outf.seekp( static_cast<size_t>(befconsttbl) + (cntconst * ScriptWordLen), ios::beg ); //Seek to the const ptr tbl
            //    *itw = curstroffset;            //Add offset to table
            //    m_outf.seekp( curpos, ios::beg ); //Seek back at the position we'll write the string at

            //    //write string
            //    //!#TODO: Convert escaped characters??
            //    itw = std::copy( constant.begin(), constant.end(), itw );
            //    *itw = '\0';
            //    ++itw;
            //    ++cntconst;
            //}

            ////Add some padding bytes if needed (padding is counted in the block's length)
            //utils::AppendPaddingBytes(itw, m_outf.tellp(), ScriptWordLen);

            ////Calculate the size of the constant strings data
            //m_constblksize = m_outf.tellp() - befconstdata;
        }

        /*
            WriteStrings
                Write the strings blocks
        */
        void WriteStrings( outit_t & itw )
        {
            if( m_scrdat.StrTblSet().empty() )
                return;

            size_t          cntstrblk       = 0;
            const streampos befstrptrs      = m_outf.tellp();
            const uint16_t  lengthconstdata = (m_scrdat.ConstTbl().size() * ScriptWordLen) + m_constblksize; //The length of the constant ptr tbl and the constant data!
            const uint16_t  szstringptrtbl = m_nbstrings * ScriptWordLen;
            m_stringblockbeg = static_cast<size_t>(befstrptrs); //Save the starting position of the string data, for later

            if( !m_scrdat.StrTblSet().empty() && m_scrdat.StrTblSet().size() != m_stringblksSizes.size() )
            {
#ifdef _DEBUG
                assert(false);
#endif
                throw std::runtime_error("SSBWriterToFile::WriteStrings(): Mismatch in expected script string blocks to ouput!!");
            }

            for( const auto & strblk : m_scrdat.StrTblSet() )
            {
                //Write each string blocks and save the length of the data into our table for later. 
                //**String block sizes include the ptr table!**
                m_stringblksSizes[cntstrblk] = WriteTableAndStrings( itw, strblk.second, lengthconstdata ) + szstringptrtbl; //We need to count the offset table too!!
                ++cntstrblk;
            }
        }


        /*
            WriteTableAndStrings
                Writes a string block, either the constants' strings or strings' strings
                Returns the length in bytes of the string data, **not counting the ptr table!**
        */
        template<class _CNT_T>
            size_t WriteTableAndStrings( outit_t      & itw,
                                         const _CNT_T & container,              //What contains the strings to write(std container needs begin() end() size() and const_iterator)
                                         size_t         ptrtbloffsebytes = 0 ) //Offset in **bytes** to add to all ptrs in the ptr table
        {
            size_t          cntstr     = 0;
            const streampos befptrs    = m_outf.tellp();
            const uint16_t  sizcptrtbl = (container.size() * ScriptWordLen);
            
            //Reserve pointer table so we can write there as we go
            std::fill_n( itw, sizcptrtbl, 0 );

            //Write strings
            const streampos befdata = m_outf.tellp();
            for( const auto & str : container )
            {
                //Write offset in table 
                streampos curpos = m_outf.tellp();

                m_outf.seekp( static_cast<size_t>(befptrs) + (cntstr * ScriptWordLen), ios::beg ); //Seek to the const ptr tbl
                itw = utils::WriteIntToBytes<uint16_t>( (ptrtbloffsebytes + (curpos - befptrs)), itw );            //Add offset to table
                m_outf.seekp( curpos, ios::beg ); //Seek back at the position we'll write the string at

                //write string
                //!#TODO: Convert escaped characters??
                itw = std::copy( str.begin(), str.end(), itw );
                *itw = '\0'; //Append zero
                ++itw;
                ++cntstr;
            }
            //Add some padding bytes if needed (padding is counted in the block's length)
            utils::AppendPaddingBytes(itw, m_outf.tellp(), ScriptWordLen);

            //Return the size of the constant strings data
            return m_outf.tellp() - befdata;
        }


        /*
            TryConvertToScriptLen
                This will divide the size/offset in bytes by 2, and validate if the result too big for the 16 bits of a word. 
                Throws an exception in that case! Otherwise, just returns the value divided by 2
        */
        inline uint16_t TryConvertToScriptLen( const streampos & lengthinbytes )
        {
            const uint32_t scrlen = lengthinbytes / ScriptWordLen;
            if( scrlen > std::numeric_limits<uint16_t>::max() )
                throw std::runtime_error("SSBWriterToFile::TryConvertToScriptLen(): Constant block size exceeds the length of a 16 bits word!!");
            return static_cast<uint16_t>(scrlen);
        }

    private:
        const pmd2::ScriptedSequence & m_scrdat;
        uint16_t            m_hdrlen; 

        size_t              m_nbstrings;
        vector<uint16_t>    m_stringblksSizes;     //in bytes //The lenghts of all strings blocks for each languages
        uint16_t            m_constblksize;        //in bytes //The length of the constant data block

        size_t              m_datalen;             //in bytes //Length of the Data block in bytes
        size_t              m_constoffset;         //in bytes //Start of the constant block from  start of file
        size_t              m_stringblockbeg;      //in bytes //Start of strings blocks from  start of file
        vector<group_entry> m_grps;

        eOpCodeVersion m_opversion; 
        eGameRegion    m_scrRegion;

        ofstream       m_outf;
    };

//=======================================================================================
//  Functions
//=======================================================================================
    /*
        ParseScript
    */
    pmd2::ScriptedSequence ParseScript(const std::string & scriptfile, eGameRegion gloc, eGameVersion gvers)
    {
        vector<uint8_t> fdata( std::move(utils::io::ReadFileToByteVector(scriptfile)) );
        eOpCodeVersion opvers = eOpCodeVersion::EoS;

        if( gvers == eGameVersion::EoS )
            opvers = eOpCodeVersion::EoS;
        else if( gvers == eGameVersion::EoT || gvers == eGameVersion::EoD )
            opvers = eOpCodeVersion::EoTD;
        else
            throw std::runtime_error("ParseScript(): Wrong game version!!");

        return std::move( SSB_Parser<vector<uint8_t>::const_iterator>(fdata.begin(), fdata.end(), opvers, gloc).Parse() );
    }

    /*
        WriteScript
    */
    void WriteScript( const std::string & scriptfile, const pmd2::ScriptedSequence & scrdat, eGameRegion gloc, eGameVersion gvers )
    {
        eOpCodeVersion opver =  (gvers == eGameVersion::EoS)?
                                    eOpCodeVersion::EoS :
                                (gvers == eGameVersion::EoD || gvers == eGameVersion::EoT)?
                                    eOpCodeVersion::EoTD :
                                    eOpCodeVersion::Invalid;
        SSBWriterTofile(scrdat, gloc, opver).Write(scriptfile);
    }


};
