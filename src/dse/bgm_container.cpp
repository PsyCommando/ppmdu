#include "bgm_container.hpp"
#include <ppmdu/fmts/sir0.hpp>
#include <ppmdu/fmts/smdl.hpp>
#include <ppmdu/fmts/swdl.hpp>
#include <ppmdu/fmts/sedl.hpp>
#include <sstream>
#include <fstream>
#include <array>
#include <iostream>
#include <types/content_type_analyser.hpp>
#include <utils/library_wide.hpp>

using namespace std;
using namespace filetypes;

namespace DSE
{
//==========================================================================================
//
//==========================================================================================

    /*
        Returns the offsets of the swdl and smdl in order.
    */
    template<class _init>
        std::array<uint32_t,2> ReadOffsetsSubHeader( _init itread, uint32_t subhdroffset )
    {
        std::array<uint32_t,2> offsets = {0};
        std::advance( itread, subhdroffset );
        itread = utils::ReadIntFromBytes( offsets[0], itread );
        itread = utils::ReadIntFromBytes( offsets[1], itread );
        return offsets;
    }

//==========================================================================================
//  Functions
//==========================================================================================

    /*
        IsBgmContainer
            Pass a path to a file, and the function will return whether or not its a bgm container.
            
            It will actually check to see if its a SIR0 container, then it will attempt to look at the
            sub-header of the SIR0 wrapper, and if there are 2 pointers, one to a swdl and another to a smdl, 
            then it will return true!
            It won't do any kind of validation on the smdl and swdl though. It only checks for magic numbers.

    */
    bool IsBgmContainer( const std::string & filepath )
    {
        ifstream infile( filepath, ios::in | ios::binary );
        
        if( infile.is_open() && infile.good() )
        {
            sir0_header hdr;

            hdr.ReadFromContainer( istreambuf_iterator<char>(infile) );
            if( hdr.magic == sir0_header::MAGIC_NUMBER )
            {
                infile.seekg(0);
                auto offsets = ReadOffsetsSubHeader( istreambuf_iterator<char>(infile) , hdr.subheaderptr );

                SWDL_Header swdhdr;
                SMDL_Header smdhdr;
                infile.seekg(offsets[0]);
                swdhdr.ReadFromContainer( istreambuf_iterator<char>(infile) );

                infile.seekg(offsets[1]);
                smdhdr.ReadFromContainer( istreambuf_iterator<char>(infile) );

                if( swdhdr.magicn == SWDL_MagicNumber && smdhdr.magicn == SMDL_MagicNumber )
                    return true;
            }
        }
        else
            throw runtime_error( "IsBgmContainer() : Failed to open file " + filepath );

        return false;
    }


    /*
        ReadBgmContainer
            Read the bgm container's content into a preset bank and a musicsequence.
    */
    std::pair<PresetBank, MusicSequence> ReadBgmContainer( const std::string & filepath )
    {
        vector<uint8_t> fdata( move( utils::io::ReadFileToByteVector( filepath ) ) );
        sir0_header     hdr;

        hdr.ReadFromContainer( fdata.begin() );

        if( hdr.magic != sir0_header::MAGIC_NUMBER )
            throw runtime_error( "ReadBgmContainer() : File is missing SIR0 header!" );
            
        auto        offsets = ReadOffsetsSubHeader( fdata.begin() , hdr.subheaderptr );

        SWDL_Header swdhdr;
        SMDL_Header smdhdr;
        swdhdr.ReadFromContainer( fdata.begin() + offsets[0] );
        smdhdr.ReadFromContainer( fdata.begin() + offsets[1] );

        size_t smdloffset = 0;
        size_t swdloffset = 0;

        //Check that the swdl and smdl containers are in the right order, or adapt if they're inverted
        if( swdhdr.magicn == SWDL_MagicNumber && smdhdr.magicn == SMDL_MagicNumber )
        {
            //If in this order
            swdloffset = offsets[0]; 
            smdloffset = offsets[1];
        }
        else if( smdhdr.magicn == SWDL_MagicNumber && swdhdr.magicn == SMDL_MagicNumber )
        {
            //If in inverted order
            swdloffset = offsets[1];
            smdloffset = offsets[0];

            if( utils::LibWide().isLogOn() )
                clog << "<!>- SWDL and SMDL containers were inverted in the bgm container \"" <<filepath <<"\" !\n";
        }
        else
        {
            stringstream sstrerror;

            //throw runtime_error( "ReadBgmContainer(): Bgm container doesn't contain" );
            sstrerror << "ReadBgmContainer(): Bgm container has unexpected content!";

            if( smdhdr.magicn == SMDL_MagicNumber )
                sstrerror << " SMDL header present! ";
            else
                sstrerror << " SMDL header missing! ";

            if( swdhdr.magicn == SWDL_MagicNumber )
                sstrerror << "SWDL header present! ";
            else
                sstrerror << "SWDL header missing! ";

            if( smdhdr.magicn == SEDL_MagicNumber )
                sstrerror << "Found unexpected SEDL header! " << "Bgm container is unexpected sound effect container!";
            else
                sstrerror << "Found unknown header magic number: " <<showbase <<hex <<smdhdr.magicn <<dec <<noshowbase << "!\n";
            throw runtime_error( sstrerror.str() );
        }

        auto itbegswdl = fdata.begin() + swdloffset;
        auto itendswdl = fdata.begin() + smdloffset;
        auto itbegsmdl = itendswdl;
        auto itendsmdl = fdata.begin() + hdr.subheaderptr;

        return move( make_pair( move(ParseSWDL( itbegswdl, itendswdl )), move(ParseSMDL( itbegsmdl, itendsmdl )) ) );               
    }

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
                            size_t                nbpadcontent,
                            size_t                alignon )
    {
    }
};