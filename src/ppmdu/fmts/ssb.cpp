#include "ssb.hpp"
#include <ppmdu/pmd2/pmd2_scripts.hpp>
#include <ppmdu/pmd2/pmd2_scripts_opcodes.hpp>
#include <utils/utility.hpp>
#include <utils/library_wide.hpp>
#include <iostream>
#include <sstream>

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

//=======================================================================================
//  SSB Parser
//=======================================================================================

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


    /*
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
                cout<<"SSB_Parser::ParseHeader(): Japanese handling not implemented yet!\n";
                assert(false);
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
            //m_codeoffset    = header_t::LEN + ssb_data_hdr::LEN + (m_dathdr.nbgrps * group_entry::LEN);
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

                    if( opcodedata->nbparams >= 0 )
                    {
                        const uint16_t nbparams = static_cast<uint16_t>(opcodedata->nbparams);
                        for( size_t cntparam = 0; cntparam < nbparams; ++cntparam )
                            inst.parameters.push_back( utils::ReadIntFromBytes<uint16_t>(m_cur, itendseq) );
                        foffset += nbparams * ScriptWordLen;
                    }
                    else
                    {
                        clog << "\n<!>- Found instruction with -1 parameter number in this script! Offset 0x" <<hex <<uppercase <<foffset <<dec <<nouppercase <<"\n";
                    }
                    sequence.push_back(std::move(inst));
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
            const size_t strlutlen = (m_nbstrs * 2); // In the file, the offset for each constants in the constant table includes the 
                                                     // length of the string lookup table(string pointers). Here, to compensate
                                                     // we subtract the length of the string LUT from each pointer read.
            m_out.ConstTbl() = std::move(ParseOffsetTblAndStrings<ScriptedSequence::consttbl_t>( m_constoffset, m_constoffset, m_nbconsts, strlutlen ));
        }

        void ParseStrings()
        {
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
    void WriteScript( const std::string & scriptfile, const pmd2::ScriptedSequence & scrdat )
    {
        assert(false);
    }


};
