#ifndef SWDL_HPP
#define SWDL_HPP
/*
swdl.hpp
2015/05/20
psycommando@gmail.com
Description: Utilities for handling Pokemon Mystery Dungeon: Explorers of Sky/Time/Darkness's .swd files.

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P

    #TODO: Move this to DSE folder + remove as many dependencies on PMD2 as possible !!!

*/
#include <dse/dse_common.hpp>
#include <dse/dse_containers.hpp>
#include <utils/utility.hpp>
#include <cstdint>
#include <vector>
#include <array>
#include <string>

#ifdef USE_PPMDU_CONTENT_TYPE_ANALYSER
    #include <types/content_type_analyser.hpp>
    namespace filetypes
    {
        extern const ContentTy CnTy_SWDL; //Content ID db handle 
    };
#endif

namespace DSE
{
//====================================================================================================
//  Typedefs
//====================================================================================================

//====================================================================================================
//  Constants
//====================================================================================================
    static const uint32_t SWDL_MagicNumber     = static_cast<uint32_t>(eDSEContainers::swdl);//0x7377646C; //"swdl"
    static const uint32_t SWDL_ChunksDefParam1 = 0x4150000;
    static const uint32_t SWDL_ChunksDefParam2 = 0x10;

    static const uint16_t SWDL_Version415      = 0x415;
    static const uint16_t SWDL_Version402      = 0x402;

    static const uint32_t SWDL_VersionOffset   = 0xC; //Offset in all DSE SWDL header where the version is stored.

//====================================================================================================
// Structs
//====================================================================================================
    
    struct SWDL_Header_v402;
    struct SWDL_Header_v415;


    /****************************************************************************************
        SWDL_HeaderData
            A common container for header data parsed from DSE SWDL files.
    ****************************************************************************************/
    struct SWDL_HeaderData
    {
        static const uint32_t FNameLen = 16;
        uint32_t unk18           = 0;
        uint32_t flen            = 0;
        uint16_t version         = 0;
        uint8_t  unk1            = 0;
        uint8_t  unk2            = 0;
        uint32_t unk3            = 0;
        uint32_t unk4            = 0;

        uint16_t year            = 0;
        uint8_t  month           = 0;
        uint8_t  day             = 0;
        uint8_t  hour            = 0;
        uint8_t  minute          = 0;
        uint8_t  second          = 0;
        uint8_t  centisec        = 0;
        std::array<char,FNameLen> fname;

        //Common 
        uint16_t nbwavislots     = 0;
        uint16_t nbprgislots     = 0;

        //v415 only
        uint16_t unk14           = 0;
        uint32_t pcmdlen         = 0;
        uint16_t wavilen         = 0;
        uint16_t unk17           = 0;

        //v402 only
        uint8_t nbkeygroups      = 0;

        SWDL_HeaderData & operator=( const SWDL_Header_v402 & other );
        SWDL_HeaderData & operator=( const SWDL_Header_v415 & other );
        operator SWDL_Header_v402();
        operator SWDL_Header_v415();
    };

    /****************************************************************************************
        SWDL_Header_v402
            The header of the version 0x402 SWDL file.
    ****************************************************************************************/
    struct SWDL_Header_v402
    {
        static const uint32_t Size     = 80;
        static const uint32_t FNameLen = 16;

        static const uint16_t DefVersion = SWDL_Version402;
        static const uint32_t DefUnk10   = 0xFFFFFF00;
        static const uint32_t DefUnk13   = 0x10;

        static const uint32_t NbPadBytes = 7;
        static const uint8_t  PadBytes   = 0xFF;

        static unsigned int size() { return Size; }

        uint32_t magicn          = 0;
        uint32_t unk18           = 0;
        uint32_t flen            = 0;
        uint16_t version         = 0;
        uint8_t  unk1            = 0;
        uint8_t  unk2            = 0;
        uint32_t unk3            = 0;
        uint32_t unk4            = 0;

        uint16_t year            = 0;
        uint8_t  month           = 0;
        uint8_t  day             = 0;
        uint8_t  hour            = 0;
        uint8_t  minute          = 0;
        uint8_t  second          = 0;
        uint8_t  centisec        = 0;

        std::array<char,FNameLen> fname;

        uint32_t unk10           = 0;
        uint32_t unk11           = 0;
        uint32_t unk12           = 0;
        uint32_t unk13           = 0;

        uint32_t unk15           = 0;
        uint16_t unk16           = 0;
        uint8_t  nbwavislots     = 0;
        uint8_t  nbprgislots     = 0;
        uint8_t  nbkeygroups     = 0;

        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes   ( SWDL_MagicNumber, itwriteto, false ); //Write constant magic number, to avoid bad surprises
            itwriteto = utils::WriteIntToBytes   ( unk18,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( flen,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( version,          itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk1,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk2,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk3,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk4,             itwriteto );

            itwriteto = utils::WriteIntToBytes   ( year,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( month,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( day,              itwriteto );
            itwriteto = utils::WriteIntToBytes   ( hour,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( minute,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( second,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( centisec,         itwriteto );

            itwriteto = utils::WriteStrToByteContainer( itwriteto,   fname.data(), fname.size() );

            itwriteto = utils::WriteIntToBytes   ( unk10,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk11,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk12,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk13,            itwriteto );

            itwriteto = utils::WriteIntToBytes   ( unk15,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk16,            itwriteto );
            
            itwriteto = utils::WriteIntToBytes   ( nbwavislots,      itwriteto );
            itwriteto = utils::WriteIntToBytes   ( nbprgislots,      itwriteto );
            itwriteto = utils::WriteIntToBytes   ( nbkeygroups,      itwriteto );
            
            //Put padding bytes
            itwriteto = std::fill_n( itwriteto, NbPadBytes, PadBytes );
            return itwriteto;
        }


        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itEnd )
        {
            itReadfrom = utils::ReadIntFromBytes( magicn,       itReadfrom, itEnd, false ); //iterator is incremented
            itReadfrom = utils::ReadIntFromBytes( unk18,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( flen,         itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( version,      itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk1,         itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk2,         itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk3,         itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk4,         itReadfrom, itEnd );

            itReadfrom = utils::ReadIntFromBytes( year,         itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( month,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( day,          itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( hour,         itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( minute,       itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( second,       itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( centisec,     itReadfrom, itEnd );

            itReadfrom  = utils::ReadStrFromByteContainer( itReadfrom, fname.data(), FNameLen );

            itReadfrom = utils::ReadIntFromBytes( unk10,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk11,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk12,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk13,        itReadfrom, itEnd );

            itReadfrom = utils::ReadIntFromBytes( unk15,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk16,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( nbwavislots,  itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( nbprgislots,  itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( nbkeygroups,  itReadfrom, itEnd );
            
            if( itReadfrom == itEnd )
                std::runtime_error("Error SWDL_Header_v402::ReadFromContainer(): Reached end of file before parsing padding bytes!");

            if( *itReadfrom != PadBytes )
                std::clog << "<*>- Warning: Unexpected padding bytes 0x" <<std::hex <<static_cast<uint16_t>(*itReadfrom) <<std::dec <<" found at the end of a v402 SWDL header.\n";

            //Put padding bytes
            std::advance( itReadfrom, NbPadBytes ); //Skip padding at the end
            return itReadfrom;
        }

        operator SWDL_HeaderData()
        {
            SWDL_HeaderData smddat;
            smddat.unk18           = unk18;
            smddat.flen            = flen;
            smddat.version         = version;
            smddat.unk1            = unk1;
            smddat.unk2            = unk2;
            smddat.unk3            = unk3;
            smddat.unk4            = unk4;

            smddat.year            = year;
            smddat.month           = month;
            smddat.day             = day;
            smddat.hour            = hour;
            smddat.minute          = minute;
            smddat.second          = second;
            smddat.centisec        = centisec;

            std::copy( std::begin(fname), std::end(fname), std::begin(smddat.fname) );

            //Common 
            smddat.nbwavislots     = nbwavislots;
            smddat.nbprgislots     = nbprgislots;

            //v415 only
            //smddat.unk14           = ;
            //smddat.pcmdlen         = ;
            //smddat.wavilen         = ;
            //smddat.unk17           = ;

            //v402 only
            smddat.nbkeygroups     = nbkeygroups;   
            return std::move(smddat);
        }
    };

    /****************************************************************************************
        SWDL_Header_v415
            The header of the version 0x415 SWDL file.
    ****************************************************************************************/
    struct SWDL_Header_v415
    {
        static const uint32_t Size     = 80;
        static const uint32_t FNameLen = 16;

        static const uint16_t DefVersion = SWDL_Version415;
        static const uint32_t DefUnk10   = 0xAAAAAA00;
        static const uint32_t DefUnk13   = 0x10;

        static const uint32_t MaskNoPCMD = 0xFFFF0000;
        static const uint32_t ValNoPCMD  = 0xAAAA0000;


        static unsigned int size() { return Size; }

        uint32_t magicn          = 0;
        uint32_t unk18           = 0;
        uint32_t flen            = 0;
        uint16_t version         = 0;
        uint8_t  unk1            = 0;
        uint8_t  unk2            = 0;
        uint32_t unk3            = 0;
        uint32_t unk4            = 0;

        uint16_t year            = 0;
        uint8_t  month           = 0;
        uint8_t  day             = 0;
        uint8_t  hour            = 0;
        uint8_t  minute          = 0;
        uint8_t  second          = 0;
        uint8_t  centisec        = 0;

        std::array<char,FNameLen> fname;
        uint32_t unk10           = 0;
        uint32_t unk11           = 0;

        uint32_t unk12           = 0;
        uint32_t unk13           = 0;
        uint32_t pcmdlen         = 0;
        uint16_t unk14           = 0;
        uint16_t nbwavislots     = 0;
        uint16_t nbprgislots     = 0;
        uint16_t unk17           = 0;
        uint16_t wavilen         = 0;


        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes   ( SWDL_MagicNumber, itwriteto, false ); //Write constant magic number, to avoid bad surprises
            itwriteto = utils::WriteIntToBytes   ( unk18,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( flen,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( version,          itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk1,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk2,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk3,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk4,             itwriteto );

            itwriteto = utils::WriteIntToBytes   ( year,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( month,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( day,              itwriteto );
            itwriteto = utils::WriteIntToBytes   ( hour,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( minute,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( second,           itwriteto );
            itwriteto = utils::WriteIntToBytes   ( centisec,         itwriteto );

            itwriteto = utils::WriteStrToByteContainer( itwriteto,   fname.data(), fname.size() );

            itwriteto = utils::WriteIntToBytes   ( unk10,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk11,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk12,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk13,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( pcmdlen,          itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk14,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( nbwavislots,      itwriteto );
            itwriteto = utils::WriteIntToBytes   ( nbprgislots,      itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk17,            itwriteto );
            itwriteto = utils::WriteIntToBytes   ( wavilen,          itwriteto );
            return itwriteto;
        }


        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itEnd )
        {
            itReadfrom = utils::ReadIntFromBytes( magicn,       itReadfrom, itEnd, false ); //iterator is incremented
            itReadfrom = utils::ReadIntFromBytes( unk18,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( flen,         itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( version,      itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk1,         itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk2,         itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk3,         itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk4,         itReadfrom, itEnd );

            itReadfrom = utils::ReadIntFromBytes( year,         itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( month,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( day,          itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( hour,         itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( minute,       itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( second,       itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( centisec,     itReadfrom, itEnd );

            itReadfrom  = utils::ReadStrFromByteContainer( itReadfrom, fname.data(), FNameLen );

            itReadfrom = utils::ReadIntFromBytes( unk10,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk11,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk12,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk13,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( pcmdlen,      itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk14,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( nbwavislots,  itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( nbprgislots,  itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( unk17,        itReadfrom, itEnd );
            itReadfrom = utils::ReadIntFromBytes( wavilen,      itReadfrom, itEnd );
            return itReadfrom;
        }

        //#DEPRECATED
        template<class _init>
            _init ReadFromContainer(  _init itReadfrom )
        {
            magicn      = utils::ReadIntFromBytes<decltype(magicn)>     (itReadfrom, false ); //iterator is incremented
            unk18       = utils::ReadIntFromBytes<decltype(unk18)>      (itReadfrom);
            flen        = utils::ReadIntFromBytes<decltype(flen)>       (itReadfrom);
            version     = utils::ReadIntFromBytes<decltype(version)>    (itReadfrom);
            unk1        = utils::ReadIntFromBytes<decltype(unk1)>       (itReadfrom);
            unk2        = utils::ReadIntFromBytes<decltype(unk2)>       (itReadfrom);
            unk3        = utils::ReadIntFromBytes<decltype(unk3)>       (itReadfrom);
            unk4        = utils::ReadIntFromBytes<decltype(unk4)>       (itReadfrom);

            year        = utils::ReadIntFromBytes<decltype(year)>       (itReadfrom);
            month       = utils::ReadIntFromBytes<decltype(month)>      (itReadfrom);
            day         = utils::ReadIntFromBytes<decltype(day)>        (itReadfrom);
            hour        = utils::ReadIntFromBytes<decltype(hour)>       (itReadfrom);
            minute      = utils::ReadIntFromBytes<decltype(minute)>     (itReadfrom);
            second      = utils::ReadIntFromBytes<decltype(second)>     (itReadfrom);
            centisec    = utils::ReadIntFromBytes<decltype(centisec)>   (itReadfrom);

            itReadfrom  = utils::ReadStrFromByteContainer( itReadfrom, fname.data(), FNameLen );

            unk10       = utils::ReadIntFromBytes<decltype(unk10)>      (itReadfrom);
            unk11       = utils::ReadIntFromBytes<decltype(unk11)>      (itReadfrom);
            unk12       = utils::ReadIntFromBytes<decltype(unk12)>      (itReadfrom);
            unk13       = utils::ReadIntFromBytes<decltype(unk13)>      (itReadfrom);
            pcmdlen     = utils::ReadIntFromBytes<decltype(pcmdlen)>    (itReadfrom);
            unk14       = utils::ReadIntFromBytes<decltype(unk14)>      (itReadfrom);
            nbwavislots = utils::ReadIntFromBytes<decltype(nbwavislots)>(itReadfrom);
            nbprgislots = utils::ReadIntFromBytes<decltype(nbprgislots)>(itReadfrom);
            unk17       = utils::ReadIntFromBytes<decltype(unk17)>      (itReadfrom);
            wavilen     = utils::ReadIntFromBytes<decltype(wavilen)>    (itReadfrom);
            return itReadfrom;
        }


        /*
            DoesContainsSamples
                Returns true if the swdl contains sample data.
        */
        bool DoesContainsSamples()const;

        /*
            IsSampleBankOnly
                Returns true if the swdl is only a sample bank, without program info.
        */
        bool IsSampleBankOnly()const;


        operator SWDL_HeaderData()
        {
            SWDL_HeaderData smddat;
            smddat.unk18           = unk18;
            smddat.flen            = flen;
            smddat.version         = version;
            smddat.unk1            = unk1;
            smddat.unk2            = unk2;
            smddat.unk3            = unk3;
            smddat.unk4            = unk4;

            smddat.year            = year;
            smddat.month           = month;
            smddat.day             = day;
            smddat.hour            = hour;
            smddat.minute          = minute;
            smddat.second          = second;
            smddat.centisec        = centisec;

            std::copy( std::begin(fname), std::end(fname), std::begin(smddat.fname) );

            //Common 
            smddat.nbwavislots     = nbwavislots;
            smddat.nbprgislots     = nbprgislots;

            //v415 only
            smddat.unk14           = unk14;
            smddat.pcmdlen         = pcmdlen;
            smddat.wavilen         = wavilen;
            smddat.unk17           = unk17;
            return std::move(smddat);
        }

    };

    /****************************************************************************************
        SWDL_Header
            The header of the SWDL file.
    ****************************************************************************************/
    //struct SWDL_Header_Old
    //{
    //    static const uint32_t Size     = 80;
    //    static const uint32_t FNameLen = 16;

    //    static const uint16_t DefVersion = SWDL_Version415;
    //    static const uint32_t DefUnk10   = 0xAAAAAA00;
    //    static const uint32_t DefUnk13   = 0x10;

    //    static const uint32_t MaskNoPCMD = 0xFFFF0000;
    //    static const uint32_t ValNoPCMD  = 0xAAAA0000;


    //    static unsigned int size() { return Size; }

    //    uint32_t magicn          = 0;
    //    uint32_t unk18           = 0;
    //    uint32_t flen            = 0;
    //    uint16_t version         = 0;
    //    uint8_t  unk1            = 0;
    //    uint8_t  unk2            = 0;
    //    uint32_t unk3            = 0;
    //    uint32_t unk4            = 0;

    //    uint16_t year            = 0;
    //    uint8_t  month           = 0;
    //    uint8_t  day             = 0;
    //    uint8_t  hour            = 0;
    //    uint8_t  minute          = 0;
    //    uint8_t  second          = 0;
    //    uint8_t  centisec        = 0;

    //    std::array<char,FNameLen> fname;
    //    uint32_t unk10           = 0;
    //    uint32_t unk11           = 0;

    //    uint32_t unk12           = 0;
    //    uint32_t unk13           = 0;
    //    uint32_t pcmdlen         = 0;
    //    uint16_t unk14           = 0;
    //    uint16_t nbwavislots     = 0;
    //    uint16_t nbprgislots     = 0;
    //    uint16_t unk17           = 0;
    //    uint16_t wavilen         = 0;


    //    template<class _outit>
    //        _outit WriteToContainer( _outit itwriteto )const
    //    {
    //        itwriteto = utils::WriteIntToBytes   ( SWDL_MagicNumber, itwriteto, false ); //Write constant magic number, to avoid bad surprises
    //        itwriteto = utils::WriteIntToBytes   ( unk18,            itwriteto );
    //        itwriteto = utils::WriteIntToBytes   ( flen,             itwriteto );
    //        itwriteto = utils::WriteIntToBytes   ( version,          itwriteto );
    //        itwriteto = utils::WriteIntToBytes   ( unk1,             itwriteto );
    //        itwriteto = utils::WriteIntToBytes   ( unk2,             itwriteto );
    //        itwriteto = utils::WriteIntToBytes   ( unk3,             itwriteto );
    //        itwriteto = utils::WriteIntToBytes   ( unk4,             itwriteto );

    //        itwriteto = utils::WriteIntToBytes   ( year,             itwriteto );
    //        itwriteto = utils::WriteIntToBytes   ( month,            itwriteto );
    //        itwriteto = utils::WriteIntToBytes   ( day,              itwriteto );
    //        itwriteto = utils::WriteIntToBytes   ( hour,             itwriteto );
    //        itwriteto = utils::WriteIntToBytes   ( minute,           itwriteto );
    //        itwriteto = utils::WriteIntToBytes   ( second,           itwriteto );
    //        itwriteto = utils::WriteIntToBytes   ( centisec,         itwriteto );

    //        itwriteto = utils::WriteStrToByteContainer( itwriteto,   fname.data(), fname.size() );

    //        itwriteto = utils::WriteIntToBytes   ( unk10,            itwriteto );
    //        itwriteto = utils::WriteIntToBytes   ( unk11,            itwriteto );
    //        itwriteto = utils::WriteIntToBytes   ( unk12,            itwriteto );
    //        itwriteto = utils::WriteIntToBytes   ( unk13,            itwriteto );
    //        itwriteto = utils::WriteIntToBytes   ( pcmdlen,          itwriteto );
    //        itwriteto = utils::WriteIntToBytes   ( unk14,            itwriteto );
    //        itwriteto = utils::WriteIntToBytes   ( nbwavislots,      itwriteto );
    //        itwriteto = utils::WriteIntToBytes   ( nbprgislots,      itwriteto );
    //        itwriteto = utils::WriteIntToBytes   ( unk17,            itwriteto );
    //        itwriteto = utils::WriteIntToBytes   ( wavilen,          itwriteto );
    //        return itwriteto;
    //    }


    //    template<class _init>
    //        _init ReadFromContainer( _init itReadfrom, _init itEnd )
    //    {
    //        itReadfrom = utils::ReadIntFromBytes( magicn,       itReadfrom, itEnd, false ); //iterator is incremented
    //        itReadfrom = utils::ReadIntFromBytes( unk18,        itReadfrom, itEnd );
    //        itReadfrom = utils::ReadIntFromBytes( flen,         itReadfrom, itEnd );
    //        itReadfrom = utils::ReadIntFromBytes( version,      itReadfrom, itEnd );
    //        itReadfrom = utils::ReadIntFromBytes( unk1,         itReadfrom, itEnd );
    //        itReadfrom = utils::ReadIntFromBytes( unk2,         itReadfrom, itEnd );
    //        itReadfrom = utils::ReadIntFromBytes( unk3,         itReadfrom, itEnd );
    //        itReadfrom = utils::ReadIntFromBytes( unk4,         itReadfrom, itEnd );

    //        itReadfrom = utils::ReadIntFromBytes( year,         itReadfrom, itEnd );
    //        itReadfrom = utils::ReadIntFromBytes( month,        itReadfrom, itEnd );
    //        itReadfrom = utils::ReadIntFromBytes( day,          itReadfrom, itEnd );
    //        itReadfrom = utils::ReadIntFromBytes( hour,         itReadfrom, itEnd );
    //        itReadfrom = utils::ReadIntFromBytes( minute,       itReadfrom, itEnd );
    //        itReadfrom = utils::ReadIntFromBytes( second,       itReadfrom, itEnd );
    //        itReadfrom = utils::ReadIntFromBytes( centisec,     itReadfrom, itEnd );

    //        itReadfrom  = utils::ReadStrFromByteContainer( itReadfrom, fname.data(), FNameLen );

    //        itReadfrom = utils::ReadIntFromBytes( unk10,        itReadfrom, itEnd );
    //        itReadfrom = utils::ReadIntFromBytes( unk11,        itReadfrom, itEnd );
    //        itReadfrom = utils::ReadIntFromBytes( unk12,        itReadfrom, itEnd );
    //        itReadfrom = utils::ReadIntFromBytes( unk13,        itReadfrom, itEnd );
    //        itReadfrom = utils::ReadIntFromBytes( pcmdlen,      itReadfrom, itEnd );
    //        itReadfrom = utils::ReadIntFromBytes( unk14,        itReadfrom, itEnd );
    //        itReadfrom = utils::ReadIntFromBytes( nbwavislots,  itReadfrom, itEnd );
    //        itReadfrom = utils::ReadIntFromBytes( nbprgislots,  itReadfrom, itEnd );
    //        itReadfrom = utils::ReadIntFromBytes( unk17,        itReadfrom, itEnd );
    //        itReadfrom = utils::ReadIntFromBytes( wavilen,      itReadfrom, itEnd );
    //        return itReadfrom;
    //    }

    //    //#DEPRECATED
    //    template<class _init>
    //        _init ReadFromContainer(  _init itReadfrom )
    //    {
    //        magicn      = utils::ReadIntFromBytes<decltype(magicn)>     (itReadfrom, false ); //iterator is incremented
    //        unk18       = utils::ReadIntFromBytes<decltype(unk18)>      (itReadfrom);
    //        flen        = utils::ReadIntFromBytes<decltype(flen)>       (itReadfrom);
    //        version     = utils::ReadIntFromBytes<decltype(version)>    (itReadfrom);
    //        unk1        = utils::ReadIntFromBytes<decltype(unk1)>       (itReadfrom);
    //        unk2        = utils::ReadIntFromBytes<decltype(unk2)>       (itReadfrom);
    //        unk3        = utils::ReadIntFromBytes<decltype(unk3)>       (itReadfrom);
    //        unk4        = utils::ReadIntFromBytes<decltype(unk4)>       (itReadfrom);

    //        year        = utils::ReadIntFromBytes<decltype(year)>       (itReadfrom);
    //        month       = utils::ReadIntFromBytes<decltype(month)>      (itReadfrom);
    //        day         = utils::ReadIntFromBytes<decltype(day)>        (itReadfrom);
    //        hour        = utils::ReadIntFromBytes<decltype(hour)>       (itReadfrom);
    //        minute      = utils::ReadIntFromBytes<decltype(minute)>     (itReadfrom);
    //        second      = utils::ReadIntFromBytes<decltype(second)>     (itReadfrom);
    //        centisec    = utils::ReadIntFromBytes<decltype(centisec)>   (itReadfrom);

    //        itReadfrom  = utils::ReadStrFromByteContainer( itReadfrom, fname.data(), FNameLen );

    //        unk10       = utils::ReadIntFromBytes<decltype(unk10)>      (itReadfrom);
    //        unk11       = utils::ReadIntFromBytes<decltype(unk11)>      (itReadfrom);
    //        unk12       = utils::ReadIntFromBytes<decltype(unk12)>      (itReadfrom);
    //        unk13       = utils::ReadIntFromBytes<decltype(unk13)>      (itReadfrom);
    //        pcmdlen     = utils::ReadIntFromBytes<decltype(pcmdlen)>    (itReadfrom);
    //        unk14       = utils::ReadIntFromBytes<decltype(unk14)>      (itReadfrom);
    //        nbwavislots = utils::ReadIntFromBytes<decltype(nbwavislots)>(itReadfrom);
    //        nbprgislots = utils::ReadIntFromBytes<decltype(nbprgislots)>(itReadfrom);
    //        unk17       = utils::ReadIntFromBytes<decltype(unk17)>      (itReadfrom);
    //        wavilen     = utils::ReadIntFromBytes<decltype(wavilen)>    (itReadfrom);
    //        return itReadfrom;
    //    }


    //    /*
    //        DoesContainsSamples
    //            Returns true if the swdl contains sample data.
    //    */
    //    bool DoesContainsSamples()const;

    //    /*
    //        IsSampleBankOnly
    //            Returns true if the swdl is only a sample bank, without program info.
    //    */
    //    bool IsSampleBankOnly()const;
    //};





//====================================================================================================
// Class
//====================================================================================================



    /*
        Represents the possible content of a swdl file.
        Just an additional layer over the swdl loaded to determine if 
    */
    //class SWDL_Content
    //{
    //public:

    //    SWDL_Content()
    //    {
    //    }

    //    /*
    //        Details on the SWDL file
    //    */
    //    bool ContainsSamples()const;

    //    /*
    //        Convert to generic format
    //    */
    //    operator pmd2::audio::PresetBank();


    //private:
    //    pmd2::audio::SampleBank     m_audiodata;
    //    pmd2::audio::ProgramBank m_instdata;


    //};

//====================================================================================================
// Functions
//====================================================================================================

    PresetBank ParseSWDL( const std::string & filename );
    void       WriteSWDL( const std::string & filename, const PresetBank & audiodata );

    //Parse from a range.
    PresetBank ParseSWDL( std::vector<uint8_t>::const_iterator itbeg, 
                          std::vector<uint8_t>::const_iterator itend );

    /*
        ReadSwdlHeader
            Reads only the SWDL header from a file.
    */
    SWDL_HeaderData ReadSwdlHeader( const std::string & filename );
    SWDL_HeaderData ReadSwdlHeader( std::vector<uint8_t>::const_iterator itbeg, 
                                    std::vector<uint8_t>::const_iterator itend );



    /*
        GetDSEHeaderLen
            Returns the length of the SWDL header for the version of DSE specified
    */
    inline size_t GetDSEHeaderLen( eDSEVersion ver )
    {
        if( ver == eDSEVersion::V415 || ver == eDSEVersion::VDef )
            return SWDL_Header_v415::Size;
        else if( ver == eDSEVersion::V402 )
            return SWDL_Header_v402::Size;
        else
        {
            throw std::runtime_error("Error: GetDSEHeaderLen() : Bad DSE version!");
            return SWDL_Header_v415::Size;
        }
    }

};

#endif