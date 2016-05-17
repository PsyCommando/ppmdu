#include "sf2.hpp"
#include <ext_fmts/riff.hpp>
#include <utils/library_wide.hpp>

#include <iostream>
#include <iomanip>
#include <fstream>
#include <array>
#include <functional>
#include <cassert>
#include <sstream>
using namespace std;

//This define is meant to be a temporary mean of toggling on and off the adding of loop bytes to make looping
//  samples much smoother.
//#define SF2_ADD_EXTRA_LOOP_BYTES 1
#define SF2_ADD_EXTRA_LOOP_BYTES2 1

namespace sf2
{
//=========================================================================================
//  Structs
//=========================================================================================

    /*
        ifilDat
            Chunk data idicating the SoundFont specification revision that the file 
            complies to.
    */
    struct ifilDat
    {
       static const uint32_t SIZE = 4;//bytes
       ifilDat( uint16_t maj = 2, uint16_t min = 1 )
           :major(maj), minor(min)
       {}

       uint16_t major; //Soundfont 2.01 by default
       uint16_t minor;

        template<class _outit> _outit Write( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( major, itwriteto );
            itwriteto = utils::WriteIntToBytes( minor, itwriteto );
            return itwriteto;
        }


        template<class _init> _init Read( _init itReadfrom )
        {
            itReadfrom = utils::ReadIntFromBytes( major, itReadfrom );
            itReadfrom = utils::ReadIntFromBytes( minor, itReadfrom );
            return itReadfrom;
        }
    };



//=========================================================================================
//  Constants
//=========================================================================================
    static const uint32_t    RIFF_HeaderTotalLen     = 12; //bytes The length of the header + the format tag

    static const uint32_t    SfSampleLoopMinEdgeDist =  8;  //The amount of sample points to copy on either sides of the loop within a sample.
    static const uint32_t    SfSampleLoopMinEndEdgeDist = 16;
    static const uint32_t    SfSampleLoopMinLen      = 32;  //Minimum distance between the start of a loop and the end, for it to be SF2 loopable.
    static const size_t      SfMinSampleZeroPad      = 92;  //Minimum amount of bytes of zero padding to append a sample. 
    static const size_t      SfMaxLongCString        = 256; //Maximum size of a long c-string. For example the ISFT chunk's software name.

    //Byte Lengths for the various chunks
    static const uint32_t    SfEntrySHDR_Len         = 46;
    static const uint32_t    SfEntryPHDR_Len         = 38;
    static const uint32_t    SfEntryPBag_Len         =  4;
    static const uint32_t    SfEntryPMod_Len         = 10;
    static const uint32_t    SfEntryPGen_Len         =  4;
    static const uint32_t    SfEntryInst_Len         = 22;
    static const uint32_t    SfEntryIBag_Len         =  4;
    static const uint32_t    SfEntryIMod_Len         = 10;
    static const uint32_t    SfEntryIGen_Len         =  4;

    static const string      SF_DefIsng              = "E-mu 10K1"; // "EMU8000"; //"E-mu 10K1";
    static const string      SF_DefSft               = "ppmd_audioutil";
    static const ifilDat     SF_VersChnkData         ( 2, 1 );  //Soundfont 2.01

    /*
        Format tags for the chunks used in the SF2 format.
    */
    enum struct eSF2Tags : uint32_t
    {
        //Main Tags
        sfbk = 0x7366626B, //"sfbk" format tag. Identify the RIFF chunk as being a sounfont
        INFO = 0x494E464F, //"INFO" fmt tag. Identify the INFO list chunk.
        sdta = 0x73647461, //"sdta" fmt tag. Identify the sdta list chunk.
        pdta = 0x70647461, //"pdta" fmt tag. Identify the pdta list chunk.

        //Mandatory INFO
        ifil = 0x6966696C, //"ifil" This chunk identify the soundont version this soundfont complies to.
        isng = 0x69736E67, //"isng" 
        INAM = 0x494E414D, //"INAM"

        //Mandatory sdta
        smpl = 0x736D706C, //"smpl"

        //Optionals INFO
        irom = 0x69726F6D, //"irom"
        iver = 0x69766572, //"iver"
        ICRD = 0x49435244, //"ICRD"
        IENG = 0x49454E47, //"IENG"
        IPRD = 0x49505244, //"IPRD"
        ICOP = 0x49434F50, //"ICOP"
        ICMT = 0x49434D54, //"ICMT"
        ISFT = 0x49534654, //"ISFT"

        //Madatory pdta
        phdr = 0x70686472, //"phdr"
        pbag = 0x70626167, //"pbag"
        pmod = 0x706D6F64, //"pmod"
        pgen = 0x7067656E, //"pgen"
        inst = 0x696E7374, //"inst"
        ibag = 0x69626167, //"ibag"
        imod = 0x696D6F64, //"imod"
        igen = 0x6967656E, //"igen"
        shdr = 0x73686472, //"shdr"
    };

    ////Values ranges for the Generators!
    //static const value_limits<int16_t>  SF_GenLimitsModLfoToPitch      { -12000,      0, 12000 };
    //static const value_limits<int16_t>  SF_GenLimitsVibLfoToPitch      { -12000,      0, 12000 };
    //static const value_limits<int16_t>  SF_GenLimitsModEnvLfoToPitch   { -12000,      0, 12000 };
    //static const value_limits<uint16_t> SF_GenLimitsInitFilterFc       {   1500,  13500, 13500 };
    //static const value_limits<uint16_t> SF_GenLimitsInitFilterQ        {      0,      0,   960 };
    //static const value_limits<int16_t>  SF_GenLimitsModLfoToFilterFc   { -12000,      0, 12000 };
    //static const value_limits<int16_t>  SF_GenLimitsModEnvLfoToFilterFc{ -12000,      0, 12000 };
    //static const value_limits<int16_t>  SF_GenLimitsModLfoToVolume     {   -960,      0,   960 };
    //static const value_limits<uint16_t> SF_GenLimitsChorusSend         {      0,      0,  1000 };
    //static const value_limits<uint16_t> SF_GenLimitsReverbSend         {      0,      0,  1000 };
    //static const value_limits<int16_t>  SF_GenLimitsPan                {   -500,      0,   500 };

    //static const value_limits<int16_t>  SF_GenLimitsModLfoDelay        { -12000, -12000,  5000 };
    //static const value_limits<int16_t>  SF_GenLimitsModLfoFreq         { -16000,      0,  4500 };
    //static const value_limits<int16_t>  SF_GenLimitsVibLfoDelay        { -12000, -12000,  5000 };
    //static const value_limits<int16_t>  SF_GenLimitsVibLfoFreq         { -16000,      0,  4500 };

    //static const value_limits<int16_t>  SF_GenLimitsModEnvDelay        { -12000, -12000,  5000 };
    //static const value_limits<int16_t>  SF_GenLimitsModEnvAttack       { -12000, -12000,  8000 };
    //static const value_limits<int16_t>  SF_GenLimitsModEnvHold         { -12000, -12000,  5000 };
    //static const value_limits<int16_t>  SF_GenLimitsModEnvDecay        { -12000, -12000,  8000 };
    //static const value_limits<uint16_t> SF_GenLimitsModEnvSustain      {      0,      0,  1000 };
    //static const value_limits<int16_t>  SF_GenLimitsModEnvRelease      { -12000, -12000,  8000 };

    //static const value_limits<int16_t>  SF_GenLimitsKeynumToModEnvHold {  -1200,      0,  1200 };
    //static const value_limits<int16_t>  SF_GenLimitsKeynumToModEnvDecay{  -1200,      0,  1200 };

    //static const value_limits<int16_t>  SF_GenLimitsVolEnvDelay        { SHRT_MIN, SHRT_MIN,  5000 };
    //static const value_limits<int16_t>  SF_GenLimitsVolEnvAttack       { SHRT_MIN, SHRT_MIN,  8000 };
    //static const value_limits<int16_t>  SF_GenLimitsVolEnvHold         { SHRT_MIN, SHRT_MIN,  5000 };
    //static const value_limits<int16_t>  SF_GenLimitsVolEnvDecay        { SHRT_MIN, SHRT_MIN,  8000 };
    //static const value_limits<uint16_t> SF_GenLimitsVolEnvSustain      {      0,      0,  1440 };
    //static const value_limits<int16_t>  SF_GenLimitsVolEnvRelease      { SHRT_MIN, SHRT_MIN,  8000 };

    //static const value_limits<int16_t>  SF_GenLimitsKeynumToVolEnvHold {  -1200,      0,  1200 };
    //static const value_limits<int16_t>  SF_GenLimitsKeynumToVolEnvDecay{  -1200,      0,  1200 };

    //static const value_limits<uint16_t> SF_GenLimitsKeyRange           { 0x0000, 0x007F, 0x7F7F};
    //static const value_limits<uint16_t> SF_GenLimitsVelRange           { 0x0000, 0x007F, 0x7F7F};

    //static const value_limits<int16_t> SF_GenLimitsKeynum              {      0,     -1,   127 };
    //static const value_limits<int16_t> SF_GenLimitsVelocity            {      0,     -1,   127 };

    //static const value_limits<int16_t> SF_GenLimitsInitAttenuation     {      0,      0,  1440 };

    //static const value_limits<int16_t> SF_GenLimitsCoarseTune          {   -120,      0,   120 };
    //static const value_limits<int16_t> SF_GenLimitsFineTune            {    -99,      0,    99 };

    //static const value_limits<uint16_t> SF_GenLimitsScaleTuning        {      0,    100,  1200 };
    //static const value_limits<uint16_t> SF_GenLimitsExcClass           {      0,      0,   127 };
    //static const value_limits<uint16_t> SF_GenLimitsOverrideRootKey    {      0,     -1,   127 };

//=========================================================================================
//  Utility
//=========================================================================================

    /**************************************************************************************
        MakeSampleLoopLegal
            This coppy the looped data points of a sample a few times, if the sample 
            is too short to be looped according to the SF2 format's specs.

            It also copy the first 8 data points if the loopbeg is the same pos as the
            beginning of the sample. 
            It does the same with the end.

            So that :
                - loopbeg > (beg + 7)
                - loopend < (end - 7)
                - (loopend - loopend) >= 32 
    **************************************************************************************/
#if 0
    std::vector<pcm16s_t> & MakeSampleLoopLegal( std::vector<pcm16s_t> & sample, uint32_t & loopbeg, uint32_t & loopend )
    {
        const size_t          smpllen      = sample.size();
        const int32_t         looplen      = loopend - loopbeg;
        size_t                allocsz      = smpllen;
        uint32_t              loopntimes   = 1;                 //The nb of times the samples should be looped to be legal

        //#0 - Determine what to do with this sample
        const bool bPrefixSmpls = loopbeg < SfSampleLoopMinEdgeDist;             //Whether we need to insert samples between beg and loopbeg
        const bool bSuffixSmpls = (smpllen - loopend) < SfSampleLoopMinEndEdgeDist; //Whether we need to insert samples between loopend and end
        const bool bExtraLoops  = looplen < SfSampleLoopMinLen;                  //Whether we need to loop the loop zone a few times to make this legal

        if( !bPrefixSmpls && !bSuffixSmpls && !bExtraLoops )
            return sample; //Nothing to do here !

        const uint32_t distbeglp2beg = loopbeg;             //The nb of samples between the start of the loop and the beginning of the sample's data
        const uint32_t distlpend2end = (smpllen - loopend); //The nb of samples between the end of the loop and the end of the sample's data

       //#1 - Compute size to reserve
        if( bExtraLoops )
        {
            loopntimes = SfSampleLoopMinLen / looplen; //Compute the amount of times to loop the sample to reach the legal length

            //If there is any remainder, just loop it another time !
            if( (SfSampleLoopMinLen % looplen) != 0 )
                ++loopntimes;

            allocsz += ( loopntimes * looplen ); //Add those extra loop samples to the amount to reserve
        }

        //
        unsigned int nbprelps  = 0;
        unsigned int nbsufflps = 0;

        if( bPrefixSmpls )
        {
            if( SfSampleLoopMinEdgeDist > looplen ) 
            {
                nbprelps = SfSampleLoopMinEdgeDist / looplen;
                if( SfSampleLoopMinEdgeDist % looplen != 0 )
                    ++nbprelps;
            }
            else
                ++nbprelps;
            loopntimes += nbprelps;
            allocsz += ( nbprelps * looplen ); //Add the nb of dummy samples to prefix
        }
        if( bSuffixSmpls )
        {
            if( SfSampleLoopMinEndEdgeDist > looplen ) 
            {
                nbsufflps = SfSampleLoopMinEndEdgeDist / looplen;
                if( SfSampleLoopMinEndEdgeDist % looplen != 0 )
                    ++nbsufflps;
            }
            else
                ++nbsufflps;
            loopntimes += nbsufflps;
            allocsz += ( nbsufflps * looplen ); //Add the nb of dummy samples to append
        }


        //#2 - Allocate
        std::vector<pcm16s_t> legalloop;
        legalloop.reserve( allocsz );

        //#3 - Assemble the new sample
        auto legalloopins = back_inserter( legalloop );

        //Copy whatever is between beg and loopbeg
        std::copy_n( sample.begin(), distbeglp2beg, legalloopins );

        //Put the data between the loop points as many times as needed
        auto itlpbeg = sample.begin() + distbeglp2beg;
        for( unsigned int cntlp = 0; cntlp < loopntimes; ++cntlp ) //Copy the sample's looped zone as many times as needed
            std::copy_n( itlpbeg, looplen, legalloopins );

        //Copy whatever is between loopend and end
        std::copy_n( sample.begin(), distlpend2end, legalloopins );

        //Move loop points!
        loopbeg = distbeglp2beg + ( nbprelps * looplen );
        loopend = loopbeg       + ( ( loopntimes - nbsufflps ) * looplen );

        sample = std::move(legalloop);
        return sample;
    }
#endif
    //{
    //    const size_t          smpllen      = sample.size();
    //    const int32_t         looplen      = loopend - loopbeg;
    //    size_t                allocsz      = smpllen;
    //    uint32_t              loopntimes   = 1;                 //The nb of times the samples should be looped to be legal

    //    //#0 - Determine what to do with this sample
    //    const bool bPrefixSmpls = loopbeg < SfSampleLoopMinEdgeDist;             //Whether we need to insert samples between beg and loopbeg
    //    const bool bSuffixSmpls = (smpllen - loopend) < SfSampleLoopMinEndEdgeDist; //Whether we need to insert samples between loopend and end
    //    const bool bExtraLoops  = looplen < SfSampleLoopMinLen;                  //Whether we need to loop the loop zone a few times to make this legal

    //    if( utils::LibWide().isLogOn() )
    //    {
    //        clog << "\tMakeSampleLoopLegal():\n" <<setw(10) <<setfill(' ')
    //             << "\t\tLength"    <<": " << smpllen <<" smpls\n" <<setw(10)
    //             << "\t\tLoopBeg"   <<": " << loopbeg <<" smpls\n" <<setw(10)
    //             << "\t\tLoopEnd"   <<": " << loopend <<" smpls\n" <<setw(10)
    //             << "\t\tLoopLen"   <<": " << looplen <<" smpls\n" <<setw(10)
    //             <<boolalpha 
    //             << "\t\tDoPrefix"  <<": " << bPrefixSmpls <<"\n" <<setw(10)
    //             << "\t\tDoSufix"   <<": " << bSuffixSmpls <<"\n" <<setw(10)
    //             << "\t\tDoExtraLp" <<": " << bExtraLoops <<"\n";
    //    }

    //    if( !bPrefixSmpls && !bSuffixSmpls && !bExtraLoops )
    //        return sample; //Nothing to do here !

    //    const uint32_t distbeglp2beg = loopbeg;             //The nb of samples between the start of the loop and the beginning of the sample's data
    //    const uint32_t distlpend2end = (smpllen - loopend); //The nb of samples between the end of the loop and the end of the sample's data
    //    const uint32_t nbdummyprefx  = std::max(0, static_cast<int32_t>(SfSampleLoopMinEdgeDist - distbeglp2beg) );
    //    const uint32_t nbdummypost   = std::max(0, static_cast<int32_t>(SfSampleLoopMinEdgeDist - distlpend2end) );

    //    //#1 - Compute size to reserve
    //    if( bExtraLoops )
    //    {
    //        loopntimes = SfSampleLoopMinLen / looplen; //Compute the amount of times to loop the sample to reach the legal length

    //        //If there is any remainder, just loop it another time !
    //        if( (SfSampleLoopMinLen % looplen) != 0 )
    //            ++loopntimes;

    //        allocsz += ( loopntimes * looplen ); //Add those extra loop samples to the amount to reserve
    //    }

    //    if( bPrefixSmpls )
    //        allocsz += nbdummyprefx; //Add the nb of dummy samples to prefix
    //    if( bSuffixSmpls )
    //        allocsz += nbdummypost; //Add the nb of dummy samples to append

    //    //#2 - Allocate
    //    std::vector<pcm16s_t> legalloop;
    //    legalloop.reserve( allocsz );

    //    //#3 - Assemble the new sample
    //    auto legalloopins = back_inserter( legalloop );

    //    //Copy whatever is between beg and loopbeg
    //    std::copy_n( sample.begin(), distbeglp2beg, legalloopins );

    //    if( bPrefixSmpls )
    //    {
    //        auto itcpbeg = sample.begin();
    //        //std::advance( itcpbeg, distbeglp2beg );

    //        //if( distbeglp2beg <= 1 )
    //            std::fill_n( legalloopins, nbdummyprefx, *itcpbeg );
    //        //else
    //        //    std::copy_n( itcpbeg, nbdummyprefx, legalloopins );
    //    }

    //    //Put the data between the loop points as many times as needed
    //    auto itlpbeg = sample.begin() + distbeglp2beg;
    //    for( unsigned int cntlp = 0; cntlp < loopntimes; ++cntlp ) //Copy the sample's looped zone as many times as needed
    //        std::copy_n( itlpbeg, looplen, legalloopins );

    //    //Copy whatever is between loopend and end
    //    std::copy_n( sample.begin(), distlpend2end, legalloopins );

    //    if( bSuffixSmpls )
    //    {
    //        auto itcpbeg = sample.begin();
    //        std::advance( itcpbeg, distbeglp2beg );
    //        std::copy_n( itcpbeg, nbdummypost, legalloopins );
    //    }


    //    if( utils::LibWide().isLogOn() )
    //    {
    //        clog <<setw(10) << "\t\tDistBegLpToBeg" <<": " <<distbeglp2beg <<"\n"
    //             <<setw(10) << "\t\tDistEndLpToEnd" <<": " <<distlpend2end <<"\n"
    //             <<setw(10) << "\t\tLoopNTimes"     <<": " <<loopntimes <<"\n";
    //    }

    //    //#4 - Move the legal sample into the old one
    //    sample = std::move(legalloop);
    //    return sample;
    //}


//=========================================================================================
//  Utilities
//=========================================================================================

/************************************************************************************************************
    RAII_WriteListChunk
        Little RAII trick to write the list chunk and avoid needing to do some dumb BS like I used to do!

        Basically, create this within its own sub-scope, run the method that writes the content of the 
        list chunk, and when the object falls out of scope it'll handle the rest!

*************************************************************************************************************/
    class RAII_WriteListChunk
    {
        std::ofstream                 * m_pout;
        uint32_t                        m_fmttag;
        size_t                        * m_onbbyteswritten;
        std::ostreambuf_iterator<char>  m_itout;
        std::ofstream::streampos        m_prewrite;

    public:

        RAII_WriteListChunk( std::ofstream & of, uint32_t fmttag, size_t & out_nbbyteswritten )
            :m_fmttag(fmttag), m_pout(&of), m_onbbyteswritten(&out_nbbyteswritten), m_itout(of), 
             m_prewrite(of.tellp()) //Save pre-write pos
        {
            //Zero out the nb of bytes written
            (*m_onbbyteswritten) = 0;

            Init();
        }

        RAII_WriteListChunk( std::ofstream & of, uint32_t fmttag )
            :m_fmttag(fmttag), m_pout(&of), m_onbbyteswritten(nullptr), m_itout(of), 
             m_prewrite(of.tellp()) //Save pre-write pos
        {
            Init();
        }


        ~RAII_WriteListChunk()
        {
    #ifdef _DEBUG
            m_pout->flush();
    #endif

            //Save Post-write pos
            const std::ofstream::streampos postwrite = m_pout->tellp();

            //Seek back to start
            m_pout->seekp(m_prewrite);

            //Write header
            riff::ChunkHeader listhdr;
            listhdr.chunk_id = static_cast<uint32_t>(riff::eChunkIDs::LIST);
            listhdr.length   = static_cast<uint32_t>(postwrite - m_prewrite) - riff::ChunkHeader::SIZE; //Don't count the header itself

            m_itout = listhdr.Write(m_itout);

            //Seek back to end
            m_pout->seekp(postwrite);

            //Return our total
            if( m_onbbyteswritten != nullptr )
                (*m_onbbyteswritten) = (postwrite - m_prewrite);
        }

    private:
        void Init()
        {
            //Skip header size
            m_itout = std::fill_n( m_itout, riff::ChunkHeader::SIZE, 0 );
    #ifdef _DEBUG
            m_pout->flush();
    #endif

            //Write format tag
            m_itout = utils::WriteIntToBytes( static_cast<uint32_t>(m_fmttag), m_itout, false );
    #ifdef _DEBUG
            m_pout->flush();
    #endif
        }

    };

/************************************************************************************************************
    RAII_WriteChunk
        Little RAII trick to write the chunk and avoid needing to do some dumb BS like I used to do!

        Basically, create this within its own sub-scope, run the method that writes the content of the 
        list chunk, and when the object falls out of scope it'll handle the rest!

*************************************************************************************************************/
    class RAII_WriteChunk
    {
        eSF2Tags                        m_tag;
        std::ofstream                 * m_pout;
        std::ofstream::streampos        m_prewritepos;
        std::ostreambuf_iterator<char>  m_itout;
        size_t                        * m_onbbyteswritten;

    public:

        RAII_WriteChunk( std::ofstream & of, eSF2Tags tag, size_t & out_nbbyteswritten )
            :m_pout(&of), m_itout(of), m_tag(tag), m_onbbyteswritten(&out_nbbyteswritten),
             m_prewritepos(of.tellp()) //Save pre-write pos
        {
            //Zero out the outputed nb of bytes written
            (*m_onbbyteswritten) = 0;

            //Skip header size
            std::fill_n( std::ostreambuf_iterator<char>(of), riff::ChunkHeader::SIZE, 0 );
        }

        RAII_WriteChunk( std::ofstream & of, eSF2Tags tag )
            :m_pout(&of), m_itout(of), m_tag(tag), m_onbbyteswritten(nullptr),
             m_prewritepos(of.tellp()) //Save pre-write pos
        {
            //Skip header size
            std::fill_n( std::ostreambuf_iterator<char>(of), riff::ChunkHeader::SIZE, 0 );
        }

        ~RAII_WriteChunk()
        {
            //Save Post-write pos
            const std::ofstream::streampos postwrite = m_pout->tellp();

            //Seek back to start
            m_pout->seekp(m_prewritepos);

            //Write header
            riff::ChunkHeader listhdr;
            listhdr.chunk_id = static_cast<uint32_t>(m_tag);
            listhdr.length   = static_cast<uint32_t>(postwrite - m_prewritepos) - riff::ChunkHeader::SIZE; //Don't count the header itself

            m_itout = listhdr.Write(m_itout);

            //Seek back to end
            m_pout->seekp(postwrite);

            //Return our total
            if( m_onbbyteswritten != nullptr )
                (*m_onbbyteswritten) = (postwrite - m_prewritepos);
        }
    };




//=========================================================================================
//  SounFontRIFFWriter
//=========================================================================================
    class SounFontRIFFWriter
    {
        typedef std::function<std::ofstream::streampos()> listmethodfun_t;
    public:
        SounFontRIFFWriter( const SoundFont & sf )
            :m_sf(sf)
        {}

        //output : stream open in binary mode
        std::ofstream Write( std::ofstream && output )
        {
            m_out = std::move(output);

            //Validate
            if( m_sf.GetNbInstruments() > numeric_limits<uint16_t>::max() )
            {
                stringstream sstr;
                sstr << "The number of instruments in the soundfont exceeds the maximum amount supported by the soundfont 2.01 format!"
                     << "Expected less than " <<numeric_limits<uint16_t>::max() <<", but got " <<m_sf.GetNbInstruments() <<"!\n";
                throw std::runtime_error(sstr.str());
            }
            if( m_sf.GetNbPresets() > numeric_limits<uint16_t>::max() )
            {
                stringstream sstr;
                sstr << "The number of presets in the soundfont exceeds the maximum amount supported by the soundfont 2.01 format!"
                     << "Expected less than " <<numeric_limits<uint16_t>::max() <<", but got " <<m_sf.GetNbPresets() <<"!\n";
                throw std::runtime_error(sstr.str());
            }
            if( m_sf.GetNbSamples() > numeric_limits<uint16_t>::max() )
            {
                stringstream sstr;
                sstr << "The number of samples in the soundfont exceeds the maximum amount supported by the soundfont 2.01 format!"
                     << "Expected less than " <<numeric_limits<uint16_t>::max() <<", but got " <<m_sf.GetNbSamples() <<"!\n";
                throw std::runtime_error(sstr.str());
            }

            //Save pre-write pos to go back to for writing the header later
            const std::ofstream::streampos prewriteoffset = m_out.tellp();

            //Skip the first 8 bytes for the RIFF header
            std::fill_n( std::ostreambuf_iterator<char>(m_out), riff::ChunkHeader::SIZE, 0 );

            //Write fmt tag
            utils::WriteIntToBytes( static_cast<uint32_t>(eSF2Tags::sfbk), ostreambuf_iterator<char>(m_out), false );

            //Write the content
            size_t                   nbwritten = 0; //Variable re-used for keeping track of the nb of bytes written
            std::ofstream::streampos datasz    = 4; //Count the 4 bytes of the fmt tag

            {
                RAII_WriteListChunk wl( m_out, static_cast<uint32_t>(eSF2Tags::INFO), nbwritten );
                WriteInfoList();
                //datasz += WriteListChunk( static_cast<uint32_t>(eSF2Tags::INFO), std::bind(&SounFontRIFFWriter::WriteInfoList,  this ) );
            }
            datasz += nbwritten;
            nbwritten = 0;

            //Build and write the sample data
            {
                RAII_WriteListChunk wl( m_out, static_cast<uint32_t>(eSF2Tags::sdta), nbwritten );
                WriteSdataList();
                //datasz += WriteListChunk( static_cast<uint32_t>(eSF2Tags::sdta), std::bind(&SounFontRIFFWriter::WriteSdataList, this ) );
            }
            datasz += nbwritten;
            nbwritten = 0;

            //Build and write the HYDRA
            {
                RAII_WriteListChunk wl( m_out, static_cast<uint32_t>(eSF2Tags::pdta), nbwritten );
                WritePdataList();
                //datasz += WriteListChunk( static_cast<uint32_t>(eSF2Tags::pdta), std::bind(&SounFontRIFFWriter::WritePdataList, this ) );
            }
            datasz += nbwritten;
            nbwritten = 0;

            //Seek back to start
            //const std::ofstream::streampos postwrite = m_out.tellp();
            m_out.seekp(prewriteoffset);

            //Write header
            riff::ChunkHeader riffhdr;
            riffhdr.chunk_id = static_cast<uint32_t>(riff::eChunkIDs::RIFF);
            riffhdr.length   = static_cast<uint32_t>(datasz); 
            riffhdr.Write( ostreambuf_iterator<char>(m_out) );

            //Seek back to end before giving back the stream
            m_out.seekp(0, std::ios::end);

            //And done !
            return std::move(m_out);
        }

    private:

        inline std::ofstream::streampos GetCurTotalNbByWritten()
        {
            return (m_out.tellp() - m_prewrite);
        }

    // ----------------------------------------------------------------
    //  Chunk Header Writing
    // ----------------------------------------------------------------
#if 0
        /*
            WriteListChunk
                This method writes the header surrounding a list chunk to the stream.
                - fmttag : The fmt tag of the list chunk
                - method : The method to execute to fill up the data of the chunk

                #FIXME: I'm not sure why I couldn't find better than this to wrap each list chunks and re-use the list making code..
                        But writing directly into a stream with this format is a pain in the butt..
        */
        ofstream::streampos WriteListChunk( uint32_t fmttag, listmethodfun_t method )
        {
            //Save pre-write pos
            const std::ofstream::streampos prewrite = m_out.tellp(); //Must be Tellp because we're seeking with this value
            std::ostreambuf_iterator<char> itout(m_out);

            //Skip header size
            itout = std::fill_n( itout, riff::ChunkHeader::SIZE, 0 );
#ifdef _DEBUG
            m_out.flush();
#endif

            //Write format tag
            itout = utils::WriteIntToBytes( static_cast<uint32_t>(fmttag), itout, false );
#ifdef _DEBUG
            m_out.flush();
#endif

            //Write the sub-chunks
            ofstream::streampos datalen = sizeof(fmttag) + method(); //Count the format tag
#ifdef _DEBUG
            m_out.flush();
#endif

            //Save Post-write pos
            const std::ofstream::streampos postwrite = m_out.tellp();

            //Seek back to start
            m_out.seekp(prewrite);

            //Write header
            riff::ChunkHeader listhdr;
            listhdr.chunk_id = static_cast<uint32_t>(riff::eChunkIDs::LIST);
            listhdr.length   = static_cast<uint32_t>(postwrite - prewrite) - riff::ChunkHeader::SIZE; //Don't count the header itself

            itout = listhdr.Write(itout);

            //Seek back to end
            m_out.seekp(postwrite);

            //Return our total
            return (postwrite - prewrite);
        }

        /*
            WriteChunk
                This method writes the header surrounding a chunk to the stream.
                - tag    : The header tag of the chunk
                - method : The method to execute to fill up the data of the chunk

                #FIXME: I'm not sure why I couldn't find better than this to wrap each list chunks and re-use the list making code..
                        But writing directly into a stream with this format is a pain in the butt..
        */
        ofstream::streampos WriteChunk( eSF2Tags tag, listmethodfun_t method )
        {
            //Save pre-write pos
            const std::ofstream::streampos prewrite = m_out.tellp(); //Must be Tellp because we're seeking with this value
            std::ostreambuf_iterator<char> itout(m_out);

            //Skip header size
            std::fill_n( std::ostreambuf_iterator<char>(m_out), riff::ChunkHeader::SIZE, 0 );

            //Write the sub-chunks
            ofstream::streampos datalen = method();

            //Save Post-write pos
            const std::ofstream::streampos postwrite = m_out.tellp();

            //Seek back to start
            m_out.seekp(prewrite);

            //Write header
            riff::ChunkHeader listhdr;
            listhdr.chunk_id = static_cast<uint32_t>(tag);
            listhdr.length   = static_cast<uint32_t>(postwrite - prewrite) - riff::ChunkHeader::SIZE; //Don't count the header itself

            itout = listhdr.Write(itout);

            //Seek back to end
            m_out.seekp(postwrite);

            //Return our total
            return (postwrite - prewrite);
        }
#endif

    //----------------------------------------------------------------
    //  INFO-list
    //----------------------------------------------------------------
        //Write the INFO-list chunk. Returns the nb of bytes written
        ofstream::streampos WriteInfoList()
        {
            //Save pre-write pos
            const std::ofstream::streampos prewrite = m_out.tellp();

            //Write the sub-chunks
            WriteifilChunk();
            WriteisngChunk();
            WriteINAMChunk();
            WriteISFTChunk();

            //Return our total
            return (m_out.tellp() - prewrite);
        }

        /*
            WriteifilChunk
        */
        void WriteifilChunk()
        {
            ostreambuf_iterator<char> itout(m_out);
            riff::ChunkHeader ifilchnk;
            ifilchnk.chunk_id = static_cast<uint32_t>( eSF2Tags::ifil );
            ifilchnk.length   = 4; //Always 4 bytes
            itout = ifilchnk.Write( itout );
            itout = SF_VersChnkData.Write( itout );
        }

        void WriteisngChunk()
        {
            auto              itout = ostreambuf_iterator<char>(m_out);
            riff::ChunkHeader isngchnk;
            isngchnk.chunk_id = static_cast<uint32_t>( eSF2Tags::isng );

            if( m_out.tellp() % 2 != 0 )
                isngchnk.length = SF_DefIsng.size() + 2; //For the extra padding 0 byte
            else
                isngchnk.length = SF_DefIsng.size() + 1;

            //Write the chunk header
            isngchnk.Write( itout );

            //Write the string
            std::copy( SF_DefIsng.begin(), SF_DefIsng.end(), itout );

            //Terminate it with a null character
            m_out.put(0);
            
            //Add extra Zero if ends on non-even byte count
            if( m_out.tellp() % 2 != 0 )
                m_out.put(0);
        }

        void WriteINAMChunk()
        {
            auto              itout = ostreambuf_iterator<char>(m_out);
            riff::ChunkHeader INAMchnk;
            INAMchnk.chunk_id = static_cast<uint32_t>( eSF2Tags::INAM );

            if( m_out.tellp() % 2 != 0 )
                INAMchnk.length = m_sf.GetName().size() + 2; //For the extra padding 0 byte
            else
                INAMchnk.length = m_sf.GetName().size() + 1;

            //Write the chunk header
            INAMchnk.Write( itout );

            //Write the string
            std::copy( m_sf.GetName().begin(), m_sf.GetName().end(), itout );

            //Terminate it with a null character
            m_out.put(0);
            
            //Add extra Zero if ends on non-even byte count
            if( m_out.tellp() % 2 != 0 )
                m_out.put(0);
        }

        void WriteISFTChunk()
        {
            auto              itout = ostreambuf_iterator<char>(m_out);
            riff::Chunk       ISFTchnk( static_cast<uint32_t>( eSF2Tags::ISFT ) );
            size_t            sftnamelen = std::min( SF_DefSft.size(), (SfMaxLongCString-1) ); //Limit to the max len of the string - 1 for the terminating 0!
            
            ISFTchnk.data_.reserve(sftnamelen + 2); //Reserve 2 extra bytes for the terminator(s)

            //copy as much of the string as we can
            copy_n( SF_DefSft.begin(), sftnamelen, back_inserter(ISFTchnk.data_) );
            ISFTchnk.data_.push_back('\0'); //Add terminating 0

            if( ( (ISFTchnk.data_.size()) % 2 ) != 0 ) //Add extra terminating zero if string len is non even
                ISFTchnk.data_.push_back('\0');

            //Write the chunk 
            ISFTchnk.Write( itout );
        }

    //----------------------------------------------------------------
    //  SData-list
    //----------------------------------------------------------------
        //Write the sdta-list chunk. Returns the nb of bytes written
        ofstream::streampos WriteSdataList()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();

            {
                RAII_WriteChunk wc( m_out, eSF2Tags::smpl );
                WriteSmplChunk();
            //WriteChunk( eSF2Tags::smpl, listmethodfun_t( std::bind(&SounFontRIFFWriter::WriteSmplChunk,  this ) ) );
            }

            return (GetCurTotalNbByWritten() - prewrite);
        }

        ofstream::streampos WriteSmplChunk()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();

            m_smplswritepos.resize(0);
            m_smplswritepos.reserve(m_sf.GetNbSamples());
            m_smplnewlppoints.resize(0);
            m_smplnewlppoints.reserve(m_sf.GetNbSamples());

            //Write the samples
            for( const auto & smpl : m_sf.GetSamples() )
            {
                //Write down position
                std::streampos smplstart  = GetCurTotalNbByWritten();
                auto           loadedsmpl = std::move( smpl.Data() );   //Never trust MSVC with move constructors and static type abstraction
                auto           loopbounds = smpl.GetLoopBounds();


                if( loopbounds.second > loadedsmpl.size() )
                {
                    cerr << "SoundFontRIFFWriter::WriteSmplChunk(): Sample end out of bound ! Attempting fix..\n";
//#ifdef _DEBUG
//                    assert(false);
//#endif
                    //#FIXME: This is evil! And really stupid. But I can't be bothered to do something less half-assed tonight!
                    //const_cast<Sample&>(smpl).SetLoopBounds( loopbounds.first, loadedsmpl.size() );
                    loopbounds.second = loadedsmpl.size();
                }

#if 0
                if( labs(loopbounds.second - loopbounds.first) != 0 )
                    MakeSampleLoopLegal( loadedsmpl, loopbounds.first, loopbounds.second );
#endif

                m_smplnewlppoints.push_back( move( std::make_pair( static_cast<size_t>(loopbounds.first), 
                                                                   static_cast<size_t>(loopbounds.second) ) ) );

                ostreambuf_iterator<char> itout(m_out);

                //Write sample data
                for( const pcm16s_t & point : loadedsmpl )
                    itout = utils::WriteIntToBytes( point, itout );

                //Save the begining and end position within the sdata chunk before zeros
                m_smplswritepos.push_back( make_pair( (smplstart - prewrite), (GetCurTotalNbByWritten() - prewrite) ) );

                //Write the stupid zeros..
                //const uint32_t finalsmpllen = loadedsmpl.size();

                //if( finalsmpllen < SfMinSampleZeroPad )
                    itout = std::fill_n( itout, SfMinSampleZeroPad, 0 ); 
                //else if( finalsmpllen % 2 == 0 )
                //    itout = std::fill_n( itout, finalsmpllen, 0 );
                //else
                //    itout = std::fill_n( itout, finalsmpllen + 1, 0 ); // Add one to make the byte count even
            }

            return (GetCurTotalNbByWritten() - prewrite);
        }

    //----------------------------------------------------------------
    //  HYDRA (PData-List)
    //----------------------------------------------------------------

        //Write the pdta-list chunk. Returns the nb of bytes written
        ofstream::streampos WritePdataList()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();

            //Write the sub-chunks

            //Write Presets
            WriteHYDRAPresets();

            //Write Instruments
            WriteHYDRAInstruments();

            //Write Sample Headers 
            {
                RAII_WriteChunk wc(m_out, eSF2Tags::shdr );
                WriteHYDRASampleHeaders();
                //WriteChunk( eSF2Tags::shdr, std::bind(&SounFontRIFFWriter::WriteHYDRASampleHeaders,  this ) );
            }
            return (GetCurTotalNbByWritten() - prewrite);
        }

        ofstream::streampos WriteHYDRAPresets()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();

            //Write phdr chunk
            {
                RAII_WriteChunk wc(m_out, eSF2Tags::phdr );
                WritePHDRChunk();
            //WriteChunk( eSF2Tags::phdr, listmethodfun_t( std::bind(&SounFontRIFFWriter::WritePHDRChunk,  this ) ) );
            }

            //Write pbag
            {
                RAII_WriteChunk wc(m_out, eSF2Tags::pbag );
                WritePBagChunk();
            //WriteChunk( eSF2Tags::pbag, listmethodfun_t(  std::bind(&SounFontRIFFWriter::WritePBagChunk,  this ) ) );
            }

            //Write pmod
            {
                RAII_WriteChunk wc(m_out, eSF2Tags::pmod );
                WritePModChunk();
            //WriteChunk( eSF2Tags::pmod, listmethodfun_t(  std::bind(&SounFontRIFFWriter::WritePModChunk,  this ) ) );
            }

            //Write pgen
            {
                RAII_WriteChunk wc(m_out, eSF2Tags::pgen );
                WritePGenChunk();
            //WriteChunk( eSF2Tags::pgen, listmethodfun_t(  std::bind(&SounFontRIFFWriter::WritePGenChunk,  this ) ) );
            }
            return (GetCurTotalNbByWritten() - prewrite);
        }

        ofstream::streampos WriteHYDRAInstruments()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();

            //Write inst chunk
            { 
                RAII_WriteChunk wc( m_out, eSF2Tags::inst ); 
                WriteInstChunk();
            //WriteChunk( eSF2Tags::inst, listmethodfun_t(  std::bind(&SounFontRIFFWriter::WriteInstChunk,  this ) ) );
            }

            //Write ibag chunk
            {
                RAII_WriteChunk wc( m_out, eSF2Tags::ibag ); 
                WriteIBagChunk();
            //WriteChunk( eSF2Tags::ibag, listmethodfun_t(  std::bind(&SounFontRIFFWriter::WriteIBagChunk,  this ) ) );
            }

            //Write imod chunk
            {
                RAII_WriteChunk wc( m_out, eSF2Tags::imod ); 
                WriteIModChunk();
            //WriteChunk( eSF2Tags::imod, listmethodfun_t(  std::bind(&SounFontRIFFWriter::WriteIModChunk,  this ) ) );
            }

            //Write igen chunk
            {
                RAII_WriteChunk wc( m_out, eSF2Tags::igen ); 
                WriteIGenChunk();
            //WriteChunk( eSF2Tags::igen, listmethodfun_t(  std::bind(&SounFontRIFFWriter::WriteIGenChunk,  this ) ) );
            }

            return (GetCurTotalNbByWritten() - prewrite);
        }

        ofstream::streampos WriteHYDRASampleHeaders()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();
            ostreambuf_iterator<char>      itout(m_out);

            //Shift the loop begining and end by 4 !

            for( size_t i = 0; i < m_sf.GetNbSamples(); ++i )
            {
                const auto & smpl        = m_sf.GetSamples()[i];
                const size_t charstocopy = std::min( smpl.GetName().size(), (ShortNameLen-1) ); 
                const size_t charstozero = ShortNameLen - charstocopy;                          //Nb of zeros to append
                auto         loop        = smpl.GetLoopBounds();
                
#if SF2_ADD_EXTRA_LOOP_BYTES
                //Calculate those first, to avoid casting all the time.. 
                const uint32_t smplbeg = static_cast<uint32_t>(m_smplswritepos[i].first)  / sizeof(pcm16s_t); //Convert from samples to bytes
                const uint32_t smplend = static_cast<uint32_t>(m_smplswritepos[i].second) / sizeof(pcm16s_t); //Convert from samples to bytes
                uint32_t       loopbeg = 0;
                uint32_t       loopend = 0;

                //#FIXME: I really don't like having a separate computation here for loop points..
                //        It should all happens when we make samples loop legal, but we don't want to modify the sample container's data..
                if( labs(loop.second - loop.first) != 0 ) //abs because we're not sure yet what the values means in the original SWDL
                {
                    uint32_t extrasmpls = 0; //Any extra samples that were added to compensate for a possible non-legal loop

                    if( loop.first < SfSampleLoopMinEdgeDist ) //Check if we actually moved the loopbeg
                    {
                        extrasmpls += std::max( 0, static_cast<int32_t>(SfSampleLoopMinEdgeDist - loop.first) );
                        loopbeg     = smplbeg + (static_cast<uint32_t>(loop.first) + extrasmpls ); //Compensate for extra required data points for being made loop legal
                    }
                    else
                        loopbeg = smplbeg + static_cast<uint32_t>(loop.first);
                    
                    //if( (smplend - loop.second) < SfSampleLoopMinEdgeDist ) //Check if we actually moved the loopend
                    //{
                    //    extrasmpls += std::min( 0, static_cast<int32_t>(SfSampleLoopMinEdgeDist - (smplend - loop.second)) );
                        loopend = smplbeg + (static_cast<uint32_t>(loop.second) + extrasmpls ); //Compensate for extra required data points for being made loop legal
                    //}
                }

#elif SF2_ADD_EXTRA_LOOP_BYTES2
                //Calculate those first, to avoid casting all the time.. 
                const uint32_t smplbeg = static_cast<uint32_t>(m_smplswritepos[i].first)  / sizeof(pcm16s_t); //In bytes
                const uint32_t smplend = static_cast<uint32_t>(m_smplswritepos[i].second) / sizeof(pcm16s_t);
                uint32_t       loopbeg = 0;
                uint32_t       loopend = 0;

                if( loop.second != 0 )
                {
                    loopbeg = smplbeg + (static_cast<uint32_t>(m_smplnewlppoints[i].first)   );
                    loopend = smplbeg + (static_cast<uint32_t>(m_smplnewlppoints[i].second)  );
                }

#else
                //Calculate those first, to avoid casting all the time.. 
                const uint32_t smplbeg = static_cast<uint32_t>(m_smplswritepos[i].first)  / sizeof(pcm16s_t); //In bytes
                const uint32_t smplend = static_cast<uint32_t>(m_smplswritepos[i].second) / sizeof(pcm16s_t);
                uint32_t       loopbeg = 0;
                uint32_t       loopend = 0;

                if( loop.second != 0 )
                {
                    loopbeg = smplbeg + (static_cast<uint32_t>(loop.first)   );
                    loopend = smplbeg + (static_cast<uint32_t>(loop.second)  );
                }
#endif

                //Put sample name + following zeros
                itout = std::copy_n( smpl.GetName().begin(), charstocopy, itout );
                itout = std::fill_n( itout, charstozero, 0 );
                //Put sample start
                itout = utils::WriteIntToBytes( smplbeg, itout );
                //Put sample end
                itout = utils::WriteIntToBytes( smplend, itout );
                //Put the sample loop beginning
                itout = utils::WriteIntToBytes( loopbeg, itout );
                //Put the sample loop end
                itout = utils::WriteIntToBytes( loopend, itout );
                //Put Sample Rate
                itout = utils::WriteIntToBytes( smpl.GetSampleRate(), itout );
                //Put Original Pitch
                itout = utils::WriteIntToBytes( smpl.GetOriginalKey(), itout );
                //Put Pitch Correction
                itout = utils::WriteIntToBytes( smpl.GetPitchCorrection(), itout );
                //Put Sample Link
                itout = utils::WriteIntToBytes( smpl.GetLinkedSample(), itout );
                //Put Sample Type
                itout = utils::WriteIntToBytes( static_cast<uint16_t>(smpl.GetSampleType()), itout );
            }

            //End the list with a zeroed out entry
            static const std::array<char,4> EOSMarker{{'E','O','S',0}};
            itout = std::copy  ( EOSMarker.begin(), EOSMarker.end(), itout );
            itout = std::fill_n( itout, (SfEntrySHDR_Len - EOSMarker.size()), 0 );

            return (GetCurTotalNbByWritten() - prewrite);
        }

    //-----------------------------
    //  Presets
    //-----------------------------
        ofstream::streampos WritePHDRChunk()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();
            ostreambuf_iterator<char>      itout(m_out);
            uint16_t                       pbagndx = 0; //Keep track of the nb of pbags so far

            for( size_t i = 0; i < m_sf.GetNbPresets(); ++i )
            {
                const auto & preset = m_sf.GetPresets()[i];
                const size_t charstocopy = std::min( preset.GetName().size(), (ShortNameLen-1) ); 
                const size_t charstozero = ShortNameLen - charstocopy;                          //Nb of zeros to append

                //Write Name
                itout = std::copy_n( preset.GetName().begin(), charstocopy, itout );
                itout = std::fill_n( itout, charstozero, 0 );
                //Write Preset #
                itout = utils::WriteIntToBytes( preset.GetPresetNo(), itout );
                //Write Bank #
                itout = utils::WriteIntToBytes( preset.GetBankNo(), itout );

                //Write Preset Bag Index
                itout = utils::WriteIntToBytes( pbagndx, itout );

                //Increment pbag!
                pbagndx += preset.GetNbZone();

                //Write Library 
                itout = utils::WriteIntToBytes( preset.GetLibrary(), itout );
                //Write Genre
                itout = utils::WriteIntToBytes( preset.GetGenre(), itout );
                //Write Morphology
                itout = utils::WriteIntToBytes( preset.GetMorpho(), itout );
            }

            //End the list with a zeroed out entry
            static const std::array<char,4> EOPMarker{{'E','O','P',0}};

            //Set some compile time constants here, so less chances for me to derp again! :D
            static const size_t             LenFirstPart = sizeof( result_of<decltype(&Preset::GetPresetNo)(Preset)>::type) +
                                                           sizeof( result_of<decltype(&Preset::GetBankNo)  (Preset)>::type);
            static const size_t             LenLastPart  = sizeof( result_of<decltype(&Preset::GetLibrary) (Preset)>::type) +
                                                           sizeof( result_of<decltype(&Preset::GetGenre)   (Preset)>::type) +
                                                           sizeof( result_of<decltype(&Preset::GetMorpho)  (Preset)>::type);

            itout = std::copy  ( EOPMarker.begin(), EOPMarker.end(),itout );
            itout = std::fill_n( itout, (ShortNameLen - EOPMarker.size()),  0  ); //Put the zeros after the string

            //Write the preset# and bank#
            itout = std::fill_n( itout, LenFirstPart,  0 );

            //** Write the very last bag index to the dummy terminal ibag entry **
            itout = utils::WriteIntToBytes( pbagndx, itout );
            
            //Then write the library genre and morphology
            itout = std::fill_n( itout, LenLastPart, 0 );

            return (GetCurTotalNbByWritten() - prewrite);
        }

        ofstream::streampos WritePBagChunk()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();
            ostreambuf_iterator<char>      itout(m_out);
            uint16_t                       pgenndx = 0;
            uint16_t                       pmodndx = 0;

            //Write the index of each preset's modulator and generators list in the pgen and pmod chunks
            for( size_t cntpres = 0; cntpres < m_sf.GetNbPresets(); ++cntpres )
                WriteBagEntries( m_sf.GetPreset(cntpres), pgenndx, pmodndx, itout );

            //End the list with a pbag entry that will be used to determine the size of the PGen and PMod chunks
            WriteABag( itout, pgenndx, pmodndx );

            return (GetCurTotalNbByWritten() - prewrite);
        }

        ofstream::streampos WritePModChunk()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();
            ostreambuf_iterator<char>      itout(m_out);

            for( size_t cntpres = 0; cntpres < m_sf.GetNbPresets(); ++cntpres )
                WriteModulatorEntries( m_sf.GetPreset(cntpres), itout );

            //End the list with a zeroed out entry
            std::fill_n( itout, SfEntryPMod_Len, 0 );

            return (GetCurTotalNbByWritten() - prewrite);
        }

        ofstream::streampos WritePGenChunk()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();
            ostreambuf_iterator<char>      itout(m_out);

            for( size_t cntpres = 0; cntpres < m_sf.GetNbPresets(); ++cntpres )
                WriteGeneratorEntries( m_sf.GetPreset(cntpres), itout );

            //End the list with a zeroed out entry
            std::fill_n( itout, SfEntryPGen_Len, 0 );

            return (GetCurTotalNbByWritten() - prewrite);
        }
    
    //-----------------------------
    //  Instruments
    //-----------------------------
        ofstream::streampos WriteInstChunk()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();
            ostreambuf_iterator<char>      itout(m_out);
            uint16_t                       instbagndx = 0; //Keep track of the total amount of ibags this far

            //Make an entry for each instrument, and put an index to the start of each instrument's bag list
            for( size_t cntinst = 0; cntinst < m_sf.GetNbInstruments(); ++cntinst )
            {
                const auto & curinst = m_sf.GetInstument(cntinst);

                const size_t charstocopy = std::min( curinst.GetName().size(), (ShortNameLen-1) ); 
                const size_t charstozero = ShortNameLen - charstocopy;                          //Nb of zeros to append

                //Write Name
                itout = std::copy_n( curinst.GetName().begin(), charstocopy, itout );
                itout = std::fill_n( itout, charstozero, 0 );

                //Write bag index
                itout = utils::WriteIntToBytes( instbagndx, itout );

                //Increment bag index
                instbagndx += curinst.GetNbZone();
            }

            //End the list with a zeroed out entry
            static const std::array<char,4> EOIMarker{{'E','O','I', 0}};

            itout = std::copy  ( EOIMarker.begin(), EOIMarker.end(),itout );
            itout = std::fill_n( itout, (ShortNameLen - EOIMarker.size()),  0  ); //Put the zeros after the string

            //And write the very last bag index to the dummy terminal ibag entry
            itout = utils::WriteIntToBytes( instbagndx, itout );

            return (GetCurTotalNbByWritten() - prewrite);
        }

        ofstream::streampos WriteIBagChunk()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();
            ostreambuf_iterator<char>      itout(m_out);
            uint16_t                       igenndx = 0;
            uint16_t                       imodndx = 0;

            //Write the index of each bagzone's modulator and generators
            for( size_t cntinst = 0; cntinst < m_sf.GetNbInstruments(); ++cntinst )
                WriteBagEntries( m_sf.GetInstument(cntinst), igenndx, imodndx, itout );

            //Write the last entry that will be used to find out the size of the IMod and IGen chunk
            WriteABag( itout, igenndx, imodndx );

            return (GetCurTotalNbByWritten() - prewrite);
        }

        ofstream::streampos WriteIModChunk()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();
            ostreambuf_iterator<char>      itout(m_out);


            //Write each Modulators for each instruments one after the other
            for( size_t cntinst = 0; cntinst < m_sf.GetNbInstruments(); ++cntinst )
                WriteModulatorEntries( m_sf.GetInstument(cntinst), itout );

            //End the list with a zeroed out entry
            std::fill_n( itout, SfEntryIMod_Len, 0 );

            return (GetCurTotalNbByWritten() - prewrite);
        }

        ofstream::streampos WriteIGenChunk()
        {
            const std::ofstream::streampos prewrite = GetCurTotalNbByWritten();
            ostreambuf_iterator<char>      itout(m_out);

            for( size_t cntinst = 0; cntinst < m_sf.GetNbInstruments(); ++cntinst )
                WriteGeneratorEntries( m_sf.GetInstument(cntinst), itout );

            //End the list with a zeroed out entry
            std::fill_n( itout, SfEntryIGen_Len, 0 );

            return (GetCurTotalNbByWritten() - prewrite);
        }



        //
        //  Utilities
        //
        template<class _ZoneOwnerCollection>
            void WriteGeneratorEntries( _ZoneOwnerCollection content, ostreambuf_iterator<char> & itout )
        {
            //Get each zone
            for( size_t cntzone = 0; cntzone < content.GetNbZone(); ++cntzone )
            {
                const auto & curzone = content.GetZone(cntzone);

                for( const auto & gene : curzone.GetGenerators() )
                {
                    itout = utils::WriteIntToBytes( static_cast<uint16_t>(gene.first), itout );
                    itout = utils::WriteIntToBytes( gene.second,                       itout );
                }
            }
        }

        template<class _ZoneOwnerCollection>
            void WriteModulatorEntries( _ZoneOwnerCollection content, ostreambuf_iterator<char> & itout )
        {
            //Get each zone
            for( size_t cntzone = 0; cntzone < content.GetNbZone(); ++cntzone )
            {
                const auto & curzone = content.GetZone(cntzone);

                for( const auto & modu : curzone.GetModulators() )
                {
                    itout = utils::WriteIntToBytes( static_cast<uint16_t>(modu.ModSrcOper),    itout );
                    itout = utils::WriteIntToBytes( static_cast<uint16_t>(modu.ModDestOper),   itout );
                    itout = utils::WriteIntToBytes( modu.modAmount,                            itout );
                    itout = utils::WriteIntToBytes( static_cast<uint16_t>(modu.ModAmtSrcOper), itout );
                    itout = utils::WriteIntToBytes( static_cast<uint16_t>(modu.ModTransOper),  itout );
                }
            }
        }

        template<class _ZoneOwnerCollection>
            void WriteBagEntries(_ZoneOwnerCollection content, uint16_t & gencnt, uint16_t & modcnt, ostreambuf_iterator<char> & itout )
        {
            //Write the index of each bagzone's modulator and generators
            for( size_t cntzone = 0; cntzone < content.GetNbZone(); cntzone++ )
            {
                const auto & curzone = content.GetZone(cntzone);
                WriteABag( itout, gencnt, modcnt );
                gencnt += static_cast<uint16_t>( curzone.GetNbGenerators() ); //The ammount is checked before writing the sf2, so we can do this cast.
                modcnt += static_cast<uint16_t>( curzone.GetNbModulators() );
            }

        }

        inline void WriteABag( ostreambuf_iterator<char> & itwhere, uint16_t genndx, uint16_t modndx )
        {
            itwhere = utils::WriteIntToBytes( genndx, itwhere );
            itwhere = utils::WriteIntToBytes( modndx, itwhere );
        }

        //
        SounFontRIFFWriter( const SounFontRIFFWriter & )           = delete;
        SounFontRIFFWriter& operator=(const SounFontRIFFWriter & ) = delete;        

    private:
        const SoundFont         & m_sf;
        std::ofstream             m_out;
        std::ofstream::streampos  m_prewrite; //The position in the stream before we began writing the entire Soundfont

        //Keep track of where the samples where written, from the beginning of the sounfont structure
        std::vector<std::pair<std::ofstream::streampos,std::ofstream::streampos>> m_smplswritepos;
        //Keep track of the modified loop points
        std::vector<std::pair<size_t,size_t>>                                     m_smplnewlppoints;
    };

//=========================================================================================
//  SounFont
//=========================================================================================

    SoundFont::SoundFont()
    {}
    
    SoundFont::SoundFont( const std::string & sfname )
        :m_sfname(sfname)
    {}

    size_t SoundFont::AddSample( Sample && smpl )
    {
        m_samples.push_back( std::move(smpl) );
        return (m_samples.size()-1);
    }

    size_t SoundFont::AddPreset( Preset && preset )
    {
        m_presets.push_back( std::move(preset) );
        return (m_presets.size()-1);
    }

    std::vector<Preset> & SoundFont::GetPresets()
    {
        return m_presets;
    }

    const std::vector<Preset> & SoundFont::GetPresets()const
    {
        return m_presets;
    }

    Preset & SoundFont::GetPreset( size_t index )
    {
        return m_presets[index];
    }

    const Preset & SoundFont::GetPreset( size_t index )const
    {
        return m_presets[index];
    }

    std::vector<Sample> & SoundFont::GetSamples()
    {
        return m_samples;
    }

    const std::vector<Sample> & SoundFont::GetSamples()const
    {
        return m_samples;
    }

    Sample & SoundFont::GetSample( size_t index )
    {
        return m_samples[index];
    }

    size_t SoundFont::AddInstrument( Instrument && inst )
    {
        size_t index = m_instruments.size();
        m_instruments.push_back( std::move(inst) );
        return index;
    }

    size_t SoundFont::Write( const std::string & sf2path )
    {
        //Then output stuff
        ofstream output( sf2path, std::ios::out | std::ios::binary );
        SounFontRIFFWriter sfw(*this);
        output = std::move( sfw.Write( std::move(output) ) );
        return static_cast<size_t>( output.tellp() ); //Stop warning #4244
    }

    size_t SoundFont::Read( const std::string & sf2path )
    {
        assert(false);
        return 0;
    }

//=========================================================================================
//  Sample
//=========================================================================================

    Sample::Sample( loadfun_t && funcload, smplcount_t samplelen)
        :m_samplety(eSmplTy::monoSample), m_linkedsmpl(0), m_loadfun(funcload), m_loopbeg(0), m_loopend(0), m_origkey(60),//MIDI middle C
         m_pitchcorr(0), m_smplrate(44100), m_smpllen(samplelen)
    {}

    /*
        Obtain the data from the sample.
    */
    Sample::operator std::vector<pcm16s_t>()const
    {
        return std::move(Data());
    }

    /*
        Obtain the data from the sample.
        Wrapper over delayed read operations
    */
    std::vector<pcm16s_t> Sample::Data()const
    {
        if( m_pcmdata.empty() && !m_loadfun )
        {
            clog << "<!>- Warning : Sample::Data() : Sample \"" <<m_name <<"\" contains no PCM data!!\n";
            return m_pcmdata;
        }
        else if( m_pcmdata.empty() )
            return std::move( m_loadfun() );
        else
            return std::move( m_pcmdata );
    }

    void Sample::SetLoopBounds( smplcount_t beg, smplcount_t end )
    {
        m_loopbeg = beg;
        m_loopend = end;
    }


//=========================================================================================
//  Preset
//=========================================================================================

    //---------------
    //  Contructors
    //---------------
    Preset::Preset()
        :m_presetNo(0), m_bankNo(0), m_genre(0), m_library(0), m_morpho(0)
    {}
    
    Preset::Preset( const std::string & name, uint16_t presetno, uint16_t bankno, uint32_t lib, uint32_t genre, uint32_t morpho )
        :m_name(name), m_presetNo(presetno), m_bankNo(bankno), m_library(lib), m_genre(genre), m_morpho(morpho)
    {}

//=========================================================================================
//  Instrument
//=========================================================================================

    Instrument::Instrument()
    {}
    
    Instrument::Instrument( const std::string & name )
        :m_name(name)
    {}

//=========================================================================================
//  BaseGeneratorUser
//=========================================================================================

    BaseGeneratorUser::genpriority_t BaseGeneratorUser::GetGenPriority( eSFGen gen )
    {
        if( gen == eSFGen::keyRange )
            return HighPriority;
        else if( gen == eSFGen::velRange )
            return HighPriority-1;
        else if( gen == eSFGen::sampleID )
            return LowPriority + 1; // +1 because an instrument generator makes everything that comes after be ignored
        else if( gen == eSFGen::instrument)
            return LowPriority;
        else
            return ( DefaultPriority - static_cast<int8_t>(gen) ); //Do this to get a unique priority ID for each value.
    }


    /*
        AddGenerator
            Add a generator value to this instrument.

            A "generator" is mainly an attribute so to speak.
            A sample Id is a generator for instance. 
                
            There can only be a single generator of a given type per instrument.
            Any generator with the same generator type as an existing generator
            will overwrite the later.

            This will also sort the generators in the standard required order !
                
            -> KeyRange Generators always first.
            -> Velocity Range generator can only be preceeded by a Keyrange generator.
            -> SampleID Generators always last.
            -> InstrumentID Generators always last.
    */
    void BaseGeneratorUser::AddGenerator( eSFGen gen, genparam_t value )
    {
        auto empres = m_gens.emplace( make_pair(gen,value) );

        //If insertion failed, overwrite the existing value
        if( !empres.second )
            empres.first->second = value;
    }

    /*
        GetGenerator
            Return a pointer to the specified generator's value, 
            or null if it doesn't exist.
    */
    genparam_t * BaseGeneratorUser::GetGenerator( eSFGen gen )
    {
        auto itfound = m_gens.find( gen );

        if( itfound != m_gens.end() )
            return &(itfound->second);
        else
            return nullptr;
    }

    const genparam_t * BaseGeneratorUser::GetGenerator( eSFGen gen )const
    {
        auto itfound = m_gens.find( gen );

        if( itfound != m_gens.end() )
            return &(itfound->second);
        else
            return nullptr;
    }

    /*
        GetGenerator
            Return a reference to the specified generator's value.
    */
    genparam_t & BaseGeneratorUser::GetGenerator( size_t index )
    {
        if( index < m_gens.size() )
        {
            auto pos = m_gens.begin();
            std::advance( pos, index );
            return pos->second;
        }
        else
            throw std::out_of_range("BaseGeneratorUser::GetGenerator() : Index out of bound !");
    }

    const genparam_t & BaseGeneratorUser::GetGenerator( size_t index )const
    {
        if( index < m_gens.size() )
        {
            auto pos = m_gens.begin();
            std::advance( pos, index );
            return pos->second;
        }
        else
            throw std::out_of_range("BaseGeneratorUser::GetGenerator() : Index out of bound !");
    }

    /*
        GetNbGenerators
    */
    size_t BaseGeneratorUser::GetNbGenerators()const
    {
        return m_gens.size();
    }

    //---------------------
    //  Common Generators
    //---------------------
    /*
        Get or Set the sample used by this instrument.
            sampleid : sample index into the SHDR sub-chunk
    */
    void BaseGeneratorUser::SetSampleId( size_t sampleid )
    {
        AddGenerator( eSFGen::sampleID, static_cast<uint16_t>(sampleid) );
    }

    std::pair<bool,size_t> BaseGeneratorUser::GetSampleId()const
    {
        auto res = GetGenerator( eSFGen::sampleID );

        if( res != nullptr )
            return make_pair(true, *res );
        else
            return make_pair(false, 0);
    }

    /*
        Get or Set the instrument ID used by this preset
            instrumentid : instrument index into the INST sub-chunk

            Returns a pair made of a boolean, and the value of the generator.
            If the boolean is false, there is currently no such generator, and 
            the value returned is thus invalid.
    */
    void BaseGeneratorUser::SetInstrumentId( size_t instrumentid )
    {
        AddGenerator( eSFGen::instrument, static_cast<uint16_t>(instrumentid) );
    }

    std::pair<bool,size_t> BaseGeneratorUser::GetInstrumentId()const
    {
        auto res = GetGenerator( eSFGen::instrument );

        if( res != nullptr )
            return make_pair(true, *res);
        else
            return make_pair(false, 0);
    }

    /*
        Set or Get the MIDI key range covered by this instrument.
            (def: 0-127, min: 0, max: 127)
    */
    void BaseGeneratorUser::SetKeyRange( int8_t lokey, int8_t hikey )
    {
        //genparam_t val = (lokey << 8) | hikey;
        //val.twosby.by1 = lokey; //MSB is lowest key
        //val.twosby.by2 = hikey;
        if( lokey != (SF_GenLimitsKeyRange.def_ & 0xFF) || hikey != ((SF_GenLimitsKeyRange.def_ >> 8) & 0xFF )  )
            AddGenerator( eSFGen::keyRange, static_cast<uint16_t>( (lokey & 0x7F) | ( (hikey & 0x7F) << 8) ) );
    }
    
    MidiKeyRange BaseGeneratorUser::GetKeyRange()const
    {
        MidiKeyRange kr;
        auto         res = GetGenerator( eSFGen::keyRange );

        if( res != nullptr )
        {
            kr.lokey = (*res) & 0xFF /*->twosby.by1*/;
            kr.hikey = (*res) >>   8 /*->twosby.by2*/;
        }

        return std::move(kr);
    }

    /*
        Set or Get the velocity range covered by this instrument.
            (def: 0-127, min: 0, max: 127)
    */
    void BaseGeneratorUser::SetVelRange( int8_t lokvel, int8_t hivel )
    {
        //genparam_t val;
        //val.twosby.by1 = lokvel; //MSB is lowest key
        //val.twosby.by2 = hivel;
        if( lokvel != (SF_GenLimitsVelRange.def_ & 0xFF) || hivel != ((SF_GenLimitsVelRange.def_ >> 8) & 0xFF )  )
            AddGenerator( eSFGen::velRange, static_cast<uint16_t>( (lokvel & 0x7F) | ( (hivel & 0x7F) << 8) ) );
    }

    MidiVeloRange BaseGeneratorUser::GetVelRange()const
    {
        MidiVeloRange vr;
        auto         res = GetGenerator( eSFGen::velRange );

        if( res != nullptr )
        {
            vr.lovel = (*res) & 0xFF /*->twosby.by1*/;
            vr.hivel = (*res) >>   8/*->twosby.by2*/;
        }

        return std::move(vr);
    }

    /*
        Set or Get the volume envelope.
            delay  : timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:5000 [ 20sec] )
            attack : timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:8000 [100sec] )
            hold   : timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:5000 [ 20sec] )
            decay  : timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:8000 [100sec] )
            sustain: centibel (def:     0 [   0dB], min:     0 [   0dB], max:1440 [ 144dB] )
            release: timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:8000 [100sec] )
    */
    void BaseGeneratorUser::SetVolEnvelope( const Envelope & env )
    {
        if( env.delay != SF_GenLimitsVolEnvDelay.def_ )
            AddGenerator( eSFGen::delayVolEnv,  static_cast<uint16_t>( env.delay /*utils::Clamp( env.delay, SF_GenLimitsVolEnvDelay.min_, SF_GenLimitsVolEnvDelay.max_  )*/ ) );

        if( env.attack != SF_GenLimitsVolEnvAttack.def_ )
            AddGenerator( eSFGen::attackVolEnv, static_cast<uint16_t>(env.attack /*utils::Clamp( env.attack, SF_GenLimitsVolEnvAttack.min_, SF_GenLimitsVolEnvAttack.max_ )*/ ) );

        if( env.hold != SF_GenLimitsVolEnvHold.def_ )
            AddGenerator( eSFGen::holdVolEnv,   static_cast<uint16_t>( env.hold/*utils::Clamp( env.hold, SF_GenLimitsVolEnvHold.min_, SF_GenLimitsVolEnvHold.max_ )*/ ) );

        if( env.decay != SF_GenLimitsVolEnvDecay.def_ )
            AddGenerator( eSFGen::decayVolEnv,  static_cast<uint16_t>( env.decay/*utils::Clamp( env.decay, SF_GenLimitsVolEnvDecay.min_, SF_GenLimitsVolEnvDecay.max_ )*/ ) );

        if( env.sustain != SF_GenLimitsVolEnvSustain.def_ )
            AddGenerator( eSFGen::sustainVolEnv,static_cast<uint16_t>( env.sustain /*utils::Clamp( env.sustain, SF_GenLimitsVolEnvSustain.min_, SF_GenLimitsVolEnvSustain.max_ )*/ ) );

        if( env.release != SF_GenLimitsVolEnvRelease.def_ )
            AddGenerator( eSFGen::releaseVolEnv,static_cast<uint16_t>( env.release/*utils::Clamp( env.release, SF_GenLimitsVolEnvRelease.min_, SF_GenLimitsVolEnvRelease.max_ )*/ ) );
        ////genparam_t param;
        ////param = static_cast<uint16_t>(env.delay);
        //AddGenerator( eSFGen::delayVolEnv,  static_cast<uint16_t>( utils::Clamp( env.delay, SF_GenLimitsVolEnvDelay.min_, SF_GenLimitsVolEnvDelay.max_ ) ) );
        ////param = static_cast<uint16_t>(env.attack);
        //AddGenerator( eSFGen::attackVolEnv, static_cast<uint16_t>( utils::Clamp( env.attack, SF_GenLimitsVolEnvAttack.min_, SF_GenLimitsVolEnvAttack.max_ ) ) );
        ////param = static_cast<uint16_t>(env.hold);
        //AddGenerator( eSFGen::holdVolEnv,   static_cast<uint16_t>( utils::Clamp( env.hold, SF_GenLimitsVolEnvHold.min_, SF_GenLimitsVolEnvHold.max_ ) ) );
        ////param = static_cast<uint16_t>(env.decay);
        //AddGenerator( eSFGen::decayVolEnv,  static_cast<uint16_t>( utils::Clamp( env.decay, SF_GenLimitsVolEnvDecay.min_, SF_GenLimitsVolEnvDecay.max_ ) ) );
        ////param = static_cast<uint16_t>(env.sustain);
        //AddGenerator( eSFGen::sustainVolEnv,static_cast<uint16_t>( utils::Clamp( env.sustain, SF_GenLimitsVolEnvSustain.min_, SF_GenLimitsVolEnvSustain.max_ ) ) );
        ////param = static_cast<uint16_t>(env.release);
        //AddGenerator( eSFGen::releaseVolEnv,static_cast<uint16_t>( utils::Clamp( env.release, SF_GenLimitsVolEnvRelease.min_, SF_GenLimitsVolEnvRelease.max_ ) ) );
    }

    Envelope BaseGeneratorUser::GetVolEnvelope()const
    {
        auto delay   = GetGenerator( eSFGen::delayVolEnv   );
        auto attack  = GetGenerator( eSFGen::attackVolEnv  );
        auto hold    = GetGenerator( eSFGen::holdVolEnv    );
        auto decay   = GetGenerator( eSFGen::decayVolEnv   );
        auto sustain = GetGenerator( eSFGen::sustainVolEnv );
        auto release = GetGenerator( eSFGen::releaseVolEnv );
        Envelope result;

        if( delay != nullptr )
            result.delay   = static_cast<int16_t>(*delay);
        if( attack != nullptr )
            result.attack  = static_cast<int16_t>(*attack);
        if( hold != nullptr )
            result.hold    = static_cast<int16_t>(*hold);
        if( decay != nullptr )
            result.decay   = static_cast<int16_t>(*decay);
        if( sustain != nullptr )
            result.sustain = static_cast<int16_t>(*sustain);
        if( release != nullptr )
            result.release = static_cast<int16_t>(*release);

        return result;
    }

    /*
        Set or Get the modulation envelope.
            delay  : timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:5000 [ 20sec] )
            attack : timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:8000 [100sec] )
            hold   : timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:5000 [ 20sec] )
            decay  : timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:8000 [100sec] )
            sustain:  	-0.1% (def:     0 [  100%], min:     0 [  100%], max:1000 [    0%] )
            release: timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:8000 [100sec] )
    */
    void BaseGeneratorUser::SetModEnvelope( const Envelope & env )
    {
        genparam_t param;
        param = static_cast<uint16_t>(env.delay);
        AddGenerator( eSFGen::delayModEnv,  utils::Clamp( param, SF_GenLimitsModEnvDelay.min_, SF_GenLimitsModEnvDelay.max_ ) );
        param = static_cast<uint16_t>(env.attack);
        AddGenerator( eSFGen::attackModEnv, utils::Clamp( param, SF_GenLimitsModEnvAttack.min_, SF_GenLimitsModEnvAttack.max_ ) );
        param = static_cast<uint16_t>(env.hold);
        AddGenerator( eSFGen::holdModEnv,   utils::Clamp( param, SF_GenLimitsModEnvHold.min_, SF_GenLimitsModEnvHold.max_ ) );
        param = static_cast<uint16_t>(env.decay);
        AddGenerator( eSFGen::decayModEnv,  utils::Clamp( param, SF_GenLimitsModEnvDecay.min_, SF_GenLimitsModEnvDecay.max_ ) );
        param = static_cast<uint16_t>(env.sustain);
        AddGenerator( eSFGen::sustainModEnv,utils::Clamp( param, SF_GenLimitsModEnvSustain.min_, SF_GenLimitsModEnvSustain.max_ ) );
        param = static_cast<uint16_t>(env.release);
        AddGenerator( eSFGen::releaseModEnv,utils::Clamp( param, SF_GenLimitsModEnvRelease.min_, SF_GenLimitsModEnvRelease.max_ ) );
    }

    Envelope BaseGeneratorUser::GetModEnvelope()const
    {
        auto delay   = GetGenerator( eSFGen::delayModEnv   );
        auto attack  = GetGenerator( eSFGen::attackModEnv  );
        auto hold    = GetGenerator( eSFGen::holdModEnv    );
        auto decay   = GetGenerator( eSFGen::decayModEnv   );
        auto sustain = GetGenerator( eSFGen::sustainModEnv );
        auto release = GetGenerator( eSFGen::releaseModEnv );
        Envelope result;

        if( delay != nullptr )
            result.delay   = static_cast<int16_t>(*delay);
        if( attack != nullptr )
            result.attack  = static_cast<int16_t>(*attack);
        if( hold != nullptr )
            result.hold    = static_cast<int16_t>(*hold);
        if( decay != nullptr )
            result.decay   = static_cast<int16_t>(*decay);
        if( sustain != nullptr )
            result.sustain = *sustain;
        if( release != nullptr )
            result.release = static_cast<int16_t>(*release);

        return result;
    }


    /*
        Set or Get the coarse tune.
            (def:0, min:-120, max:120)
            Pitch offset in semitones to be applied to the note.
            Positive means a higher pitch, negative, a lower.
            Its additive with the "FineTune" generator.
    */
    void BaseGeneratorUser::SetCoarseTune( int16_t tune )
    {
#if 1
        if( tune != SF_GenLimitsCoarseTune.def_ )
            AddGenerator( eSFGen::coarseTune, utils::Clamp( tune, SF_GenLimitsCoarseTune.min_, SF_GenLimitsCoarseTune.max_ ) );
#else
        if( tune != SF_GenLimitsCoarseTune.def_ )
            AddGenerator( eSFGen::coarseTune, tune );
#endif
    }
    
    int16_t BaseGeneratorUser::GetCoarseTune()const
    {
        auto ctune = GetGenerator( eSFGen::coarseTune );

        if( ctune != nullptr )
            return *ctune;
        else
            return SF_GenLimitsCoarseTune.def_;
    }

    /*
        Set or Get the fine tune.
            (def:0, min:-99, max:99)
            Pitch offset in cents which should be applied to the note. 
            Positive means a higher pitch, negative, a lower.
            Its additive with the "CoarseTune" generator.
                
    */
    void BaseGeneratorUser::SetFineTune( int16_t ftune )
    {
#if 0
        if( ftune != SF_GenLimitsFineTune.def_ )
            AddGenerator( eSFGen::fineTune, utils::Clamp( ftune, SF_GenLimitsFineTune.min_, SF_GenLimitsFineTune.max_ ) );
#else
        if( ftune != SF_GenLimitsFineTune.def_ )
            AddGenerator( eSFGen::fineTune, ftune );
#endif
    }

    int16_t BaseGeneratorUser::GetFineTune()const
    {
        auto ftune = GetGenerator( eSFGen::fineTune );

        if( ftune != nullptr )
            return *ftune;
        else
            return SF_GenLimitsFineTune.def_;
    }

    /*
        Set or Get the sample mode. (sample looping)
            (def:0, no loop)
    */
    void BaseGeneratorUser::SetSmplMode( eSmplMode mode )
    {
        if( mode != eSmplMode::noloop )
            AddGenerator( eSFGen::sampleMode, static_cast<uint16_t>(mode) );
    }

    eSmplMode BaseGeneratorUser::GetSmplMode()const
    {
        auto smpmode = GetGenerator( eSFGen::sampleMode );

        if( smpmode != nullptr )
            return static_cast<eSmplMode>(*smpmode);
        else
            return eSmplMode::noloop;
    }

    /*
        Set or Get the Scale Tuning
            (def:100, min:0, max:1200)
            0   : means MIDI key numbers has no effect on pitch.
            100 : means the MIDI key number have full effect on the pitch.

    */
    void BaseGeneratorUser::SetScaleTuning( uint16_t scale )
    {
        if( scale != SF_GenLimitsScaleTuning.def_ )
            AddGenerator( eSFGen::scaleTuning, utils::Clamp( scale, SF_GenLimitsScaleTuning.min_, SF_GenLimitsScaleTuning.max_ ) );
    }

    uint16_t BaseGeneratorUser::GetScaleTuning()const
    {
        auto scaletune = GetGenerator( eSFGen::scaleTuning );

        if( scaletune != nullptr )
            return *scaletune;
        else
            return SF_GenLimitsScaleTuning.def_;
    }

    /*
        Set or Get the Initial Attenuation
            (def:0, min:0, max:1440)
            The attenuation in centibels applied to the note.
            0  == no attenuation
            60 == attenuated by 6dB
    */
    void BaseGeneratorUser::SetInitAtt( uint16_t att )
    {
        if( att != SF_GenLimitsInitAttenuation.def_ )
            AddGenerator( eSFGen::initialAttenuation, utils::Clamp( att, SF_GenLimitsInitAttenuation.min_, SF_GenLimitsInitAttenuation.max_ ) );
    }

    uint16_t BaseGeneratorUser::GetInitAtt()const
    {
        auto initatt = GetGenerator( eSFGen::initialAttenuation );

        if( initatt != nullptr )
            return *initatt;
        else
            return SF_GenLimitsInitAttenuation.def_;
    }

    /*
        Set or Get the Pan
            (def:0 center, min:-500 left, max: 500 right)
            The pan in 0.1% applied to the note. 
    */
    void BaseGeneratorUser::SetPan( int16_t pan )
    {
        if( pan != SF_GenLimitsPan.def_ )
            AddGenerator( eSFGen::pan, utils::Clamp( pan, SF_GenLimitsPan.min_, SF_GenLimitsPan.max_ ) );
    }

    int16_t BaseGeneratorUser::GetPan()const
    {
        auto pan = GetGenerator( eSFGen::pan );

        if( pan != nullptr )
            return *pan;
        else
            return SF_GenLimitsPan.def_;
    }

    /*
        Set or Get the Exclusive Class id
            (def:0, min:0, max:127)
            Basically, instruments  within the same
            Preset, with the same Exclusive Class Id cut eachother
            when they play.
    */
    void BaseGeneratorUser::SetExclusiveClass( uint16_t id )
    {
        if( id != SF_GenLimitsExcClass.def_ )
            AddGenerator( eSFGen::exclusiveClass, utils::Clamp( id, SF_GenLimitsExcClass.min_, SF_GenLimitsExcClass.max_ ) );
    }

    uint16_t BaseGeneratorUser::GetExclusiveClass()const
    {
        auto exclass = GetGenerator( eSFGen::exclusiveClass );

        if( exclass != nullptr )
            return *exclass;
        else
            return SF_GenLimitsExcClass.def_;
    }

    /*
        Set or Get the Reverb Send
            (def:0, min:0, max:1000)
            The amount of reverb effect in 0.1% applied to the note. 
            1000 == 100%
            http://www.pjb.com.au/midi/sfspec21.html#g16
    */
    void BaseGeneratorUser::SetReverbSend( uint16_t send )
    {
        if( send != SF_GenLimitsReverbSend.def_ )
            AddGenerator( eSFGen::reverbEffectsSend, utils::Clamp( send, SF_GenLimitsReverbSend.min_, SF_GenLimitsReverbSend.max_ ) );
    }

    uint16_t BaseGeneratorUser::GetReverbSend()const
    {
        auto send = GetGenerator( eSFGen::reverbEffectsSend );

        if( send != nullptr )
            return *send;
        else
            return SF_GenLimitsReverbSend.def_;
    }

    /*
        Set or Get the Chorus Send
            (def:0, min:0, max:1000)
            The amount of chorus effect in 0.1% applied to the note. 
            1000 == 100%
            http://www.pjb.com.au/midi/sfspec21.html#g15
    */
    void BaseGeneratorUser::SetChorusSend( uint16_t send )
    {
        if( send != SF_GenLimitsChorusSend.def_ )
            AddGenerator( eSFGen::chorusEffectsSend, utils::Clamp( send, SF_GenLimitsChorusSend.min_, SF_GenLimitsChorusSend.max_ ) );
    }

    uint16_t BaseGeneratorUser::GetChorusSend()const
    {
        auto send = GetGenerator( eSFGen::chorusEffectsSend );

        if( send != nullptr )
            return *send;
        else
            return SF_GenLimitsChorusSend.def_;
    }

    void BaseGeneratorUser::SetRootKey( int16_t key )
    {
        if( key != SF_GenLimitsOverrideRootKey.def_ )
            AddGenerator( eSFGen::overridingRootKey, utils::Clamp( key, SF_GenLimitsOverrideRootKey.min_, SF_GenLimitsOverrideRootKey.max_ ) );
    }

    int16_t BaseGeneratorUser::GetRootKey()const
    {
        auto overrideroot = GetGenerator( eSFGen::overridingRootKey );

        if( overrideroot != nullptr )
            return *overrideroot;
        else
            return SF_GenLimitsOverrideRootKey.def_;
    }

//=========================================================================================
//  BaseModulatorUser
//=========================================================================================

    /*
        AddModulator
            Return modulator index in this instrument's list.
    */
    size_t BaseModulatorUser::AddModulator( SFModEntry && mod )
    {
        //Search for a modulator with the same set of 
        // ModSrcOper, ModDestOper, and ModSrcAmtOper
        for( size_t i = 0; i < m_mods.size(); ++i )
        {
            auto & exmod = m_mods[i];

            //If we find a modulator with the same 3 defining parameters
            if( exmod.ModSrcOper    == mod.ModSrcOper  &&
                exmod.ModDestOper   == mod.ModDestOper &&
                exmod.ModAmtSrcOper == mod.ModAmtSrcOper )
            {
                //Then just affect the other values, as we can't have more than once the same combination of defining paramenters
                exmod.modAmount    = mod.modAmount;
                exmod.ModTransOper = mod.ModTransOper;
                return i;
            }
        }

        //If we didn't get a match, add it to the list
        m_mods.push_back( std::move(mod) );
        return (m_mods.size()-1);
    }

    /*
        GetModulator
            Return the modulator at the index specified.
    */
    SFModEntry & BaseModulatorUser::GetModulator( size_t index )
    {
        return m_mods[index];
    }

    const SFModEntry & BaseModulatorUser::GetModulator( size_t index )const
    {
        return m_mods[index];
    }

    /*
        GetNbModulators
    */
    size_t BaseModulatorUser::GetNbModulators()const
    {
        return m_mods.size();
    }

//=====================================================================================
//  ZoneBag
//=====================================================================================
    //void ZoneBag::Sort()
    //{
    //    //Sort Generators:
    //    //std::sort( m_gens.begin(), m_gens.end(), 
    //    //    []( const map<eSFGen,genparam_t>::value_type & v1, 
    //    //        const map<eSFGen,genparam_t>::value_type & v2 )
    //    //    {
    //    //        return ( GetGenPriority(v1.first) > GetGenPriority(v2.first) ); //Normally the comparison is "<", but since higher values means higher priority we reverse it !
    //    //    });
    //}

};