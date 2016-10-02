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
        BPCDecompressor
    */
    template<class _outit>
        class BPCDecompressor
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

        BPCDecompressor( outit_t destout )
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







    //! #IDEA : Decompression Input Iterator?
        struct decompstate
        {
            uint32_t r0      = 0;
            uint32_t r2      = 0;
            uint32_t r3      = 0; //Contains a boolean state
            uint32_t r7      = 0;
            uint32_t r13_14h = 0; //Contains a value saved for handling later
            uint32_t r13_2Ch = 0;
            size_t   proccnt = 0; //Processing index. Stored in r6. Is the modified position in the buffer. Its modified by the value of some command bytes.
            size_t   nbbytesoutput = 0;
        };
    /*
        bpc_decompression_iterator
            #NOTE: Avoid copying this iterator as much as possible.
    */
    template<class _ContainerType, class _parentitert = std::iterator<std::forward_iterator_tag, typename _ContainerType::value_type> >
        class bpc_decompression_iterator : public _parentitert
    {
        static const size_t BuffPreAlloc = 256; //Pre-alloc this many bytes

    public:
        typedef _parentitert            myiter_t;
        typedef _ContainerType          container_t;
        typedef typename  container_t * container_ptr_t;

        explicit bpc_decompression_iterator( container_ptr_t pcontainer, std::size_t index = 0, std::size_t buffprealloc = BuffPreAlloc )throw()
            :m_index(index), m_pContainer(pcontainer),m_buffcurbyte(0),m_baseindex(index)
        {
            m_decompbuffer.reserve(buffprealloc);
            m_decompbuffer.resize(0);
        }

        bpc_decompression_iterator( const bpc_decompression_iterator<container_t,myiter_t> & other )throw()
            :m_index(other.m_index), m_pContainer(other.m_pContainer), m_decompbuffer(other.m_decompbuffer),m_buffcurbyte(other.m_buffcurbyte), m_state(other.m_state),m_baseindex(other.m_baseindex)
        {}

        virtual ~bpc_decompression_iterator()throw()
        {}

        bpc_decompression_iterator<container_t,myiter_t> & operator=( const bpc_decompression_iterator<container_t,myiter_t> & other )
        {
            m_index         = other.m_index;
            m_baseindex     = other.m_baseindex;
            m_pContainer    = other.m_pContainer;
            m_decompbuffer  = other.m_decompbuffer;
            m_buffcurbyte   = other.m_buffcurbyte;
            m_state         = other.m_state;
            return *this;
        }

        inline bool operator==(const bpc_decompression_iterator & iter)const throw()
        {
            return (m_buffcurbyte >= m_decompbuffer.size()) && (m_index == iter.m_index);   //Two iterators are only at the same position if the buffer is empty
        }
        //inline bool operator<( const bpc_decompression_iterator & otherit )const   
        //{
        //    return (m_index < otherit.m_index)  (m_buffcurbyte >= m_decompbuffer.size());
        //}

        inline bool operator!=                        (const bpc_decompression_iterator & iter)const throw(){return !(*this == iter);}
        inline bpc_decompression_iterator & operator++()                                                    {return *this;}//Does nothing
        inline bpc_decompression_iterator operator++  (int)                                                 {return *this;}//does nothing
        inline bpc_decompression_iterator & operator+=( typename myiter_t::difference_type n )              {return *this;}//does nothing
        inline bpc_decompression_iterator operator+   ( typename myiter_t::difference_type n )const         {return *this;}//does nothing
        //inline bool operator>                         ( const bpc_decompression_iterator & otherit )const   {return otherit < *this;}
        //inline bool operator>=                        ( const bpc_decompression_iterator & otherit )const   {return !(*this < otherit);}
        //inline bool operator<=                        ( const bpc_decompression_iterator & otherit )const   {return !(*this > otherit);}

        inline const uint8_t operator*()const  {return (*m_pContainer)[m_index];}
        uint8_t operator*()        
        { 
            if( m_index >= m_pContainer->size() )
                throw std::out_of_range("bpc_decompression_iterator::operator*() : Iterator out of range!");

            //If we don't have anything in the buffer, handle bytes until we get something
            while( m_buffcurbyte >= m_decompbuffer.size() ) 
                HandleNextByte();

            assert(m_buffcurbyte < m_decompbuffer.size());

            //Grab the processed byte from the buffer!
            uint8_t retval = m_decompbuffer[m_buffcurbyte];
            ++m_buffcurbyte;
            m_state.nbbytesoutput += 1;
            return retval;
        }

    private:

        inline uint8_t LoadNextCmdByteAndIncr()
        {
            if( m_state.proccnt >= m_pContainer->size() )
                throw std::out_of_range("bpc_decompression_iterator::LoadNextCmdByteAndIncr() : Iterator out of range! Either the compressed data is corrupted or part of the file is missing!");
            uint8_t tmp = (*m_pContainer)[m_state.proccnt];
            m_state.proccnt += 1;
            m_index = m_baseindex + m_state.proccnt; //Update index
            return tmp;
        }

        inline size_t GetBegRelativeIndex()const 
        {
            return (m_index - m_baseindex);
        }

        inline uint8_t PeekByteAtCurIndex( const size_t offset = 0 )
        {
            if( (m_index + offset) >= m_pContainer->size() )
                throw std::out_of_range("bpc_decompression_iterator::PeekByteAtCurIndex() : Iterator out of range! Either the compressed data is corrupted or part of the file is missing!");
            return (*m_pContainer)[m_index + offset];
        }

        inline uint8_t PeekByteAtProcIndex( const size_t offset = 0 )
        {
            if( (m_baseindex + m_state.proccnt + offset) >= m_pContainer->size() )
                throw std::out_of_range("bpc_decompression_iterator::PeekByteAtProcIndex() : Iterator out of range!(" + to_string((m_baseindex + m_state.proccnt + offset)) +") Either the compressed data is corrupted or part of the file is missing! Output " + to_string(m_state.nbbytesoutput));
            return (*m_pContainer)[m_baseindex + m_state.proccnt + offset];
        }

        /*
            - If cmdbyte < 0x80
                - If cmdbyte < 0x80 && cmdbyte < 0x7E
                    ->cmdbyte is nb of time to copy the value stored on (0x80 - cmdbyte) bytes after the command byte.
                - If cmdbyte < 0x80 && cmdbyte >= 0x7E
                    ->The nb of times to copy the value stored on (0x80 - cmdbyte) bytes after the command byte.
                - Then If "ExtraByteToAppend"(r3) is true, we read the next byte after having or not read the length to copy, and shift it by 8, and "OR" it with the "LeftoverByte"(r2)
                -> We then copy the appropriate nb of times the next 2 bytes!
                - If the NbToCopy was even, read the next byte into "LeftoverByte"(r2), and set "ExtraByteToAppend"(r3) to true!

            - Otherwise, If cmdbyte >= 0x80
                - If cmdbyte >= 0xE0 (Value is between 0xE0 and 0xFF)
                    - If cmdbyte == 0xFF, the next byte contains the size to copy.
                    - Otherwise, subtract 0xE0 from cmdbyte, to get the size to copy.
                    -> Then swap the "LeftoverByte2"(r7) with the "LeftoverByteBuffer"(r13+14h)
                - If cmdbyte >= 0xC0 (Value is between 0xC0 and 0xDF)
                    - If cmdbyte == 0xDF, the next byte contains the size to copy.
                    - Otherwise, subtract 0xC0 from cmdbyte, to get the size to copy.
                - If cmdbyte >= 0x80 (Value is between 0x80 and 0xBF)
                    - If cmdbyte == 0xBF, the next byte contains the size to copy.
                    - Otherwise, subtract 0x80 from cmdbyte, to get the size to copy.
                    -> Then put the "LeftoverByte2"(r7) into the "LeftoverByteBuffer"(r13+14h), and read the next byte into "LeftoverByte2"(r7).
                - Then If "ExtraByteToAppend"(r3) is true, write (LeftoverByte | (LeftoverByte2 << 8)) to the output, and decrement the size to copy by one! And set "ExtraByteToAppend"(r3) to false.
                -> Then set "LeftoverByte"(r2) to the value of (LeftoverByte2 | (LeftoverByte2 << 8)) and copy it that value the ammount of times required.
                - Then If the size copied is even, set "LeftoverByte"(r2) to the value of "LeftoverByte2"(r7), and set "ExtraByteToAppend"(r3) to true!
                    
        
        */


        void HandleNextByte()
        {
            m_buffcurbyte = 0;
            m_decompbuffer.resize(0); //Doesn't reallocate
            uint8_t cmdbyte = LoadNextCmdByteAndIncr();
            m_state.r0      = cmdbyte;
            cout <<hex <<"Cmd byte : 0x" <<uppercase << static_cast<uint16_t>(cmdbyte) <<nouppercase <<", State R3 : " <<m_state.r3 <<"\n";

            if( cmdbyte >= 0x80 )
            {
                if( cmdbyte >= 0xE0 )
                {
                    if( cmdbyte == 0xFF )
                    {
                        //022EC71C 05D10000 ldreqb  r0,[r1]
                        //022EC720 02816001 addeq   r6,r1,1h
                        m_state.r0      = PeekByteAtCurIndex();
                        m_state.proccnt = GetBegRelativeIndex() + 1; 
                    }
                    else
                    {
                        //022EC728 124000E0 subne   r0,r0,0E0h
                        m_state.r0 -= 0xE0;
                    }
                    //022EC724 E59D1014 ldr     r1,[r13,14h]
                    //022EC72C E58D7014 str     r7,[r13,14h]
                    //022EC730 E1A07001 mov     r7,r1
                    uint32_t swap   = m_state.r13_14h;
                    m_state.r13_14h = m_state.r7;
                    m_state.r7      = swap;

                }
                else if( cmdbyte >= 0xC0 )
                {
                    if( cmdbyte == 0xDF )
                    {
                        //022EC708 05D10000 ldreqb  r0,[r1]
                        //022EC70C 02816001 addeq   r6,r1,1h
                        m_state.r0      = PeekByteAtCurIndex();
                        m_state.proccnt = GetBegRelativeIndex() + 1; 
                    }
                    else
                    {
                        //022EC710 124000C0 subne   r0,r0,0C0h
                        m_state.r0 -= 0xC0;
                    }
                    //022EC714 EA000006 b       22EC734h              ///022EC734 v
                }
                else if( cmdbyte >= 0x80 )
                {
                    if( cmdbyte == 0xBF )
                    {
                        //022EC6E4 05D10000 ldreqb  r0,[r1]           //r0 = curdbbbyte = PtrCurBPC_DBB (Load next byte!)
                        //022EC6E8 02816001 addeq   r6,r1,1h          //r6 = PtrCurBPC_DBB + 1
                        m_state.r0      = PeekByteAtCurIndex();
                        m_state.proccnt = GetBegRelativeIndex() + 1; 
                    }
                    else
                    {
                        //022EC6F0 12400080 subne   r0,r0,80h         //r0 = curdbbbyte - 0x80
                        m_state.r0 -= 0x80;
                    }
                    //022EC6EC E58D7014 str     r7,[r13,14h]
                    //022EC6F4 E4D67001 ldrb    r7,[r6],1h            //r7 = [PtrCurBPC_DBB], ++PtrCurBPC_DBB (Load next byte and increment current byte ptr!)
                    //022EC6F8 EA00000D b       22EC734h              ///022EC734 v
                    m_state.r13_14h = m_state.r7;
                    m_state.r7 = PeekByteAtProcIndex();
                    m_state.proccnt += 1;
                }


                //022EC734 E3530000 cmp     r3,0h
                if( m_state.r3 != 0 )
                {
                    //022EC738 11821407 orrne   r1,r2,r7,lsl 8h       //r1 = r2 | (nextbpcdbbbyte << 8)
                    //022EC73C 10CB10B2 strneh  r1,[r11],2h           //[PtrCurVRAM] = (r2 | (nextbpcdbbbyte << 8)), PtrCurVRAM = PtrCurVRAM + 2
                    //022EC74C 13A03000 movne   r3,0h                 //r3 = 0
                    //022EC750 12400001 subne   r0,r0,1h              //r0 = LastComparedValue - 1
                    utils::WriteIntToBytes( static_cast<uint16_t>(m_state.r2 | (m_state.r7 << 8)), std::back_inserter(m_decompbuffer));
                    m_state.r3 = 0;
                    m_state.r0 -= 1;
                }

                //022EC740 E1871407 orr     r1,r7,r7,lsl 8h           //r1 = nextbpcdbbbyte | (nextbpcdbbbyte << 8)
                //022EC744 E1A01801 mov     r1,r1,lsl 10h             //r1 = r1 << 16
                //022EC748 E1A02821 mov     r2,r1,lsr 10h             //r2 = r1 >> 16 (Make unsigned int16)

                //022EC754 E3A01000 mov     r1,0h                     //r1 = CNT_6 = 0
                //022EC758 EA000001 b       22EC764h              ///022EC764 v
                m_state.r2 = m_state.r7 | (m_state.r7 << 8);

                if(m_state.r0 != static_cast<uint32_t>(-1))
                {
                    const size_t nbtocopy = m_state.r0;
                    size_t cntcopy = 0;
                    for(; cntcopy < nbtocopy; cntcopy+=2 )
                    {
                        /////022EC75C
                        //022EC75C E0CB20B2 strh    r2,[r11],2h               //[r11] = r2, r11 = r11 + 2
                        //022EC760 E2811002 add     r1,r1,2h                  //r1 = CNT_6 + 2
                        utils::WriteIntToBytes( static_cast<uint16_t>(m_state.r2), std::back_inserter(m_decompbuffer));
                    }
                    /////022EC764
                    //022EC764 E1510000 cmp     r1,r0
                    //if( CNT_6 < (curdbbbyte - 0x80) )
                    //    022EC768 BAFFFFFB blt     22EC75Ch              ///022EC75C ^ 
                    //else if( CNT_6 <= (curdbbbyte - 0x80) )
                    if( cntcopy <= nbtocopy )
                    {
                        //022EC76C D3A03001 movle   r3,1h                 //r3 = 1
                        //022EC770 D1A02007 movle   r2,r7                 //r2 = nextbpcdbbbyte
                        m_state.r3 = 1;
                        m_state.r2 = m_state.r7;
                    }
                }
            }
            else
            {
                if( cmdbyte == 0x7F )
                {
                    //022EC66C 05D1C001 ldreqb  r12,[r1,1h]           //r12 = ThirdByteAfterCur
                    //022EC670 05D10000 ldreqb  r0,[r1]               //r0  = SecondByteAfterCur
                    //022EC674 02816002 addeq   r6,r1,2h              //r6  = PtrCurBPC_DBB + 2
                    //022EC678 0180040C orreq   r0,r0,r12,lsl 8h      //r0  = SecondByteAfterCur | (ThirdByteAfterCur << 8)
                    //022EC67C 0A000002 beq     22EC68Ch              ///022EC68C v
                    uint32_t tmp    = PeekByteAtCurIndex();
                    m_state.r0      = 0;
                    m_state.r0      = tmp | (PeekByteAtCurIndex(1) << 8);
                    m_state.proccnt += 2; 

                    cout <<"\tNbToCopy: " <<m_state.r0 <<"\n";
                }
                else if( cmdbyte == 0x7E )
                {
                    //022EC684 05D10000 ldreqb  r0,[r1]
                    //022EC688 02816001 addeq   r6,r1,1h
                    m_state.r0      = 0;
                    m_state.r0      = PeekByteAtCurIndex();
                    m_state.proccnt += 1; 

                    cout <<"\tNbToCopy: " <<m_state.r0 <<"\n";
                }

                //022EC68C E3530000 cmp     r3,0h                 //r3 is a boolean flag triggered by some command bytes
                //if( r3 != 0 )
                if( m_state.r3 != 0 )
                {
                    //022EC690 14D61001 ldrneb  r1,[r6],1h            //r1 = [PtrCurBPC_DBB], ++PtrCurBPC_DBB
                    //022EC694 13A03000 movne   r3,0h                 //r3 = 0
                    //022EC698 12400001 subne   r0,r0,1h              //r0 = curdbbbyte - 1
                    //022EC69C 11821401 orrne   r1,r2,r1,lsl 8h       //r1 = r2 | (r1 << 8)
                    //022EC6A0 10CB10B2 strneh  r1,[r11],2h           //[r11] = r1, r11 = r11 + 2
                    utils::WriteIntToBytes( static_cast<uint16_t>(m_state.r2 | (PeekByteAtProcIndex() << 8)), std::back_inserter(m_decompbuffer));
                    m_state.r3       = 0;
                    m_state.proccnt += 1;
                    m_state.r0      -= 1;
                }

                if(m_state.r0 != static_cast<uint32_t>(-1)) //Since we used signed value do a check here
                {
                    //022EC6A4 E3A01000 mov     r1,0h                     //r1 = 0
                    //022EC6A8 EA000005 b       22EC6C4h              ///022EC6C4 v
                    const size_t NbToCopy = m_state.r0;
                    size_t cntcopy = 0;
                    for(; cntcopy < NbToCopy; )
                    {
                        /////022EC6AC
                        //022EC6AC E5D6E001 ldrb    r14,[r6,1h]
                        //022EC6B0 E4D6C002 ldrb    r12,[r6],2h
                        //022EC6B4 E2811002 add     r1,r1,2h
                        //022EC6B8 E58DC02C str     r12,[r13,2Ch]
                        //022EC6BC E18CC40E orr     r12,r12,r14,lsl 8h
                        //022EC6C0 E0CBC0B2 strh    r12,[r11],2h
                        uint16_t by1 = PeekByteAtProcIndex(1);
                        uint16_t by2 = PeekByteAtProcIndex();
                        m_state.proccnt += 2;
                        cntcopy         += 2;
                        m_state.r13_2Ch = by2;
                        utils::WriteIntToBytes( static_cast<uint16_t>(by2 | (by1 << 8)), std::back_inserter(m_decompbuffer));
                    }
                    /////022EC6C4
                    //022EC6C4 E1510000 cmp     r1,r0
                    //if( r1 < curdbbbyte )
                    //    022EC6C8 BAFFFFF7 blt     22EC6ACh              ///022EC6AC ^
                    if( cntcopy == NbToCopy )
                    {
                        //if( r1 <= curdbbbyte )
                        //    022EC6CC D3A03001 movle   r3,1h             //r3 = 1
                        //    022EC6D0 D4D62001 ldrleb  r2,[r6],1h        //r2 = [PtrCurBPC_DBB], ++PtrCurBPC_DBB   
                        //022EC6D4 EA000026 b       22EC774h              ///022EC774 v
                        m_state.r3      = 1;
                        m_state.r2      = PeekByteAtProcIndex();
                        m_state.proccnt += 1;
                    }
                }
            }
        }

    protected:
        std::size_t             m_index;        //The index of the byte we're currently decompressing from the container!
        std::size_t             m_baseindex;    //The value of the starting index specified at construction!
        std::size_t             m_buffcurbyte;  //The index of the byte we're currently set to output from the decompression buffer!
        container_ptr_t         m_pContainer;
        std::vector<uint8_t>    m_decompbuffer; //Bytes processed that decompress to more than one byte will store their result in this, so they can be outputed on the following calls of the * operator
        decompstate             m_state;
    };

    /*
        const_bpc_decompression_iterator
            The same thing as above, but constant
    */
    template<class _CONTAINER_T>
    class const_bpc_decompression_iterator : public bpc_decompression_iterator<_CONTAINER_T, 
                                                       std::iterator<
                                                                     std::random_access_iterator_tag, 
                                                                     typename _CONTAINER_T::value_type, 
                                                                     std::ptrdiff_t, 
                                                                     const typename _CONTAINER_T::value_type *, 
                                                                     const typename _CONTAINER_T::value_type > 
                                                                    >
    {
    public:
        const_bpc_decompression_iterator( container_ptr_t pcontainer, std::size_t index = 0 )throw()
            :bpc_decompression_iterator(pcontainer,index)
        {}

        const_bpc_decompression_iterator( const const_bpc_decompression_iterator<container_t> & other )throw()
            :bpc_decompression_iterator(other.m_pContainer)
        {}

        virtual ~const_bpc_decompression_iterator()throw()
        {}

        const_bpc_decompression_iterator<container_t> & operator=( const const_bpc_decompression_iterator<container_t> & other )
        {
            m_index         = other.m_index;
            m_baseindex     = other.m_baseindex;
            m_pContainer    = other.m_pContainer;
            m_decompbuffer  = other.m_decompbuffer;
            m_buffcurbyte   = other.m_buffcurbyte;
            m_state         = other.m_state;
            return *this;
        }
    };

};
#endif
