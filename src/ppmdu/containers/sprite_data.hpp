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
#include <vector>
#include <map>
#include <cstdint>

namespace pmd2{ namespace graphics
{
//=========================================================================================
//  SpriteAnimationFrame
//=========================================================================================
    /*
        MetaFrame
            Stores properties for a single frame.
    */
    struct MetaFrame
    {
        enum
        {
            eRES_8x8   = 0,
            eRES_16x16 = 0x40,
            eRES_32x32 = 0x80,
            eRES_64x64 = 0xC0,
        };


        uint32_t image_index; //The index of the actual image data in the image data vector
        uint16_t offset_y;    //The frame will be offset this much, ?in absolute coordinates?
        uint8_t  offset_x;    //The frame will be offset this much, ?in absolute coordinates?
        uint8_t  resolution;  //The resolution of the frame, which is one of the 4 in the enum above
        bool     vFlip;       //Whether the frame is flipped vertically
        bool     hFlip;       //Whether the frame is flipped horizontally
        uint8_t  unkdat;      //Extra data stored in the lower nybble of the flag byte in the sprites
        uint16_t unkdat2;     //Unknown data, stored in last 16 bits integer in the entry for a meta-frame

        std::vector<uint32_t> anim_frames_referencing; //list of the indexes of the anim frames referencing to this!


        virtual std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const;
        virtual std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom );
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
        /*
            Properties for a single frame of an animation sequence.
        */
        struct AnimFrame
        {
            uint16_t frame_duration;    //The duration this frame will be displayed 0-0xFFFF
            uint16_t meta_frame_index;  //The index of the meta-frame to be displayed
            uint16_t spr_offset_x;      //The offset from the center of the sprite the frame will be drawn at.
            uint16_t spr_offset_y;      //The offset from the center of the sprite the frame will be drawn at.
            uint16_t shadow_offset_x;   //The offset from the center of the sprite the shadow under the sprite will be drawn at.
            uint16_t shadow_offset_y;   //The offset from the center of the sprite the shadow under the sprite will be drawn at.
            unsigned int LENGTH = 12u;  //The length of the animation frame data in raw form!
        };

        AnimationSequence( uint32_t nbframes = 0 );             //Not counting the obligatory closing null frame!!

        unsigned int      getNbFrames()const;                   //Not counting the obligatory closing null frame!!
        const AnimFrame & getFrame( unsigned int index )const;
        AnimFrame       & getFrame( unsigned int index );

        uint32_t insertFrame( AnimFrame       && aframe, int index = -1 );  //Inserts a frame. If index == -1, inserts at the end! Return index of frame!
        uint32_t insertFrame( const AnimFrame  & aframe, int index = -1 );  //Inserts a frame. If index == -1, inserts at the end! Return index of frame!

        //Parse/write from/to raw data
        virtual std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const;
        virtual std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom );

    private:
        std::vector<AnimFrame> m_frames;
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
        std::string                    group_name;
        std::vector<AnimationSequence> sequences;
    };

//=========================================================================================
//  SpriteData
//=========================================================================================
    /*
    */
    template<class TIMG_Type>
        class SpriteData
    {
    public:
        typedef TIMG_Type img_t; 

        uint32_t InsertAFrame( const img_t & img )
        {
        }

        uint32_t InsertAFrame( img_t && img )
        {
        }

        void     RemoveAFrame( uint32_t index ) //Remove a frame and all its references!
        {
        }

    private:

        struct framerefkeeper
        {
            framerefkeeper()
            {
                meta_frames_refering.reserve(4); //reserve 4, to avoid a bunch of re-alloc
            }

            std::vector<uin32_t> meta_frames_refering;  //Contains the indexes of the meta-frames refering to this
            img_t                imagedata;
        };

    private:
        std::vector<framerefkeeper>                m_frames;      //Actual image data, with reference list.
        std::vector<MetaFrame>                     m_metaframes;  //List of frames usable in anim sequences
        std::vector<SpriteAnimationGroup>          m_animgroups;  //Groups containing animation sequences
        std::vector<std::vector<gimg::colorRGB24>> m_palettes;    //The palettes for this sprite

        // Sprite Properties
        bool m_is8WaySprite; //Whether the sprite uses 8 way animations. For character sprites!
    };
};};

#endif