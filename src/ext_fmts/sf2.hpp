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
#include <utils/utility.hpp>
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <memory>
#include <array>
#include <functional>


/***************************************************************************************************************
                                ===========================================
                                == Simplified Layout of a SoundFont file ==
                                ===========================================

                                ----------------------------------------
                                |           SoundFont File             |
                                ----------------------------------------
                                                   | 1
                                                   |
                                                   |
                -----------------------------------------------------------------------
                |                                  |                                  |
                V 1..n                             |                                  V 1..n
    ------------------------------                 |                    ------------------------------
    |         Samples            |                 |                    |           Presets          |
    ------------------------------                 V 1..n               ------------------------------
                ^ 1                   ------------------------------ 1                | 1        
                |                     |         Instruments        |<--               |
                |                     ------------------------------  |               V 1..n
                |                                  | 1                | ------------------------------
                |                                  |                  | |         PBagEntry          |
                |                                  V 1..n             | ------------------------------
                |                     ------------------------------  |               | 1
                |                     |        IBagEntry           |  |               |
                |                     ------------------------------  | 1             V 0..n
                |                                  | 1                | ------------------------------
                |                                  |                  --|          PMod/PGen **      |
                |                                  V 0..n               ------------------------------
                |                   1 ------------------------------
                ----------------------|         IMod/IGen **       |
                                      ------------------------------


        **NOTE: PGens/PMods and IGens/IMods aren't solely for linking to instruments or samples. 
                Only Generators may link to samples or instruments. And there are many different
                types of Generators, most of which do not link to a sample or instrument.

                But, you still need at least one Bag entry per link to one Sample or Instrument. 
                More Bag entries if you're linking to more Instruments or Samples. 
                Because, while you can have many generators all of different types refered to by 
                a Bag entry, you can only have a single Instrument or Sample ID Generator per Bag entry!

                Which is something I feel the official documentation doesn't insists enough on..
                
***************************************************************************************************************/

namespace sf2
{

//===========================================================================================
//  Utility Functions
//===========================================================================================
    /***********************************************************************************
        SecondsToTimecents
            Convert a duration in seconds to a duration in timecents.
    ***********************************************************************************/
    inline int32_t SecondsToTimecents( int32_t seconds )
    {
        return lround( log2(seconds) * 1200.00 );
    }

    /***********************************************************************************
        MSecsToTimecents
            Convert a duration in milliseconds to a duration in timecents.
    ***********************************************************************************/
    inline int32_t MSecsToTimecents( int32_t msecs )
    {
        //static const double LOG2_VAL = log(2.00);
        double sec = ( static_cast<double>(msecs) / 1000.00 );
        //!#FIXME: I hate logs... And I hate time cents..
        //!        I can't figure out who has the correct formula to turn msec into timecents..
        //return lround( (sec / LOG2_VAL) /*+ 6.66666*/ );
        //return lround( log(sec) / log( 2.00 ) * 1200.00 );
        return lround( 1200.00 * log2(sec) );
    }

    inline int32_t MSecsToTimecentsDecay( int32_t msecs )
    {
        return MSecsToTimecents(msecs);
    }

//===========================================================================================
//  Constants
//===========================================================================================
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

    /************************************************************************************
        eSmplMode
            Special enum value for the sample mod generator
    ************************************************************************************/
    enum struct eSmplMode : uint16_t
    {
        noloop        = 0,
        loop          = 1,
        unused        = 2,
        loopWhileHold = 3, //Loop while holding the key, then play the rest of the sample
    };

//===========================================================================================
//  SF2 Values Limits
//===========================================================================================
    //Values ranges for the Generators!
                                                                              //     Min,      Def,   Max,  Mid
    static const utils::value_limits<int16_t>  SF_GenLimitsModLfoToPitch      {   -12000,        0, 12000,    0 };
    static const utils::value_limits<int16_t>  SF_GenLimitsVibLfoToPitch      {   -12000,        0, 12000,    0 };
    static const utils::value_limits<int16_t>  SF_GenLimitsModEnvLfoToPitch   {   -12000,        0, 12000,    0 };
    static const utils::value_limits<uint16_t> SF_GenLimitsInitFilterFc       {     1500,    13500, 13500, 1500 };
    static const utils::value_limits<uint16_t> SF_GenLimitsInitFilterQ        {        0,        0,   960,    0 };
    static const utils::value_limits<int16_t>  SF_GenLimitsModLfoToFilterFc   {   -12000,        0, 12000,    0 };
    static const utils::value_limits<int16_t>  SF_GenLimitsModEnvLfoToFilterFc{   -12000,        0, 12000,    0 };
    static const utils::value_limits<int16_t>  SF_GenLimitsModLfoToVolume     {     -960,        0,   960,    0 };
    static const utils::value_limits<uint16_t> SF_GenLimitsChorusSend         {        0,        0,  1000,  500 };
    static const utils::value_limits<uint16_t> SF_GenLimitsReverbSend         {        0,        0,  1000,  500 };
    static const utils::value_limits<int16_t>  SF_GenLimitsPan                {     -500,        0,   500,    0 };

    static const utils::value_limits<int16_t>  SF_GenLimitsModLfoDelay        {   -12000,   -12000,  5000,    0 };
    static const utils::value_limits<int16_t>  SF_GenLimitsModLfoFreq         {   -16000,        0,  4500,    0 };
    static const utils::value_limits<int16_t>  SF_GenLimitsVibLfoDelay        {   -12000,   -12000,  5000,    0 };
    static const utils::value_limits<int16_t>  SF_GenLimitsVibLfoFreq         {   -16000,        0,  4500,    0 };

    static const utils::value_limits<int16_t>  SF_GenLimitsModEnvDelay        {   -12000,   -12000,  5000,    0 };
    static const utils::value_limits<int16_t>  SF_GenLimitsModEnvAttack       {   -12000,   -12000,  8000,    0 };
    static const utils::value_limits<int16_t>  SF_GenLimitsModEnvHold         {   -12000,   -12000,  5000,    0 };
    static const utils::value_limits<int16_t>  SF_GenLimitsModEnvDecay        {   -12000,   -12000,  8000,    0 };
    static const utils::value_limits<uint16_t> SF_GenLimitsModEnvSustain      {        0,        0,  1000,  500 };
    static const utils::value_limits<int16_t>  SF_GenLimitsModEnvRelease      {   -12000,   -12000,  8000,    0 };

    static const utils::value_limits<int16_t>  SF_GenLimitsKeynumToModEnvHold {    -1200,        0,  1200,    0 };
    static const utils::value_limits<int16_t>  SF_GenLimitsKeynumToModEnvDecay{    -1200,        0,  1200,    0 };

    static const utils::value_limits<int16_t>  SF_GenLimitsVolEnvDelay        { SHRT_MIN,   -12000,  5000,    0 }; //Shortest valid value is -12000. SHRT_MIN means its disabled. Other values between SHRT_MIN and -12000 have undefined effects!
    static const utils::value_limits<int16_t>  SF_GenLimitsVolEnvAttack       { SHRT_MIN,   -12000,  8000,    0 };
    static const utils::value_limits<int16_t>  SF_GenLimitsVolEnvHold         { SHRT_MIN,   -12000,  5000,    0 };
    static const utils::value_limits<int16_t>  SF_GenLimitsVolEnvDecay        { SHRT_MIN,   -12000,  8000,    0 };
    static const utils::value_limits<uint16_t> SF_GenLimitsVolEnvSustain      {        0,        0,  1440,  720 };
    static const utils::value_limits<int16_t>  SF_GenLimitsVolEnvRelease      { SHRT_MIN,   -12000,  8000,    0 };

    static const utils::value_limits<int16_t>  SF_GenLimitsKeynumToVolEnvHold {    -1200,        0,  1200,    0 };
    static const utils::value_limits<int16_t>  SF_GenLimitsKeynumToVolEnvDecay{    -1200,        0,  1200,    0 };

    static const utils::value_limits<uint16_t> SF_GenLimitsKeyRange           {        0,   0x007F, 0x7F7F,   0 };
    static const utils::value_limits<uint16_t> SF_GenLimitsVelRange           {        0,   0x007F, 0x7F7F,   0 };

    static const utils::value_limits<int16_t> SF_GenLimitsKeynum              {        0,       -1,   127,    0 };
    static const utils::value_limits<int16_t> SF_GenLimitsVelocity            {        0,       -1,   127,    0 };

    static const utils::value_limits<int16_t> SF_GenLimitsInitAttenuation     {        0,        0,  1440,    0 };

    static const utils::value_limits<int16_t> SF_GenLimitsCoarseTune          {     -120,        0,   120,    0 };
    static const utils::value_limits<int16_t> SF_GenLimitsFineTune            {      -99,        0,    99,    0 };

    static const utils::value_limits<uint16_t> SF_GenLimitsScaleTuning        {        0,      100,  1200,  600 };
    static const utils::value_limits<uint16_t> SF_GenLimitsExcClass           {        0,        0,   127,    0 };
    static const utils::value_limits<uint16_t> SF_GenLimitsOverrideRootKey    {        0,       -1,   127,    0 };

//===========================================================================================
//  Structures and Enums
//===========================================================================================
    /************************************************************************************
        SFModulatorSrc
            http://www.pjb.com.au/midi/sfspec21.html#8.2
            Represent a single modulator source. 
            Not to be confused with a modulator entry.

            The Modulator source is basically a int16 crammed full of details 
            about the source of the data modulating the selected output value/state.

            This is mainly a convenience struct to make it easier than
            to edit all the bits manually each times.
    ************************************************************************************/
    struct SFModulatorSrc
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
        SFModulatorSrc( eSrc     type      = eSrc::Linear, 
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
        SFModulatorSrc & operator=( uint16_t other )
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
        genparam_t
           Type for containing the parameter of a Generator.
    ***********************************************************************************/
    typedef uint16_t genparam_t;

    /***********************************************************************************
        SFGenEntry
            http://www.pjb.com.au/midi/sfspec21.html#7.5
            Basically represent a generator entry for either a Preset or Instrument.
    ***********************************************************************************/
    struct SFGenEntry  
    {
        static const uint32_t SIZE = 4; //bytes
        eSFGen     GenOper   = eSFGen::startAddrsOffset;  //Is == 0
        genparam_t genAmount; //Can be used a uint16, a int16, or 2 bytes. But that's based on the generator, and up to the user.
    };

    /***********************************************************************************
        SFModEntry
            http://www.pjb.com.au/midi/sfspec21.html#7.4
            Basically represent a modulator entry for a Preset or Instrument
    ***********************************************************************************/
    struct SFModEntry
    {
        static const uint32_t SIZE = 10; //bytes

        SFModulatorSrc ModSrcOper;                               //By default is 0  
        eSFGen         ModDestOper   = eSFGen::startAddrsOffset; //Is == 0
        int16_t        modAmount     = 0;  
        SFModulatorSrc ModAmtSrcOper;                            //By default is 0 
        eSFTransform   ModTransOper  = eSFTransform::linear;     //Is == 0
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
        int16_t delay   = SF_GenLimitsVolEnvDelay.def_;         //timecents
        int16_t attack  = SF_GenLimitsVolEnvAttack.def_;        //timecents
        int16_t hold    = SF_GenLimitsVolEnvHold.def_;          //timecents
        int16_t sustain = SF_GenLimitsVolEnvSustain.def_;       //Attenuation in cB (144 dB is 1440 cB for instance)
        int16_t decay   = SF_GenLimitsVolEnvDecay.def_;         //timecents
        int16_t release = SF_GenLimitsVolEnvRelease.def_;       //timecents
    };


//==================================================================================
//  SoundFont Classes
//==================================================================================
    /***********************************************************************************
        BaseGeneratorUser
            Base class that implements methods common to all generator users.
    ***********************************************************************************/
    class BaseGeneratorUser
    {
    public:
        //Functor to replace the generator map sort predicate!
        struct CmpPriority
        {
            inline bool operator()(eSFGen gen1, eSFGen gen2 )const
            { 
                //Keyrange always in first !
                if( gen1 == eSFGen::keyRange )
                    return true;
                else if( gen2 == eSFGen::keyRange )
                    return false;

                //Vel range is next if there isn't a key range being compared
                if( gen1 == eSFGen::velRange )
                    return true;
                else if( gen2 == eSFGen::velRange )
                    return false;

                //Then if its either the Sample ID or instrument id, make them always loose a comparison, so they end up at the bottom!
                if( gen1 == eSFGen::sampleID || gen1 == eSFGen::instrument )
                    return false;   // Always bigger than gen2
                else if( gen2 == eSFGen::sampleID || gen2 == eSFGen::instrument )
                    return true;  //Always bigger than gen1

                //If not one of the special cases above, just compare their enum value
                return gen1 < gen2; 
            } 
        };
        typedef std::map<eSFGen,genparam_t,CmpPriority> genlist_t;

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
                Return all generators.
        */
        genlist_t       & GetGenerators()      { return m_gens; }
        const genlist_t & GetGenerators()const { return m_gens; }

        /*
            GetNbGenerators
        */
        size_t GetNbGenerators()const;

        ////////////////////////////////////////
        //  Common Generators Helper Methods
        ////////////////////////////////////////
        
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

        /*
            Set or Get the Root Key
                (def:-1, min:0, max:127)
                This overrides the root key from the sample's smaple header.
                A value of -1, does nothing.
        */
        void    SetRootKey( int16_t key );
        int16_t GetRootKey()const;

    protected:
        /*
            genpriority_t
                A number indicating the priority of a generator in the list.
                High priority generators will be placed at the begining, ordered by priority.
                Low priority generators will be placed at the end, ordered by priority.

                Its really stupid, but since I'm using an std::map, and that
                the comparison predicate used to sort elements is also used to 
                determine whether 2 keys are identical or not, we need to assign
                a unique priority value to each generator type..
        */
        typedef uint16_t genpriority_t;
        static const genpriority_t DefaultPriority = USHRT_MAX/2; 
        static const genpriority_t HighPriority    = USHRT_MAX;
        static const genpriority_t LowPriority     = 0;

        /*
            GetGenPriority
                This method returns the priority of a generator. 
        */
        static genpriority_t GetGenPriority( eSFGen gen );

    protected:
        genlist_t m_gens; //There can only be a single generator of a given type
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
        size_t AddModulator( SFModEntry && mod );

        /*
            GetModulator
                Return the modulator at the index specified.
        */
        SFModEntry       & GetModulator( size_t index );
        const SFModEntry & GetModulator( size_t index )const;

        std::vector<SFModEntry>       & GetModulators()      { return m_mods; }
        const std::vector<SFModEntry> & GetModulators()const { return m_mods; }

        /*
            GetNbModulators
        */
        size_t GetNbModulators()const;

        //---------------------
        //  Common Modulators
        //---------------------
        //None. Modulators are very rarely used !

    protected:
        std::vector<SFModEntry> m_mods;
    };

    /***********************************************************************************
        ZoneBag
            Represent a list of generator and modulators for a given Zone.

            Each zones can only have a single Instrument or SampleID generator.


            (Still, its overcomplicated for nothing.. But I didn't write this format..)
    ***********************************************************************************/
    class ZoneBag : public BaseModulatorUser, public BaseGeneratorUser
    {
    public:
        /*
            Sort
                Sort the Generators and Modulators in the order the SF2 format requires.
                Copies of generators/modulators that should appear only once are moved
                at the end of the list, and will be ignored by anything conforming to the SF2 standard.
        */
        /*void Sort();*/
    }; //Not much to put in here..

    /***********************************************************************************
        BaseZoneBagOwner
            Class for handling common tasks to all Bag users.

            Handle assigning new zone, and accessing them.
    ***********************************************************************************/
    class BaseZoneBagOwner
    {
    public:
        virtual ~BaseZoneBagOwner(){}

        size_t          AddZone(ZoneBag && zone)      { m_zones.push_back(std::move(zone)); return (m_zones.size()-1); } //Can't trust MSVC with move op..
        ZoneBag       & GetZone( size_t index )       { return m_zones[index]; }
        const ZoneBag & GetZone( size_t index )const  { return m_zones[index]; }
        size_t          GetNbZone()const              { return m_zones.size(); }

        /*
            SortZonesGens
                Sorts the generators in all of the preset's zones!
        */
        //inline void SortZonesGens()
        //{
        //    for( auto & zone : m_zones )
        //        zone.Sort();
        //}

    protected:
        std::vector<ZoneBag> m_zones;
    };


    /***********************************************************************************
        Instrument
            Represent a single Soundfont instrument entry.
    ***********************************************************************************/
    class Instrument : public BaseZoneBagOwner
    {
    public:
        //----------------
        //  Construction
        //----------------
        Instrument();
        Instrument( const std::string & name );

        //----------------
        //  Properties
        //----------------
        /*
            Get/Set Name
                Name will be truncated to the first 19 characters.
        */
        inline void                SetName( const std::string & name ){ m_name = name; }
        inline const std::string & GetName()const                     { return m_name; }

    private:
        std::string          m_name;
    };


    /***********************************************************************************
        Preset
    ***********************************************************************************/
    class Preset : public BaseZoneBagOwner
    {
    public:

        //---------------
        //  Contructors
        //---------------
        Preset();
        Preset( const std::string & name, uint16_t presetno, uint16_t bankno = 0, uint32_t lib = 0, uint32_t genre = 0, uint32_t morpho = 0 );

        //------------
        //  Properties
        //------------

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
                Set the Soundfont bank index to assign to this particular preset.
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

        //
        //
        //


    private:
        std::string                   m_name;
        uint16_t                      m_presetNo;
        uint16_t                      m_bankNo;
        uint32_t                      m_library;
        uint32_t                      m_genre;
        uint32_t                      m_morpho;

        //std::vector<Instrument>       m_instruments;
    };

    /***********************************************************************************
        Sample
    ***********************************************************************************/

    /*
        ##########################################################################################################
        #TODO: Obfuscate sample handling and sample sizes better !!
        #      We need to know the actual PCM16 len for a lot of calculations, and the begoffset and endoffsets
        #      are usually byte offsets, but could also be pcm16 offsets.. This is way too confusing and error 
        #      prone..
        ##########################################################################################################
    */

    class Sample
    {
        //enum struct eLoadType
        //{
        //    DelayedRawVec,
        //    DelayedPCMVec,
        //    DelayedRaw,
        //    DelayedPCM,

        //    DelayedFile,
        //    DelayedFunc,
        //};
    public:
        /*
        */
        typedef std::function<std::vector<pcm16s_t>()> loadfun_t;
        typedef uint32_t                               smplcount_t; //Express a quantity in amount of pcm16 samples

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
            Load from a function. "samplelen" is the length in pcm16 data points of the sound sample that 
            the function "funcload" will return!
        */
        Sample( loadfun_t && funcload, smplcount_t samplelen );

        /*
            This one copy from a range the signed pcm16 data.
        */
        template<class _init>
            Sample( _init               itbeg, 
                    _init               itend, 
                    const std::string & name, 
                    smplcount_t         loopbeg  =     0, 
                    smplcount_t         loopend  =     0,
                    uint32_t            smplrate = 44100,
                    uint8_t             origkey  =    60,
                    int8_t              pitchcor =     0,
                    eSmplTy             type     = eSmplTy::monoSample )
                :m_pcmdata  (itbeg, itend),
                 m_loopbeg  (loopbeg),
                 m_loopend  (loopend),
                 m_name     (name),
                 m_origkey  (origkey),
                 m_pitchcorr(pitchcor),
                 m_samplety (type),
                 m_smplrate (smplrate),
                 m_linkedsmpl(0)
        {
            m_smpllen = m_pcmdata.size();
        }

        

        /*
            Obtain the data from the sample.
            This execute the function passed to the constructor, and move the result out!
        */
        operator std::vector<pcm16s_t>()const;

        /*
            Obtain the data from the sample.
            Wrapper over delayed read operations
            This execute the function passed to the constructor, and move the result out!
        */
        std::vector<pcm16s_t> Data()const;

        /*
            Return the length of the data in bytes.
        */
        //inline size_t GetDataByteLength()const { return (m_endoff - m_begoff); }

        /*
            Returns the length of the data in sample points !
        */
        inline smplcount_t GetDataSampleLength()const { return m_smpllen; }

        /*
            Returns the length of the data in sample points ! If the beg and end offset were the range in byte containing the ADPCM data.
            Basically, it multiplies the byte len by 2, then substract the 4 bytes preamble.
        */
        //inline smplcount_t GetDataSampleLengthADPCM()const { return ((GetDataByteLength() - 4) * sizeof(pcm16s_t)); }

        /*
            Get/Set Name
                Name will be truncated to the first 19 characters.
        */
        inline void                SetName( const std::string & name ) { m_name = name; }
        inline const std::string & GetName()const                      { return m_name; }

        /*
            Set/Get the loop points for this sample.
                The loop points in pcm16 data points to be used with this sound sample.
        */
        void                                      SetLoopBounds( smplcount_t beg, smplcount_t end );
        inline std::pair<smplcount_t,smplcount_t> GetLoopBounds()const                    
        { 
            return std::make_pair( m_loopbeg, m_loopend ); 
        }

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
        //eLoadType                            m_loadty;

        loadfun_t                            m_loadfun;
        //std::string                          m_fpath;
        std::vector<int16_t>                 m_pcmdata;

        //std::vector<uint8_t>               * m_pRawVec;
        //uint8_t                            * m_pRaw;

        //std::weak_ptr<std::vector<pcm16s_t>> m_pPcmVec;
        //std::weak_ptr<pcm16s_t>              m_pPcm;

        //Actual Sample Data
        std::string                          m_name;
        //size_t                               m_begoff; //Offset are
        //size_t                               m_endoff;
        smplcount_t                          m_smpllen;

        smplcount_t                          m_loopbeg;
        smplcount_t                          m_loopend;

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
                                    
        size_t                      AddInstrument( Instrument && inst );
        inline Instrument         & GetInstument( size_t index )      { return m_instruments[index]; }
        inline const Instrument   & GetInstument( size_t index )const { return m_instruments[index]; }
        inline size_t               GetNbInstruments()const           { return m_instruments.size(); }


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
        std::string             m_sfname;

        std::vector<Sample>     m_samples;
        std::vector<Preset>     m_presets;
        std::vector<Instrument> m_instruments;
    };

};

#endif