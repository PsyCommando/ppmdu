#ifndef DSE_COMMON_HPP
#define DSE_COMMON_HPP
/*
dse_common.hpp
2015/06/06
psycommando@gmail.com
Description: Common data between several of the Procyon Studio Digital Sound Element sound driver.
*/
//#include <ppmdu/pmd2/pmd2_audio_data.hpp>
#include <ppmdu/utils/utility.hpp>
#include <cstdint>
#include <ctime>
#include <vector>
#include <array>
#include <string>
#include <map>

namespace DSE
{
//====================================================================================================
//  Typedefs
//====================================================================================================

//====================================================================================================
//  Constants
//====================================================================================================

    //Enum containing the IDs of all chunks used by the Procyon DSE system for sequenced music and sound effects
    enum struct eDSEChunks : uint32_t
    {
        invalid,
        wavi = 0x77617669, //"wavi"
        prgi = 0x70726769, //"prgi"
        kgrp = 0x6B677270, //"kgrp"
        pcmd = 0x70636D64, //"pcmd"
        song = 0x736F6E67, //"song"
        trk  = 0x74726B20, //"trk\0x20"
        seq  = 0x73657120, //"seq\0x20"
        bnkl = 0x626E6B6C, //"bnkl"
        mcrl = 0x6D63726C, //"mcrl"
        eoc  = 0x656F6320, //"eoc\0x20"
        eod  = 0x656F6420, //"eod\0x20"
    };
    static const unsigned int NB_DSEChunks    = 11;
    static const uint32_t     SpecialChunkLen = 0xFFFFFFB0; //Value some special chunks have as their length

    extern const std::array<eDSEChunks, NB_DSEChunks> DSEChunksList; //Array containing all chunks labels

    //DSE Chunk ID stuff
    inline eDSEChunks IntToChunkID( uint32_t   value ); //Return eDSEChunks::invalid, if invalid ID !
    inline uint32_t   ChunkIDToInt( eDSEChunks id    );


//====================================================================================================
// Structs
//====================================================================================================

    /****************************************************************************************
        DateTime
            Format used to store a date + time stamp used by all DSE formats.
    ****************************************************************************************/
    struct DateTime
    {
        uint16_t year;
        uint8_t  month;
        uint8_t  day;
        uint8_t  hour;
        uint8_t  minute;
        uint8_t  second;
        uint8_t  centsec; //100th of a second ? We don't really know what this is for..

        inline DateTime()
            :year(0), month(0), day(0), hour(0), minute(0), second(0), centsec(0)
        {}

        inline DateTime( const std::tm & src )
        {
            //http://en.cppreference.com/w/cpp/chrono/c/tm
            year    = src.tm_year + 1900; //tm_year counts the nb of years since 1900
            month   = src.tm_mon;
            day     = src.tm_mday-1;      //tm_mday begins at 1, while the time in the DSE timestamp begins at 0!
            hour    = src.tm_hour;
            minute  = src.tm_min;
            second  = (src.tm_sec == 60)? 59 : src.tm_sec; //We're not dealing with leap seconds...
        }

        //Convert into the standard std::tm calendar time format 
        inline operator std::tm()const
        {
            std::tm result;
            result.tm_year  = year - 1900;
            result.tm_mon   = month;
            result.tm_mday  = day + 1;
            result.tm_hour  = hour;
            result.tm_min   = minute;
            result.tm_sec   = second;
            result.tm_isdst = -1; //No info available
            return std::move(result);
        }

        friend std::ostream & operator<<(std::ostream &os, const DateTime &obj );
    };


    /****************************************************************************************
        ChunkHeader
            Format for chunks used in Procyon's Digital Sound Element SWDL, SMDL, SEDL format, 
            used in PMD2.
    ****************************************************************************************/
    struct ChunkHeader
    {
        static const uint32_t Size          = 16; //Length of the header
        static const uint32_t OffsetDataLen = 12; //Offset from the start of the header where the length of the chunk is stored
        uint32_t label  = 0;
        uint32_t param1 = 0;
        uint32_t param2 = 0;
        uint32_t datlen = 0;

        static unsigned int size      ()      { return Size; } //Get the size of the structure in bytes
        bool                hasLength ()const { return (datlen != SpecialChunkLen); } //Returns whether this chunk has a valid data length
        eDSEChunks          GetChunkID()const { return IntToChunkID( label ); } //Returns the enum value representing this chunk's identity, judging from the label

        //Write the structure using an iterator to a byte container
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToByteVector( label,  itwriteto, false );
            itwriteto = utils::WriteIntToByteVector( param1, itwriteto );
            itwriteto = utils::WriteIntToByteVector( param2, itwriteto );
            itwriteto = utils::WriteIntToByteVector( datlen, itwriteto );
            return itwriteto;
        }

        //Read the structure from an iterator on a byte container
        template<class _init>
            _init ReadFromContainer(  _init itReadfrom )
        {
            label   = utils::ReadIntFromByteVector<decltype(label)> (itReadfrom, false ); //iterator is incremented
            param1  = utils::ReadIntFromByteVector<decltype(param1)>(itReadfrom);
            param2  = utils::ReadIntFromByteVector<decltype(param2)>(itReadfrom);
            datlen  = utils::ReadIntFromByteVector<decltype(datlen)>(itReadfrom);
            return itReadfrom;
        }
    };

    /****************************************************************************************
        WavInfo
            Entry from the "wavi" chunk in a swdl file.
    ****************************************************************************************/
    struct WavInfo
    {
        static const uint32_t Size = 64;
        enum struct eSmplFmt : uint16_t 
        {
            invalid,
            pcm       = 0x100,
            ima_adpcm = 0x200,
        };

        uint16_t unk1       = 0;
        uint16_t id         = 0; //Index/ID of the sample
        uint16_t unk2       = 0;
        uint16_t unk3       = 0;
        uint16_t unk4       = 0;
        uint16_t unk5       = 0;
        uint16_t unk6       = 0;
        uint16_t unk7       = 0;
        uint16_t version    = 0; //
        uint16_t smplfmt    = 0; //Format of the sample 0x100 == PCM 16, 0x200 == IMA ADPCM
        uint8_t  unk9       = 0; 
        uint8_t  unk14      = 0;
        uint16_t unk10      = 0;
        uint16_t unk11      = 0;
        uint16_t unk12      = 0;
        uint32_t unk13      = 0;
        uint32_t smplrate   = 0; //Sampling rate of the sample
        uint32_t smplpos    = 0; //Offset within pcmd chunk of the sample
        uint32_t loopspos   = 0; //Position in sample, of the beginning of the loop 
        uint32_t looplen    = 0; //Length in number of samples of the loop
        uint8_t  unk17      = 0;
        uint8_t  unk18      = 0;
        uint8_t  unk19      = 0;
        uint8_t  unk20      = 0;
        uint16_t unk21      = 0;
        uint16_t unk22      = 0;
        uint16_t unk23      = 0;
        uint16_t unk24      = 0;
        uint16_t unk25      = 0;
        uint16_t unk26      = 0;

        //Write the structure using an iterator to a byte container
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToByteVector( unk1,  itwriteto );
            itwriteto = utils::WriteIntToByteVector( id, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk2, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk3, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk4, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk5, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk6, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk7, itwriteto );
            itwriteto = utils::WriteIntToByteVector( version, itwriteto );
            itwriteto = utils::WriteIntToByteVector( smplfmt, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk9, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk14, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk10, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk11, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk12, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk13, itwriteto );
            itwriteto = utils::WriteIntToByteVector( smplrate, itwriteto );
            itwriteto = utils::WriteIntToByteVector( smplpos, itwriteto );
            itwriteto = utils::WriteIntToByteVector( loopspos, itwriteto );
            itwriteto = utils::WriteIntToByteVector( looplen, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk17, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk18, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk19, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk20, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk21, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk22, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk23, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk24, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk25, itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk26, itwriteto );

            return itwriteto;
        }

        //Read the structure from an iterator on a byte container
        template<class _init>
            _init ReadFromContainer( _init itReadfrom )
        {
            unk1       = utils::ReadIntFromByteVector<decltype(unk1)>     (itReadfrom); //iterator is incremented
            id         = utils::ReadIntFromByteVector<decltype(id)>       (itReadfrom);
            unk2       = utils::ReadIntFromByteVector<decltype(unk2)>     (itReadfrom);
            unk3       = utils::ReadIntFromByteVector<decltype(unk3)>     (itReadfrom);
            unk4       = utils::ReadIntFromByteVector<decltype(unk4)>     (itReadfrom);
            unk5       = utils::ReadIntFromByteVector<decltype(unk5)>     (itReadfrom);
            unk6       = utils::ReadIntFromByteVector<decltype(unk6)>     (itReadfrom);
            unk7       = utils::ReadIntFromByteVector<decltype(unk7)>     (itReadfrom);
            version    = utils::ReadIntFromByteVector<decltype(version)>  (itReadfrom);
            smplfmt    = utils::ReadIntFromByteVector<decltype(smplfmt)>  (itReadfrom);
            unk9       = utils::ReadIntFromByteVector<decltype(unk9)>     (itReadfrom); 
            unk14      = utils::ReadIntFromByteVector<decltype(unk14)>    (itReadfrom); 
            unk10      = utils::ReadIntFromByteVector<decltype(unk10)>    (itReadfrom); 
            unk11      = utils::ReadIntFromByteVector<decltype(unk11)>    (itReadfrom); 
            unk12      = utils::ReadIntFromByteVector<decltype(unk12)>    (itReadfrom); 
            unk13      = utils::ReadIntFromByteVector<decltype(unk13)>    (itReadfrom); 
            smplrate   = utils::ReadIntFromByteVector<decltype(smplrate)> (itReadfrom); 
            smplpos    = utils::ReadIntFromByteVector<decltype(smplpos)>  (itReadfrom); 
            loopspos   = utils::ReadIntFromByteVector<decltype(loopspos)> (itReadfrom);
            looplen    = utils::ReadIntFromByteVector<decltype(looplen)>  (itReadfrom);
            unk17      = utils::ReadIntFromByteVector<decltype(unk17)>    (itReadfrom);
            unk18      = utils::ReadIntFromByteVector<decltype(unk18)>    (itReadfrom);
            unk19      = utils::ReadIntFromByteVector<decltype(unk19)>    (itReadfrom);
            unk20      = utils::ReadIntFromByteVector<decltype(unk20)>    (itReadfrom);
            unk21      = utils::ReadIntFromByteVector<decltype(unk21)>    (itReadfrom);
            unk22      = utils::ReadIntFromByteVector<decltype(unk22)>    (itReadfrom);
            unk23      = utils::ReadIntFromByteVector<decltype(unk23)>    (itReadfrom);
            unk24      = utils::ReadIntFromByteVector<decltype(unk24)>    (itReadfrom);
            unk25      = utils::ReadIntFromByteVector<decltype(unk25)>    (itReadfrom);
            unk26      = utils::ReadIntFromByteVector<decltype(unk26)>    (itReadfrom);


            return itReadfrom;
        }
    };



    /************************************************************************
        DSE_MetaData
            Leftover game-specific data from parsing a DSE file format.
    ************************************************************************/
    struct DSE_MetaData
    {
        DSE_MetaData()
            :unk1(0),unk2(0),createtime()
        {}

        uint8_t     unk1;       //Some kind of ID
        uint8_t     unk2;       //Some kind of volume value maybe
        std::string fname;      //Internal filename
        DateTime    createtime; //Time this was created on
    };

    struct DSE_MetaMusicSeq : public DSE_MetaData
    {
        DSE_MetaMusicSeq()
            :DSE_MetaData(), tpqn(0)
        {}

        uint16_t tpqn; //ticks per quarter note
    };

    struct DSE_MetaBank : public DSE_MetaData
    {
        DSE_MetaBank()
            :DSE_MetaData(), nbwavislots(0), nbprgislots(0)
        {}

        uint16_t nbwavislots;
        uint16_t nbprgislots;
    };

//====================================================================================================
// Class
//====================================================================================================

//====================================================================================================
// Functions
//====================================================================================================

    /************************************************************************
        DSE_ChunkIDLookup
            This singleton's "Find" static method returns the first chunk id 
            whose's highest byte matches the specified byte.
    ************************************************************************/
    class DSE_ChunkIDLookup
    {
    public:
        static std::vector<eDSEChunks> Find( uint8_t highbyte )
        {
            static DSE_ChunkIDLookup s_instance; //creates it when first called
            auto lambdaFId = [&highbyte]( const std::pair<uint8_t,eDSEChunks>& val )->bool
            { 
                return (val.first == highbyte); 
            };

            std::vector<eDSEChunks> possiblematches;

            for( auto itfound = s_instance.m_lutbl.find(highbyte);  //Search once, if no match will not loop once.
                 itfound != s_instance.m_lutbl.end(); 
                 itfound = std::find_if( ++itfound, s_instance.m_lutbl.end(), lambdaFId ) ) //If we had a match search again, one slot after our last match
            {
                if( itfound->second != eDSEChunks::invalid )
                    possiblematches.push_back(itfound->second);
            }

            return std::move( possiblematches );
        }

    private:
        //Build the quick lookup table
        DSE_ChunkIDLookup()
        {
            for( eDSEChunks id : DSEChunksList )
                m_lutbl.insert( std::make_pair( static_cast<uint8_t>((static_cast<uint32_t>(id) >> 24) & 0xFF) , id ) ); //Isolate highest byte
        }

        //No copy, no move
        DSE_ChunkIDLookup( const DSE_ChunkIDLookup & );
        DSE_ChunkIDLookup( DSE_ChunkIDLookup && );
        DSE_ChunkIDLookup & operator=( const DSE_ChunkIDLookup & );
        DSE_ChunkIDLookup & operator=( DSE_ChunkIDLookup && );

        std::map<uint8_t, eDSEChunks> m_lutbl;
    };

    /************************************************************************
        FindNextChunk
            Find the start of the next chunk that has the specified chunk id.

            If the chunk is not found, the function returns "end".

            NOTE: "beg" must be aligned on 4 bytes!
    ************************************************************************/
    template<class _init>
        _init FindNextChunk( _init beg, _init end, eDSEChunks chnkid )
    {
        //search
        while( beg != end ) 
        {
            //check if we possibly are at the beginning of a chunk, looking for its highest byte.
            vector<eDSEChunks> possibleid = std::move( DSE_ChunkIDLookup::Find( *beg ) ); 
            size_t             skipsize = 4; //Default byte skip size on each loop (The NDS makes 4 bytes aligned reads)

            //Check all found results
            for( auto & potential : possibleid )
            {
                //Check if its really the chunk's header start, or just a coincidence
                uint32_t actualid = utils::ReadIntFromByteVector<uint32_t>( _init(beg), false ); //Make a copy of beg, to avoid it being incremented

                if( actualid == static_cast<uint32_t>(chnkid) ) //Check if we match the chunk we're looking for
                    return beg;
                else if( actualid == static_cast<uint32_t>(potential) ) //If it actually matches another chunk's id
                {
                    _init backup = beg;

                    //Read the chunk's size and skip if possible
                    std::advance( beg, ChunkHeader::OffsetDataLen );
                    uint32_t chnksz = utils::ReadIntFromByteVector<uint32_t>(beg);

                    if( chnksz != DSE::SpecialChunkLen ) //Some chunks have an invalid length that is equal to this value.
                    {
                        //Then attempt to skip
                        try
                        {
                            skipsize = chnksz;  //We have to do this like so, because some chunks may use bogus sizes 
                                                // for some mindblowingly odd reasons.. It shouldn't happen too often though..
                        }
                        catch(std::exception &)
                        {
                            beg = backup; //Restore iterator to last valid state if fails
                        }
                    }
                    else
                        skipsize = 0; //otherwise, just continue on, without further incrementing the iterator

                    break; //After all this we know its an actual chunk, just kick us out of the loop, as other matches don't matter anymore!
                }
            }

            //Skip the required ammount of bytes
            if( skipsize != 4 )
                std::advance( beg, skipsize ); //Advance is much faster, and here we're certain we're not supposed to go out of bounds.
            else
                for( int cnt = 0; cnt < 4 && beg != end; ++cnt, ++beg ); //SMDL files chunk headers are always 4 bytes aligned
        }

        //Return Result
        return beg;
    }

};

#endif