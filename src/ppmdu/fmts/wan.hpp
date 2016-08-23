#ifndef WAN_HPP
#define WAN_HPP
/*
wan.hpp
2014/12/31
psycommando@gmail.com
Description: Utilities for reading ".wan" sprite files, and its derivatives.
*/
#include <ppmdu/fmts/sir0.hpp>
#include <types/content_type_analyser.hpp>
#include <utils/utility.hpp>
#include <utils/handymath.hpp>
#include <ppmdu/containers/sprite_data.hpp>
#include <ppmdu/containers/sprite_io.hpp>
#include <ppmdu/containers/color.hpp>
#include <ppmdu/pmd2/sprite_rle.hpp>
#include <ppmdu/containers/tiled_image.hpp>
#include <ppmdu/pmd2/pmd2_image_formats.hpp>
#include <utils/library_wide.hpp>
#include <utils/handymath.hpp>
#include <atomic>
#include <algorithm>
#include <type_traits>
#include <iomanip>
#include <functional>

namespace filetypes 
{

    typedef std::vector<std::vector<std::string>> animnamelst_t;

    static const bool         WAN_REVERSED_PIX_ORDER = true; //Whether we should read the pixels in reversed bit order, when applicable(4bpp)
    static const unsigned int WAN_LENGTH_META_FRM    = 10; //bytes
    static const unsigned int WAN_LENGTH_ANIM_FRM    = 12; //bytes
    static const unsigned int WAN_LENGTH_ANIM_GRP    = 8;  //bytes

    extern const ContentTy CnTy_WAN; //Contains the ID for the content type in the content type DB. Also contains the file extension

//=============================================================================================
//  WAN Structures
//=============================================================================================

    /**********************************************************************
        ImgAsmTblEntry
            An entry in an image's assembly table. This struct makes it
            easier to read them and write them from/into an assembly 
            table.
    **********************************************************************/
    struct ImgAsmTblEntry
    {
        static const unsigned int LENGTH = 12u;
        uint32_t pixelsrc = 0;  //The source of the pixels to use to rebuild the image. Either an address, or 0
        uint16_t pixamt   = 0;  //The amount of pixels to copy / insert
        uint16_t unk14    = 0;  //Not sure what this does..
        uint32_t zIndex   = 0;  //Possibly z index

        unsigned int    size()const   { return LENGTH; }
        bool            isNull()const { return (!pixelsrc && !pixamt && !zIndex); } //Whether its a null entry or not 

        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( pixelsrc, itwriteto );
            itwriteto = utils::WriteIntToBytes( pixamt,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unk14,    itwriteto );
            itwriteto = utils::WriteIntToBytes( zIndex,   itwriteto );
            return itwriteto;
        }
        
        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itPastEnd )
        {
            pixelsrc = utils::ReadIntFromBytes<decltype(pixelsrc)>(itReadfrom, itPastEnd);
            pixamt   = utils::ReadIntFromBytes<decltype(pixamt)>  (itReadfrom, itPastEnd);
            unk14    = utils::ReadIntFromBytes<decltype(unk14)>   (itReadfrom, itPastEnd);
            zIndex   = utils::ReadIntFromBytes<decltype(zIndex)>  (itReadfrom, itPastEnd);
            return itReadfrom;
        }
    };


    /**********************************************************************
        The 12 bytes sub-header that is linked to by an SIR0 header.
        Contains pointers to important sprite information.
    **********************************************************************/
    struct wan_sub_header
    {
        uint32_t ptr_animinfo = 0;
        uint32_t ptr_imginfo  = 0;
        uint16_t spriteType   = 0;
        uint16_t unk12        = 0;

        static const unsigned int DATA_LEN = 12u;
        unsigned int size()const{return DATA_LEN;}

        template<class _outIt>
            _outIt WriteToContainer( _outIt itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( ptr_animinfo, itwriteto );
            itwriteto = utils::WriteIntToBytes( ptr_imginfo,  itwriteto );
            itwriteto = utils::WriteIntToBytes( spriteType,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unk12,        itwriteto );
            return itwriteto;
        }

        /*
            This will register all pointers written into "ptroffsettbl".
            Used mainly for writing into a SIR0 container.

            Takes a reference to the raw output buffer, and a pointer to a
            function to handle the actual writing and logic for registering the pointer!
        */
        void WriteToWanContainer( std::back_insert_iterator<std::vector<uint8_t>> backins, std::function<void(uint32_t)> funRegisterPtr )const
        {
            //auto backins = std::back_inserter( appendto );
            //Register ptr offset
            //ptroffsettbl.push_back( appendto.size() );
            //utils::WriteIntToBytes( ptr_animinfo, backins );
            funRegisterPtr( ptr_animinfo );

            //Register ptr offset
            //ptroffsettbl.push_back( appendto.size() );
            //utils::WriteIntToBytes( ptr_imginfo,  backins );
            funRegisterPtr(ptr_imginfo);

            utils::WriteIntToBytes( spriteType,   backins );
            utils::WriteIntToBytes( unk12,        backins );
        }

        template<class _inIt>
            _inIt ReadFromContainer( _inIt itReadfrom, _inIt itEnd )
        {
            ptr_animinfo = utils::ReadIntFromBytes<decltype(ptr_animinfo)>(itReadfrom, itEnd);
            ptr_imginfo  = utils::ReadIntFromBytes<decltype(ptr_imginfo)> (itReadfrom, itEnd);
            spriteType   = utils::ReadIntFromBytes<decltype(spriteType)>  (itReadfrom, itEnd);
            unk12        = utils::ReadIntFromBytes<decltype(unk12)>       (itReadfrom, itEnd);
            return itReadfrom;
        }

        void FillFromSprite( pmd2::graphics::BaseSprite * sprite )
        {
            unk12      = sprite->getSprInfo().Unk12;
            spriteType = static_cast<uint16_t>(sprite->getSprInfo().spriteType);
        }
    };

    /**********************************************************************
        The part of the header containing the pointers to the actual frames 
        of the sprite, and the palette.
    **********************************************************************/
    struct wan_img_data_info
    {
        static const unsigned int DATA_LEN = 16u;

        uint32_t ptrImgsTbl   = 0; // Pointer to the the table of pointer to the individual images
        uint32_t ptrPal       = 0; // Pointer to the pointer to the palette info
        uint16_t unk13        = 0; // 1 == mosaic sprite?,   0 == non-mosaic sprite?
        uint16_t is256Colors  = 0; // 1 == 8bpp 256 colors, 0 == 4bpp 16 colors
        uint16_t unk11        = 0; // Unknown, seems to range between 0, 1, and up..
        uint16_t nbImgsTblPtr = 0; // Number of entries in the table of pointers to each frames.

        unsigned int size()const{return DATA_LEN;}

        template<class _outIt>
            _outIt WriteToContainer( _outIt itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( ptrImgsTbl,   itwriteto );
            itwriteto = utils::WriteIntToBytes( ptrPal,       itwriteto );
            itwriteto = utils::WriteIntToBytes( unk13,     itwriteto );
            itwriteto = utils::WriteIntToBytes( is256Colors,  itwriteto );
            itwriteto = utils::WriteIntToBytes( unk11,        itwriteto );
            itwriteto = utils::WriteIntToBytes( nbImgsTblPtr, itwriteto );
            return itwriteto;
        }

        /*
            This will register all pointers written into "ptroffsettbl".
            Used mainly for writing into a SIR0 container.

            Takes a reference to the raw output buffer, and a pointer to a
            function to handle the actual writing and logic for registering the pointer!
        */
        void WriteToWanContainer( std::back_insert_iterator<std::vector<uint8_t>> backins, std::function<void(uint32_t)> funRegisterPtr )const
        {
            //auto backins = std::back_inserter( appendto );

            //Register ptr offset to SIR0 ptr table
            //ptroffsettbl.push_back( appendto.size() );
            //utils::WriteIntToBytes( ptrImgsTbl,    backins );
            funRegisterPtr(ptrImgsTbl);

            //Register ptr offset to SIR0 ptr table
            //ptroffsettbl.push_back( appendto.size() );
            //utils::WriteIntToBytes( ptrPal,        backins );
            funRegisterPtr(ptrPal);

            utils::WriteIntToBytes( unk13,        backins );
            utils::WriteIntToBytes( is256Colors,  backins );
            utils::WriteIntToBytes( unk11,        backins );
            utils::WriteIntToBytes( nbImgsTblPtr, backins );
        }

        template<class _inIt>
            _inIt ReadFromContainer( _inIt itReadfrom, _inIt itend )
        {
            ptrImgsTbl   = utils::ReadIntFromBytes<decltype(ptrImgsTbl)>  (itReadfrom, itend);
            ptrPal       = utils::ReadIntFromBytes<decltype(ptrPal)>      (itReadfrom, itend);
            unk13        = utils::ReadIntFromBytes<decltype(unk13)>       (itReadfrom, itend);
            is256Colors  = utils::ReadIntFromBytes<decltype(is256Colors)> (itReadfrom, itend);
            unk11        = utils::ReadIntFromBytes<decltype(unk11)>       (itReadfrom, itend);
            nbImgsTblPtr = utils::ReadIntFromBytes<decltype(nbImgsTblPtr)>(itReadfrom, itend);
            return itReadfrom;
        }

        void FillFromSprite( pmd2::graphics::BaseSprite * sprite )
        {
            unk13                  = sprite->getSprInfo().Unk13;
            is256Colors            = sprite->getSprInfo().is256Sprite;
            unk11                  = sprite->getSprInfo().Unk11;
            nbImgsTblPtr           = sprite->getNbFrames();
        }
    };

    /**********************************************************************
        The part of the header containing the pointers to unidentified kind 
        of info for the sprite
    **********************************************************************/
    struct wan_anim_info
    {
        static const unsigned int DATA_LEN = 24u;

        uint32_t ptr_metaFrmTable  = 0;  //pointer to the table containing pointers to every meta frames. 
        uint32_t ptr_pOffsetsTable = 0; //ptr to Particle offset table
        uint32_t ptr_animGrpTable  = 0;  //ptr to the table with pointers to every animation groups
        uint16_t nb_anim_groups    = 0;
        uint16_t unk6              = 0;
        uint16_t unk7              = 0;
        uint16_t unk8              = 0;
        uint16_t unk9              = 0;
        uint16_t unk10             = 0;

        unsigned int size()const{return DATA_LEN;}

        //std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const;
        //std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom );


        template<class _outIt>
            _outIt WriteToContainer( _outIt itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( ptr_metaFrmTable,  itwriteto );
            itwriteto = utils::WriteIntToBytes( ptr_pOffsetsTable, itwriteto );
            itwriteto = utils::WriteIntToBytes( ptr_animGrpTable,  itwriteto );
            itwriteto = utils::WriteIntToBytes( nb_anim_groups,    itwriteto );
            itwriteto = utils::WriteIntToBytes( unk6,              itwriteto );
            itwriteto = utils::WriteIntToBytes( unk7,              itwriteto );
            itwriteto = utils::WriteIntToBytes( unk8,              itwriteto );
            itwriteto = utils::WriteIntToBytes( unk9,              itwriteto );
            itwriteto = utils::WriteIntToBytes( unk10,             itwriteto );
            return itwriteto;
        }

        /*
            This will register all pointers written into "ptroffsettbl".
            Used mainly for writing into a SIR0 container.

            Takes a reference to the raw output buffer, and a pointer to a
            function to handle the actual writing and logic for registering the pointer!
        */
        void WriteToWanContainer( std::back_insert_iterator<std::vector<uint8_t>> backins, std::function<void(uint32_t)> funRegisterPtr )const
        {
            //auto backins = std::back_inserter( appendto );

            //Register ptr offset
            //ptroffsettbl.push_back( appendto.size() );
            //utils::WriteIntToBytes( ptr_metaFrmTable,  backins );
            funRegisterPtr(ptr_metaFrmTable);

            //if( ptr_pOffsetsTable != 0 ) //Omit appending it to the ptr offset list if null
            //    ptroffsettbl.push_back( appendto.size() );
            //utils::WriteIntToBytes( ptr_pOffsetsTable, backins );
            funRegisterPtr(ptr_pOffsetsTable);

            //ptroffsettbl.push_back( appendto.size() );
            //utils::WriteIntToBytes( ptr_animGrpTable,  backins );
            funRegisterPtr(ptr_animGrpTable);

            utils::WriteIntToBytes( nb_anim_groups,    backins );
            utils::WriteIntToBytes( unk6,              backins );
            utils::WriteIntToBytes( unk7,              backins );
            utils::WriteIntToBytes( unk8,              backins );
            utils::WriteIntToBytes( unk9,              backins );
            utils::WriteIntToBytes( unk10,             backins );
        }

        template<class _inIt>
            _inIt ReadFromContainer( _inIt itReadfrom, _inIt itend )
        {
            ptr_metaFrmTable  = utils::ReadIntFromBytes<decltype(ptr_metaFrmTable)> (itReadfrom, itend);
            ptr_pOffsetsTable = utils::ReadIntFromBytes<decltype(ptr_pOffsetsTable)>(itReadfrom, itend);
            ptr_animGrpTable  = utils::ReadIntFromBytes<decltype(ptr_animGrpTable)> (itReadfrom, itend);
            nb_anim_groups    = utils::ReadIntFromBytes<decltype(nb_anim_groups)>   (itReadfrom, itend);
            unk6              = utils::ReadIntFromBytes<decltype(unk6)>             (itReadfrom, itend);
            unk7              = utils::ReadIntFromBytes<decltype(unk7)>             (itReadfrom, itend);
            unk8              = utils::ReadIntFromBytes<decltype(unk8)>             (itReadfrom, itend);
            unk9              = utils::ReadIntFromBytes<decltype(unk9)>             (itReadfrom, itend);
            unk10             = utils::ReadIntFromBytes<decltype(unk10)>            (itReadfrom, itend);
            return itReadfrom;
        }

        void FillFromSprite( pmd2::graphics::BaseSprite * sprite )
        {
            nb_anim_groups = sprite->getAnimGroups().size();
            unk6           = sprite->getSprInfo().Unk6;
            unk7           = sprite->getSprInfo().Unk7;
            unk8           = sprite->getSprInfo().Unk8;
            unk9           = sprite->getSprInfo().Unk9;
            unk10          = sprite->getSprInfo().Unk10;
        }
    };

    /**********************************************************************
        wan_pal_info
    **********************************************************************/
    struct wan_pal_info
    {
        static const unsigned int DATA_LEN = 16u;

        uint32_t ptrpal         = 0;
        uint16_t unk3           = 0;
        uint16_t nbcolorsperrow = 0;
        uint16_t unk4           = 0;
        uint16_t unk5           = 0;
        uint32_t nullbytes      = 0;

        unsigned int size()const{return DATA_LEN;}

        template<class _outIt>
            _outIt WriteToContainer( _outIt itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( ptrpal,         itwriteto );
            itwriteto = utils::WriteIntToBytes( unk3,           itwriteto );
            itwriteto = utils::WriteIntToBytes( nbcolorsperrow, itwriteto );
            itwriteto = utils::WriteIntToBytes( unk4,           itwriteto );
            itwriteto = utils::WriteIntToBytes( unk5,           itwriteto );
            itwriteto = utils::WriteIntToBytes( nullbytes,      itwriteto );
            return itwriteto;
        }

        template<class _inIt>
            _inIt ReadFromContainer( _inIt itReadfrom, _inIt itpastend )
        {
            ptrpal         = utils::ReadIntFromBytes<decltype(ptrpal)>        (itReadfrom, itpastend);
            unk3           = utils::ReadIntFromBytes<decltype(unk3)>          (itReadfrom, itpastend);
            nbcolorsperrow = utils::ReadIntFromBytes<decltype(nbcolorsperrow)>(itReadfrom, itpastend);
            unk4           = utils::ReadIntFromBytes<decltype(unk4)>          (itReadfrom, itpastend);
            unk5           = utils::ReadIntFromBytes<decltype(unk5)>          (itReadfrom, itpastend);
            nullbytes      = utils::ReadIntFromBytes<decltype(nullbytes)>     (itReadfrom, itpastend);
            return itReadfrom;
        }

        void FillFromSprite( pmd2::graphics::BaseSprite * sprite )
        {
            unk3           = sprite->getSprInfo().Unk3;
            nbcolorsperrow = sprite->getSprInfo().nbColorsPerRow;
            unk4           = sprite->getSprInfo().Unk4;
            unk5           = sprite->getSprInfo().Unk5;
            nullbytes      = 0;
        }

        /*
            This will register all pointers written into "ptroffsettbl".
            Used mainly for writing into a SIR0 container.

            Takes a reference to the raw output buffer, and a pointer to a
            function to handle the actual writing and logic for registering the pointer!
        */
        void WriteToWanContainer( std::back_insert_iterator<std::vector<uint8_t>> backins, std::function<void(uint32_t)> funRegisterPtr )const
        {
            //auto backins = std::back_inserter( appendto );

            funRegisterPtr(ptrpal);
            utils::WriteIntToBytes( unk3,           backins );
            utils::WriteIntToBytes( nbcolorsperrow, backins );
            utils::WriteIntToBytes( unk4,           backins );
            utils::WriteIntToBytes( unk5,           backins );
            utils::WriteIntToBytes( nullbytes,      backins );
        }
    };

//=============================================================================================
//  Functions / Functors
//=============================================================================================

    /**********************************************************************
        ---------------------------
           ParseZeroStrippedTImg
        ---------------------------
            A function to read the compression table for a compressed image 
            in a WAN sprite, and

            - itcomptblbeg : The iterator to beginning of the specific 
                             compression table to decode!
            - filebeg      : An iterator to the beginning of the file, or 
                             to where the offsets in the table are relative 
                             to!
            - imgres       : The resolution of the image to decode !
                             (Obtained from a meta-frame refering to this
                              image!)
            - itinsertat   : A pixel reader to a tiled image where to put
                             the pixel read!

    **********************************************************************/
    template<class _TIMG_t, class _randit>
        uint32_t ParseZeroStrippedTImg( _randit                    itcomptblbeg, 
                                        _randit                    filebeg, 
                                        _randit                    fileend,
                                        utils::Resolution          imgres, 
                                        gimg::PxlReadIter<_TIMG_t> itinsertat )
    {
        using namespace std;
        using namespace utils;
        ImgAsmTblEntry entry;
        uint32_t nb_bytesread = 0;

        do
        {
            itcomptblbeg = entry.ReadFromContainer(itcomptblbeg,fileend);

            if( !(entry.isNull()) )
            {
                //Exec the entry!
                if( entry.pixelsrc == 0  )
                    std::fill_n( itinsertat, entry.pixamt, 0 );
                else
                    std::copy_n( filebeg + entry.pixelsrc, entry.pixamt, itinsertat );
                nb_bytesread += entry.pixamt;
            }

        }while( !(entry.isNull()) );
        return nb_bytesread;
    }

    /**********************************************************************
        ---------------------------
            FillAsmTable
        ---------------------------
            Reads the "assembly table" for a compressed image.
            The assembly table is basically a table that contains 
            offsets indicating where to put zeros in-between strips 
            of pixels to assemble the decompressed tiled image!
    **********************************************************************/
    template<class _randit>
        std::vector<ImgAsmTblEntry> FillAsmTable( _randit itcomptblbeg, _randit itpastend )
    {
        using namespace std;
        using namespace utils;
        std::vector<ImgAsmTblEntry> asmtbl;
        ImgAsmTblEntry              entry;

        do
        {
            itcomptblbeg = entry.ReadFromContainer(itcomptblbeg,itpastend);

            if( !(entry.isNull()) )
            {
                asmtbl.push_back( entry );
            }

        }while( !(entry.isNull()) );

        return std::move(asmtbl);
    }

    /**********************************************************************
        ----------------
            WAN_Parser
        ----------------
            A class to parse a WAN sprite into a SpriteData structure.
            Because image formats can vary between sprites, 
    **********************************************************************/
    class WAN_Parser
    {
    public:

        class Ex_Parsing8bppAs4bpp : public std::runtime_error { public: Ex_Parsing8bppAs4bpp():std::runtime_error("ERROR: Tried to parse 4bpp sprite as 8bpp!"){} };
        class Ex_Parsing4bppAs8bpp : public std::runtime_error { public: Ex_Parsing4bppAs8bpp():std::runtime_error("ERROR: Tried to parse 8bpp sprite as 4bpp!"){} };

        //Constructor. Pass the data to parse.
        WAN_Parser( std::vector<uint8_t>       && rawdata, const animnamelst_t * animnames = nullptr );
        WAN_Parser( const std::vector<uint8_t> &  rawdata, const animnamelst_t * animnames = nullptr );

        //Use this to determine which parsing method to use!
        pmd2::graphics::eSpriteImgType getSpriteType()const;

        template<class TIMG_t>
            pmd2::graphics::SpriteData<TIMG_t> Parse( std::atomic<uint32_t> * pProgress = nullptr )
        {
            SpriteData<TIMG_t> sprite;

            m_pProgress = pProgress;
        
            //Parse the common stuff first!
            DoParse( sprite.m_palette, 
                     sprite.m_common, 
                     sprite.m_metaframes, 
                     sprite.m_metafrmsgroups, 
                     sprite.m_animgroups,
                     sprite.m_animSequences,
                     sprite.m_partOffsets );

            if( m_wanImgDataInfo.is256Colors == 1 )
            {
                if( TIMG_t::pixel_t::GetBitsPerPixel() != 8 ) //Conversion not supported !
                    throw Ex_Parsing8bppAs4bpp();
            }
            else
            {
                if( TIMG_t::pixel_t::GetBitsPerPixel() != 4 ) //Conversion not supported !
                    throw Ex_Parsing4bppAs8bpp();
            }

            //Allocate, so we can build the refs, and speed up the process!
            sprite.m_frames.resize( m_wanImgDataInfo.nbImgsTblPtr );

            //Build references!
            sprite.RebuildAllReferences();

            //Read images, use meta frames to get proper res, thanks to the refs!
            ReadImages<TIMG_t>( sprite.m_frames, sprite.m_metaframes, sprite.m_metarefs, sprite.getPalette(), sprite.getImgsInfo() );

            return std::move( sprite );
        }

        //This parse all images of the sprite as 4bpp!
        pmd2::graphics::SpriteData<gimg::tiled_image_i4bpp> ParseAs4bpp( std::atomic<uint32_t> * pProgress = nullptr)
        {
            return std::move(Parse<gimg::tiled_image_i4bpp>(pProgress));
        }

        //This parse all images of the sprite as 8bpp!
        pmd2::graphics::SpriteData<gimg::tiled_image_i8bpp> ParseAs8bpp(std::atomic<uint32_t> * pProgress = nullptr)
        {
            return std::move(Parse<gimg::tiled_image_i8bpp>(pProgress));
        }

    private:
        //Methods

        //A generic parsing method to parse common non-image format specific stuff!
        void DoParse( std::vector<gimg::colorRGB24>                     & out_pal, 
                      pmd2::graphics::SprInfo                           & out_sprinf,
                      std::vector<pmd2::graphics::MetaFrame>            & out_mfrms,
                      std::vector<pmd2::graphics::MetaFrameGroup>       & out_mtfgrps,
                      std::vector<pmd2::graphics::SpriteAnimationGroup> & out_anims,
                      std::vector<pmd2::graphics::AnimationSequence>    & out_animseqs,
                      std::vector<pmd2::graphics::sprOffParticle>       & out_offsets );

        template<class TIMG_t>
            void ReadImages( std::vector<TIMG_t>                           & out_imgs, 
                             const std::vector<pmd2::graphics::MetaFrame>  & metafrms,
                             const std::multimap<uint32_t,uint32_t>        & metarefs,
                             const std::vector<gimg::colorRGB24>           & pal,
                             std::vector<pmd2::graphics::ImageInfo>        & out_imginfo )
        {
            using namespace std;
            vector<uint8_t>::const_iterator itfrmptr       = (m_rawdata.begin() + m_wanImgDataInfo.ptrImgsTbl); //Make iterator to frame pointer table
            //uint32_t                        progressBefore = 0; 
            //ensure capacity
            out_imgs.resize( m_wanImgDataInfo.nbImgsTblPtr ); 

            //Save a little snapshot of the progress this far
            //if(m_pProgress != nullptr)
            //    progressBefore = m_pProgress->load(); 

            //Read all ptrs in the raw data!
            for( unsigned int i = 0; i < m_wanImgDataInfo.nbImgsTblPtr; ++i )
            {
                uint32_t ptrtoimg = utils::ReadIntFromBytes<uint32_t>( itfrmptr, static_cast<vector<uint8_t>::const_iterator>(m_rawdata.end()) ); //iter is incremented automatically

                if( utils::LibWide().isLogOn() )
                    std::clog <<"== Frame #" <<i <<" ==\n";

                ReadImage( m_rawdata.begin() + ptrtoimg, metafrms, metarefs, pal, out_imgs[i], i, out_imginfo );
                
                //if( m_pProgress != nullptr )
                //    m_pProgress->store( progressBefore + ( (ProgressProp_Frames * (i+1)) / m_wanImgDataInfo.nbImgsTblPtr ) );
            }
        }

        template<class TIMG_t>
            void ReadImage( std::vector<uint8_t>::iterator             itwhere, 
                               const std::vector<pmd2::graphics::MetaFrame>  & metafrms,
                               const std::multimap<uint32_t,uint32_t>  & metarefs,
                               const std::vector<gimg::colorRGB24>     & pal,
                               TIMG_t                                  & cur_img,
                               uint32_t                                  curfrmindex,
                               std::vector<pmd2::graphics::ImageInfo>        & out_imginfo )
        {
            auto              itfound    = metarefs.find( curfrmindex ); //Find if we have a meta-frame pointing to that frame
            utils::Resolution myres      = RES_64x64_SPRITE;
            uint32_t          totalbyamt = 0;

            //Read the assembly table
            auto asmtable = FillAsmTable( itwhere, m_rawdata.end() );

            //Log the asm table if logging is on
            if( utils::LibWide().isLogOn() )
            {
                std::clog << "Assembly Table:\n";
                for( unsigned int i = 0; i < asmtable.size(); ++i )
                {
                    std::clog << "-> " <<std::setfill(' ') <<std::setw(6) <<asmtable[i].pixelsrc <<", " <<std::setw(4) 
                              <<asmtable[i].pixamt <<", " <<std::setw(4) <<asmtable[i].unk14 <<", " <<std::setw(4) <<asmtable[i].zIndex <<"\n";
                }
                std::clog << "\n";
            }

            //Count the total size of the resulting image
            for( const auto & entry : asmtable )
                totalbyamt += entry.pixamt;

            //Keep track of the z index
            ImageInfo imginf;
            imginf.zindex = asmtable.front().zIndex;
            out_imginfo.push_back(imginf);

            if( itfound != metarefs.end() )
            {
                //If we have a meta-frame for this image, take the resolution from it.
                myres = MetaFrame::eResToResolution( metafrms[itfound->second].resolution );
            }
            else
            {
#ifdef HANDLE_MINUS_ONE_METAFRAME_TEST
                //Run a search on the whole thing again, looking for meta-frames with special index, and check the value of unk15
                uint32_t i = 0;
                for( ; i < metafrms.size(); ++i )
                {
                    if( metafrms[i].HasSpecialImageIndex() && metafrms[i].unk15 == curfrmindex )
                    {
                        myres = MetaFrame::eResToResolution( metafrms[i].resolution );
                        if( utils::LibWide().isLogOn() )
                        {
                            uint32_t unk15_val = metafrms[i].unk15;
                            std::clog << "Image #" <<curfrmindex <<" Meta-Frame refering to -1 found! unk15 value is 0x" 
                                      <<hex << unk15_val
                                      << ". Resolution set to " <<dec <<myres <<"!\n";
                            std::cerr << "Image #" <<curfrmindex <<" Meta-Frame refering to -1 found! unk15 value is 0x" 
                                      <<hex << unk15_val
                                      << ". Resolution set to " <<dec <<myres <<"!\n";
                        }
                        break;
                    }
                }

                if( i == metafrms.size()  )
                {
                    if( utils::LibWide().isLogOn() )
                    {
                        std::clog << "Image #" <<curfrmindex <<", no matching Special Meta-frames found !\n";  
                        std::cerr << "Image #" <<curfrmindex <<", no matching Special Meta-frames found !\n";  
                    }
                }
#else
                if( utils::LibWide().isLogOn() )
                {
                    std::clog << "Image #" <<curfrmindex <<", is orphaned! No matching meta-frame found!\n";
                    std::cerr << "Image #" <<curfrmindex <<", is orphaned! No matching meta-frame found!\n";
                }
#endif
            }
            //else if( metafrms.front().HasSpecialImageIndex() ) 
            //{
            //    if( utils::LibWide().isLogOn() )
            //        std::clog << "Image #" <<curfrmindex <<" Meta-Frame refering to -1 found! unk15 value is " << <<"\n";
            //    assert(false);
            //    //If we DON'T have a meta-frame for this image, take the resolution from the first meta-frame, if its img index is 0xFFFF.
            //    myres = MetaFrame::eResToResolution( metafrms.front().resolution );
            //    //Kind of a bad logic here.. Its really possible that the 0xFFFF meta-frame is not the first one.
            //    // Besides, we're not even 100% sure what to do if we end up with several 0xFFFF pointing meta-frames..
            //    //#TODO: fix this !
            //}
            //else
            //    assert(false);

            uint32_t nbpixperbyte = 8 / TIMG_t::pixel_t::GetBitsPerPixel();
            uint32_t nbPixInBytes = totalbyamt * nbpixperbyte;

            if( nbPixInBytes > (myres.width * myres.height) )
            {
                std::stringstream sstr;
                sstr <<"WARNING: Image #" <<curfrmindex <<", has a resolution that doesn't match the amount of pixels to fill it with!\n"
                     <<"\tExpected resolution of " <<myres <<"(" <<(myres.width * myres.height) <<" pixels), but got a total of " <<nbPixInBytes <<" pixels!\n";
                
                //This is bad, fallback to guessing the size of the image via pixel total
                double squareroot = ceil( sqrt( nbPixInBytes ) );
                long  squareRes  = std::lround(squareroot);
                long  resdivbyTile  = ( (squareRes % TIMG_t::getTileWidth()) == 0 )?
                                        squareRes :
                                        CalcClosestHighestDenominator( squareRes, TIMG_t::getTileWidth() );

                myres = { static_cast<uint32_t>(resdivbyTile), static_cast<uint32_t>(resdivbyTile) };
                sstr << "\tDefaulting to nearest match divisible by 8 : " <<myres <<" !\n"; //Flush

                std::string strres = sstr.str();
                std::cerr <<strres;
                std::clog <<strres;
            }
            else if( nbPixInBytes < (myres.width * myres.height) )
            {
                std::stringstream sstr;
                sstr <<"WARNING:  Image #" <<curfrmindex <<", got less pixels than expected !\n\tExpected " 
                     <<(myres.width * myres.height) <<", but got " <<nbPixInBytes <<" instead !\n";
                std::string strres = sstr.str();
                std::cerr <<strres;
                std::clog <<strres;
            }

            //Parse the image with the best resolution we could find!
            cur_img.setPixelResolution( myres.width, myres.height );
            gimg::PxlReadIter<TIMG_t> myPixelReader(cur_img); 

            //Build the image
            uint32_t byteshandled = ParseZeroStrippedTImg<TIMG_t>(  itwhere, 
                                                                    m_rawdata.begin(), 
                                                                    m_rawdata.end(), 
                                                                    myres,
                                                                    myPixelReader );

            //Copy the palette!
            cur_img.getPalette() = pal;
        }


        void                                        ReadSir0Header();
        void                                        ReadWanHeader();
        std::vector<gimg::colorRGB24>               ReadPalette();

        //Read all the meta-frames + meta-frame groups
        std::vector<pmd2::graphics::MetaFrame>            ReadMetaFrameGroups( std::vector<pmd2::graphics::MetaFrameGroup> & out_metafrmgrps );

        //This reads metaframes from a single meta-frame list pointed to by the meta-frame ref table
        void                                        ReadAMetaFrameGroup( std::vector<pmd2::graphics::MetaFrameGroup> & out_metafrmgrps,
                                                                         std::vector<pmd2::graphics::MetaFrame>      & out_metafrms,
                                                                         uint32_t                                grpbeg/*,
                                                                         uint32_t                                grpend*/ );
        
        //Read a single meta-frame
        pmd2::graphics::MetaFrame                          ReadAMetaFrame( std::vector<uint8_t>::const_iterator & itread, bool & out_isLastFrm );

        std::vector<pmd2::graphics::SpriteAnimationGroup>  ReadAnimGroups();   // Reads the animation data
        pmd2::graphics::AnimationSequence                  ReadASequence( std::vector<uint8_t>::const_iterator itwhere );
        std::vector<uint32_t>                        ReadAnimGroupSeqRefs( std::vector<uint8_t>::const_iterator itwhere, 
                                                                           std::vector<uint8_t>::const_iterator itend,
                                                                           unsigned int nbsequences/*, unsigned int parentgroupindex*/ );
        
        //This get all anim sequences refered to by those groups, and it changes the pointer offsets to indexes in the anim sequence table!
        std::vector<pmd2::graphics::AnimationSequence>    ReadAnimSequences( std::vector<pmd2::graphics::SpriteAnimationGroup> & groupsWPtr );
        
        std::vector<pmd2::graphics::sprOffParticle>       ReadParticleOffsets( const std::vector<pmd2::graphics::SpriteAnimationGroup> & groupsWPtr ); 
        uint32_t                                    CalcFileOffsetBegSeqTable( /*const std::vector<graphics::SpriteAnimationGroup> & groupsPtr*/ );
        /*
            #TODO: is this still in use anymore ?
        */
        template<class _retty>
            inline _retty ReadOff( uint32_t fileoffset, bool littleendian = true )const
        {
            return utils::ReadIntFromBytes<_retty>( (m_rawdata.begin() + fileoffset), m_rawdata.end(), littleendian );
        }

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

        //static const unsigned int       ProgressProp_Frames     = 40; //% of the job
        //static const unsigned int       ProgressProp_MetaFrames = 20; //% of the job
        //static const unsigned int       ProgressProp_Animations = 20; //% of the job
        //static const unsigned int       ProgressProp_Offsets    = 10; //% of the job
        //static const unsigned int       ProgressProp_Other      = 10; //% of the job
    };

    /**********************************************************************
        ----------------
            WAN_Writer
        ----------------
        Class to write a WAN file from a SpriteData object!
    **********************************************************************/
    class WAN_Writer
    {
        static const uint32_t MAX_NB_PIXELS_SPRITE_IMG = 4096;//(graphics::RES_64x64_SPRITE.width * graphics::RES_64x64_SPRITE.height);
    public:
        //typedef _Sprite_T sprite_t;
        WAN_Writer( pmd2::graphics::BaseSprite * pSprite );

        std::vector<uint8_t> write( std::atomic<uint32_t> * pProgress = nullptr );
        void                 write( const std::string     & outputpath, std::atomic<uint32_t> * pProgress = nullptr );

    private:
        /*
            Specialization of ImgAsmTblEntry to add an extra bool to make encoding easier !
        */
        struct ImgAsmTbl_WithOpTy : public ImgAsmTblEntry
        {
            bool isZeroEntry = true; //Whether this entry is for copying zeroes or actual bytes.
        };

        /*
            WriteAPointer
                To make things simpler, always use this method to write the value of a pointer to the buffer at the current pos !
                It will automatically add an entry to the pointer offset table !
        */
        void WriteAPointer( uint32_t val );

        /*
            WritePaddingBytes
                Write padding bytes at the current position
        */
        void WritePaddingBytes( uint32_t alignon );

        /*
            Utilities for allocating and building the headers and structures of the 
            resulting wan file!
        */
        void FillFileInfoStructures();
        void AllocateAndEstimateResultLength();

        /*
        */
        void WriteMetaFramesBlock();
        void WriteAMetaFrame( const pmd2::graphics::MetaFrame & cur, bool setLastBit = false ); //setLastBit set this to true for the last frame in a group !;

        /*
        */
        void WriteAnimationSequencesBlock();
        void WriteAnAnimFrame( const pmd2::graphics::AnimFrame & curfrm );


        /*
            This writes all the image data, stripping them of their zeros when neccessary.
        */
        template<class _frmTy>
            void WriteFramesBlock( const std::vector<_frmTy> & frms )
        {
            //utils::MrChronometer chronoTotal("WriteWanFrames");
            std::vector<uint8_t> imgbuff; //This contains the raw bytes of the current frame
            imgbuff.reserve( MAX_NB_PIXELS_SPRITE_IMG ); //Reserve the maximum frame size

            uint32_t cptfrmindex = 0;
            for( const auto & afrm : frms )
            {
                gimg::WriteTiledImg( std::back_inserter(imgbuff), afrm, WAN_REVERSED_PIX_ORDER );
                WriteACompressedFrm( imgbuff, m_pSprite->getImgsInfo()[cptfrmindex].zindex );
                imgbuff.resize(0);
                ++cptfrmindex;
            }
        }

        /*
            This insert the next sequence into the zero strip table.
            If its a sequence of zero, it won't write into the pixel strip table. If it is, it will.
        */
        ImgAsmTbl_WithOpTy MakeImgAsmTableEntry( std::vector<uint8_t>::const_iterator & itReadAt, 
                                                         std::vector<uint8_t>::const_iterator   itEnd,
                                                         std::vector<uint8_t>                 & pixStrips,
                                                         uint32_t                             & totalbytecnt,
                                                         uint32_t                               imgZIndex );

        /*
            Same as above, but it simply makes a single assembly table entry for the whole image, not stripping the image of 
            any zeroes. Essentially bypassing the whole purpose of the assembly table.
        */
        ImgAsmTbl_WithOpTy MakeImgAsmTableEntryNoStripping( std::vector<uint8_t>::const_iterator & itReadAt, 
                                                                    std::vector<uint8_t>::const_iterator   itEnd,
                                                                    std::vector<uint8_t>                 & pixStrips,
                                                                    uint32_t                             & totalbytecnt,
                                                                    uint32_t                               imgZIndex );

        /*
        , -dontStripZeros: if set to true, the frame will be saved as a is, without stripping the zero
                           but in a way the game can load it. Used for UI sprites !
        */
        void WriteACompressedFrm( const std::vector<uint8_t> & frm, uint32_t imgZIndex, bool dontStripZeros = false );

        /*
        */
        void WritePaletteBlock();

        /*
            MetaFramesRefTable
        */
        void WriteMetaFrameGroupPtrTable();

        /*
            ParticleOffsetsTable
                Write all the offset entries
        */
        void WriteParticleOffsetsBlock();

        /*
        */
        void WriteAnimSequencePtrTable();
        void WriteAnimGroupPtrTable();
        void WriteCompImagePtrTable();

        /*
        */
        void WriteAnimInfoHeadr();
        void WriteImgInfoHeadr();
        void WriteWANHeadr();

        /*
        */
        void WriteSIR0HeaderAndEncodedPtrList();

    private:

        pmd2::graphics::BaseSprite  *m_pSprite;
        std::atomic<uint32_t> *m_pProgress;
        std::string            m_outPath;            //The path to where the file will be written to disk!

        std::vector<uint8_t>   m_outBuffer;          //The file will be written here, before being written to disk !
        std::back_insert_iterator<std::vector<uint8_t>> m_itbackins;

        //Fill those up as we go !!!
        wan_sub_header         m_wanHeadr;
        wan_anim_info          m_wanHeadr_anim;
        wan_img_data_info      m_wanHeadr_img;
        wan_pal_info           m_wanPalInfo;
        sir0_header            m_sir0Header;

        std::vector<uint32_t>  m_MFramesGrpOffsets;       //As we write the meta-frames, keep track of the starting offsets of 
                                                          // groups in there so we can write the pointer table later on!
        std::map<uint32_t,uint32_t>  m_AnimSequenceOffsets;     //Keep tracks of where each animation sequence's frames list begins at !
                                                                // first is index in sequence table, second is offset in file being written

        std::vector<uint32_t>  m_AnimSequencesListOffset; //Keep tracks of where each animation group's sequences ptr table begins at!

        std::vector<uint32_t>  m_CompImagesTblOffsets;    //The places where the zero-strip table for each compressed image is at

        std::vector<uint32_t>  m_ptrOffsetTblToEncode;      //List of all the pointers offsets in the resulting raw file !
    };


};

#endif