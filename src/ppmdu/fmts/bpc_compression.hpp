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
    template<class _outit>
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
        typedef _outit outit_t;

       enum struct eState : int
        {
            ERROR               = -1,
            WaitingForCmd       =  0,
            FillingParamBuffer  =  1,
        };

        struct patternbuffer
        {
            uint16_t wordbuf   = 0;   //r2
            uint8_t  hbyte     = 0;   //r7
            uint8_t  cachedhby = 0;   //r13_14h
        };

        struct DecompState
        {
            eState          state           = eState::WaitingForCmd;
            uint8_t         lastcmdbyte     = 0;                        //The last command byte parsed
            bool            bhasleftover    = false;                    //Whether there is a leftover word to handle writing!
            patternbuffer   patbuff;                                    //The buffer storing the patterns to copy
            
        };

        BPCImgDecompressor( outit_t destout )
            :m_itw(destout), m_nbbytoconsume(0)
        {
            m_parambuffer.reserve(SizePreAlloc);
        }

        void operator()( uint8_t by )
        {
            //Process the byte accordingly
            switch(m_state.state)
            {
                case eState::WaitingForCmd:
                {
                    HandleCommandByte(by);
                    break;
                }
                case eState::FillingParamBuffer:
                {
                    HandleParameterByte(by);
                    break;
                }
                case eState::ERROR:
                default:
                {
                    throw std::runtime_error("BPCDecompressor::operator()(): Decompressor is an invalid state!");
                }
            };
        }

    private:
        inline bool IsBufferedPatternOp(uint8_t cmdby)const { return (cmdby >= CMD_LoadByteAsPatternAndCp); }
        inline bool IsLoadingPatternFromNextByte(uint8_t cmdby)const {return (cmdby >= CMD_LoadByteAsPatternAndCp && cmdby < CMD_UseLastPatternAndCp); }

        int CalcCurrentNbToCopy()const 
        {
            int nbtocp = 0;
            switch(m_state.lastcmdbyte)
            {
                case CMD_CyclePatternAndCp_NbCpNextByte:
                case CMD_UseLastPatternAndCp_NbCpNextByte:
                case CMD_LoadByteAsPatternAndCp_NbCpNextByte:
                case CMD_LoadNextByteAsNbToCopy:
                {
                    if(m_parambuffer.size() < 1)
                        throw std::out_of_range("BPCDecompressor::CalcCurrentNbToCopy(): Parameter buffer doesn't contains enough bytes to compute the nb of words to copy. Need 1.");
                    nbtocp = m_parambuffer[0];
                    break;
                }
                case CMD_LoadNextWordAsNbToCopy:
                {
                    if(m_parambuffer.size() < 2)
                        throw std::out_of_range("BPCDecompressor::CalcCurrentNbToCopy(): Parameter buffer doesn't contains enough bytes to compute the nb of words to copy. Need 2.");
                    nbtocp = m_parambuffer[0] | (m_parambuffer[1] << 8);
                    break;
                }
                default:
                {
                    nbtocp = m_state.lastcmdbyte;
                    if(m_state.lastcmdbyte >= CMD_CyclePatternAndCp)
                        nbtocp -= CMD_CyclePatternAndCp;
                    else if(m_state.lastcmdbyte >= CMD_UseLastPatternAndCp)
                        nbtocp -= CMD_UseLastPatternAndCp;
                    else if(m_state.lastcmdbyte >= CMD_LoadByteAsPatternAndCp)
                        nbtocp -= CMD_LoadByteAsPatternAndCp;
                }
            };

            if(m_state.bhasleftover)
                nbtocp -= 1;

            return nbtocp;
        }
        
        inline size_t NbBytesNeededForNbToCopy()const
        {
            switch(m_state.lastcmdbyte)
            {
                case CMD_CyclePatternAndCp_NbCpNextByte:
                case CMD_UseLastPatternAndCp_NbCpNextByte:
                case CMD_LoadByteAsPatternAndCp_NbCpNextByte:
                case CMD_LoadNextByteAsNbToCopy:
                    return 1;
                case CMD_LoadNextWordAsNbToCopy:
                    return 2;
            };
            return 0;
        }
        
        size_t NbParamBytesNeededCurOp()const
        {
            //First check if we get the nb to copy from the next bytes
            size_t nbtoload = NbBytesNeededForNbToCopy();

            //Since we load a byte for this add one more byte!
            if( IsLoadingPatternFromNextByte(m_state.lastcmdbyte) ) // >= CMD_LoadByteAsPatternAndCp && m_state.lastcmdbyte < CMD_UseLastPatternAndCp) 
                ++nbtoload;

            //Anything below this will copy x bytes stored after the command byte and length to copy
            if( m_state.lastcmdbyte < CMD_LoadByteAsPatternAndCp )
            {
                //Try to predict if we'll load an extra byte if the nb copied is even
                if( NbBytesNeededForNbToCopy() <= m_parambuffer.size() ) 
                {
                    int nbtocopy = CalcCurrentNbToCopy();
                    //Add an extra byte to load, because if not divisible by 2 we load 2 bytes anyways, and if divisivle by 2, we load another byte to fill the pattern buffer
                    if( nbtocopy >= 0 )
                        nbtoload += static_cast<uint32_t>(nbtocopy + 1); 
                }
                 
                //If we also have a leftover byte, we're going to take another extra byte
                if(m_state.bhasleftover)
                    nbtoload += 1; 
            }

            return nbtoload;    //Otherwise at least wait for those bytes for now
        }

        void HandleCommandByte( uint8_t by )
        {
            m_state.lastcmdbyte = by;

            //First check if we have enough data accumulated to do anything
            size_t nbbyneeded = NbParamBytesNeededCurOp();

            //cout <<"Cmd : 0x" <<hex <<uppercase <<static_cast<uint16_t>(by) <<nouppercase <<dec <<", ParamLen: " <<nbbyneeded <<", Leftover: " <<boolalpha <<m_state.bhasleftover <<noboolalpha <<"\n";

            //Wait to fill up the buffer first!
            if( nbbyneeded > m_parambuffer.size())
            {
                m_nbbytoconsume = (nbbyneeded - m_parambuffer.size());
                m_state.state   = eState::FillingParamBuffer;
                cout <<"Pushing bytes: ";
                return;
            }

            Process();
        }

        void HandleParameterByte( uint8_t by )
        {
            //cout <<hex <<uppercase <<static_cast<uint16_t>(by) <<dec <<nouppercase <<" ";
            assert(m_nbbytoconsume > 0);
            m_parambuffer.push_back(by);
            --m_nbbytoconsume;

            //Re-evaluate here, so we can update the nb of bytes to load if needed
            const size_t nbtoload = NbParamBytesNeededCurOp();
            if( nbtoload > m_parambuffer.size() )
                m_nbbytoconsume = (nbtoload - m_parambuffer.size());

            //If we got everything then jump to processing the thing
            if(m_nbbytoconsume == 0)
                Process();
        }

        inline bool ShouldCycleBytePattern()const
        {
            return IsLoadingPatternFromNextByte(m_state.lastcmdbyte) || (m_state.lastcmdbyte >= CMD_CyclePatternAndCp && m_state.lastcmdbyte <= CMD_CyclePatternAndCp_NbCpNextByte);
        }

        inline void CycleHighBytePattern()
        {
            uint8_t tmp               = m_state.patbuff.cachedhby;
            m_state.patbuff.cachedhby = m_state.patbuff.hbyte;
            m_state.patbuff.hbyte     = tmp;
        }

        void Process()
        {
            //cout <<"\nProcessing..\n";
            //Calculate the nb of words to copy
            int nbtocopy = CalcCurrentNbToCopy();

            //Get the offset of the first parameter after the nb to copy
            size_t offparam = NbBytesNeededForNbToCopy();

            //This is done before the leftover byte
            if(ShouldCycleBytePattern())
                CycleHighBytePattern();
            if( IsLoadingPatternFromNextByte(m_state.lastcmdbyte) )
            {
                m_state.patbuff.hbyte = m_parambuffer[offparam];
                ++offparam;
            }

            //Then check for leftover bytes patterns to add
            if(m_state.bhasleftover)
            {
                uint16_t pattern = 0;
                if(IsBufferedPatternOp(m_state.lastcmdbyte))
                    pattern = (m_state.patbuff.wordbuf) | (m_state.patbuff.hbyte << 8);
                else
                {
                    pattern = (m_state.patbuff.wordbuf) | (m_parambuffer[offparam] << 8);
                    ++offparam;
                }
                //m_itw                = utils::WriteIntToBytes(pattern, m_itw);
                WriteWord(pattern);
                m_state.bhasleftover = false;
            }

            //Run the main operation only if we have a non-zero nb of bytes to copy.
            if( nbtocopy >= 0 )
                HandleMainOp(nbtocopy,offparam);

            //Clear our buffer, and reset the state after processing!
            m_parambuffer.resize(0);
            m_nbbytoconsume = 0;
            m_state.state   = eState::WaitingForCmd;
            //cout <<"\n\n";
        }

        void HandleMainOp( size_t nbtocopy, size_t offparam )
        {
            //Handle the main operation now
            size_t cntcopy = 0;
            if(IsBufferedPatternOp(m_state.lastcmdbyte))
            {
                uint16_t pattern = 0;
                m_state.patbuff.wordbuf = m_state.patbuff.hbyte | (m_state.patbuff.hbyte << 8);
                pattern                 = m_state.patbuff.wordbuf;

                //Copy the patterns the nb of times needed
                for( ; cntcopy < nbtocopy; cntcopy += 2 )
                    WriteWord(pattern);
            }
            else
            {
                //022EC6B8 E58DC02C str     r12,[r13,2Ch]             //[r13,2Ch] = r12 //This should be done here, but it seems like it does absolutely nothing??
                for( ; cntcopy < nbtocopy; cntcopy += 2, offparam += 2 )
                {
                    uint16_t value = 0;

                    if( (offparam + 1) < m_parambuffer.size() )
                        value = m_parambuffer[offparam] | (m_parambuffer[offparam + 1] << 8);
                    else 
                        assert(false);
                    WriteWord(value);
                }
            }

            //If the ammount copied was even, we setup the copy a leftover word on the next command byte
            if( cntcopy == nbtocopy )
            {
                m_state.bhasleftover = true;
                if(IsBufferedPatternOp(m_state.lastcmdbyte))
                    m_state.patbuff.wordbuf = m_state.patbuff.hbyte;
                else
                {
                    m_state.patbuff.wordbuf = m_parambuffer[offparam];
                    ++offparam;
                }
            }
        }

        inline void WriteWord( uint16_t w )
        {
            //cout <<hex <<uppercase <<w <<dec <<nouppercase <<" ";
            m_itw = utils::WriteIntToBytes(w, m_itw);
        }

    private:
        outit_t                 m_itw;
        std::vector<uint8_t>    m_parambuffer;    //Parameter bytes for the current operation will be stockpiled here!
        long                    m_nbbytoconsume;  //The nb of parameters bytes left to feed the functor so it can execute the current op properly
        DecompState             m_state;
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

        BPC_TileMapDecompressor( init_t   itbeg, 
                                  init_t   itend, 
                                  size_t   decomplen ) //The length of the data decompressed in bytes ((decomplen - 1) * NbWordsPerEntry) * sizeof(int16_t)
            :m_itbeg(itbeg), m_itend(itend), m_decomplen(decomplen), m_itw(std::back_inserter(m_outputbuf))
        {}
        
        outcnt_t operator()()
        {
            init_t itcur = m_itbeg;
            m_outputbuf.reserve(m_decomplen);

            //Step#1: Write the words, with their high bytes.
            while( (m_outputbuf.size() < m_decomplen) && (itcur != m_itend) )
                HandleCmdTableA(itcur);

            if(itcur == m_itend || m_outputbuf.size() < m_decomplen )
                throw std::runtime_error("BPC_TileMapDecompressor::operator()(): Input data ended unexpectedly.");

            //Step#2: Write the low bytes of the specified words.
            auto itbufcur = std::begin(m_outputbuf);
            auto itbufend = std::end(m_outputbuf);
            while( (itbufcur != itbufend) && (itcur != m_itend) )
            {
                if(itcur == m_itend)
                    throw std::runtime_error("BPC_TileMapDecompressor::operator()(): Input data ended unexpectedly.");
                HandleCmdTableB(itcur, itbufcur, itbufend);
            }

            //Step#3: Add value to words.
            //! #TODO
            //Its possible that its not necessary

            return std::move(m_outputbuf);
        }

    private:
        void HandleCmdTableA(init_t & itsrc)
        {
            size_t cmdby = ReadSrcByte(itsrc);

            if( cmdby < CMD_FillOutBeg )
            {
                //The cmdby is the nb of words to zero out!
                std::fill_n( m_itw, ((cmdby * sizeof(int16_t)) + 2), 0 ); //We always write at least one if we get here
            }
            else if( cmdby >= CMD_FillOutBeg && cmdby < CMD_CopyBytesBeg )
            {
                //(cmdby - CMD_FillOutBeg) is the nb of words to write with the next parameter byte as high byte
                uint16_t param = ReadSrcByte(itsrc) << 8;

                const size_t NbToWrite = ((cmdby - CMD_FillOutBeg) * sizeof(int16_t)) + 2; //We always write at least one if we get here
                for( size_t cntby = 0; cntby < NbToWrite; cntby += sizeof(int16_t) )
                    utils::WriteIntToBytes( param, m_itw );
            }
            else if( cmdby >= CMD_CopyBytesBeg )
            {
                //(cmdby - CMD_CopyBytesBeg) is the nb of words to write with the sequence of bytes as high byte
                const size_t NbToWrite = ((cmdby - CMD_CopyBytesBeg) * sizeof(int16_t)) + 2; //We always write at least one if we get here
                for( size_t cntby = 0; (cntby < NbToWrite); cntby += sizeof(int16_t) )
                {
                    uint16_t param = ReadSrcByte(itsrc) << 8;
                    utils::WriteIntToBytes( param, m_itw );
                }
            }
        }

        template<class _cntit>
            void HandleCmdTableB(init_t & itsrc, _cntit & itword, _cntit itwordend)
        {
            size_t cmdby = ReadSrcByte(itsrc);

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
                const size_t   nbtoprocess = (cmdby - CMD_FillLowBeg) + 1; //We always write at least one if we get here
                const uint16_t lbyte       = ReadSrcByte(itsrc);

                for( size_t i = 0; i < nbtoprocess; ++i )
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
                const size_t   nbtoprocess = (cmdby - CMD_CopyLowBeg) + 1; //We always write at least one if we get here
                for( size_t i = 0; i < nbtoprocess; ++i )
                {
                    if( itword == itwordend )
                        throw std::runtime_error("BPC_TileMapDecompressor::HandleCmdTableB(): Output data shorter than expected, or input data corrupted.");
                    (*itword) |= ReadSrcByte(itsrc);
                    itword+=2;
                }
            }
        }

        /*
            ReadSrcByte
                Reads a byte from the source, and check and increment the iterator.
        */
        inline uint8_t ReadSrcByte(init_t & itcur)
        {
            if(itcur == m_itend)
                throw std::runtime_error("BPC_TileMapDecompressor::ReadSrcByte(): Input data shorter than expected.");
            const uint8_t val = (*itcur);
            ++itcur;
            return val;
        }

    private:
        init_t                              m_itbeg;
        init_t                              m_itend;
        std::back_insert_iterator<outcnt_t> m_itw;
        outcnt_t                            m_outputbuf;    //The temporary buffer where the data is put before the second pass
        size_t                              m_decomplen;    //The length of the expected decompressed output in bytes
    };

};
#endif
