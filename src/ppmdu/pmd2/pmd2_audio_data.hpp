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



    /*
    */



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


        /*
            mergedProgData
                Structure containing references to presets, ordered by banks. While also maintaining a list of references to 
                the files those were originally from.
                This allows handling conflicting Preset ID from multiple files.
        */
        struct mergedProgData
        {
            typedef uint16_t                             presetid_t;           //The ID of a DSE Preset
            typedef uint16_t                             conpresindex_t;       //The index of a conflicting preset within a conflictingpresets_t list!
            typedef std::vector<DSE::ProgramInfo*>         conflictingpresets_t; //A list of pointers to presets sharing the same preset ID

            /*
                GetPresetByFile
                    For a given file index, within the list of loaded swd files, returns the correct preset at "presid".
            */
            inline DSE::ProgramInfo* GetPresetByFile( uint16_t fileindex, presetid_t presid )
            {
                return mergedpresets[ presid ][ filetopreset[fileindex].at(presid) ];
            }

            /*
                GetPresetByBank
                    For a given bank, returns the preset at preset ID !
            */
            inline DSE::ProgramInfo* GetPresetByBank( uint16_t bank, presetid_t presid )
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
        mergedProgData ExportSoundfont( const std::string & destf )const;

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
        void ExportSoundfontAndMIDIs_new( const std::string & destdir )const;

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
            Read all loaded smd/swd pairs and compile a list of instruments presets info, and a list of
            where each instrument preset from each smd+swd pair was put into that compiled instrument info list.

            The list can have several entries for the same instrument slot, and they're all
            pointers, so any null instrument preset slot will be represented as such.

            The list containing the position of each presets from each smd+swd pairs uses a map to associate 
            an instrument ID (uint16_t) to a position in the second dimension of the merged instrument info list.
        */
        mergedProgData PrepareMergedInstrumentTable()const;

    private:
        std::string               m_mbankpath;

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
    std::pair< DSE::PresetBank, std::vector<std::pair<DSE::MusicSequence,DSE::PresetBank>> > LoadBankAndPairs( const std::string & bank, const std::string & smdroot, const std::string & swdroot );

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
    void ExportPresetBank( const std::string & directory, const DSE::PresetBank & bnk );

    /*
        To use the ExportSequence,
    */

};};


//Ostream operators


#endif 