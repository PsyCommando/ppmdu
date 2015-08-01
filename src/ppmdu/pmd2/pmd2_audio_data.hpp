#ifndef PMD2_AUDIO_DATA_HPP
#define PMD2_AUDIO_DATA_HPP
/*
pmd2_audio_data.hpp
2015/05/20
psycommando@gmail.com
Description: Containers and utilities for data parsed from PMD2's audio, and sequencer files.

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P
*/
#include <ppmdu/fmts/dse_common.hpp>
#include <ppmdu/fmts/dse_sequence.hpp>
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <future>

namespace pmd2 { namespace audio 
{
//====================================================================================================
//  Typedefs
//====================================================================================================

//====================================================================================================
//  Constants
//====================================================================================================
    static const std::string SMDL_FileExtension = "smd";
    static const std::string SWDL_FileExtension = "swd";
    static const std::string SEDL_FileExtension = "sed";

    static const uint32_t    DSE_MaxDecayDur    =  8; //second
    static const uint32_t    DSE_MaxAttackDur   = 10; //second
    static const uint32_t    DSE_MaxHoldDur     = 10; //second
    static const uint32_t    DSE_MaxReleaseDur  =  8; //second

//====================================================================================================
// Structs
//====================================================================================================

//====================================================================================================
// Class
//====================================================================================================

    /*****************************************************************************************
        KeyGroups
            Contains several groups of notes each with an ID.
    *****************************************************************************************/
    //class KeyGroups
    //{
    //public:

    //    struct KeyGroup
    //    {
    //        uint16_t id   = 0;
    //        uint8_t  unk1 = 0;
    //        uint8_t  unk2 = 0;
    //        uint16_t unk3 = 0;
    //        uint16_t unk4 = 0;
    //    };

    //    KeyGroups()
    //    {
    //    }

    //    std::vector<KeyGroup> getKeygrp() { return ; }

    //private:
    //    std::vector<KeyGroup> m_Groups;
    //};

    struct KeyGroup
    {
        friend std::ostream & operator<<( std::ostream &  strm, const pmd2::audio::KeyGroup & other );

        static const uint32_t SIZE = 8; //bytes
        static uint32_t size() {return SIZE;}

        uint16_t id       = 0;
        uint8_t  poly     = 0;
        uint8_t  priority = 0;
        uint8_t  vclow    = 0;
        uint8_t  vchigh   = 0;
        uint8_t  unk50    = 0;
        uint8_t  unk51    = 0;

        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToByteVector( id,       itwriteto );
            itwriteto = utils::WriteIntToByteVector( poly,     itwriteto );
            itwriteto = utils::WriteIntToByteVector( priority, itwriteto );
            itwriteto = utils::WriteIntToByteVector( vclow,    itwriteto );
            itwriteto = utils::WriteIntToByteVector( vchigh,   itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk50,    itwriteto );
            itwriteto = utils::WriteIntToByteVector( unk51,    itwriteto );
            return itwriteto;
        }


        template<class _init>
            _init ReadFromContainer( _init itReadfrom )
        {
            id       = utils::ReadIntFromByteVector<decltype(id)>  (itReadfrom);
            poly     = utils::ReadIntFromByteVector<decltype(poly)>(itReadfrom);
            priority = utils::ReadIntFromByteVector<decltype(priority)>(itReadfrom);
            vclow    = utils::ReadIntFromByteVector<decltype(vclow)>(itReadfrom);
            vchigh   = utils::ReadIntFromByteVector<decltype(vchigh)>(itReadfrom);
            unk50    = utils::ReadIntFromByteVector<decltype(unk50)>(itReadfrom);
            unk51    = utils::ReadIntFromByteVector<decltype(unk51)>(itReadfrom);
            return itReadfrom;
        }
    };

    /*****************************************************************************************
        ProgramInfo
            Contains data for a single instrument.
    *****************************************************************************************/
    class ProgramInfo
    {
    public:

        friend std::ostream & operator<<( std::ostream &  strm, const pmd2::audio::ProgramInfo & other );

        /*---------------------------------------------------------------------
            InstInfoHeader
                First 16 bytes of an instrument info block
        ---------------------------------------------------------------------*/
        struct InstInfoHeader
        {
            static const uint32_t SIZE = 16; //bytes
            static uint32_t size() { return SIZE; }

            uint16_t id        = 0;
            uint16_t nbsplits  = 0;
            uint8_t  insvol    = 0;
            uint8_t  inspan    = 0;
            uint16_t unk3      = 0;
            uint16_t unk4      = 0;
            uint8_t  unk5      = 0;
            uint8_t  nblfos    = 0; //Nb entries in the first table 
            uint8_t  padbyte   = 0; //character used for padding
            uint8_t  unk7      = 0;
            uint8_t  unk8      = 0;
            uint8_t  unk9      = 0;

            inline bool operator==( const InstInfoHeader & other )const
            {
                return ( id       == other.id       && 
                         nbsplits == other.nbsplits && 
                         insvol   == other.insvol   && 
                         inspan   == other.inspan   && 
                         unk3     == other.unk3     && 
                         unk4     == other.unk4     && 
                         unk5     == other.unk5     &&  
                         nblfos   == other.nblfos   &&
                         padbyte  == other.padbyte  &&
                         unk7     == other.unk7     &&
                         unk8     == other.unk8     &&
                         unk9     == other.unk9     );
            }

            inline bool operator!=( const InstInfoHeader & other )const
            {
                return !( operator==(other));
            }

            template<class _outit>
                _outit WriteToContainer( _outit itwriteto )const
            {
                itwriteto = utils::WriteIntToByteVector( id,        itwriteto );
                itwriteto = utils::WriteIntToByteVector( nbsplits,  itwriteto );
                itwriteto = utils::WriteIntToByteVector( insvol,    itwriteto );
                itwriteto = utils::WriteIntToByteVector( inspan,    itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk3,      itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk4,      itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk5,      itwriteto );
                itwriteto = utils::WriteIntToByteVector( nblfos,    itwriteto );
                itwriteto = utils::WriteIntToByteVector( padbyte,   itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk7,      itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk8,      itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk9,      itwriteto );
                return itwriteto;
            }


            template<class _init>
                _init ReadFromContainer( _init itReadfrom )
            {
                id        = utils::ReadIntFromByteVector<decltype(id)>       (itReadfrom);
                nbsplits  = utils::ReadIntFromByteVector<decltype(nbsplits)> (itReadfrom);

                insvol    = utils::ReadIntFromByteVector<decltype(insvol)>   (itReadfrom);
                inspan    = utils::ReadIntFromByteVector<decltype(inspan)>   (itReadfrom);

                unk3      = utils::ReadIntFromByteVector<decltype(unk3)>     (itReadfrom);
                unk4      = utils::ReadIntFromByteVector<decltype(unk4)>     (itReadfrom);
                unk5      = utils::ReadIntFromByteVector<decltype(unk5)>     (itReadfrom);
                nblfos    = utils::ReadIntFromByteVector<decltype(nblfos)>   (itReadfrom);
                padbyte   = utils::ReadIntFromByteVector<decltype(padbyte)>  (itReadfrom);
                unk7      = utils::ReadIntFromByteVector<decltype(unk7)>     (itReadfrom);
                unk8      = utils::ReadIntFromByteVector<decltype(unk8)>     (itReadfrom);
                unk9      = utils::ReadIntFromByteVector<decltype(unk9)>     (itReadfrom);
                return itReadfrom;
            }
        };

        /*---------------------------------------------------------------------
            LFOTblEntry
                First table after the header
        ---------------------------------------------------------------------*/
        struct LFOTblEntry
        {
            static const uint32_t SIZE = 16; //bytes
            static uint32_t size() { return SIZE; }

            uint8_t  unk34 = 0;
            uint8_t  unk52 = 0;
            uint8_t  unk26 = 0;
            uint8_t  unk27 = 0;
            uint16_t unk28 = 0;
            uint16_t unk29 = 0;
            uint16_t unk30 = 0;
            uint16_t unk31 = 0;
            uint16_t unk32 = 0;
            uint16_t unk33 = 0;

            template<class _outit>
                _outit WriteToContainer( _outit itwriteto )const
            {
                itwriteto = utils::WriteIntToByteVector( unk34, itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk52, itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk26, itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk27, itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk28, itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk29, itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk30, itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk31, itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk32, itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk33, itwriteto );
                return itwriteto;
            }


            template<class _init>
                _init ReadFromContainer( _init itReadfrom )
            {
                unk34      = utils::ReadIntFromByteVector<decltype(unk34)>(itReadfrom);
                unk52      = utils::ReadIntFromByteVector<decltype(unk52)>(itReadfrom);
                unk26      = utils::ReadIntFromByteVector<decltype(unk26)>(itReadfrom);
                unk27      = utils::ReadIntFromByteVector<decltype(unk27)>(itReadfrom);
                unk28      = utils::ReadIntFromByteVector<decltype(unk28)>(itReadfrom);
                unk29      = utils::ReadIntFromByteVector<decltype(unk29)>(itReadfrom);
                unk30      = utils::ReadIntFromByteVector<decltype(unk30)>(itReadfrom);
                unk31      = utils::ReadIntFromByteVector<decltype(unk31)>(itReadfrom);
                unk32      = utils::ReadIntFromByteVector<decltype(unk32)>(itReadfrom);
                unk33      = utils::ReadIntFromByteVector<decltype(unk33)>(itReadfrom);
                return itReadfrom;
            }
        };

        /*---------------------------------------------------------------------
            SplitEntry
                Data on a particular sample mapped to this instrument
        ---------------------------------------------------------------------*/
        struct SplitEntry
        {
            static const uint32_t SIZE = 48; //bytes
            static uint32_t size() { return SIZE; }

            uint8_t  unk10    = 0; //0x0
            uint8_t  id       = 0; //0x1
            uint8_t  unk11    = 0; //0x2
            uint8_t  unk25    = 0; //0x3

            int8_t   lowkey   = 0; //0x4
            int8_t   hikey    = 0; //0x5

            int8_t   unk13    = 0; //0x6
            int8_t   unk46    = 0; //0x7
            int8_t   unk14    = 0; //0x8
            int8_t   unk47    = 0; //0x9
            int8_t   unk15    = 0; //0xA
            int8_t   unk48    = 0; //0xB

            uint32_t unk16    = 0; //0xC
            uint16_t unk17    = 0; //0x10
            uint16_t smplid   = 0; //0x12

            int8_t  tune      = 0; //0x14
            int8_t  group     = 0; //0x15

            int8_t  rootkey   = 0; //0x16
            int8_t  ctune     = 0; //0x17

            uint8_t smplvol   = 0; //0x18
            uint8_t smplpan   = 0; //0x19

            uint8_t  smplgain = 0; //0x1A
            uint8_t  unk22    = 0; //0x1B
            uint16_t unk23    = 0; //0x1C
            uint16_t unk24    = 0; //0x1E
            //The last 16 bytes are a perfect copy of the last 16 bytes of a wavi info block
            uint8_t  unk35      = 0; //0x20
            uint8_t  unk36      = 0; //0x21
            uint8_t  unk37      = 0; //0x22
            uint8_t  unk38      = 0; //0x23
            uint16_t unk39      = 0; //0x24
            int16_t  unk40      = 0; //0x26

            int8_t   atkvol     = 0; //0x28
            int8_t   attack     = 0; //0x29

            int8_t   decay      = 0; //0x2A
            int8_t   sustain    = 0; //0x2B

            int8_t   hold       = 0; //0x2C
            int8_t   decay2     = 0; //0x2D

            int8_t   release    = 0; //0x2E
            int8_t   rx         = 0; //0x2F

            template<class _outit>
                _outit WriteToContainer( _outit itwriteto )const
            {
                itwriteto = utils::WriteIntToByteVector( unk10,    itwriteto );
                itwriteto = utils::WriteIntToByteVector( id,       itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk11,    itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk25,    itwriteto );

                itwriteto = utils::WriteIntToByteVector( lowkey,   itwriteto );
                itwriteto = utils::WriteIntToByteVector( hikey,    itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk13,    itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk46,    itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk14,    itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk47,    itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk15,    itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk48,    itwriteto );

                itwriteto = utils::WriteIntToByteVector( unk16,    itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk17,    itwriteto );
                itwriteto = utils::WriteIntToByteVector( smplid,   itwriteto );

                itwriteto = utils::WriteIntToByteVector( tune,     itwriteto );
                itwriteto = utils::WriteIntToByteVector( group,    itwriteto );

                itwriteto = utils::WriteIntToByteVector( rootkey,  itwriteto );
                itwriteto = utils::WriteIntToByteVector( ctune,    itwriteto );

                itwriteto = utils::WriteIntToByteVector( smplvol,  itwriteto );
                itwriteto = utils::WriteIntToByteVector( smplpan,  itwriteto );

                itwriteto = utils::WriteIntToByteVector( smplgain, itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk22,    itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk23,    itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk24,    itwriteto );

                itwriteto = utils::WriteIntToByteVector( unk35,    itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk36,    itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk37,    itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk38,    itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk39,    itwriteto );
                itwriteto = utils::WriteIntToByteVector( unk40,    itwriteto );

                itwriteto = utils::WriteIntToByteVector( atkvol,   itwriteto );
                itwriteto = utils::WriteIntToByteVector( attack,    itwriteto );

                itwriteto = utils::WriteIntToByteVector( decay, itwriteto );
                itwriteto = utils::WriteIntToByteVector( sustain,   itwriteto );

                itwriteto = utils::WriteIntToByteVector( hold,  itwriteto );
                itwriteto = utils::WriteIntToByteVector( decay2,   itwriteto );

                itwriteto = utils::WriteIntToByteVector( release,  itwriteto );
                itwriteto = utils::WriteIntToByteVector( rx, itwriteto );
                return itwriteto;
            }


            template<class _init>
                _init ReadFromContainer( _init itReadfrom )
            {
                itReadfrom = utils::ReadIntFromByteContainer( unk10,    itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( id,       itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( unk11,    itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( unk25,    itReadfrom );

                itReadfrom = utils::ReadIntFromByteContainer( lowkey,   itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( hikey,    itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( unk13,    itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( unk46,    itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( unk14,    itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( unk47,    itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( unk15,    itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( unk48,    itReadfrom );

                itReadfrom = utils::ReadIntFromByteContainer( unk16,    itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( unk17,    itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( smplid,   itReadfrom );

                itReadfrom = utils::ReadIntFromByteContainer( tune,     itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( group,    itReadfrom );

                itReadfrom = utils::ReadIntFromByteContainer( rootkey,  itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( ctune,    itReadfrom );

                itReadfrom = utils::ReadIntFromByteContainer( smplvol,  itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( smplpan,  itReadfrom );

                itReadfrom = utils::ReadIntFromByteContainer( smplgain, itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( unk22,    itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( unk23,    itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( unk24,    itReadfrom );

                itReadfrom = utils::ReadIntFromByteContainer( unk35,    itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( unk36,    itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( unk37,    itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( unk38,    itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( unk39,    itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( unk40,    itReadfrom );

                itReadfrom = utils::ReadIntFromByteContainer( atkvol,   itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( attack,   itReadfrom );

                itReadfrom = utils::ReadIntFromByteContainer( decay,    itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( sustain,  itReadfrom );

                itReadfrom = utils::ReadIntFromByteContainer( hold,     itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( decay2,   itReadfrom );

                itReadfrom = utils::ReadIntFromByteContainer( release,  itReadfrom );
                itReadfrom = utils::ReadIntFromByteContainer( rx,       itReadfrom );

                return itReadfrom;
            }
        };

        //----------------------------
        //  ProgramInfo
        //----------------------------
        ProgramInfo()
        {}

        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = m_hdr.WriteToContainer(itwriteto);

            for( const auto & entry : m_lfotbl )
                itwriteto = entry.WriteToContainer(itwriteto);

            //16 bytes of padding
            itwriteto = std::fill_n( itwriteto, 16, m_hdr.padbyte );

            for( const auto & smpl : m_splitstbl )
                itwriteto = smpl.WriteToContainer(itwriteto);

            return itwriteto;
        }


        template<class _init>
            _init ReadFromContainer( _init itReadfrom )
        {
            itReadfrom = m_hdr.ReadFromContainer(itReadfrom);

            m_lfotbl   .resize(m_hdr.nblfos);
            m_splitstbl.resize(m_hdr.nbsplits);

            for( auto & entry : m_lfotbl )
                itReadfrom = entry.ReadFromContainer(itReadfrom);

            //16 bytes of padding
            std::advance( itReadfrom, 16 );

            for( auto & smpl : m_splitstbl )
                itReadfrom = smpl.ReadFromContainer(itReadfrom);

            return itReadfrom;
        }

        enum struct eCompareRes
        {
            different,
            sharesamples,
            identical,
        };

        eCompareRes isSimilar( const ProgramInfo & other )const
        {
            //
            size_t  nbmatchsmpls   = 0;

            //Test shared samples first (We don't care about slight variations in the samples parameters)
            for( const auto & asmpl : m_splitstbl )
            {
                auto found = std::find_if( other.m_splitstbl.begin(), 
                                           other.m_splitstbl.end(), 
                                           [&asmpl](const SplitEntry& entry){ return (entry.smplid == asmpl.smplid); } );
                
                if( found != other.m_splitstbl.end() )
                    ++nbmatchsmpls;
            }

            if( nbmatchsmpls != std::max( m_splitstbl.size(), m_splitstbl.size() ) )
                return eCompareRes::sharesamples;
            else if( nbmatchsmpls == 0 )
                return eCompareRes::different;

            //Test header and prestbl for saying the whole thing is identical
            if( m_hdr == other.m_hdr )
                return eCompareRes::identical;
            else
                return eCompareRes::different;
        }

    /*private:*/
        InstInfoHeader           m_hdr;
        std::vector<LFOTblEntry> m_lfotbl;
        std::vector<SplitEntry>  m_splitstbl;
    };

    /*
    */

    /*****************************************************************************************
        SampleBank
            This class is used to maintain references to sample data. 
            The samples in this are refered to by entries in a SampleMap instance.
    *****************************************************************************************/
    class SampleBank
    {
    public:
        typedef std::unique_ptr<std::vector<uint8_t>> smpl_t;            //Pointer to a vector of raw sample data
        typedef std::unique_ptr<DSE::WavInfo>         wavinfoptr_t;      //Pointer to wavinfo

        struct SampleBlock
        {
            wavinfoptr_t pinfo_;
            smpl_t       pdata_;

            inline bool isnull() { return (pinfo_ == nullptr) && (pdata_ == nullptr); }

            SampleBlock(){}
            SampleBlock(const SampleBlock&)             = delete;
            SampleBlock & operator=(const SampleBlock&) = delete;

            SampleBlock( SampleBlock && other )
            {
                pinfo_.reset( other.pinfo_.release() );
                pdata_.reset( other.pdata_.release() );
            }

            SampleBlock & operator=( SampleBlock && other )
            {
                pinfo_.reset( other.pinfo_.release() );
                pdata_.reset( other.pdata_.release() );
                return *this;
            }
        };
        typedef SampleBlock smpldata_t;

        SampleBank( std::vector<smpldata_t> && smpls )
            :m_SampleData(std::move(smpls)) //MSVC is too derp to use the correct constructor..
        {}

        SampleBank( SampleBank && mv )
        {
            m_SampleData = std::move( (mv.m_SampleData) );
        }

        SampleBank & operator=( SampleBank && mv )
        {
            m_SampleData = std::move( (mv.m_SampleData) );
            return *this;
        }

        SampleBank( const SampleBank & other )
        {
            DoCopyFrom(other);
        }

        const SampleBank & operator=( const SampleBank & other )
        {
            DoCopyFrom(other);
            return *this;
        }

    public:
        //Info

        //Nb of sample slots with or without data
        int                                 NbSlots()const     { return m_SampleData.size(); } 

        //Access
        bool                                IsInfoPresent      ( unsigned int index )const { return m_SampleData[index].pinfo_ != nullptr; }
        bool                                IsDataPresent      ( unsigned int index )const { return m_SampleData[index].pdata_ != nullptr; }

        inline DSE::WavInfo               * sampleInfo         ( unsigned int index )      { return m_SampleData[index].pinfo_.get(); }
        inline DSE::WavInfo const         * sampleInfo         ( unsigned int index )const { return m_SampleData[index].pinfo_.get(); }

        inline std::vector<uint8_t>       * sample             ( unsigned int index )      { return m_SampleData[index].pdata_.get(); }
        inline const std::vector<uint8_t> * sample             ( unsigned int index )const { return m_SampleData[index].pdata_.get(); }

        inline std::vector<uint8_t>       * operator[]         ( unsigned int index )      { return sample(index); }
        inline const std::vector<uint8_t> * operator[]         ( unsigned int index )const { return sample(index); }

    private:

        //Copy the content pointed by the pointers, and not just the pointers themselves !
        void DoCopyFrom( const SampleBank & other )
        {
            m_SampleData.resize( other.m_SampleData.size() );

            for( size_t i = 0; i < other.m_SampleData.size(); ++i  )
            {
                if( other.m_SampleData[i].pdata_ != nullptr )
                    m_SampleData[i].pdata_.reset( new std::vector<uint8_t>( *(other.m_SampleData[i].pdata_) ) ); //Copy each objects and make a pointer

                if( other.m_SampleData[i].pinfo_ != nullptr )
                    m_SampleData[i].pinfo_.reset( new DSE::WavInfo( *(other.m_SampleData[i].pinfo_) ) ); //Copy each objects and make a pointer
            }
        }

    private:
        std::vector<smpldata_t>         m_SampleData;
    };


    /*****************************************************************************************
        InstrumentBank
            Contains the entries for each instruments
            Data on what samples an instrument uses, how to play those, the key mapping, etc..
    *****************************************************************************************/
    class InstrumentBank
    {
    public:
        typedef std::unique_ptr<ProgramInfo> ptrinst_t;

        InstrumentBank( std::vector<ptrinst_t> && instinf, std::vector<KeyGroup> && kgrp )
            :m_instinfoslots(std::move(instinf)),  //MSVC is too derp to use the correct constructor..
             m_Groups(kgrp)
        {}

        InstrumentBank( InstrumentBank && mv )
        {
            m_instinfoslots = std::move(mv.m_instinfoslots);
            m_Groups        = std::move(mv.m_Groups);
        }

        InstrumentBank & operator=( InstrumentBank&& mv )
        {
            m_instinfoslots = std::move(mv.m_instinfoslots);
            m_Groups        = std::move(mv.m_Groups);
            return *this;
        }

        ptrinst_t       & operator[]( size_t index )      { return m_instinfoslots[index]; }
        const ptrinst_t & operator[]( size_t index )const { return m_instinfoslots[index]; }

        std::vector<ptrinst_t>       & instinfo()      { return m_instinfoslots; }
        const std::vector<ptrinst_t> & instinfo()const { return m_instinfoslots; }

        std::vector<KeyGroup>        & keygrps()       { return m_Groups;        }
        const std::vector<KeyGroup>  & keygrps()const  { return m_Groups;        }

    private:
        std::vector<ptrinst_t> m_instinfoslots;
        std::vector<KeyGroup>  m_Groups;

        //Can't copy
        InstrumentBank( const InstrumentBank& );
        InstrumentBank & operator=( const InstrumentBank& );
    };

    /*****************************************************************************************
        PresetBank
            Is the combination of a SampleBank, and an InstrumentBank.
            Or just an instrument bank if samples are not available
    *****************************************************************************************/
    class PresetBank
    {
    public:

        typedef std::shared_ptr<InstrumentBank>   ptrinst_t;
        typedef std::weak_ptr<InstrumentBank>     wptrinst_t;

        typedef std::shared_ptr<SampleBank>       ptrsmpl_t;
        typedef std::weak_ptr<SampleBank>         wptrsmpl_t;

        PresetBank()
        {}

        PresetBank( DSE::DSE_MetaBank && meta, std::unique_ptr<InstrumentBank> && pInstrument, std::unique_ptr<SampleBank>  && pSmpl )
            :m_pInstbnk(std::move(pInstrument)), m_pSamples(std::move(pSmpl)), m_meta(std::move(meta))
        {}

        PresetBank( DSE::DSE_MetaBank && meta, std::unique_ptr<InstrumentBank> && pInstrument )
            :m_pInstbnk(std::move(pInstrument)), m_pSamples(nullptr), m_meta(std::move(meta))
        {}

        PresetBank( DSE::DSE_MetaBank && meta, std::unique_ptr<SampleBank> && pSmpl )
            :m_pInstbnk(nullptr), m_pSamples(std::move(pSmpl)), m_meta(std::move(meta))
        {}

        PresetBank( PresetBank && mv )
        {
            m_pInstbnk = std::move( mv.m_pInstbnk );
            m_pSamples = std::move( mv.m_pSamples );
            m_meta     = std::move( mv.m_meta     );
        }

        PresetBank & operator=( PresetBank && mv )
        {
            m_pInstbnk = std::move( mv.m_pInstbnk );
            m_pSamples = std::move( mv.m_pSamples );
            m_meta     = std::move( mv.m_meta     );
            return *this;
        }


        DSE::DSE_MetaBank       & metadata()                                                 { return m_meta; }
        const DSE::DSE_MetaBank & metadata()const                                            { return m_meta; }
        void                      metadata( const DSE::DSE_MetaBank & data )                 { m_meta = data; }
        void                      metadata( DSE::DSE_MetaBank && data )                      { m_meta = data; }
        
        //Returns a weak_ptr to the samplebank
        wptrsmpl_t                smplbank()                                                { return m_pSamples; }
        const wptrsmpl_t          smplbank()const                                           { return m_pSamples; }
        void                      smplbank( ptrsmpl_t && samplesbank)                       { m_pSamples = std::move(samplesbank); }
        //void                      smplbank( SampleBank  * samplesbank)                       { m_pSamples = samplesbank; }
        void                      smplbank( std::unique_ptr<SampleBank>  && samplesbank)     { m_pSamples = std::move(samplesbank); }

        //Returns a weak_ptr to the instrument bank
        wptrinst_t                instbank()                                                { return m_pInstbnk; }
        const wptrinst_t          instbank()const                                           { return m_pInstbnk; }
        void                      instbank( ptrinst_t    && instbank)                       { m_pInstbnk = std::move(instbank); }
        //void                      instbank( InstrumentBank * instbank)                       { m_pInstbnk.reset(instbank); }
        void                      instbank( std::unique_ptr<InstrumentBank>     && instbank) { m_pInstbnk = std::move(instbank); }

    private:
        //Can't copy
        PresetBank( const PresetBank& );
        PresetBank& operator=( const PresetBank& );

        DSE::DSE_MetaBank m_meta;
        ptrinst_t         m_pInstbnk; //An instrument bank may not be shared by many
        ptrsmpl_t         m_pSamples; //A sample bank may be shared by many
    };

    /*****************************************************************************************
        MusicTrack
            Represent a single track of DSE events within a music sequence!
    *****************************************************************************************/
    class MusicTrack
    {
    public:
        typedef std::vector<DSE::TrkEvent>::iterator       iterator;
        typedef std::vector<DSE::TrkEvent>::const_iterator const_iterator;


        MusicTrack() {}

        DSE::TrkEvent       & operator[]( size_t index )      {return m_events[index];}
        const DSE::TrkEvent & operator[]( size_t index )const {return m_events[index];}

        iterator       begin()      { return m_events.begin(); }
        const_iterator begin()const { return m_events.begin(); }

        iterator       end()      { return m_events.end(); }
        const_iterator end()const { return m_events.end(); }

        size_t size         ()const       { return m_events.size();   }
        void   reserve      ( size_t sz ) { m_events.reserve(sz);     }
        void   resize       ( size_t sz ) { m_events.resize(sz);      } 
        void   shrink_to_fit()            { m_events.shrink_to_fit(); }

        void push_back( DSE::TrkEvent && ev ) { m_events.push_back(ev); }
        void push_back( DSE::TrkEvent ev )    { m_events.push_back(std::move(ev)); }

        /*
            Get the DSE events for this track
        */
        std::vector<DSE::TrkEvent>       & getEvents()      { return m_events; }
        const std::vector<DSE::TrkEvent> & getEvents()const { return m_events; }

        /*
            Get or set the MIDI channel that was assigned to this track.
        */
        void    SetMidiChannel( uint8_t chan ) { m_midichan = chan; }
        uint8_t GetMidiChannel()const          { return m_midichan; }

    private:
        uint8_t                    m_midichan; //The channel of the track
        std::vector<DSE::TrkEvent> m_events;
    };

    /*****************************************************************************************
        MusicSequence
            Contains data for a single musical sequence from the PMD2 games.
            Contains events, and music parameters, along with sample information and mapping.
    *****************************************************************************************/
    class MusicSequence
    {
    public:
        MusicSequence( std::vector< MusicTrack > && tracks, 
                       DSE::DSE_MetaMusicSeq     && meta,
                       PresetBank                 * presets = nullptr )
            :m_meta(meta), m_tracks(tracks), m_pPresetBank(presets)
        {}

        PresetBank * presetbnk()                                             { return m_pPresetBank; }
        void         presetbnk( PresetBank * ptrbnk )                        { m_pPresetBank = ptrbnk; }

        DSE::DSE_MetaMusicSeq       & metadata()                                 { return m_meta; }
        const DSE::DSE_MetaMusicSeq & metadata()const                            { return m_meta; }
        void                      metadata( const DSE::DSE_MetaMusicSeq & data ) { m_meta = data; }
        void                      metadata( DSE::DSE_MetaMusicSeq && data )      { m_meta = data; }

        size_t                             getNbTracks()const                { return m_tracks.size(); }

        MusicTrack      & track( size_t index )             { return m_tracks[index]; }
        const MusicTrack & track( size_t index )const        { return m_tracks[index]; }

        MusicTrack      & operator[]( size_t index )        { return m_tracks[index]; }
        const MusicTrack & operator[]( size_t index )const   { return m_tracks[index]; }

        //Print the entire content of the music sequence
        std::string tostr()const;

        //Print statistics on the music sequence
        std::string printinfo()const;

    private:
        DSE::DSE_MetaMusicSeq    m_meta;
        std::vector<MusicTrack>  m_tracks;
        PresetBank             * m_pPresetBank;
    };

//====================================================================================================
//  Specialized Loaders/Exporters
//====================================================================================================
    /*
        BatchAudioLoader
            Used to load an entire set of smd+swd pairs all refering to a master bank.
            Just like what PMD2 uses.

            Can also be used to load the master bank, and then either a single or more smd+swd pairs.
    */
    class BatchAudioLoader
    {
    public:
        typedef std::pair< MusicSequence, PresetBank > smdswdpair_t;


        /*
            mergedInstData
                Structure containing references to presets, ordered by banks. While also maintaining a list of references to 
                the files those were originally from.
                This allows handling conflicting Preset ID from multiple files.
        */
        struct mergedInstData
        {
            typedef uint16_t                             presetid_t;           //The ID of a DSE Preset
            typedef uint16_t                             conpresindex_t;       //The index of a conflicting preset within a conflictingpresets_t list!
            typedef std::vector<ProgramInfo*>         conflictingpresets_t; //A list of pointers to presets sharing the same preset ID

            /*
                GetPresetByFile
                    For a given file index, within the list of loaded swd files, returns the correct preset at "presid".
            */
            inline ProgramInfo* GetPresetByFile( uint16_t fileindex, presetid_t presid )
            {
                return mergedpresets[ presid ][ filetopreset[fileindex].at(presid) ];
            }

            /*
                GetPresetByBank
                    For a given bank, returns the preset at preset ID !
            */
            inline ProgramInfo* GetPresetByBank( uint16_t bank, presetid_t presid )
            {
                return mergedpresets[presid][bank];
            }

            std::vector<conflictingpresets_t>                   mergedpresets; //Each slots in the vector correspond to a DSE Preset ID.
                                                                               //Each slot contains a vector of pointers to "ProgramInfo"
                                                                               // objects. Each of those pointers is a Preset that shares the 
                                                                               // same preset ID as the others in this "row".
                                                                               // So if mergedpresets[Y][X], Y is the DSE PresetID,
                                                                               // and X is a "bank" for this ID, with a different instrument that shares the same DSE Preset ID.

            std::vector< std::map<presetid_t, conpresindex_t> > filetopreset;  //Each slot in the vector is a file from the m_pair vector.
                                                                               //Each key values in the map, is a preset ID.
                                                                               //Each value for each key value is the "bank" or the index within the 
                                                                               // conflictingpresets_t list of presets sharing the same preset ID
        };

    //-----------------------------
    // Construction
    //-----------------------------
        /*
            mbank : Path to Master SWD Bank to load.
        */
        BatchAudioLoader( const std::string & mbank );

    //-----------------------------
    // Loading Methods
    //-----------------------------
        void LoadMasterBank();
        void LoadMasterBank( const std::string & mbank );

        void LoadSmdSwdPair( const std::string & smd, const std::string & swd );

    //-----------------------------
    // Exporting Methods
    //-----------------------------
        /*
            Builds a single soundfont from the master bank's samples, and from the
            individual swds from each smd+swd pairs.
            Any duplicate presets are ignored if they're identical, or they're placed into
            other banks for the same preset ID.
        */
        mergedInstData ExportSoundfont( const std::string & destf )const;

        /*
            Does the same as the "ExportSoundfont" method, but additionnaly also
            exports all loaded smd as MIDIs, with the appropriate bank events to use
            the correct instrument presets.
        */
        void ExportSoundfontAndMIDIs( const std::string & destdir )const;

        /*
            Exports a MIDI along with a minimal soundfont for the specified smd+swd pair.
        */
        //

        /*
            Attempts to export as a sounfont, following the General MIDI standard instrument patch list.

            dsetogm : A map consisting of a list of filenames associated to a vector where each indexes correspond to a
                      dse preset entry ID (AKA instrument ID), and where the integer at that index correspond to the 
                      GM patch number to attribute it during conversion.
        */
        void ExportSoundfontAsGM( const std::string & destf, const std::map< std::string, std::vector<int> > & dsetogm )const;

    private:

        /*
            GetSizeLargestPrgiChunk
                Search the entire list of loaded swdl files, and pick the largest prgi chunk.
                This will avoid crashing when a song has more presets than the master bank !
        */
        uint16_t GetSizeLargestPrgiChunk()const;

        /*
            Read all loaded smd/swd pairs and compile a list of instruments presets info, and a list of
            where each instrument preset from each smd+swd pair was put into that compiled instrument info list.

            The list can have several entries for the same instrument slot, and they're all
            pointers, so any null instrument preset slot will be represented as such.

            The list containing the position of each presets from each smd+swd pairs uses a map to associate 
            an instrument ID (uint16_t) to a position in the second dimension of the merged instrument info list.
        */
        mergedInstData PrepareMergedInstrumentTable()const;

    private:
        std::string               m_mbankpath;

        PresetBank                m_master;
        std::vector<smdswdpair_t> m_pairs;

        BatchAudioLoader( const BatchAudioLoader & )           = delete;
        BatchAudioLoader & operator=(const BatchAudioLoader& ) = delete;
    };

//====================================================================================================
// Functions
//====================================================================================================

    //-------------------
    //  Audio Loaders
    //-------------------

    // ======================= 1. Main Bank + Sequences + RefBanks ( smdl or sedl ) ( mainbank.swd + 001.smd + 001.swd ) =======================
    std::pair< PresetBank, std::vector<std::pair<MusicSequence,PresetBank>> > LoadBankAndPairs( const std::string & bank, const std::string & smdroot, const std::string & swdroot );

    // ======================= 2. 1 Sequence + 1 Bank ( 001.smd + 001.swd ) =======================
    std::pair<MusicSequence,PresetBank> LoadSmdSwdPair( const std::string & smd, const std::string & swd );

    // ======================= 3. Individual Bank ( bank.swd ) =======================
    PresetBank LoadSwdBank( const std::string & file );

    // ======================= 4. Sequence only =======================
    MusicSequence LoadSequence( const std::string & file );

    //-------------------
    //  Audio Exporters
    //-------------------

    /*
        Export all sequences 
    */

    /*
        Exports a Sequence as MIDI and a corresponding SF2 file if the PresetBank's samplebank ptr is not null.
    */
    bool ExportSeqAndBank( const std::string & filename, const MusicSequence & seq, const PresetBank & bnk );
    bool ExportSeqAndBank( const std::string & filename, const std::pair<MusicSequence,PresetBank> & seqandbnk );

    /*
        Export the PresetBank to a directory as XML and WAV samples.
    */
    void ExportPresetBank( const std::string & directory, const PresetBank & bnk );

    /*
        To use the ExportSequence,
    */

};};


//Ostream operators


#endif 