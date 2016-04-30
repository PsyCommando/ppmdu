#ifndef BGM_CONTAINER
#define BGM_CONTAINER
/*
bgm_container.hpp
2015/12/11
psycommando@gmail.com
Description: This is meant to handle smdl and swdl files packed together as a pair into a SIR0 container!
*/
#include <ppmdu/fmts/sir0.hpp>
#include <dse/dse_containers.hpp>
#include <string>
#include <utility>

namespace DSE
{
    /*
        IsBgmContainer
            Pass a path to a file, and the function will return whether or not its a bgm container.
            
            It will actually check to see if its a SIR0 container, then it will attempt to look at the
            sub-header of the SIR0 wrapper, and if there are 2 pointers, one to a swdl and another to a smdl, 
            then it will return true!
            It won't do any kind of validation on the smdl and swdl though. It only checks for magic numbers.

    */
    bool IsBgmContainer( const std::string & filepath );


    /*
        ReadBgmContainer
            Read the bgm container's content into a preset bank and a musicsequence.
    */
    std::pair<PresetBank, MusicSequence> ReadBgmContainer( const std::string & filepath );

    /*
        WriteBgmContainer
            Writes a bgm container from a music sequence and a preset bank.

            - filepath      : The filename + path to output to. Extension included.
            - presbnk       : A preset bank to be exported.
            - mus           : A music sequence to be exported.
            - nbpadcontent  : The nb of bytes of padding to put between the SIR0 header and the 
                              header of the first sub-container. Usually 16 to 64
            - alignon       : Extra padding will be added between sections and at the end of the file
                              so that section start on offsets divisible by that.

        The SWDL is written first, then the SMDL, and the pointers in the subheader are in that order too !
    */
    void WriteBgmContainer( const std::string   & filepath, 
                            const PresetBank    & presbnk, 
                            const MusicSequence & mus,
                            size_t                nbpadcontent = 64,
                            size_t                alignon      = 32 );

};

#endif