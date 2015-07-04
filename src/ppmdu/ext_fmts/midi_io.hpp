#ifndef MIDI_IO_HPP
#define MIDI_IO_HPP
/*
midi_io.hpp
2015/06/27
psycommando@gmail.com
Description: A class for handling common conversion to MIDI. And parsing from MIDI files.

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P
*/
#include <ppmdu/pmd2/pmd2_audio_data.hpp>
#include <ppmdu/fmts/dse_common.hpp>
#include <ppmdu/fmts/dse_sequence.hpp>
#include <vector>
#include <string>
#include <cstdint>

namespace pmd2 { namespace audio
{

    /*
    */
    void WriteSequenceToMIDI( const MusicSequence & seq, const std::string & filename );

    /*
        presetconvtbl == A conversion table to convert the instrument presets to the desired values.
                         The index value is the DSE preset, the value at that index is the value to be used instead.

        asGM          == If set to true, the sequence is modified to be played back properly by MIDI compatible players and DAWs using
                         a General MIDI instrument bank.
    */
    void WriteSequenceToMIDI( const MusicSequence        & seq, 
                              const std::string          & filename, 
                              const std::vector<uint8_t> & presetconvtbl, 
                              bool                         asGM = false );

    /*
    */
    MusicSequence ReadMIDIToSequence( const std::string & filename );

};};

#endif