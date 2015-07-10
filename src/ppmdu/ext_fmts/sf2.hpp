#ifndef SF2_HPP
#define SF2_HPP
/*
sf2.hpp
2015/05/20
psycommando@gmail.com
Description: Utilities for reading and writing SF2 soundfonts files. 

Used this page as reference:
http://www.pjb.com.au/midi/sfspec21.html
*/
#include <ppmdu/containers/audio_sample.hpp>
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <memory>
#include <array>
#include <functional>

namespace sf2
{
    static const uint32_t ShortNameLen = 20; //20 characters max for Presets, Instruments and Samples.


    /***********************************************************************************
        eSFGen
            http://www.pjb.com.au/midi/sfspec21.html#8.1
            List of sf2 generator types
    ***********************************************************************************/
    enum struct eSFGen : uint16_t
    {
        startAddrsOffset                = 0,
        endAddrsOffset                  = 1,
        startloopAddrsOffset            = 2,
        endloopAddrsOffset              = 3,
        startAddrsCoarseOffset          = 4,
        modLfoToPitch                   = 5,
        vibLfoToPitch                   = 6,
        modEnvToPitch                   = 7,
        initialFilterFc                 = 8,
        initialFilterQ 	                = 9,
        modLfoToFilterFc                = 10,
        modEnvToFilterFc                = 11,
        endAddrsCoarseOffset            = 12,
        modLfoToVolume                  = 13,
        unused1                         = 14, //Unused, ignore
        chorusEffectsSend               = 15,
        reverbEffectsSend               = 16,
        pan                             = 17,
        unused2                         = 18, //Unused, ignore
        unused3                         = 19, //Unused, ignore
        unused4                         = 20, //Unused, ignore
        delayModLFO                     = 21,
        freqModLFO                      = 22,
        delayVibLFO                     = 23,
        freqVibLFO                      = 24,
        delayModEnv                     = 25,
        attackModEnv                    = 26,
        holdModEnv                      = 27,
        decayModEnv                     = 28,
        sustainModEnv                   = 29,
        releaseModEnv                   = 30,
        keynumToModEnvHold              = 31,
        keynumToModEnvDecay             = 32,
        delayVolEnv                     = 33,
        attackVolEnv                    = 34,
        holdVolEnv                      = 35,
        decayVolEnv                     = 36,
        sustainVolEnv                   = 37,
        releaseVolEnv                   = 38,
        keynumToVolEnvHold              = 39,
        keynumToVolEnvDecay             = 40,
        instrument                      = 41,
        reserved1                       = 42, //Unused, ignore
        keyRange                        = 43,
        velRange                        = 44,
        startloopAddrsCoarseOffset      = 45,
        keynum                          = 46,
        velocity                        = 47,
        initialAttenuation              = 48,
        reserved2                       = 49, //Unused, ignore
        endloopAddrsCoarseOffset        = 50,
        coarseTune                      = 51,
        fineTune                        = 52,
        sampleID                        = 53,
        sampleMode                      = 54,
        reserved3                       = 55, //Unused, ignore
        scaleTuning                     = 56,
        exclusiveClass                  = 57,
        overridingRootKey               = 58,
        unused5                         = 59, //Unused, ignore
        endOper                         = 60, //Unused, ignore

        Invalid                         = 0xBEEF,
    };

    /*
        eSmplMode
            Special enum value for the sample mod generator
    */
    enum struct eSmplMode : uint16_t
    {
        noloop        = 0,
        loop          = 1,
        unused        = 2,
        loopWhileHold = 3, //Loop while holding the key, then play the rest of the sample
    };

    /***********************************************************************************
        SFModulator
            http://www.pjb.com.au/midi/sfspec21.html#8.2
            Represent a single modulator.
    ***********************************************************************************/
    struct SFModulator
    {
        //-------------
        //  Enums
        //-------------

        /*
            eCtrlPal
        */
        enum struct eCtrlPal : uint8_t
        {
            NoCtrlr    = 0,  //No controller
            NoteOnVel  = 2,  //Note on velocity
            NoteOnKey  = 3,  //Note on key number
            PolyPress  = 10, //Poly Pressure
            ChanPress  = 13, //Chan Pressure
            PitchWheel = 14, 
            PitchWSens = 16, //Pitch Wheel Sensitivity 
        };

        /*
            eSrc
                Source/Type for a modulator
        */
        enum struct eSrc : uint8_t
        {
            Linear  = 0,
            Concave = 1,
            Convex  = 2,
            Switch  = 3,
        };

        //------------
        //  Methods
        //------------

        //Constructor
        SFModulator( eSrc     type      = eSrc::Linear, 
                     eCtrlPal index     = eCtrlPal::NoCtrlr,
                     bool     contctrl  = false,
                     bool     direction = false,
                     bool     polarity  = false )
            :type_(type), index_(index), contctrlrf_(contctrl), direction_(direction), polarity_(polarity)
        {}

        //Cast operator
        operator uint16_t()const
        {
            /*
            15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
            |......Type.....| P  D  C  |.......Index......|
            */
            return ( static_cast<uint16_t>(type_)            | 
                   (static_cast<uint16_t>(contctrlrf_) << 7) |
                   (static_cast<uint16_t>(direction_)  << 8) |
                   (static_cast<uint16_t>(polarity_)   << 9) |
                   (static_cast<uint16_t>(type_)       << 10) );
        }

        //Cast operator
        SFModulator & operator=( uint16_t other )
        {
            /*
            15 14 13 12 11 10 09 08 07 06 05 04 03 02 01 00
            |......Type.....| P  D  C  |.......Index......|
            */
            type_       = static_cast<eSrc>( (other >> 10) & 0x3F );
            polarity_   = (other >> 9 ) & 0x1;
            direction_  = (other >> 8 ) & 0x1;
            contctrlrf_ = (other >> 7 ) & 0x1;
            index_      = static_cast<eCtrlPal>( other & 0x7F );
            return *this;
        }

        //-------------
        //  Variables
        //-------------
        eSrc     type_;
        eCtrlPal index_;
        bool     contctrlrf_; //Continuous controller flag
        bool     direction_; 
        bool     polarity_;
    };

    /***********************************************************************************
        eSFTransform
            http://www.pjb.com.au/midi/sfspec21.html#8.3
            Enum with all the possible values for a modulator transform.
    ***********************************************************************************/
    enum struct eSFTransform : uint16_t
    {
        linear = 0,
    };

    /***********************************************************************************
        SFGenZone
            http://www.pjb.com.au/midi/sfspec21.html#7.5
            Basically represent a generator entry for either a Preset or Instrument.
    ***********************************************************************************/
    struct SFGenZone
    {
        static const uint32_t SIZE = 4; //bytes
        eSFGen   GenOper   = eSFGen::startAddrsOffset;  //Is == 0
        uint16_t genAmount = 0; //Can be used a uint16, a int16, or 2 bytes. But that's based on the generator, and up to the user.
    };

    /***********************************************************************************
        SFModZone
            http://www.pjb.com.au/midi/sfspec21.html#7.4
            Basically represent a modulator entry for a Preset or Instrument
    ***********************************************************************************/
    struct SFModZone
    {
        static const uint32_t SIZE = 10; //bytes

        SFModulator  ModSrcOper;                               //By default is 0  
        eSFGen       ModDestOper   = eSFGen::startAddrsOffset; //Is == 0
        int16_t      modAmount     = 0;  
        SFModulator  ModAmtSrcOper;                            //By default is 0 
        eSFTransform ModTransOper  = eSFTransform::linear;     //Is == 0
    };

    /***********************************************************************************
        MidiKeyRange
            A range of MIDI notes.
    ***********************************************************************************/
    struct MidiKeyRange
    {
        int8_t lokey = 0;
        int8_t hikey = 127;
    };
        
    /***********************************************************************************
        MidiVeloRange
            A range of velocities.
    ***********************************************************************************/
    struct MidiVeloRange
    {
        int8_t lovel = 0;
        int8_t hivel = 127;
    };

    /***********************************************************************************
        Envelope
    ***********************************************************************************/
    struct Envelope
    {
        int16_t delay   = 0;
        int16_t attack  = 0;
        int16_t hold    = 0;
        int16_t sustain = 0;
        int16_t decay   = 0;
        int16_t release = 0;
    };

    ///***********************************************************************************
    //    SFSamples
    //        Maintain a list of sample data, and their data source.
    //        Wraps the "SHDR", and "sdat-list" chunks.
    //***********************************************************************************/
    //class SFSamples
    //{
    //    enum struct eSmplTy : uint32_t 
    //    {
    //        monoSample      = 1,        //Mono
    //        rightSample     = 2,        //Unsupported..
    //        leftSample      = 4,        //Unsupported..
    //        linkedSample    = 8,        //Stereo
    //        //..Unsupported below..
    //        RomMonoSample   = 32769, 
    //        RomRightSample  = 32770, 
    //        RomLeftSample   = 32772, 
    //        RomLinkedSample = 32776,
    //    };

    //    /*
    //        Entry from the SHDR chunk
    //        Contains details on the location of the sample within the sdata-list
    //        along with other things to properly play the sample
    //    */
    //    struct sfSample
    //    {
    //       std::array<char,20>  name;
    //       uint32_t             Start; 
    //       uint32_t             End;
    //       uint32_t             Startloop; 
    //       uint32_t             Endloop;
    //       uint32_t             SampleRate;  
    //       uint8_t              OriginalPitch;  
    //       int8_t               PitchCorrection;
    //       uint16_t             SampleLink;     //Index of a sample linked to this one, to be played at the same time.
    //       eSmplTy              SampleType;
    //    };

    //public:
    //    /*
    //        The type of a function that returns the data of an entire 16 bits sample.
    //        Sounfont supports only 16 bits signed.
    //    */
    //    typedef std::function<std::vector<int16_t>()> smplreadfun_t;


    //    /*
    //        AddSample
    //            Add a sample to the sample manager.

    //            - sdatfun   : A function that when executed will return a vector of
    //                          signed int16 samples containing the samples for this sample.
    //            - smpllength: The length of the sample, in samples(int16).
    //            - name      : A 19 character max string to identify this sample.
    //            - loopbeg   : The position in samples, where the loop for this sample begin.
    //                            If no loop, leave to 0.
    //            - loopend   : The position in samples, where the loop for this sample ends.
    //                            If no loop, leave to 0.
    //            - smplrate  : The sample rate of the sample in Hertz.
    //            - midikey   : The root MIDI key number of the sample. AKA the MIDI note number 
    //                          corresponding to the note the instrument was playing when recorded.
    //            - pitchcorr : A correction in cent (1/100th of a semitone) to apply to sample going
    //                          from -127 to 127.
    //            - ismono    : Whether the sample is mono or stereo. 

    //            Returns the index to refer to this sample in the rest of the soundfont (SHDR index)

    //            **NOTE: At this time matched left and right samples are not supported. Neither are ROM samples!
    //    */
    //    uint32_t AddSample( smplreadfun_t       sdatfun,
    //                        size_t              smpllength,
    //                        const std::string & name, 
    //                        uint32_t            loopbeg   = 0,
    //                        uint32_t            loopend   = 0,
    //                        uint32_t            smplrate  = 22050,
    //                        uint8_t             midikey   = 60,
    //                        int8_t              pitchcorr = 0,
    //                        bool                ismono    = true );

    //    /*
    //        Access to the shdr content for each samples
    //    */
    //    sfSample       & operator[]( uint32_t index );
    //    const sfSample & operator[]( uint32_t index )const;

    //    /*
    //        WriteSdat
    //            Write the entire sdata-list chunk, chunk header included.

    //            - out: a binary stream where to append the content of the chunk.

    //            Return the offsets of the begining of each samples
    //            from the begining of the sdat list chunk, in samples count(int16).
    //            (You should feed that to the "WriteSHDR" method *hint*)
    //    */
    //    std::vector<uint32_t> WriteSdat( std::ofstream & out );

    //    /*
    //        WriteSHDR
    //            Write the entire SHDR chunk, chunk header included.

    //            - out            : a binary stream where to append the content of the chunk.
    //            - sdatsmploffsets: The location of each samples within the sdat chunk, in samples count(int16).

    //            Return the nb of bytes written.
    //    */
    //    size_t WriteSHDR( std::ofstream & out, const std::vector<uint32_t> & sdatsmploffsets );

    //private:
    //    std::vector<sfSample>      m_shdr;
    //    std::vector<smplreadfun_t> m_sdata;
    //};


    ///***********************************************************************************
    //    SFInstruments
    //        Represent data on soundfont instruments.

    //        Wraps the "inst", "ibag", "imod", and "igen" chunks.
    //***********************************************************************************/
    //class SFInstruments
    //{
    //    struct sfInst
    //    {
    //       std::array<char,20> InstName; 
    //       uint16_t            InstBagNdx;
    //    };

    //    struct sfInstBag
    //    {
    //        uint16_t InstGenNdx; 
    //        uint16_t InstModNdx;
    //    };

    //public:

    //    /*
    //        Instance this and pass it to AddInstrument to add one !
    //    */
    //    struct Instrument
    //    {
    //        std::string            name_;
    //        std::vector<SFGenZone> gens_;
    //        std::vector<SFModZone> mods_;
    //    };


    //    /*
    //        AddInstrument
    //            Add an instrument, and its modulators and generators.

    //            This will also sort the generators and modulators in the standard required order !
    //            
    //            -> KeyRange Generators always first.
    //            -> Velocity Range generator can only be preceeded by a Keyrange generator.
    //            -> SampleID Generators always last.

    //            **NOTE: It is not neccessary to add a terminating null Instrument at the end of the list! 
    //                    Its handled automatically.

    //            Return the index the instrument was added to in the INST chunk
    //    */
    //    int16_t AddInstrument( Instrument && inst );

    //    
    //    size_t       GetNbInstruments()const;
    //    Instrument & operator[]      ( size_t index );


    //    /*
    //        WriteAllInstChunks
    //            Writes the 4 instrument chunks one after the other.
    //            inst, ibag, imod, igen.

    //            Return the nb of bytes written.
    //    */
    //    size_t WriteAllInstChunks( std::ofstream & out );

    //private:
    //    std::vector<Instrument> m_inst;
    //};

    ///***********************************************************************************
    //    SFPresets

    //    Wraps the "PHDR", "pbag", "pmod", and "pgen" chunks.
    //***********************************************************************************/
    //class SFPresets
    //{
    //    struct sfPresetBag
    //    {
    //       uint16_t GenNdx; 
    //       uint16_t ModNdx;
    //    };

    //public:

    //    struct Preset
    //    {
    //       std::array<char,20>    PresetName; 
    //       uint16_t               Bank       = 0; 
    //       std::vector<SFGenZone> Gens;
    //       std::vector<SFGenZone> Mods;
    //    };

    //    /*
    //        AddPreset
    //            Add a preset to the list.
    //            Returns the position in the preset list.(PHDR)
    //    */
    //    uint16_t AddPreset( Preset && pres );

    //    /*
    //    */
    //    Preset & operator[]( uint16_t index );

    //    /*
    //    */
    //    size_t GetNbPresets()const;

    //    /*
    //        WriteAllPresetChunks
    //            Writes the 4 Preset chunks one after the other.
    //            phdr, pbag, pmod, pgen.

    //            Return the nb of bytes written.
    //    */
    //    size_t WriteAllPresetChunks( std::ofstream & out );

    //private:
    //    std::vector<Preset> m_presets;
    //};

    ///***********************************************************************************
    //    SoundFont
    //***********************************************************************************/
    //class SoundFont
    //{
    //public:
    ////--------------------------------
    ////  Types Stuff
    ////--------------------------------
    //    typedef size_t   sampleid_t;
    //    typedef uint16_t presetid_t;

    //    friend class SounFontRIFFWriter;

    //public:
    ////--------------------------------
    ////  Nested Classes Stuff
    ////--------------------------------
    //    //Contains details about a sample to be added to the soundfont
    //    struct smplinfo
    //    {
    //        std::array<char,20> name; //Unused characters must be set to 0
    //        uint32_t loopbeg     = 0;    //Begining sample of the loop
    //        uint32_t loopend     = 0;    //Ending sample of the loop (leave to 0 if no loop)
    //        uint32_t smplrate    = 0;    //Hertz
    //        uint8_t  midirootkey = 0;    //Also known as "Original Pitch" The midi key the sampled instrument was recorded playing.
    //        int8_t   pitchcorr   = 0;    //Pitch correction value in cent.. -127 to 127 
    //    };

    //    //Contains needed details to add a preset
    //    struct presetinfo
    //    {
    //        std::array<char,20> name;
    //        uint32_t            id     = 0;
    //        uint32_t            bankid = 0;

    //        //amount of samples linked to this preset
    //        uint32_t            nbsmpls = 1;
    //        //We'll use this to compute our nb of pbags, pmods, and pgens entries in the soundfount
    //    };

    //    /*
    //        sampleloc
    //            Represent a sample's location.
    //            Can be a location in a file, or a location in a container in memory of either raw bytes or pcm16 samples.
    //    */
    //    struct sampleloc
    //    {
    //        sampleloc( const std::string & path = std::string(), size_t begoffset = 0, size_t endoffset = 0 )
    //            :offbeg_(begoffset), offend_(endoffset), fpath_(path)
    //        {}

    //        sampleloc( std::weak_ptr<std::vector<uint8_t>> && praw, size_t begoffset = 0, size_t endoffset = 0  )
    //            :offbeg_(begoffset), offend_(endoffset), prawdata_(praw)
    //        {}

    //        sampleloc( std::weak_ptr<std::vector<pcm16s_t>> && ppcm, size_t begoffset = 0, size_t endoffset = 0  )
    //            :offbeg_(begoffset), offend_(endoffset), ppcmdata_(ppcm)
    //        {}

    //        std::weak_ptr<std::vector<pcm16s_t>> ppcmdata_;
    //        std::weak_ptr<std::vector<uint8_t>>  prawdata_;
    //        std::string                          fpath_;
    //        size_t                               offbeg_; //Offset in the specified file or byte container where the raw samples begin.
    //        size_t                               offend_; //Offset in the specified file or byte container where the raw samples ends.
    //    };



    //public:
    ////--------------------------------
    ////  Constructors
    ////--------------------------------
    //    /*
    //        path          : path to sf2 file to load, or write.
    //        sounfontfname : the name to give this soundfont internally
    //    */
    //    SoundFont( const std::string & path, const std::string & sounfontfname )
    //        :m_path(path), m_sfname(sounfontfname)
    //    {}

    ////--------------------------------
    ////  IO Stuff
    ////--------------------------------

    //    /*
    //        Write
    //            Write data structures and samples to disk
    //    */
    //    void Write();

    //    /*
    //        Read
    //            Parse data structures to memory, samples stays in the file and are loaded on demand.
    //    */
    //    void Read();

    ////--------------------------------
    ////  Simplified 
    ////--------------------------------

    //    /*
    //        AddPreset
    //            Adds an empty preset to the list, with the specified name, preset number, and bank.

    //                - name     : Internal name. 20 characters max, the rest will be truncated.
    //                - presetid : The preset number to give this preset. (0-128 are valid for MIDI. 128 is reserved for drums)
    //                - bankno   : The bank indice for this preset. 

    //                Returns the unique index in the internal preset table. Use this index to refer to this preset in the future.
    //    */
    //    size_t AddPreset( const std::string & name, 
    //                      presetid_t          presetid, 
    //                      uint16_t            bankno );


    //    /*
    //        AddInstrument
    //            Adds a minimal instrument to the soundfont. 

    //            Instruments are the link between samples and presets. Each instruments
    //            can refer to a single sample, have a single key range, a single velocity range,
    //            volume envelope, and etc..
    //            A preset can have several instruments, thus several samples, key ranges, velocity ranges..
    //    */
    //    size_t AddInstrument( const std::string & name,
    //                          size_t              preset,
    //                          size_t              smplid );



    //    /*
    //        AddSampleToPreset
    //            Adds a sample, and directly assign it to a preset. Takes the sample from a memory location.

    //                - presetindex: The index of the preset to assign this sample to.
    //                - pdat       : A pointer to the data of the sample.
    //                - info       : Information on the sample.
    //                - keyrange   : Range of MIDI keys that this sample will be played on.
    //                - velrange   : Range of MIDI velocity that this sample will be played on.

    //            Return the index to the sample in the internal sample table. Use this value to refer to this 
    //            particular sample.

    //            **NOTE: The user must ensure that the data pointed to by the pointer is valid for the 
    //                    lifetime of the SoundFont object !
    //    */
    //    size_t AddSampleToPreset( size_t                  presetindex, 
    //                              std::weak_ptr<uint8_t>  pdat,  
    //                              smplinfo                info,
    //                              MidiKeyRange            keyrange  = MidiKeyRange(), 
    //                              MidiVeloRange           velorange = MidiVeloRange() );

    //    /*
    //        AddSampleToPreset
    //            Adds a sample, and directly assign it to a preset. Takes the sample from a file.

    //                - presetindex: The index of the preset to assign this sample to.
    //                - fpath      : The path to the file containing the sample.
    //                - foffset    : The offset to read the sample at.
    //                - smpllen    : The length of the sample, in 16 bits samples.
    //                - info       : Information on the sample.
    //                - keyrange   : Range of MIDI keys that this sample will be played on.
    //                - velrange   : Range of MIDI velocity that this sample will be played on.

    //            Return the index to the sample in the internal sample table. Use this value to refer to this 
    //            particular sample.
    //    */
    //    size_t AddSample( size_t                 presetindex, 
    //                      const std::string     &fpath, 
    //                      size_t                 foffset, 
    //                      size_t                 smpllen, 
    //                      smplinfo               info,
    //                      MidiKeyRange           keyrange  = MidiKeyRange(), 
    //                      MidiVeloRange          velorange = MidiVeloRange() );

    //    /*
    //        LinkSampleToPreset
    //            Links an existing sample to an existing preset. 
    //            (Internally creates a new sfinstrument with that sample assigned, and link it to the preset)

    //            - presetindex : the index inside the internal preset table obtained when adding the preset.
    //            - sampleindex : the index inside the internal sample table obtained when adding the sample.
    //            - lokey       : The lowest MIDI key that this sample will be played on.
    //            - hikey       : The highest MIDI key that this sample will be played on.
    //            - lovel       : The lowest MIDI velocity that this sample will be played on.
    //            - hivel       : The highest MIDI velocity that this sample will be played on.
    //    */
    //    void LinkSampleToPreset( size_t        presetindex, 
    //                             size_t        sampleindex,
    //                             MidiKeyRange  keyrange    = MidiKeyRange(), 
    //                             MidiVeloRange velorange   = MidiVeloRange() );
    //    void LinkSampleToPreset( size_t        presetindex, 
    //                             size_t        sampleindex,
    //                             MidiKeyRange  keyrange    = MidiKeyRange(), 
    //                             MidiVeloRange velorange   = MidiVeloRange(),
    //                             Envelope      volenv      = Envelope(), 
    //                             Envelope      modenv      = Envelope() );


    ////--------------------------------
    ////  Advanced
    ////--------------------------------

    //    /*
    //        Those return references to the underlying soundfont chunks
    //        wrappers. 
    //        Use those to assign uncommon generators, modulators, and
    //        etc..
    //    */
    //    SFSamples     & GetSamplesWrapper()    { return m_smpldb;   }
    //    SFInstruments & GetInstrumentWrapper() { return m_instdb;   }
    //    SFPresets     & GetPresetWrapper()     { return m_presetdb; }

    //private:

    //    /*
    //        LoadSmpl
    //            This is the function passed to the underlying SFSamples
    //            for loading samples. It must be binded first.
    //    */
    //    static std::vector<int16_t> LoadSmpl( const sampleloc & location );

    //    /*
    //        MakeLoadSampleFun
    //            This Makes the binded function object passed to the underlying SFSamples
    //            object.
    //    */
    //    static SFSamples::smplreadfun_t MakeLoadSampleFun( const sampleloc & location );

    //private:
    //    std::string            m_path;
    //    std::string            m_sfname;

    //    // This is a vector containing the data to merge transparently delayed samples loading and direct samples loading.
    //    // It containes details on where to fetch the sample data for each samples registered using the simplified sample.
    //    // loading. **Note that, if the underlying SFSamples object has samples loaded, objects in this vector will be ignored.
    //    std::vector<sampleloc> m_smplsfstrs;

    //    //Managers
    //    SFSamples     m_smpldb;
    //    SFInstruments m_instdb;
    //    SFPresets     m_presetdb;

    //    //No copy plz
    //    SoundFont(const SoundFont&)            = delete;
    //    SoundFont& operator=(const SoundFont&) = delete;
    //};

    /***********************************************************************************
        BaseGeneratorUser
            Base class that implements methods common to all generator users.
    ***********************************************************************************/
    class BaseGeneratorUser
    {
    public:
        /*
            The parameter for the generator can be interpreted in those ways
        */
        typedef union 
        {
            uint16_t                            uword;
            int16_t                             word;
            union{ uint8_t by1; uint8_t by2; }  twouby;
            union{ int8_t  by1;  int8_t  by2; } twosby;
        } genparam_t;

        virtual ~BaseGeneratorUser(){}

        /*
            AddGenerator
                Add a generator value to this instrument.

                A "generator" is mainly an attribute so to speak.
                A sample Id is a generator for instance. 
                
                There can only be a single generator of a given type per instrument.
                Any generator with the same generator type as an existing generator
                will overwrite the later.

                When the object is later written to the sf2, 
                generators will be sorted in the standard required order !
                
                -> KeyRange Generators always first.
                -> Velocity Range generator can only be preceeded by a Keyrange generator.
                -> SampleID Generators always last.
                -> InstrumentID Generators always last.
        */
        void AddGenerator( eSFGen gen, genparam_t value );

        /*
            GetGenerator
                Return a pointer to the specified generator's value, 
                or null if it doesn't exist.
        */
        genparam_t       * GetGenerator( eSFGen gen );
        const genparam_t * GetGenerator( eSFGen gen )const;

        /*
            GetGenerator
                Return a reference to the specified generator's value.
        */
        genparam_t       & GetGenerator( size_t index );
        const genparam_t & GetGenerator( size_t index )const;

        /*
            GetGenerators
                Return a reference to the specified generator's value.
        */
        std::map<eSFGen,genparam_t>       & GetGenerators()      { return m_gens; }
        const std::map<eSFGen,genparam_t> & GetGenerators()const { return m_gens; }

        /*
            GetNbGenerators
        */
        size_t GetNbGenerators()const;

        //---------------------
        //  Common Generators
        //---------------------
        
        /*
            Get or Set the sample used by this instrument.
                sampleid : sample index into the SHDR sub-chunk

                Returns a pair made of a boolean, and the value of the generator.
                If the boolean is false, there is currently no such generator, and 
                the value returned is thus invalid.
        */
        void                   SetSampleId( size_t sampleid );
        std::pair<bool,size_t> GetSampleId()const;

        /*
            Get or Set the instrument ID used by this preset
                instrumentid : instrument index into the INST sub-chunk

                Returns a pair made of a boolean, and the value of the generator.
                If the boolean is false, there is currently no such generator, and 
                the value returned is thus invalid.
        */
        void                   SetInstrumentId( size_t instrumentid );
        std::pair<bool,size_t> GetInstrumentId()const;

        /*
            Set or Get the MIDI key range covered by this instrument.
                (def: 0-127, min: 0, max: 127)

                Return the default value if no generator is present
        */
        void         SetKeyRange( int8_t lokey, int8_t hikey );
        void         SetKeyRange( MidiKeyRange kr )               { SetKeyRange( kr.lokey, kr.hikey ); }
        MidiKeyRange GetKeyRange()const;

        /*
            Set or Get the velocity range covered by this instrument.
                (def: 0-127, min: 0, max: 127)

                Return the default value if no generator is present
        */
        void          SetVelRange( int8_t lokvel, int8_t hivel );
        void          SetVelRange( MidiVeloRange vr )             { SetVelRange( vr.lovel, vr.hivel ); }
        MidiVeloRange GetVelRange()const;

        /*
            Set or Get the volume envelope.
                delay  : timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:5000 [ 20sec] )
                attack : timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:8000 [100sec] )
                hold   : timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:5000 [ 20sec] )
                decay  : timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:8000 [100sec] )
                sustain: centibel (def:     0 [   0dB], min:     0 [   0dB], max:1440 [ 144dB] )
                release: timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:8000 [100sec] )

                Return the default value if no generator is present
        */
        void     SetVolEnvelope( const Envelope & env );
        Envelope GetVolEnvelope()const;

        /*
            Set or Get the modulation envelope.
                delay  : timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:5000 [ 20sec] )
                attack : timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:8000 [100sec] )
                hold   : timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:5000 [ 20sec] )
                decay  : timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:8000 [100sec] )
                sustain:  	-0.1% (def:     0 [  100%], min:     0 [  100%], max:1000 [    0%] )
                release: timecent (def:-12000 [ < 1ms], min:-12000 [   1ms], max:8000 [100sec] )

                Return the default value if no generator is present
        */
        void     SetModEnvelope( const Envelope & env );
        Envelope GetModEnvelope()const;


        /*
            Set or Get the coarse tune.
                (def:0, min:-120, max:120)
                Pitch offset in semitones to be applied to the note.
                Positive means a higher pitch, negative, a lower.
                Its additive with the "FineTune" generator.

                Return the default value if no generator is present
        */
        void    SetCoarseTune( int16_t tune );
        int16_t GetCoarseTune()const;

        /*
            Set or Get the fine tune.
                (def:0, min:-99, max:99)
                Pitch offset in cents which should be applied to the note. 
                Positive means a higher pitch, negative, a lower.
                Its additive with the "CoarseTune" generator.
                
                Return the default value if no generator is present
        */
        void    SetFineTune( int16_t tune );
        int16_t GetFineTune()const;

        /*
            Set or Get the sample mode. (sample looping)
                (def:0, no loop)

                Return the default value if no generator is present
        */
        void      SetSmplMode( eSmplMode mode );
        eSmplMode GetSmplMode()const;

        /*
            Set or Get the Scale Tuning
                (def:100, min:0, max:1200)
                0   : means MIDI key numbers has no effect on pitch.
                100 : means the MIDI key number have full effect on the pitch.

                Return the default value if no generator is present
        */
        void     SetScaleTuning( uint16_t scale );
        uint16_t GetScaleTuning()const;

        /*
            Set or Get the Initial Attenuation
                (def:0, min:0, max:1440)
                The attenuation in centibels applied to the note.
                0  == no attenuation
                60 == attenuated by 6dB

                Return the default value if no generator is present
        */
        void     SetInitAtt( uint16_t att );
        uint16_t GetInitAtt()const;

        /*
            Set or Get the Pan
                (def:0 center, min:-500 left, max: 500 right)
                The pan in 0.1% applied to the note. 

                Return the default value if no generator is present
        */
        void    SetPan( int16_t pan );
        int16_t GetPan()const;

        /*
            Set or Get the Exclusive Class id
                (def:0, min:0, max:127)
                Basically, instruments  within the same
                Preset, with the same Exclusive Class Id cut eachother
                when they play.

                Return the default value if no generator is present
        */
        void     SetExclusiveClass( uint16_t id );
        uint16_t GetExclusiveClass()const;

        /*
            Set or Get the Reverb Send
                (def:0, min:0, max:1000)
                The amount of reverb effect in 0.1% applied to the note. 
                1000 == 100%
                http://www.pjb.com.au/midi/sfspec21.html#g16

                Return the default value if no generator is present
        */
        void     SetReverbSend( uint16_t send );
        uint16_t GetReverbSend()const;

        /*
            Set or Get the Chorus Send
                (def:0, min:0, max:1000)
                The amount of chorus effect in 0.1% applied to the note. 
                1000 == 100%
                http://www.pjb.com.au/midi/sfspec21.html#g15

                Return the default value if no generator is present
        */
        void     SetChorusSend( uint16_t send );
        uint16_t GetChorusSend()const;


    protected:
        /*
            genpriority_t
                A number from 0-255 indicating the priority of a generator in the list.
                The default value for regular priority is 127. Lowest priority is 0, highest is 255.
                High priority generators will be placed at the begining, ordered by priority.
                Low priority generators will be placed at the end, ordered by priority.
        */
        typedef uint8_t genpriority_t;
        static const genpriority_t DefaultPriority = 127;
        static const genpriority_t HighPriority    = 255;
        static const genpriority_t LowPriority     = 0;

        /*
            GetGenPriority
                This method returns the priority of a generator. 
        */
        virtual genpriority_t GetGenPriority( eSFGen gen )const;

    protected:
        std::map<eSFGen,genparam_t> m_gens; //There can only be a single generator of a given type
    };

    /***********************************************************************************
        BaseModulatorUser
            Base class that implements methods common to all modulator users.
    ***********************************************************************************/
    class BaseModulatorUser
    {
    public:

        virtual ~BaseModulatorUser(){}

        /*
            AddModulator
                Return modulator index in this instrument's list.
        */
        size_t AddModulator( SFModZone && mod );

        /*
            GetModulator
                Return the modulator at the index specified.
        */
        SFModZone       & GetModulator( size_t index );
        const SFModZone & GetModulator( size_t index )const;

        std::vector<SFModZone>       & GetModulators()      { return m_mods; }
        const std::vector<SFModZone> & GetModulators()const { return m_mods; }

        /*
            GetNbModulators
        */
        size_t GetNbModulators()const;

        //---------------------
        //  Common Modulators
        //---------------------

    protected:
        std::vector<SFModZone> m_mods;
    };


    /***********************************************************************************
        Instrument
    ***********************************************************************************/
    class Instrument : public BaseGeneratorUser, public BaseModulatorUser
    {
    public:
        //
        //
        //
        Instrument();
        Instrument( const std::string & name );

        //----------------
        //  Generic
        //----------------

        /*
            Get/Set Name
                Name will be truncated to the first 19 characters.
        */
        inline void                SetName( const std::string & name ){ m_name = name; }
        inline const std::string & GetName()const                     { return m_name; }

    private:
        std::string m_name;
    };


    /***********************************************************************************
        Preset
    ***********************************************************************************/
    class Preset : public BaseGeneratorUser, public BaseModulatorUser
    {
    public:

        //---------------
        //  Contructors
        //---------------
        Preset();
        Preset( const std::string & name, uint16_t presetno, uint16_t bankno = 0, uint32_t lib = 0, uint32_t genre = 0, uint32_t morpho = 0 );

        //------------
        //  
        //------------
        /*

        */
        size_t AddInstrument( Instrument && inst );

        /*
        */
        inline Instrument       & GetInstument( size_t index )      { return m_instruments[index]; }
        inline const Instrument & GetInstument( size_t index )const { return m_instruments[index]; }

        /*
        */
        inline size_t GetNbInstruments()const            { return m_instruments.size(); }

        /*
            Get/Set Name
                Name will be truncated to the first 19 characters.
        */
        inline void                SetName( const std::string & name ) { m_name = name; }
        inline const std::string & GetName()const                      { return m_name; }

        /*
            Set/Get Preset Number
        */
        void     SetPresetNo( uint16_t no ) { m_presetNo = no; }
        uint16_t GetPresetNo()const         { return m_presetNo; }

        /*
            Set/Get Bank Number
        */
        void     SetBankNo( uint16_t no ) { m_bankNo = no; }
        uint16_t GetBankNo()const         { return m_bankNo; }

        /*
            Set/Get Library
        */
        void     SetLibrary( uint32_t lib ) { m_library = lib; }
        uint32_t GetLibrary()const          { return m_library; }

        /*
            Set/Get Genre
        */
        void     SetGenre( uint32_t genre ) { m_genre = genre; }
        uint32_t GetGenre()const          { return m_genre; }

        /*
            Set/Get Morphology
        */
        void     SetMorpho( uint32_t morpho ) { m_morpho = morpho; }
        uint32_t GetMorpho()const             { return m_morpho; }

    private:
        std::string                   m_name;
        uint16_t                      m_presetNo;
        uint16_t                      m_bankNo;
        uint32_t                      m_library;
        uint32_t                      m_genre;
        uint32_t                      m_morpho;

        std::vector<Instrument>       m_instruments;
    };

    /***********************************************************************************
        Sample
    ***********************************************************************************/
    class Sample
    {
        enum struct eLoadType
        {
            DelayedRawVec,
            DelayedPCMVec,
            DelayedRaw,
            DelayedPCM,

            DelayedFile,
            DelayedFunc,
        };
    public:
        /*
        */
        typedef std::function<std::vector<pcm16s_t>()> loadfun_t;

        /*
        */
        enum struct eSmplTy : uint32_t 
        {
            monoSample      = 1,        //Mono
            rightSample     = 2,        //Unsupported..
            leftSample      = 4,        //Unsupported..
            linkedSample    = 8,        //Stereo
            //..Unsupported below..
            RomMonoSample   = 32769, 
            RomRightSample  = 32770, 
            RomLeftSample   = 32772, 
            RomLinkedSample = 32776,
        };

        /*
            Load from a file. begoff and endoff are in bytes.
        */
        Sample( const std::string & fpath,                    size_t begoff, size_t endoff );

        /*
            Load from a vector. begoff and endoff are in bytes.
        */
        Sample( std::vector<uint8_t> * prawvec, size_t begoff, size_t endoff );
        Sample( uint8_t              * praw,    size_t begoff, size_t endoff );

        /*
            Load from a vector. begoff and endoff are in int16 !
        */
        Sample( std::weak_ptr<std::vector<pcm16s_t>> ppcmvec, size_t begoff, size_t endoff );
        Sample( std::weak_ptr<pcm16s_t>              ppcm,    size_t begoff, size_t endoff );

        /*
            Load from a function. begoff and endoff are in int16 !
        */
        Sample( loadfun_t && funcload,                        size_t begoff, size_t endoff );

        /*
            Obtain the data from the sample.
        */
        operator std::vector<pcm16s_t>()const;

        /*
            Obtain the data from the sample.
            Wrapper over delayed read operations
        */
        std::vector<pcm16s_t> Data()const;


        size_t GetDataLength()const { return (m_endoff - m_begoff); }

        /*
            Get/Set Name
                Name will be truncated to the first 19 characters.
        */
        inline void                SetName( const std::string & name ) { m_name = name; }
        inline const std::string & GetName()const                      { return m_name; }

        /*
            Set/Get the loop points for this sample.
                beg and end must be relative to the beginning of the sample's data!
        */
        void                            SetLoopBounds( size_t beg, size_t end );
        inline std::pair<size_t,size_t> GetLoopBounds()const                    { return std::make_pair( m_loopbeg, m_loopend ); }

        /*
            Set/Get Sample Rate
        */
        inline void     SetSampleRate( uint32_t smplrate ) { m_smplrate = smplrate; }
        inline uint32_t GetSampleRate()const               { return m_smplrate; }

        /*
            Set/Get Original Midi Key
                AKA the MIDI key the sampled instrument is playing.
                Valid values between 0-127 are in the legal range, and 255 is legal for
                unpitched sounds. Any other values should be rejected.
        */
        inline void    SetOriginalKey( uint8_t origkey ) { if( origkey != 255 ) m_origkey = (origkey & 0x7F); else m_origkey = origkey; }
        inline uint8_t GetOriginalKey()const             { return m_origkey; }    

        /*
            Set/Get Pitch Correction
                Correction in cent to apply. Range is -127 to 127.
        */
        inline void   SetPitchCorrection( int8_t pc ) { m_pitchcorr = pc; }
        inline int8_t GetPitchCorrection()const       { return m_pitchcorr; }

        /*
            SAMPLE LINK IS UNSUPORTED RIGHT NOW
        */
        inline void     SetLinkedSample( uint16_t smplindex ) { m_linkedsmpl = smplindex; }
        inline uint16_t GetLinkedSample()const                { return m_linkedsmpl; }

        /*
            Get/Set Sample Type
        */
        inline void    SetSampleType( eSmplTy type ) { m_samplety = type; }
        inline eSmplTy GetSampleType()const          { return m_samplety; }

    private:

        //Sample Loading Details
        eLoadType                            m_loadty;

        loadfun_t                            m_loadfun;
        std::string                          m_fpath;

        std::vector<uint8_t>               * m_pRawVec;
        uint8_t                            * m_pRaw;

        std::weak_ptr<std::vector<pcm16s_t>> m_pPcmVec;
        std::weak_ptr<pcm16s_t>              m_pPcm;

        //Actual Sample Data
        std::string                          m_name;
        size_t                               m_begoff;
        size_t                               m_endoff;

        size_t                               m_loopbeg;
        size_t                               m_loopend;

        uint32_t                             m_smplrate;
        uint8_t                              m_origkey;
        int8_t                               m_pitchcorr;
        uint16_t                             m_linkedsmpl;
        eSmplTy                              m_samplety;
    };

    /***********************************************************************************
        SoundFont
            Represent a SounFont in memory.
    ***********************************************************************************/
    class SoundFont
    {
    public:
        SoundFont();
        SoundFont( const std::string & sfname );

        size_t                      AddSample( Sample && smpl ); //Returns index into the SHDR sub-chunk
        std::vector<Sample>       & GetSamples();
        const std::vector<Sample> & GetSamples()const;
        Sample                    & GetSample( size_t index );
        inline size_t               GetNbSamples()const       { return m_samples.size(); }

        size_t                      AddPreset( Preset && preset );
        std::vector<Preset>       & GetPresets();
        const std::vector<Preset> & GetPresets()const;
        Preset                    & GetPreset( size_t index );
        const Preset              & GetPreset( size_t index )const;

        inline size_t               GetNbPresets()const       { return m_presets.size(); }


        /*
            Get/Set Name
                Name will be truncated to the first 255 characters.
        */
        void                SetName( const std::string & sfname ) { m_sfname = sfname; }
        const std::string & GetName()const                        { return m_sfname; }

        /*
            Write
                Write the soundfont to a .sf2 file.

                Returns the nb of bytes written.
        */
        size_t Write( const std::string & sf2path );

        /*
            Read
                Read a sounfont from a file.

                **We need to hang on the file internally, for delayed sample loading.**
        */
        size_t Read( const std::string & sf2path );

    private:
        std::string         m_sfname;

        std::vector<Sample> m_samples;
        std::vector<Preset> m_presets;
    };

};

#endif