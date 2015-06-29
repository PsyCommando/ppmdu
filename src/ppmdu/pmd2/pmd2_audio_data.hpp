#ifndef PMD2_AUDIO_DATA_HPP
#define PMD2_AUDIO_DATA_HPP
/*
pmd2_audio_data.hpp
2015/05/20
psycommando@gmail.com
Description: Containers and utilities for data parsed from PMD2's audio, and sequencer files.
*/
#include <ppmdu/fmts/dse_common.hpp>
#include <ppmdu/fmts/dse_sequence.hpp>
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
    static const std::string SMDL_FileExtension = "smd";
    static const std::string SWDL_FileExtension = "swd";
    static const std::string SEDL_FileExtension = "sed";


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
            :m_wavinfotbl(std::move(wavitbl)), m_SampleData(std::move(smplsraw))
        {}

        SampleBank( SampleBank && mv )
        {
            m_wavinfotbl = std::move( mv.m_wavinfotbl );
            m_SampleData = std::move( mv.m_SampleData );
        }

        SampleBank & operator=( SampleBank && mv )
        {
            m_wavinfotbl = std::move( mv.m_wavinfotbl );
            m_SampleData = std::move( mv.m_SampleData );
            return *this;
        }

        SampleBank( const SampleBank & other )
        {
            DoCopyFrom(other);
        }

        SampleBank & operator=( const SampleBank & other )
        {
            DoCopyFrom(other);
            return *this;
        }

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

        void DoCopyFrom( const SampleBank & other )
        {
            m_wavinfotbl.resize( other.m_wavinfotbl.size() );

            for( size_t i = 0; i < other.m_wavinfotbl.size(); ++i  )
            {
                if( other.m_wavinfotbl[i] != nullptr )
                    m_wavinfotbl[i].reset( new DSE::WavInfo( *(other.m_wavinfotbl[i]) ) ); //Copy each objects and make a pointer
            }

            m_SampleData.resize( other.m_SampleData.size() );
            std::copy( other.m_SampleData.begin(), other.m_SampleData.end(), m_SampleData.begin() );
        }

    private:
        std::vector<wavinfoptr_t> m_wavinfotbl; //Data on the samples
        std::vector<smpldata_t>   m_SampleData; //Actual samples
    };


    /*
        OwnershipWrap
            This is used to create an abstraction layer over the ownership of a dynamically allocated object.

            So that the user of the class doesn't need to know or care whether they own the
            object or merely refer to an existing one.

            This is neccessary because SWDL files may have samples, or may refer to a common
            sample bank that is shared among other such SWDL, for example.. However there are no way of
            knowing this in advance.

            Pass a unique_ptr if the object owns the object!
            Pass a regular pointer if the object only refers to the object!
    */
    template<class _OwnedTy>
        class OwnershipWrap
    {
    public:
        typedef _OwnedTy owned_t;

        OwnershipWrap()
            :m_pShared(nullptr), m_pOwned(nullptr)
        {}

        explicit OwnershipWrap( std::unique_ptr<owned_t> && ptrowned )
            :m_pShared(nullptr), m_pOwned(ptrowned)
        {}

        explicit OwnershipWrap( _OwnedTy * ptrshare )
            :m_pShared(ptrshare), m_pOwned(nullptr)
        {}

        OwnershipWrap( std::nullptr_t )
            :m_pShared(nullptr), m_pOwned(nullptr)
        {}

        OwnershipWrap( OwnershipWrap<_OwnedTy> && mv )
        {
            m_pOwned  = std::move( mv.m_pOwned  );
            m_pShared = std::move( mv.m_pShared );
            
            //Ensure we leave the other object in a consistent state
            mv.m_pOwned  = nullptr;
            mv.m_pShared = nullptr;
        }

        OwnershipWrap<_OwnedTy> & operator=( OwnershipWrap<_OwnedTy> && mv)
        {
            m_pOwned  = std::move( mv.m_pOwned  );
            m_pShared = std::move( mv.m_pShared );

            //Ensure we leave the other object in a consistent state
            mv.m_pOwned  = nullptr;
            mv.m_pShared = nullptr;
            return *this;
        }

        owned_t & operator*()
        {
            return *(get());
        }

        owned_t * get()
        {
            if( m_pOwned != nullptr )
                return m_pOwned.get();
            else
                return m_pShared;
        }

        void set( owned_t * ptrshare )
        {
            if( m_pOwned != nullptr )
                m_pOwned.reset(nullptr);

            m_pShared = ptrshare;
        }

        void set( std::unique_ptr<owned_t> && ptrowned )
        {
            if( m_pShared != nullptr )
                m_pShared = nullptr;

            m_pOwned.reset(nullptr); //have to do this before moving, to make sure the previously owned object is deleted!
            m_pOwned = std::move(ptrowned);
        }

        inline void operator=( owned_t                    * ptrshare ) { set(ptrshare); }
        inline void operator=( std::unique_ptr<_OwnedTy> && ptrowned ) { set(std::move(ptrowned)); }
        inline void operator=( std::nullptr_t )                        { set(nullptr); }

        inline bool IsOwned ()const { return m_pOwned != nullptr;  }
        inline bool IsShared()const { return m_pShared != nullptr; }
        inline bool IsNull  ()const { return (m_pShared == nullptr) && (m_pOwned == nullptr); }

    private:
        //No copies !
        OwnershipWrap                      ( const OwnershipWrap<_OwnedTy> & );
        OwnershipWrap<_OwnedTy> & operator=( const OwnershipWrap<_OwnedTy> & );


        std::unique_ptr<owned_t>  m_pOwned;
        owned_t                 * m_pShared;
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

        InstrumentBank()
        {}

        InstrumentBank( InstrumentBank && mv )
        {
            m_instinfoslots = std::move(mv.m_instinfoslots);
        }

        InstrumentBank & operator=( InstrumentBank&& mv )
        {
            m_instinfoslots = std::move(mv.m_instinfoslots);
            return *this;
        }

        ptrinst_t       & operator[]( size_t index )      { return m_instinfoslots[index]; }
        const ptrinst_t & operator[]( size_t index )const { return m_instinfoslots[index]; }

    private:
        std::vector<ptrinst_t> m_instinfoslots;

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
        typedef OwnershipWrap<InstrumentBank>   wrapinst_t;
        typedef OwnershipWrap<SampleBank>       wrapsmpl_t;

        PresetBank( DSE::DSE_MetaData && meta, wrapinst_t && pInstrument, wrapsmpl_t && pSmpl )
            :m_pInstbnk(std::move(pInstrument)), m_pSamples(std::move(pSmpl)), m_meta(std::move(meta))
        {}

        PresetBank( DSE::DSE_MetaData && meta, wrapinst_t && pInstrument )
            :m_pInstbnk(std::move(pInstrument)), m_pSamples(nullptr), m_meta(std::move(meta))
        {}

        PresetBank( DSE::DSE_MetaData && meta, wrapsmpl_t && pSmpl )
            :m_pInstbnk(nullptr), m_pSamples(std::move(pSmpl)), m_meta(std::move(meta))
        {}


        DSE::DSE_MetaData       & metadata()                                                 { return m_meta; }
        const DSE::DSE_MetaData & metadata()const                                            { return m_meta; }
        void                      metadata( const DSE::DSE_MetaData & data )                 { m_meta = data; }
        void                      metadata( DSE::DSE_MetaData && data )                      { m_meta = data; }
        
        wrapsmpl_t               & smplbank()                                                { return m_pSamples; }
        const wrapsmpl_t         & smplbank()const                                           { return m_pSamples; }
        void                      smplbank( wrapsmpl_t && samplesbank)                       { m_pSamples = std::move(samplesbank); }
        void                      smplbank( SampleBank  * samplesbank)                       { m_pSamples = samplesbank; }
        void                      smplbank( std::unique_ptr<SampleBank>  && samplesbank)     { m_pSamples = std::move(samplesbank); }

        wrapinst_t               & instbank()                                                { return m_pInstbnk; }
        const wrapinst_t         & instbank()const                                           { return m_pInstbnk; }
        void                      instbank( wrapinst_t    && instbank)                       { m_pInstbnk = std::move(instbank); }
        void                      instbank( InstrumentBank * instbank)                       { m_pInstbnk = instbank; }
        void                      instbank( std::unique_ptr<InstrumentBank>     && instbank) { m_pInstbnk = std::move(instbank); }

    private:
        //Can't copy
        PresetBank( const PresetBank& );
        PresetBank& operator=( const PresetBank& );

        DSE::DSE_MetaData m_meta;
        wrapinst_t        m_pInstbnk; //An instrument bank may not be shared by many
        wrapsmpl_t        m_pSamples; //A sample bank may be shared by many
    };

    /*
        MusicTrack
    */
    class MusicTrack
    {
    public:
        typedef std::vector<DSE::TrkEvent>::iterator       iterator;
        typedef std::vector<DSE::TrkEvent>::const_iterator const_iterator;


        MusicTrack()
        {
        }

        DSE::TrkEvent operator[]( size_t index ) {return m_events[index];}

        iterator       begin()      { return m_events.begin(); }
        const_iterator begin()const { return m_events.begin(); }

        iterator       end()      { return m_events.end(); }
        const_iterator end()const { return m_events.end(); }

        void reserve( size_t sz ) { m_events.reserve(sz); }
        void resize(size_t sz) { m_events.resize(sz); } 
        void shrink_to_fit() {m_events.shrink_to_fit();}

        void push_back( DSE::TrkEvent && ev ) { m_events.push_back(ev); }
        void push_back( DSE::TrkEvent ev ) { m_events.push_back(std::move(ev)); }

        std::vector<DSE::TrkEvent> & getEvents() {return m_events;}
        const std::vector<DSE::TrkEvent> & getEvents()const {return m_events;}

        void SetMidiChannel( uint8_t chan ) {m_midichan = chan;}
        uint8_t GetMidiChannel()const {return m_midichan;}

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

        size_t                             getNbTracks()const                { return m_tracks.size(); }

        MusicTrack      & track( size_t index )             { return m_tracks[index]; }
        const MusicTrack & track( size_t index )const        { return m_tracks[index]; }

        MusicTrack      & operator[]( size_t index )        { return m_tracks[index]; }
        const MusicTrack & operator[]( size_t index )const   { return m_tracks[index]; }

        std::string tostr()const;

    private:
        DSE::DSE_MetaData        m_meta;
        std::vector<MusicTrack>  m_tracks;
        PresetBank             * m_pPresetBank;
    };

//====================================================================================================
// Functions
//====================================================================================================

    //Audio loaders

    //1. Main Bank + Sequences + RefBanks ( smdl or sedl ) ( mainbank.swd + 001.smd + 001.swd )

    //2. 1 Sequence + 1 Bank ( 001.smd + 001.swd )

    //3. Individual Bank ( bank.swd )

    //4. Sequence only
    MusicSequence LoadSequence( const std::string & file );

    //adpcm encoding/decoding

};};

#endif 