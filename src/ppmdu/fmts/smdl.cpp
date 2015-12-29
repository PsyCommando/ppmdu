#include "smdl.hpp"
#include <dse/dse_sequence.hpp>
#include <dse/dse_containers.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <thread>
#include <mutex>
#include <atomic>
#include <iterator>
using namespace std;



namespace DSE
{
//====================================================================================================
// Constants
//====================================================================================================

    //Static values of the Parameters for the Trk Chunk
    static const uint32_t Trk_Chunk_Param1 = 0x1000000;
    static const uint32_t Trk_Chunk_Param2 = 0xFF04;

//====================================================================================================
// Utility
//====================================================================================================


//====================================================================================================
// SMDL_Parser
//====================================================================================================

    /*
        SMDL_Parser
            Takes a random access iterator as template param.
    */
    template<class _rait = vector<uint8_t>::const_iterator >
        class SMDL_Parser
    {
    public:
        typedef _rait rd_iterator_t;

        SMDL_Parser( const vector<uint8_t> & filedata )
            :m_itbeg(filedata.begin()), m_itend(filedata.end())
        {}

        SMDL_Parser( _rait itbeg, _rait itend )
            :m_itbeg(itbeg), m_itend(itend)
        {}

        operator MusicSequence()
        {
            //Set our iterator
            m_itread = m_itbeg;//m_src.begin();
            m_itEoC  = DSE::FindNextChunk( m_itbeg, m_itend, DSE::eDSEChunks::eoc ); //Our end is either the eoc chunk, or the vector's end

            //Get the headers
            ParseHeader();
            ParseSong();

            //Build Meta-info
            DSE::DSE_MetaDataSMDL meta;
            meta.createtime.year    = m_hdr.year;
            meta.createtime.month   = m_hdr.month;
            meta.createtime.day     = m_hdr.day;
            meta.createtime.hour    = m_hdr.hour;
            meta.createtime.minute  = m_hdr.minute;
            meta.createtime.second  = m_hdr.second;
            meta.createtime.centsec = m_hdr.centisec;
            meta.fname              = string( m_hdr.fname.data());
            meta.unk1               = m_hdr.unk1;
            meta.unk2               = m_hdr.unk2;
            meta.tpqn               = m_song.tpqn;

            //Parse tracks and return
            return std::move( MusicSequence( ParseAllTracks(), std::move(meta) ) );
        }

    private:

        //Parse the SMDL header
        inline void ParseHeader()
        {
            m_itread = m_hdr.ReadFromContainer( m_itread );
            //Skip padding
            std::advance( m_itread, 8 );
        }

        //Parse the song chunk
        inline void ParseSong()
        {
            m_itread = m_song.ReadFromContainer(m_itread);
        }

        std::vector<MusicTrack> ParseAllTracks()
        {
            vector<MusicTrack> tracks;
            tracks.reserve( m_song.nbtrks );

            try
            {
                while( m_itread != m_itEoC )
                {
                    //#1 - Find next track chunk
                    m_itread = DSE::FindNextChunk( m_itread, m_itEoC, DSE::eDSEChunks::trk );
                    if( m_itread != m_itEoC )
                    {
                        //Parse Track
                        tracks.push_back( ParseTrack() );
                    }
                }
            }
            catch( std::runtime_error & e )
            {
                stringstream sstr;
                sstr << e.what() << " Caught exception while parsing track # " <<tracks.size() <<"! ";
                throw std::runtime_error( sstr.str() );
            }

            return std::move(tracks);
        }

        MusicTrack ParseTrack()
        {
            DSE::ChunkHeader      hdr;
            hdr.ReadFromContainer(m_itread); //Don't increment itread
            auto itend     = m_itread + (hdr.datlen + DSE::ChunkHeader::size());
            auto itpreread = m_itread;
            m_itread = itend; //move it past the chunk already
            //And skip padding bytes
            for( ;m_itread != m_itEoC && (*m_itread) == static_cast<uint8_t>(eTrkEventCodes::EndOfTrack); ++m_itread );

            auto parsed = DSE::ParseTrkChunk(itpreread, itend);

            MusicTrack mtrk;
            mtrk.SetMidiChannel( parsed.second.chanid );
            mtrk.getEvents() = move(parsed.first);

            return move(mtrk);
        }

    private:
        rd_iterator_t                   m_itbeg;
        rd_iterator_t                   m_itend;

        rd_iterator_t                   m_itread;
        rd_iterator_t                   m_itEoC;    //Pos of the "end of chunk" chunk
        SMDL_Header                     m_hdr;
        SongChunk                       m_song;

    };

//====================================================================================================
// SMDL_Writer
//====================================================================================================
//====================================================================================================
// Functions
//====================================================================================================

    MusicSequence ParseSMDL( const std::string & file )
    {
        if( utils::LibWide().isLogOn() )
        {
            clog << "================================================================================\n"
                 << "Parsing SMDL " <<file << "\n"
                 << "================================================================================\n";
        }
        return std::move( SMDL_Parser<>( utils::io::ReadFileToByteVector(file) )); //Apparently it being an implicit move isn't enough for MSVC..
    }

    MusicSequence ParseSMDL( std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend )
    {
        return std::move( SMDL_Parser<>( itbeg, itend ));
    }

    void WriteSMDL( const std::string & file, const MusicSequence & seq )
    {
        //# TODO : Write the SMDL writer !
        cerr<<"Not Implemented!\n";
        assert(false);
    }
};

//ppmdu's type analysis system
#ifdef USE_PPMDU_CONTENT_TYPE_ANALYSER

    #include <types/content_type_analyser.hpp>
    //################################### Filetypes Namespace Definitions ###################################
    namespace filetypes
    {
        const ContentTy CnTy_SMDL {"smdl"}; //Content ID db handle

    //========================================================================================================
    //  smdl_rule
    //========================================================================================================
        /*
            smdl_rule
                Rule for identifying a SMDL file. With the ContentTypeHandler!
        */
        class smdl_rule : public IContentHandlingRule
        {
        public:
            smdl_rule(){}
            ~smdl_rule(){}

            //Returns the value from the content type enum to represent what this container contains!
            virtual cnt_t getContentType()const
            {
                return filetypes::CnTy_SMDL;
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
                DSE::SMDL_Header headr;
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
                return (utils::ReadIntFromBytes<uint32_t>(itdatabeg,false) == DSE::SMDL_MagicNumber);
            }

        private:
            cntRID_t m_myID;
        };

    //========================================================================================================
    //  smdl_rule_registrator
    //========================================================================================================
        /*
            smdl_rule_registrator
                A small singleton that has for only task to register the smdl_rule!
        */
        RuleRegistrator<smdl_rule> RuleRegistrator<smdl_rule>::s_instance;

    };
#endif