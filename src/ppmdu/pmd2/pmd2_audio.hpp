#ifndef PMD2_AUDIO_HPP
#define PMD2_AUDIO_HPP
/*
pmd2_audio.hpp
*/
#include <cstdint>
#include <map>
#include <string>
#include <memory>

namespace pmd2
{

    /*
        GameAudio
            Indexes the game's audio data for easier access and retrieval.
    */
    class GameAudio
    {
    public:
        GameAudio( const std::wstring & snddir );

        void IndexData(  );

        //Load specific parts
        //audio::MusicSequence LoadMusicTrack( unsigned int trkno );

        //audio::MusicSequence LoadMETrack( unsigned int trkno );

        // LoadEffects();

    private:

    };

};

#endif 