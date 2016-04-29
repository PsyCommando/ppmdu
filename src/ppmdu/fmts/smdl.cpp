#include "smdl.hpp"
#include <dse/dse_sequence.hpp>
#include <dse/dse_containers.hpp>
#include <utils/library_wide.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
//#include <thread>
//#include <mutex>
//#include <atomic>
#include <iterator>
#include <map>
#include <cassert>
using namespace std;



namespace DSE
{
//====================================================================================================
// Constants
//====================================================================================================

    //Static values of the Parameters for the Trk Chunk
    //static const uint32_t Trk_Chunk_Param1 = 0x1000000;
    //static const uint32_t Trk_Chunk_Param2 = 0xFF04;
    ////Static values of the Parameters for the Eoc Chunk
    //static const uint32_t Eoc_Chunk_Param1 = Trk_Chunk_Param1;
    //static const uint32_t Eoc_Chunk_Param2 = Trk_Chunk_Param2;

    static const uint32_t EocParam1Default = TrkParam1Default;  //The default value for the parameter 1 value in the eoc chunk header!
    static const uint32_t EocParam2Default = TrkParam2Default;  //The default value for the parameter 2 value in the eoc chunk header!


//====================================================================================================
// Other Definitions
//====================================================================================================

    /************************************************************************
        SongChunk
            The raw song chunk.
            For some reasons, most of the data in this chunk rarely ever 
            changes in-between games or files.. Only the nb of channels and
            tracks does..
    ************************************************************************/
    class SongChunk_v415
    {
    public:
        static const uint32_t SizeNoPadd    = 48; //bytes
        static const uint32_t LenMaxPadding = 16; //bytes
        //Default Values
        static const uint32_t DefUnk1       = 0x1000000;
        static const uint32_t DefUnk2       = 0xFF10;
        static const uint32_t DefUnk3       = 0xFFFFFFB0;
        static const uint16_t DefUnk4       = 0x1;
        static const uint16_t DefTPQN       = 48;
        static const uint16_t DefUnk5       = 0xFF01;
        static const uint32_t DefUnk6       = 0xF000000;
        static const uint32_t DefUnk7       = 0xFFFFFFFF;
        static const uint32_t DefUnk8       = 0x40000000;
        static const uint32_t DefUnk9       = 0x404000;
        static const uint16_t DefUnk10      = 0x200;
        static const uint16_t DefUnk11      = 0x800;
        static const uint32_t DefUnk12      = 0xFFFFFF00;


        unsigned int size()const { return SizeNoPadd + unkpad.size(); }

        uint32_t label   = 0;
        uint32_t unk1    = 0;
        uint32_t unk2    = 0;
        uint32_t unk3    = 0;
        uint16_t unk4    = 0;
        uint16_t tpqn    = 0;
        uint16_t unk5    = 0;
        uint8_t  nbtrks  = 0;
        uint8_t  nbchans = 0;
        uint32_t unk6    = 0;
        uint32_t unk7    = 0;
        uint32_t unk8    = 0;
        uint32_t unk9    = 0;
        uint16_t unk10   = 0;
        uint16_t unk11   = 0;
        uint32_t unk12   = 0;
        std::vector<uint8_t> unkpad;

        //
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( static_cast<uint32_t>(eDSEChunks::song), itwriteto, false ); //Force this, to avoid bad surprises
            itwriteto = utils::WriteIntToBytes( unk1,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk2,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk3,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk4,    itwriteto );
            itwriteto = utils::WriteIntToBytes( tpqn,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk5,    itwriteto );
            itwriteto = utils::WriteIntToBytes( nbtrks,  itwriteto );
            itwriteto = utils::WriteIntToBytes( nbchans, itwriteto );
            itwriteto = utils::WriteIntToBytes( unk6,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk7,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk8,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk9,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk10,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unk11,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unk12,   itwriteto );
            itwriteto = std::copy( unkpad.begin(), unkpad.end(), itwriteto );
            return itwriteto;
        }

        //
        template<class _init>
            _init ReadFromContainer(  _init itReadfrom )
        {
            itReadfrom = utils::ReadIntFromBytes( label,    itReadfrom, false ); //iterator is incremented
            itReadfrom = utils::ReadIntFromBytes( unk1,     itReadfrom); 
            itReadfrom = utils::ReadIntFromBytes( unk2,     itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( unk3,     itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( unk4,     itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( tpqn,     itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( unk5,     itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( nbtrks,   itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( nbchans,  itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( unk6,     itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( unk7,     itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( unk8,     itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( unk9,     itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( unk10,    itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( unk11,    itReadfrom);
            itReadfrom = utils::ReadIntFromBytes( unk12,    itReadfrom);

            for( uint32_t i = 0; i < LenMaxPadding; ++i, ++itReadfrom )
            {
                if( *itReadfrom == 0xFF )
                    unkpad.push_back( 0xFF ); //save on dereferencing the iterator when we already know its value..
                else
                    break;
            }

            return itReadfrom;
        }

        operator SongData()
        {
            SongData sdat;
            sdat.tpqn    = tpqn;
            sdat.nbtrks  = nbtrks;
            sdat.nbchans = nbchans;
            sdat.mainvol = 127;
            sdat.mainpan = 64;
            return move(sdat);
        }
    };

    /*****************************************************
        SongChunk_v402
            For DSE Version 0x402
    *****************************************************/
    class SongChunk_v402
    {
    public:
        static const uint32_t Size = 32; //bytes

        //Default Values
        static const uint32_t DefUnk1       = 0x1000000;
        static const uint32_t DefUnk2       = 0xFF10;
        static const uint32_t DefUnk3       = 0xFFFFFFB0;
        static const uint16_t DefUnk4       = 0x1;
        static const uint16_t DefTPQN       = 48;
        static const uint8_t  DefUnk5       = 0x1;
        static const uint8_t  DefUnk6       = 0x2;
        static const uint16_t DefUnk7       = 0x8;
        static const uint8_t  DefMVol       = 127;
        static const uint8_t  DefMPan       = 64;
        static const uint32_t DefUnk8       = 0x0F000000;

        uint32_t size()const { return Size; }

        uint32_t label   = 0;
        uint32_t unk1    = 0;
        uint32_t unk2    = 0;
        uint32_t unk3    = 0;
        uint16_t unk4    = 0;
        uint16_t tpqn    = 0;
        uint8_t  nbtrks  = 0;
        uint8_t  nbchans = 0;
        uint8_t  unk5    = 0;
        uint8_t  unk6    = 0;
        uint16_t unk7    = 0;
        int8_t   mainvol = 0;
        int8_t   mainpan = 0;
        uint32_t unk8    = 0;

        //
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( static_cast<uint32_t>(eDSEChunks::song), itwriteto, false ); //Force this, to avoid bad surprises
            itwriteto = utils::WriteIntToBytes( unk1,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk2,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk3,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk4,    itwriteto );
            itwriteto = utils::WriteIntToBytes( tpqn,    itwriteto );
            itwriteto = utils::WriteIntToBytes( nbtrks,  itwriteto );
            itwriteto = utils::WriteIntToBytes( nbchans, itwriteto );
            itwriteto = utils::WriteIntToBytes( unk5,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk6,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk7,    itwriteto );
            itwriteto = utils::WriteIntToBytes( mainvol, itwriteto );
            itwriteto = utils::WriteIntToBytes( mainpan, itwriteto );
            itwriteto = utils::WriteIntToBytes( unk8,    itwriteto );
            return itwriteto;
        }

        //
        template<class _init>
            _init ReadFromContainer(  _init itReadfrom )
        {
            itReadfrom = utils::ReadIntFromBytes( label,   itReadfrom, false ); //Force this, to avoid bad surprises
            itReadfrom = utils::ReadIntFromBytes( unk1,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk2,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk3,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk4,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( tpqn,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( nbtrks,  itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( nbchans, itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk5,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk6,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk7,    itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( mainvol, itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( mainpan, itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( unk8,    itReadfrom );
            return itReadfrom;
        }

        operator SongData()
        {
            SongData sdat;
            sdat.tpqn    = tpqn;
            sdat.nbtrks  = nbtrks;
            sdat.nbchans = nbchans;
            sdat.mainvol = mainvol;
            sdat.mainpan = mainpan;
            return move(sdat);
        }
    };

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
            if( utils::LibWide().isLogOn() )
                clog << "=== Parsing SMDL ===\n";
            //Set our iterator
            m_itread = m_itbeg;//m_src.begin();

            //#FIXME: It might have been easier to just check for trk chunks and stop when none are left? We'd save a lot of iteration!!
            m_itEoC  = DSE::FindNextChunk( m_itbeg, m_itend, DSE::eDSEChunks::eoc ); //Our end is either the eoc chunk, or the vector's end

            //Get the headers
            ParseHeader();
            ParseSong();
            DSE::DSE_MetaDataSMDL meta( MakeMeta() );

            //Check version
            if( m_hdr.version == static_cast<uint16_t>(eDSEVersion::V415) )
            {
                //Parse tracks and return
                return std::move( MusicSequence( ParseAllTracks(), std::move(meta) ) );
            }
            else if( m_hdr.version == static_cast<uint16_t>(eDSEVersion::V402) )
            {
                //Parse tracks and return
                return std::move( MusicSequence( ParseAllTracks(), std::move(meta) ) );
            }
            else
            {
#ifdef _DEBUG
                cerr << "SMDL_Parser::operator MusicSequence() : Unsupported DSE version!!";
                assert(false);
#endif
                throw runtime_error( "SMDL_Parser::operator MusicSequence() : Unsupported DSE version!!" );
            }
        }

    private:

        //Parse the SMDL header
        inline void ParseHeader()
        {
            m_itread = m_hdr.ReadFromContainer( m_itread, m_itend );

            if( utils::LibWide().isLogOn() )
            {
                clog << m_hdr;
            }
        }

        //Parse the song chunk
        inline void ParseSong()
        {
            //
            if( m_hdr.version == static_cast<uint16_t>(eDSEVersion::V402) )
            {
                SongChunk_v402 schnk;
                m_itread = schnk.ReadFromContainer( m_itread );
                if( utils::LibWide().isLogOn() )
                    clog << schnk;
                m_song = schnk;
            }
            else if( m_hdr.version == static_cast<uint16_t>(eDSEVersion::V415) )
            {
                SongChunk_v415 schnk;
                m_itread = schnk.ReadFromContainer( m_itread );
                if( utils::LibWide().isLogOn() )
                    clog << schnk;
                m_song = schnk;
            }
            else
            {
                stringstream sstr;
                sstr << "SMDL_Parser::operator MusicSequence() : DSE version 0x" <<hex <<m_hdr.version <<" is unsupported/unknown at the moment!";
                throw runtime_error( sstr.str() );
            }
        }

        DSE::DSE_MetaDataSMDL MakeMeta()
        {
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
            meta.origversion        = intToDseVer( m_hdr.version );
            return move(meta);
        }

        std::vector<MusicTrack> ParseAllTracks()
        {
            vector<MusicTrack> tracks;
            tracks.reserve( m_song.nbtrks );

            try
            {
                size_t cnttrk = 0;
                if( utils::LibWide().isLogOn() )
                    clog << "\t--- Parsing Tracks ---\n";
                while( m_itread != m_itEoC )
                {
                    //#1 - Find next track chunk
                    m_itread = DSE::FindNextChunk( m_itread, m_itEoC, DSE::eDSEChunks::trk );
                    if( m_itread != m_itEoC )
                    {
                        //Parse Track
                        tracks.push_back( ParseTrack() );
                        if( utils::LibWide().isLogOn() )
                            clog << "\t- Track " <<cnttrk <<", parsed " << tracks.back().size() <<" event(s)\n";
                        ++cnttrk;
                    }
                }

                if( utils::LibWide().isLogOn() )
                    clog << "\n\n";
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
            hdr.ReadFromContainer(m_itread, m_itend); //Don't increment itread
            auto itend     = m_itread + (hdr.datlen + DSE::ChunkHeader::size());
            auto itpreread = m_itread;
            m_itread = itend; //move it past the chunk already

            //And skip padding bytes
            for( ;m_itread != m_itEoC && (*m_itread) == static_cast<uint8_t>(eTrkEventCodes::EndOfTrack); ++m_itread );

            auto parsed = DSE::ParseTrkChunk(itpreread, itend);

            MusicTrack mtrk;
            mtrk.SetMidiChannel( parsed.second.chanid );
            mtrk.getEvents() = std::move(parsed.first);

            return std::move(mtrk);
        }

    private:
        rd_iterator_t                   m_itbeg;
        rd_iterator_t                   m_itend;
        rd_iterator_t                   m_itread;
        rd_iterator_t                   m_itEoC;    //Pos of the "end of chunk" chunk
        SMDL_Header                     m_hdr;
        SongData                        m_song;

    };

//====================================================================================================
// SMDL_Writer
//====================================================================================================

    template<class _TargetTy>
        class SMDL_Writer;


    template<>
        class SMDL_Writer<std::ofstream>
    {
    public:
        typedef std::ofstream cnty;

        SMDL_Writer( cnty & tgtcnt, const MusicSequence & srcseq, eDSEVersion dseVersion = eDSEVersion::VDef )
            :m_tgtcn(tgtcnt), m_src(srcseq), m_version(dseVersion)
        {
        }

        void operator()()
        {
            std::ostreambuf_iterator<char> itout(m_tgtcn); 
            std::set<int>                  existingchan;        //Keep track of the nb of unique channels used by the tracks
            size_t                         nbwritten    = 0;

            //Reserve Header
            itout = std::fill_n( itout, SMDL_Header::size(), 0 );
            nbwritten += SMDL_Header::size();

            //Reserve Song chunk
            if( m_version == eDSEVersion::V402 )
            {
                itout = std::fill_n( itout, SongChunk_v402::Size, 0 );
                nbwritten += SongChunk_v402::Size;
            }
            else if( m_version == eDSEVersion::V415 )
            {
                itout = std::fill_n( itout, (SongChunk_v415::SizeNoPadd + SongChunk_v415::LenMaxPadding), 0 );
                nbwritten += (SongChunk_v415::SizeNoPadd + SongChunk_v415::LenMaxPadding);
            }
            else
                throw std::runtime_error( "SMDL_Writer::operator()(): Invalid DSE version supplied!!" );

            if( utils::LibWide().isLogOn() )
            {
                clog << "-------------------------\n"
                     << "Writing SMDL\n"
                     << "-------------------------\n"
                     << m_src.printinfo() 
                     <<"\n"
                     ;
            }

            //Write tracks
            for( uint8_t i = 0; i < m_src.getNbTracks(); ++i )
            {
                if( m_src[i].empty() )
                    continue; //ignore empty tracks

                const DSE::MusicTrack & atrk = m_src[i];
                TrkPreamble preamble;
                preamble.trkid  = i;
                preamble.chanid = atrk.GetMidiChannel();
                preamble.unk1   = 0;
                preamble.unk2   = 0;
                uint32_t lenmod = WriteTrkChunk( itout, preamble, atrk.begin(), atrk.end(), atrk.size() );
                nbwritten += lenmod;

                //Write padding
                lenmod = (lenmod % 4);
                uint32_t lenpad = lenmod;
                if( lenmod != 0 )
                    std::fill_n( itout, lenpad, static_cast<uint8_t>(eTrkEventCodes::EndOfTrack) );

                existingchan.insert( preamble.chanid );
            }

            //Write end chunk
            DSE::ChunkHeader eoc;
            eoc.label  = static_cast<uint32_t>(eDSEChunks::eoc);
            eoc.datlen = 0;
            eoc.param1 = EocParam1Default;
            eoc.param2 = EocParam2Default;
            itout      = eoc.WriteToContainer( itout );

            //Go back to write the header and song chunk!
            size_t flen = static_cast<size_t>(m_tgtcn.tellp());
            m_tgtcn.seekp(0); 
            itout = std::ostreambuf_iterator<char>(m_tgtcn);
            WriteHeader( itout, existingchan, flen );
        }

    private:

        void WriteHeader( std::ostreambuf_iterator<char> & itout, const std::set<int> & existingchan, size_t filelen )
        {
            //Header
            SMDL_Header smdhdr; 
            smdhdr.unk7     = 0;
            smdhdr.flen     = filelen;
            smdhdr.version  = DseVerToInt(m_version);
            smdhdr.unk1     = m_src.metadata().unk1;
            smdhdr.unk2     = m_src.metadata().unk2;
            smdhdr.unk3     = 0;
            smdhdr.unk4     = 0;
            smdhdr.year     = m_src.metadata().createtime.year;
            smdhdr.month    = m_src.metadata().createtime.month;
            smdhdr.day      = m_src.metadata().createtime.day;
            smdhdr.hour     = m_src.metadata().createtime.hour;
            smdhdr.minute   = m_src.metadata().createtime.minute;
            smdhdr.second   = m_src.metadata().createtime.second;
            smdhdr.centisec = m_src.metadata().createtime.centsec;
            std::copy_n( begin(m_src.metadata().fname), smdhdr.fname.size(), begin(smdhdr.fname) );
            smdhdr.unk5     = SMDL_Header::DefUnk5;
            smdhdr.unk6     = SMDL_Header::DefUnk6;
            smdhdr.unk8     = SMDL_Header::DefUnk8;
            smdhdr.unk9     = SMDL_Header::DefUnk9;
            smdhdr.WriteToContainer( itout ); //The correct magic number for SMDL is forced on write, whatever the value in smdhdr.magicn is.

            //Song Chunk
            if( m_version == eDSEVersion::V402 )
            {
                SongChunk_v402 songchnk;
                songchnk.unk1    = SongChunk_v402::DefUnk1;
                songchnk.unk2    = SongChunk_v402::DefUnk2;
                songchnk.unk3    = SongChunk_v402::DefUnk3;
                songchnk.unk4    = SongChunk_v402::DefUnk4;
                songchnk.tpqn    = m_src.metadata().tpqn;
                songchnk.unk5    = SongChunk_v402::DefUnk5;
                songchnk.nbtrks  = static_cast<uint8_t>(m_src.getNbTracks());
                songchnk.nbchans = static_cast<uint8_t>(existingchan.size());
                songchnk.unk6    = SongChunk_v402::DefUnk6;
                songchnk.unk7    = SongChunk_v402::DefUnk7;
                songchnk.mainvol = m_src.metadata().mainvol;
                songchnk.mainpan = m_src.metadata().mainpan;
                songchnk.unk8    = SongChunk_v402::DefUnk8;
                itout = songchnk.WriteToContainer(itout);
            }
            else if( m_version == eDSEVersion::V415 )
            {
                SongChunk_v415 songchnk;
                songchnk.unk1    = SongChunk_v415::DefUnk1;
                songchnk.unk2    = SongChunk_v415::DefUnk2;
                songchnk.unk3    = SongChunk_v415::DefUnk3;
                songchnk.unk4    = SongChunk_v415::DefUnk4;
                songchnk.tpqn    = m_src.metadata().tpqn;
                songchnk.unk5    = SongChunk_v415::DefUnk5;
                songchnk.nbtrks  = static_cast<uint8_t>(m_src.getNbTracks());
                songchnk.nbchans = static_cast<uint8_t>(existingchan.size());
                songchnk.unk6    = SongChunk_v415::DefUnk6;
                songchnk.unk7    = SongChunk_v415::DefUnk7;
                songchnk.unk8    = SongChunk_v415::DefUnk8;
                songchnk.unk9    = SongChunk_v415::DefUnk9;
                songchnk.unk10   = SongChunk_v415::DefUnk10;
                songchnk.unk11   = SongChunk_v415::DefUnk11;
                songchnk.unk12   = SongChunk_v415::DefUnk12;
                std::fill_n( std::back_inserter(songchnk.unkpad), SongChunk_v415::LenMaxPadding, 0xFF );
                itout = songchnk.WriteToContainer(itout); //Correct chunk label is written automatically, bypassing what's in "label"!
            }
        }

    private:
        cnty                & m_tgtcn;
        const MusicSequence & m_src;
        eDSEVersion           m_version;
    };


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
        std::ofstream outf(file, std::ios::out | std::ios::binary );

        if( !outf.is_open() || outf.bad() )
            throw std::runtime_error( "WriteSMDL(): Couldn't open output file " + file );

        SMDL_Writer<std::ofstream>(outf, seq)();
    }

    /***********************************************************************
        operator<< SMDL_Header
    ***********************************************************************/
    std::ostream & operator<<( std::ostream &os, const SMDL_Header & hdr )
    {
        os  << "\t-- SMDL Header --\n" 
            <<showbase
            <<hex <<uppercase
            << "\tmagicn       : " << hdr.magicn    <<"\n"
            << "\tUnk7         : " << hdr.unk7     <<"\n"
            <<dec <<nouppercase
            << "\tFile lenght  : " << hdr.flen     <<" bytes\n"
            <<hex <<uppercase
            << "\tVersion      : " << hdr.version <<"\n"
            << "\tUnk1         : " << static_cast<short>(hdr.unk1)   <<"\n"
            << "\tUnk2         : " << static_cast<short>(hdr.unk2)      <<"\n"
            << "\tUnk3         : " << hdr.unk3     <<"\n"
            << "\tUnk4         : " << hdr.unk4     <<"\n"
            <<dec <<nouppercase
            << "\tYear         : " << hdr.year     <<"\n"
            << "\tMonth        : " << static_cast<short>(hdr.month)     <<"\n"
            << "\tDay          : " << static_cast<short>(hdr.day)     <<"\n"
            << "\tHour         : " << static_cast<short>(hdr.hour)     <<"\n"
            << "\tMinute       : " << static_cast<short>(hdr.minute)     <<"\n"
            << "\tSecond       : " << static_cast<short>(hdr.second)     <<"\n"
            << "\tCentisec     : " << static_cast<short>(hdr.centisec)     <<"\n"
            << "\tFile Name    : " << string( begin(hdr.fname), end(hdr.fname) ) <<"\n"
            <<hex <<uppercase
            << "\tUnk5         : " << hdr.unk5     <<"\n"
            << "\tUnk6         : " << hdr.unk6     <<"\n"
            << "\tUnk8         : " << hdr.unk8     <<"\n"
            << "\tUnk9         : " << hdr.unk9     <<"\n"
            <<dec <<nouppercase
            <<noshowbase
            <<"\n"
            ;
        return os;
    }

    /***********************************************************************
        operator<< SongChunk_v415
    ***********************************************************************/
    std::ostream & operator<<( std::ostream & os, const SongChunk_v415 & sd )
    {
        os  << "\t-- Song Chunk v0x415 --\n" 
            <<showbase
            <<hex <<uppercase
            << "\tLabel        : " << sd.label    <<"\n"
            << "\tUnk1         : " << sd.unk1     <<"\n"
            << "\tUnk2         : " << sd.unk2     <<"\n"
            << "\tUnk3         : " << sd.unk3     <<"\n"
            << "\tUnk4         : " << sd.unk4     <<"\n"
            <<dec <<nouppercase
            << "\tTPQN         : " << sd.tpqn     <<"\n"
            <<hex <<uppercase
            << "\tUnk5         : " << sd.unk5     <<"\n"
            <<dec <<nouppercase
            << "\tNbTracks     : " << static_cast<short>(sd.nbtrks)   <<"\n"
            << "\tNbChans      : " << static_cast<short>(sd.nbchans)  <<"\n"
            <<hex <<uppercase
            << "\tUnk6         : " << sd.unk6     <<"\n"
            << "\tUnk7         : " << sd.unk7     <<"\n"
            << "\tUnk8         : " << sd.unk8     <<"\n"
            << "\tUnk9         : " << sd.unk9     <<"\n"
            << "\tUnk10        : " << sd.unk10    <<"\n"
            << "\tUnk11        : " << sd.unk11    <<"\n"
            << "\tUnk12        : " << sd.unk12    <<"\n"
            <<dec <<nouppercase
            <<noshowbase
            <<"\n"
            ;
        return os;
    }

    /***********************************************************************
        operator<< SongChunk_v402
    ***********************************************************************/
    std::ostream & operator<<( std::ostream & os, const SongChunk_v402 & sd )
    {
        os  << "\t-- Song Chunk v0x402 --\n" 
            <<showbase
            <<hex <<uppercase
            << "\tLabel        : " << sd.label    <<"\n"
            << "\tUnk1         : " << sd.unk1     <<"\n"
            << "\tUnk2         : " << sd.unk2     <<"\n"
            << "\tUnk3         : " << sd.unk3     <<"\n"
            << "\tUnk4         : " << sd.unk4     <<"\n"
            <<dec <<nouppercase
            << "\tTPQN         : " << sd.tpqn     <<"\n"
            << "\tNbTracks     : " << static_cast<short>(sd.nbtrks)   <<"\n"
            << "\tNbChans      : " << static_cast<short>(sd.nbchans)  <<"\n"
            <<hex <<uppercase
            << "\tUnk5         : " << static_cast<short>(sd.unk5)     <<"\n"
            << "\tUnk6         : " << static_cast<short>(sd.unk6)     <<"\n"
            << "\tUnk7         : " << sd.unk7     <<"\n"
            << "\tMainVol      : " << static_cast<short>(sd.mainvol)   <<"\n"
            << "\tMainPan      : " << static_cast<short>(sd.mainpan)  <<"\n"
            << "\tUnk8         : " << sd.unk8     <<"\n"
            <<dec <<nouppercase
            <<noshowbase
            <<"\n"
            ;
        return os;
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