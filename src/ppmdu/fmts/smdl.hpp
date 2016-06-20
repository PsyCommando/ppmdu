#ifndef SMDL_HPP
#define SMDL_HPP
/*
smdl.hpp
2015/05/20
psycommando@gmail.com
Description: Utilities for handling Pokemon Mystery Dungeon: Explorers of Sky/Time/Darkness's .smd files.

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P

    #TODO: Move this to DSE folder + remove as many dependencies on PMD2 as possible !!!

*/
#include <dse/dse_common.hpp>
#include <dse/dse_containers.hpp>

#ifdef USE_PPMDU_CONTENT_TYPE_ANALYSER
    #include <types/content_type_analyser.hpp>
    namespace filetypes
    {
        extern const ContentTy CnTy_SMDL; //Content ID db handle
    };
#endif

namespace DSE
{

//====================================================================================================
//  Typedefs / Enums
//====================================================================================================
    static const uint32_t SMDL_MagicNumber = static_cast<uint32_t>(eDSEContainers::smdl); //0x736D646C; //"smdl"

//====================================================================================================
// Structs
//====================================================================================================

    /****************************************************************************************
        SMDL_Header
            The header of the SMDL file.
    ****************************************************************************************/
    struct SMDL_Header
    {
        static const uint32_t Size     = 64;
        static const uint32_t FNameLen = 16;
        static const uint16_t DefVers  = 0x415;
        static const uint32_t DefUnk5  = 1;
        static const uint32_t DefUnk6  = 1;
        static const int32_t  DefUnk8  = 0xFFFFFFFF;
        static const int32_t  DefUnk9  = 0xFFFFFFFF;

        static unsigned int size() { return Size; }

        uint32_t magicn          = 0;
        uint32_t unk7            = 0;
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

        uint32_t unk5            = 0;
        uint32_t unk6            = 0;

        int32_t  unk8            = 0;
        int32_t  unk9            = 0;

        /*
            Expects the destination to be at least as big as what size() returns!
        */
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes   ( SMDL_MagicNumber, itwriteto, false ); //Write constant magic number, to avoid bad surprises
            itwriteto = utils::WriteIntToBytes   ( unk7,             itwriteto );
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

            itwriteto = utils::WriteIntToBytes   ( unk5,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk6,             itwriteto );

            itwriteto = utils::WriteIntToBytes   ( unk8,             itwriteto );
            itwriteto = utils::WriteIntToBytes   ( unk9,             itwriteto );

            return itwriteto;
        }


        /*
            The iterator range must be at least as large as the header, or larger.
        */
        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itend )
        {
            itReadfrom = utils::ReadIntFromBytes( magicn,   itReadfrom, itend, false ); //false to write as big endian ,iterator is incremented
            itReadfrom = utils::ReadIntFromBytes( unk7,     itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( flen,     itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( version,  itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk1,     itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk2,     itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk3,     itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk4,     itReadfrom, itend );

            itReadfrom = utils::ReadIntFromBytes( year,     itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( month,    itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( day,      itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( hour,     itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( minute,   itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( second,   itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( centisec, itReadfrom, itend );

            itReadfrom  = utils::ReadStrFromByteContainer( itReadfrom, itend, fname.data(), FNameLen );

            itReadfrom = utils::ReadIntFromBytes( unk5,     itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk6,     itReadfrom, itend );

            itReadfrom = utils::ReadIntFromBytes( unk8,     itReadfrom, itend );
            itReadfrom = utils::ReadIntFromBytes( unk9,     itReadfrom, itend );

            return itReadfrom;
        }

        //#DEPRECATED: Now using the version above that includes a safety range check.
        //template<class _init>
        //    _init ReadFromContainer( _init itReadfrom, _init itPastEnd )
        //{
        //    itReadfrom = utils::ReadIntFromBytes( magicn,   itReadfrom, itPastEnd, false ); //false to write as big endian ,iterator is incremented
        //    itReadfrom = utils::ReadIntFromBytes( unk7,     itReadfrom, itPastEnd );
        //    itReadfrom = utils::ReadIntFromBytes( flen,     itReadfrom, itPastEnd );
        //    itReadfrom = utils::ReadIntFromBytes( version,  itReadfrom, itPastEnd );
        //    itReadfrom = utils::ReadIntFromBytes( unk1,     itReadfrom, itPastEnd );
        //    itReadfrom = utils::ReadIntFromBytes( unk2,     itReadfrom, itPastEnd );
        //    itReadfrom = utils::ReadIntFromBytes( unk3,     itReadfrom, itPastEnd );
        //    itReadfrom = utils::ReadIntFromBytes( unk4,     itReadfrom, itPastEnd );
        //    itReadfrom = utils::ReadIntFromBytes( year,     itReadfrom, itPastEnd );
        //    itReadfrom = utils::ReadIntFromBytes( month,    itReadfrom, itPastEnd );
        //    itReadfrom = utils::ReadIntFromBytes( day,      itReadfrom, itPastEnd );
        //    itReadfrom = utils::ReadIntFromBytes( hour,     itReadfrom, itPastEnd );
        //    itReadfrom = utils::ReadIntFromBytes( minute,   itReadfrom, itPastEnd );
        //    itReadfrom = utils::ReadIntFromBytes( second,   itReadfrom, itPastEnd );
        //    itReadfrom = utils::ReadIntFromBytes( centisec, itReadfrom, itPastEnd );

        //    itReadfrom  = utils::ReadStrFromByteContainer( itReadfrom, fname.data(), FNameLen );

        //    itReadfrom = utils::ReadIntFromBytes( unk5,     itReadfrom, itPastEnd );
        //    itReadfrom = utils::ReadIntFromBytes( unk6,     itReadfrom, itPastEnd );
        //    itReadfrom = utils::ReadIntFromBytes( unk8,     itReadfrom, itPastEnd );
        //    itReadfrom = utils::ReadIntFromBytes( unk9,     itReadfrom, itPastEnd );

        //    return itReadfrom;
        //}

        friend std::ostream & operator<<( std::ostream &os, const SMDL_Header & hdr );
    };




//====================================================================================================
// Class
//====================================================================================================



//====================================================================================================
// Constants
//====================================================================================================



//====================================================================================================
// Functions
//====================================================================================================
    MusicSequence ParseSMDL( const std::string & file );
    void          WriteSMDL( const std::string & file, const MusicSequence & seq );

    MusicSequence ParseSMDL( std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend );

};

#endif