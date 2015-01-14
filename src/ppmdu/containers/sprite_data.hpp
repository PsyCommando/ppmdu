#ifndef SPRITE_DATA_HPP
#define SPRITE_DATA_HPP
/*
sprite_data.hpp
2015/01/03
psycommando@gmail.com
Description:
    Generic container for sprite data.
*/
#include <ppmdu/containers/tiled_image.hpp>
#include <ppmdu/pmd2/pmd2_image_formats.hpp>
#include <vector>
#include <map>
#include <cstdint>
#include <utility>
#include <algorithm>

//Forward declare for the friendly io modules !
namespace pmd2{ namespace filetypes{ class Parse_WAN; class Write_WAN; }; };

namespace pmd2{ namespace graphics
{
//=========================================================================================
//  SpriteAnimationFrame
//=========================================================================================
    /*
        MetaFrame
            Stores properties for a single frame.
    */
    class MetaFrame
    {
    public:
        //Used for storing what animation frames refers to this meta-frame!
        struct animrefs_t
        {
            uint32_t refgrp,
                     refseq,
                     reffrm;
        };

        enum struct eRes : uint8_t
        {
            //First nybble is highest 2 bits value of YOffset, and lowest nybble highest 2 bits of XOffset.
            // 
            //                 YOffset :             XOffset :
            _8x8   = 0,     // 00xx xxxx xxxx xxxx - 00xx xxxx xxxx xxxx
            _16x16 = 0x04,  // 00xx xxxx xxxx xxxx - 01xx xxxx xxxx xxxx
            _32x32 = 0x08,  // 00xx xxxx xxxx xxxx - 10xx xxxx xxxx xxxx
            _64x64 = 0x0C,  // 00xx xxxx xxxx xxxx - 11xx xxxx xxxx xxxx

            _8x16  = 0x40,  // 01xx xxxx xxxx xxxx - 00xx xxxx xxxx xxxx
            _16x8  = 0x80,  // 10xx xxxx xxxx xxxx - 00xx xxxx xxxx xxxx

            _32x8  = 0x44,  // 01xx xxxx xxxx xxxx - 01xx xxxx xxxx xxxx
            _8x32  = 0x84,  // 10xx xxxx xxxx xxxx - 01xx xxxx xxxx xxxx

            _32x16 = 0x48,  // 01xx xxxx xxxx xxxx - 10xx xxxx xxxx xxxx
            _16x32 = 0x88,  // 10xx xxxx xxxx xxxx - 10xx xxxx xxxx xxxx

            _64x32 = 0x4C,  // 01xx xxxx xxxx xxxx - 11xx xxxx xxxx xxxx
            _32x64 = 0x8C,  // 10xx xxxx xxxx xxxx - 11xx xxxx xxxx xxxx
        };
        static const unsigned int LENGTH = 10u;  //The length of the meta frame data in raw form!


        

        //Pass the raw offsets with their first 2 bits containing the img resolution
        // and get the resolution as a eRes !
        static inline eRes GetResolutionFromOffset_uint16( uint16_t xoffset, uint16_t yoffset )
        {
            //Isolate the first 2 bits and shift them into a single byte, each in their own nybble
            return static_cast<MetaFrame::eRes>( (0xC000 & yoffset) >> 8 | (0xC000 & xoffset) >> 12 );
        }

        static utils::Resolution eResToResolution( eRes ares )
        {
            switch(ares)
            {
                //square
                case eRes::_8x8:   return graphics::RES_8x8_SPRITE;
                case eRes::_16x16: return graphics::RES_16x16_SPRITE;
                case eRes::_32x32: return graphics::RES_32x32_SPRITE;
                case eRes::_64x64: return graphics::RES_64x64_SPRITE;

                //non-square
                case eRes::_8x16: return graphics::RES_8x16_SPRITE;
                case eRes::_16x8: return graphics::RES_16x8_SPRITE;
                case eRes::_32x8: return graphics::RES_32x8_SPRITE;
                case eRes::_8x32: return graphics::RES_8x32_SPRITE;
                case eRes::_32x16: return graphics::RES_32x16_SPRITE;
                case eRes::_16x32: return graphics::RES_16x32_SPRITE;
                case eRes::_64x32: return graphics::RES_64x32_SPRITE;
                case eRes::_32x64: return graphics::RES_32x64_SPRITE;
            };
            assert(false); //Asked for invalid resolution !
            return RES_INVALID;
        }

        //Cleaned up values from file( the Interpreted values below, were removed from those ):
        uint16_t image_index;    //The index of the actual image data in the image data vector
        uint16_t unk0;           //?
        uint16_t offset_y;       //The frame will be offset this much, ?in absolute coordinates?
        uint16_t offset_x;       //The frame will be offset this much, ?in absolute coordinates?
        uint16_t unk1;           //Unknown data, stored in last 16 bits integer in the entry for a meta-frame

        //Interpreted values:
        //#NOTE: lots of boolean for now, will eventually change, so don't refer to the unamed ones as much as possible !
        eRes     resolution;     //The resolution of the frame, which is one of the 4 in the enum above
        bool     vFlip;          //Whether the frame is flipped vertically
        bool     hFlip;          //Whether the frame is flipped horizontally
        bool     Mosaic;         //Whether the frame triggers mosaic mode.
        bool     XOffbit5;       //The state of bit 5 of the bits assigned to flags in XOffset (0000 1000 0000 0000)
        bool     XOffbit6;       //The state of bit 6 of the bits assigned to flags in XOffset (0000 0100 0000 0000)
        bool     XOffbit7;       //The state of bit 7 of the bits assigned to flags in XOffset (0000 0010 0000 0000)
        bool     YOffbit3;       //The state of bit 3 of the bits assigned to flags in YOffset (0010 0000 0000 0000)
        bool     YOffbit5;       //The state of bit 5 of the bits assigned to flags in YOffset (0000 1000 0000 0000)
        bool     YOffbit6;       //The state of bit 6 of the bits assigned to flags in YOffset (0000 0100 0000 0000)
        
        // --- Construct ---
        MetaFrame()
        {
            m_animFrmsRefer.reserve(1); //All meta-frames will be refered to at least once !
        }

        //--- References handling ---
        unsigned int       getNbRefs()const                  { return m_animFrmsRefer.size(); }
        const animrefs_t & getRef( unsigned int index )const { return m_animFrmsRefer[index]; }

        void         addRef   ( uint32_t refanimgrp, uint32_t refseq, uint32_t refererindex ) { m_animFrmsRefer.push_back( animrefs_t{refanimgrp, refseq,refererindex }); }
        void         remRef   ( uint32_t refanimgrp, uint32_t refseq, uint32_t refererindex ) 
        {
            for( auto it = m_animFrmsRefer.begin(); it != m_animFrmsRefer.end(); ++it )
            {
                if( it->refgrp == refanimgrp && it->refseq == refseq && it->reffrm == refererindex )
                {
                    m_animFrmsRefer.erase(it);
                    return;
                }
            }
        }

        //Empty the reference table completely
        void clearRefs() { m_animFrmsRefer.resize(0); }


    private:
        std::vector<animrefs_t> m_animFrmsRefer; //list of the indexes of the anim frames referencing to this!
    };


//=========================================================================================
//  AnimFrame
//=========================================================================================
    /*
        Properties for a single frame of an animation sequence.
    */
    struct AnimFrame
    {
        uint16_t frame_duration   = 0;    //The duration this frame will be displayed 0-0xFFFF
        uint16_t meta_frame_index = 0;  //The index of the meta-frame to be displayed
        uint16_t spr_offset_x     = 0;      //The offset from the center of the sprite the frame will be drawn at.
        uint16_t spr_offset_y     = 0;      //The offset from the center of the sprite the frame will be drawn at.
        uint16_t shadow_offset_x  = 0;   //The offset from the center of the sprite the shadow under the sprite will be drawn at.
        uint16_t shadow_offset_y  = 0;   //The offset from the center of the sprite the shadow under the sprite will be drawn at.
        static const unsigned int LENGTH = 12u;  //The length of the animation frame data in raw form!

        inline bool isNull()const
        {
            return frame_duration == 0  && meta_frame_index == 0 && spr_offset_x == 0 && spr_offset_y == 0 &&
                   shadow_offset_x == 0 && shadow_offset_y == 0;
        }
    };


//=========================================================================================
//  AnimationSequence
//=========================================================================================
    /*
        DBH and DBI combined
    */
    class AnimationSequence
    {
    public:
        AnimationSequence( const std::string & name = "", uint32_t nbframes = 0 ) //Not counting the obligatory closing null frame!!
            :m_frames(nbframes), m_name(name)
        {}
        
        void              reserve( unsigned int count ) { m_frames.reserve(count); }

        unsigned int      getNbFrames()const                  { return m_frames.size(); } //Not counting the obligatory closing null frame!!
        const AnimFrame & getFrame( unsigned int index )const { return m_frames[index]; }
        AnimFrame       & getFrame( unsigned int index )      { return m_frames[index]; }

        uint32_t insertFrame( AnimFrame && aframe, int index = -1 )  //Inserts a frame. If index == -1, inserts at the end! Return index of frame!
        {
            uint32_t insertpos = 0;

            if( index != -1 )
            {
                m_frames.insert( m_frames.begin() + index, aframe );
                insertpos = index;
            }
            else
            {
                m_frames.push_back(aframe);
                insertpos = (m_frames.size() - 1u);
            }
            return insertpos;
        }

        uint32_t insertFrame( const AnimFrame & aframe, int index = -1 )  //Inserts a frame. If index == -1, inserts at the end! Return index of frame!
        { 
            return insertFrame( AnimFrame( aframe ), index ); 
        }

        void removeframe( unsigned int index )
        {
            m_frames.erase( m_frames.begin() + index );
        }

        const std::string & getName()const     { return m_name; }
        void setName(const std::string & name) { m_name = name; }

    private:
        std::vector<AnimFrame> m_frames;
        std::string            m_name;      //A temporary human friendly name given to this sequence!
    };

//=========================================================================================
//  SpriteAnimationGroup
//=========================================================================================
    /*
        SpriteAnimationGroup
            Stores animation sequences under a same group.
            Basically DBG.
    */
    struct SpriteAnimationGroup
    {
        std::string                    group_name; //A human readable name for the group. Not saved when writing a file!
        std::vector<AnimationSequence> sequences;
    };

//=========================================================================================
//  Sprite info
//=========================================================================================

    /*
        Common data contained in a sprite. 
        Put in its own struct to avoid having to care about the 
        Sprite's templates arguments when dealing with common data.
    */
    struct SprInfo
    {
        // Sprite Color / Image Format Properties:
        uint16_t m_Unk3           = 0;
        uint16_t m_nbColorsPerRow = 0; //0 to 16.. Changes the amount of colors loaded on a single row in the runtime palette sheet.
        uint16_t m_Unk4           = 0;
        uint16_t m_Unk5           = 0;

        // Sprite Anim Properties:
        uint16_t m_Unk6           = 0;
        uint16_t m_Unk7           = 0;
        uint16_t m_Unk8           = 0;
        uint16_t m_Unk9           = 0;
        uint16_t m_Unk10          = 0;

        // Sprite Properties:
        uint16_t m_is8WaySprite   = 0; //Whether the sprite uses 8 way animations. For character sprites!
        uint16_t m_is256Sprite    = 0; //If 1, sprite is 256 colors, if 0 is 16 colors.
        uint16_t m_IsMosaicSpr    = 0; //If 1, load the first row of tiles of each images one after the other, the the second, and so on. Seems to be for very large animated sprites!
        uint16_t m_Unk11          = 0; //Unknown value.
        uint16_t m_Unk12          = 0; //Unknown value at the end of the file!
    };

//=========================================================================================
//  SpriteData
//=========================================================================================

    /*
    */
    template<class TIMG_Type>
        class SpriteData
    {
        friend class pmd2::filetypes::Parse_WAN; 
        friend class pmd2::filetypes::Write_WAN;
    public:
        typedef TIMG_Type img_t; 

        //uint32_t InsertAFrame( const img_t & img )
        //{
        //}

        //uint32_t InsertAFrame( img_t && img )
        //{
        //}

        //void     RemoveAFrame( uint32_t index ) //Remove a frame and all its references!
        //{
        //}

    private:

        //struct framerefkeeper
        //{
        //    framerefkeeper()
        //    {
        //        meta_frames_refering.reserve(4); //reserve 4, to avoid a bunch of re-alloc
        //    }
        //    std::vector<uin32_t> meta_frames_refering;  //Contains the indexes of the meta-frames refering to this
        //    img_t                imagedata;
        //};

        /*
            InsertMetaFrame
        */
        unsigned int InsertMetaFrame( MetaFrame && mfrm )
        {
            uint32_t imgindex = mfrm.image_index;
            m_metaframes.push_back(mfrm);

            //Insert reference
            m_metarefs.insert( std::make_pair<uint32_t,uint32_t>( imgindex, (m_metaframes.size() - 1) ) );

            return (m_metaframes.size() - 1);
        }

        /*
            ReplaceMetaFrame
        */
        void ReplaceMetaFrame( uint32_t indextoreplace, MetaFrame && newmfrm )
        {
            //Validate the reference to the existing image
            uint32_t oldreferingto = m_metaframes[indextoreplace].image_index;

            if( oldreferingto == newmfrm.image_index )
            {
                //If the original and new frame refer to the same image
                m_metaframes[indextoreplace] = newmfrm;
            }
            else
            {
                //If the original and new frame refer to different images

                if( newmfrm.image_index >= m_metaframes.size() )
                    throw std::out_of_range( "ERROR: Trying to insert meta-frame pointing to a non-existing image!" );

                //Flush old references, and add a new one to the correct frame
                auto itfoundold = m_metarefs.find( oldreferingto );

                if( itfoundold != m_metarefs.end() )
                    m_metarefs.erase(itfoundold);
                else
                {
                    //std::cerr << "\nTrying to replace meta-frame with broken dependency!\n"; 
                    assert(false);//This should never happen!
                }

                m_metaframes[indextoreplace] = newmfrm;
                m_metarefs.insert( std::make_pair( m_metaframes[indextoreplace].image_index ,indextoreplace ) );
            }
        }

        /*
            RemoveMetaFrame
        */
        void RemoveMetaFrame( uint32_t index )
        {
            //Remove all anim frames that depend on it!
            const unsigned int NB_Refs  = m_metaframes[index].getNbRefs();
            auto             & metafref = m_metaframes[index];

            for( unsigned int i = 0; i < NB_Refs; ++i )
            {
                //Remove anim frame that refers to this meta-frame!
                const auto & myref = metafref.getRef(i);
                m_animgroups[myref.refgrp].sequences[myref.refseq].removeframe( myref.reffrm );
            }

            //Unregister our meta-frame ref to our image!
            auto itfoundold = m_metarefs.find(m_metaframes[index].image_index);
            if( itfoundold != m_metarefs.end() )
                m_metarefs.erase(itfoundold);
        }

        /*
            Rebuilds the entire reference maps for everything currently in the sprite!
        */
        void RebuildAllReferences()
        {
            //Clear the meta reference map
            m_metarefs.clear();

            //First set the meta-frames references
            for( unsigned int i = 0; i < m_metaframes.size(); ++i )
            {
                if( m_metaframes[i].image_index < m_frames.size() )
                {
                    //clear all anim references!
                    m_metaframes[i].clearRefs();
                    //Register meta to frame reference 
                    m_metarefs.insert( make_pair( m_metaframes[i].image_index, i ) );
                }
                else
                {
                    //Bad image index ! Sometimes this is acceptable, in the case of 0xFFFF !
                    if( m_metaframes[i].image_index != 0xFFFF )
                        assert(false);
                }
            }

            //Then handle animations references!

            //Iterate each animation groups
            for( unsigned int cpgrp = 0; cpgrp < m_animgroups.size(); ++cpgrp )
            {
                auto        &CurSeqs = m_animgroups[cpgrp].sequences;
                unsigned int NbSeq   = CurSeqs.size();

                //Each Animation Sequences
                for( unsigned int cpseq = 0; cpseq < NbSeq; ++cpseq )
                {
                    auto &CurSequence = CurSeqs[cpseq];

                    //And each animation frames!
                    for( unsigned int cpafrm = 0; cpafrm < CurSequence.getNbFrames(); ++cpafrm )
                    {
                        auto & curframe = CurSequence.getFrame(cpafrm);

                        if( curframe.meta_frame_index < m_metaframes.size() )
                        {
                            //Register ref into the meta-frame at the index specified!
                            m_metaframes[curframe.meta_frame_index].addRef( cpgrp, cpseq, cpafrm );
                        }
                        else
                        {
                            //Bad meta-frame index ! Sometimes this is acceptable, in the case of 0xFF !
                            assert(false);
                        }
                    }
                }
            }
        }

    private: 
        std::vector<img_t>                m_frames;      //Actual image data, with reference list.
        std::multimap<uint32_t,uint32_t>  m_metarefs;    //A map of the meta-frames refering to a specific frame. 
                                                         // The frame index in "m_frames" is the keyval, the value 
                                                         // is the index of the metaframe in "m_metaframes"
        std::vector<MetaFrame>            m_metaframes;  //List of frames usable in anim sequences
        std::vector<SpriteAnimationGroup> m_animgroups;  //Groups containing animation sequences
        std::vector<gimg::colorRGB24>     m_palette;     //The palette for this sprite
        SprInfo                           m_common;      //Common properties about the sprite not affected by template type!
    };
};};

#endif