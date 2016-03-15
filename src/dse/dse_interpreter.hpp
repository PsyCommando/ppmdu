#ifndef DSE_INTERPRETER_HPP
#define DSE_INTERPRETER_HPP
/*
dse_interpreter.hpp
2015/07/01
psycommando@gmail.com
Description: This class is meant to interpret a sequence of DSE audio events into standard MIDI events or text.

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P
*/
#include <dse/dse_conversion.hpp>
#include <dse/dse_common.hpp>
#include <dse/dse_sequence.hpp>
#include <dse/dse_conversion_info.hpp>
//#include <vector>
#include <string>
//#include <map>
#include <cstdint>

namespace DSE
{
    static const uint32_t NbMicrosecPerMinute = 60000000;
    //#TODO: Put this into a MIDI utility header, or something like that..
    inline uint32_t ConvertTempoToMicrosecPerQuarterNote( uint32_t bpm )
    {
        return NbMicrosecPerMinute / bpm;
    }

    inline uint32_t ConvertMicrosecPerQuarterNoteToBPM( uint32_t mpqn )
    {
        return mpqn / NbMicrosecPerMinute;
    }

//===============================================================================
//  Export Utilities
//===============================================================================
    /*************************************************************************************************
        eMIDIFormat
            The standard MIDI file format to use to export the MIDI data.
            - SingleTrack : Is format 0, a single track for all events.
            - MultiTrack  : Is format 1, one dedicated tempo track, and all the other tracks for events.
    *************************************************************************************************/
    //enum struct eMIDIFormat
    //{
    //    SingleTrack,
    //    MultiTrack,
    //};


    /*************************************************************************************************
        eMIDIMode
            The MIDI file's "sub-standard".
            - GS inserts a GS Mode reset SysEx event, and then turns the drum channel off.
            - XG insets a XG reset Sysex event.
            - GM doesn't insert any special SysEx events.
    *************************************************************************************************/
    enum struct eMIDIMode
    {
        GM,
        GS,
        XG, //#TODO: Fix XG export. Though its vastly unsupported.
    };

    /*************************************************************************************************
        SequenceToMidi
            This function convert a MusicSequence to a midi according to the parameters specified!
                -outmidi    : The path and name of the MIDI file that will be exported.
                - seq       : The MusicSequence to export.
                - remapdata : Information on how each DSE track presets, translate to MIDI presets.
                - midmode   : The MIDI sub-standard to use for bypassing GM's limitations. 
                              GS is preferred as its the most supported one!
    *************************************************************************************************/
    void SequenceToMidi( const std::string              & outmidi, 
                         const MusicSequence            & seq, 
                         const SMDLPresetConversionInfo & remapdata,
                         int                              nbloop      = 0,
                         eMIDIMode                        midmode     = eMIDIMode::GS );

    /*************************************************************************************************
        SequenceToMidi
            Same as above, except without any preset conversion info!
    *************************************************************************************************/
    void SequenceToMidi( const std::string              & outmidi, 
                         const MusicSequence            & seq, 
                         int                              nbloop      = 0,
                         eMIDIMode                        midmode     = eMIDIMode::GS );


    /*************************************************************************************************
        MidiToSequence
            Converts a MIDI file into a DSE Sequence.
    *************************************************************************************************/
    MusicSequence MidiToSequence( const std::string & inmidi );

};

#endif