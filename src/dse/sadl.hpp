#ifndef SADL_HPP
#define SADL_HPP
/*
sadl.hpp
2015/12/21
psycommando@gmail.com
Description:
    Tools for parsing SADL files.
*/
#include <dse/dse_common.hpp>
#include <dse/bgm_container.hpp>

#ifdef USE_PPMDU_CONTENT_TYPE_ANALYSER
    #include <types/content_type_analyser.hpp>
    namespace filetypes
    {
        extern const ContentTy CnTy_SADL; //Content ID db handle
    };
#endif

namespace DSE
{
    const uint32_t SADL_MagicNumber = static_cast<uint32_t>(eDSEContainers::sadl);


    struct SADL_Header
    {
        static const size_t  SIZE       = 256;
        static const size_t  FNameLen   = 16; 
        static const size_t  PaddingLen = 144;
        static const uint8_t PaddinByte = 0;
        inline size_t size()const { return SIZE; }

        //0x0 - 0xF
        uint32_t magicn         = 0;
        uint32_t unk1           = 0;
        uint32_t flen           = 0;
        uint16_t version        = 0;
        uint16_t unk3           = 0;

        //0x10 - 0x1F
        uint32_t unk4           = 0;
        uint32_t unk5           = 0;
        uint16_t year           = 0;
        uint8_t  month          = 0;
        uint8_t  day            = 0;
        uint8_t  hour           = 0;
        uint8_t  minute         = 0;
        uint8_t  second         = 0;
        uint8_t  centisec       = 0;

        //0x20 - 0x2F
        std::array<char,FNameLen> fname;

        //0x30 - 0x3F
        uint8_t  unk6           = 0;
        uint8_t  loopflag       = 0;
        uint8_t  nbchan         = 0;
        uint8_t  audiofmt       = 0; //High nybble is encoding(0xB0 == Procyon ADPCM, 0x70 == IMA ADPCM), lower 6 bits is sample rate( 0x2 == 16364hz, 0x4 == 32728hz ).
        uint8_t  unk7           = 0;
        uint8_t  unk8           = 0;
        uint8_t  unk9           = 0;
        uint8_t  unk10          = 0;
        uint16_t unk11          = 0;
        uint16_t unk12          = 0;
        uint16_t unk13          = 0;   
        uint16_t unk14          = 0;   
        
        //0x40 - 0x4F
        uint32_t unk15          = 0;
        uint32_t unk16          = 0;
        uint32_t unk17          = 0;
        uint32_t unk18          = 0;

        //0x50 - 0x5F
        uint32_t unk19          = 0;
        uint32_t loopstart      = 0;
        uint32_t unk20          = 0;
        uint32_t unk21          = 0;

        //0x60 - 0x6F
        int8_t   volume         = 0;
        int8_t   pan            = 0;
        int8_t   unk22          = 0;
        int8_t   unk23          = 0;
        uint32_t unk24          = 0;
        uint32_t unk25          = 0;
        uint32_t unk26          = 0;

        //0x70 - 0xFF
        //Padding bytes!

        template<class _outit>
            _outit WriteToContainer( _outit itwriteto, _outit itend )const
        {
            //0x0 - 0xF
            itwriteto = utils::WriteIntToBytes   ( SADL_MagicNumber, itwriteto, false ); //Write constant magic number, to avoid bad surprises
            itwriteto = utils::WriteIntToBytes   ( unk1,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( flen,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( version,          itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk3,             itwriteto );

            //0x10 - 0x1F
            itwriteto = utils::WriteIntToBytes   ( unk4,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk5,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( year,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( month,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( day,              itwriteto );
            itwriteto = utils::WriteIntToBytes   ( hour,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( minute,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( second,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( centisec,         itwriteto );

            //0x20 - 0x2F
            itwriteto = utils::WriteStrToByteContainer( itwriteto,   fname, fname.size() );

            //0x30 - 0x3F
            itwriteto = utils::WriteIntToBytes   ( unk6,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( loopflag,        itwriteto );
            itwriteto = utils::WriteIntToBytes   ( nbchan,          itwriteto );
            itwriteto = utils::WriteIntToBytes   ( audiofmt,        itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk7,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk8,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk9,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk10,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk11,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk12,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk13,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk14,           itwriteto );
        
            //0x40 - 0x4Fa
            itwriteto = utils::WriteIntToBytes   ( unk15,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk16,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk17,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk18,           itwriteto );

            //0x50 - 0x5F
            itwriteto = utils::WriteIntToBytes   ( unk19,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( loopstart,       itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk20,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk21,           itwriteto );

            //0x60 - 0x6F
            itwriteto = utils::WriteIntToBytes   ( volume,          itwriteto );
            itwriteto = utils::WriteIntToBytes   ( pan,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk22,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk23,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk24,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk25,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk26,           itwriteto );

            //0x70 - 0xFF Padding bytes!
            return std::fill_n( itwriteto, PaddingLen, PaddinByte );
        }


        template<class _init>
            _init ReadFromContainer(  _init itReadfrom, _init itend )
        {
            //0x0 - 0xF
            itReadfrom = utils::ReadIntFromBytes<decltype(magicn), decltype(itReadfrom), false>   ( magicn,           itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( unk1,             itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( flen,             itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( version,          itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( unk3,             itReadfrom, itend );

            //0x10 - 0x1F
            itReadfrom = utils::ReadIntFromBytes   ( unk4,             itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( unk5,             itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( year,             itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( month,            itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( day,              itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( hour,             itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( minute,           itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( second,           itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( centisec,         itReadfrom, itend );

            //0x20 - 0x2F
            itReadfrom = utils::WriteStrToByteContainer( itReadfrom,   fname, fname.size() );

            //0x30 - 0x3F
            itReadfrom = utils::ReadIntFromBytes   ( unk6,            itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( loopflag,        itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( nbchan,          itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( audiofmt,        itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( unk7,            itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( unk8,            itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( unk9,            itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( unk10,           itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( unk11,           itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( unk12,           itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( unk13,           itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( unk14,           itReadfrom, itend );
        
            //0x40 - 0x4Fa
            itReadfrom = utils::ReadIntFromBytes   ( unk15,           itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( unk16,           itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( unk17,           itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( unk18,           itReadfrom, itend );

            //0x50 - 0x5F
            itReadfrom = utils::ReadIntFromBytes   ( unk19,           itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( loopstart,       itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( unk20,           itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( unk21,           itReadfrom, itend );

            //0x60 - 0x6F
            itReadfrom = utils::ReadIntFromBytes   ( volume,          itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( pan,             itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( unk22,           itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( unk23,           itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( unk24,           itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( unk25,           itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes   ( unk26,           itReadfrom, itend );

            //0x70 - 0xFF Padding bytes!
            return utils::advAsMuchAsPossible( itReadfrom, itend, PaddingLen );
        }
    };
};

#endif