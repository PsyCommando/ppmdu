#include "midi_io.hpp"

#include <jdksmidi/world.h>
#include <jdksmidi/track.h>
#include <jdksmidi/multitrack.h>
#include <jdksmidi/filereadmultitrack.h>
#include <jdksmidi/fileread.h>
#include <jdksmidi/fileshow.h>
#include <jdksmidi/filewritemultitrack.h>

using namespace std;

namespace pmd2 { namespace audio 
{

    /*
        
    */
    class DSEToMid
    {
    public:



    private:
    };

//
//  Functions:
//

    /*
    */
    void WriteSequenceToMIDI( const MusicSequence & seq, const std::string & filename )
    {
    }

    /*
        presetconvtbl == A conversion table to convert the instrument presets to the desired values.
                         The index value is the DSE preset, the value at that index is the value to be used instead.

        asGM          == If set to true, the sequence is modified to be played back properly by MIDI compatible players and DAWs using
                         a General MIDI instrument bank.
    */
    void WriteSequenceToMIDI( const MusicSequence        & seq, 
                              const std::string          & filename, 
                              const std::vector<uint8_t> & presetconvtbl, 
                              bool                         asGM )
    {
    }

    /*
    */
    //MusicSequence ReadMIDIToSequence( const std::string & filename )
    //{
    //}

};};