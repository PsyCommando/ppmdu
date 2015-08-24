#ifndef DSE_CONTAINERS_HPP
#define DSE_CONTAINERS_HPP
/*
dse_containers.hpp
2015/08/23
psycommando@gmail.com
Description: Several container objects for holding the content of loaded DSE files!
*/
#include <dse/dse_common.hpp>
#include <dse/dse_sequence.hpp>
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <future>

namespace DSE
{
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
        typedef std::unique_ptr<DSE::ProgramInfo> ptrinst_t;

        InstrumentBank( std::vector<ptrinst_t> && instinf, std::vector<DSE::KeyGroup> && kgrp )
            :m_instinfoslots(std::move(instinf)),  //MSVC is too derp to use the right constructor..
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

        std::vector<DSE::KeyGroup>        & keygrps()       { return m_Groups;        }
        const std::vector<DSE::KeyGroup>  & keygrps()const  { return m_Groups;        }

    private:
        std::vector<ptrinst_t>      m_instinfoslots;
        std::vector<DSE::KeyGroup>  m_Groups;

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

        PresetBank( DSE::DSE_MetaDataSWDL && meta, std::unique_ptr<InstrumentBank> && pInstrument, std::unique_ptr<SampleBank>  && pSmpl )
            :m_pInstbnk(std::move(pInstrument)), m_pSamples(std::move(pSmpl)), m_meta(std::move(meta))
        {}

        PresetBank( DSE::DSE_MetaDataSWDL && meta, std::unique_ptr<InstrumentBank> && pInstrument )
            :m_pInstbnk(std::move(pInstrument)), m_pSamples(nullptr), m_meta(std::move(meta))
        {}

        PresetBank( DSE::DSE_MetaDataSWDL && meta, std::unique_ptr<SampleBank> && pSmpl )
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


        DSE::DSE_MetaDataSWDL       & metadata()                                                 { return m_meta; }
        const DSE::DSE_MetaDataSWDL & metadata()const                                            { return m_meta; }
        void                      metadata( const DSE::DSE_MetaDataSWDL & data )                 { m_meta = data; }
        void                      metadata( DSE::DSE_MetaDataSWDL && data )                      { m_meta = data; }
        
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

        DSE::DSE_MetaDataSWDL m_meta;
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
                       DSE::DSE_MetaDataSMDL     && meta,
                       PresetBank                 * presets = nullptr )
            :m_meta(meta), m_tracks(tracks), m_pPresetBank(presets)
        {}

        PresetBank * presetbnk()                                             { return m_pPresetBank; }
        void         presetbnk( PresetBank * ptrbnk )                        { m_pPresetBank = ptrbnk; }

        DSE::DSE_MetaDataSMDL       & metadata()                                 { return m_meta; }
        const DSE::DSE_MetaDataSMDL & metadata()const                            { return m_meta; }
        void                      metadata( const DSE::DSE_MetaDataSMDL & data ) { m_meta = data; }
        void                      metadata( DSE::DSE_MetaDataSMDL && data )      { m_meta = data; }

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
        DSE::DSE_MetaDataSMDL    m_meta;
        std::vector<MusicTrack>  m_tracks;
        PresetBank             * m_pPresetBank;
    };
};

#endif