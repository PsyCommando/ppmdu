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


namespace utils{ namespace io
{
    static const uint32_t RIFF_MagicNumber           = 0x52494646; //"RIFF"
    static const uint32_t RIFF_BigEndian_MagicNumber = 0x52494658; //"RIFX"

    /*******************************************************************************************
        RIFF_Chunk
    *******************************************************************************************/
    class RIFF_Chunk
    {
    public:
        RIFF_Chunk( uint32_t id = 0, uint32_t len = 0 )
            :chunk_id(0),length(0)
        {}

        virtual ~RIFF_Chunk(){}

        /*
            Will determine length itself.
            Return new read position, after the chunk that was parsed.
        */
        template<class _init>
            virtual _init parseChunk( _init itbeg, _init itfileend )
        {

            chunk_id = utils::ReadIntFromByteVector<decltype(chunk_id)>(itbeg);
            length   = utils::ReadIntFromByteVector<decltype(length)>  (itbeg);

            if( length > 0 )
            {
                content.reserve( length );
                _init itdatend = itbeg;
                std::advance( itdatend, length );

                for( _init itdat = itbeg; itdat != itdatend; ++itdat )
                {
                    content.push_back( *itdat );
                }
            }

            return itbeg;
        }

        /*
            Return new write position.
        */
        template<class _outit>
            virtual _outit writeChunk( _outit itwhere )
        {
            itwhere = utils::WriteIntToByteVector(chunk_id, itwhere );
            itwhere = utils::WriteIntToByteVector(length,   itwhere );
            return std::copy( content.begin(), content.end(), itwhere );
        }

        std::vector<uint8_t> content;
        uint32_t             chunk_id;
        uint32_t             length;
    };


    /*******************************************************************************************
        RIFF_Handler
            An object template to be specialized, that is passed as template to the CRIFF_File 
            to handle the file's content.
    *******************************************************************************************/
    class RIFF_Handler
    {
    public:

    private:

    };

    /*******************************************************************************************
        RIFF_File
            Parse a RIFF file as-is chunks by chunks for further processing by more specialized
            code.
    *******************************************************************************************/
    /*template<class _RIFF_Handler>*/
        class RIFF_File 
    {
    public:
        /*typedef _RIFF_Handler handler_t;*/

        RIFF_File()
        {
        }

        void parse( const std::string & file )
        {
            std::vector<uint8_t> fdata  = utils::io::ReadFileToByteVector( file );
            auto                 itread = fdata.begin(); 

            std::clog << "Parsing RIFF file \"" << file <<"\"\n";

            uint32_t magic = utils::ReadIntFromByteVector<uint32_t>(itread);
            if( magic == RIFF_MagicNumber )
            {
                uint32_t lengthtotal = utils::ReadIntFromByteVector<uint32_t>(itread);
                std::clog << "Data length : " << lengthtotal <<"\n";

                ParseChunks( itread, (itread + lengthtotal) );
            }
            else if( magic == RIFF_BigEndian_MagicNumber )
            {
                throw std::runtime_error( "RIFX format not supported!" );
            }
            else
                throw std::runtime_error( "File is not a RIFF file!" );
        }

        void write( const std::string & file )
        {
        }

        /*handler_t                & handler() { return m_handler; }*/
        std::vector<RIFF_Chunk> & chunks()  { return m_chunks;  }

    private:

        template<class _init>
            void ParseChunks( _init itbeg, _init endofdata )
        {
            //Get the content's magic number
            uint32_t datatype    = utils::ReadIntFromByteVector<uint32_t>(itread);

            //Not pretty, but is only for testing
            char *   datatypestr = reinterpret_cast<char*>(&datatype);
            std::clog << "Content type : " 
                      << datatypestr[0] << datatypestr[1] << datatypestr[2] << datatypestr[3] <<"\n";

            while( itbeg != endofdata )
            {
                itbeg = ParseAChunk( itbeg, endofdata );
            }
        }

        template<class _init>
            _init ParseAChunk( _init itbeg, _init endofdata )
        {
            RIFF_Chunk curchunk;
            curchunk.parseChunk( itbeg, endofdata );
            char * cidstr = reinterpret_cast<char*>(&curchunk.chunk_id);

            std::clog << "# Sub-Chunk : " 
                      << cidstr[0] << cidstr[1] << cidstr[2] << cidstr[3] <<"\n"
                      <<"  Size: " <<curchunk.length <<" byte(s)\n";

            /*m_handler.handle(curchunk);*/
            m_chunks.push_back( std::move(curchunk) );
        }

        void WriteChunks( std::back_insert_iterator<std::vector<uint8_t>> itout )
        {
        }

    private:
        /*handler_t                m_handler;*/
        std::vector<RIFF_Chunk> m_chunks;
    };

};};

#endif