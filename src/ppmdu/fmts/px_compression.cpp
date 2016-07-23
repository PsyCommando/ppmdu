#ifndef PX_COMPRESSION_CPP
#define PX_COMPRESSION_CPP
#include <ppmdu/fmts/px_compression.hpp>
//!#FIXME: LOTS of probably useless includes here!!!
#include <iterator>
#include <cstdint>
#include <algorithm>

//#include <thread>
#include <future>
#include <atomic>
#include <vector>
#include <deque>


#include <iostream>
#include <iomanip>
#include <fstream>
//#include <exception>
#include <sstream>
#include <cassert>
//#include <climits>
#include <string>
//#include <map>
#include <numeric>
#include <Poco/File.h>
#include <utils/utility.hpp>
using namespace std;
using namespace utils;


namespace compression
{
    using utils::MrChronometer;

//=========================================
// Constants
//=========================================
    static const uint32_t PX_LOOKBACK_BUFFER_SIZE = 4096u; //Length of the default lookback buffer. The "sliding window" so to speak!
                                                            // Used to determine how far back the compressor looks for matching sequences!
    static const uint32_t PX_MAX_MATCH_SEQLEN     = 18u; //The longest sequence of similar bytes we can use!
    static const uint32_t PX_MIN_MATCH_SEQLEN     = 3u; //The shortest sequence of similar bytes we can use!
    static const uint32_t PX_NB_POSSIBLE_SEQ_LEN  = 7u; // The nb of unique lengths we can use when copying a sequence. This is due to ctrl flags taking over a part of the value range between 0x0 and 0xF


//=========================================
// Utiliy functions to throw our exceptions
//=========================================
    void ExCompressedSzOverflow::throwme( uint32_t sizegot )
    {
        stringstream strserror;
        strserror << "PX Compression Error: Compressed size overflows 16 bits unsigned integer!\n Expected smaller than " 
                    <<std::numeric_limits<uint16_t>::max() <<", and got " <<sizegot <<".";
        throw ExCompressedSzOverflow(strserror.str());
    }

    void ExDecompressedSzOverflow::throwme( uint64_t sizegot )
    {
        stringstream strserror;
        strserror << "PX Compression Error: Decompressed size overflows 32 bits unsigned integer!\n Expected smaller than " 
                    <<std::numeric_limits<uint32_t>::max() <<", and got " <<sizegot <<".";
        throw ExDecompressedSzOverflow(strserror.str());
    }

//=========================================
// Other Utility Functions
//=========================================




//=========================================
//               Structs/Classes
//=========================================

    /************************************************************************************
        multistep_completion
            A container to contain and represent several steps of a multistep process.
    ************************************************************************************/
    template<int NBSTEPS>
        struct multistep_completion
    {
        //Each steps contains a percentage (0-100) of completion !
        array<atomic<uint8_t>, NBSTEPS> steps;
    };

//=========================================
// px_compressor
//=========================================
    /*********************************************************************************
        px_compressor
            This is used to avoid passing a large ammount of parameters between 
            compression methods,and keeping a "state" based on where we are in the 
            compression process !
    *********************************************************************************/
    template<class _inRandit, class _outit = std::back_insert_iterator< std::vector<uint8_t> > >
        class px_compressor
    {
    //------------------------------------------------------
    //Typedefs
    //------------------------------------------------------
        typedef _inRandit  inIterRand_t;
        //typedef _outRandit outIterRand_t;
        typedef _outit     outIter_t;

    //------------------------------------------------------
    // Enum / Struct px_compressor
    //------------------------------------------------------
        /*********************************************************************************
            All the possible operations that can be done to compress data!
            Entries 0 to 8 correspond to their respective ctrl flag indexes!
        *********************************************************************************/
        enum struct ePXOperation : int8_t
        {
            COPY_ASIS                                 =-1,
            COPY_NYBBLE_4TIMES                        = 0,
            COPY_NYBBLE_4TIMES_EX_INCRALL_DECRNYBBLE0 = 1,
            COPY_NYBBLE_4TIMES_EX_DECRNYBBLE1         = 2,
            COPY_NYBBLE_4TIMES_EX_DECRNYBBLE2         = 3,
            COPY_NYBBLE_4TIMES_EX_DECRNYBBLE3         = 4,
            COPY_NYBBLE_4TIMES_EX_DECRALL_INCRNYBBLE0 = 5,
            COPY_NYBBLE_4TIMES_EX_INCRNYBBLE1         = 6,
            COPY_NYBBLE_4TIMES_EX_INCRNYBBLE2         = 7,
            COPY_NYBBLE_4TIMES_EX_INCRNYBBLE3         = 8,
            COPY_SEQUENCE                             = 9,
        };

        /*********************************************************************************
            compOp
                Stores an operation to insert into the output buffer.
        *********************************************************************************/
        struct compOp
        {
            ePXOperation type;              //The operation to do
            uint8_t      highnybble,        //The value of the compressed high nybble if applicable
                         lownybble,         //The value of the compressed low nybble 
                         nextbytevalue;     //value of the compressed next byte if applicable

            void reset()
            {
                type          = ePXOperation::COPY_ASIS;
                highnybble    = 0;
                lownybble     = 0;
                nextbytevalue = 0;
            }
        };

        /*********************************************************************************
            matchingsequence
                A little struct for clarity's sake to make the code more readable. 
                Better than using std::pair !
        *********************************************************************************/
        struct matchingsequence
        {
            //vector<uint8_t>::const_iterator itpos;
            inIterRand_t itpos;
            size_t       length;
        };

    //------------------------------------------------------
    // Methods px_compressor
    //------------------------------------------------------
    public:                                                                  
        px_compressor(  vector<uint8_t> & out_compresseddata,
                        inIterRand_t      itinbeg,
                        inIterRand_t      itinend,
                        bool              blogenabled );

        //px_compressor(  inIterRand_t  itinbeg,
        //                inIterRand_t  itinend,
        //                outIterRand_t itoutbeg,
        //                outIterRand_t itoutend,
        //                bool          blogenabled );

        px_compressor(  inIterRand_t  itinbeg,
                        inIterRand_t  itinend,
                        outIter_t     itoutbeg,
                        bool          blogenabled );

        px_info_header Compress( ePXCompLevel              compressionlvl     = ePXCompLevel::LEVEL_3, 
                                 bool                      shouldsearchfirst  = false, 
                                 multistep_completion<2> * pTotalBytesHandled = nullptr );

    private:
    //-------------------------------------------------------------
    // Compression Methods px_compressor
    //-------------------------------------------------------------
        /*********************************************************************************
            HandleABlock
                Handle a block of 8 bytes and up to be compressed.
                Return false, when has reached the end.
        *********************************************************************************/
        bool HandleABlock( ePXCompLevel compressionlvl, uint64_t * pTotalBytesHandled, bool shouldsearchfirst );

        /*********************************************************************************
            DetermineBestOperation
                Determine and run the best possible operation to compress the data at 
                m_itInCur.
        *********************************************************************************/
        compOp DetermineBestOperation( ePXCompLevel compressionlvl, bool shouldsearchfirst, uint64_t * pTotalBytesHandled );

        /*********************************************************************************
            CanCompressTo2In1Byte
                Check whether the 2 bytes at "itcurbyte" can be stored as a single byte.
        *********************************************************************************/
        bool CanCompressTo2In1Byte( inIterRand_t itcurbyte, compOp  & out_result );

        /*********************************************************************************
            CanCompressTo2In1ByteWithManipulation
                Check whether the 2 bytes at "itcurbyte" can be stored as a single byte, 
                only if we use special operations based on the ctrl flag index contained 
                in the high nybble!
        *********************************************************************************/
        bool CanCompressTo2In1ByteWithManipulation( inIterRand_t itcurbyte, compOp  & out_result );

        /*********************************************************************************
            CanUseAMatchingSequence
                Search through the lookback buffer for a string of bytes that matches the 
                string beginning at "itcurbyte". It searches for at least 3 matching bytes 
                at first, then, finds the longest matching sequence it can!
        *********************************************************************************/
        bool CanUseAMatchingSequence( inIterRand_t itcurbyte, compOp  & out_result );

        /*********************************************************************************
            FindLongestMatchingSequence
                Find the longest matching sequence of at least PX_MIN_MATCH_SEQLEN bytes 
                and at most PX_MAX_MATCH_SEQLEN bytes.

                - searchbeg      : Beginning of the zone to look for the sequence.
                - searchend      : End of the zone to look for the sequence.
                - tofindbeg      : Beginning of the sequence to find.
                - tofindend      : End of the sequence to find.
                - sequencelenght : Length of the sequence to look for in bytes.
        *********************************************************************************/
        matchingsequence FindLongestMatchingSequence( inIterRand_t itsearchbeg,
                                                      inIterRand_t itsearchend,
                                                      inIterRand_t ittofindbeg,
                                                      inIterRand_t ittofindend,
                                                      uint32_t     sequencelenght );

        /*********************************************************************************
            Because the length is stored as the high nybble in the compressed output, and 
            that the high nybble also contains the ctrl flags, we need to make sure the 
            lengths of sequences to use do not overlap over values of the control flags ! 
            So we'll build a list of length to reserve as we go!

            -> If the value is in our reserved list, and we have PX_NB_POSSIBLE_SEQ_LEN 
                of them already, return true.
            -> If the value isn't in our reserved list, and we still have space left, 
                add it and return true!
            -> If the value isn't in our reserved list, and all PX_NB_POSSIBLE_SEQ_LEN 
                slots are taken, return false!

            NOTE:
                DO NOT pass the exact sequence length. The value stored in the 
                high nybble is essentially : SequenceLen - PX_MIN_MATCH_SEQLEN
        *********************************************************************************/
        bool CheckSequenceHighNybbleValidOrAdd( uint8_t hnybbleorlen );

        /*********************************************************************************
            OuptutAnOperation
                Outputs into the output buffer at position "m_itOutCur" the compressed
                form of the operation passed in parameter!
        *********************************************************************************/
        void OuptutAnOperation( const compOp & operations );

        /*********************************************************************************
            BuildCtrlFlagsList
                This determines all the control flags values, based on what matching
                sequence lengths have been reserved so far !
        *********************************************************************************/
        void BuildCtrlFlagsList();

        /*********************************************************************************
            OutputAllOperations
                This does the neccessary to execute all operations we put in our operation
                queue. It also calculate the proper high nybble value for operation 
                using a control flag index !

                NOTE: After this all our output iterators are invalidated ! Since it
                      shrinks the output to its final size !
        *********************************************************************************/
        void OutputAllOperations(atomic<uint8_t> * pPercentDone);


    //-------------------------------------------------------------
    // Variables
    //-------------------------------------------------------------
        //Iterators on the intput vector
        inIterRand_t  m_itInBeg,
                      m_itInCur,
                      m_itInEnd;
        //Input file length
        vector<uint8_t>::size_type       m_inputSize;

        //Reference on the output vector
        vector<uint8_t>                * m_pCompressedData;

        //Iterators on the output vector
          //m_itOutBeg,
        outIter_t   m_itOutCur;
                       //m_itOutEnd;
        unsigned int m_nbCompressedByteWritten;

        //This contains the 7 possible high nybble values we can use for copying a sequence over
        vector<uint8_t>                  m_highNybbleLenghtsPossible;

        //List of operations to apply. Stored here, because we compute our ctrl flags at the end
        deque<compOp>                   m_PendingOperations;

        //Data gathered during compression to be used later for building the header
        px_info_header                   m_compressioninfo;

        //Progress monitoring
        atomic_int32_t                  *m_pTotalBytesHandled;

        //Whether we should write compression operations to a log file!
        bool                            m_bLoggingEnabled;
        ofstream                        m_mylog;
    };


//=========================================
// px_decompressor
//=========================================
    //#TODO: Make this behave more like a functor, and consider replacing the functions with this!!
    /*********************************************************************************
        px_decompressor
            This is used to avoid passing a large ammount of parameters between decompression methods,
            and keeping a "state" based on where we are in the decompression process !
    *********************************************************************************/
    template<class _init, class _outit>
    class px_decompressor
    {
    public:
        //typedef vector<uint8_t>::iterator       iterator_t;
        //typedef vector<uint8_t>::const_iterator constiterator_t;
        typedef _outit iterator_t;
        typedef _init  constiterator_t;

        // ----------------------- Constructor ----------------------- 
        px_decompressor( constiterator_t databeg,
                         constiterator_t dataend,
                         iterator_t       outbeg,
                         iterator_t       outend,
                         px_info_header   thepxinfo,
                         bool             bshouldlog )
            :itdatacur(databeg),itdatabeg(databeg),itdataend(dataend),itoutcurbyte(outbeg),
             itoutbeg(outbeg),itoutend(outend),pxinfo(thepxinfo), bloggingenabled(bshouldlog)
        {
//#ifdef _DEBUG
            if( bloggingenabled )
            {
                mylog.open(PX_DECOMPRESSION_LOGFILE_NAME, ios::app );
                mylog << "====================================================\nDecompressor Init\n====================================================\n";
            }

            longestsequencecopied       = 0;
            biggestpositioncopiedfrom   = 0;
            nbtimesused_sequence        = 0;
            nbtimesused_copyasis        = 0;
            nbtimesused_2bytesinone     = 0;
            nbtimesused_2bytesinone_adv = 0;
//#endif
        }

        // ----------------------- Methods ----------------------- 
        void DecompressPX();

    private:

        // ----------------------- Methods ----------------------- 
        void HandleCommandByte();
        void HandleSpecialCasesPatterns();

        // ----------------------- Variables ----------------------- 
        //Iterators to source
        constiterator_t itdatacur,
                        itdatabeg,
                        itdataend;
        //Iterators to destination
        iterator_t      itoutcurbyte,
                        itoutbeg,
                        itoutend;
        //Compression info
        px_info_header  pxinfo;

//#ifdef _DEBUG
        bool            bloggingenabled;
        //Statistics and stuff for writing the debug log!
        ofstream        mylog;
        uint32_t        longestsequencecopied;
        int16_t         biggestpositioncopiedfrom;
        vector<uint8_t> allsequencecopyhighnybbles;
        uint64_t        nbtimesused_sequence,
                        nbtimesused_copyasis,
                        nbtimesused_2bytesinone,
                        nbtimesused_2bytesinone_adv;
//#endif
    };

//=========================================
// Helper Functions Decompress
//=========================================
    /*********************************************************************************
        Compute4NybblesPattern
            This returns a container with 2 bytes/4 nybbles in it.
    *********************************************************************************/
    vector<uint8_t> Compute4NybblesPattern( uint8_t indexControlFlag, uint8_t ctrlLowNybble )
    {
        vector<uint8_t> out_the2bytes(2,0);

        if( indexControlFlag == 0 )
        {
            //#In this case, all our 4 nybbles have the value of the "ctrlLowNybble" as their value
            // Since we're dealing with half bytes, shift one left by 4 and bitwise OR it with the other!
            fill( out_the2bytes.begin(), out_the2bytes.end(), ( (ctrlLowNybble<<4) | ctrlLowNybble ) );
        }
        else 
        {
            //#Here we handle 2 special cases together 
            uint8_t nybbleval = ctrlLowNybble;

            //#1 - Find out the value we'll put in our nybbles.
            //     If the ctrl flag index is exactly one of those values, 
            //     we either increment or decrement all the nybbles
            //     respectively.
            if     ( indexControlFlag == 1 )
                ++nybbleval;
            else if( indexControlFlag == 5 )
                --nybbleval;

            //#2 - Build our individual nybble list, just for convenience.
            array<uint8_t,4> the4nybbles;
            fill( the4nybbles.begin(), the4nybbles.end(), nybbleval );

            //#3 - If the ctrl flag index fits between those values either 
            //     decrement or increment a specific nybble!
            if( indexControlFlag >= 1 && indexControlFlag <= 4 )
                the4nybbles[ indexControlFlag - 1 ] -= 1; //decrement this particuliar nybble (substract by 1 to get a 0-3 index)
            else 
                the4nybbles[ indexControlFlag - 5 ] += 1; //increment this particuliar nybble. (substract by 5 to get a 0-3 index)

            //#4 - We output everything to our output container
            out_the2bytes[0] = ( (the4nybbles[0] << 4) | the4nybbles[1] );
            out_the2bytes[1] = ( (the4nybbles[2] << 4) | the4nybbles[3] );
        }

        return std::move(out_the2bytes);
    }

//=========================================
// Decompressor Definitions
//=========================================
    /*********************************************************************************
        DecompressPX
    *********************************************************************************/
    template<class _init, class _outit>
        void px_decompressor<_init,_outit>::DecompressPX()
    {
        //Iterate through all bytes
        while( itdatacur != itdataend )
        {
            HandleCommandByte();
        }

        //Write some statistics for the log
        if( bloggingenabled )
        {
            mylog <<"\n\nLongest sequence copied over: " <<longestsequencecopied <<"\n"
                  <<"Most distant offset to copy from:" <<static_cast<int16_t>(biggestpositioncopiedfrom) <<"\n"
                  <<"All high nybble values for copy sequence cmd bytes :\n";

            std::sort(allsequencecopyhighnybbles.begin(), allsequencecopyhighnybbles.end() );

            for( auto & anhighybble : allsequencecopyhighnybbles )
            {
                mylog<<hex <<static_cast<unsigned short>(anhighybble) <<"\n";
            }

            mylog << "All control flags :\n";

            vector<uint8_t> sortedflags( pxinfo.controlflags.begin(), pxinfo.controlflags.end() );
            std::sort( sortedflags.begin(), sortedflags.end() );

            for( auto & actrlflag : sortedflags )
            {
                mylog<<hex <<static_cast<unsigned short>(actrlflag) <<"\n";
            }

            mylog <<"Nb times copied a sequence : " <<dec <<nbtimesused_sequence  <<"\n"
                  <<"Nb times copied as is : " <<dec <<nbtimesused_copyasis  <<"\n"
                  <<"Nb times used 2 bytes in one : " <<dec <<nbtimesused_2bytesinone  <<"\n"
                  <<"Nb times used 2 bytes in one with extra manip : " <<dec <<nbtimesused_2bytesinone_adv  <<endl;
        }
        //Debug check if we have less bytes than expected as this is the best sign that there is an issue..
#ifdef _DEBUG
        size_t outputedsize = std::distance( itoutbeg, itoutcurbyte );
        assert( outputedsize >= pxinfo.decompressedsz );
#endif
    }

    /*********************************************************************************
        HandleCommandByte
    *********************************************************************************/
    template<class _init, class _outit>
        void px_decompressor<_init,_outit>::HandleCommandByte()
    {
        //Create a lambda to check if we continue looping, to make things more readable
        auto shouldloop = [&](uint8_t curmask)->bool
        {
            return (curmask>0) &&
                   (itoutcurbyte != itoutend) && 
                   (itdatacur    != itdataend); 
        };

        //Take the command byte and advance the iterator
        uint8_t cmdbyte = *itdatacur;
        ++itdatacur;

        if( bloggingenabled )
            mylog << "-> Command Byte 0x" <<std::hex <<static_cast<unsigned short>(cmdbyte) <<"\n";

        //Each bits of the command byte indicates us what to do with the next 8 bytes, based on whether the bit is 1 or 0.
        for( uint8_t mask = 0x80; shouldloop(mask); mask >>= 1 ) //We shift the byte right until its gone! That way we handle all 8 bits!
        {
            if( mask & cmdbyte ) //If the bit we isolated is NOT equal to 0
            {
                if( bloggingenabled )
                {
                    ++nbtimesused_copyasis;
                    mylog << "      Bit is 1 : Copy 0x" <<std::hex <<static_cast<unsigned short>(*itdatacur) <<" as is!\n";
                }

                //We just append the value to our destination image
                (*itoutcurbyte) = (*itdatacur);
            
                //Increment both iterators
                ++itdatacur;
                ++itoutcurbyte;
            }
            else //If the bit we isolated IS equal to 0
            {
                if( bloggingenabled )
                    mylog << "      Bit is 0 : ";

                HandleSpecialCasesPatterns();
            }
        }
        if( bloggingenabled )
            mylog <<"Current Ouput size : " <<dec <<std::distance( itoutbeg, itoutcurbyte ) <<endl; //#FIXME: This is really dumb.. Store that in an internal variable instead..
    }

    /*********************************************************************************
        HandleSpecialCasesPatterns
    *********************************************************************************/
    template<class _init, class _outit>
        void px_decompressor<_init,_outit>::HandleSpecialCasesPatterns()
    {
        static const int32_t MAX_LOOKBACK_DIFFERENCE = ( - static_cast<int32_t>(PX_LOOKBACK_BUFFER_SIZE) );

        //So, if we're here, we want to check the value of the next byte, split it in half, and take a decision based on what we find out..
        uint8_t nextbyte = *itdatacur,
                nbhigh   = (nextbyte >> 4) & 0xF, //Get the high nybble
                nblow    = nextbyte & 0xF;        //Get the low nybble

        ++itdatacur; //increment iter before forgetting

        //Search in our control flags if we have one with the same value as what we've got in the high half of our byte
        auto pendflags = pxinfo.controlflags.end(),
             pfound    = find( pxinfo.controlflags.begin(), pendflags, nbhigh );

        if( pfound != pendflags )
        {
            //#In this case, append 2 bytes/4 nybbles pattern

            //#1 - Get the index we found the high half of the byte at
            uint8_t indexControlFlag = static_cast<uint8_t>( distance( pxinfo.controlflags.begin(), pfound ) );

            //#2 - Figure out what nybble pattern we will append
            auto the2bytes = Compute4NybblesPattern( indexControlFlag, nblow );

            //#3 - Append to the output the nybble pattern we just figured out
            copy( the2bytes.begin(), the2bytes.end(), itoutcurbyte );
            std::advance( itoutcurbyte, the2bytes.size() ); //increment the output iterator

            if( bloggingenabled )
            {
                mylog << "appending 2 bytes.. ";

                if( indexControlFlag == 0 )
                    ++nbtimesused_2bytesinone;
                else
                    ++nbtimesused_2bytesinone_adv;

                for(auto& abyte : the2bytes )
                    mylog <<hex <<static_cast<unsigned short>(abyte) <<" ";
                mylog<<endl;
            }
        }
        else
        {
            //#In this case, we append a a sequence from a previous 
            // position in the decompressed data.
            //
            // High half-byte is length of sequence copied over
            // Low half-byte is part of the position of the sequence

            //#1 - Get the location we copy the data from in our decompressed data this far
            int16_t offset = -0x1000; //The value is stored as a negative signed 12bits integer, so to store it in a 16 bit int, we need the first nybble to be 0xF 
            offset += static_cast<int16_t>( (nblow << 8) | (*itdatacur) ); //#TODO: Is this cast neccessary?
            ++itdatacur; //increment input iterator

            //Check for problems
            if( offset < MAX_LOOKBACK_DIFFERENCE )
            {
                cerr <<"<!>- FATAL ERROR: Sequence to copy out of bound! Expected < " <<MAX_LOOKBACK_DIFFERENCE <<", but got " <<offset <<"\n"
                     <<"                  Aborting decompression!!\n";
                stringstream strserror;
                strserror <<"Sequence to copy out of bound! Expected < " <<MAX_LOOKBACK_DIFFERENCE <<", but got " <<offset <<"\n"
                          <<"Either the data to decompress is not valid PX compressed data, or\n"
                          <<"something happened with our iterator that made us read the wrong\n" 
                          <<"bytes.. Try enabling logging, and trace back what went wrong!\n";
                throw std::runtime_error(strserror.str());
            }

            if( bloggingenabled )
            {
                ++nbtimesused_sequence;
                mylog << "appending sequence..\n        (highnybble: 0x" <<hex <<static_cast<unsigned short>(nbhigh) 
                      <<", lownybble: 0x" <<hex <<static_cast<unsigned short>(nblow) <<", calculatedoffset: 0x";
                if( longestsequencecopied < nbhigh + PX_MIN_MATCH_SEQLEN )
                    longestsequencecopied = nbhigh + PX_MIN_MATCH_SEQLEN;

                auto itfound = find( allsequencecopyhighnybbles.begin(), allsequencecopyhighnybbles.end(), nbhigh );
                if( itfound == allsequencecopyhighnybbles.end() )
                    allsequencecopyhighnybbles.push_back(nbhigh);

                if( biggestpositioncopiedfrom > offset )
                    biggestpositioncopiedfrom = offset;
                mylog <<hex <<setfill('0') <<setw(4) <<offset <<"( decimal " <<dec <<offset <<" ) )" <<endl <<"        ";
            }

            //#2 - Copy the data sequence
            auto itpos = itoutcurbyte + offset; //Create an iterator at the pos where 
                                                // the sequence to copy begins in our 
                                                // decompressed data so far.
            for( uint8_t i = 0; i < (nbhigh + PX_MIN_MATCH_SEQLEN); ++i, ++itoutcurbyte ) 
            {
                if( bloggingenabled )
                    mylog <<hex <<setfill('0') <<setw(2) <<static_cast<unsigned short>(*(itpos+i))  <<" ";

                (*itoutcurbyte) = *(itpos+i);
            }

            if( bloggingenabled )
                mylog <<endl;
        }

    }

//=========================================
// px_compressor Definitions
//=========================================

    /*********************************************************************************
        Constructor
    *********************************************************************************/
    template<class _inRandit, class _outRandit>
        px_compressor<_inRandit,_outRandit>::px_compressor(  vector<uint8_t> & out_compresseddata,
                                                             _inRandit         itinbeg,
                                                             _inRandit         itinend,
                                                             bool              blogenabled )
        :m_pCompressedData(&out_compresseddata), m_itInBeg(itinbeg), m_itInCur(itinbeg), m_itInEnd(itinend),
        m_highNybbleLenghtsPossible(PX_NB_POSSIBLE_SEQ_LEN,0), m_inputSize(0), m_bLoggingEnabled(blogenabled),
        m_nbCompressedByteWritten(0), m_itOutCur(vector<uint8_t>())
    {
        m_inputSize = std::distance(itinbeg, itinend);

        //Pre-emptive resize
        //if( m_pCompressedData->size() < m_inputSize )
            m_pCompressedData->reserve( m_inputSize );

        m_itOutCur = std::back_inserter(out_compresseddata);

        //Setup our iterators
        //m_itOutBeg = m_pCompressedData->begin();
        //m_itOutCur = m_itOutBeg;
        //m_itOutEnd = m_pCompressedData->end();

        //Resize to zero to allow pushbacks, and preserve allocation
        m_highNybbleLenghtsPossible.resize(0);

        //#FIXME: disabled logging for now !
        assert( !blogenabled );
        //if( m_bLoggingEnabled )
        //    m_mylog.open(PX_COMPRESSION_LOGFILE_NAME);
    }

    //template<class _inRandit, class _outRandit>
    //    px_compressor<_inRandit,_outRandit>::px_compressor(  inIterRand_t  itinbeg,
    //                                                         inIterRand_t  itinend,
    //                                                         outIterRand_t itoutbeg,
    //                                                         outIterRand_t itoutend,
    //                                                         bool          blogenabled )
    //    :m_pCompressedData(nullptr), m_itInBeg(itinbeg), m_itInCur(itinbeg), m_itInEnd(itinend),
    //    m_highNybbleLenghtsPossible(PX_NB_POSSIBLE_SEQ_LEN,0), m_inputSize(0), m_bLoggingEnabled(blogenabled),
    //    m_itOutBeg(itoutbeg), m_itOutCur(itoutbeg), m_itOutEnd(itoutend)
    //{
    //    //Resize to zero to allow pushbacks, and preserve allocation
    //    m_highNybbleLenghtsPossible.resize(0);

    //    //#FIXME: disabled logging for now !
    //    assert( !blogenabled );
    //    //if( m_bLoggingEnabled )
    //    //    m_mylog.open(PX_COMPRESSION_LOGFILE_NAME);
    //}

    template<class _inRandit, class _outit>
        px_compressor<_inRandit,_outit>::px_compressor(  inIterRand_t  itinbeg,
                                                             inIterRand_t  itinend,
                                                             outIter_t     itout,
                                                             bool          blogenabled )
        :m_pCompressedData(nullptr), m_itInBeg(itinbeg), m_itInCur(itinbeg), m_itInEnd(itinend),
        m_highNybbleLenghtsPossible(PX_NB_POSSIBLE_SEQ_LEN,0), m_inputSize(0), m_bLoggingEnabled(blogenabled),
        m_itOutCur(itout), m_nbCompressedByteWritten(0)
    {
        //Resize to zero to allow pushbacks, and preserve allocation
        m_highNybbleLenghtsPossible.resize(0);

        //#FIXME: disabled logging for now !
        assert( !blogenabled );
        //if( m_bLoggingEnabled )
        //    m_mylog.open(PX_COMPRESSION_LOGFILE_NAME);
    }

    /*********************************************************************************
        Compress
    *********************************************************************************/
    template<class _inRandit, class _outRandit>
        px_info_header px_compressor<_inRandit,_outRandit>::Compress(ePXCompLevel compressionlvl, bool shouldsearchfirst, multistep_completion<2> * pTotalBytesHandled)
    {
        //Caluclate the size of the input
        m_inputSize = std::distance( m_itInBeg, m_itInEnd );

        //Verify if we overflow
        if( m_inputSize > static_cast<decltype(m_inputSize)>(std::numeric_limits<uint32_t>::max()) )
        {
            assert(false);
            ExDecompressedSzOverflow::throwme(m_inputSize);
            return px_info_header();
        }

        //If we're using a vector as input
        if( m_pCompressedData != nullptr )
        {
            //Allocate at least as much memory as the input + some extra in case of dummy compression!
            m_pCompressedData->resize( m_inputSize + m_inputSize / 8u + (m_inputSize % 8u != 0 )? 1 : 0 ); //Worst case, we got 1 more bytes per 8 bytes.
                                                                                                           // And if we're not divisible by 8, add an extra
                                                                                                           // byte for the last command byte !
            m_pCompressedData->resize(0);                                                                  //Reset size to 0, to allow pushbacks and preserve alloc
        }

        m_PendingOperations.resize( m_inputSize );                    //In the worst case scenario, we got 1 operation per bytes!
        m_PendingOperations.resize(0);                                //Reset size to 0, to allow pushbacks and preserve alloc

        //Resets those
        m_highNybbleLenghtsPossible.resize(0);

        //Setup the output iterators
        //m_itOutBeg = m_compresseddata.begin();
        //m_itOutCur = m_itOutBeg;
        //m_itOutEnd = m_compresseddata.end();

        //Set by default those two possible matching sequence length, given we want 99% of the time to
        // have those 2 to cover the gap between the 2 bytes to 1 algorithm and the string search, and also get 
        // to use the string search's capability to its maximum!
        m_highNybbleLenghtsPossible.push_back(0);   //We want 0 !
        m_highNybbleLenghtsPossible.push_back(0xF); //We want 0xF !

        //Do compression
        uint64_t nbBytesHandled=0;

        while( HandleABlock( compressionlvl, &nbBytesHandled, shouldsearchfirst ) )
        {
            //Update progress
            if( pTotalBytesHandled != nullptr )
                pTotalBytesHandled->steps[0] = static_cast<uint8_t>((nbBytesHandled * 100ul) / m_inputSize);
        }

        //Build control flag table, now that we determined all our string search lengths !
        BuildCtrlFlagsList();

        //Execute all operations from our queue
        if( pTotalBytesHandled != nullptr )
            OutputAllOperations(&(pTotalBytesHandled->steps[1]));
        else
            OutputAllOperations(nullptr);

        //Compute compressed size
        //unsigned int compressedsz = ( m_pCompressedData != nullptr )? m_pCompressedData->size() : std::distance(m_itOutBeg, m_itOutCur); 

        //Validate compressed size
        if( m_nbCompressedByteWritten > static_cast<unsigned int>(std::numeric_limits<uint16_t>::max()) )
        {
            assert(false);
            ExCompressedSzOverflow::throwme(m_nbCompressedByteWritten);
        }

        //Set compressed size
        m_compressioninfo.compressedsz   = static_cast<uint16_t>(m_nbCompressedByteWritten);
        //Set decompressed size
        m_compressioninfo.decompressedsz = static_cast<uint32_t>(m_inputSize);

        return m_compressioninfo;
    }


    /*********************************************************************************
        HandleABlock
            Handle a block of 8 bytes and up to be compressed.
            Return false, when has reached the end.
    *********************************************************************************/
    template<class _inRandit, class _outRandit>
        bool px_compressor<_inRandit,_outRandit>::HandleABlock( ePXCompLevel compressionlvl, uint64_t * pTotalBytesHandled, bool shouldsearchfirst )
    {
        if( m_itInCur != m_itInEnd )
        {
            //Determine what to do for as much bytes as possible
            for( uint8_t i = 0; i < 8 && m_itInCur != m_itInEnd; ++i )
                m_PendingOperations.push_back( DetermineBestOperation( compressionlvl, shouldsearchfirst, pTotalBytesHandled ) );

            return true;
        }
        return false;
    }

    /*********************************************************************************
        DetermineBestOperation
            Determine and run the best possible operation to compress the data at 
            m_itInCur.
    *********************************************************************************/
    template<class _inRandit, class _outRandit>
        typename px_compressor<_inRandit,_outRandit>::compOp px_compressor<_inRandit,_outRandit>::DetermineBestOperation( ePXCompLevel compressionlvl, bool shouldsearchfirst, uint64_t * pTotalBytesHandled )
    {
        compOp myoperation;
        myoperation.reset(); //Zero everything

#ifdef _DEBUG
        //Just a little thing I made to keep track of what offset we're compressing. Useful for breakpoints!
        uint32_t debugoffsettracker = distance(m_itInBeg, m_itInCur);
        debugoffsettracker = debugoffsettracker + 0;
#endif

        bool     bRunSearchFirst = shouldsearchfirst;
        uint32_t amountToAdvance = 0;

        if( bRunSearchFirst && compressionlvl >= ePXCompLevel::LEVEL_3 && CanUseAMatchingSequence(m_itInCur, myoperation ) )
        {
            amountToAdvance = (myoperation.highnybble + PX_MIN_MATCH_SEQLEN);
        }
        else if( compressionlvl >= ePXCompLevel::LEVEL_1 &&  CanCompressTo2In1Byte( m_itInCur, myoperation ) )
        {
            amountToAdvance = 2;
        }
        else if( compressionlvl >= ePXCompLevel::LEVEL_2 && CanCompressTo2In1ByteWithManipulation( m_itInCur, myoperation ) )
        {
            amountToAdvance = 2;
        }
        else if( !bRunSearchFirst && compressionlvl >= ePXCompLevel::LEVEL_3 && CanUseAMatchingSequence(m_itInCur, myoperation ) ) 
        {
            amountToAdvance = (myoperation.highnybble + PX_MIN_MATCH_SEQLEN);
        }
        else //Level 0
        {
            //If all else fails, add the byte as-is
            myoperation.type       = ePXOperation::COPY_ASIS;
            myoperation.highnybble = (*m_itInCur >> 4) & 0x0F;
            myoperation.lownybble  = (*m_itInCur)      & 0x0F;
            amountToAdvance        = 1;
        }

        //Advance the iterator and progress meter
        advance( m_itInCur, amountToAdvance );
        if(pTotalBytesHandled != nullptr)
            (*pTotalBytesHandled) += amountToAdvance;

        return myoperation;
    }

    /*********************************************************************************
        CanCompressTo2In1Byte
            Check whether the 2 bytes at "itcurbyte" can be stored as a single byte.
    *********************************************************************************/
    template<class _inRandit, class _outRandit>
        bool px_compressor<_inRandit,_outRandit>::CanCompressTo2In1Byte( inIterRand_t itcurbyte, compOp  & out_result )
    {
        uint16_t bothbytes = 0;

        for( int i = 1; i >= 0; --i )
        {
            if( itcurbyte != m_itInEnd )
            {
                bothbytes |=  (*itcurbyte) << (8 * i);
                ++itcurbyte;
            }
            else
            {
                return false;
            }
        }

        out_result.lownybble = static_cast<uint8_t>(bothbytes & 0x0F);
        for( int i = 3; i >= 0; --i )
        {
            //Compare every nybbles with the low nybble we got above.
            // The 4 must match for this to work !
            if( ( ( bothbytes >> (4 * i) ) & 0x0F ) != out_result.lownybble )
                return false;
        }

        out_result.type = ePXOperation::COPY_NYBBLE_4TIMES;

        return true;
    }

    /*********************************************************************************
        CanCompressTo2In1ByteWithManipulation
            Check whether the 2 bytes at "itcurbyte" can be stored as a single byte, 
            only if we use special operations based on the ctrl flag index contained 
            in the high nybble!
    *********************************************************************************/
    template<class _inRandit, class _outRandit>
        bool px_compressor<_inRandit,_outRandit>::CanCompressTo2In1ByteWithManipulation( inIterRand_t itcurbyte, compOp  & out_result )
    {
        array<uint8_t, 4> nybbles   = {0}; 

        //Read 4 nybbles from the input
        for( int i = 0; i < 4; i+=2 )
        {
            if( itcurbyte != m_itInEnd )
            {
                nybbles[i]   = ((*itcurbyte) >> 4) & 0x0F;
                nybbles[i+1] = (*itcurbyte) & 0x0F;
                ++itcurbyte;
            }
            else
                return false;
        }

        //Count the nb of occurences for each nybbles
        array<uint8_t, 4> nymatches  = {0}; 
        for( unsigned int i = 0; i < nybbles.size(); ++i )
        {
            nymatches[i] = count( nybbles.begin(), nybbles.end(), nybbles[i] );
        }

        //We got at least 3 values that come back 3 times
        if( count( nymatches.begin(), nymatches.end(), 3 ) == 3 )
        {
            auto minmaxvalues = minmax_element( nybbles.begin(), nybbles.end() );

            //If the difference between the biggest and smallest nybble is one, we're good
            if( *(minmaxvalues.second) - *(minmaxvalues.first) == 1 )
            {
                // Get the index of the smallest value
                uint32_t indexsmallest = distance( nybbles.begin(), minmaxvalues.first );
                uint32_t indexlargest  = distance( nybbles.begin(), minmaxvalues.second ); 

                if( nymatches[indexsmallest] == 1 )
                {
                    // This case is for ctrl flag indexes 1 to 4. There are 2 cases here:
                    // A) The decompressor decrements a nybble not at index 0 once. 
                    // B) The decompressor increments all of them once, and then decrements the one at index 0 !
                    // indexsmallest : is the index of the nybble that gets decremented.
                    out_result.type = static_cast<ePXOperation>( indexsmallest + static_cast<uint32_t>(ePXOperation::COPY_NYBBLE_4TIMES_EX_INCRALL_DECRNYBBLE0) );
                    
                    if( indexsmallest == 0 )
                        out_result.lownybble = nybbles[indexsmallest]; //Copy as-is, given the decompressor increment it then decrement this value
                    else
                        out_result.lownybble = nybbles[indexsmallest] + 1; //Add one since we subtract 1 during decompression
                }
                else
                {
                    // This case is for ctrl flag indexes 5 to 8. There are 2 cases here:
                    // A) The decompressor increments a nybble not at index 0 once. 
                    // B) The decompressor decrements all of them once, and then increments the one at index 0 again!
                    // indexlargest : is the index of the nybble that gets incremented.
                    out_result.type = static_cast<ePXOperation>( indexlargest + static_cast<uint32_t>(ePXOperation::COPY_NYBBLE_4TIMES_EX_DECRALL_INCRNYBBLE0) );
                    
                    if( indexlargest == 0 )
                        out_result.lownybble = nybbles[indexlargest]; //Since we decrement and then increment this one during decomp, use it as-is 
                    else
                        out_result.lownybble = nybbles[indexlargest] - 1; //Subtract 1 since we increment during decompression
                }

                return true;
            }
        }
        return false;
    }


    /*********************************************************************************
        FindLongestMatchingSequence
            Find the longest matching sequence of at least PX_MIN_MATCH_SEQLEN bytes 
            and at most PX_MAX_MATCH_SEQLEN bytes.

            - searchbeg      : Beginning of the zone to look for the sequence.
            - searchend      : End of the zone to look for the sequence.
            - tofindbeg      : Beginning of the sequence to find.
            - tofindend      : End of the sequence to find.
            - sequencelenght : Length of the sequence to look for in bytes.
    *********************************************************************************/
    template<class _inRandit, class _outRandit>
    typename px_compressor<_inRandit,_outRandit>::matchingsequence px_compressor<_inRandit,_outRandit>::FindLongestMatchingSequence( inIterRand_t searchbeg,
                                                                                inIterRand_t searchend,
                                                                                inIterRand_t tofindbeg,
                                                                                inIterRand_t tofindend,
                                                                                uint32_t sequencelenght)
    {
        matchingsequence longestmatch = { searchend, 0};
        auto seqToFindShortEnd = advAsMuchAsPossible( tofindbeg, tofindend, PX_MIN_MATCH_SEQLEN );

        for( auto cursearchpos = searchbeg; cursearchpos != searchend;  )
        {
            cursearchpos = search( cursearchpos, searchend, tofindbeg, seqToFindShortEnd );

            if( cursearchpos != searchend )
            {
                unsigned int nbmatches = CountEqualConsecutiveElem( cursearchpos, 
                                                                        advAsMuchAsPossible(cursearchpos, searchend, PX_MAX_MATCH_SEQLEN), 
                                                                        tofindbeg, 
                                                                        tofindend );

                if( longestmatch.length < nbmatches )
                {
                    longestmatch.length = nbmatches;
                    longestmatch.itpos  = cursearchpos;
                }

                if( nbmatches == PX_MAX_MATCH_SEQLEN )
                    return longestmatch;

                ++cursearchpos;
            }
        }

        return longestmatch;
    }


    /*********************************************************************************
        CanUseAMatchingSequence
            Search through the lookback buffer for a string of bytes that matches the 
            string beginning at "itcurbyte". It searches for at least 3 matching bytes 
            at first, then, finds the longest matching sequence it can!
    *********************************************************************************/
    template<class _inRandit, class _outRandit>
        bool px_compressor<_inRandit,_outRandit>::CanUseAMatchingSequence( inIterRand_t itcurbyte, compOp  & out_result )
    {
        //Get offset of LookBack Buffer beginning
        int32_t  currentOffset       = distance( m_itInBeg, itcurbyte );
        uint32_t lbBufferBeg         = (currentOffset > PX_LOOKBACK_BUFFER_SIZE)? 
                                        currentOffset - PX_LOOKBACK_BUFFER_SIZE : 
                                        0;

        //Setup our iterators for clarity's sake
        inIterRand_t itLookBackBeg = m_itInBeg + lbBufferBeg,
                     itLookBackEnd = itcurbyte,
                     itSequenceBeg = itcurbyte,
                     itSequenceEnd = advAsMuchAsPossible(itcurbyte, m_itInEnd, PX_MAX_MATCH_SEQLEN );

        uint32_t curSeqLen = distance( itSequenceBeg, itSequenceEnd );

        //Make sure our sequence is at least 3 bytes long
        if( curSeqLen < PX_MIN_MATCH_SEQLEN ) 
            return false;

        matchingsequence result = FindLongestMatchingSequence( itLookBackBeg, itLookBackEnd, itSequenceBeg, itSequenceEnd, curSeqLen );

        if( result.length >= PX_MIN_MATCH_SEQLEN )
        {
            uint8_t validhighnybble = static_cast<uint8_t>(result.length - PX_MIN_MATCH_SEQLEN); //Subtract 3 given that's how they're stored!
            
            //Check the length in the table !
            if( !CheckSequenceHighNybbleValidOrAdd( result.length - PX_MIN_MATCH_SEQLEN ) )
            {
                //If the size is not one of the allowed ones, and we can't add it to the list, 
                // shorten our found sequence to the longest length in the list of allowed lengths!
                for( unsigned int i = 0; 
                     i < m_highNybbleLenghtsPossible.size() && 
                     (m_highNybbleLenghtsPossible[i] + PX_MIN_MATCH_SEQLEN) < result.length; 
                     ++i )
                {
                    //Since the list is sorted, just break once we can't find anything smaller than the value we found !
                    if( (m_highNybbleLenghtsPossible[i] + PX_MIN_MATCH_SEQLEN) < result.length )
                        validhighnybble = m_highNybbleLenghtsPossible[i]; 
                }
                assert( validhighnybble <= (PX_MAX_MATCH_SEQLEN - PX_MIN_MATCH_SEQLEN) );
            }


            int16_t signedoffset = distance( result.itpos, itcurbyte );
            signedoffset = -signedoffset;

            out_result.lownybble     = static_cast<uint8_t>(( signedoffset >> 8 ) & 0x0F);
            out_result.nextbytevalue = static_cast<uint8_t>(signedoffset          & 0xFF);
            out_result.highnybble    = validhighnybble;
            out_result.type          = ePXOperation::COPY_SEQUENCE;

            return true;
        }

        return false;
    }



    /*********************************************************************************
        Because the length is stored as the high nybble in the compressed output, and 
        that the high nybble also contains the ctrl flags, we need to make sure the 
        lengths of sequences to use do not overlap over values of the control flags ! 
        So we'll build a list of length to reserve as we go!

        -> If the value is in our reserved list, and we have PX_NB_POSSIBLE_SEQ_LEN 
            of them already, return true.
        -> If the value isn't in our reserved list, and we still have space left, 
            add it and return true!
        -> If the value isn't in our reserved list, and all PX_NB_POSSIBLE_SEQ_LEN 
            slots are taken, return false!

        NOTE:
            DO NOT pass the exact sequence length. The value stored in the 
            high nybble is essentially : SequenceLen - PX_MIN_MATCH_SEQLEN
    *********************************************************************************/
    template<class _inRandit, class _outRandit>
        bool px_compressor<_inRandit,_outRandit>::CheckSequenceHighNybbleValidOrAdd( uint8_t hnybbleorlen )
    {
        auto itfound = find( m_highNybbleLenghtsPossible.begin(), m_highNybbleLenghtsPossible.end(), hnybbleorlen );

        if( itfound == m_highNybbleLenghtsPossible.end() )
        {
            //We didn't find the length.. Check if we can add it.
            if(  m_highNybbleLenghtsPossible.size() < PX_NB_POSSIBLE_SEQUENCES_LEN )
            {
                m_highNybbleLenghtsPossible.push_back(hnybbleorlen);
                sort( m_highNybbleLenghtsPossible.begin(), m_highNybbleLenghtsPossible.end() );
                return true;
            }
        }
        else
            return true; //We found it in the list!

        return false;
    }

    /*********************************************************************************
        OuptutAnOperation
            Outputs into the output buffer at position "m_itOutCur" the compressed
            form of the operation passed in parameter!
    *********************************************************************************/
    template<class _inRandit, class _outRandit>
        void px_compressor<_inRandit,_outRandit>::OuptutAnOperation(const compOp & operation)
    {
        //if(m_itOutCur == m_itOutEnd)
        //{
        //    assert(false);
        //    throw std::out_of_range("px_compressor::OuptutAnOperation(): Output range too small for compressed output !");
        //}

        if( operation.type == ePXOperation::COPY_ASIS )
        {
            //m_compresseddata.push_back( (operation.highnybble << 4 & 0xF0) | operation.lownybble );
            *m_itOutCur = (operation.highnybble << 4 & 0xF0) | operation.lownybble;
            ++m_itOutCur;
            ++m_nbCompressedByteWritten;
        }
        else if( operation.type == ePXOperation::COPY_SEQUENCE )
        {
            //m_compresseddata.push_back( (operation.highnybble << 4 & 0xF0) | operation.lownybble );
            *m_itOutCur = (operation.highnybble << 4 & 0xF0) | operation.lownybble;
            ++m_itOutCur;
            ++m_nbCompressedByteWritten;
            //m_compresseddata.push_back( operation.nextbytevalue );
            *m_itOutCur = operation.nextbytevalue;
            ++m_itOutCur;
            ++m_nbCompressedByteWritten;
        }
        else
        {
            //m_compresseddata.push_back( ( m_compressioninfo.controlflags[static_cast<uint8_t>(operation.type)] << 4 ) | operation.lownybble );
            *m_itOutCur = ( m_compressioninfo.controlflags[static_cast<uint8_t>(operation.type)] << 4 ) | operation.lownybble;
            ++m_itOutCur;
            ++m_nbCompressedByteWritten;
        }
    }

    /*********************************************************************************
        BuildCtrlFlagsList
            This determines all the control flags values, based on what matching
            sequence lengths have been reserved so far !
    *********************************************************************************/
    template<class _inRandit, class _outRandit>
        void px_compressor<_inRandit, _outRandit>::BuildCtrlFlagsList()
    {
        //Make sure we got PX_NB_POSSIBLE_SEQ_LEN values taken up by the length nybbles
        if( m_highNybbleLenghtsPossible.size() != PX_NB_POSSIBLE_SEQ_LEN )
        {
            // If we don't have PX_NB_POSSIBLE_SEQ_LEN nybbles reserved for the lengths,
            //  just come up with some then.. Its a possible eventuality..
            for( uint8_t nybbleval = 0; nybbleval < 0xF && m_highNybbleLenghtsPossible.size() < PX_NB_POSSIBLE_SEQ_LEN; ++nybbleval )
            {
                auto itfound = find( m_highNybbleLenghtsPossible.begin(), m_highNybbleLenghtsPossible.end(), nybbleval );

                if( itfound == m_highNybbleLenghtsPossible.end() && m_highNybbleLenghtsPossible.size() < PX_NB_POSSIBLE_SEQ_LEN )
                    m_highNybbleLenghtsPossible.push_back(nybbleval);
            }
        }

        //Build our flag list, based on the allowed length values!
        // We only have 16 possible values to contain lengths and control flags..
        array<uint8_t,9>::iterator itctrlflaginsert = m_compressioninfo.controlflags.begin(); //Pos to insert a ctrl flag at

        for( uint8_t flagval = 0; flagval < 0xF; ++flagval )
        {
            auto itfound = find( m_highNybbleLenghtsPossible.begin(), m_highNybbleLenghtsPossible.end(), flagval );
            if( itfound == m_highNybbleLenghtsPossible.end() )
            {
                if( itctrlflaginsert != m_compressioninfo.controlflags.end() )
                {
                    //Flag value is not taken ! So go ahead and make it a control flag value !
                    (*itctrlflaginsert) = flagval;
                    ++itctrlflaginsert;
                }
            }
        }
    }

    /*********************************************************************************
        OutputAllOperations
            This does the neccessary to execute all operations we put in our operation
            queue. It also calculate the proper high nybble value for operation 
            using a control flag index !
    *********************************************************************************/
    template<class _inRandit, class _outRandit>
        void px_compressor<_inRandit,_outRandit>::OutputAllOperations( atomic<uint8_t> * pPercentDone )
    {
        uint32_t initialNbOps = m_PendingOperations.size();

        //Output all our operations !
        while( !m_PendingOperations.empty() )
        {
            //Make a command byte using the 8 first operations in the operation queue !
            uint8_t commandbyte = 0;
            for( unsigned int i = 0; i < 8u && m_PendingOperations.size() > i; ++i )
            {
                if(m_PendingOperations[i].type == ePXOperation::COPY_ASIS)
                {
                    //Set the bit to 1 only when we copy the byte as-is !
                    commandbyte |= 1 << (7 - i);
                }
            }


            //Output command byte (FFS how did I manage to forget about that the first time ?!)
            //(*m_itOutCur) = commandbyte;
            *m_itOutCur =  commandbyte;
            ++m_itOutCur;
            ++m_nbCompressedByteWritten;

            //m_compresseddata.push_back( commandbyte );
            //(*m_itOutCur) = commandbyte;
            //++m_itOutCur;

            //Run 8 operations before another command byte !
            for( int i = 0; i < 8 && !m_PendingOperations.empty(); ++i )
            {
                OuptutAnOperation( m_PendingOperations.front() );
                m_PendingOperations.pop_front();
            }

            //Update progress
            if( pPercentDone != nullptr )
                (*pPercentDone) = static_cast<uint8_t>( ((initialNbOps - m_PendingOperations.size()) * 100u) / initialNbOps );
        }

        //After we're done, shrink down the vector, and remove all the extra space
        //uint32_t ouputsize = distance( m_itOutBeg, m_itOutCur );
        //m_compresseddata.resize(ouputsize); //After this all our iterators are invalidated !
    }

//=========================================
//              Functions
//=========================================

    //Print progress
    void PrintProgress( multistep_completion<2> & progressatomic, uint64_t totalsize )
    {
        uint32_t total = 0;

        cout<<"\r";
        for( unsigned int i = 0; i < progressatomic.steps.size(); ++i )
        {
            cout <<"Step " <<i <<":" <<std::setfill(' ') <<std::setw(3) <<std::dec 
                <<static_cast<unsigned int>(progressatomic.steps[i].load()) <<"%, ";
            total += progressatomic.steps[i];
        }
        cout <<"Total " <<static_cast<unsigned short>((total * 100) / (progressatomic.steps.size() * 100)) <<"%";
        cout.flush();
    }


    /*********************************************************************************
        DecompressPX
    *********************************************************************************/
    //template<class _init, class _outstdcontainer>
    //    void DecompressPX( px_info_header             info, 
    //                       _init                      itdatabeg, 
    //                       _init                      itdataend, 
    //                       _outstdcontainer         & out_decompresseddata,
    //                       bool                       blogenabled)

    void DecompressPX( px_info_header                         info, 
                       std::vector<uint8_t>::const_iterator   itdatabeg, 
                       std::vector<uint8_t>::const_iterator   itdataend, 
                       std::vector<uint8_t>                 & out_decompresseddata,
                       bool                                   blogenabled)
    {
        //Resize the vector properly
        //out_decompresseddata.resize( info.decompressedsz );
        if(info.decompressedsz != out_decompresseddata.size()) //Those must be the same size !
        {
            stringstream sstr;
            sstr << "DecompressPX() : The output buffer is not of the expected size! Current buffer size : "
                 << out_decompresseddata.size() << " bytes, expected " <<info.decompressedsz <<" bytes!";
            throw std::runtime_error( sstr.str() );
        }

        //Create our state
        px_decompressor<std::vector<uint8_t>::const_iterator, std::vector<uint8_t>::iterator>
                        ( itdatabeg, 
                          itdataend, 
                          out_decompresseddata.begin(),  
                          out_decompresseddata.end(), 
                          info,
                          blogenabled ).DecompressPX(); //Run right after construction, we don't use it afterwards anyways
    }

    void DecompressPX( px_info_header                         info, 
                       std::vector<uint8_t>::const_iterator   itdatabeg, 
                       std::vector<uint8_t>::const_iterator   itdataend, 
                       std::vector<uint8_t>::iterator         itoutbeg, 
                       std::vector<uint8_t>::iterator         itoutend, 
                       bool                                   blogenabled)
    {
        //Resize the vector properly
        auto diff = distance( itoutbeg, itoutend );
        if(info.decompressedsz != diff ) //Those must be the same size !
        {
            stringstream sstr;
            sstr << "DecompressPX() : The output buffer is not of the expected size! Current buffer size : "
                 << diff << " bytes, expected " <<info.decompressedsz <<" bytes!";
            throw std::runtime_error( sstr.str() );
        }

        //Create our state
        px_decompressor<std::vector<uint8_t>::const_iterator, std::vector<uint8_t>::iterator>
                        ( itdatabeg, 
                          itdataend, 
                          itoutbeg,  
                          itoutend, 
                          info,
                          blogenabled ).DecompressPX(); //Run right after construction, we don't use it afterwards anyways
    }

    /*********************************************************************************
        CompressPX
    *********************************************************************************/
    px_info_header CompressPX( vector<uint8_t>::const_iterator   itdatabeg, 
                               vector<uint8_t>::const_iterator   itdataend,
                               vector<uint8_t>                 & out_compresseddata,
                               ePXCompLevel                      compressionlvl,
                               bool                              bZealousSearch,
                               bool                              displayprogress,
                               bool                              blogenabled)
    {
        multistep_completion<2> mycompletion;
        atomic<bool>            shouldstopthread(false);
        uint64_t                origfilesize = distance(itdatabeg,itdataend);
        
        auto lambdaProgress = []( atomic<bool> & shouldstop, multistep_completion<2> & progressatomic, uint64_t totalsize )->bool
        {
            while( !shouldstop )
            {
                PrintProgress( progressatomic, totalsize );
                this_thread::sleep_for( std::chrono::milliseconds(100) );
            }

            //Write the progress one last time
            PrintProgress( progressatomic, totalsize );
            return true;
        };

        px_info_header result;

        if( displayprogress )
        {
            auto myfuture = std::async( std::launch::async, lambdaProgress, std::ref(shouldstopthread), std::ref(mycompletion), origfilesize );
            result        = px_compressor<vector<uint8_t>::const_iterator>( out_compresseddata, itdatabeg, itdataend, blogenabled ).Compress(compressionlvl, bZealousSearch, &(mycompletion) );
            shouldstopthread = true;
            myfuture.get();
        }
        else
        {
            result = px_compressor<vector<uint8_t>::const_iterator>( out_compresseddata, itdatabeg, itdataend, blogenabled ).Compress(compressionlvl, bZealousSearch );
        }

        return result;
    }


    //template<class _init, class _randit>
    //    px_info_header CompressPX( _init        itdatabeg,
    //                               _init        itdataend,
    //                               _randit      itoutbeg,
    //                               _randit      itoutend,
    //                               ePXCompLevel compressionlvl,
    //                               bool         bZealousSearch,
    //                               bool         displayprogress,
    //                               bool         blogenabled )

    //px_info_header CompressPX( std::vector<uint8_t>::const_iterator itdatabeg,
    //                           std::vector<uint8_t>::const_iterator itdataend,
    //                           std::vector<uint8_t>::iterator       itoutbeg,
    //                           ePXCompLevel                          compressionlvl,
    //                           bool                                  bZealousSearch,
    //                           bool                                  displayprogress,
    //                           bool                                  blogenabled )
    //{
    //    //#1 - Check if we at least have enough space between the output iterators
    //    //unsigned int inputsz  = std::distance( itdatabeg, itdataend );
    //    //unsigned int outputsz = std::distance( itoutbeg,  itoutend  );

    //    //if( compressionlvl == ePXCompLevel::LEVEL_0 && !( ( (inputsz / 8) + (inputsz % 8) + inputsz ) <= outputsz ) )
    //    //{
    //    //    assert(false);
    //    //    throw std::out_of_range( "CompressPX(): Compression level 0. Not enough space within output range to output compressed data !" );
    //    //    return px_info_header();
    //    //}
    //    //else if( !( outputsz >= inputsz ) ) //We need at least as much space
    //    //{
    //    //    assert(false);
    //    //    throw std::out_of_range( "CompressPX(): Not enough space within output range to output compressed data !" );
    //    //    return px_info_header();
    //    //}

    //    multistep_completion<2> mycompletion;
    //    atomic<bool>            shouldstopthread(false);
    //    uint64_t                origfilesize = distance(itdatabeg,itdataend);
    //    
    //    auto lambdaProgress = []( atomic<bool> & shouldstop, multistep_completion<2> & progressatomic, uint64_t totalsize )->bool
    //    {
    //        while( !shouldstop )
    //        {
    //            PrintProgress( progressatomic, totalsize );
    //            this_thread::sleep_for( std::chrono::milliseconds(100) );
    //        }

    //        //Write the progress one last time
    //        PrintProgress( progressatomic, totalsize );
    //        return true;
    //    };

    //    px_info_header result;

    //    if( displayprogress )
    //    {
    //        auto myfuture = std::async( std::launch::async, lambdaProgress, std::ref(shouldstopthread), std::ref(mycompletion), origfilesize );
    //        result        = px_compressor< std::vector<uint8_t>::const_iterator,  std::vector<uint8_t>::iterator >
    //                        ( itdatabeg, itdataend, itoutbeg, blogenabled ).Compress(compressionlvl, bZealousSearch, &(mycompletion) );
    //        shouldstopthread = true;
    //        myfuture.get();
    //    }
    //    else
    //    {
    //        result = px_compressor<  std::vector<uint8_t>::const_iterator,  std::vector<uint8_t>::iterator >
    //                 ( itdatabeg, itdataend, itoutbeg, blogenabled ).Compress(compressionlvl, bZealousSearch );
    //    }

    //    return result;
    //}


    px_info_header CompressPX( std::vector<uint8_t>::const_iterator            itdatabeg,
                               std::vector<uint8_t>::const_iterator            itdataend,
                               std::back_insert_iterator<std::vector<uint8_t>> itoutbeg,
                               ePXCompLevel                                    compressionlvl,
                               bool                                            bZealousSearch,
                               bool                                            displayprogress, 
                               bool                                            blogenabled )
    {
        multistep_completion<2> mycompletion;
        atomic<bool>            shouldstopthread(false);
        uint64_t                origfilesize = distance(itdatabeg,itdataend);
        
        auto lambdaProgress = []( atomic<bool> & shouldstop, multistep_completion<2> & progressatomic, uint64_t totalsize )->bool
        {
            while( !shouldstop )
            {
                PrintProgress( progressatomic, totalsize );
                this_thread::sleep_for( std::chrono::milliseconds(100) );
            }

            //Write the progress one last time
            PrintProgress( progressatomic, totalsize );
            return true;
        };

        px_info_header result;

        if( displayprogress )
        {
            auto myfuture = std::async( std::launch::async, lambdaProgress, std::ref(shouldstopthread), std::ref(mycompletion), origfilesize );
            result        = px_compressor< std::vector<uint8_t>::const_iterator,  decltype(itoutbeg) >
                            ( itdatabeg, itdataend, itoutbeg, blogenabled ).Compress(compressionlvl, bZealousSearch, &(mycompletion) );
            shouldstopthread = true;
            myfuture.get();
        }
        else
        {
            result = px_compressor<  std::vector<uint8_t>::const_iterator,  decltype(itoutbeg) >
                     ( itdatabeg, itdataend, itoutbeg, blogenabled ).Compress(compressionlvl, bZealousSearch );
        }

        return result;
    }



    /*********************************************************************************
        CleanExistingCompressionLogs
    *********************************************************************************/
    //void CleanExistingCompressionLogs()
    //{
    //    Poco::File logfileComp( PX_COMPRESSION_LOGFILE_NAME ), 
    //               logfileDecomp( PX_DECOMPRESSION_LOGFILE_NAME );

    //    if( logfileComp.exists() )
    //        logfileComp.remove();

    //    if( logfileDecomp.exists() )
    //        logfileDecomp.remove();

    //    cout<<"Cleaned log files!\n";
    //}

};
#endif