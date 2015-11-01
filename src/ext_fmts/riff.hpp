#ifndef RIFF_HPP
#define RIFF_HPP
/*
riff.hpp
2015/05/20
psycommando@gmail.com
Description: Utilities for working with RIFF files.
*/
#include <utils/utility.hpp>
#include <string>
#include <vector>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <sstream>

namespace riff
{

    enum struct eChunkIDs : uint32_t
    {
        RIFF  = 0x52494646, //"RIFF"
        RIFX  = 0x52494658, //"RIFX" Big-endian RIFF format
        LIST  = 0x4C495354, //"LIST"
        JUNK  = 0x4A554E4B, //"JUNK" Basically padding chunks to skip completely
    };

    inline bool ChunkContainsSubChunks( uint32_t chunkid )
    {
        return (chunkid == static_cast<uint32_t>(eChunkIDs::LIST) || 
                chunkid == static_cast<uint32_t>(eChunkIDs::RIFF) || 
                chunkid == static_cast<uint32_t>(eChunkIDs::RIFX) );
    }


    /************************************************************************************
        ChunkHeader
            Represents a simple RIFF Chunk Header.
    ************************************************************************************/
    struct ChunkHeader
    {
        ChunkHeader( uint32_t chnkid = 0, uint32_t len = 0 )
            :chunk_id(chnkid), length(len)
        {}

        static const uint32_t SIZE = 8;//bytes
        uint32_t chunk_id = 0;
        uint32_t length   = 0;

        /*
            Return new read position, after the chunk header that was parsed.
        */
        template<class _init>
            _init Read( _init itbeg )
        {
            chunk_id = utils::ReadIntFromBytes<decltype(chunk_id)>(itbeg, false);
            length   = utils::ReadIntFromBytes<decltype(length)>  (itbeg);
            return itbeg;
        }

        /*
            Return new write position.
        */
        template<class _outit>
            _outit Write( _outit itwhere )
        {
            itwhere = utils::WriteIntToBytes(chunk_id, itwhere, false );
            itwhere = utils::WriteIntToBytes(length,   itwhere );
            return itwhere;
        }

    };

//===========================================================================
//  RIFF Container
//===========================================================================

    /*******************************************************************
        Chunk
            Represent a regular RIFF chunk and its content.
    *******************************************************************/
    class Chunk
    {
    public:

        Chunk( uint32_t fourcc = 0, uint32_t formatid = 0 )
            :fourcc_(fourcc), fmtid_(formatid)
        {}

        Chunk( uint32_t fourcc, std::vector<uint8_t> && data )
            :fourcc_(fourcc), data_(std::move(data)), fmtid_(0)
        {}

        Chunk( uint32_t fourcc, std::vector<Chunk> && subchunks, uint32_t formatid = 0 )
            :fourcc_(fourcc), subchunks_(std::move(subchunks)), fmtid_(formatid)
        {}

        virtual ~Chunk()
        {}

        //#TODO: It might be a good thing to put an abstraction layer over the data of a chunk
        //       using iterators maybe ? That way, if the chunk is loaded from file, we don't have to load
        //       it completely, and the parent of the chunk will insure that the file stream is kept valid.
        //
        //       We really have 3 cases to take into account:
        //       1- Chunk is loaded from a file.
        //       2- Chunk is being written to a file.
        //          - However, this means that, we can't just edit chunks once they've been added to the riff file object, because we'd 
        //            end up overwriting other chunks if we'd add more data and etc, and it would frankly be a mess..
        //       3- Chunk is being written to memory.
        //

        /*
            ShouldParseSubChunks
                This determines whether the chunk will be parsed as having sub-chunks.
        */
        virtual bool ShouldParseSubChunks()const
        {
            return ChunkContainsSubChunks(fourcc_);
        }

        /*
            Reads the whole chunk from a source.
            Return read position.
            - itbeg     : Beginning of the chunk's header.
            - itfileend : End of the entire RIFF container. Meant to be used to validate the header.
        */
        template<class _init>
            _init Read( _init itbeg, _init itfileend )
        {
            ChunkHeader hdr;
            itbeg = hdr.Read( itbeg );
            fourcc_ = hdr.chunk_id; //Fill our fourcc

            //Check if we're a standard sub-chunk containing chunk
            if( ShouldParseSubChunks() )
            {
                auto itchnkend = itbeg;
                std::advance(itchnkend, hdr.length);

                //Read format tag 
                fmtid_ = utils::ReadIntFromBytes<decltype(fmtid_)>(itbeg, false);

                //Read all sub-chunks
                while( itbeg != itchnkend )
                    itbeg = Read( itbeg, itchnkend );
            }
            else
            {
                //Read all the data
                data_.resize( hdr.length );
                size_t cntb = 0;
                for( ; cntb < hdr.length && itbeg != itfileend; ++cntb, ++itbeg )
                    data_[cntb] = (*itbeg);

                if( itbeg == itfileend && cntb < hdr.length )
                {
                    std::stringstream sstr;
                    sstr << "riff::Chunk::Read(): Error, encountered end of container before the expected end of the chunk! Expected length "
                         << hdr.length <<" bytes. Actual length " <<cntb <<" bytes!";
                    throw std::runtime_error( sstr.str() );
                }

            }
            return itbeg;
        }

        /*
            Return new write position.
        */
        template<class _outit>
            _outit Write( _outit itwhere )const
        {
            itwhere = ChunkHeader( fourcc_, ComputeChnkDataLength() ).Write( itwhere );

            //Write our content
            if( !subchunks_.empty() )
            {
                //write format tag
                itwhere = utils::WriteIntToBytes( fmtid_, itwhere, false );

                //write content
                for( const auto & achunk : subchunks_ )
                    itwhere = achunk.Write(itwhere);
            }
            else if( HasSubChunks() && subchunks_.empty() )
            {
                std::clog << "<!>- Warning, writing a standard chunk requiring to contain sub-chunks, while it contains no sub-chunks!\n";
            }
            else //Simple data chunk
                return std::copy( data_.begin(), data_.end(), itwhere );

            return itwhere;
        }

        /*
            Returns whether the chunk contains sub-chunks.
        */
        inline bool HasSubChunks()const { return !subchunks_.empty(); }

        /*
            Returns whether the chunk has a format tag!
        */
        inline bool HasFormatTag()const { return fmtid_ != 0; }

        /*
            ComputeChnkDataLength
                Calculates the size of the contained data for this chunk!
        */
        virtual size_t ComputeChnkDataLength()const
        {
            size_t totalsz = 0;

            if( HasSubChunks() )
            {
                totalsz += sizeof(fmtid_); //Add the format tag!
                for( const auto & achnk : subchunks_ )
                    totalsz += achnk.ComputeChnkDataLength() + ChunkHeader::SIZE; //Count the headers too!! 
            }
            else
                totalsz += data_.size();

            return totalsz;
        }

        uint32_t             fourcc_;       //The chunk id
        uint32_t             fmtid_;        //If the chunk contains sub-chunks, it has a format id tag
        std::vector<uint8_t> data_;         //If its a simple data chunk, its content is stored here
        std::vector<Chunk>   subchunks_;    //If its a chunk containing sub-chunks, its content is stored here!
    };


    /*******************************************************************
        RIFF_Container
            Represents the full content of a RIFF file in memory.
    *******************************************************************/
    class RIFF_Container : public Chunk
    {
    public:
        RIFF_Container( uint32_t formatid = 0 )
            :Chunk( static_cast<uint32_t>(eChunkIDs::RIFF), formatid )
        {}

        virtual ~RIFF_Container()
        {}

        /*
            Reads the whole chunk from a source.
            Return read position.
            - itbeg     : Beginning of the chunk's header.
            - itfileend : End of the entire RIFF container. Meant to be used to validate the header.
        */
        template<class _init>
            _init ReadAndValidate( _init itbeg, _init itfileend )
        {
            ChunkHeader hdr;
            hdr.Read( itbeg );

            //Validate
            if( hdr.chunk_id != static_cast<uint32_t>(eChunkIDs::RIFF) )
            {
                if( hdr.chunk_id == static_cast<uint32_t>(eChunkIDs::RIFX) )
                    throw std::runtime_error("RIFF_Container::Read(): RIFX big-endian RIFF format not supported yet!");
                else
                    throw std::runtime_error("RIFF_Container::Read(): Invalid RIFF container header!");
            }

            auto flen = std::distance( itbeg, itfileend );
            if( flen < (hdr.length + ChunkHeader::SIZE) )
            {
                throw std::runtime_error("RIFF_Container::Read(): Unexpected end of data encountered while parsing RIFF header!");
            }

            //Parse everything
            return Chunk::Read( itbeg, itfileend );
        }

        /*
            Get/Set the content id tag, right before the fmt chunk.
        */
        inline uint32_t FormatID()const            { return fmtid_; }
        inline void     FormatID( uint32_t fmtid ) { fmtid_ = fmtid; }
    };


//===========================================================================
//  RIFF Analysis
//===========================================================================

    /*******************************************************************
        ChunkInfo 
            A struct containing details on the position and content of a chunk within the RIFF structure.
    *******************************************************************/
    struct ChunkInfo
    {
        ChunkInfo()
            :fileoffset(0)
        {}

        ChunkHeader            hdr;         //The header of the chunk
        size_t                 fileoffset;  //The beginning of the chunk's header in the file!
        std::vector<ChunkInfo> subchunks;   //Any subchunks the chunk might have, if its a LIST or RIFF chunk!
    };


    /*******************************************************************************************
        RIFFMap
            A class that maps the structure of a riff structure's content, without loading 
            the content fully in-memory. 
    *******************************************************************************************/
    template<class _init>
        class RIFFMap
    {
    public:
        typedef _init    inputit_t;
        typedef size_t   offset_t;
        typedef uint32_t fmtid_t;

        //STDLib equired typedefs
        typedef std::vector<ChunkInfo>::iterator       iterator;
        typedef std::vector<ChunkInfo>::const_iterator const_iterator;

        RIFFMap() 
            :m_fmt(0)
        {}

        /*
            beg = beginning of the structure containing the riff file's data.
            end = end of the structure containing the riff file's data
        */
        RIFFMap( _init beg, _init end )
            :m_beg(beg), m_end(end), m_fmt(0)
        {}

        //Analyze the RIFF structure
        void analyze()
        {
            auto     itread    = m_beg;
            offset_t curoffset = 0;     //Keep track of the file offset to avoid doing std::distance calls all the time..

            //Read the RIFF header
            ChunkHeader riffhdr;
            itread = riffhdr.Read(itread);
            curoffset += ChunkHeader::SIZE;

            if( riffhdr.chunk_id != eChunkIDs::RIFF && riffhdr.chunk_id != eChunkIDs::RIFFX )
                throw std::runtime_error( "RIFFMap::analyze(): Unrecognized RIFF header!" );
            else if( riffhdr.chunk_id == eChunkIDs::RIFFX )
                throw std::runtime_error( "RIFFMap::analyze(): RIFFX(Big endian RIFF) format not supported." );

            //Read the content type tag
            itread = utils::ReadIntFromBytes( m_fmt, itread, false );
            curoffset += sizeof(m_fmt);

            //Make a list of all the contained chunks
            while( itread != m_end )
                m_chunks.push_back( ParseAChunk(itread,curoffset) );
        }

        inline iterator                       begin()            { return m_chunks.begin(); }
        inline const_iterator                 begin()const       { return m_chunks.begin(); }
        inline iterator                       end()              { return m_chunks.end();   }
        inline const_iterator                 end()const         { return m_chunks.end();   }
        inline std::vector<ChunkInfo>       & chunks()           { return m_chunks; }
        inline const std::vector<ChunkInfo> & chunks()const      { return m_chunks; }
        inline fmtid_t                        GetFormatID()const { return m_fmt; }

    private:

        ChunkInfo ParseAChunk( inputit_t & itread, offset_t & curoffset ) //Iter is incremented by ref
        {
            ChunkInfo   cinf;
            itread     = cinf.hdr.Read(itread);
            curoffset += ChunkHeader::SIZE;

            //Mark our position
            cinf.fileoffset = curoffset;

            //Handle LIST chunks differently to list their content.
            if( cinf.hdr.chunk_id == eChunkIDs::LIST )
            {
                //Get an iterator to the end of the chunk
                auto itchnkend = itread;
                std::advance( itchnkend, cinf.hdr.length );

                //Recursively look for known chunks that have sub-chunks
                while( itread != itchnkend && itread != m_end )
                    cinf.subchunks.push_back( ParseAChunk( itread, curoffset ) );
            }
            else
            {
                //Jump to after the end of the chunk
                std::advance( itread, cinf.hdr.length );
                curoffset += cinf.hdr.length;
            }

            return std::move(cinf);
        }


    private:
        inputit_t              m_beg;
        inputit_t              m_end;
        fmtid_t                m_fmt;
        std::vector<ChunkInfo> m_chunks;
    };

};

#endif