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
};};