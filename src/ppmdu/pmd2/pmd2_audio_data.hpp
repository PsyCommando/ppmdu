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

    /*
        Contains several groups of notes each with an ID.
    */
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
        SampleBank
            This class is used to maintain references to sample data. 
            The samples in this are refered to by entries in a SampleMap instance.
    *****************************************************************************************/
    class SampleBank
    {
    public:
        typedef std::vector<uint8_t>          smpldata_t;
        typedef std::unique_ptr<DSE::WavInfo> wavinfoptr_t;



    private:

        std::vector<wavinfoptr_t> m_wavinfotbl; //Data on the samples
        std::vector<smpldata_t>   m_SampleData; //Actual samples
    };


    /*****************************************************************************************
        SampleMap
            Contains meta-data on samples. What notes they're mapped to, what sample they 
            refer to, and etc..

            All SampleMap refering to a SampleBank must be destroyed, before destroying the
            SampleBank.
    *****************************************************************************************/
    //class SampleMap
    //{
    //public:
    //    SampleMap( std::shared_ptr<SampleBank> samples = nullptr );

    //    void                        setSampleBank( std::shared_ptr<SampleBank> samples ) { m_pSamples = std::move(samples); }
    //    std::shared_ptr<SampleBank> getSampleBank()const                                 { return m_pSamples;               }
    //    SampleBank *                getSampleBankPtr()const                              { return m_pSamples.get();         }

    //private:

    //    std::shared_ptr<SampleBank> m_pSamples;
    //};

    /*
        Contains the entries for each instruments
    */
    class InstrumentBank
    {
    public:
        InstrumentBank();

    private:
    };



    /*
        Is the combination of a SampleBank, and an InstrumentBank.
        Or just an instrument bank if samples are not available
    */
    class PresetBank
    {
    public:
        PresetBank( const InstrumentBank & instbank, SampleBank * samplesbank = nullptr );

        SampleBank *           SamplesBank()                         { return m_pSamples; }
        void                   SamplesBank(SampleBank * samplesbank) { m_pSamples = samplesbank; }
        const InstrumentBank & InstrumentBank()const                 { return m_instruments; }

    private:
        const InstrumentBank & m_instruments;
        SampleBank           * m_pSamples;
    };

    /*****************************************************************************************
        MusicSequence
            Contains data for a single musical sequence from the PMD2 games.
            Contains events, and music parameters, along with sample information and mapping.
    *****************************************************************************************/
    class MusicSequence
    {
    public:
        MusicSequence( PresetBank * presets = nullptr );

    private:

        PresetBank * m_pPresetBank;
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