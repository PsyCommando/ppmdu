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

//# TODO: Move implementation into CPP !!

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
        typedef std::vector<smpldata_t>::iterator       iterator;
        typedef std::vector<smpldata_t>::const_iterator const_iterator;

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

        inline iterator       begin()     { return m_SampleData.begin(); }
        inline const_iterator begin()const{ return m_SampleData.begin(); }
        inline iterator       end()       { return m_SampleData.end();   }
        inline const_iterator end()const  { return m_SampleData.end();   }
        inline size_t         size()const { return NbSlots(); }

    public:
        //Info

        //Nb of sample slots with or without data
        inline size_t                       NbSlots()const     { return m_SampleData.size(); } 

        //Access
        bool                                IsInfoPresent      ( unsigned int index )const { return sampleInfo(index) != nullptr; }
        bool                                IsDataPresent      ( unsigned int index )const { return sample(index)     != nullptr; }

        inline DSE::WavInfo               * sampleInfo         ( unsigned int index )      
        { 
            if( m_SampleData.size() > index )
                return m_SampleData[index].pinfo_.get(); 
            else
                return nullptr;
        }

        inline DSE::WavInfo const         * sampleInfo         ( unsigned int index )const 
        { 
            if( m_SampleData.size() > index )
                return m_SampleData[index].pinfo_.get(); 
            else
                return nullptr;
        }


        inline std::vector<uint8_t>       * sample             ( unsigned int index )      
        { 
            if( m_SampleData.size() > index )
                return m_SampleData[index].pdata_.get(); 
            else 
                return nullptr;
        }

        inline const std::vector<uint8_t> * sample             ( unsigned int index )const 
        { 
            if( m_SampleData.size() > index )
                return m_SampleData[index].pdata_.get(); 
            else 
                return nullptr;
        }

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
        std::vector<smpldata_t> m_SampleData;
    };

    /*****************************************************************************************
        KeygroupList
            Encapsulate and validate keygroups.
    *****************************************************************************************/
    class KeyGroupList
    {
    public:
        typedef std::vector<DSE::KeyGroup>::iterator            iterator;
        typedef std::vector<DSE::KeyGroup>::const_iterator      const_iterator;
        typedef std::vector<DSE::KeyGroup>::reference           reference;
        typedef std::vector<DSE::KeyGroup>::const_reference     const_reference;

        KeyGroupList()
        {}

        KeyGroupList( const std::vector<DSE::KeyGroup> & cpy )
            :m_groups(cpy)
        {}

        inline size_t             size()const       { return m_groups.size();  }
        inline bool               empty()const      { return m_groups.empty(); }

        inline iterator           begin()           { return m_groups.begin(); }
        inline const_iterator     begin()const      { return m_groups.begin(); }
        inline iterator           end()             { return m_groups.end(); }
        inline const_iterator     end()const        { return m_groups.end(); }

        inline reference       & front()            { return m_groups.front(); }
        inline const_reference & front()const       { return m_groups.front(); }
        inline reference       & back()             { return m_groups.back(); }
        inline const_reference & back()const        { return m_groups.back(); }

        //Validated Kgrp access!
        inline DSE::KeyGroup & operator[]( size_t kgrp )
        {
            if( kgrp < size() )
                return m_groups[kgrp];
            else
                return m_groups.front(); //Return the gloabal keygroup if the kgrp number is invalid (This might differ in the way the DSE engine handles out of range kgrp values)
        }

        inline const DSE::KeyGroup & operator[]( size_t kgrp )const 
        {
            if( kgrp < size() )
                return m_groups[kgrp];
            else
                return m_groups.front(); //Return the gloabal keygroup if the kgrp number is invalid (This might differ in the way the DSE engine handles out of range kgrp values)
        }

        //Access to raw vector
        inline std::vector<DSE::KeyGroup>       & GetVector()       { return m_groups; }
        inline const std::vector<DSE::KeyGroup> & GetVector()const  { return m_groups; }

    private:
        std::vector<DSE::KeyGroup>  m_groups;
    };


    /*****************************************************************************************
        ProgramBank
            Contains the entries for each program contained in a SWD file, along with the
            file's keygroup table!
    *****************************************************************************************/
    class ProgramBank
    {
    public:
        typedef std::unique_ptr<DSE::ProgramInfo> ptrprg_t;

        ProgramBank( std::vector<ptrprg_t> && prgminf, std::vector<DSE::KeyGroup> && kgrp )
            :m_prgminfoslots(std::move(prgminf)),  //MSVC is too derp to use the right constructor..
             m_Groups(kgrp)
        {}

        ProgramBank( ProgramBank && mv )
        {
            m_prgminfoslots = std::move(mv.m_prgminfoslots);
            m_Groups        = std::move(mv.m_Groups);
        }

        ProgramBank & operator=( ProgramBank&& mv )
        {
            m_prgminfoslots = std::move(mv.m_prgminfoslots);
            m_Groups        = std::move(mv.m_Groups);
            return *this;
        }

        ptrprg_t                          & operator[]( size_t index )         { return m_prgminfoslots[index]; }
        const ptrprg_t                    & operator[]( size_t index )const    { return m_prgminfoslots[index]; }

        std::vector<ptrprg_t>             & PrgmInfo()      { return m_prgminfoslots; }
        const std::vector<ptrprg_t>       & PrgmInfo()const { return m_prgminfoslots; }

        KeyGroupList                      & Keygrps()       { return m_Groups;        }
        const KeyGroupList                & Keygrps()const  { return m_Groups;        }

    private:
        std::vector<ptrprg_t>       m_prgminfoslots;
        //std::vector<DSE::KeyGroup>  m_Groups;
        KeyGroupList                m_Groups;

        //Can't copy
        ProgramBank( const ProgramBank& );
        ProgramBank & operator=( const ProgramBank& );
    };

    /*****************************************************************************************
        PresetBank
            Is the combination of a SampleBank, and an ProgramBank.
            Or just an instrument bank if samples are not available
    *****************************************************************************************/
    class PresetBank
    {
    public:

        typedef std::shared_ptr<ProgramBank>   ptrprg_t;
        typedef std::weak_ptr<ProgramBank>     wptrprg_t;

        typedef std::shared_ptr<SampleBank>       ptrsmpl_t;
        typedef std::weak_ptr<SampleBank>         wptrsmpl_t;

        PresetBank()
        {}

        PresetBank( DSE::DSE_MetaDataSWDL && meta, std::unique_ptr<ProgramBank> && pInstrument, std::unique_ptr<SampleBank>  && pSmpl )
            :m_pPrgbnk(std::move(pInstrument)), m_pSamples(std::move(pSmpl)), m_meta(std::move(meta))
        {}

        PresetBank( DSE::DSE_MetaDataSWDL && meta, std::unique_ptr<ProgramBank> && pInstrument )
            :m_pPrgbnk(std::move(pInstrument)), m_pSamples(nullptr), m_meta(std::move(meta))
        {}

        PresetBank( DSE::DSE_MetaDataSWDL && meta, std::unique_ptr<SampleBank> && pSmpl )
            :m_pPrgbnk(nullptr), m_pSamples(std::move(pSmpl)), m_meta(std::move(meta))
        {}

        PresetBank( PresetBank && mv )
        {
            m_pPrgbnk = std::move( mv.m_pPrgbnk );
            m_pSamples = std::move( mv.m_pSamples );
            m_meta     = std::move( mv.m_meta     );
        }

        PresetBank & operator=( PresetBank && mv )
        {
            m_pPrgbnk = std::move( mv.m_pPrgbnk );
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

        //Returns a weak_ptr to the program bank
        wptrprg_t                prgmbank()                                                { return m_pPrgbnk; }
        const wptrprg_t          prgmbank()const                                           { return m_pPrgbnk; }
        void                      prgmbank( ptrprg_t    && bank)                           { m_pPrgbnk = std::move(bank); }
        //void                      prgmbank( ProgramBank * bank)                           { m_pPrgbnk.reset(bank); }
        void                      prgmbank( std::unique_ptr<ProgramBank>     && bank)       { m_pPrgbnk = std::move(bank); }

    private:
        //Can't copy
        PresetBank( const PresetBank& );
        PresetBank& operator=( const PresetBank& );

        DSE::DSE_MetaDataSWDL m_meta;
        ptrprg_t             m_pPrgbnk;  //A program bank may not be shared by many
        ptrsmpl_t             m_pSamples; //A sample bank may be shared by many
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

        bool   empty        ()const       { return m_events.empty();  }
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

        //Makes it possible to iterate through the tracks using a foreach loop
        typedef std::vector<MusicTrack>::iterator       iterator;
        typedef std::vector<MusicTrack>::const_iterator const_iterator;

        inline iterator       begin()      { return move(m_tracks.begin()); }
        inline const_iterator begin()const { return move(m_tracks.begin()); }
        inline iterator       end()        { return move(m_tracks.end()); }
        inline const_iterator end()const   { return move(m_tracks.end()); }
        inline bool           empty()const { return m_tracks.empty(); }

    private:
        DSE::DSE_MetaDataSMDL    m_meta;
        std::vector<MusicTrack>  m_tracks;
        PresetBank             * m_pPresetBank;
    };
};

#endif