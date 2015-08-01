#include "swdl.hpp"
#include <ppmdu/fmts/content_type_analyser.hpp>
#include <ppmdu/utils/library_wide.hpp>
#include <iostream>
#include <iomanip>
using namespace std;

namespace DSE
{

    static const uint32_t SWDL_PCMDSpecialSize = 0xAAAA0000; //Value the pcmd lenght in the SWDL header had to indicate it refers to a master bank of samples.

//
//
//

    class SWDLParser
    {
    public:
        SWDLParser( const std::vector<uint8_t> & src )
            :m_src(src)
        {
        }

        operator pmd2::audio::PresetBank()
        {
            using namespace pmd2::audio;
            ParseHeader();
            ParseMeta();


            //Parse instruments + keygroups
            auto pinst = ParseInstruments();

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

        std::unique_ptr<pmd2::audio::InstrumentBank> ParseInstruments()
        {
            using namespace pmd2::audio;

            //Handle keygroups first
            auto kgrps = ParseKeygroups();

            if( utils::LibWide().isLogOn() )
                clog <<"\t== Parsing Programs ==\n";

            //Find the prgi chunk
            auto itprgi = DSE::FindNextChunk( m_src.begin(), m_src.end(), eDSEChunks::prgi );

            //Its possible there are no instruments
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
            vector<InstrumentBank::ptrinst_t> instinf( m_hdr.nbprgislots );

            auto itreadinst = itprgi;

            for( auto & infslot : instinf )
            {
                //Read a ptr
                uint16_t instinfblk = utils::ReadIntFromByteVector<uint16_t>(itreadinst); //Iterator is incremented

                if( instinfblk != 0 )
                {
                    ProgramInfo curblock;
                    curblock.ReadFromContainer( instinfblk + itprgi );
                    infslot.reset( new ProgramInfo(curblock) );

                    if( utils::LibWide().isLogOn() )
                        clog <<"\tInstrument ID#" <<infslot->m_hdr.id <<":\n" <<*infslot <<"\n";
                }
            }
            if( utils::LibWide().isLogOn() )
                clog << endl;
            return move( unique_ptr<InstrumentBank>( new InstrumentBank( move(instinf), move(kgrps) ) ) );
        }

        vector<pmd2::audio::KeyGroup> ParseKeygroups()
        {
            using namespace pmd2::audio;

            if( utils::LibWide().isLogOn() )
                clog <<"\t== Parsing Keygroups ==\n";

            //Find the KGRP chunk
            auto itkgrp = DSE::FindNextChunk( m_src.begin(), m_src.end(), eDSEChunks::kgrp );

            if( itkgrp == m_src.end() )
            {
                if( utils::LibWide().isLogOn() )
                    clog <<"\t\tNo Keygroups found!\n";
                return vector<pmd2::audio::KeyGroup>(); //Return empty vector when no keygrp chunk found
            }

            //Get the kgrp chunk's header
            ChunkHeader kgrphdr;
            itkgrp = kgrphdr.ReadFromContainer( itkgrp ); //Move iter after header
            
            vector<pmd2::audio::KeyGroup> keygroups(kgrphdr.datlen / KeyGroup::size());
            
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

        std::unique_ptr<pmd2::audio::SampleBank> ParseSamples()
        {
            using pmd2::audio::SampleBank;

            //Grab the info on every samples
            vector<SampleBank::smpldata_t> smpldat(std::move( ParseWaviChunk() ));

            //Find the PCMD chunk
            auto itpcmd = DSE::FindNextChunk( m_src.begin(), m_src.end(), eDSEChunks::pcmd );
            
            if( itpcmd == m_src.end() )
                throw std::runtime_error("SWDLParser::ParseSamples(): Couldn't find PCMD chunk!!!!!");

            //Get the pcmd chunk's header
            ChunkHeader pcmdhdr;
            itpcmd = pcmdhdr.ReadFromContainer( itpcmd ); //Move iter after header

            //Grab each samples from the pcmd chunk
            //map<size_t,SampleBank::smpldata_t> smpldat;


            //smpldat.reserve(winf.size());

            //Build sample bounds table
            map<size_t,vector<uint8_t>::const_iterator> boundmap; //Each key is the sample that own this sample start location
            //smplbounds.reserve(winf.size() + 1);

            for( size_t cntsmpl = 0; cntsmpl < smpldat.size(); ++cntsmpl )
            {
                if( smpldat[cntsmpl].pinfo_ != nullptr )
                    boundmap.emplace( cntsmpl, (itpcmd + (smpldat[cntsmpl].pinfo_->smplpos) ) );
            }

            //Append the end of the pcmd chunk to grab the last sample
            //smplbounds.push_back( ( itpcmd + pcmdhdr.datlen) );

            //Read the raw sample data, using the sample bounds
            auto itbounds = boundmap.begin();
            for( size_t i = 0; i < boundmap.size(); ++i, ++itbounds )//( auto it = boundmap.begin(); it != boundmap.end(); ++it ) 
            {
                auto nextit = itbounds;
                ++nextit;

                if( nextit != boundmap.end() )
                    smpldat[itbounds->first].pdata_.reset( new vector<uint8_t>( itbounds->second, nextit->second ) ); 
                else //For the last element before the end, use the end of the chunk as terminator
                    smpldat[itbounds->first].pdata_.reset( new vector<uint8_t>( itbounds->second, (itpcmd + pcmdhdr.datlen) ) );
            }

            return std::unique_ptr<SampleBank>( new SampleBank( move(smpldat) ) );
        }

        vector<pmd2::audio::SampleBank::smpldata_t> ParseWaviChunk()
        {
            auto itwavi = DSE::FindNextChunk( m_src.begin(), m_src.end(), eDSEChunks::wavi );

            if( itwavi == m_src.end() )
                throw std::runtime_error("SWDLParser::ParseWaviChunk(): Couldn't find wavi chunk !!!!!");

            ChunkHeader wavihdr;
            itwavi = wavihdr.ReadFromContainer( itwavi ); //Move iterator past the header

            //Create the vector with the nb of slots mentioned in the header
            vector<pmd2::audio::SampleBank::smpldata_t> waviptrs( m_hdr.nbwavislots );
            
            auto itreadptr = itwavi; //Copy the iterator to keep one on the start of the wavi data

            for( auto & ablock : waviptrs )
            {
                //Read a ptr
                uint16_t smplinfoffset = utils::ReadIntFromByteVector<uint16_t>(itreadptr); //Iterator is incremented

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
        DSE_MetaBank                 m_meta;
        SWDL_Header                  m_hdr;
        const std::vector<uint8_t> & m_src;
    };

//========================================================================================================
//  Functions
//========================================================================================================
    pmd2::audio::PresetBank ParseSWDL( const std::string & filename )
    {
        if( utils::LibWide().isLogOn() )
        {
            clog <<"--------------------------------------------------------------------------\n"
                 <<"Parsing SWDL \"" <<filename <<"\"\n"
                 <<"--------------------------------------------------------------------------\n";
        }
        return std::move( SWDLParser( utils::io::ReadFileToByteVector( filename ) ).operator pmd2::audio::PresetBank() );
    }
};

//========================================================================================================
//  swdl_rule
//========================================================================================================
    /*
        swdl_rule
            Rule for identifying a SMDL file. With the ContentTypeHandler!
    */
    class swdl_rule : public pmd2::filetypes::IContentHandlingRule
    {
    public:
        swdl_rule(){}
        ~swdl_rule(){}

        //Returns the value from the content type enum to represent what this container contains!
        virtual pmd2::filetypes::e_ContentType getContentType()const
        {
            return pmd2::filetypes::e_ContentType::SWDL_FILE;
        }

        //Returns an ID number identifying the rule. Its not the index in the storage array,
        // because rules can me added and removed during exec. Thus the need for unique IDs.
        //IDs are assigned on registration of the rule by the handler.
        virtual pmd2::filetypes::cntRID_t getRuleID()const                          { return m_myID; }
        virtual void                      setRuleID( pmd2::filetypes::cntRID_t id ) { m_myID = id; }

        //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
        //## This method will call "CContentHandler::AnalyseContent()" for each sub-content container found! ##
        //virtual ContentBlock Analyse( types::constitbyte_t   itdatabeg, 
        //                              types::constitbyte_t   itdataend );
        virtual pmd2::filetypes::ContentBlock Analyse( const pmd2::filetypes::analysis_parameter & parameters )
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
        virtual bool isMatch(  pmd2::types::constitbyte_t   itdatabeg, 
                                pmd2::types::constitbyte_t   itdataend,
                               const std::string & filext)
        {
            using namespace pmd2::filetypes;
            return (utils::ReadIntFromByteVector<uint32_t>(itdatabeg,false) == DSE::SWDL_MagicNumber);
        }

    private:
        pmd2::filetypes::cntRID_t m_myID;
    };

//========================================================================================================
//  swdl_rule_registrator
//========================================================================================================
    /*
        swdl_rule_registrator
            A small singleton that has for only task to register the swdl_rule!
    */
    pmd2::filetypes::RuleRegistrator<swdl_rule> pmd2::filetypes::RuleRegistrator<swdl_rule>::s_instance;
