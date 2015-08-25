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
#include <ext_fmts/supported_io.hpp>
#include <vector>
#include <map>
#include <cstdint>
#include <utility>
#include <algorithm>
#include <atomic>
#include <cstdint>

//Forward declare for the friendly io modules !
namespace pmd2{ namespace filetypes{ class WAN_Parser; class WAN_Writer; }; };

namespace pmd2{ namespace graphics
{
//=========================================================================================
//=========================================================================================
    //What particular format the sprite is in!
    enum struct eSpriteImgType : short
    {
        sprInvalid,
        spr4bpp,
        spr8bpp,
    };

    /*
        The sprite type value at the end of the sprite file.
    */
    enum struct eSprTy : uint16_t
    {
        Generic   = 0,
        Character = 1,
        Unknown   = 2,
        WAT_fmt   = 3,
    };

//=========================================================================================
//  Utility
//=========================================================================================

//=========================================================================================
//  sprOffParticle
//=========================================================================================
    /*
        sprOffParticle
            Stores the value of an offset in the offset table.
    */
    struct sprOffParticle
    {
        int16_t offx = 0;
        int16_t offy = 0;
    };

//=========================================================================================
//  Image Info
//=========================================================================================
    /*
        Contains data specific to the images 
    */
    struct ImageInfo
    {
        uint32_t zindex = 0;
    };

//=========================================================================================
//  MetaFrame
//=========================================================================================
    /*
        MetaFrame
            Stores properties for a single frame.
    */
    class MetaFrame
    {
    public:
        /*
            This table contains all the possible resolution for an image pointed to by a meta-frame.
            The value each entry contains is the value of the 2 first bits of the x and y offsets that needs
            to be set in the wan file to idicate that resolution.
            The Y offset's 2 highest bits are stored in the high nybble, while X offset's are stored in the low nybble!
        */
        enum struct eRes : uint8_t
        {
            //First nybble is highest 2 bits value of YOffset, and lowest nybble highest 2 bits of XOffset.
            // 
            //                 YOffset :             XOffset :
            _8x8   = 0,     // 00xx xxxx xxxx xxxx - 00xx xxxx xxxx xxxx
            _16x16 = 0x04,  // 00xx xxxx xxxx xxxx - 01xx xxxx xxxx xxxx
            _32x32 = 0x08,  // 00xx xxxx xxxx xxxx - 10xx xxxx xxxx xxxx
            _64x64 = 0x0C,  // 00xx xxxx xxxx xxxx - 11xx xxxx xxxx xxxx

            _16x8  = 0x40,  // 01xx xxxx xxxx xxxx - 00xx xxxx xxxx xxxx
            _8x16  = 0x80,  // 10xx xxxx xxxx xxxx - 00xx xxxx xxxx xxxx

            _32x8  = 0x44,  // 01xx xxxx xxxx xxxx - 01xx xxxx xxxx xxxx
            _8x32  = 0x84,  // 10xx xxxx xxxx xxxx - 01xx xxxx xxxx xxxx
            
            _32x16 = 0x48,  // 01xx xxxx xxxx xxxx - 10xx xxxx xxxx xxxx
            _16x32 = 0x88,  // 10xx xxxx xxxx xxxx - 10xx xxxx xxxx xxxx

            _64x32 = 0x4C,  // 01xx xxxx xxxx xxxx - 11xx xxxx xxxx xxxx
            _32x64 = 0x8C,  // 10xx xxxx xxxx xxxx - 11xx xxxx xxxx xxxx

            _INVALID,       //Returned in cases of errors
        };

        //Used for storing what animation frames refers to this meta-frame!
        struct animrefs_t
        {
            uint32_t refseq;
            uint32_t reffrm;
        };

        /*
            Struct used to associate the value in the eRes enum to numerical representation
            of the resolution they represent!
        */
        struct integerAndRes
        {
            eRes    enumres;
            uint8_t x, 
                    y; 
        };

    //-----------------------------
    // Static Methods
    //-----------------------------
        //Pass the raw offsets with their first 2 bits containing the img resolution
        // and get the resolution as a eRes !
        static inline eRes GetResolutionFromOffset_uint16( uint16_t xoffset, uint16_t yoffset )
        {
            //Isolate the first 2 bits and shift them into a single byte, each in their own nybble
            return static_cast<MetaFrame::eRes>( (0xC000 & yoffset) >> 8 | (0xC000 & xoffset) >> 12 );
        }

        static utils::Resolution eResToResolution( eRes ares );

        /*
            Return the correct eRes entry to match the integer representation of the entry.
        */
        static eRes IntegerResTo_eRes( uint8_t xres, uint8_t yres );
        
    //-----------------------------
    // Methods
    //-----------------------------
        // --- Construct ---
        MetaFrame()
        {
            m_animFrmsRefer.reserve(1); //All meta-frames will be refered to at least once !
            imageIndex = SPECIAL_METAFRM_INDEX;
            unk0       = 0;
            offsetY    = 0;
            offsetX    = 0;
            unk15      = 0;
            unk1       = 0;
        }

        //--- References handling ---
        inline unsigned int       getNbRefs()const                  { return m_animFrmsRefer.size(); }
        inline const animrefs_t & getRef( unsigned int index )const { return m_animFrmsRefer[index]; }

        //Add/remove to our list of anim frames that refer to us
        inline void  addRef( uint32_t refseq, uint32_t refererindex ) { m_animFrmsRefer.push_back( animrefs_t{refseq,refererindex }); }
        void         remRef( uint32_t refseq, uint32_t refererindex );

        //Empty the reference table completely
        inline void clearRefs() { m_animFrmsRefer.resize(0); }

        /*
            This returns whether the meta-frame has -1 as image index!
        */
        inline bool HasSpecialImageIndex()const { return imageIndex == SPECIAL_METAFRM_INDEX; }
        inline bool HasValidImageIndex  ()const { return imageIndex >= 0; }

    //-----------------------------
    // IO Methods
    //-----------------------------
        //#TODO: we'll need to make a proper generic sprite data class that isn't heavily based on the structure of wan files...
        std::vector<uint8_t>::const_iterator ReadFromWANContainer( std::vector<uint8_t>::const_iterator & itread, bool & out_isLastFrm );
        void WriteToWANContainer( std::back_insert_iterator<std::vector<uint8_t>> itbackins, bool setLastBit )const; //setLastBit set this to true for the last frame in a group !

    //-----------------------------
    // Variables
    //-----------------------------
        //Cleaned up values from file( the Interpreted values below, were removed from those ):
        int16_t  imageIndex;     //The index of the actual image data in the image data vector
        uint16_t unk0;          //?
        int16_t  offsetY;       //The frame will be offset this much, ?in absolute coordinates?
        int16_t  offsetX;       //The frame will be offset this much, ?in absolute coordinates?
        uint8_t  unk15;         //Possibly a secondary reference to an image...
        uint8_t  unk1;          //More often than not is 0xC

        //Interpreted values:
        //#NOTE: lots of boolean for now, will eventually change, so don't refer to the unamed ones as much as possible !
        eRes     resolution;     //The resolution of the frame, which is one of the 4 in the enum above
        bool     vFlip;          //Whether the frame is flipped vertically
        bool     hFlip;          //Whether the frame is flipped horizontally
        bool     Mosaic;         //Whether the frame triggers mosaic mode.
        bool     XOffbit6;       //The state of bit 6 of the bits assigned to flags in XOffset (0000 0100 0000 0000)
        bool     XOffbit7;       //The state of bit 7 of the bits assigned to flags in XOffset (0000 0010 0000 0000)
        bool     YOffbit3;       //The state of bit 3 of the bits assigned to flags in YOffset (0010 0000 0000 0000)
        bool     YOffbit5;       //The state of bit 5 of the bits assigned to flags in YOffset (0000 1000 0000 0000)
        bool     YOffbit6;       //The state of bit 6 of the bits assigned to flags in YOffset (0000 0100 0000 0000)

        /*
            Table containing the resolution in numerical form for each entries in the eRes enum !
        */
        static const std::vector<integerAndRes> ResEquiv;
        static const int16_t                    SPECIAL_METAFRM_INDEX = -1;// 0xFFFF;

    private:
        std::vector<animrefs_t> m_animFrmsRefer; //list of the indexes of the anim frames referencing to this!
    };


//=========================================================================================
//  Meta-Frame Group
//=========================================================================================
    /*
        Contains a list of meta-frames indices to assemble together when loaded into the
        NDS memory.
        Implemented as a struct for future expansion on the concept of a group of meta-frame.
    */
    struct MetaFrameGroup
    {
        //Forward some basic vector stuff
        inline std::size_t       & operator[]( unsigned int index )      { return metaframes[index]; }
        inline const std::size_t & operator[]( unsigned int index )const { return metaframes[index]; }
        inline std::size_t       size      ()const                       { return metaframes.size(); }

        inline std::vector<std::size_t>::iterator       begin()     { return metaframes.begin(); }
        inline std::vector<std::size_t>::const_iterator begin()const{ return metaframes.begin(); }
        inline std::vector<std::size_t>::iterator       end()       { return metaframes.end(); }
        inline std::vector<std::size_t>::const_iterator end()const  { return metaframes.end(); }

        std::vector<std::size_t> metaframes;
    };


//=========================================================================================
//  AnimFrame
//=========================================================================================
    /*
        Properties for a single frame of an animation sequence.
    */
    struct AnimFrame
    {
        uint16_t frameDuration    = 0; //The duration this frame will be displayed 0-0xFFFF
        uint16_t metaFrmGrpIndex  = 0; //The index of the meta-frame group to be displayed
        int16_t  sprOffsetX       = 0; //The offset from the center of the sprite the frame will be drawn at.
        int16_t  sprOffsetY       = 0; //The offset from the center of the sprite the frame will be drawn at.
        int16_t  shadowOffsetX    = 0; //The offset from the center of the sprite the shadow under the sprite will be drawn at.
        int16_t  shadowOffsetY    = 0; //The offset from the center of the sprite the shadow under the sprite will be drawn at.

        inline bool isNull()const
        {
            return frameDuration == 0  && metaFrmGrpIndex == 0 && sprOffsetX == 0 && sprOffsetY == 0 &&
                   shadowOffsetX == 0 && shadowOffsetY == 0;
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

        inline void                reserve( unsigned int count )       { m_frames.reserve(count); }
        inline unsigned int        getNbFrames()const                  { return m_frames.size(); } //Not counting the obligatory closing null frame!!
        inline const AnimFrame   & getFrame( unsigned int index )const { return m_frames[index]; }
        inline AnimFrame         & getFrame( unsigned int index )      { return m_frames[index]; }

        //Inserts a frame. If index == -1, inserts at the end! Return index of frame!
        inline uint32_t            insertFrame( const AnimFrame & aframe, int index = -1 ) {  return insertFrame( AnimFrame( aframe ), index ); }
        inline void                removeframe( unsigned int index )                       { m_frames.erase( m_frames.begin() + index ); }
        inline const std::string & getName    ()const                                      { return m_name; }
        inline void                setName    (const std::string & name)                   { m_name = name; }


        uint32_t insertFrame( AnimFrame && aframe, int index = -1 ) //Inserts a frame. If index == -1, inserts at the end! Return index of frame!
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
        std::string           group_name;  //A human readable name for the group. Not saved when writing a file!
        std::vector<uint32_t> seqsIndexes; //Indexes to the animation sequences in the anim sequence table that belong to this group.
                                           //If no sequences are in there, the group is empty !
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
        uint16_t Unk3           = 0;
        uint16_t nbColorsPerRow = 0; //0 to 16.. Changes the amount of colors loaded on a single row in the runtime palette sheet.
        uint16_t Unk4           = 0;
        uint16_t Unk5           = 0;

        // Sprite Anim Properties:
        uint16_t Unk6           = 0;
        uint16_t Unk7           = 0;
        uint16_t Unk8           = 0;
        uint16_t Unk9           = 0;
        uint16_t Unk10          = 0;

        // Sprite Properties:
        eSprTy   spriteType     = eSprTy::Generic;
        uint16_t is256Sprite  = 0; //If 1, sprite is 256 colors, if 0 is 16 colors.
        uint16_t Unk13          = 0; //If 1, load the first row of tiles of each images one after the other, the the second, and so on. Seems to be for very large animated sprites!
        uint16_t Unk11          = 0; //Unknown value.
        uint16_t Unk12          = 0; //Unknown value at the end of the file!

        //Constants
        static const std::string DESC_Unk3;
        static const std::string DESC_nbColorsPerRow;
        static const std::string DESC_Unk4;
        static const std::string DESC_Unk5;

        static const std::string DESC_Unk6;
        static const std::string DESC_Unk7;
        static const std::string DESC_Unk8;
        static const std::string DESC_Unk9;
        static const std::string DESC_Unk10;

        static const std::string DESC_spriteType;
        static const std::string DESC_Is256Sprite;
        static const std::string DESC_Unk13;
        static const std::string DESC_Unk11;
        static const std::string DESC_Unk12;
    };

//=========================================================================================
//  SpriteData
//=========================================================================================

    /***************************************************************************************
        BaseSprite
            An interface to bypass having to deal with template when using external objects
            for IO such as the sprite parser/writer, or the wan parser/writer.
            Mainly because its impossible to know at compilation what image format the
            sprite the user will give us is in!
    ***************************************************************************************/
    class BaseSprite
    {
    public:
        virtual ~BaseSprite() {}

        virtual const std::vector<sprOffParticle>      & getPartOffsets ()const=0;
        virtual std::vector<sprOffParticle>            & getPartOffsets ()=0;
        virtual const std::multimap<uint32_t,uint32_t> & getMetaRefs    ()const=0;
        //No write access to metarefs !!
        virtual const std::vector<MetaFrame>           & getMetaFrames  ()const=0;
        virtual std::vector<MetaFrame>                 & getMetaFrames  ()=0;
        virtual const std::vector<MetaFrameGroup>      & getMetaFrmsGrps()const=0;
        virtual std::vector<MetaFrameGroup>            & getMetaFrmsGrps()=0;

        virtual const std::vector<SpriteAnimationGroup>& getAnimGroups  ()const=0;
        virtual std::vector<SpriteAnimationGroup>      & getAnimGroups  ()=0;

        virtual const std::vector<AnimationSequence>   & getAnimSequences()const=0;
        virtual std::vector<AnimationSequence>         & getAnimSequences()=0;

        virtual const std::vector<gimg::colorRGB24>    & getPalette     ()const=0;
        virtual std::vector<gimg::colorRGB24>          & getPalette     ()=0;
        virtual const SprInfo                          & getSprInfo     ()const=0;
        virtual SprInfo                                & getSprInfo     ()=0;

        virtual const std::vector<ImageInfo>           & getImgsInfo    ()const=0;
        virtual std::vector<ImageInfo>                 & getImgsInfo    ()=0;

        //Get the data format of the sprite
        virtual const eSpriteImgType                   & getSpriteType  ()const=0;
        virtual std::size_t                              getNbFrames    ()const=0;

        //Access to sprite image data via pointer. If you use the wrong version, will return nullptr!
        // use getSpriteType() to get the correct type.
        virtual std::vector<gimg::tiled_image_i8bpp> * getFramesAs8bpp() { return nullptr; }
        virtual std::vector<gimg::tiled_image_i4bpp> * getFramesAs4bpp() { return nullptr; }
    };


    /***************************************************************************************
        SpriteData
            
    ***************************************************************************************/
    template<class TIMG_Type>
        class SpriteData : public BaseSprite
    {
        friend class pmd2::filetypes::WAN_Parser; 
        friend class pmd2::filetypes::WAN_Writer;
        static const uint32_t MY_NB_BITS_PER_PIXEL = TIMG_Type::pixel_t::mypixeltrait_t::BITS_PER_PIXEL;

        //A couple of constants and typedef to make the conditional statement below(MY_SPRITE_TYPE) more readable!
        typedef std::integral_constant<eSpriteImgType,eSpriteImgType::spr4bpp>    SPRTy_4BPP;
        typedef std::integral_constant<eSpriteImgType,eSpriteImgType::spr8bpp>    SPRTy_8BPP;
        typedef std::integral_constant<eSpriteImgType,eSpriteImgType::sprInvalid> SPRTy_INVALID; 
        static const bool IsSprite4bpp = MY_NB_BITS_PER_PIXEL == 4;
        static const bool IsSprite8bpp = MY_NB_BITS_PER_PIXEL == 8;

        //This contains the correct enum values depending on the type of images the sprite contains!
        // Supports only 4, and 8 bpp for now! (Hopefully MSVSC will implement constexpr before then...)
        static const eSpriteImgType MY_SPRITE_TYPE = 
            typename std::conditional< IsSprite4bpp, 
                                       SPRTy_4BPP,
                                       std::conditional< IsSprite8bpp, SPRTy_8BPP, SPRTy_INVALID >::type 
                                     >::type::value;

    public:
        typedef TIMG_Type img_t; 
        inline const std::vector<img_t>                & getFrames      ()const { return m_frames;        }

        virtual const std::vector<sprOffParticle>      & getPartOffsets ()const { return m_partOffsets;   } 
        virtual const std::multimap<uint32_t,uint32_t> & getMetaRefs    ()const { return m_metarefs;      }
        virtual const std::vector<MetaFrame>           & getMetaFrames  ()const { return m_metaframes;    }
        virtual const std::vector<MetaFrameGroup>      & getMetaFrmsGrps()const { return m_metafrmsgroups;}
        virtual const std::vector<SpriteAnimationGroup>& getAnimGroups  ()const { return m_animgroups;    }
        virtual const std::vector<AnimationSequence>   & getAnimSequences()const{ return m_animSequences; }
        virtual const std::vector<gimg::colorRGB24>    & getPalette     ()const { return m_palette;       }
        virtual const SprInfo                          & getSprInfo     ()const { return m_common;        }
        virtual const std::vector<ImageInfo>           & getImgsInfo    ()const { return m_imgsinfo;      }

        virtual std::vector<sprOffParticle>            & getPartOffsets ()      { return m_partOffsets;   } 
        virtual std::vector<MetaFrame>                 & getMetaFrames  ()      { return m_metaframes;    }
        virtual std::vector<MetaFrameGroup>            & getMetaFrmsGrps()      { return m_metafrmsgroups;}
        virtual std::vector<SpriteAnimationGroup>      & getAnimGroups  ()      { return m_animgroups;    }
        virtual std::vector<AnimationSequence>         & getAnimSequences()     { return m_animSequences; }
        virtual std::vector<gimg::colorRGB24>          & getPalette     ()      { return m_palette;       }
        virtual SprInfo                                & getSprInfo     ()      { return m_common;        }
        virtual std::vector<ImageInfo>                 & getImgsInfo    ()      { return m_imgsinfo;      }

        //Get the data format of the sprite
        virtual const eSpriteImgType & getSpriteType()const { return MY_SPRITE_TYPE; }
        virtual std::size_t            getNbFrames  ()const { return m_frames.size(); }


        virtual std::vector<gimg::tiled_image_i8bpp> * getFramesAs8bpp() 
        { 
            return GetMyFramePtr<SpriteData<img_t>, std::vector<gimg::tiled_image_i8bpp>>(const_cast<SpriteData<img_t>*>(this)); 
        }

        virtual std::vector<gimg::tiled_image_i4bpp> * getFramesAs4bpp() 
        { 
            return GetMyFramePtr<SpriteData<img_t>, std::vector<gimg::tiled_image_i4bpp>>(const_cast<SpriteData<img_t>*>(this)); 
        }


        /*
            Get the correct frame pointer for polymorphic methods
        */
        template<class _MyTy, class _frmTy>
            static _frmTy * GetMyFramePtr( _MyTy * ptrme )
        {
            return nullptr; //If the two types do not match the specialization below, it will return a null pointer
        }

            //Compiled only when 4bpp
            template<>
                static std::vector<gimg::tiled_image_i4bpp> * GetMyFramePtr( SpriteData<gimg::tiled_image_i4bpp> * ptrme )
            {
                return &(ptrme->m_frames);
            }

            //Compiled only when 8bpp
            template<>
                static std::vector<gimg::tiled_image_i8bpp> * GetMyFramePtr( SpriteData<gimg::tiled_image_i8bpp> * ptrme )
            {
                return &(ptrme->m_frames);
            }


        /*
            Rebuilds the entire reference maps for everything currently in the sprite!
        */
        void RebuildAllReferences()
        {
            //Clear the meta reference map
            m_metarefs.clear();

            //First set the meta-frames references
            for( unsigned int ctmf = 0; ctmf < m_metaframes.size(); ++ctmf )
                RebuildAMetaFrameRefs(ctmf);

            //Then handle animations references!
            for( unsigned int ctseq = 0; ctseq < m_animSequences.size(); ++ctseq )
            {
                unsigned int NbFrms = m_animSequences[ctseq].getNbFrames();
                //And each animation frames!
                for( unsigned int ctfrm = 0; ctfrm < NbFrms; ++ctfrm )
                    RebuildAnAnimRefs( ctseq, ctfrm );
            }
        }

        void RebuildAMetaFrameRefs( unsigned int ctmf )
        {
            auto & currentMf = m_metaframes[ctmf];

            if( currentMf.HasSpecialImageIndex() ) //Handle special frame
            {
                //#EXPERIMENTAL HANDLING
#ifdef HANDLE_MINUS_ONE_METAFRAME_TEST
                std::clog <<"Meta-Frame #" <<ctmf <<" has image index of -1! Using unk15 as image index value for reference (" 
                          <<static_cast<unsigned int>(currentMf.unk15) <<")\n";
                assert( currentMf.unk15 < static_cast<uint16_t>(currentMf.imageIndex) );

                //clear all anim references!
                currentMf.clearRefs();
                //Register meta to frame reference 
                m_metarefs.insert( std::make_pair( currentMf.unk15, ctmf ) );
#else
                std::stringstream strs;
                strs <<"Meta-Frame #" <<ctmf <<" has image index of -1! -1 frame handling is off, IGNORING!\n   Unk15 value is : " 
                     <<static_cast<unsigned int>(currentMf.unk15) <<"\n";
                std::string errmess = strs.str();
                std::clog << errmess;
#endif
            }
            else if(currentMf.imageIndex < 0 ) //Handle invalid frames
            {
                std::stringstream strs;
                strs <<"Error while rebuilding meta-frames to images references. Meta-frame #" <<ctmf 
                     <<" refers to a negative image index that isn't -1 !";
                std::string errmess = strs.str();
                std::clog << errmess;
                throw std::runtime_error(errmess);
            }
            else if( static_cast<uint16_t>(currentMf.imageIndex) > m_frames.size() ) //safe to cast here now, and get rid of warnings
            {
                std::stringstream strs;
                strs <<"Error while rebuilding meta-frames to images references. Meta-frame #" <<ctmf 
                     <<" refers to an image index that is out of range ! (" <<static_cast<uint16_t>(currentMf.imageIndex) <<")";
                std::string errmess = strs.str();
                std::clog << errmess;
                throw std::out_of_range(errmess);
            }
            else //If we end up here we're good
            {
                //clear all anim references!
                currentMf.clearRefs();

                //Register meta to frame reference 
                m_metarefs.insert( std::make_pair( currentMf.imageIndex, ctmf ) );
            }
        }

        /*
            Check if the current meta-frame points to an image with a resolution that doesn't match
            and fix it.
        */
        //void CheckForMissingResolution( MetaFrame & curmf, unsigned int ctmf )
        //{
        //    utils::Resolution mfres = MetaFrame::eResToResolution(curmf.resolution);
        //    auto            & img   = m_frames.at(curmf.imageIndex);

        //    if( curmf.resolution == MetaFrame::eRes::_INVALID || 
        //        curmf.resolution != MetaFrame::IntegerResTo_eRes( img.getNbPixelWidth(), img.getNbPixelHeight() ) )
        //    {
        //        //If we got a resolution mismatch fix it!

        //    }
        //}

        void RebuildAnAnimRefs( unsigned int ctseq, unsigned int ctfrm )
        {
            auto & curframe =  m_animSequences[ctseq].getFrame(ctfrm);
            if( curframe.metaFrmGrpIndex < m_metaframes.size() )
            {
                //Register ref into the meta-frame at the index specified!
                m_metaframes[curframe.metaFrmGrpIndex].addRef( ctseq, ctfrm );
            }
            else
            {
                //Bad meta-frame index ! Sometimes this might be acceptable..
                assert(false);
            }
        }

        SpriteData(){}
        SpriteData( SpriteData<TIMG_Type> && other )
        {
            m_frames         = std::move( other.m_frames         ); 
            m_metarefs       = std::move( other.m_metarefs       );
            m_metaframes     = std::move( other.m_metaframes     );
            m_metafrmsgroups = std::move( other.m_metafrmsgroups );
            m_animgroups     = std::move( other.m_animgroups     );
            m_animSequences  = std::move( other.m_animSequences  );
            m_palette        = std::move( other.m_palette        );
            m_common         = std::move( other.m_common         );
            m_partOffsets    = std::move( other.m_partOffsets    );
            m_imgsinfo       = std::move( other.m_imgsinfo       );
        }

        SpriteData<TIMG_Type> & operator=( SpriteData<TIMG_Type> && other )
        {
            m_frames         = std::move( other.m_frames         ); 
            m_metarefs       = std::move( other.m_metarefs       );
            m_metaframes     = std::move( other.m_metaframes     );
            m_metafrmsgroups = std::move( other.m_metafrmsgroups );
            m_animgroups     = std::move( other.m_animgroups     );
            m_animSequences  = std::move( other.m_animSequences  );
            m_palette        = std::move( other.m_palette        );
            m_common         = std::move( other.m_common         );
            m_partOffsets    = std::move( other.m_partOffsets    );
            m_imgsinfo       = std::move( other.m_imgsinfo       );
            return *this;
        }


    /*private: */
        std::vector<img_t>                   m_frames;      //Actual image data, with reference list.
        std::multimap<uint32_t,uint32_t>     m_metarefs;    //A map of the meta-frames refering to a specific frame. 
                                                             // The frame index in "m_frames" is the keyval, the value 
                                                             // is the index of the metaframe in "m_metaframes"
        std::vector<MetaFrame>               m_metaframes;      //Contains all the meta-frames ordered by index
        std::vector<MetaFrameGroup>          m_metafrmsgroups;  //List of meta-frames groups, for grouping those assembled together. Only index is stored!
        
        std::vector<SpriteAnimationGroup>    m_animgroups;      //Groups containing animation sequences

        std::vector<AnimationSequence>       m_animSequences;   //All animation sequences and frames

        std::vector<gimg::colorRGB24>        m_palette;         //The palette for this sprite
        SprInfo                              m_common;          //Common properties about the sprite not affected by template type!
        std::vector<sprOffParticle>          m_partOffsets;     //The particle offsets list
        std::vector<ImageInfo>               m_imgsinfo;        //Data about the actual images. Things like the Z index. 

    private:
        ////We don't wanna copy
        //SpriteData( const SpriteData<TIMG_Type> & other );
        //SpriteData<TIMG_Type> & operator=( const SpriteData<TIMG_Type> & other );
    };

};};

#endif