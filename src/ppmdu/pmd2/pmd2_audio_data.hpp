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
#include <dse/dse_common.hpp>
#include <dse/dse_sequence.hpp>
#include <dse/dse_containers.hpp>
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <future>

//
//          ###### TODO ######
//          Move this to properly named file! Most of this is applicable to most DSE games!!!
//

namespace DSE{ struct SMDLPresetConversionInfo; };

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

    //static const uint32_t    DSE_MaxDecayDur    =  8; //second
    //static const uint32_t    DSE_MaxAttackDur   = 10; //second
    //static const uint32_t    DSE_MaxHoldDur     = 10; //second
    //static const uint32_t    DSE_MaxReleaseDur  =  8; //second

    static const utils::value_limits<int8_t> DSE_LimitsPan { 0,  64, 127, 64 };
    static const utils::value_limits<int8_t> DSE_LimitsVol { 0, 127, 127, 64 };

//====================================================================================================
// Structs
//====================================================================================================

//====================================================================================================
// Class
//====================================================================================================


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
        typedef std::pair< DSE::MusicSequence, DSE::PresetBank > smdswdpair_t;

    //-----------------------------
    // Construction
    //-----------------------------
        /*
            mbank     : Path to Master SWD Bank to load.
            singleSF2 : If set to true, the batch loader will do its best to allocate all presets into a single SF2!
        */
        BatchAudioLoader( /*const std::string & mbank,*/ bool singleSF2 = true );

    //-----------------------------
    // Loading Methods
    //-----------------------------
        //void LoadMasterBank();
        void LoadMasterBank( const std::string & mbank );

        void LoadSmdSwdPair( const std::string & smd, const std::string & swd );

        //For loading all pairs in one or two directories into a batch loader
        /*
            LoadMatchedSMDLSWDLPairs
                This function loads all matched smdl + swdl pairs in one or two different directories
        */
        void LoadMatchedSMDLSWDLPairs( const std::string & swdldir, const std::string & smdldir );


    //-----------------------------
    // Exporting Methods
    //-----------------------------
        /*
            Builds a single soundfont from the master bank's samples, and from the
            individual swds from each smd+swd pairs.
            Any duplicate presets are ignored if they're identical, or they're placed into
            other banks for the same preset ID.
        */
        //mergedProgData ExportSoundfont( const std::string & destf )const;

        //A test for a simpler implementation
        std::vector<DSE::SMDLPresetConversionInfo> ExportSoundfont_New( const std::string & destf );

        /*
            Does the same as the "ExportSoundfont" method, but additionnaly also
            exports all loaded smd as MIDIs, with the appropriate bank events to use
            the correct instrument presets.
        */
        void ExportSoundfontAndMIDIs( const std::string & destdir, int nbloops = 0 );

        /*
            Attempts to export as a sounfont, following the General MIDI standard instrument patch list.

            dsetogm : A map consisting of a list of filenames associated to a vector where each indexes correspond to a
                      dse preset entry ID (AKA instrument ID), and where the integer at that index correspond to the 
                      GM patch number to attribute it during conversion.
        */
        void ExportSoundfontAsGM( const std::string & destf, const std::map< std::string, std::vector<int> > & dsetogm )const;

    //-----------------------------
    // State Methods
    //-----------------------------
        bool IsMasterBankLoaded()const;

    private:
        /*
            GetSizeLargestPrgiChunk
                Search the entire list of loaded swdl files, and pick the largest prgi chunk.
                This will avoid crashing when a song has more presets than the master bank !
        */
        uint16_t GetSizeLargestPrgiChunk()const;

        /*
            BuildPresetConversionDB
                Set to replace the above method.
                Builds a table for every files, containing data on what DSE preset IDs are converted to in the soundfont.

        */
        std::vector<DSE::SMDLPresetConversionInfo> BuildPresetConversionDB()const;

        /*
            BuildMasterFromPairs
                If no main bank is loaded, and the loaded pairs contain their own samples, 
                build a main bank from those!
        */
        void BuildMasterFromPairs();

        /*
        */
        void AllocPresetSingleSF2( std::vector<DSE::SMDLPresetConversionInfo> & toalloc )const;
        void AllocPresetDefault  ( std::vector<DSE::SMDLPresetConversionInfo> & toalloc )const;

    private:
        std::string               m_mbankpath;
        bool                      m_bSingleSF2;

        DSE::PresetBank           m_master;
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
    std::pair< DSE::PresetBank, std::vector<std::pair<DSE::MusicSequence,DSE::PresetBank>> > LoadBankAndPairs     ( const std::string & bank, const std::string & smdroot, const std::string & swdroot );
    std::pair< DSE::PresetBank, std::vector<std::pair<DSE::MusicSequence,DSE::PresetBank>> > LoadBankAndSinglePair( const std::string & bank, const std::string & smd,     const std::string & swd );

    // ======================= 2. 1 Sequence + 1 Bank ( 001.smd + 001.swd ) =======================
    std::pair<DSE::MusicSequence,DSE::PresetBank> LoadSmdSwdPair( const std::string & smd, const std::string & swd );

    // ======================= 3. Individual Bank ( bank.swd ) =======================
    DSE::PresetBank LoadSwdBank( const std::string & file );

    // ======================= 4. Sequence only =======================
    DSE::MusicSequence LoadSequence( const std::string & file );





    //-------------------
    //  Audio Exporters
    //-------------------

    /*
        Export all sequences 
    */

    /*
        Exports a Sequence as MIDI and a corresponding SF2 file if the PresetBank's samplebank ptr is not null.
    */
    bool ExportSeqAndBank( const std::string & filename, const DSE::MusicSequence & seq, const DSE::PresetBank & bnk );
    bool ExportSeqAndBank( const std::string & filename, const std::pair<DSE::MusicSequence,DSE::PresetBank> & seqandbnk );

    /*
        Export the PresetBank to a directory as XML and WAV samples.
    */
    void ExportPresetBank( const std::string & directory, const DSE::PresetBank & bnk, bool samplesonly = true );

    /*
        To use the ExportSequence,
    */

};};


//Ostream operators


#endif 