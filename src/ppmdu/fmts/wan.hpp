#ifndef WAN_HPP
#define WAN_HPP
/*
wan.hpp
2014/12/31
psycommando@gmail.com
Description: Utilities for reading ".wan" sprite files, and its derivatives.
*/
#include <ppmdu/fmts/sir0.hpp>
#include <ppmdu/utils/utility.hpp>
#include <ppmdu/containers/sprite_data.hpp>
#include <ppmdu/containers/color.hpp>
#include <atomic>
#include <algorithm>

namespace pmd2 { namespace filetypes 
{

    typedef std::vector<std::vector<std::string>> animnamelst_t;

//=============================================================================================
//  WAN Structures
//=============================================================================================

    struct zerostrip_table_entry
    {
        static const unsigned int LENGTH = 12u;
        uint32_t pixelsrc,  //The source of the pixels to use to rebuild the image. Either an address, or 0
                 pixamt,    //The amount of pixels to copy / insert
                 unknown;   //Not sure what this does..

        unsigned int    size()const   { return LENGTH; }
        bool            isNull()const { return (!pixelsrc && !pixamt && !unknown); } //Whether its a null entry or not 

        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToByteVector( pixelsrc, itwriteto );
            itwriteto = utils::WriteIntToByteVector( pixamt,   itwriteto );
            itwriteto = utils::WriteIntToByteVector( unknown,  itwriteto );
            return itwriteto;
        }
        
        template<class _init>
            _init ReadFromContainer( _init itReadfrom )
        {
            pixelsrc = utils::ReadIntFromByteVector<decltype(pixelsrc)>(itReadfrom);
            pixamt   = utils::ReadIntFromByteVector<decltype(pixamt)>  (itReadfrom);
            unknown  = utils::ReadIntFromByteVector<decltype(unknown)> (itReadfrom);
            return itReadfrom;
        }
    };


    /**********************************************************************
        The 12 bytes sub-header that is linked to by an SIR0 header.
        Contains pointers to important sprite information.
    **********************************************************************/
    struct wan_sub_header : public utils::data_array_struct
    {
        uint32_t ptr_animinfo,
                 ptr_imginfo;
        uint16_t is8DirectionSprite,
                 unk12;

        static const unsigned int DATA_LEN = 12u;

        unsigned int size()const{return DATA_LEN;}
        std::string toString()const;

        std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const;
        std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom );
    };

    /**********************************************************************
        The part of the header containing the pointers to the actual frames 
        of the sprite, and the palette.
    **********************************************************************/
    struct wan_img_data_info : public utils::data_array_struct
    {
        static const unsigned int DATA_LEN = 16u;

        uint32_t ptr_img_table,             // Pointer to the the table of pointer to the individual images
                 ptr_palette;               // Pointer to the pointer to the palette info
        uint16_t isMosaic,                  // 1 == mosaic sprite,   0 == non-mosaic sprite
                 is256Colors,               // 1 == 8bpp 256 colors, 0 == 4bpp 16 colors
                 unk11,                     // Unknown, seems to range between 0 and 1..
                 nb_ptrs_frm_ptrs_table;    // Number of entries in the table of pointers to each frames.

        unsigned int size()const{return DATA_LEN;}
        std::string toString()const;

        std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const;
        std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom );
    };

    /**********************************************************************
        The part of the header containing the pointers to unidentified kind 
        of info for the sprite
    **********************************************************************/
    struct wan_anim_info : public utils::data_array_struct
    {
        static const unsigned int DATA_LEN = 24u;

        uint32_t ptr_metaFrmTable,  //pointer to the table containing pointers to every meta frames. 
                 ptr_pOffsetsTable, //ptr to Particle offset table
                 ptr_animGrpTable;  //ptr to the table with pointers to every animation groups
        uint16_t nb_anim_groups,
                 unk6,
                 unk7,
                 unk8,
                 unk9,
                 unk10;

        unsigned int size()const{return DATA_LEN;}
        std::string toString()const;

        std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const;
        std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom );
    };

    /**********************************************************************
        wan_pal_info
    **********************************************************************/
    struct wan_pal_info : public utils::data_array_struct
    {
        static const unsigned int DATA_LEN = 16u;

        uint32_t ptrpal;
        uint16_t unk3,
                 nbcolorsperrow,
                 unk4,
                 unk5;
        uint32_t nullbytes;

        unsigned int size()const{return DATA_LEN;}
        std::string toString()const;

        std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const;
        std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom );
    };

    /*
    */
    struct wan_comp_img
    {
        std::vector<uint8_t> DecompImage();

        template<class _init>
            void CompressImage( _init itbeg, _init itend )
        {
            assert(false);
            //#TODO: FINISH THIS!
        }

        std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const;
        std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom );

        std::vector<uint8_t> compressedimg;
    };

//=============================================================================================
//  Functions / Functors
//=============================================================================================

    /*
        A function to read the compression table for a compressed frame in a WAN sprite, and
        return the decompressed image via move constructor!

        - itcomptblbeg : The iterator to beginning of the specific compression table to decode!
        - filebeg      : An iterator to the beginning of the file, or to where the offsets in the table are relative to!
        - imgres       : The resolution of the image to decode !
    */
    template<class _TIMG_t, class _randit>
        _TIMG_t ParseZeroStrippedTImg( _randit itcomptblbeg, _randit filebeg, utils::Resolution imgres )
    {
        using namespace std;
        using namespace utils;

        _TIMG_t                 myimg( imgres.width, imgres.height );
        auto                    itinsertat = myimg.begin();
        zerostrip_table_entry   entry;

        do
        {
            itcomptblbeg = entry.ReadFromContainer(itcomptblbeg);

            if( !(entry.isNull()) && itinsertat != myimg.end() )
            {
                //Exec the entry!
                if( entry.pixelsrc == 0  )
                {
                    std::fill_n( itinsertat, entry.pixamt, 0 );
                }
                else
                {
                    std::copy_n( filebeg + entry.pixelsrc, entry.pixamt, itinsertat );
                }
            }

        }while( !(entry.isNull()) && itinsertat != myimg.end() );

        return std::move( myimg );
    }

    /*
        A class to parse a WAN sprite into a SpriteData structure.
        Because image formats can vary between sprites, 
    */
    class Parse_WAN
    {
    public:

        class Ex_Parsing8bppAs4bpp : public std::runtime_error { public: Ex_Parsing8bppAs4bpp():std::runtime_error("ERROR: Tried to parse 4bpp sprite as 8bpp!"){} };
        class Ex_Parsing4bppAs8bpp : public std::runtime_error { public: Ex_Parsing4bppAs8bpp():std::runtime_error("ERROR: Tried to parse 8bpp sprite as 4bpp!"){} };

        //What particular format the sprite is in!
        enum struct eSpriteType : short
        {
            sprInvalid,
            spr4bpp,
            spr8bpp,
        };

        //Constructor. Pass the data to parse.
        Parse_WAN( std::vector<uint8_t>       && rawdata, const animnamelst_t * animnames = nullptr );
        Parse_WAN( const std::vector<uint8_t> &  rawdata, const animnamelst_t * animnames = nullptr );

        //Use this to determine which parsing method to use!
        eSpriteType getSpriteType()const;

        template<class TIMG_t>
            graphics::SpriteData<TIMG_t> Parse( std::atomic<uint32_t> * pProgress = nullptr)
        {
            SpriteData<TIMG_t> sprite;
        
            //Parse the common stuff first!
            DoParse( sprite.m_palette, sprite.m_common, sprite.m_metaframes, sprite.m_animgroups );

            if( m_wanImgDataInfo.is256Colors == 1 )
            {
                if( TIMG_t::pixel_t::GetBitsPerPixel() != 8 )
                    throw Ex_Parsing8bppAs4bpp();
            }
            else
            {
                if( TIMG_t::pixel_t::GetBitsPerPixel() != 4 )
                    throw Ex_Parsing4bppAs8bpp();
            }

            //Allocate, so we can build the refs, and speed up the process!
            sprite.m_frames.resize( m_wanImgDataInfo.nb_ptrs_frm_ptrs_table );

            //Build references!
            sprite.RebuildAllReferences();

            //Read images, use meta frames to get proper res, thanks to the refs!
            //ReadFramesAs8bpp( sprite.m_frames, sprite.m_metaframes);
            ReadFrames<TIMG_t>( sprite.m_frames, sprite.m_metaframes, sprite.m_metarefs );

            return std::move( sprite );
        }

        //This parse all images of the sprite as 4bpp!
        graphics::SpriteData<gimg::tiled_image_i4bpp> ParseAs4bpp( std::atomic<uint32_t> * pProgress = nullptr)
        {
            return Parse<gimg::tiled_image_i4bpp>(pProgress);
        }

        //This parse all images of the sprite as 8bpp!
        graphics::SpriteData<gimg::tiled_image_i8bpp> ParseAs8bpp(std::atomic<uint32_t> * pProgress = nullptr)
        {
            return Parse<gimg::tiled_image_i8bpp>(pProgress);
        }

    private:
        //Methods

        //A generic parsing method to parse common non-image format specific stuff!
        void DoParse( std::vector<gimg::colorRGB24>               & out_pal, 
                      graphics::SprInfo                           & out_sprinf,
                      std::vector<graphics::MetaFrame>            & out_mfrms,
                      std::vector<graphics::SpriteAnimationGroup> & out_anims );

        template<class TIMG_t>
            void ReadFrames( std::vector<TIMG_t>                    & out_imgs, 
                             const std::vector<graphics::MetaFrame> & metafrms,
                             const std::multimap<uint32_t,uint32_t> & metarefs)
        {
            using namespace std;
            vector<uint8_t>::const_iterator itfrmptr = m_rawdata.begin() + m_wanImgDataInfo.ptr_img_table; //Make iterator to frame pointer table

            //ensure capacity
            out_imgs.resize( m_wanImgDataInfo.nb_ptrs_frm_ptrs_table ); 

            //Read all ptrs in the raw data!
            for( unsigned int i = 0; i < m_wanImgDataInfo.nb_ptrs_frm_ptrs_table; ++i )
            {
                uint32_t          ptrtoimg = utils::ReadIntFromByteVector<uint32_t>( itfrmptr ); //iter is incremented automatically
                utils::Resolution myres    = RES_32x32_SPRITE;   //Default to 32x32
                auto              itfound  = metarefs.find( i ); //Find if we have a meta-frame pointing to that frame

                if( itfound != metarefs.end() )
                    myres = MetaFrame::eResToResolution( metafrms[itfound->second].resolution );

                //Parse the image with the best resolution we could find!
                out_imgs[i] = ParseZeroStrippedTImg<TIMG_t>( m_rawdata.begin() + ptrtoimg, 
                                                             m_rawdata.begin(), 
                                                             myres );
            }
        }


        void                                        ReadSir0Header();
        void                                        ReadWanHeader();
        std::vector<gimg::colorRGB24>               ReadPalette();      //
        std::vector<graphics::MetaFrame>            ReadMetaFrames();   // Read Meta-Frames
        graphics::MetaFrame                         ReadAMetaFrame( std::vector<uint8_t>::const_iterator itread );

        //void                                        ReadFramesAs4bpp( std::vector<gimg::tiled_image_i4bpp>   & out_imgs, 
        //                                                              const std::vector<graphics::MetaFrame> & metafrms,
        //                                                              const std::map<uint32_t,uint32_t>      & metarefs); // Reads the actual images data for each frames
        //void                                        ReadFramesAs8bpp( std::vector<gimg::tiled_image_i8bpp>   & out_imgs, 
        //                                                              const std::vector<graphics::MetaFrame> & metafrms,
        //                                                              const std::map<uint32_t,uint32_t>      & metarefs); // Reads the actual images data for each frames
        std::vector<graphics::SpriteAnimationGroup> ReadAnimations();   // Reads the animation data
        graphics::AnimationSequence                 ReadASequence( std::vector<uint8_t>::const_iterator itwhere );
        std::vector<graphics::AnimationSequence>    ReadAnimSequences( std::vector<uint8_t>::const_iterator itwhere, unsigned int nbsequences, unsigned int parentgroupindex );

    private:
        //Variables
        sir0_header            m_sir0Header;
        wan_sub_header         m_wanHeader;
        wan_anim_info          m_wanAnimInfo;
        wan_img_data_info      m_wanImgDataInfo;
        wan_pal_info           m_paletteInfo;

        std::vector<uint8_t>   m_rawdata;
        const animnamelst_t   *m_pANameList; //List of names to give animation groups and its sequences! The first name in the sub-vector is the name of the group! The others are the names of the sequences for that group!
        std::atomic<uint32_t> *m_pProgress;
    };

    //A function form of the above, that deals with a file path, instead of the raw data of the file.
    //gimg::SpriteData<gimg::tiled_image_i4bpp> ParseA_4bpp_WAN_Sprite( std::string wanfilepath );

    /*
        Class to write a WAN file from a SpriteData object!
    */

    class Write_WAN
    {
    public:
        //typedef _Sprite_T sprite_t;

        //Parse_WAN( sprite_t && sprite )
        //{
        //}

        //Parse_WAN( const sprite_t &  sprite )
        //{
        //}

        void WriteUnpacked(const std::string & outputpath, std::atomic<uint32_t> * pProgress = nullptr )
        {
        }

        void WriteWan(const std::string & outputpath, std::atomic<uint32_t> * pProgress = nullptr )
        {
            //Don't forget to build the SIR0 pointer offset table !
            // We must gather the offset of ALL pointers!
        }

    private:
        //sprite_t               m_sprite;
        std::atomic<uint32_t> *m_pProgress;
    };


};};

#endif