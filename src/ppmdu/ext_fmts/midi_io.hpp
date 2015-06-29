#ifndef MIDI_IO_HPP
#define MIDI_IO_HPP
/*
midi_io.hpp
2015/06/27
psycommando@gmail.com
Description: A class for handling common conversion to MIDI. And parsing from MIDI files.
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
    void WriteSequenceToMidi( const MusicSequence & seq, const std::string & filename );

    /*
        presetconvtbl == A conversion table to convert the instrument presets to the desired values.
                         The index value is the DSE preset, the value at that index is the value to be used instead.
    */
    void WriteSequenceToMidi( const MusicSequence & seq, const std::string & filename, const std::vector<uint8_t> & presetconvtbl );

    /*
    */
    MusicSequence ReadMidiToSequence( const std::string & filename );

};};

#endif