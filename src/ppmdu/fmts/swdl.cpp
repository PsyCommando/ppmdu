#include "swdl.hpp"
#include <ppmdu/pmd2/pmd2_filetypes.hpp>
#include <types/content_type_analyser.hpp>
#include <dse/dse_containers.hpp>
#include <utils/library_wide.hpp>
#include <iostream>
#include <iomanip>
#include <fstream>
using namespace std;
using namespace filetypes;

namespace filetypes
{
    const ContentTy CnTy_SWDL{"swdl"}; //Content ID db handle
};

namespace DSE
{

    static const uint32_t SWDL_PCMDSpecialSize     = 0xAAAA0000; //Value the pcmd lenght in the SWDL header had to indicate it refers to a master bank of samples.
    static const uint32_t SWDL_PCMDSpecialSizeMask = 0xFFFF0000; //Mask to apply to verify the special size. As the two lower bytes seems to contain some value in some swd files!


//===============================================================================
//  SWDL_Header
//===============================================================================
    /*
        DoesSWDLContainsSamples
            Returns true if the swdl contains sample data.
    */
    bool SWDL_Header::DoesContainsSamples()const
    {
        return (pcmdlen > 0) && 
               ((pcmdlen & SWDL_PCMDSpecialSizeMask) != SWDL_PCMDSpecialSize);
    }

    /*
        IsSWDLSampleBankOnly
            Returns true if the swdl is only a sample bank, without program info.
    */
    bool SWDL_Header::IsSampleBankOnly()const
    {
        return (pcmdlen > 0) && 
               ((pcmdlen & SWDL_PCMDSpecialSizeMask) != SWDL_PCMDSpecialSize) &&
               (nbprgislots == 0);
    }



//===============================================================================
//
//===============================================================================

    class SWDLParser
    {
    public:
        SWDLParser( const std::vector<uint8_t> & src )
            :m_src(src)
        {
        }

        PresetBank Parse()
        {
            //using namespace pmd2::audio;
            ParseHeader();
            ParseMeta();


            //Parse programs + keygroups
            auto pinst = ParsePrograms();

            //Parse pcmd + wavi
            if( m_hdr.pcmdlen != 0 && (m_hdr.pcmdlen & 0xFFFF0000) != SWDL_PCMDSpecialSize )
            {
                auto psmpls = ParseSamples();
                return std::move( PresetBank( move(m_meta), 
                                   move(pinst), 
                                   move(psmpls) ) );
            }
            else
                return std::move( PresetBank( move(m_meta), 
                                   move(pinst) ) );
        }

    private:
        void ParseHeader()
        {
            m_hdr.ReadFromContainer(m_src.begin());
        }

        void ParseMeta()
        {
            m_meta.fname = string( m_hdr.fname.data() );
            m_meta.unk1               = m_hdr.unk1;
            m_meta.unk2               = m_hdr.unk2;
            m_meta.createtime.year    = m_hdr.year;
            m_meta.createtime.month   = m_hdr.month;
            m_meta.createtime.day     = m_hdr.day;
            m_meta.createtime.hour    = m_hdr.hour;
            m_meta.createtime.minute  = m_hdr.minute;
            m_meta.createtime.second  = m_hdr.second;
            m_meta.createtime.centsec = m_hdr.centisec;
            m_meta.nbprgislots        = m_hdr.nbprgislots;
            m_meta.nbwavislots        = m_hdr.nbwavislots;
        }

        std::unique_ptr<ProgramBank> ParsePrograms()
        {
            //using namespace pmd2::audio;

            //Handle keygroups first
            auto kgrps = ParseKeygroups();

            if( utils::LibWide().isLogOn() )
                clog <<"\t== Parsing Programs ==\n";

            //Find the prgi chunk
            auto itprgi = DSE::FindNextChunk( m_src.begin(), m_src.end(), eDSEChunks::prgi );

            //Its possible there are no programs
            if( itprgi == m_src.end() )
            {
                if( utils::LibWide().isLogOn() )
                    clog <<"\t\tNo Programs found!\n";
                return nullptr;
            }

            //Read chunk header
            ChunkHeader prgihdr;
            itprgi = prgihdr.ReadFromContainer( itprgi ); //Move iter after header

            //Read instrument info slots
            vector<ProgramBank::ptrprg_t> prginf( m_hdr.nbprgislots );

            auto itreadprg = itprgi;

            for( auto & infslot : prginf )
            {
                //Read a ptr
                uint16_t prginfblk = utils::ReadIntFromBytes<uint16_t>(itreadprg); //Iterator is incremented

                if( prginfblk != 0 )
                {
                    ProgramInfo curblock;
                    curblock.ReadFromContainer( prginfblk + itprgi );
                    infslot.reset( new ProgramInfo(curblock) );

                    if( utils::LibWide().isLogOn() )
                        clog <<"\tInstrument ID#" <<infslot->m_hdr.id <<":\n" <<*infslot <<"\n";
                }
            }
            if( utils::LibWide().isLogOn() )
                clog << endl;
            return move( unique_ptr<ProgramBank>( new ProgramBank( move(prginf), move(kgrps) ) ) );
        }

        vector<KeyGroup> ParseKeygroups()
        {
            //using namespace pmd2::audio;

            if( utils::LibWide().isLogOn() )
                clog <<"\t== Parsing Keygroups ==\n";

            //Find the KGRP chunk
            auto itkgrp = DSE::FindNextChunk( m_src.begin(), m_src.end(), eDSEChunks::kgrp );

            if( itkgrp == m_src.end() )
            {
                if( utils::LibWide().isLogOn() )
                    clog <<"\t\tNo Keygroups found!\n";
                return vector<KeyGroup>(); //Return empty vector when no keygrp chunk found
            }

            //Get the kgrp chunk's header
            ChunkHeader kgrphdr;
            itkgrp = kgrphdr.ReadFromContainer( itkgrp ); //Move iter after header
            
            vector<KeyGroup> keygroups(kgrphdr.datlen / KeyGroup::size());
            
            //Read all keygroups
            for( auto & grp : keygroups )
            {
                itkgrp = grp.ReadFromContainer(itkgrp);

                if( utils::LibWide().isLogOn() )
                    clog <<"\tKeygroup ID#" <<grp.id <<":\n" <<grp <<"\n";
                
            }

            if( utils::LibWide().isLogOn() )
                clog << endl;

            return move(keygroups);
        }

        std::unique_ptr<SampleBank> ParseSamples()
        {

            //Grab the info on every samples
            vector<SampleBank::smpldata_t> smpldat(std::move( ParseWaviChunk() ));

            //Find the PCMD chunk
            auto itpcmd = DSE::FindNextChunk( m_src.begin(), m_src.end(), eDSEChunks::pcmd );
            
            if( itpcmd == m_src.end() )
                throw std::runtime_error("SWDLParser::ParseSamples(): Couldn't find PCMD chunk!!!!!");

            //Get the pcmd chunk's header
            ChunkHeader pcmdhdr;
            itpcmd = pcmdhdr.ReadFromContainer( itpcmd ); //Move iter after header

            //Grab the samples
            for( size_t cntsmpl = 0; cntsmpl < smpldat.size(); ++cntsmpl )
            {
                const auto & psinfo = smpldat[cntsmpl].pinfo_;
                if( psinfo != nullptr )
                {
                    auto   itsmplbeg = itpcmd + psinfo->smplpos;
                    size_t smpllen   = DSESampleLoopOffsetToBytes( psinfo->loopbeg + psinfo->looplen );
                    smpldat[cntsmpl].pdata_.reset( new vector<uint8_t>( itsmplbeg, 
                                                                        itsmplbeg + smpllen ) );
                }
            }

            return std::unique_ptr<SampleBank>( new SampleBank( move(smpldat) ) );
        }

        vector<SampleBank::smpldata_t> ParseWaviChunk()
        {
            auto itwavi = DSE::FindNextChunk( m_src.begin(), m_src.end(), eDSEChunks::wavi );

            if( itwavi == m_src.end() )
                throw std::runtime_error("SWDLParser::ParseWaviChunk(): Couldn't find wavi chunk !!!!!");

            ChunkHeader wavihdr;
            itwavi = wavihdr.ReadFromContainer( itwavi ); //Move iterator past the header

            //Create the vector with the nb of slots mentioned in the header
            vector<SampleBank::smpldata_t> waviptrs( m_hdr.nbwavislots );
            
            auto itreadptr = itwavi; //Copy the iterator to keep one on the start of the wavi data

            for( auto & ablock : waviptrs )
            {
                //Read a ptr
                uint16_t smplinfoffset = utils::ReadIntFromBytes<uint16_t>(itreadptr); //Iterator is incremented

                if( smplinfoffset != 0 )
                {
                    WavInfo winf;
                    winf.ReadFromContainer( smplinfoffset + itwavi );
                    ablock.pinfo_.reset( new WavInfo(winf) );
                }
            }

            return move(waviptrs);
        }

    private:
        DSE_MetaDataSWDL             m_meta;
        SWDL_Header                  m_hdr;
        const std::vector<uint8_t> & m_src;
    };

//========================================================================================================
//  Functions
//========================================================================================================
    PresetBank ParseSWDL( const std::string & filename )
    {
        if( utils::LibWide().isLogOn() )
        {
            clog <<"--------------------------------------------------------------------------\n"
                 <<"Parsing SWDL \"" <<filename <<"\"\n"
                 <<"--------------------------------------------------------------------------\n";
        }
        return std::move( SWDLParser( utils::io::ReadFileToByteVector( filename ) ).Parse() );
    }


    SWDL_Header ReadSwdlHeader( const std::string & filename )
    {
        ifstream infile( filename, ios::in | ios::binary );
        istreambuf_iterator<char> init(infile);
        SWDL_Header hdr;
        hdr.ReadFromContainer(init);
        return move(hdr);
    }

};

//========================================================================================================
//  swdl_rule
//========================================================================================================
    /*
        swdl_rule
            Rule for identifying a SMDL file. With the ContentTypeHandler!
    */
    class swdl_rule : public filetypes::IContentHandlingRule
    {
    public:
        swdl_rule(){}
        ~swdl_rule(){}

        //Returns the value from the content type enum to represent what this container contains!
        virtual cnt_t getContentType()const
        {
            return filetypes::CnTy_SWDL;
        }

        //Returns an ID number identifying the rule. Its not the index in the storage array,
        // because rules can me added and removed during exec. Thus the need for unique IDs.
        //IDs are assigned on registration of the rule by the handler.
        virtual cntRID_t getRuleID()const                          { return m_myID; }
        virtual void                      setRuleID( cntRID_t id ) { m_myID = id; }

        //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
        //## This method will call "CContentHandler::AnalyseContent()" for each sub-content container found! ##
        //virtual ContentBlock Analyse( vector<uint8_t>::const_iterator   itdatabeg, 
        //                              vector<uint8_t>::const_iterator   itdataend );
        virtual ContentBlock Analyse( const analysis_parameter & parameters )
        {
            using namespace pmd2::filetypes;
            DSE::SWDL_Header headr;
            ContentBlock cb;

            //Read the header
            headr.ReadFromContainer( parameters._itdatabeg );

            //build our content block info 
            cb._startoffset          = 0;
            cb._endoffset            = headr.flen;
            cb._rule_id_that_matched = getRuleID();
            cb._type                 = getContentType();

            return cb;
        }

        //This method is a quick boolean test to determine quickly if this content handling
        // rule matches, without in-depth analysis.
        virtual bool isMatch(  vector<uint8_t>::const_iterator   itdatabeg, 
                               vector<uint8_t>::const_iterator   itdataend,
                               const std::string & filext)
        {
            using namespace pmd2::filetypes;
            return (utils::ReadIntFromBytes<uint32_t>(itdatabeg,false) == DSE::SWDL_MagicNumber);
        }

    private:
        cntRID_t m_myID;
    };

//========================================================================================================
//  swdl_rule_registrator
//========================================================================================================
    /*
        swdl_rule_registrator
            A small singleton that has for only task to register the swdl_rule!
    */
    RuleRegistrator<swdl_rule> RuleRegistrator<swdl_rule>::s_instance;
