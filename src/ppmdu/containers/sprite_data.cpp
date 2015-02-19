#include "sprite_data.hpp"
#include <string>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <atomic>
#include <ppmdu/containers/tiled_image.hpp>
#include <ppmdu/ext_fmts/png_io.hpp>
#include <ppmdu/ext_fmts/riff_palette.hpp>
#include <ppmdu/utils/poco_wrapper.hpp>
#include <Poco/File.h>
#include <Poco/Path.h>
using namespace std;

namespace pmd2{ namespace graphics
{
    /*
        Equivalence table for sprite resolutions
    */
    const std::vector<MetaFrame::integerAndRes> MetaFrame::ResEquiv=
    {
        { eRes::_8x8,    8,  8 },
        { eRes::_16x16, 16, 16 },
        { eRes::_32x32, 32, 32 },
        { eRes::_64x64, 64, 64 },
        { eRes::_8x16,   8, 16 },
        { eRes::_16x8,  16,  8 },
        { eRes::_8x32,   8, 32 },
        { eRes::_32x8,  32,  8 },
        { eRes::_16x32, 16, 32 },
        { eRes::_32x16, 32, 16 },
        { eRes::_32x64, 32, 64 },
        { eRes::_64x32, 64, 32 },
    };


    utils::Resolution MetaFrame::eResToResolution( MetaFrame::eRes ares )
    {
        switch(ares)
        {
            //square
            case eRes::_8x8  : return graphics::RES_8x8_SPRITE;
            case eRes::_16x16: return graphics::RES_16x16_SPRITE;
            case eRes::_32x32: return graphics::RES_32x32_SPRITE;
            case eRes::_64x64: return graphics::RES_64x64_SPRITE;

            //non-square
            case eRes::_8x16 : return graphics::RES_8x16_SPRITE;
            case eRes::_16x8 : return graphics::RES_16x8_SPRITE;
            case eRes::_32x8 : return graphics::RES_32x8_SPRITE;
            case eRes::_8x32 : return graphics::RES_8x32_SPRITE;
            case eRes::_32x16: return graphics::RES_32x16_SPRITE;
            case eRes::_16x32: return graphics::RES_16x32_SPRITE;
            case eRes::_64x32: return graphics::RES_64x32_SPRITE;
            case eRes::_32x64: return graphics::RES_32x64_SPRITE;
        };
        assert(false); //Asked for invalid resolution !
        return RES_INVALID;
    }

    /*
        Return the correct eRes entry to match the integer representation of the entry.
    */
    MetaFrame::eRes MetaFrame::IntegerResTo_eRes( uint8_t xres, uint8_t yres )
    {
        for( const auto & entry : ResEquiv )
        {
            if( entry.x == xres && entry.y == yres )
                return entry.enumres;
        }

        return eRes::_INVALID; //If things go wrong, return the biggest
    }

    void MetaFrame::remRef( uint32_t refseq, uint32_t refererindex ) 
    {
        for( auto it = m_animFrmsRefer.begin(); it != m_animFrmsRefer.end(); ++it )
        {
            if( it->refseq == refseq && it->reffrm == refererindex )
            {
                m_animFrmsRefer.erase(it);
                return;
            }
        }
    }


    vector<uint8_t>::const_iterator MetaFrame::ReadFromWANContainer( vector<uint8_t>::const_iterator & itread, bool & out_isLastFrm )
    {
        //Read the raw values first
        imageIndex      = utils::ReadIntFromByteVector<decltype(imageIndex)>(itread); //itread is incremented automatically!
        unk0            = utils::ReadIntFromByteVector<decltype(unk0)>(itread);
        uint16_t offyfl = utils::ReadIntFromByteVector<uint16_t>(itread);
        uint16_t offxfl = utils::ReadIntFromByteVector<uint16_t>(itread);
        unk15           = utils::ReadIntFromByteVector<decltype(unk15)>(itread);
        unk1            = utils::ReadIntFromByteVector<decltype(unk1)>(itread);

        //Set the cleaned offsets
        offsetY         = 0x03FF & offyfl; //keep the 10 lowest bits
        offsetX         = 0x01FF & offxfl; //Keep the 9  lowest bits

        //Get the resolution
        resolution      = MetaFrame::GetResolutionFromOffset_uint16( offxfl, offyfl );
        
        //x offset flags
        vFlip           = utils::IsBitOn( offxfl, 13u );
        hFlip           = utils::IsBitOn( offxfl, 12u );
        out_isLastFrm   = utils::IsBitOn( offxfl, 11u ); //X bit 5, tells whether this is the last meta-f in a grp
        XOffbit6        = utils::IsBitOn( offxfl, 10u );
        XOffbit7        = utils::IsBitOn( offxfl,  9u );
        //y offset flags
        YOffbit3        = utils::IsBitOn( offyfl, 13u );
        Mosaic          = utils::IsBitOn( offyfl, 12u );
        YOffbit5        = utils::IsBitOn( offyfl, 11u );
        YOffbit6        = utils::IsBitOn( offyfl, 10u );

        return itread;
    }

    void MetaFrame::WriteToWANContainer( back_insert_iterator<vector<uint8_t>> itbackins, bool setLastBit )const //setLastBit set this to true for the last frame in a group !
    {
        utils::WriteIntToByteVector( imageIndex, itbackins );
        utils::WriteIntToByteVector( unk0,       itbackins );

        //Get the value of the resolution as a byte
        uint8_t resval    = static_cast<uint8_t>(resolution);
        uint8_t EndbitVal = ( (setLastBit)?1:0 ); //set it to one if is last!

        uint16_t YOffset = ( ( resval << 8 ) & 0xC000 ) | (YOffbit3 << 13) | ((Mosaic)?1:0) << 12 | (YOffbit5 << 11) | (YOffbit5 << 10) | offsetY;
        utils::WriteIntToByteVector( YOffset,       itbackins );

    //# Don't forget to make sure XOffset bit 5 is set to 1 for the last meta-frame in a group !
        uint16_t XOffset = ( ( resval << 12 ) & 0xC000 ) | ( ((vFlip)?1:0 ) << 13) | ( ((hFlip)?1:0 ) << 12) | ( EndbitVal << 11) | (XOffbit6 << 10) | (XOffbit7 << 9) | offsetX;
        utils::WriteIntToByteVector( XOffset,       itbackins );

        utils::WriteIntToByteVector( unk15, itbackins );
        utils::WriteIntToByteVector( unk1,  itbackins );
    }

};};