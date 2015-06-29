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
    void WriteSequenceToMidi( const MusicSequence & seq, const std::string & filename )
    {
    }

    /*
    */
    //MusicSequence ReadMidiToSequence( const std::string & filename )
    //{
    //    assert(false);
    //    return MusicSequence();
    //}

};};