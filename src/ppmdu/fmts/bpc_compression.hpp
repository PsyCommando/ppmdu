#ifndef BPC_COMPRESSION_HPP
#define BPC_COMPRESSION_HPP
/*
bpc_compression.hpp
2016/09/30
psycommando@gmail.com
Description: 
*/
#include <utils/gbyteutils.hpp>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <iterator>
#include <deque>

namespace bpc_compression
{

    /*
        BPCImgDecompressor
    */
    template<class _init, class _outit>
        class BPCImgDecompressor
    {
        static const uint8_t CMD_CyclePatternAndCp                      = 0xE0;
        static const uint8_t CMD_CyclePatternAndCp_NbCpNextByte         = 0xFF;

        static const uint8_t CMD_UseLastPatternAndCp                    = 0xC0;
        static const uint8_t CMD_UseLastPatternAndCp_NbCpNextByte       = 0xDF;

        static const uint8_t CMD_LoadByteAsPatternAndCp                 = 0x80;
        static const uint8_t CMD_LoadByteAsPatternAndCp_NbCpNextByte    = 0xBF;
        

        static const uint8_t CMD_LoadNextByteAsNbToCopy = 0x7E;
        static const uint8_t CMD_LoadNextWordAsNbToCopy = 0x7F;

        static const  size_t SizePreAlloc = 16;
    public:
        typedef _init  init_t;
        typedef _outit outit_t;

        struct DecompState
        {
            bool        bhasleftover = false; //Whether there is a leftover word to handle writing!
            //The buffer storing the patterns to copy
            uint16_t    wordbuf   = 0;   //r2
            uint8_t     hbyte     = 0;   //r7
            uint8_t     cachedhby = 0;   //r13_14h
        };

        BPCImgDecompressor( _init & itbeg, _init itend, size_t decomplen )
            :m_itcur(itbeg), m_itend(itend), m_decomplen(decomplen),m_bytesoutput(0),m_bytesread(0),m_pitw(nullptr)
        {
        }

        void operator()(outit_t & destout)
        {
            m_bytesoutput = 0;
            m_bytesread   = 0;
            m_pitw = &destout;
            for( ; m_itcur != m_itend && m_bytesoutput < m_decomplen;  )
                Process();

            clog<<"Compressed img length : 0x" <<hex <<uppercase <<m_bytesread <<nouppercase <<dec <<"\n";

            if(m_itcur == m_itend )
                throw std::runtime_error("BPCImgDecompressor::operator()(): Reached the end of input data unexpectedly!");

            if( (m_bytesread % 2) != 0 )
                std::advance(m_itcur, 1);
        }

        inline size_t GetNbBytesOutput()const { return m_bytesoutput; }
        inline size_t GetNbBytesRead()const   { return m_bytesread; }

    private:

        inline uint8_t ReadByteFromSrc()
        {
            if( m_itcur == m_itend )
                throw std::runtime_error("BPCImgDecompressor::ReadByteFromSrc(): Unexpectedly reached end of input data!");
            uint8_t val = *m_itcur;
            ++m_itcur;
            ++m_bytesread;
            return val;
        }

        inline void WriteWord( uint16_t w )
        {
            assert(m_pitw);
            utils::WriteIntToBytes(w, (*m_pitw));
            m_bytesoutput += 2;
        }

        inline bool IsBufferedPatternOp(uint8_t cmdby)const { return (cmdby >= CMD_LoadByteAsPatternAndCp); }
        inline bool IsLoadingPatternFromNextByte(uint8_t cmdby)const {return (cmdby >= CMD_LoadByteAsPatternAndCp && cmdby < CMD_UseLastPatternAndCp); }

        int DetermineLenToOutput(uint8_t cmdbyte) 
        {
            int nbtocp = 0;
            switch(cmdbyte)
            {
                case CMD_CyclePatternAndCp_NbCpNextByte:
                case CMD_UseLastPatternAndCp_NbCpNextByte:
                case CMD_LoadByteAsPatternAndCp_NbCpNextByte:
                case CMD_LoadNextByteAsNbToCopy:
                {
                    nbtocp = ReadByteFromSrc();
                    break;
                }
                case CMD_LoadNextWordAsNbToCopy:
                {
                    uint8_t lowbyte = ReadByteFromSrc();
                    nbtocp = lowbyte | (ReadByteFromSrc() << 8);
                    break;
                }
                default:
                {
                    nbtocp = cmdbyte;
                    if(cmdbyte >= CMD_CyclePatternAndCp)
                        nbtocp -= CMD_CyclePatternAndCp;
                    else if(cmdbyte >= CMD_UseLastPatternAndCp)
                        nbtocp -= CMD_UseLastPatternAndCp;
                    else if(cmdbyte >= CMD_LoadByteAsPatternAndCp)
                        nbtocp -= CMD_LoadByteAsPatternAndCp;
                }
            };

            //when we have a leftover word, we subtract one word automatically
            if(m_state.bhasleftover)
                nbtocp -= 1;

            return nbtocp;
        }

        inline bool ShouldCycleBytePattern(uint8_t cmdbyte)const
        {
            return IsLoadingPatternFromNextByte(cmdbyte) || (cmdbyte >= CMD_CyclePatternAndCp && cmdbyte <= CMD_CyclePatternAndCp_NbCpNextByte);
        }

        inline void CycleHighBytePattern()
        {
            uint8_t tmp       = m_state.cachedhby;
            m_state.cachedhby = m_state.hbyte;
            m_state.hbyte     = tmp;
        }

        void Process()
        {
            uint8_t cmd        = ReadByteFromSrc();
            int     nbwordsout = DetermineLenToOutput(cmd);

            //Cycle the pattern bytes and or load a new pattern byte if needed, before the leftover byte
            if(ShouldCycleBytePattern(cmd))
                CycleHighBytePattern();
            if( IsLoadingPatternFromNextByte(cmd) )
                m_state.hbyte = ReadByteFromSrc();

            //Then check for leftover bytes patterns to add
            if(m_state.bhasleftover)
            {
                uint16_t pattern = 0;
                if(IsBufferedPatternOp(cmd))
                    pattern = (m_state.wordbuf) | (m_state.hbyte << 8);
                else
                    pattern = (m_state.wordbuf) | (ReadByteFromSrc() << 8);
                WriteWord(pattern);
                m_state.bhasleftover = false;
            }

            //Run the main operation only if we have a non-negative nb of bytes to copy.
            if( nbwordsout >= 0 )
                HandleMainOp(cmd, nbwordsout);
        }

        void HandleMainOp( uint8_t cmd, size_t nbtocopy )
        {
            //Handle the main operation now
            size_t cntcopy = 0;
            if(IsBufferedPatternOp(cmd))
            {
                uint16_t pattern = 0;
                m_state.wordbuf = m_state.hbyte | (m_state.hbyte << 8);
                pattern         = m_state.wordbuf;

                //Copy the patterns the nb of times needed
                for( ; cntcopy < nbtocopy; cntcopy += 2 )
                    WriteWord(pattern);
            }
            else
            {
                //022EC6B8 E58DC02C str     r12,[r13,2Ch]             //[r13,2Ch] = r12 //This should be done here, but it seems like it does absolutely nothing??
                for( ; cntcopy < nbtocopy && m_itcur != m_itend; cntcopy += 2 )
                {
                    uint16_t value = ReadByteFromSrc();
                    if(m_itcur == m_itend)
                        throw std::runtime_error("BPCImgDecompressor::HandleMainOp(): Unexpected end of the input data!");
                    value |= (ReadByteFromSrc() << 8);
                    WriteWord(value);
                }

                if(cntcopy < nbtocopy)
                    throw std::runtime_error("BPCImgDecompressor::HandleMainOp(): Unexpected end of the input data!");
            }

            //If the ammount copied was even, we setup the copy a leftover word on the next command byte
            if( cntcopy == nbtocopy )
            {
                m_state.bhasleftover = true;
                if(IsBufferedPatternOp(cmd))
                    m_state.wordbuf = m_state.hbyte;
                else
                    m_state.wordbuf = ReadByteFromSrc();
            }
        }



    private:
        DecompState  m_state;           //Decompression state
        init_t     & m_itcur;
        init_t       m_itend;
        outit_t    * m_pitw;            //Pointer to the output iterator
        size_t       m_decomplen;       //The nb of bytes we expect to output
        size_t       m_bytesoutput;     //The nb of bytes output
        size_t       m_bytesread;       //The nb of bytes that were read so far
    };

    /*
        BPCImgCompressor
    */
    template<class _intit, class _outit>
        class BPCImgCompressor
    {
        typedef _outit outit_t;
        typedef _intit init_t;

        /*
        */
        enum struct eOpType
        {
            Invalid,
            SequenceCopy,   //Copy a sequence of words as-is
            ByteCopy,       //Copy a single byte over several words
        };

        /*
        */
        enum struct ePatternBuffOp
        {
            UseCurrentByte, //Uses the current pattern byte (r7)
            UseLastByte,    //Swap the current pattern byte with the one used that was used before the current one (r7 <-> [r13,14h])
            SetNewByte,     //Load the next byte after the len to copy as the new current pattern byte, and keep track of the old one. (r7 -> [r13,14h], then load into r7)
        };

        /*
        */
        struct Operation
        {
            eOpType         type;

            //Sequence info
            init_t          itbegs;     //The start of the sequence of bytes to copy
            init_t          itends;     //The end of the sequence of bytes to copy
            size_t          seqlen;     //The length of the sequence to be copied. The distance between "itends" and "itbegs".
                                        // If odd, the "differed" variable will be used to fill in, as the game always copies an even nb of bytes.
            uint8_t         differed;   //If the sequence of similar bytes' "seqlen" is odd, this will contains the value of the byte that differed.

            //Byte Patterns stuff
            ePatternBuffOp patop;       //This will be set according to what the currently buffered pattern byte is.
            uint8_t        newpatbyte;  //If the "patop" is "SetNewByte" this will contain the new byte to set!

            Operation()
                :seqlen(0),type(eOpType::Invalid), patop(ePatternBuffOp::UseCurrentByte)
            {}

            inline bool HasLeftover()const
            {
                return (seqlen % 2 != 0);
            }
        };

    public:
        BPCImgCompressor( init_t itbeg, init_t itend )
            :m_itbeg(itbeg), m_itend(itend)
        {}

        void operator()(outit_t itout)
        {
            m_itw = itout;
        }

    private:

        //#1- We want to take small 128 bytes chunks and try to see what is the 2 most common bytes, so we can set those as patterns in those locations

        //#2- Sequences of the same byte that end on a odd number of similar bytes need to include the byte which differed at the end of the sequence!


        std::vector<uint8_t> MakeCommandByte( const Operation & curop )
        {
            //If a sequence to copy ends with a word made of 2 different bytes, we want the sequence(minus the word made of 2 different bytes) to be written as a odd number of bytes in the command byte, 
        }

    private:
        init_t                  m_itbeg;
        init_t                  m_itend;
        outit_t                 m_itw;
    };


//
//  Tile Map Compression
//
    /*
        BPC_TileMapDecompressor
    */
    template<class _init, class _outcnt>
        class BPC_TileMapDecompressor
    {
        typedef _init   init_t;
        typedef _outcnt outcnt_t;
        
        //Phase 1 CMD
        static const uint8_t CMD_ZeroOutBeg    = 0x00;  //Write null words
        static const uint8_t CMD_FillOutBeg    = 0x80;  //Write words with the specified high byte
        static const uint8_t CMD_CopyBytesBeg  = 0xC0;  //Write words with the specified high bytes sequence

        //Phase 2 CMD
        static const uint8_t CMD_SeekOffsetBeg = 0x00;  //Seek from the position of the word currently being operated on.
        static const uint8_t CMD_FillLowBeg    = 0x80;  //Add a specific low byte to the words currently operating on.
        static const uint8_t CMD_CopyLowBeg    = 0xC0;  //Add the low bytes sequence to the words currently operating on.

        static const size_t  NbWordsPerEntry = 9;

    public:

        BPC_TileMapDecompressor( _init & itbeg, 
                                 _init   itend, 
                                 size_t  decomplen ) //The length of the data decompressed in bytes ((decomplen - 1) * NbWordsPerEntry) * sizeof(int16_t)
            :m_itcur(itbeg), m_itend(itend), m_decomplen(decomplen), m_itw(std::back_inserter(m_outputbuf))
        {}
        
        outcnt_t operator()()
        {
            m_outputbuf.reserve(m_decomplen);

            //Step#1: Write the words, with their high bytes.
            while( (m_outputbuf.size() < m_decomplen) && (m_itcur != m_itend) )
                HandleCmdTableA();

            if(m_itcur == m_itend || m_outputbuf.size() < m_decomplen )
                throw std::runtime_error("BPC_TileMapDecompressor::operator()(): Input data ended unexpectedly.");

            //Step#2: Write the low bytes of the specified words.
            auto itbufcur = std::begin(m_outputbuf);
            auto itbufend = std::end(m_outputbuf);
            while( (itbufcur != itbufend) && (m_itcur != m_itend) )
            {
                if(m_itcur == m_itend)
                    throw std::runtime_error("BPC_TileMapDecompressor::operator()(): Input data ended unexpectedly.");
                HandleCmdTableB(itbufcur, itbufend);
            }
            return std::move(m_outputbuf);
        }

    private:
        void HandleCmdTableA()
        {
            size_t cmdby = ReadSrcByte();

            if( cmdby < CMD_FillOutBeg )
            {
                //The cmdby is the nb of words to zero out!
                for( size_t cnt = 0; cnt <= cmdby; ++cnt )
                    utils::WriteIntToBytes<uint16_t>(0,m_itw);//We always write at least one if we get here
            }
            else if( cmdby >= CMD_FillOutBeg && cmdby < CMD_CopyBytesBeg )
            {
                //(cmdby - CMD_FillOutBeg) is the nb of words to write with the next parameter byte as high byte
                uint16_t param = ReadSrcByte() << 8;
                for( size_t cntw = CMD_FillOutBeg; cntw <= cmdby; ++cntw )
                    utils::WriteIntToBytes( param, m_itw );
            }
            else if( cmdby >= CMD_CopyBytesBeg )
            {
                //(cmdby - CMD_CopyBytesBeg) is the nb of words to write with the sequence of bytes as high byte
                for( size_t cntw = CMD_CopyBytesBeg; cntw <= cmdby; ++cntw )
                {
                    uint16_t param = ReadSrcByte() << 8;
                    utils::WriteIntToBytes( param, m_itw );
                }
            }
        }

        template<class _cntit>
            void HandleCmdTableB(_cntit & itword, _cntit itwordend)
        {
            size_t cmdby = ReadSrcByte();

            if( cmdby < CMD_FillLowBeg )
            {
                //We skip over the nb of words indicated by the cmdbyte
                const size_t skiplen = (cmdby + 1) * sizeof(int16_t);
                for( size_t i = 0; i < skiplen; ++i )
                {
                    if( itword != itwordend )
                        ++itword;
                    else
                        throw std::runtime_error("BPC_TileMapDecompressor::HandleCmdTableB(): Output data shorter than expected, or input data corrupted.");
                }
            }
            else if( cmdby >= CMD_FillLowBeg && cmdby < CMD_CopyLowBeg )
            {
                //We put the value of the param byte as the low byte of the nb of words contained in the cmdbyte
                const size_t   nbwords = cmdby; 
                const uint16_t lbyte   = ReadSrcByte();

                for( size_t cntw = CMD_FillLowBeg; cntw <= nbwords; ++cntw )//We always write at least one if we get here
                {
                    if( itword == itwordend )
                        throw std::runtime_error("BPC_TileMapDecompressor::HandleCmdTableB(): Output data shorter than expected, or input data corrupted.");
                    (*itword) |= lbyte;
                    itword+=2;
                }
            }
            else if( cmdby >= CMD_CopyLowBeg )
            {
                //We put the value of the param byte as the low byte of the nb of words contained in the cmdbyte
                const size_t   nbwords = cmdby; //We always write at least one if we get here
                for( size_t cntw = CMD_CopyLowBeg; cntw <= nbwords; ++cntw )
                {
                    if( itword == itwordend )
                        throw std::runtime_error("BPC_TileMapDecompressor::HandleCmdTableB(): Output data shorter than expected, or input data corrupted.");
                    (*itword) |= ReadSrcByte();
                    itword+=2;
                }
            }
        }

        inline uint8_t ReadSrcByte()
        {
            if(m_itcur == m_itend)
                throw std::runtime_error("BPC_TileMapDecompressor::ReadSrcByte(): Input data shorter than expected.");
            const uint8_t val = (*m_itcur);
            ++m_itcur;
            return val;
        }

    private:
        init_t                            & m_itcur;
        init_t                              m_itend;
        std::back_insert_iterator<outcnt_t> m_itw;

        outcnt_t    m_outputbuf;
        size_t      m_decomplen;    //The length of the decompressed output in bytes
    };

};
#endif
