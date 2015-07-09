#ifndef RIFF_HPP
#define RIFF_HPP
/*
riff.hpp
2015/05/20
psycommando@gmail.com
Description: Utilities for working with RIFF files.
*/
#include <ppmdu/utils/utility.hpp>
#include <string>
#include <vector>
#include <cstdint>
#include <iostream>


namespace riff
{
    //static const uint32_t RIFF_MagicNumber           = 0x52494646; //"RIFF"
    //static const uint32_t RIFF_BigEndian_MagicNumber = 0x52494658; //"RIFX"

    enum struct eChunkIDs : uint32_t
    {
        RIFF  = 0x52494646, //"RIFF"
        RIFFX = 0x52494658, //"RIFX" Big-endian RIFF format
        LIST  = 0x4C495354, //"LIST"
        JUNK  = 0x4A554E4B, //"JUNK" Basically padding chunks to skip completely
    };


    /************************************************************************************
        ChunkHeader
            Represents a simple RIFF Chunk Header.
    ************************************************************************************/
    struct ChunkHeader
    {
        static const uint32_t SIZE = 4;//bytes
        uint32_t chunk_id = 0;
        uint32_t length   = 0;

        /*
            Return new read position, after the chunk header that was parsed.
        */
        template<class _init>
            _init Read( _init itbeg, _init itfileend )
        {

            chunk_id = utils::ReadIntFromByteVector<decltype(chunk_id)>(itbeg, false);
            length   = utils::ReadIntFromByteVector<decltype(length)>  (itbeg);

            return itbeg;
        }

        /*
            Return new write position.
        */
        template<class _outit>
            _outit Write( _outit itwhere )
        {
            itwhere = utils::WriteIntToByteVector(chunk_id, itwhere, false );
            itwhere = utils::WriteIntToByteVector(length,   itwhere );
            return itwhere;
        }

    };


    /*
        chunkinfo 
            A struct containing details on the position and content of a chunk within the RIFF structure.
    */
    struct ChunkInfo
    {
        ChunkInfo()
            :pos(0)
        {}

        ChunkHeader            hdr;
        size_t                 pos;
        std::vector<ChunkInfo> subchunks;
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
            itread = utils::ReadIntFromByteContainer( m_fmt, itread, false );
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
            cinf.pos = curoffset;

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