#ifndef PMD2_AUDIO_DATA_HPP
#define PMD2_AUDIO_DATA_HPP
/*
dse_conversion.hpp
2015/05/20
psycommando@gmail.com
Description: Containers and utilities for data parsed from PMD2's audio, and sequencer files.

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P
*/
#include <dse/dse_common.hpp>
#include <dse/dse_sequence.hpp>
#include <dse/dse_containers.hpp>
#include <dse/dse_conversion_info.hpp>
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <future>
#include <map>
#include <sstream>

//namespace DSE{ struct SMDLPresetConversionInfo; };

namespace sf2{ class SoundFont; class Instrument; };

namespace DSE
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


//
//
//
    /*
        ProcessedPresets
            A transition container used when doing extra processing on the sample data from the game.
            Since the envelope, LFO, etc, are "baked" into the sample themselves, we need to change 
            a lot about them.
    */
    class ProcessedPresets
    {
    public:
        struct PresetEntry
        {
            PresetEntry()
            {}

            PresetEntry( PresetEntry && mv )
                :prginf(std::move(mv.prginf)), splitsmplinf(std::move( mv.splitsmplinf )), splitsamples( std::move(mv.splitsamples) )
            {
            }

            DSE::ProgramInfo                    prginf;       //
            std::vector< DSE::WavInfo >         splitsmplinf; //Modified sample info for a split's sample.
            std::vector< std::vector<int16_t> > splitsamples; //Sample for each split of a preset.
        };

        typedef std::map< int16_t, PresetEntry >::iterator       iterator;
        typedef std::map< int16_t, PresetEntry >::const_iterator const_iterator;

       inline void AddEntry( PresetEntry && entry )
        {
            m_smpldata.emplace( std::move( std::make_pair( entry.prginf.id, std::move(entry) ) ) );
        }

        inline iterator       begin()      {return m_smpldata.begin();}
        inline const_iterator begin()const {return m_smpldata.begin();}

        inline iterator       end()        {return m_smpldata.end();}
        inline const_iterator end()const   {return m_smpldata.end();}

        inline iterator       find( int16_t presid ) { return m_smpldata.find(presid); }
        inline const_iterator find( int16_t presid )const { return m_smpldata.find(presid); }

        inline size_t         size()const  { return m_smpldata.size(); }

    private:
        std::map< int16_t, PresetEntry > m_smpldata;
    };

//====================================================================================================
//  Specialized Loaders/Exporters
//====================================================================================================
    /*
        BatchAudioLoader
            Used to load an entire set of smd+swd pairs all refering to a master bank.
            Just like what PMD2 uses.

            Can also be used to load the master bank, and then either a single or more smd+swd pairs.

            From there, several operations requiring pairs of those files loaded can be done!
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
        BatchAudioLoader( /*const std::string & mbank,*/ bool singleSF2 = true, bool lfofxenabled = true );

    //-----------------------------
    // Loading Methods
    //-----------------------------

        /*
            LoadMasterBank
        */
        void LoadMasterBank( const std::string & mbank );

        /*
            LoadSmdSwdPair
        */
        void LoadSmdSwdPair( const std::string & smd, const std::string & swd );

        /*
            LoadMatchedSMDLSWDLPairs
                This function loads all matched smdl + swdl pairs in one or two different directories
        */
        void LoadMatchedSMDLSWDLPairs( const std::string & swdldir, const std::string & smdldir );

        /*
            LoadBgmContainer
                Load a single bgm container file.
                Bgm containers are SWDL and SMDL pairs packed into a single file using a SIR0 container.
        */
        void LoadBgmContainer( const std::string & file );

        /*
            LoadBgmContainers
                Load all pairs in the folder. 
                Bgm containers are SWDL and SMDL pairs packed into a single file using a SIR0 container.

                - bgmdir : The directory where the bgm containers are located at.
                - ext    : The file extension the bgm container files have.
        */
        void LoadBgmContainers( const std::string & bgmdir, const std::string & ext );

        /*
            LoadSingleSMDLs
                Loads only SMDL in the folder.
        */
        void LoadSingleSMDLs( const std::string & smdldir );

        void LoadSMDL( const std::string & smdl );


    //-----------------------------
        /*
            Load from either a directory or single file blob files
        */
        void LoadSMDLSWDLSPairsFromBlob( const std::string & blob, bool matchbyname );

        /*
        */
        void LoadSMDLSWDLPairsAndBankFromBlob( const std::string & blob, const std::string & bankname );

        /*
            Loads from a single blob file, no directory
        */
        void LoadFromBlobFile(const std::string & blob, bool matchbyname);

    //-----------------------------
    // Exporting Methods
    //-----------------------------
        /*
            Builds a single soundfont from the master bank's samples, and from the
            individual swds from each smd+swd pairs.
            Any duplicate presets are ignored if they're identical, or they're placed into
            other banks for the same preset ID.
        */
        std::vector<DSE::SMDLPresetConversionInfo> ExportSoundfont( const std::string & destf );

        /*
            ExportSoundfontBakedSamples
                Same as above, except that the samples have baked envelopes
        */
        std::vector<SMDLPresetConversionInfo> ExportSoundfontBakedSamples( const std::string & destf );

        /*
            ExportXMLPrograms
                Export all the presets for each loaded swdl pairs! And if the sample data is present,
                it will also export it!
        */
        void ExportXMLPrograms( const std::string & destf );

        /*
            ExportSoundfontAndMIDIs
                Does the same as the "ExportSoundfont" method, but additionnaly also
                exports all loaded smd as MIDIs, with the appropriate bank events to use
                the correct instrument presets.
        */
        void ExportSoundfontAndMIDIs( const std::string & destdir, int nbloops = 0, bool bbakesamples = true );

        /*
            ExportSoundfontAsGM
                Attempts to export as a sounfont, following the General MIDI standard instrument patch list.

                dsetogm : A map consisting of a list of filenames associated to a vector where each indexes correspond to a
                          dse preset entry ID (AKA instrument ID), and where the integer at that index correspond to the 
                          GM patch number to attribute it during conversion.
        */
        void ExportSoundfontAsGM( const std::string & destf, const std::map< std::string, std::vector<int> > & dsetogm )const;

        /*
            ExportXMLAndMIDIs
                Export all music sequences to midis and export all preset data to xml + pcm16 samples!
        */
        void ExportXMLAndMIDIs( const std::string & destdir, int nbloops = 0 );

        /*
            ExportMIDIs
                Export only the sequences that were loaded to MIDI!
        */
        void ExportMIDIs( const std::string & destdir, const std::string & cvinfopath = "", int nbloops = 0 );

    //
    //
    //
        /*
            Writes a CVInfo files from the preset data currently loaded!
        */
        //void WriteBlankCvInfoFile( const std::string & destf );

    //-----------------------------
    // State Methods
    //-----------------------------
        bool IsMasterBankLoaded()const;

    private:
        struct audiostats
        {
            template<class T>
                struct LimitVal
            {
                typedef T val_t;
                val_t min;
                val_t avg;
                val_t max;
                int   cntavg; //Counts nb of value samples
                int   accavg; //Accumulate values

                LimitVal()
                    :min(0), avg(0), max(0), cntavg(0), accavg(0)
                {}

                void Process(val_t anotherval )
                {
                    if( anotherval < min )
                        min = anotherval;
                    else if( anotherval > max )
                        max = anotherval;

                    ++cntavg;
                    accavg += anotherval;
                    avg = static_cast<val_t>(accavg / cntavg);
                }

                std::string Print()const
                {
                    std::stringstream sstr;
                    sstr <<"(" << static_cast<int>( min ) <<", " <<static_cast<int>( max ) <<" ) Avg : " <<avg; 
                    return std::move( sstr.str() );
                }
            };

            audiostats()
            {}

            std::string Print()const
            {
                std::stringstream sstr;
                sstr << "Batch Converter Statistics:\n"
                     << "-----------------------------\n"
                     << "\tlforate : " <<lforate.Print() <<"\n"
                     << "\tlfodepth : " <<lfodepth.Print() <<"\n";
                return std::move(sstr.str());
            }

            LimitVal<int16_t> lforate;
            LimitVal<int16_t> lfodepth;
        };

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

        /*
        */
        void HandleBakedPrg( const ProcessedPresets               & entry, 
                            sf2::SoundFont                        * destsf2, 
                            const std::string                     & curtrkname, 
                            int                                     cntpair, 
                            std::vector<SMDLPresetConversionInfo> & presetcvinf,
                            int                                   & instidcnt,
                            int                                   & presetidcnt,
                            const DSE::KeyGroupList      & keygroups );

        void HandleBakedPrgInst( const ProcessedPresets::PresetEntry   & entry, 
                            sf2::SoundFont                        * destsf2, 
                            const std::string                     & presname, 
                            int                                     cntpair, 
                            SMDLPresetConversionInfo::PresetConvData  & presetcvinf,
                            int                                   & instidcnt,
                            int                                   & presetidcnt,
                            const DSE::KeyGroupList      & keygroups );

        void HandlePrgSplitBaked( sf2::SoundFont                     * destsf2, 
                                  const DSE::SplitEntry              & split,
                                  size_t                               sf2sampleid,
                                  const DSE::WavInfo                 & smplinf,
                                  const DSE::KeyGroup                & keygroup,
                                  SMDLPresetConversionInfo::PresetConvData  & presetcvinf,
                                  sf2::Instrument                    * destinstsf2 );

    private:
        std::string               m_mbankpath;
        bool                      m_bSingleSF2;
        bool                      m_lfoeffects; //Whether lfo effects should be processed

        DSE::PresetBank           m_master;
        std::vector<smdswdpair_t> m_pairs;

        audiostats                m_stats;      //Used for research mainly. Stores statistics during processing of the DSE files

        BatchAudioLoader( const BatchAudioLoader & )           = delete;
        BatchAudioLoader & operator=(const BatchAudioLoader& ) = delete;
    };




//====================================================================================================
// Functions
//====================================================================================================

    //-------------------
    //  Sample Handling
    //-------------------


    /*
        DSESampleConvertionInfo
            Details on the resulting sample after being converted.
    */
    struct DSESampleConvertionInfo
    {
        //In sample points
        size_t loopbeg_ = 0;
        size_t loopend_ = 0;
    };

    /*
        ConvertDSESample
            Converts the given raw samples from a DSE compatible format to a signed pcm16 sample.

                * smplfmt    : The DSE sample type ID.
                * origloopbeg: The begining pos of the loop, straight from the WavInfo struct !
                * in_smpl    : The raw sample data as bytes.
                * out_cvinfo : The conversion info struct containing details on the resulting sample.
                * out_smpl   : The resulting signed pcm16 sample.

            Return the sample type.
    */
    eDSESmplFmt ConvertDSESample( int16_t                                smplfmt, 
                                  size_t                                 origloopbeg,
                                  const std::vector<uint8_t>           & in_smpl,
                                  DSESampleConvertionInfo              & out_cvinfo,
                                  std::vector<int16_t>                 & out_smpl );

    /*
        ConvertAndLoopDSESample
            Converts the given raw samples from a DSE compatible format to a signed pcm16 sample.

                * smplfmt    : The DSE sample type ID.
                * origloopbeg: The begining pos of the loop, straight from the WavInfo struct !
                * origlooplen: The lenght of the loop, straight from the WavInfo struct !
                * nbloops    : The nb of times to loop the sample.
                * in_smpl    : The raw sample data as bytes.
                * out_cvinfo : The conversion info struct containing details on the resulting sample.
                * out_smpl   : The resulting signed pcm16 sample.

            Return the sample type.
    */
    eDSESmplFmt ConvertAndLoopDSESample( int16_t                                smplfmt, 
                                         size_t                                 origloopbeg,
                                         size_t                                 origlooplen,
                                         size_t                                 nbloops,
                                         const std::vector<uint8_t>           & in_smpl,
                                         DSESampleConvertionInfo              & out_cvinfo,
                                         std::vector<int16_t>                 & out_smpl );


    /*
        ProcessDSESamples
            Takes the samples used in the programbank, convert them, and bake the envelope into them.

            * srcsmpl         : The samplebank containing all the raw samples used by the programbank.
            * prestoproc      : The programbank containing all the program whose samples needs to be processed.
            * desiredsmplrate : The desired sample rate in hertz to resample all samples to! (-1 means no resampling)
            * bakeenv         : Whether the envelopes should be baked into the samples.

            Returns a ProcessedPresets object, contining the new program data, along with the new samples.
    */
    DSE::ProcessedPresets ProcessDSESamples( const DSE::SampleBank  & srcsmpl, 
                                             const DSE::ProgramBank & prestoproc, 
                                             int                      desiredsmplrate = -1, 
                                             bool                     bakeenv         = true );

    //-------------------
    //  Audio Loaders
    //-------------------

    // ======================= 1. Main Bank + Sequences + RefBanks ( smdl or sedl ) ( mainbank.swd + 001.smd + 001.swd ) =======================
    //std::pair< DSE::PresetBank, std::vector<std::pair<DSE::MusicSequence,DSE::PresetBank>> > LoadBankAndPairs     ( const std::string & bank, const std::string & smdroot, const std::string & swdroot );
    //std::pair< DSE::PresetBank, std::vector<std::pair<DSE::MusicSequence,DSE::PresetBank>> > LoadBankAndSinglePair( const std::string & bank, const std::string & smd,     const std::string & swd );

    // ======================= 2. 1 Sequence + 1 Bank ( 001.smd + 001.swd ) =======================
    //std::pair<DSE::MusicSequence,DSE::PresetBank> LoadSmdSwdPair( const std::string & smd, const std::string & swd );

    // ======================= 3. Individual Bank ( bank.swd ) =======================
    //DSE::PresetBank LoadSwdBank( const std::string & file );

    // ======================= 4. Sequence only =======================
    //DSE::MusicSequence LoadSequence( const std::string & file );





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
    void ExportPresetBank( const std::string & directory, const DSE::PresetBank & bnk, bool samplesonly = true, bool hexanumbers = true, bool noconvert = true );

    /*
        To use the ExportSequence,
    */

//=================================================================================================
//  XML
//=================================================================================================

/*
    PresetBankToXML
        Write the 3 XML files for a given set of presets and samples.
*/
void PresetBankToXML( const DSE::PresetBank & srcbnk, const std::string & destdir );

/*
    XMLToPresetBank
        Read the 3 XML files for a given set of presets and samples.
*/
DSE::PresetBank XMLToPresetBank( const std::string & srcdir );

};


//Ostream operators


#endif 