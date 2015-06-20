#ifndef PMD2_AUDIO_DATA_HPP
#define PMD2_AUDIO_DATA_HPP
/*
pmd2_audio_data.hpp
2015/05/20
psycommando@gmail.com
Description: Containers and utilities for data parsed from PMD2's audio, and sequencer files.
*/
#include <ppmdu/fmts/dse_common.hpp>
#include <cstdint>
#include <vector>
#include <string>
#include <memory>

namespace pmd2 { namespace audio 
{
//====================================================================================================
//  Typedefs
//====================================================================================================

//====================================================================================================
//  Constants
//====================================================================================================

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
    class KeyGroups
    {
    public:

        struct KeyGroup
        {

        };

        KeyGroups()
        {
        }

    private:
        std::vector<KeyGroup> m_Groups;
    };

    /*****************************************************************************************
        InstrumentInfo
            Contains data for a single instrument.
    *****************************************************************************************/
    class InstrumentInfo
    {
    public:
        InstrumentInfo(){}


    private:
    };

    /*****************************************************************************************
        SampleBank
            This class is used to maintain references to sample data. 
            The samples in this are refered to by entries in a SampleMap instance.
    *****************************************************************************************/
    class SampleBank
    {
    public:
        typedef std::vector<uint8_t>                smpldata_t;
        typedef std::unique_ptr<DSE::WavInfo>       wavinfoptr_t;
        typedef std::unique_ptr<const DSE::WavInfo> constwavinfoptr_t;

        SampleBank( std::vector<wavinfoptr_t> && wavitbl, std::vector<smpldata_t> && smplsraw )
            :m_wavinfotbl(wavitbl), m_SampleData(smplsraw)
        {}

        //Info
        int NbSamples()const { return m_SampleData.size(); } //Nb of sample slots with actual data.
        int NbSlots  ()const { return m_wavinfotbl.size(); } //Nb of slots for samples, empty or not.

        //Access
        bool                 IsSampleInfoPresent( unsigned int index )const { return m_wavinfotbl[index] != nullptr; }

        DSE::WavInfo       * sampleInfo         ( unsigned int index )      { return m_wavinfotbl[index].get(); }
        DSE::WavInfo const * sampleInfo         ( unsigned int index )const { return m_wavinfotbl[index].get(); }

        smpldata_t         & sample             ( unsigned int index )      { return m_SampleData[index]; }
        const smpldata_t   & sample             ( unsigned int index )const { return m_SampleData[index]; }

        smpldata_t         & operator[]         ( unsigned int index )      { return m_SampleData[index]; }
        const smpldata_t   & operator[]         ( unsigned int index )const { return m_SampleData[index]; }

    private:
        std::vector<wavinfoptr_t> m_wavinfotbl; //Data on the samples
        std::vector<smpldata_t>   m_SampleData; //Actual samples
    };


    /*
        SmplBnkWrap
            This is used to create an abstraction layer over the ownership of a sample bank.

            So that the user of the class doesn't need to know or care whether they own the
            sample bank or merely refer to an existing bank.

            This is neccessary because SWDL files may have samples, or may refer to a common
            sample bank that is shared among other such SWDL.. However there are no way of
            knowing this in advance.

            Pass a unique_ptr if the object owns the samplebank!
            Pass a regular pointer if the object only refers to the samplebank!
    */
    class SmplBnkWrap
    {
    public:

        SmplBnkWrap()
            :m_pShared(nullptr), m_pOwned(nullptr)
        {}

        SmplBnkWrap( std::unique_ptr<SampleBank> && ptrowned )
            :m_pShared(nullptr), m_pOwned(ptrowned)
        {}

        SmplBnkWrap( SampleBank * ptrshare )
            :m_pShared(ptrshare), m_pOwned(nullptr)
        {}

        SampleBank & operator*()
        {
            return *(get());
        }

        SampleBank * get()
        {
            if( m_pOwned != nullptr )
                return m_pOwned.get();
            else
                return m_pShared;
        }

        void set( SampleBank * ptrshare )
        {
            if( m_pOwned != nullptr )
                m_pOwned.reset(nullptr);

            m_pShared = ptrshare;
        }

        void set( std::unique_ptr<SampleBank> && ptrowned )
        {
            if( m_pShared != nullptr )
                m_pShared = nullptr;

            m_pOwned.reset(nullptr); //have to do this before moving, to make sure the previously owned object is deleted!
            m_pOwned = ptrowned;
        }

        inline void operator=( SampleBank                   * ptrshare ) { set(ptrshare); }
        inline void operator=( std::unique_ptr<SampleBank> && ptrowned ) { set(ptrowned); }

        inline bool IsOwned ()const { return m_pOwned != nullptr;  }
        inline bool IsShared()const { return m_pShared != nullptr; }
        inline bool IsNull  ()const { return (m_pShared == nullptr) && (m_pOwned == nullptr); }

    private:
        std::unique_ptr<SampleBank>  m_pOwned;
        SampleBank                 * m_pShared;
    };


    /*****************************************************************************************
        InstrumentBank
            Contains the entries for each instruments
            Data on what samples an instrument uses, how to play those, the key mapping, etc..
    *****************************************************************************************/
    class InstrumentBank
    {
    public:
        typedef std::unique_ptr<InstrumentInfo> ptrinst_t;

        InstrumentBank(){}

        ptrinst_t       & operator[]( size_t index )      { return m_instinfoslots[index]; }
        const ptrinst_t & operator[]( size_t index )const { return m_instinfoslots[index]; }

    private:
        std::vector<ptrinst_t> m_instinfoslots;
    };

    /*****************************************************************************************
        PresetBank
            Is the combination of a SampleBank, and an InstrumentBank.
            Or just an instrument bank if samples are not available
    *****************************************************************************************/
    class PresetBank
    {
    public:
        typedef std::unique_ptr<InstrumentBank> ptrinst_t;
        typedef std::unique_ptr<SampleBank>     ptrsmpl_t;
        
        PresetBank( InstrumentBank    &  instbank, 
                    DSE::DSE_MetaData && meta )
            :m_pInstbnk(instbank), m_meta(meta)
        {}

        //Constructor for all varieties of preset ptr
        PresetBank( InstrumentBank    &  instbank, 
                    DSE::DSE_MetaData && meta,
                    ptrsmpl_t         && samplesbank )
            :m_pInstbnk(instbank), m_pSamples(samplesbank), m_meta(meta)
        {}

        PresetBank( InstrumentBank    &  instbank, 
                    DSE::DSE_MetaData && meta,
                    SampleBank         * samplesbank )
            :m_pInstbnk(instbank), m_pSamples(samplesbank), m_meta(meta)
        {}

        PresetBank( InstrumentBank    & instbank, 
                    DSE::DSE_MetaData &&meta,
                    SmplBnkWrap       &&samplesbank )
            :m_pInstbnk(instbank), m_pSamples(samplesbank), m_meta(meta)
        {}

        DSE::DSE_MetaData       & metadata()                                 { return m_meta; }
        const DSE::DSE_MetaData & metadata()const                            { return m_meta; }
        void                      metadata( const DSE::DSE_MetaData & data ) { m_meta = data; }
        void                      metadata( DSE::DSE_MetaData && data )      { m_meta = data; }

        SmplBnkWrap              & smplbank()                                 { return m_pSamples; }
        const SmplBnkWrap        & smplbank()const                            { return m_pSamples; }
        void                      smplbank( SmplBnkWrap && samplesbank)       { m_pSamples = samplesbank; }
        void                      smplbank( SampleBank   * samplesbank)       { m_pSamples = samplesbank; }
        void                      smplbank( ptrsmpl_t   && samplesbank)       { m_pSamples = samplesbank; }

        ptrinst_t               & instbank()                                  { return m_pInstbnk; }
        const ptrinst_t         & instbank()const                             { return m_pInstbnk; }
        void                    & instbank( ptrinst_t && pinst )              { m_pInstbnk = pinst;}


    private:
        DSE::DSE_MetaData   m_meta;
        ptrinst_t           m_pInstbnk; //An instrument bank may not be shared by many
        SmplBnkWrap         m_pSamples; //A sample bank may be shared by many
    };

    /*****************************************************************************************
        MusicSequence
            Contains data for a single musical sequence from the PMD2 games.
            Contains events, and music parameters, along with sample information and mapping.
    *****************************************************************************************/
    class MusicSequence
    {
    public:
        MusicSequence( std::vector< std::vector<DSE::TrkEvent> > && tracks, 
                       DSE::DSE_MetaData                         && meta,
                       PresetBank                                 * presets = nullptr )
            :m_meta(meta), m_tracks(tracks), m_pPresetBank(presets)
        {}

        PresetBank * presetbnk()                                             { return m_pPresetBank; }
        void         presetbnk( PresetBank * ptrbnk )                        { m_pPresetBank = ptrbnk; }

        DSE::DSE_MetaData       & metadata()                                 { return m_meta; }
        const DSE::DSE_MetaData & metadata()const                            { return m_meta; }
        void                      metadata( const DSE::DSE_MetaData & data ) { m_meta = data; }
        void                      metadata( DSE::DSE_MetaData && data )      { m_meta = data; }


        std::vector<DSE::TrkEvent>       & track( size_t index )             { return m_tracks[index]; }
        const std::vector<DSE::TrkEvent> & track( size_t index )const        { return m_tracks[index]; }

        std::vector<DSE::TrkEvent>       & operator[]( size_t index )        { return m_tracks[index]; }
        const std::vector<DSE::TrkEvent> & operator[]( size_t index )const   { return m_tracks[index]; }

    private:
        DSE::DSE_MetaData                          m_meta;
        std::vector< std::vector<DSE::TrkEvent> >  m_tracks;
        PresetBank                               * m_pPresetBank;
    };

//====================================================================================================
// Functions
//====================================================================================================

    //Audio loaders

    //1. Main Bank + Sequences + RefBanks ( smdl or sedl ) ( mainbank.swd + 001.smd + 001.swd )

    //2. 1 Sequence + 1 Bank ( 001.smd + 001.swd )

    //3. Individual Bank ( bank.swd )

    //4. 

    //adpcm encoding/decoding

};};

#endif 