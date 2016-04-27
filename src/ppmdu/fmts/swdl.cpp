#include "swdl.hpp"
#include <dse/dse_containers.hpp>
#include <utils/library_wide.hpp>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
using namespace std;

namespace DSE
{

    static const uint32_t SWDL_PCMDSpecialSize     = 0xAAAA0000; //Value the pcmd lenght in the SWDL header had to indicate it refers to a master bank of samples.
    //+#FIXME: The upper half is actually the padding byte used by the devs. Not always 0xAAAA
    
    static const uint32_t SWDL_PCMDSpecialSizeMask = 0xFFFF0000; //Mask to apply to verify the special size. As the two lower bytes seems to contain some value in some swd files!


    struct DSESmplFmtInfo
    {
        eDSESmplFmt fmt;
        uint8_t     unk9;
        uint8_t     smplblk;
        uint8_t     bitdepth;
        uint8_t     bps;
    };
    const static DSESmplFmtInfo              DSEInvalidSmplFmtData { eDSESmplFmt::invalid, 0, 0, 0, 0 };
    const static std::vector<DSESmplFmtInfo> DSESmplFmtData
    {{
        { eDSESmplFmt::pcm8,      1, 4,  8, 1 },
        { eDSESmplFmt::pcm16,     1, 2, 16, 2 },
        { eDSESmplFmt::ima_adpcm, 9, 8,  4, 1 },
        //Idk if there are any other formats.
    }};


//===============================================================================
//  SWDL_Header
//===============================================================================
    /*
        DoesSWDLContainsSamples
            Returns true if the swdl contains sample data.

        #FIXME : This isn't going to work in every cases! Especially with Version 0x402 DSE
    */
    bool SWDL_Header::DoesContainsSamples()const
    {
        return (pcmdlen > 0) && 
               ((pcmdlen & SWDL_PCMDSpecialSizeMask) != SWDL_PCMDSpecialSize);
    }

    /*
        IsSWDLSampleBankOnly
            Returns true if the swdl is only a sample bank, without program info.
    */
    bool SWDL_Header::IsSampleBankOnly()const
    {
        return (pcmdlen > 0) &&                                                  // #FIXME : This isn't going to work in every cases! Especially with Version 0x402 DSE
               ((pcmdlen & SWDL_PCMDSpecialSizeMask) != SWDL_PCMDSpecialSize) && // #FIXME : This isn't going to work in every cases! Especially with Version 0x402 DSE
               (nbprgislots == 0);
    }

//===============================================================================
//  Data Structures
//===============================================================================

    /***********************************************
        WavInfo_v415
            
    ***********************************************/
    struct WavInfo_v415
    {
        static const uint32_t Size     = 64;
        static const uint16_t DefUnk1  = 0xAA01;
        static const uint8_t  DefUnk5  = 0;
        static const uint8_t  DefUnk58 = 2;
        static const uint16_t DefUnk6  = 0;
        static const uint16_t DefUnk7  = 0xAAAA;

        static const uint8_t  DefUnk10 = 1;
        static const uint8_t  DefUnk11 = 0;

        static const uint32_t DefUnk13 = 0;
        static const uint8_t  DefUnk19 = 1;
        static const uint8_t  DefUnk20 = 3;
        static const uint16_t DefUnk21 = 0x03FF;
        static const uint16_t DefUnk22 = 0xFFFF;
        static const uint8_t  DefUnk57 = 0xFF;

        uint16_t unk1;
        uint16_t id;        //Index/ID of the sample
        int8_t   ftune; 
        int8_t   ctune;
        uint8_t  rootkey;   //Possibly the MIDI key matching the pitch the sample was sampled at!
        int8_t   ktps;      //Transpose
        int8_t   vol;
        int8_t   pan;
        uint8_t  unk5;
        uint8_t  unk58;
        uint16_t unk6;
        uint16_t unk7;
        uint16_t version;   //dse version. usually 0x415
        uint16_t smplfmt;   //Format of the sample 0x100 == PCM 16, 0x200 == IMA ADPCM
        uint8_t  unk9;      //Some enum value. Is usually set to 1, but is set to 9 with ADPCM samples.
        uint8_t  smplloop;    //loop flag, 1 = loop, 0 = no loop
        uint8_t  unk10;
        uint8_t  smplblk;    //Nb of samples per block of 32 bits
        uint8_t  unk11;     //
        uint8_t  bitdepth;    //Nb of bits per sample
        uint8_t  bps1;      //Bytes per sample ?
        uint8_t  bps2;      //Bytes per sample again?
        uint32_t unk13;
        uint32_t smplrate;  //Sampling rate of the sample
        uint32_t smplpos;   //Offset within pcmd chunk of the sample
        uint32_t loopbeg;   //Loop start in int32 (based on the resulting PCM16)
        uint32_t looplen;   //Length of the sample in int32
        uint8_t  envon;
        uint8_t  envmult;
        uint8_t  unk19;
        uint8_t  unk20;
        uint16_t unk21;
        uint16_t unk22;
        int8_t   atkvol;
        int8_t   attack;
        int8_t   decay;
        int8_t   sustain;
        int8_t   hold;
        int8_t   decay2;
        int8_t   release;
        int8_t   unk57;

        WavInfo_v415()
        {
            unk1       = 0;
            id         = 0;
            ftune      = 0; 
            ctune      = 0;
            rootkey    = 0;
            ktps       = 0;
            vol        = 0;
            pan        = 0;
            unk5       = 0;
            unk58      = 0;
            unk6       = 0;
            unk7       = 0;
            version    = 0;
            smplfmt    = 0;
            unk9       = 0;
            smplloop   = 0;
            unk10      = 0;
            smplblk    = 0;
            unk11      = 0;
            bitdepth   = 0;
            bps1       = 0;
            bps2       = 0;
            unk13      = 0;
            smplrate   = 0;
            smplpos    = 0;
            loopbeg    = 0;
            looplen    = 0;
            envon      = 0;
            envmult    = 0;
            unk19      = 0;
            unk20      = 0;
            unk21      = 0;
            unk22      = 0;
            atkvol     = 0;
            attack     = 0;
            decay      = 0;
            sustain    = 0;
            hold       = 0;
            decay2     = 0;
            release    = 0;
            unk57      = 0;
        }

        WavInfo_v415( const WavInfo & winf )
        {
            (*this) = winf;
        }


        //Write the structure using an iterator to a byte container
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( unk1,      itwriteto );
            itwriteto = utils::WriteIntToBytes( id,        itwriteto );
            itwriteto = utils::WriteIntToBytes( ftune,     itwriteto );
            itwriteto = utils::WriteIntToBytes( ctune,     itwriteto );
            itwriteto = utils::WriteIntToBytes( rootkey,   itwriteto );
            itwriteto = utils::WriteIntToBytes( ktps,      itwriteto );
            itwriteto = utils::WriteIntToBytes( vol,       itwriteto );
            itwriteto = utils::WriteIntToBytes( pan,       itwriteto );
            itwriteto = utils::WriteIntToBytes( unk5,      itwriteto );
            itwriteto = utils::WriteIntToBytes( unk58,     itwriteto );
            itwriteto = utils::WriteIntToBytes( unk6,      itwriteto );
            itwriteto = utils::WriteIntToBytes( unk7,      itwriteto );
            itwriteto = utils::WriteIntToBytes( version,   itwriteto );
            itwriteto = utils::WriteIntToBytes( smplfmt,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unk9,      itwriteto );
            itwriteto = utils::WriteIntToBytes( smplloop,  itwriteto );
            itwriteto = utils::WriteIntToBytes( unk10,     itwriteto );
            itwriteto = utils::WriteIntToBytes( smplblk,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unk11,     itwriteto );
            itwriteto = utils::WriteIntToBytes( bitdepth,  itwriteto );
            itwriteto = utils::WriteIntToBytes( bps1,      itwriteto );
            itwriteto = utils::WriteIntToBytes( bps2,      itwriteto );
            itwriteto = utils::WriteIntToBytes( unk13,     itwriteto );
            itwriteto = utils::WriteIntToBytes( smplrate,  itwriteto );
            itwriteto = utils::WriteIntToBytes( smplpos,   itwriteto );
            itwriteto = utils::WriteIntToBytes( loopbeg,   itwriteto );
            itwriteto = utils::WriteIntToBytes( looplen,   itwriteto );
            itwriteto = utils::WriteIntToBytes( envon,     itwriteto );
            itwriteto = utils::WriteIntToBytes( envmult,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unk19,     itwriteto );
            itwriteto = utils::WriteIntToBytes( unk20,     itwriteto );
            itwriteto = utils::WriteIntToBytes( unk21,     itwriteto );
            itwriteto = utils::WriteIntToBytes( unk22,     itwriteto );
            itwriteto = utils::WriteIntToBytes( atkvol,    itwriteto );
            itwriteto = utils::WriteIntToBytes( attack,    itwriteto );
            itwriteto = utils::WriteIntToBytes( decay,     itwriteto );
            itwriteto = utils::WriteIntToBytes( sustain,   itwriteto );
            itwriteto = utils::WriteIntToBytes( hold,      itwriteto );
            itwriteto = utils::WriteIntToBytes( decay2,    itwriteto );
            itwriteto = utils::WriteIntToBytes( release,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unk57,     itwriteto );
            return itwriteto;
        }

        //Read the structure from an iterator on a byte container
        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itend )
        {
            itReadfrom = utils::ReadIntFromBytes( unk1,     itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( id,       itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( ftune,    itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( ctune,    itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( rootkey,  itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( ktps,     itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( vol,      itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( pan,      itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk5,     itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk58,    itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk6,     itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk7,     itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( version,  itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( smplfmt,  itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk9,     itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( smplloop, itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk10,    itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( smplblk,  itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk11,    itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( bitdepth, itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( bps1,     itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( bps2,     itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk13,    itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( smplrate, itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( smplpos,  itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( loopbeg,  itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( looplen,  itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( envon,    itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( envmult,  itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk19,    itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk20,    itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk21,    itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk22,    itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( atkvol,   itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( attack,   itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( decay,    itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( sustain,  itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( hold,     itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( decay2,   itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( release,  itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk57,    itReadfrom, itend );
            return itReadfrom;
        }


        operator WavInfo()
        {
            WavInfo winf;
            winf.id       = id;
            winf.ftune    = ftune; 
            winf.ctune    = ctune;
            winf.rootkey  = rootkey;
            winf.ktps     = ktps;
            winf.vol      = vol;
            winf.pan      = pan;
            winf.smplfmt  = IntToDSESmplFmt(smplfmt);
            winf.smplloop = smplloop != 0;
            winf.smplrate = smplrate;
            winf.smplpos  = smplpos;
            winf.loopbeg  = loopbeg;
            winf.looplen  = looplen;
            winf.envon    = envon;
            winf.envmult  = envmult;
            winf.atkvol   = atkvol;
            winf.attack   = attack;
            winf.decay    = decay;
            winf.sustain  = sustain;
            winf.hold     = hold;
            winf.decay2   = decay2;
            winf.release  = release;
            return move(winf);
        }

        /*
            Prepare from a WaveInfo.
        */
        WavInfo_v415 & operator=( const WavInfo & winf )
        {
            auto itfoundfmt = std::find_if( std::begin(DSESmplFmtData), 
                                            std::end(DSESmplFmtData), 
                                            [&]( const DSESmplFmtInfo & info )->bool{ return info.fmt == winf.smplfmt; } );

            if( itfoundfmt == std::end(DSESmplFmtData) )
                throw runtime_error("WaveInfo_v415::operator=() : Unknown sample format..");

            unk1       = DefUnk1;
            id         = winf.id;
            ftune      = winf.ftune; 
            ctune      = winf.ctune;
            rootkey    = winf.rootkey; 
            ktps       = winf.ktps;
            vol        = winf.vol;
            pan        = winf.pan;
            unk5       = DefUnk5;
            unk58      = DefUnk58;
            unk6       = 0;
            unk7       = DefUnk7;
            version    = static_cast<uint16_t>(eDSEVersion::V415);
            //Sample Info
            smplfmt    = static_cast<uint16_t>(winf.smplfmt);
            unk9       = itfoundfmt->unk9; 
            smplloop   = winf.smplloop; 
            unk10      = DefUnk10;
            smplblk    = itfoundfmt->smplblk;
            unk11      = 0;
            bitdepth   = itfoundfmt->bitdepth;
            bps1       = itfoundfmt->bps;
            bps2       = itfoundfmt->bps;
            unk13      = 0;
            smplrate   = winf.smplrate;
            smplpos    = winf.smplpos;
            loopbeg    = winf.loopbeg;
            looplen    = winf.looplen;
            //Envelope
            envon      = winf.envon;
            envmult    = winf.envmult;
            unk19      = DefUnk19;
            unk20      = DefUnk20;
            unk21      = DefUnk21;
            unk22      = DefUnk22;
            atkvol     = winf.atkvol;
            attack     = winf.attack;
            decay      = winf.decay;
            sustain    = winf.sustain;
            hold       = winf.hold;
            decay2     = winf.decay2;
            release    = winf.release;
            unk57      = DefUnk57;
            return *this;
        }

    };

    /***********************************************
        WavInfo_v402
            
    ***********************************************/
    struct WavInfo_v402
    {
        static const uint32_t Size     = 64;
        static const uint8_t  DefUnk1  = 0x01;
        static const uint8_t  DefUnk5  = 2;
        static const uint8_t  DefUnk19 = 1;
        static const uint8_t  DefUnk20 = 3;
        static const uint16_t DefUnk21 = 0x03FF;
        static const uint16_t DefUnk22 = 0xFFFF;
        static const uint8_t  DefUnk57 = 0xFF;

        uint8_t  unk1       ;
        uint16_t id         ; //Index/ID of the sample
        int8_t   unk2       ;
        int8_t   unk6       ;
        uint8_t  rootkey    ; //Possibly the MIDI key matching the pitch the sample was sampled at!
        int8_t   ktps       ; //Transpose
        int8_t   vol        ;
        int8_t   pan        ;
        uint16_t smplfmt    ; //Format of the sample 0x100 == PCM 16, 0x200 == IMA ADPCM
        uint16_t unk3       ;
        uint32_t unk4       ;
        uint8_t  unk5       ;
        uint8_t  smplloop   ; //loop flag, 1 = loop, 0 = no loop
        uint32_t smplrate   ; //Sampling rate of the sample
        uint32_t smplpos    ; //Offset within pcmd chunk of the sample
        uint32_t loopbeg    ; //Loop start in int32 (based on the resulting PCM16)
        uint32_t looplen    ; //Length of the sample in int32
        // ... 16 bytes of junk/padding here ....
        uint8_t  envon      ;
        uint8_t  envmult    ;
        uint8_t  unk19      ;
        uint8_t  unk20      ;
        uint16_t unk21      ;
        uint16_t unk22      ;
        int8_t   atkvol     ;
        int8_t   attack     ;
        int8_t   decay      ;
        int8_t   sustain    ;
        int8_t   hold       ;
        int8_t   decay2     ;
        int8_t   release    ;
        int8_t   unk57      ;


        WavInfo_v402()
        {
            unk1       = 0;
            id         = 0; //Index/ID of the sample
            unk2       = 0;
            unk6       = 0;
            rootkey    = 0; //Possibly the MIDI key matching the pitch the sample was sampled at!
            ktps       = 0; //Transpose
            vol        = 0;
            pan        = 0;
            smplfmt    = 0; //Format of the sample 0x100 == PCM 16, 0x200 == IMA ADPCM
            unk3       = 0;
            unk4       = 0;
            unk5       = 0;
            smplloop   = 0; //loop flag, 1 = loop, 0 = no loop
            smplrate   = 0; //Sampling rate of the sample
            smplpos    = 0; //Offset within pcmd chunk of the sample
            loopbeg    = 0; //Loop start in int32 (based on the resulting PCM16)
            looplen    = 0; //Length of the sample in int32
            envon      = 0;
            envmult    = 0;
            unk19      = 0;
            unk20      = 0;
            unk21      = 0;
            unk22      = 0;
            atkvol     = 0;
            attack     = 0;
            decay      = 0;
            sustain    = 0;
            hold       = 0;
            decay2     = 0;
            release    = 0;
            unk57      = 0;
        }

        WavInfo_v402( const WavInfo & winf )
        {
            unk1       = DefUnk1;
            id         =  winf.id; //Index/ID of the sample
            unk2       = 0;
            unk6       = 0;
            rootkey    = winf.rootkey; //Possibly the MIDI key matching the pitch the sample was sampled at!
            ktps       = winf.ktps; //Transpose
            vol        = winf.vol;
            pan        = winf.pan;
            smplfmt    = static_cast<uint16_t>(winf.smplfmt); //Format of the sample 0x100 == PCM 16, 0x200 == IMA ADPCM
            unk3       = 0;
            unk4       = 0;
            unk5       = DefUnk5;
            smplloop   = winf.smplloop; //loop flag, 1 = loop, 0 = no loop
            smplrate   = winf.smplrate; //Sampling rate of the sample
            smplpos    = winf.smplpos; //Offset within pcmd chunk of the sample
            loopbeg    = winf.loopbeg; //Loop start in int32 (based on the resulting PCM16)
            looplen    = winf.looplen; //Length of the sample in int32
            envon      = winf.envon;
            envmult    = winf.envmult;
            unk19      = DefUnk19;
            unk20      = DefUnk20;
            unk21      = DefUnk21;
            unk22      = DefUnk22;
            atkvol     = winf.atkvol;
            attack     = winf.attack;
            decay      = winf.decay;
            sustain    = winf.sustain;
            hold       = winf.hold;
            decay2     = winf.decay2;
            release    = winf.release;
            unk57      = DefUnk57;
        }

        //Write the structure using an iterator to a byte container
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( unk1,       itwriteto );
            itwriteto = utils::WriteIntToBytes( id,         itwriteto );
            itwriteto = utils::WriteIntToBytes( unk2,       itwriteto );
            itwriteto = utils::WriteIntToBytes( unk6,       itwriteto );
            itwriteto = utils::WriteIntToBytes( rootkey,    itwriteto );
            itwriteto = utils::WriteIntToBytes( ktps,       itwriteto );
            itwriteto = utils::WriteIntToBytes( vol,        itwriteto );
            itwriteto = utils::WriteIntToBytes( pan,        itwriteto );
            itwriteto = utils::WriteIntToBytes( smplfmt,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk3,       itwriteto );
            itwriteto = utils::WriteIntToBytes( unk4,       itwriteto );
            itwriteto = utils::WriteIntToBytes( unk5,       itwriteto );
            itwriteto = utils::WriteIntToBytes( smplloop,   itwriteto );
            itwriteto = utils::WriteIntToBytes( smplrate,   itwriteto );
            itwriteto = utils::WriteIntToBytes( smplpos,    itwriteto );
            itwriteto = utils::WriteIntToBytes( loopbeg,    itwriteto );
            itwriteto = utils::WriteIntToBytes( looplen,    itwriteto );
            // ... 16 bytes of junk/padding here ....
            std::fill_n( itwriteto, 16, 0 );
            itwriteto = utils::WriteIntToBytes( envon,      itwriteto );
            itwriteto = utils::WriteIntToBytes( envmult,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk19,      itwriteto );
            itwriteto = utils::WriteIntToBytes( unk20,      itwriteto );
            itwriteto = utils::WriteIntToBytes( unk21,      itwriteto );
            itwriteto = utils::WriteIntToBytes( unk22,      itwriteto );
            itwriteto = utils::WriteIntToBytes( atkvol,     itwriteto );
            itwriteto = utils::WriteIntToBytes( attack,     itwriteto );
            itwriteto = utils::WriteIntToBytes( decay,      itwriteto );
            itwriteto = utils::WriteIntToBytes( sustain,    itwriteto );
            itwriteto = utils::WriteIntToBytes( hold,       itwriteto );
            itwriteto = utils::WriteIntToBytes( decay2,     itwriteto );
            itwriteto = utils::WriteIntToBytes( release,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk57,      itwriteto );
            return itwriteto;
        }

        //Read the structure from an iterator on a byte container
        template<class _init>
            _init ReadFromContainer( _init itRead, _init itend )
        {
            itRead = utils::ReadIntFromBytes( unk1,       itRead, itend );
            itRead = utils::ReadIntFromBytes( id,         itRead, itend );
            itRead = utils::ReadIntFromBytes( unk2,       itRead, itend );
            itRead = utils::ReadIntFromBytes( unk6,       itRead, itend );
            itRead = utils::ReadIntFromBytes( rootkey,    itRead, itend );
            itRead = utils::ReadIntFromBytes( ktps,       itRead, itend );
            itRead = utils::ReadIntFromBytes( vol,        itRead, itend );
            itRead = utils::ReadIntFromBytes( pan,        itRead, itend );
            itRead = utils::ReadIntFromBytes( smplfmt,    itRead, itend );
            itRead = utils::ReadIntFromBytes( unk3,       itRead, itend );
            itRead = utils::ReadIntFromBytes( unk4,       itRead, itend );
            itRead = utils::ReadIntFromBytes( unk5,       itRead, itend );
            itRead = utils::ReadIntFromBytes( smplloop,   itRead, itend );
            itRead = utils::ReadIntFromBytes( smplrate,   itRead, itend );
            itRead = utils::ReadIntFromBytes( smplpos,    itRead, itend );
            itRead = utils::ReadIntFromBytes( loopbeg,    itRead, itend );
            itRead = utils::ReadIntFromBytes( looplen,    itRead, itend );
            // ... 16 bytes of junk/padding here ....
            itRead = utils::advAsMuchAsPossible( itRead, itend, 16 );
            itRead = utils::ReadIntFromBytes( envon,      itRead, itend );
            itRead = utils::ReadIntFromBytes( envmult,    itRead, itend );
            itRead = utils::ReadIntFromBytes( unk19,      itRead, itend );
            itRead = utils::ReadIntFromBytes( unk20,      itRead, itend );
            itRead = utils::ReadIntFromBytes( unk21,      itRead, itend );
            itRead = utils::ReadIntFromBytes( unk22,      itRead, itend );
            itRead = utils::ReadIntFromBytes( atkvol,     itRead, itend );
            itRead = utils::ReadIntFromBytes( attack,     itRead, itend );
            itRead = utils::ReadIntFromBytes( decay,      itRead, itend );
            itRead = utils::ReadIntFromBytes( sustain,    itRead, itend );
            itRead = utils::ReadIntFromBytes( hold,       itRead, itend );
            itRead = utils::ReadIntFromBytes( decay2,     itRead, itend );
            itRead = utils::ReadIntFromBytes( release,    itRead, itend );
            itRead = utils::ReadIntFromBytes( unk57,      itRead, itend );
            return itRead;
        }

        /*
            Put the relevant info into a WavInfo
        */
        operator WavInfo()
        {
            WavInfo winf;
            winf.id       = id;
            winf.ftune    = 0;  //There doesn't seem to be any tuning info for samples in v402
            winf.ctune    = 0;  //There doesn't seem to be any tuning info for samples in v402
            winf.rootkey  = rootkey;
            winf.ktps     = ktps;
            winf.vol      = vol;
            winf.pan      = pan;
            winf.smplfmt  = IntToDSESmplFmt(smplfmt);
            winf.smplloop = smplloop != 0;
            winf.smplrate = smplrate;
            winf.smplpos  = smplpos;
            winf.loopbeg  = loopbeg;
            winf.looplen  = looplen;
            winf.envon    = envon;
            winf.envmult  = envmult;
            winf.atkvol   = atkvol;
            winf.attack   = attack;
            winf.decay    = decay;
            winf.sustain  = sustain;
            winf.hold     = hold;
            winf.decay2   = decay2;
            winf.release  = release;
            return move(winf);
        }

        /*
            Prepare from a WaveInfo.
        */
        WavInfo_v402 & operator=( const WavInfo & winf )
        {
            unk1       = DefUnk1;
            id         = winf.id;
            unk2       = 0;
            unk6       = 0;
            rootkey    = winf.rootkey; 
            ktps       = winf.ktps;
            vol        = winf.vol;
            pan        = winf.pan;
            smplfmt    = static_cast<uint16_t>(winf.smplfmt);
            unk3       = 0;
            unk4       = 0;
            unk5       = DefUnk5;
            smplloop   = winf.smplloop; 
            smplrate   = winf.smplrate;
            smplpos    = winf.smplpos;
            loopbeg    = winf.loopbeg;
            looplen    = winf.looplen;
            envon      = winf.envon;
            envmult    = winf.envmult;
            unk19      = DefUnk19;
            unk20      = DefUnk20;
            unk21      = DefUnk21;
            unk22      = DefUnk22;
            atkvol     = winf.atkvol;
            attack     = winf.attack;
            decay      = winf.decay;
            sustain    = winf.sustain;
            hold       = winf.hold;
            decay2     = winf.decay2;
            release    = winf.release;
            unk57      = DefUnk57;
            return *this;
        }
    };




    /*---------------------------------------------------------------------
        SplitEntry_v415
            Data on a particular sample mapped to this instrument
    ---------------------------------------------------------------------*/
    struct SplitEntry_v415
    {
        static const uint32_t SIZE     = 48; //bytes
        static const uint8_t  DefUnk22 = 2;
        static const uint8_t  DefUnk37 = 1;
        static const uint8_t  DefUnk38 = 3;
        static const uint16_t DefUnk39 = 0xFF03;
        static const uint16_t DefUnk40 = 0xFFFF;
        static const uint8_t  DefRX    = 0xFF;


        static uint32_t size() { return SIZE; }

        uint16_t id     ; //0x1
        uint8_t  unk11  ; //0x2
        uint8_t  unk25  ; //0x3
        int8_t   lowkey ; //0x4
        int8_t   hikey  ; //0x5
        int8_t   lowkey2; //0x6
        int8_t   hikey2 ; //0x7
        int8_t   lovel  ; //0x8
        int8_t   hivel  ; //0x9
        int8_t   lovel2 ; //0xA
        int8_t   hivel2 ; //0xB
        uint32_t unk16  ; //0xC
        uint16_t unk17  ; //0x10
        uint16_t smplid ; //0x12
        int8_t   ftune  ; //0x14
        int8_t   ctune  ; //0x15
        int8_t   rootkey; //0x16 
        int8_t   ktps   ; //0x17 //Key transposition
        uint8_t  smplvol; //0x18
        uint8_t  smplpan; //0x19
        uint8_t  kgrpid ; //0x1A
        uint8_t  unk22  ; //0x1B
        uint16_t unk23  ; //0x1C
        uint16_t unk24  ; //0x1E
        //Envelope
        uint8_t  envon  ; //0x20  //Switch on or off envelope
        uint8_t  envmult; //0x21 //Multiplier for other envelope params
        uint8_t  unk37  ; //0x22
        uint8_t  unk38  ; //0x23
        uint16_t unk39  ; //0x24
        int16_t  unk40  ; //0x26
        int8_t   atkvol ; //0x28
        int8_t   attack ; //0x29
        int8_t   decay  ; //0x2A
        int8_t   sustain; //0x2B
        int8_t   hold   ; //0x2C
        int8_t   decay2 ; //0x2D
        int8_t   release; //0x2E
        int8_t   rx     ; //0x2F

        SplitEntry_v415()
        {
            id       = 0;
            unk11    = 0;
            unk25    = 0;
            lowkey   = 0;
            hikey    = 0;
            lowkey2  = 0;
            hikey2   = 0;
            lovel    = 0;
            hivel    = 0;
            lovel2   = 0;
            hivel2   = 0;
            unk16    = 0;
            unk17    = 0;
            smplid   = 0;
            ftune    = 0;
            ctune    = 0;
            rootkey  = 0;
            ktps     = 0;
            smplvol  = 0;
            smplpan  = 0;
            kgrpid   = 0;
            unk22    = 0;
            unk23    = 0;
            unk24    = 0;
            envon    = 0;
            envmult  = 0;
            unk37    = 0;
            unk38    = 0;
            unk39    = 0;
            unk40    = 0;
            atkvol   = 0;
            attack   = 0;
            decay    = 0;
            sustain  = 0;
            hold     = 0;
            decay2   = 0;
            release  = 0;
            rx       = 0;
        }

        SplitEntry_v415( const SplitEntry & sent )
        {
            this->operator=(sent);
        }


        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( id,       itwriteto );
            itwriteto = utils::WriteIntToBytes( unk11,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk25,    itwriteto );
            itwriteto = utils::WriteIntToBytes( lowkey,   itwriteto );
            itwriteto = utils::WriteIntToBytes( hikey,    itwriteto );
            itwriteto = utils::WriteIntToBytes( lowkey2,  itwriteto );
            itwriteto = utils::WriteIntToBytes( hikey2,   itwriteto );
            itwriteto = utils::WriteIntToBytes( lovel,    itwriteto );
            itwriteto = utils::WriteIntToBytes( hivel,    itwriteto );
            itwriteto = utils::WriteIntToBytes( lovel2,   itwriteto );
            itwriteto = utils::WriteIntToBytes( hivel2,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unk16,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk17,    itwriteto );
            itwriteto = utils::WriteIntToBytes( smplid,   itwriteto );
            itwriteto = utils::WriteIntToBytes( ftune,    itwriteto );
            itwriteto = utils::WriteIntToBytes( ctune,    itwriteto );
            itwriteto = utils::WriteIntToBytes( rootkey,  itwriteto );
            itwriteto = utils::WriteIntToBytes( ktps,     itwriteto );
            itwriteto = utils::WriteIntToBytes( smplvol,  itwriteto );
            itwriteto = utils::WriteIntToBytes( smplpan,  itwriteto );
            itwriteto = utils::WriteIntToBytes( kgrpid,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unk22,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk23,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk24,    itwriteto );
            itwriteto = utils::WriteIntToBytes( envon,    itwriteto );
            itwriteto = utils::WriteIntToBytes( envmult,  itwriteto );
            itwriteto = utils::WriteIntToBytes( unk37,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk38,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk39,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk40,    itwriteto );
            itwriteto = utils::WriteIntToBytes( atkvol,   itwriteto );
            itwriteto = utils::WriteIntToBytes( attack,   itwriteto );
            itwriteto = utils::WriteIntToBytes( decay,    itwriteto );
            itwriteto = utils::WriteIntToBytes( sustain,  itwriteto );
            itwriteto = utils::WriteIntToBytes( hold,     itwriteto );
            itwriteto = utils::WriteIntToBytes( decay2,   itwriteto );
            itwriteto = utils::WriteIntToBytes( release,  itwriteto );
            itwriteto = utils::WriteIntToBytes( rx,       itwriteto );
            return itwriteto;
        }


        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itpastend )
        {
            itReadfrom = utils::ReadIntFromBytes( id,       itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk11,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk25,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( lowkey,   itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( hikey,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( lowkey2,  itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( hikey2,   itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( lovel,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( hivel,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( lovel2,   itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( hivel2,   itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk16,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk17,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( smplid,   itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( ftune,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( ctune,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( rootkey,  itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( ktps,     itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( smplvol,  itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( smplpan,  itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( kgrpid,   itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk22,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk23,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk24,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( envon,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( envmult,  itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk37,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk38,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk39,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk40,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( atkvol,   itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( attack,   itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( decay,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( sustain,  itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( hold,     itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( decay2,   itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( release,  itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( rx,       itReadfrom, itpastend );
            return itReadfrom;
        }

        /*
            Convert to Version independent Split Entry. AKA remove the useless junk.
        */
        operator SplitEntry()const
        {
            SplitEntry sent;
            sent.id        = id;
            sent.unk11     = unk11;
            sent.unk25     = unk25;
            sent.lowkey    = lowkey;
            sent.hikey     = hikey;
            sent.lowkey2   = lowkey2;
            sent.hikey2    = hikey2;
            sent.lovel     = lovel;
            sent.hivel     = hivel;
            sent.lovel2    = lovel2;
            sent.hivel2    = hivel2;
            sent.smplid    = smplid;
            sent.ftune     = ftune;
            sent.ctune     = ctune;
            sent.rootkey   = rootkey;
            sent.ktps      = ktps;
            sent.smplvol   = smplvol;
            sent.smplpan   = smplpan;
            sent.kgrpid    = kgrpid;
            sent.envon     = envon;

            sent.env.envmulti = envmult;
            sent.env.atkvol   = atkvol;
            sent.env.attack   = attack;
            sent.env.decay    = decay;
            sent.env.sustain  = sustain;
            sent.env.hold     = hold;
            sent.env.decay2   = decay2;
            sent.env.release  = release;
            return std::move(sent);
        }

        SplitEntry_v415 & operator=( const SplitEntry & sent )
        {
            id       = sent.id;
            unk11    = sent.unk11;
            unk25    = sent.unk25;
            lowkey   = sent.lowkey;
            hikey    = sent.hikey;
            lowkey2  = sent.lowkey2;
            hikey2   = sent.hikey2;
            lovel    = sent.lovel;
            hivel    = sent.hivel;
            lovel2   = sent.lovel2;
            hivel2   = sent.hivel2;
            unk16    = 0; //Should be the padding byte, but honestly, those are probably just skipped
            unk17    = 0; //Should be the padding byte, but honestly, those are probably just skipped
            smplid   = sent.smplid;
            ftune    = sent.ftune;
            ctune    = sent.ctune;
            rootkey  = sent.rootkey;
            ktps     = sent.ktps;
            smplvol  = sent.smplvol;
            smplpan  = sent.smplpan;
            kgrpid   = sent.kgrpid;
            unk22    = DefUnk22;
            unk23    = 0;
            unk24    = 0; //Should be the padding byte, but honestly, those are probably just skipped
            envon    = sent.envon;
            envmult  = sent.env.envmulti;
            unk37    = DefUnk37;
            unk38    = DefUnk38;
            unk39    = DefUnk39;
            unk40    = DefUnk40;
            atkvol   = sent.env.atkvol;
            attack   = sent.env.attack;
            decay    = sent.env.decay;
            sustain  = sent.env.sustain;
            hold     = sent.env.hold;
            decay2   = sent.env.decay2;
            release  = sent.env.release;
            rx       = DefRX;
            return *this;
        }
    };


    /*****************************************************************************************
        ProgramInfo_v415
            Contains data for a single instrument.
    *****************************************************************************************/
    class ProgramInfo_v415
    {
    public:

        ProgramInfo_v415()
        {}

        ProgramInfo_v415( const ProgramInfo & prginf )
        {
            this->operator=(prginf);
        }

        ProgramInfo_v415( ProgramInfo && prginf )
        {
            this->operator=(std::move(prginf));
        }


        /*---------------------------------------------------------------------
            InstInfoHeader
                First 16 bytes of an instrument info block
        ---------------------------------------------------------------------*/
        struct PrgInfoHeader
        {
            static const uint32_t SIZE    = 16; //bytes
            static const uint16_t DefUnk4 = 0x200;

            static uint32_t size() { return SIZE; }

            uint16_t id        = 0;
            uint16_t nbsplits  = 0;
            uint8_t  prgvol    = 0;
            uint8_t  prgpan    = 0;
            uint8_t  unk3      = 0;
            uint8_t  unkpoly   = 0;

            uint16_t unk4      = 0;
            uint8_t  unk5      = 0;
            uint8_t  nblfos    = 0; 
            uint8_t  padbyte   = 0; //character used for padding
            uint8_t  unk7      = 0;
            uint8_t  unk8      = 0;
            uint8_t  unk9      = 0;

            inline bool operator==( const PrgInfoHeader & other )const
            {
                return ( id       == other.id       && 
                         nbsplits == other.nbsplits && 
                         prgvol   == other.prgvol   && 
                         prgpan   == other.prgpan   && 
                         unk3     == other.unk3     && 
                         unkpoly  == other.unkpoly  && 
                         unk4     == other.unk4     && 
                         unk5     == other.unk5     &&  
                         nblfos   == other.nblfos   &&
                         padbyte  == other.padbyte  &&
                         unk7     == other.unk7     &&
                         unk8     == other.unk8     &&
                         unk9     == other.unk9     );
            }

            inline bool operator!=( const PrgInfoHeader & other )const
            {
                return !( operator==(other));
            }

            template<class _outit>
                _outit WriteToContainer( _outit itwriteto )const
            {
                itwriteto = utils::WriteIntToBytes( id,        itwriteto );
                itwriteto = utils::WriteIntToBytes( nbsplits,  itwriteto );
                itwriteto = utils::WriteIntToBytes( prgvol,    itwriteto );
                itwriteto = utils::WriteIntToBytes( prgpan,    itwriteto );
                itwriteto = utils::WriteIntToBytes( unk3,      itwriteto );
                itwriteto = utils::WriteIntToBytes( unkpoly,   itwriteto );
                itwriteto = utils::WriteIntToBytes( unk4,      itwriteto );
                itwriteto = utils::WriteIntToBytes( unk5,      itwriteto );
                itwriteto = utils::WriteIntToBytes( nblfos,    itwriteto );
                itwriteto = utils::WriteIntToBytes( padbyte,   itwriteto );
                itwriteto = utils::WriteIntToBytes( unk7,      itwriteto );
                itwriteto = utils::WriteIntToBytes( unk8,      itwriteto );
                itwriteto = utils::WriteIntToBytes( unk9,      itwriteto );
                return itwriteto;
            }


            template<class _init>
                _init ReadFromContainer( _init itReadfrom, _init itpastend )
            {
                itReadfrom = utils::ReadIntFromBytes( id,        itReadfrom, itpastend );
                itReadfrom = utils::ReadIntFromBytes( nbsplits,  itReadfrom, itpastend );
                itReadfrom = utils::ReadIntFromBytes( prgvol,    itReadfrom, itpastend );
                itReadfrom = utils::ReadIntFromBytes( prgpan,    itReadfrom, itpastend );
                itReadfrom = utils::ReadIntFromBytes( unk3,      itReadfrom, itpastend );
                itReadfrom = utils::ReadIntFromBytes( unkpoly,   itReadfrom, itpastend );
                itReadfrom = utils::ReadIntFromBytes( unk4,      itReadfrom, itpastend );
                itReadfrom = utils::ReadIntFromBytes( unk5,      itReadfrom, itpastend );
                itReadfrom = utils::ReadIntFromBytes( nblfos,    itReadfrom, itpastend );
                itReadfrom = utils::ReadIntFromBytes( padbyte,   itReadfrom, itpastend );
                itReadfrom = utils::ReadIntFromBytes( unk7,      itReadfrom, itpastend );
                itReadfrom = utils::ReadIntFromBytes( unk8,      itReadfrom, itpastend );
                itReadfrom = utils::ReadIntFromBytes( unk9,      itReadfrom, itpastend );
                return itReadfrom;
            }
        };

        //----------------------------
        //  ProgramInfo
        //----------------------------

        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = m_hdr.WriteToContainer(itwriteto);

            for( const auto & entry : m_lfotbl )
                itwriteto = entry.WriteToContainer(itwriteto);

            //16 bytes of padding
            itwriteto = std::fill_n( itwriteto, 16, m_hdr.padbyte );

            for( const auto & splitentry : m_splitstbl )
                itwriteto = SplitEntry_v415(splitentry).WriteToContainer(itwriteto);

            return itwriteto;
        }


        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itpastend )
        {
            itReadfrom = m_hdr.ReadFromContainer(itReadfrom, itpastend);

            m_lfotbl   .resize(m_hdr.nblfos);
            m_splitstbl.resize(m_hdr.nbsplits);

            for( auto & entry : m_lfotbl )
                itReadfrom = entry.ReadFromContainer(itReadfrom, itpastend);

            //16 bytes of padding
            itReadfrom = utils::advAsMuchAsPossible( itReadfrom, itpastend, 16 );

            for( auto & smpl : m_splitstbl )
                itReadfrom = smpl.ReadFromContainer(itReadfrom, itpastend );

            return itReadfrom;
        }


        //Operators:
        operator ProgramInfo()const
        {
            ProgramInfo prginf;
            prginf.id        = m_hdr.id;
            prginf.prgvol    = m_hdr.prgvol;
            prginf.prgpan    = m_hdr.prgpan;
            prginf.unkpoly   = m_hdr.unkpoly;
            prginf.unk4      = m_hdr.unk4;
            prginf.padbyte   = m_hdr.padbyte;

            //Just copy LFOs directly
            prginf.m_lfotbl = m_lfotbl;

            prginf.m_splitstbl.reserve(m_hdr.nbsplits);
            for( const auto & split : m_splitstbl )
                prginf.m_splitstbl.push_back(split); //implicit conversion operator is called

            return std::move(prginf);
        }

        ProgramInfo_v415& operator=( const ProgramInfo & prginf )
        {
            m_hdr.id        = prginf.id;
            m_hdr.nbsplits  = static_cast<uint16_t>(prginf.m_splitstbl.size());
            m_hdr.prgvol    = prginf.prgvol;
            m_hdr.prgpan    = prginf.prgpan;
            m_hdr.unk3      = 0;
            m_hdr.unkpoly   = prginf.unkpoly;
            m_hdr.unk4      = prginf.unk4;
            m_hdr.unk5      = 0;
            m_hdr.nblfos    = static_cast<uint8_t>(prginf.m_lfotbl.size()); //Should always be 4 though #FIXME
            m_hdr.padbyte   = prginf.padbyte;
            m_hdr.unk7      = 0;
            m_hdr.unk8      = 0;
            m_hdr.unk9      = 0;

            //Just copy LFOs directly
            m_lfotbl = prginf.m_lfotbl;

            m_splitstbl.reserve(m_hdr.nbsplits);
            for( const auto & split : prginf.m_splitstbl )
                m_splitstbl.push_back(split);
            return *this;
        }

        ProgramInfo_v415& operator=( ProgramInfo && prginf )
        {
            m_hdr.id        = prginf.id;
            m_hdr.nbsplits  = static_cast<uint16_t>(prginf.m_splitstbl.size());
            m_hdr.prgvol    = prginf.prgvol;
            m_hdr.prgpan    = prginf.prgpan;
            m_hdr.unk3      = 0;
            m_hdr.unkpoly   = prginf.unkpoly;
            m_hdr.unk4      = prginf.unk4;
            m_hdr.unk5      = 0;
            m_hdr.nblfos    = static_cast<uint8_t>(prginf.m_lfotbl.size()); //Should always be 4 though #FIXME
            m_hdr.padbyte   = prginf.padbyte;
            m_hdr.unk7      = 0;
            m_hdr.unk8      = 0;
            m_hdr.unk9      = 0;

            //Just copy LFOs directly
            m_lfotbl = std::move(prginf.m_lfotbl);

            m_splitstbl.reserve(m_hdr.nbsplits);
            for( const auto & split : prginf.m_splitstbl )
                m_splitstbl.push_back(split);
            return *this;
        }


        PrgInfoHeader                 m_hdr;
        std::vector<LFOTblEntry>      m_lfotbl;
        std::vector<SplitEntry_v415>  m_splitstbl;
        
    };



    /*---------------------------------------------------------------------
        SplitEntry_v402
            Data on a particular sample mapped to this instrument
    ---------------------------------------------------------------------*/
    struct SplitEntry_v402
    {
        static const uint32_t SIZE     = 48; //bytes
        static const uint8_t  DefUnk22 = 2;
        static const uint8_t  DefUnk37 = 1;
        static const uint8_t  DefUnk38 = 3;
        static const uint16_t DefUnk39 = 0xFF03;
        static const uint16_t DefUnk40 = 0xFFFF;
        static const uint8_t  DefRX    = 0xFF;


        static uint32_t size() { return SIZE; }

        uint16_t id;        //0x1
        uint8_t  unk11;     //0x2
        uint8_t  unk25;     //0x3
        int8_t   lowkey;    //0x4
        int8_t   hikey;     //0x5
        int8_t   lowkey2;   //0x6
        int8_t   hikey2;    //0x7
        int8_t   lovel;     //0x8
        int8_t   hivel;     //0x9
        int8_t   lovel2;    //0xA
        int8_t   hivel2;    //0xB
        uint32_t unk16;     //0xC
        uint8_t  unk17;     //0x10
        uint8_t  smplid;    //0x11
        int8_t   ftune;     //0x12
        int8_t   ctune;     //0x13
        int8_t   rootkey;   //0x14
        int8_t   ktps;      //0x15
        uint8_t  smplvol;   //0x16
        uint8_t  smplpan;   //0x17
        uint8_t  kgrpid;    //0x18
        uint8_t  unk22;     //0x19
        uint16_t unk23;     //0x1A
        uint32_t unk24;     //0x1C
        //The last 16 bytes are a perfect copy of the last 16 bytes of a wavi info block
        uint8_t  envon;     //0x20 
        uint8_t  envmult;   //0x21 //Multiplier for other envelope params
        uint8_t  unk37;     //0x22
        uint8_t  unk38;     //0x23
        uint16_t unk39;     //0x24
        int16_t  unk40;     //0x26
        int8_t   atkvol;    //0x28
        int8_t   attack;    //0x29
        int8_t   decay;     //0x2A
        int8_t   sustain;   //0x2B
        int8_t   hold;      //0x2C
        int8_t   decay2;    //0x2D
        int8_t   release;   //0x2E
        int8_t   rx;        //0x2F

        SplitEntry_v402()
        {
            id       = 0;
            unk11    = 0;
            unk25    = 0;
            lowkey   = 0;
            hikey    = 0;
            lowkey2  = 0;
            hikey2   = 0;
            lovel    = 0;
            hivel    = 0;
            lovel2   = 0;
            hivel2   = 0;
            unk16    = 0;
            unk17    = 0;
            smplid   = 0;
            ftune    = 0;
            ctune    = 0;
            rootkey  = 0;
            ktps     = 0;
            smplvol  = 0;
            smplpan  = 0;
            kgrpid   = 0;
            unk22    = 0;
            unk23    = 0;
            unk24    = 0;
            envon    = 0;
            envmult  = 0;
            unk37    = 0;
            unk38    = 0;
            unk39    = 0;
            unk40    = 0;
            atkvol   = 0;
            attack   = 0;
            decay    = 0;
            sustain  = 0;
            hold     = 0;
            decay2   = 0;
            release  = 0;
            rx       = 0;
        }

        SplitEntry_v402( const SplitEntry & sent )
        {
            (*this) = sent;
        }


        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( id,       itwriteto );
            itwriteto = utils::WriteIntToBytes( unk11,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk25,    itwriteto );
            itwriteto = utils::WriteIntToBytes( lowkey,   itwriteto );
            itwriteto = utils::WriteIntToBytes( hikey,    itwriteto );
            itwriteto = utils::WriteIntToBytes( lowkey2,  itwriteto );
            itwriteto = utils::WriteIntToBytes( hikey2,   itwriteto );
            itwriteto = utils::WriteIntToBytes( lovel,    itwriteto );
            itwriteto = utils::WriteIntToBytes( hivel,    itwriteto );
            itwriteto = utils::WriteIntToBytes( lovel2,   itwriteto );
            itwriteto = utils::WriteIntToBytes( hivel2,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unk16,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk17,    itwriteto );
            itwriteto = utils::WriteIntToBytes( smplid,   itwriteto );
            itwriteto = utils::WriteIntToBytes( ftune,    itwriteto );
            itwriteto = utils::WriteIntToBytes( ctune,    itwriteto );
            itwriteto = utils::WriteIntToBytes( rootkey,  itwriteto );
            itwriteto = utils::WriteIntToBytes( ktps,     itwriteto );
            itwriteto = utils::WriteIntToBytes( smplvol,  itwriteto );
            itwriteto = utils::WriteIntToBytes( smplpan,  itwriteto );
            itwriteto = utils::WriteIntToBytes( kgrpid,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unk22,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk23,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk24,    itwriteto );
            itwriteto = utils::WriteIntToBytes( envon,    itwriteto );
            itwriteto = utils::WriteIntToBytes( envmult,  itwriteto );
            itwriteto = utils::WriteIntToBytes( unk37,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk38,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk39,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk40,    itwriteto );
            itwriteto = utils::WriteIntToBytes( atkvol,   itwriteto );
            itwriteto = utils::WriteIntToBytes( attack,   itwriteto );
            itwriteto = utils::WriteIntToBytes( decay,    itwriteto );
            itwriteto = utils::WriteIntToBytes( sustain,  itwriteto );
            itwriteto = utils::WriteIntToBytes( hold,     itwriteto );
            itwriteto = utils::WriteIntToBytes( decay2,   itwriteto );
            itwriteto = utils::WriteIntToBytes( release,  itwriteto );
            itwriteto = utils::WriteIntToBytes( rx,       itwriteto );
            return itwriteto;
        }


        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itpastend )
        {
            itReadfrom = utils::ReadIntFromBytes( id,       itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk11,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk25,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( lowkey,   itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( hikey,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( lowkey2,  itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( hikey2,   itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( lovel,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( hivel,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( lovel2,   itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( hivel2,   itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk16,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk17,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( smplid,   itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( ftune,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( ctune,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( rootkey,  itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( ktps,     itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( smplvol,  itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( smplpan,  itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( kgrpid,   itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk22,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk23,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk24,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( envon,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( envmult,  itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk37,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk38,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk39,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( unk40,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( atkvol,   itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( attack,   itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( decay,    itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( sustain,  itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( hold,     itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( decay2,   itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( release,  itReadfrom, itpastend );
            itReadfrom = utils::ReadIntFromBytes( rx,       itReadfrom, itpastend );
            return itReadfrom;
        }

        /*
            Convert to Version independent Split Entry. AKA remove the useless junk.
        */
        operator SplitEntry()const
        {
            SplitEntry sent;
            sent.id        = id;
            sent.unk11     = unk11;
            sent.unk25     = unk25;
            sent.lowkey    = lowkey;
            sent.hikey     = hikey;
            sent.lowkey2   = lowkey2;
            sent.hikey2    = hikey2;
            sent.lovel     = lovel;
            sent.hivel     = hivel;
            sent.lovel2    = lovel2;
            sent.hivel2    = hivel2;
            sent.smplid    = smplid;
            sent.ftune     = ftune;
            sent.ctune     = ctune;
            sent.rootkey   = rootkey;
            sent.ktps      = ktps;
            sent.smplvol   = smplvol;
            sent.smplpan   = smplpan;
            sent.kgrpid    = kgrpid;
            sent.envon     = envon;

            sent.env.envmulti = envmult;
            sent.env.atkvol   = atkvol;
            sent.env.attack   = attack;
            sent.env.decay    = decay;
            sent.env.sustain  = sustain;
            sent.env.hold     = hold;
            sent.env.decay2   = decay2;
            sent.env.release  = release;
            return std::move(sent);
        }

        SplitEntry_v402 & operator=( const SplitEntry & sent )
        {
            id       = sent.id;
            unk11    = sent.unk11;
            unk25    = sent.unk25;
            lowkey   = sent.lowkey;
            hikey    = sent.hikey;
            lowkey2  = sent.lowkey2;
            hikey2   = sent.hikey2;
            lovel    = sent.lovel;
            hivel    = sent.hivel;
            lovel2   = sent.lovel2;
            hivel2   = sent.hivel2;
            unk16    = 0; //Should be the padding byte, but honestly, those are probably just skipped
            unk17    = 0; //Should be the padding byte, but honestly, those are probably just skipped
            smplid   = sent.smplid;
            ftune    = sent.ftune;
            ctune    = sent.ctune;
            rootkey  = sent.rootkey;
            ktps     = sent.ktps;
            smplvol  = sent.smplvol;
            smplpan  = sent.smplpan;
            kgrpid   = sent.kgrpid;
            unk22    = DefUnk22;
            unk23    = 0;
            unk24    = 0; //Should be the padding byte, but honestly, those are probably just skipped
            envon    = sent.envon;
            envmult  = sent.env.envmulti;
            unk37    = DefUnk37;
            unk38    = DefUnk38;
            unk39    = DefUnk39;
            unk40    = DefUnk40;
            atkvol   = sent.env.atkvol;
            attack   = sent.env.attack;
            decay    = sent.env.decay;
            sustain  = sent.env.sustain;
            hold     = sent.env.hold;
            decay2   = sent.env.decay2;
            release  = sent.env.release;
            rx       = DefRX;
            return *this;
        }
    };


    /*****************************************************************************************
        ProgramInfo_v402
            Contains data for a single instrument.
    *****************************************************************************************/
    class ProgramInfo_v402
    {
    public:

        ProgramInfo_v402()
        {}

        ProgramInfo_v402( ProgramInfo && prginf )
        {
            this->operator=(std::move(prginf));
        }

        ProgramInfo_v402( const ProgramInfo & prginf )
        {
            this->operator=(prginf);
        }


        /*---------------------------------------------------------------------
            InstInfoHeader
                First 16 bytes of an instrument info block
        ---------------------------------------------------------------------*/
        struct PrgInfoHeader
        {
            static const uint32_t SIZE    = 16; //bytes
            static const uint16_t DefUnk4 = 0x200;

            static uint32_t size() { return SIZE; }

            uint8_t  id        = 0;
            uint8_t  nbsplits  = 0;
            uint16_t unk99     = 0;
            uint8_t  prgvol    = 0;
            uint8_t  prgpan    = 0;
            uint8_t  unk3      = 0;
            uint8_t  unkpoly   = 0;
            uint16_t unk4      = 0;
            uint8_t  unk5      = 0;
            uint8_t  nblfos    = 0; 
            uint8_t  padbyte   = 0;
            uint8_t  unk7      = 0;
            uint8_t  unk8      = 0;
            uint8_t  unk9      = 0;

            template<class _outit>
                _outit WriteToContainer( _outit itwriteto )const
            {
                itwriteto = utils::WriteIntToBytes( id,        itwriteto );
                itwriteto = utils::WriteIntToBytes( nbsplits,  itwriteto );
                itwriteto = utils::WriteIntToBytes( unk99,     itwriteto );
                itwriteto = utils::WriteIntToBytes( prgvol,    itwriteto );
                itwriteto = utils::WriteIntToBytes( prgpan,    itwriteto );
                itwriteto = utils::WriteIntToBytes( unk3,      itwriteto );
                itwriteto = utils::WriteIntToBytes( unkpoly,   itwriteto );
                itwriteto = utils::WriteIntToBytes( unk4,      itwriteto );
                itwriteto = utils::WriteIntToBytes( unk5,      itwriteto );
                itwriteto = utils::WriteIntToBytes( nblfos,    itwriteto );
                itwriteto = utils::WriteIntToBytes( padbyte,   itwriteto );
                itwriteto = utils::WriteIntToBytes( unk7,      itwriteto );
                itwriteto = utils::WriteIntToBytes( unk8,      itwriteto );
                itwriteto = utils::WriteIntToBytes( unk9,      itwriteto );
                return itwriteto;
            }


            template<class _init>
                _init ReadFromContainer( _init itReadfrom, _init itpastend )
            {
                itReadfrom = utils::ReadIntFromBytes( id,        itReadfrom, itpastend );
                itReadfrom = utils::ReadIntFromBytes( nbsplits,  itReadfrom, itpastend );
                itReadfrom = utils::ReadIntFromBytes( unk99,     itReadfrom, itpastend );
                itReadfrom = utils::ReadIntFromBytes( prgvol,    itReadfrom, itpastend );
                itReadfrom = utils::ReadIntFromBytes( prgpan,    itReadfrom, itpastend );
                itReadfrom = utils::ReadIntFromBytes( unk3,      itReadfrom, itpastend );
                itReadfrom = utils::ReadIntFromBytes( unkpoly,   itReadfrom, itpastend );
                itReadfrom = utils::ReadIntFromBytes( unk4,      itReadfrom, itpastend );
                itReadfrom = utils::ReadIntFromBytes( unk5,      itReadfrom, itpastend );
                itReadfrom = utils::ReadIntFromBytes( nblfos,    itReadfrom, itpastend );
                itReadfrom = utils::ReadIntFromBytes( padbyte,   itReadfrom, itpastend );
                itReadfrom = utils::ReadIntFromBytes( unk7,      itReadfrom, itpastend );
                itReadfrom = utils::ReadIntFromBytes( unk8,      itReadfrom, itpastend );
                itReadfrom = utils::ReadIntFromBytes( unk9,      itReadfrom, itpastend );
                return itReadfrom;
            }
        };

        //----------------------------
        //  ProgramInfo
        //----------------------------

        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = m_hdr.WriteToContainer(itwriteto);

            //Write keygroup table?
            for( const auto & entry : m_kgrp2tbl )
                itwriteto = entry.WriteToContainer(itwriteto);

            //Write LFO
            for( const auto & entry : m_lfotbl )
                itwriteto = entry.WriteToContainer(itwriteto);

            //Write Splits
            //for( const auto & smpl : m_splitstbl )
            //    itwriteto = smpl.WriteToContainer(itwriteto);

            for( const auto & splitentry : m_splitstbl )
                itwriteto = SplitEntry_v402(splitentry).WriteToContainer(itwriteto);

            return itwriteto;
        }


        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itpastend )
        {
            itReadfrom = m_hdr.ReadFromContainer(itReadfrom, itpastend);

            m_lfotbl   .resize(m_hdr.nblfos);
            m_splitstbl.resize(m_hdr.nbsplits);

            //Read keygroup table?
            for( auto & entry : m_kgrp2tbl )
                itReadfrom = entry.ReadFromContainer(itReadfrom, itpastend);

            //Read lfo table
            for( auto & entry : m_lfotbl )
                itReadfrom = entry.ReadFromContainer(itReadfrom, itpastend);

            //Read splits table
            for( auto & smpl : m_splitstbl )
                itReadfrom = smpl.ReadFromContainer(itReadfrom, itpastend);

            return itReadfrom;
        }


        //Operators:
        operator ProgramInfo()const
        {
            ProgramInfo prginf;
            prginf.id        = m_hdr.id;
            prginf.prgvol    = m_hdr.prgvol;
            prginf.prgpan    = m_hdr.prgpan;
            prginf.unkpoly   = m_hdr.unkpoly;
            prginf.unk4      = m_hdr.unk4;
            prginf.padbyte   = m_hdr.padbyte;

            //Ignore kgrp table for now 
            //!#FIXME

            //Just copy LFOs directly
            prginf.m_lfotbl = m_lfotbl;

            prginf.m_splitstbl.reserve(m_hdr.nbsplits);
            for( const auto & split : m_splitstbl )
                prginf.m_splitstbl.push_back(split); //implicit conversion operator is called

            return std::move(prginf);
        }

        ProgramInfo_v402 & operator=( const ProgramInfo & prginf )
        {
            m_hdr.id        = prginf.id;
            m_hdr.nbsplits  = static_cast<uint16_t>(prginf.m_splitstbl.size());
            m_hdr.prgvol    = prginf.prgvol;
            m_hdr.prgpan    = prginf.prgpan;
            m_hdr.unk3      = 0;
            m_hdr.unkpoly   = prginf.unkpoly;
            m_hdr.unk4      = prginf.unk4;
            m_hdr.unk5      = 0;
            m_hdr.nblfos    = static_cast<uint8_t>(prginf.m_lfotbl.size()); //!Should always be 4 though #FIXME
            m_hdr.padbyte   = prginf.padbyte;
            m_hdr.unk7      = 0;
            m_hdr.unk8      = 0;
            m_hdr.unk9      = 0;

            //Build a default kgrp table #FIXME
            for( auto & kgrp : m_kgrp2tbl )
            {
                kgrp.id       = 0;
                kgrp.poly     = KeyGroup::DefPoly;
                kgrp.priority = KeyGroup::DefPrio;
                kgrp.vclow    = 0;
                kgrp.vchigh   = KeyGroup::DefVcHi;
                kgrp.unk50    = 0;
                kgrp.unk51    = 0;
            }

            //Just copy LFOs directly
            m_lfotbl = prginf.m_lfotbl;

            m_splitstbl.reserve(m_hdr.nbsplits);
            for( const auto & split : prginf.m_splitstbl )
                m_splitstbl.push_back(split);

            return *this;
        }

        ProgramInfo_v402 & operator=( ProgramInfo && prginf )
        {
            m_hdr.id        = prginf.id;
            m_hdr.nbsplits  = static_cast<uint16_t>(prginf.m_splitstbl.size());
            m_hdr.prgvol    = prginf.prgvol;
            m_hdr.prgpan    = prginf.prgpan;
            m_hdr.unk3      = 0;
            m_hdr.unkpoly   = prginf.unkpoly;
            m_hdr.unk4      = prginf.unk4;
            m_hdr.unk5      = 0;
            m_hdr.nblfos    = static_cast<uint8_t>(prginf.m_lfotbl.size()); //!Should always be 4 though #FIXME
            m_hdr.padbyte   = prginf.padbyte;
            m_hdr.unk7      = 0;
            m_hdr.unk8      = 0;
            m_hdr.unk9      = 0;

            //!Build a default kgrp table #FIXME
            for( auto & kgrp : m_kgrp2tbl )
            {
                kgrp.id       = 0;
                kgrp.poly     = KeyGroup::DefPoly;
                kgrp.priority = KeyGroup::DefPrio;
                kgrp.vclow    = 0;
                kgrp.vchigh   = KeyGroup::DefVcHi;
                kgrp.unk50    = 0;
                kgrp.unk51    = 0;
            }

            //Just copy LFOs directly
            m_lfotbl = std::move( prginf.m_lfotbl );

            m_splitstbl.reserve(m_hdr.nbsplits);
            for( const auto & split : prginf.m_splitstbl )
                m_splitstbl.push_back(split);

            return *this;
        }

        PrgInfoHeader                 m_hdr;
        std::vector<LFOTblEntry>      m_lfotbl;
        std::vector<SplitEntry_v402>  m_splitstbl;
        std::array<KeyGroup,16>       m_kgrp2tbl;
    };




//===============================================================================
// SWDLParser
//===============================================================================

    template<class _rait = std::vector<uint8_t>::const_iterator >
        class SWDLParser
    {
    public:
        typedef _rait rd_iterator_t;

        SWDLParser( _rait itbeg, _rait itend )
            :m_itbeg(itbeg),m_itend(itend)
        {}

        SWDLParser( const std::vector<uint8_t> & src )
            :m_itbeg(src.begin()),m_itend(src.end())
        {}

        PresetBank Parse()
        {
            ParseHeader();
            ParseMeta();

            //Version check
            if( m_hdr.version == DseVerToInt(eDSEVersion::V415) )
            {
                //Parse programs + keygroups
                auto pinst = ParsePrograms<ProgramInfo_v415>();

                //Parse pcmd + wavi
                if( m_hdr.pcmdlen != 0 && (m_hdr.pcmdlen & 0xFFFF0000) != SWDL_PCMDSpecialSize )
                {
                    //Grab the info on every samples
                    vector<SampleBank::smpldata_t> smpldat(std::move( ParseWaviChunk<WavInfo_v415>() ));
                    auto psmpls = ParseSamples(smpldat);
                    return std::move( PresetBank( move(m_meta), 
                                      move(pinst), 
                                      move(psmpls) ) );
                }
                else
                    return std::move( PresetBank( move(m_meta), 
                                      move(pinst) ) );
            }
            else if( m_hdr.version == DseVerToInt(eDSEVersion::V402) )
            {
                auto pinst  = ParsePrograms<ProgramInfo_v402>();
                //Grab the info on every samples
                vector<SampleBank::smpldata_t> smpldat(std::move( ParseWaviChunk<WavInfo_v402>() ));
                auto psmpls = ParseSamples(smpldat);
                return std::move( PresetBank( move(m_meta), 
                                  move(pinst), 
                                  move(psmpls) ) );
            }
            else
            {
                stringstream sstr;
                sstr << "SWDLParser::Parse() : Unsuported DSE version " << hex <<"0x" <<m_hdr.version;
                throw runtime_error( sstr.str() );
            }
        }

    private:
        void ParseHeader()
        {
            m_hdr.ReadFromContainer(m_itbeg);

            if( utils::LibWide().isLogOn() )
                clog << "\tDSE Version: 0x" <<hex <<uppercase <<m_hdr.version <<dec <<nouppercase <<"\n";
            if( utils::LibWide().isVerboseOn() )
            {
                clog <<"#TODO verbose\n";
            }
        }

        void ParseMeta()
        {
            m_meta.fname = string( m_hdr.fname.data() );
            m_meta.unk1               = m_hdr.unk1;
            m_meta.unk2               = m_hdr.unk2;
            m_meta.createtime.year    = m_hdr.year;
            m_meta.createtime.month   = m_hdr.month;
            m_meta.createtime.day     = m_hdr.day;
            m_meta.createtime.hour    = m_hdr.hour;
            m_meta.createtime.minute  = m_hdr.minute;
            m_meta.createtime.second  = m_hdr.second;
            m_meta.createtime.centsec = m_hdr.centisec;
            m_meta.nbprgislots        = m_hdr.nbprgislots;
            m_meta.nbwavislots        = m_hdr.nbwavislots;
            m_meta.unk17              = m_hdr.unk17;

            if( intToDseVer(m_hdr.version) == eDSEVersion::VInvalid )
                throw runtime_error( "SWDLParser::ParseMeta() : Invalid/Unknown DSE version!" );
            else
                m_meta.origversion = intToDseVer(m_hdr.version);
        }

        template<class _PrgInfoTy>
            std::unique_ptr<ProgramBank> ParsePrograms()
        {
            //using namespace pmd2::audio;

            //Handle keygroups first
            auto kgrps = ParseKeygroups();

            if( utils::LibWide().isLogOn() )
                clog <<"\t== Parsing Programs ==\n";

            //Find the prgi chunk
            auto itprgi = DSE::FindNextChunk( m_itbeg, m_itend, eDSEChunks::prgi );

            //Its possible there are no programs
            if( itprgi == m_itend )
            {
                if( utils::LibWide().isLogOn() )
                    clog <<"\t\tNo Programs found!\n";
                return nullptr;
            }

            //Read chunk header
            ChunkHeader prgihdr;
            itprgi = prgihdr.ReadFromContainer( itprgi, m_itend ); //Move iter after header

            //Read instrument info slots
            vector<ProgramBank::ptrprg_t> prginf( m_hdr.nbprgislots );

            auto itreadprg = itprgi;

            for( auto & infslot : prginf )
            {
                //Read a ptr
                uint16_t prginfblk = utils::ReadIntFromBytes<uint16_t>(itreadprg); //Iterator is incremented

                if( prginfblk != 0 )
                {
                    _PrgInfoTy curblock;
                    curblock.ReadFromContainer( prginfblk + itprgi, m_itend );
                    infslot.reset( new ProgramInfo(curblock) );

                    if( utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn() )
                        clog <<"\tInstrument ID#" <<infslot->id <<":\n" <<*infslot <<"\n";
                }
            }
            if( utils::LibWide().isLogOn() )
                clog << endl;
            return move( unique_ptr<ProgramBank>( new ProgramBank( move(prginf), move(kgrps) ) ) );
        }

        vector<KeyGroup> ParseKeygroups()
        {
            //using namespace pmd2::audio;

            if( utils::LibWide().isLogOn() )
                clog <<"\t== Parsing Keygroups ==\n";

            //Find the KGRP chunk
            auto itkgrp = DSE::FindNextChunk( m_itbeg, m_itend, eDSEChunks::kgrp );

            if( itkgrp == m_itend )
            {
                if( utils::LibWide().isLogOn() )
                    clog <<"\t\tNo Keygroups found!\n";
                return vector<KeyGroup>(); //Return empty vector when no keygrp chunk found
            }

            //Get the kgrp chunk's header
            ChunkHeader kgrphdr;
            itkgrp = kgrphdr.ReadFromContainer( itkgrp, m_itend ); //Move iter after header
            
            vector<KeyGroup> keygroups(kgrphdr.datlen / KeyGroup::size());
            
            //Read all keygroups
            for( auto & grp : keygroups )
            {
                itkgrp = grp.ReadFromContainer(itkgrp, m_itend);

                if( utils::LibWide().isLogOn() && utils::LibWide().isVerboseOn() )
                    clog <<"\tKeygroup ID#" <<grp.id <<":\n" <<grp <<"\n";
                
            }

            if( utils::LibWide().isLogOn() )
                clog << endl;

            return move(keygroups);
        }

        std::unique_ptr<SampleBank> ParseSamples( vector<SampleBank::smpldata_t> & smpldat )
        {
            //Find the PCMD chunk
            auto itpcmd = DSE::FindNextChunk( m_itbeg, m_itend, eDSEChunks::pcmd );
            
            if( itpcmd == m_itend )
                throw std::runtime_error("SWDLParser::ParseSamples(): Couldn't find PCMD chunk!!!!!");

            //Get the pcmd chunk's header
            ChunkHeader pcmdhdr;
            itpcmd = pcmdhdr.ReadFromContainer( itpcmd, m_itend ); //Move iter after header

            //Grab the samples
            for( size_t cntsmpl = 0; cntsmpl < smpldat.size(); ++cntsmpl )
            {
                const auto & psinfo = smpldat[cntsmpl].pinfo_;
                if( psinfo != nullptr )
                {
                    auto   itsmplbeg = itpcmd + psinfo->smplpos;
                    size_t smpllen   = DSESampleLoopOffsetToBytes( psinfo->loopbeg + psinfo->looplen );
                    smpldat[cntsmpl].pdata_.reset( new vector<uint8_t>( itsmplbeg, 
                                                                        itsmplbeg + smpllen ) );
                }
            }

            return std::unique_ptr<SampleBank>( new SampleBank( move(smpldat) ) );
        }

        template<class _WaviEntryFmt>
            vector<SampleBank::smpldata_t> ParseWaviChunk()
        {
            auto itwavi = DSE::FindNextChunk( m_itbeg, m_itend, eDSEChunks::wavi );

            if( itwavi == m_itend )
                throw std::runtime_error("SWDLParser::ParseWaviChunk(): Couldn't find wavi chunk !!!!!");

            ChunkHeader wavihdr;
            itwavi = wavihdr.ReadFromContainer( itwavi, m_itend ); //Move iterator past the header

            //Create the vector with the nb of slots mentioned in the header
            vector<SampleBank::smpldata_t> waviptrs( m_hdr.nbwavislots );
            
            auto itreadptr = itwavi; //Copy the iterator to keep one on the start of the wavi data

            for( auto & ablock : waviptrs )
            {
                //Read a ptr
                uint16_t smplinfoffset = utils::ReadIntFromBytes<uint16_t>(itreadptr); //Iterator is incremented

                if( smplinfoffset != 0 )
                {
                    _WaviEntryFmt winf;
                    winf.ReadFromContainer( smplinfoffset + itwavi, m_itend );
                    ablock.pinfo_.reset( new WavInfo(winf) );
                }
            }

            return move(waviptrs);
        }

    private:
        DSE_MetaDataSWDL             m_meta;
        SWDL_Header                  m_hdr;

        rd_iterator_t                m_itbeg;
        rd_iterator_t                m_itend;

        //const std::vector<uint8_t> & m_src;
    };

//====================================================================================================
// SMDL_Writer
//====================================================================================================

    template<class _TargetTy>
        class SWDL_Writer;


    template<>
        class SWDL_Writer<std::ofstream>
    {
        typedef ostreambuf_iterator<char> writeit_t;
    public:
        typedef std::ofstream cnty;

        /*
            - pcmdflag : The value of the lowest 16 bits of the pcmdlen value when there is no pcmd chunk. Used in some games.
        */
        SWDL_Writer( cnty & tgtcnt, const DSE::PresetBank & srcbnk, uint16_t pcmdflag = 0x0000, uint8_t padbyte = 0xAA, eDSEVersion dseVersion = eDSEVersion::VDef )
            :m_tgtcn(tgtcnt), m_src(srcbnk), m_pcmdflag(pcmdflag), m_padbyte(padbyte), m_version(dseVersion)
        {}

        void operator()()
        {
            streampos befswdl = m_tgtcn.tellp();

            writeit_t                  itout(m_tgtcn);
            std::vector<uint32_t>      sampleOffsets; //This is used to store the offset to each samples in the PCMD chunk!
            auto                       ptrsbnk = m_src.smplbank().lock();
            bool                       hassmpldata  = false;
            if( ptrsbnk == nullptr )
                throw runtime_error("SWDL_Writer::WritePCMD() : SWDL has no sample info or data!");

            hassmpldata = !std::all_of( ptrsbnk->begin(), 
                                        ptrsbnk->end(), 
                                        []( const SampleBank::smpldata_t & entry )->bool 
                                        { return entry.pdata_ == nullptr; } );

            sampleOffsets.resize( ptrsbnk->NbSlots(), 0 );

            //Reserve Header
            itout = std::fill_n( itout, SWDL_Header::Size, 0 );

            //Reserve Wavi chunk
            streampos beforewavi = m_tgtcn.tellp();
            itout = std::fill_n( itout, SWDL_Header::Size, 0 );

            //Write prgi chunk
            WritePrgi(itout);

            //Write kgrp
            WriteKgrp(itout);

            //Write PCMD
            size_t pcmdlen = 0;
            if(hassmpldata)
                pcmdlen = WritePCMD( itout, sampleOffsets, *ptrsbnk );
            else
            {
                //If we end up here, we need to come up with the offset values for each samples. 
                //Its not clear how those are calculated. But it seems we just add up the length of each samples we're using 
                // in the wavitable in the same order.
                BuildSampleOffsetsNoPCMDChnk( sampleOffsets, *ptrsbnk );
            }

            //Write EoD
            WriteEod(itout);
            size_t flen = m_tgtcn.tellp();

            //Write wavi chunk
            m_tgtcn.seekp(beforewavi);
            size_t wavilen = WriteWavi( sampleOffsets, *ptrsbnk );

            //Finish and write header
            streampos aftswdl = m_tgtcn.tellp();
            m_tgtcn.seekp(befswdl); 
            WriteHeader( itout, flen, pcmdlen, wavilen );
            m_tgtcn.seekp(aftswdl);
        }

    private:

        inline void WriteChunkHeader( writeit_t & itout, uint32_t label, uint32_t size )
        {
            ChunkHeader hdr;
            hdr.label  = label;
            hdr.param1 = static_cast<uint16_t>(m_version) << 16; //The DSE version is in every chunk's header
            hdr.param2 = SWDL_ChunksDefParam2;
            hdr.datlen = size;
            hdr.WriteToContainer(itout);
        }

        void WriteHeader( writeit_t & itout, size_t filelen, size_t pcmdatalen, size_t wavilen )
        {
            SWDL_Header hdr;
            hdr.unk18    = 0; //Always null
            hdr.flen     = filelen;
            hdr.version  = SWDL_Header::DefVersion; 
            hdr.unk1     = m_src.metadata().unk1;
            hdr.unk2     = m_src.metadata().unk2;
            hdr.unk3     = 0; //Always null
            hdr.unk4     = 0; //Always null
            hdr.year     = m_src.metadata().createtime.year;
            hdr.month    = m_src.metadata().createtime.month;
            hdr.day      = m_src.metadata().createtime.day;
            hdr.hour     = m_src.metadata().createtime.hour;
            hdr.minute   = m_src.metadata().createtime.minute;
            hdr.second   = m_src.metadata().createtime.second;
            hdr.centisec = m_src.metadata().createtime.centsec;
            std::copy( begin(m_src.metadata().fname), end(m_src.metadata().fname), begin(hdr.fname) );
            hdr.unk10    = SWDL_Header::DefUnk10;
            hdr.unk11    = 0; //Always null
            hdr.unk12    = 0; //Always null
            hdr.unk13    = SWDL_Header::DefUnk13; 
            
            if( pcmdatalen != 0 )
                hdr.pcmdlen = pcmdatalen;
            else
                hdr.pcmdlen = (SWDL_Header::ValNoPCMD | m_pcmdflag);

            hdr.unk14    = 0; //Always null

            auto ptrsmplbnk = m_src.smplbank().lock();
            if( ptrsmplbnk == nullptr )
                throw runtime_error( "SWDL_Writer::WritePCMD() : SWDL has no sample info or data!" );
            hdr.nbwavislots = ptrsmplbnk->NbSlots();

            auto ptrpresbnk = m_src.prgmbank().lock();
            if( ptrpresbnk != nullptr )
                hdr.nbprgislots = ptrpresbnk->PrgmInfo().size();
            else
                hdr.nbprgislots = 0;

            hdr.unk17   = m_src.metadata().unk17;
            hdr.wavilen = wavilen;
            hdr.WriteToContainer(itout);
        }

        size_t WriteWavi( const std::vector<uint32_t> & sampleoffsets, const DSE::SampleBank & smplbank )
        {
            //auto ptrsbnk = m_src.smplbank().lock();
            //if( ptrsbnk == nullptr )
            //    throw runtime_error("SWDL_Writer::WriteWavi() : SWDL has no sample info or data!");

            streampos   befhdr = m_tgtcn.tellp();
            writeit_t   itout(m_tgtcn);
            //ChunkHeader wavihdr;

            //Reserve chunk header
            std::fill_n( itout, ChunkHeader::Size, 0 );
            
            //Reserve Table
            streampos beftbl = m_tgtcn.tellp();
            std::fill_n( itout, smplbank.NbSlots() * 2, 0 ); //Each slots is 16 bits
            
            //Place table padding 
            int padlen =  (m_tgtcn.tellp() % 16);
            if( padlen != 0 )
                std::fill_n( itout, padlen, m_padbyte );

            //Write down wavi entries, and add them to the table!
            const size_t nbslots = smplbank.NbSlots();
            for( size_t i = 0; i < nbslots; ++i )
            {
                const WavInfo * ptrentry = smplbank.sampleInfo(i);
                if( ptrentry != nullptr)
                    WriteWaviEntry( *ptrentry, itout, beftbl, i, sampleoffsets[i] );
            }

            //Go back to the beginning and write header!
            streamoff endchunk = m_tgtcn.tellp();
            size_t    chunklen = (m_tgtcn.tellp() - beftbl);
            m_tgtcn.seekp(befhdr);

            WriteChunkHeader( itout, 
                              static_cast<uint32_t>(eDSEChunks::wavi), 
                              chunklen );
            //wavihdr.label  = static_cast<uint32_t>(eDSEChunks::wavi);
            //wavihdr.datlen = chunklen;
            //wavihdr.param1 = SWDL_ChunksDefParam1;
            //wavihdr.param2 = SWDL_ChunksDefParam2;
            //wavihdr.WriteToContainer(itout);

            //Seek to end of wavi chunk
            m_tgtcn.seekp(endchunk);

            return chunklen;
        }

        void WriteWaviEntry( DSE::WavInfo wavientry, writeit_t & itout, std::streampos beftbl, size_t entryindex, size_t pcmdsmploffset )
        {
            streampos entryoffset = m_tgtcn.tellp();
            wavientry.smplpos = pcmdsmploffset; //Set the pcmd chunk relative sample offset

            if( m_version == eDSEVersion::V415 )
            {
                WavInfo_v415 vdwavinf = wavientry;

                //Fill in missing data
                assert(false);

                //write
                itout = vdwavinf.WriteToContainer(itout);
            }
            else if( m_version == eDSEVersion::V402 )
            {
                WavInfo_v402 vdwavinf( wavientry);

                //Fill in missing data
                assert(false);

                //write
                itout = vdwavinf.WriteToContainer(itout);
            }


            streampos afterentry = m_tgtcn.tellp();
            int offfromtbl = (entryoffset - beftbl);
            if( offfromtbl > std::numeric_limits<uint16_t>::max() )
                throw overflow_error("SWDL_Writer::WriteWavi(): Couldn't add offset to table. Offset overflows a int16!");
                    
            //Seek back to the pointer table, to write the offset
            std::streampos offsetptr = beftbl + std::streampos(entryindex * 2);
            m_tgtcn.seekp(offsetptr);
            utils::WriteIntToBytes( static_cast<uint16_t>(offfromtbl), itout );

            //Go back to after the entry
            m_tgtcn.seekp(afterentry);
        }

        void WritePrgi( writeit_t & itout )
        {
            auto ptrpresbnk = m_src.prgmbank().lock();
            if( ptrpresbnk == nullptr )
                return;
            const auto & prgbnk = ptrpresbnk->PrgmInfo();

            streampos befprgi = m_tgtcn.tellp();
            //Reserve header
            std::fill_n( itout, ChunkHeader::Size, 0 );
            streampos begtbl = m_tgtcn.tellp();

            //Reserve pointer table
            streampos beftbl = m_tgtcn.tellp();
            std::fill_n( itout, prgbnk.size() * 2, 0 ); //Each slots is 16 bits
            
            //Place table padding 
            int padlen =  (m_tgtcn.tellp() % 16);
            if( padlen != 0 )
                std::fill_n( itout, padlen, m_padbyte );

            //Write Entries
            for( size_t i = 0; i < prgbnk.size(); ++i )
            {
                const DSE::ProgramInfo * ptrentry = prgbnk[i].get();
                if( ptrentry != nullptr)
                    WritePrgiEntry( itout, *ptrentry, beftbl, i );
            }

            streampos endprgi = m_tgtcn.tellp();
            m_tgtcn.seekp(befprgi);

            //Write header
            WriteChunkHeader( itout, 
                              static_cast<uint32_t>(eDSEChunks::prgi), 
                              endprgi.seekpos() );
            //ChunkHeader hdr;
            //hdr.label  = static_cast<uint32_t>(eDSEChunks::prgi);
            //hdr.param1 = SWDL_ChunksDefParam1;
            //hdr.param2 = SWDL_ChunksDefParam2;
            //hdr.datlen = endprgi.seekpos();
            //hdr.WriteToContainer(itout);
            m_tgtcn.seekp(endprgi);
        }

        void WritePrgiEntry( writeit_t & itout, const DSE::ProgramInfo & entry, streampos beftbl, size_t entryindex )
        {
            streampos befentry = m_tgtcn.tellp();

            if( m_version == eDSEVersion::V415 )
            {
                ProgramInfo_v415 prginf(entry);

                //!#TODO: Fill in missing data
                assert(false);

                //write
                itout = prginf.WriteToContainer(itout);
            }
            else if( m_version == eDSEVersion::V402 )
            {
                ProgramInfo_v402 prginf(entry);

                //!#TODO: Fill in missing data
                assert(false);

                //write
                itout = prginf.WriteToContainer(itout);
            }


            //entry.WriteToContainer( itout );

            long long offsbegtbl = befentry.seekpos() - beftbl.seekpos();
            if( offsbegtbl > std::numeric_limits<uint16_t>::max() )
                throw overflow_error("SWDL_Writer::WritePrgiEntry(): Couldn't add offset of entry#" + to_string(entryindex) + " to table. Offset overflows a int16!");

            //Go write pointer in the table
            streampos afentry = m_tgtcn.tellp();
            m_tgtcn.seekp( beftbl.seekpos() + offsbegtbl );
            utils::WriteIntToBytes( static_cast<uint16_t>(offsbegtbl), itout );

            //Seek back to end
            m_tgtcn.seekp(afentry);
        }

        //Return pcm data length
        fpos_t WritePCMD( writeit_t & itout, std::vector<uint32_t> & sampleoffsets, const DSE::SampleBank & smplbank )
        {
            //auto ptrsbnk = m_src.smplbank().lock();
            //if( ptrsbnk == nullptr )
            //    throw runtime_error("SWDL_Writer::WritePCMD() : SWDL has no sample info or data!");

            streampos befhdr = m_tgtcn.tellp();
            //Reserve header
            itout = std::fill_n( itout, ChunkHeader::Size, 0 );
            streampos begdata = m_tgtcn.tellp();

            const size_t nbslots = smplbank.NbSlots();
            for( size_t i = 0; i < nbslots; ++i )
            {
                const vector<uint8_t> * ptrdata = smplbank.sample(i);
                if( ptrdata != nullptr )
                {
                    streampos befsmpl = m_tgtcn.tellp();
                    sampleoffsets[i]  = static_cast<uint32_t>(befsmpl.seekpos()); //Store sample offset!
                    itout = std::copy( ptrdata->begin(), ptrdata->end(), itout );
                }
            }

            //Seek to beginning and write the header!
            //ChunkHeader hdr;
            streampos   afterdata = m_tgtcn.tellp();
            m_tgtcn.seekp(befhdr);
            
            WriteChunkHeader( itout, 
                              static_cast<uint32_t>(eDSEChunks::pcmd), 
                              static_cast<uint32_t>(afterdata.seekpos()) );
            //hdr.label  = static_cast<uint32_t>(eDSEChunks::pcmd);
            //hdr.param1 = SWDL_ChunksDefParam1;
            //hdr.param2 = SWDL_ChunksDefParam2;
            //hdr.datlen = static_cast<uint32_t>(afterdata.seekpos());
            //itout = hdr.WriteToContainer(itout);

            //Seek back to end
            m_tgtcn.seekp(afterdata);

            return afterdata.seekpos();
        }

        void WriteKgrp( writeit_t & itout )
        {
            auto ptrpresbnk = m_src.prgmbank().lock();
            if( ptrpresbnk == nullptr )
                return;
            const DSE::KeyGroupList & kgrplist = ptrpresbnk->Keygrps();

            //Reserve header
            streampos befhdr = m_tgtcn.tellp();
            std::fill_n( itout, ChunkHeader::Size, 0 );

            //Write keygroups
            for( const auto & akgrp : kgrplist )
                akgrp.WriteToContainer(itout);

            uint32_t chunklen = static_cast<uint32_t>(m_tgtcn.tellp());

            //Add padding
            int padlen =  (m_tgtcn.tellp() % 16);
            if( padlen != 0 )
                std::fill_n( itout, padlen, m_padbyte );

            //Write header
            streampos aftkgrps = m_tgtcn.tellp();
            m_tgtcn.seekp(befhdr);
            WriteChunkHeader( itout, static_cast<uint32_t>(eDSEChunks::kgrp), chunklen );
            m_tgtcn.seekp(aftkgrps);
        }

        void WriteEod( writeit_t & itout )
        {
            //ChunkHeader eodhdr;
            //eodhdr.label  = static_cast<uint32_t>(eDSEChunks::eod);
            //eodhdr.datlen = 0;
            //eodhdr.param1 = SWDL_ChunksDefParam1;
            //eodhdr.param2 = SWDL_ChunksDefParam2;
            //eodhdr.WriteToContainer(itout);
            WriteChunkHeader( itout, 
                              static_cast<uint32_t>(eDSEChunks::eod), 
                              0 );
        }

        void BuildSampleOffsetsNoPCMDChnk( std::vector<uint32_t> & smploffsets, const DSE::SampleBank & smplbank )
        {
            //auto ptrnbk = m_src.smplbank().lock();

            //if( ptrnbk == nullptr )
            //    std::runtime_error( "SWDL_Writer::BuildSampleOffsetsNoPCMDChnk() : Sample bank is null!" );

            size_t currentoffset = 0; //We add up the size of each samples in there

            //Add up each valid samples
            for( size_t i = 0; i < smplbank.NbSlots(); ++i )
            {
                const DSE::WavInfo * ptrwaientry = smplbank.sampleInfo(i);
                if( ptrwaientry != nullptr )
                {
                    size_t smpllen = (ptrwaientry->loopbeg + ptrwaientry->looplen) * 4; //Multiply by 4 since the lengths are counted as int32.
                    smploffsets[i] = currentoffset;
                    currentoffset += smpllen;
                }
            }
        }

    private:
        cnty                  & m_tgtcn;
        const DSE::PresetBank & m_src;
        const uint16_t          m_pcmdflag;
        const uint8_t           m_padbyte;
        eDSEVersion             m_version;
    };

//========================================================================================================
//  Functions
//========================================================================================================
    PresetBank ParseSWDL( const std::string & filename )
    {
        if( utils::LibWide().isLogOn() )
        {
            clog <<"--------------------------------------------------------------------------\n"
                 <<"Parsing SWDL \"" <<filename <<"\"\n"
                 <<"--------------------------------------------------------------------------\n";
        }
        return std::move( SWDLParser<>( utils::io::ReadFileToByteVector( filename ) ).Parse() );
    }

    PresetBank ParseSWDL( std::vector<uint8_t>::const_iterator itbeg, 
                          std::vector<uint8_t>::const_iterator itend )
    {
        return std::move( SWDLParser<>( itbeg, itend ).Parse() );
    }


    SWDL_Header ReadSwdlHeader( const std::string & filename )
    {
        ifstream infile( filename, ios::in | ios::binary );
        istreambuf_iterator<char> init(infile);
        SWDL_Header hdr;
        hdr.ReadFromContainer(init);
        return move(hdr);
    }

    void WriteSWDL( const std::string & filename, const PresetBank & audiodata )
    {
        std::ofstream outf(filename, std::ios::out | std::ios::binary );

        if( !outf.is_open() || outf.bad() )
            throw std::runtime_error( "WriteSMDL(): Couldn't open output file " + filename );

        SWDL_Writer<std::ofstream>(outf, audiodata)();
    }

};

#ifdef USE_PPMDU_CONTENT_TYPE_ANALYSER
    #include <types/content_type_analyser.hpp>

    namespace filetypes
    {
        const ContentTy CnTy_SWDL{"swdl"}; //Content ID db handle

    //========================================================================================================
    //  swdl_rule
    //========================================================================================================
        /*
            swdl_rule
                Rule for identifying a SMDL file. With the ContentTypeHandler!
        */
        class swdl_rule : public IContentHandlingRule
        {
        public:
            swdl_rule(){}
            ~swdl_rule(){}

            //Returns the value from the content type enum to represent what this container contains!
            virtual cnt_t getContentType()const
            {
                return CnTy_SWDL;
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
                DSE::SWDL_Header headr;
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
                return (utils::ReadIntFromBytes<uint32_t>(itdatabeg,false) == DSE::SWDL_MagicNumber);
            }

        private:
            cntRID_t m_myID;
        };

    //========================================================================================================
    //  swdl_rule_registrator
    //========================================================================================================
        /*
            swdl_rule_registrator
                A small singleton that has for only task to register the swdl_rule!
        */
        RuleRegistrator<swdl_rule> RuleRegistrator<swdl_rule>::s_instance;
    };
#endif