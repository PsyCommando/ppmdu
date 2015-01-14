#include "wan.hpp"
#include <ppmdu/fmts/content_type_analyser.hpp>
#include <ppmdu/basetypes.hpp>
#include <ppmdu/containers/sprite_data.hpp>
#include <ppmdu/utils/utility.hpp>
#include <ppmdu/containers/index_iterator.hpp>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <iostream>

using namespace std;
using namespace pmd2::graphics;

namespace pmd2{ namespace filetypes
{
//=============================================================================================
//  WAN File Specifics
//=============================================================================================




//============================================
//            spr_anim_grp_entry
//============================================
    struct spr_anim_grp_entry
    {
        uint32_t ptrgrp,
                 nbseqs;

        std::vector<uint8_t>::iterator WriteToContainer(  std::vector<uint8_t>::iterator itwriteto )const
        {
            itwriteto = utils::WriteIntToByteVector( ptrgrp, itwriteto );
            itwriteto = utils::WriteIntToByteVector( nbseqs, itwriteto );
            return itwriteto;
        }

        std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom )
        {
            ptrgrp = utils::ReadIntFromByteVector<decltype(ptrgrp)>(itReadfrom);
            nbseqs = utils::ReadIntFromByteVector<decltype(nbseqs)>(itReadfrom);
            return itReadfrom;
        }
    };
//============================================
//             
//============================================
    AnimFrame ReadAnimFrameFromContainer( vector<uint8_t>::const_iterator & itread )
    {
        AnimFrame myfrm;

        myfrm.frame_duration   = utils::ReadIntFromByteVector<decltype(myfrm.frame_duration)>  (itread);
        myfrm.meta_frame_index = utils::ReadIntFromByteVector<decltype(myfrm.meta_frame_index)>(itread);
        myfrm.spr_offset_x     = utils::ReadIntFromByteVector<decltype(myfrm.spr_offset_x)>    (itread);
        myfrm.spr_offset_y     = utils::ReadIntFromByteVector<decltype(myfrm.spr_offset_y)>    (itread);
        myfrm.shadow_offset_x  = utils::ReadIntFromByteVector<decltype(myfrm.shadow_offset_x)> (itread);
        myfrm.shadow_offset_y  = utils::ReadIntFromByteVector<decltype(myfrm.shadow_offset_y)> (itread);

        return std::move(myfrm);
    }

    vector<uint8_t>::iterator WriteAnimFrameToContainer( const AnimFrame & frame, vector<uint8_t>::iterator itwrite )
    {
        itwrite = utils::WriteIntToByteVector( frame.frame_duration,    itwrite );
        itwrite = utils::WriteIntToByteVector( frame.meta_frame_index,  itwrite );
        itwrite = utils::WriteIntToByteVector( frame.spr_offset_x,      itwrite );
        itwrite = utils::WriteIntToByteVector( frame.spr_offset_y,      itwrite );
        itwrite = utils::WriteIntToByteVector( frame.shadow_offset_x,   itwrite );
        itwrite = utils::WriteIntToByteVector( frame.shadow_offset_y,   itwrite );

        return itwrite;
    }

//============================================
//              wan_sub_header
//============================================
    std::vector<uint8_t>::iterator wan_sub_header::WriteToContainer(  std::vector<uint8_t>::iterator itwriteto )const
    {
        itwriteto = utils::WriteIntToByteVector( ptr_animinfo,       itwriteto );
        itwriteto = utils::WriteIntToByteVector( ptr_imginfo,        itwriteto );
        itwriteto = utils::WriteIntToByteVector( is8DirectionSprite, itwriteto );
        itwriteto = utils::WriteIntToByteVector( unk12,              itwriteto );
        return itwriteto;
    }

    std::vector<uint8_t>::const_iterator wan_sub_header::ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom )
    {
        ptr_animinfo       = utils::ReadIntFromByteVector<decltype(ptr_animinfo)>      (itReadfrom);
        ptr_imginfo        = utils::ReadIntFromByteVector<decltype(ptr_imginfo)>       (itReadfrom);
        is8DirectionSprite = utils::ReadIntFromByteVector<decltype(is8DirectionSprite)>(itReadfrom);
        unk12              = utils::ReadIntFromByteVector<decltype(unk12)>             (itReadfrom);
        return itReadfrom;
    }

    std::string wan_sub_header::toString()const
    {
        static const int IND = 24;
        stringstream strs;
        //<<setfill('.') <<setw(24) <<left 
        strs <<setfill('.') <<setw(IND) <<left <<"Offset Animation Info"  <<": 0x" <<setfill('0') <<setw(8) <<right <<uppercase <<hex <<ptr_animinfo       <<nouppercase <<"\n"
             <<setfill('.') <<setw(IND) <<left <<"Offset Img Format Info" <<": 0x" <<setfill('0') <<setw(8) <<right <<uppercase <<hex <<ptr_imginfo        <<nouppercase <<"\n"
             <<setfill('.') <<setw(IND) <<left <<"Is 8 Way Sprite"        <<": 0x" <<setfill('0') <<setw(4) <<right <<uppercase <<hex <<is8DirectionSprite <<nouppercase <<"\n"
             <<setfill('.') <<setw(IND) <<left <<"Unknown #12"            <<": 0x" <<setfill('0') <<setw(4) <<right <<uppercase <<hex <<unk12              <<nouppercase <<"\n";
        return strs.str();
    }


//============================================
//              wan_frame_data
//============================================
    std::vector<uint8_t>::iterator wan_img_data_info::WriteToContainer(  std::vector<uint8_t>::iterator itwriteto )const
    {
        itwriteto = utils::WriteIntToByteVector( ptr_img_table,          itwriteto );
        itwriteto = utils::WriteIntToByteVector( ptr_palette,            itwriteto );
        itwriteto = utils::WriteIntToByteVector( isMosaic,               itwriteto );
        itwriteto = utils::WriteIntToByteVector( is256Colors,            itwriteto );
        itwriteto = utils::WriteIntToByteVector( unk11,                  itwriteto );
        itwriteto = utils::WriteIntToByteVector( nb_ptrs_frm_ptrs_table, itwriteto );
        return itwriteto;
    }

    std::vector<uint8_t>::const_iterator wan_img_data_info::ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom )
    {
        ptr_img_table          = utils::ReadIntFromByteVector<decltype(ptr_img_table)>         (itReadfrom);
        ptr_palette            = utils::ReadIntFromByteVector<decltype(ptr_palette)>           (itReadfrom);
        isMosaic               = utils::ReadIntFromByteVector<decltype(isMosaic)>              (itReadfrom);
        is256Colors            = utils::ReadIntFromByteVector<decltype(is256Colors)>           (itReadfrom);
        unk11                  = utils::ReadIntFromByteVector<decltype(unk11)>                 (itReadfrom);
        nb_ptrs_frm_ptrs_table = utils::ReadIntFromByteVector<decltype(nb_ptrs_frm_ptrs_table)>(itReadfrom);
        return itReadfrom;
    }

    std::string wan_img_data_info::toString()const
    {
        static const int IND = 24;
        stringstream strs;
        strs <<setfill('.') <<setw(IND) <<left <<"Offset Image Ptr Table" <<": 0x" <<setfill('0') <<setw(8) <<right <<uppercase <<hex <<ptr_img_table <<nouppercase <<"\n"
             <<setfill('.') <<setw(IND) <<left <<"Offset palette Info"    <<": 0x" <<setfill('0') <<setw(8) <<right <<uppercase <<hex <<ptr_palette   <<nouppercase <<"\n"
             <<setfill('.') <<setw(IND) <<left <<"Is mosaic sprite"       <<": 0x" <<setfill('0') <<setw(4) <<right <<uppercase <<hex <<isMosaic      <<nouppercase <<"\n"
             <<setfill('.') <<setw(IND) <<left <<"Is 256 color sprite"    <<": 0x" <<setfill('0') <<setw(4) <<right <<uppercase <<hex <<is256Colors   <<nouppercase <<"\n"
             <<setfill('.') <<setw(IND) <<left <<"Unknown #11"            <<": 0x" <<setfill('0') <<setw(4) <<right <<uppercase <<hex <<unk11         <<nouppercase <<"\n"
             <<setfill('.') <<setw(IND) <<left <<"Nb frames"              <<": "   <<right        <<dec     <<nb_ptrs_frm_ptrs_table           <<"\n";
        return strs.str();
    }

//============================================
//               wan_info_data
//============================================

    std::vector<uint8_t>::iterator wan_anim_info::WriteToContainer(  std::vector<uint8_t>::iterator itwriteto )const
    {
        itwriteto = utils::WriteIntToByteVector( ptr_metaFrmTable,  itwriteto );
        itwriteto = utils::WriteIntToByteVector( ptr_pOffsetsTable, itwriteto );
        itwriteto = utils::WriteIntToByteVector( ptr_animGrpTable,  itwriteto );
        itwriteto = utils::WriteIntToByteVector( nb_anim_groups,    itwriteto );
        itwriteto = utils::WriteIntToByteVector( unk6,              itwriteto );
        itwriteto = utils::WriteIntToByteVector( unk7,              itwriteto );
        itwriteto = utils::WriteIntToByteVector( unk8,              itwriteto );
        itwriteto = utils::WriteIntToByteVector( unk9,              itwriteto );
        itwriteto = utils::WriteIntToByteVector( unk10,             itwriteto );
        return itwriteto;
    }

    std::vector<uint8_t>::const_iterator wan_anim_info::ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom )
    {
        ptr_metaFrmTable  = utils::ReadIntFromByteVector<decltype(ptr_metaFrmTable)> (itReadfrom);
        ptr_pOffsetsTable = utils::ReadIntFromByteVector<decltype(ptr_pOffsetsTable)>(itReadfrom);
        ptr_animGrpTable  = utils::ReadIntFromByteVector<decltype(ptr_animGrpTable)> (itReadfrom);
        nb_anim_groups    = utils::ReadIntFromByteVector<decltype(nb_anim_groups)>   (itReadfrom);
        unk6              = utils::ReadIntFromByteVector<decltype(unk6)>             (itReadfrom);
        unk7              = utils::ReadIntFromByteVector<decltype(unk7)>             (itReadfrom);
        unk8              = utils::ReadIntFromByteVector<decltype(unk8)>             (itReadfrom);
        unk9              = utils::ReadIntFromByteVector<decltype(unk9)>             (itReadfrom);
        unk10             = utils::ReadIntFromByteVector<decltype(unk10)>            (itReadfrom);
        return itReadfrom;
    }

    std::string wan_anim_info::toString()const
    {
        stringstream strs;
        static const int IND = 30;

        strs <<setfill('.') <<setw(IND) <<left 
             <<"Offset MFrames Table"           <<": 0x" <<right <<setfill('0') <<setw(8) <<hex <<uppercase << ptr_metaFrmTable <<nouppercase <<"\n"

             <<setfill('.') <<setw(IND) <<left 
             <<"Offset Particle Offsets Table"  <<": 0x" <<right <<setfill('0') <<setw(8) <<hex <<uppercase << ptr_pOffsetsTable    <<nouppercase <<"\n"

             <<setfill('.') <<setw(IND) <<left 
             <<"Offset table G"                 <<": 0x" <<right <<setfill('0') <<setw(8) <<hex <<uppercase << ptr_animGrpTable    <<nouppercase <<"\n"

             <<setfill('.') <<setw(IND) <<left 
             <<"NbEntries table G"              <<": "   <<right <<dec          << nb_anim_groups           <<"\n"

             <<setfill('.') <<setw(IND) <<left 
             <<"Unknown #6"                     <<": "   <<right <<dec          << unk6             <<"\n"

             <<setfill('.') <<setw(IND) <<left 
             <<"Unknown #7"                     <<": 0x" <<right <<setfill('0') <<setw(4) <<hex <<uppercase << unk7        <<nouppercase <<"\n"

             <<setfill('.') <<setw(IND) <<left 
             <<"Unknown #8"                     <<": 0x" <<right <<setfill('0') <<setw(4) <<hex <<uppercase << unk8        <<nouppercase <<"\n"

             <<setfill('.') <<setw(IND) <<left 
             <<"Unknown #9"                     <<": 0x" <<right <<setfill('0') <<setw(4) <<hex <<uppercase << unk9        <<nouppercase <<"\n"

             <<setfill('.') <<setw(IND) <<left 
             <<"Unknown #10"                    <<": 0x" <<right <<setfill('0') <<setw(4) <<hex <<uppercase << unk10        <<nouppercase <<"\n";
        return strs.str();
    }

//============================================
//              wan_pal_info
//============================================
    string wan_pal_info::toString()const
    {
        stringstream sstr;

        assert(false);

        return sstr.str();
    }

    vector<uint8_t>::iterator wan_pal_info::WriteToContainer( vector<uint8_t>::iterator itwriteto )const
    {
        itwriteto = utils::WriteIntToByteVector( ptrpal,         itwriteto );
        itwriteto = utils::WriteIntToByteVector( unk3,           itwriteto );
        itwriteto = utils::WriteIntToByteVector( nbcolorsperrow, itwriteto );
        itwriteto = utils::WriteIntToByteVector( unk4,           itwriteto );
        itwriteto = utils::WriteIntToByteVector( unk5,           itwriteto );
        itwriteto = utils::WriteIntToByteVector( nullbytes,      itwriteto );
        return itwriteto;
    }

    vector<uint8_t>::const_iterator wan_pal_info::ReadFromContainer( vector<uint8_t>::const_iterator itReadfrom )
    {
        ptrpal         = utils::ReadIntFromByteVector<decltype(ptrpal)>        (itReadfrom);
        unk3           = utils::ReadIntFromByteVector<decltype(unk3)>          (itReadfrom);
        nbcolorsperrow = utils::ReadIntFromByteVector<decltype(nbcolorsperrow)>(itReadfrom);
        unk4           = utils::ReadIntFromByteVector<decltype(unk4)>          (itReadfrom);
        unk5           = utils::ReadIntFromByteVector<decltype(unk5)>          (itReadfrom);
        nullbytes      = utils::ReadIntFromByteVector<decltype(nullbytes)>     (itReadfrom);
        return itReadfrom;
    }

//=============================================================================================
//  Utility Functions
//=============================================================================================



//=============================================================================================
//  Parse_WAN
//=============================================================================================

    Parse_WAN::Parse_WAN( std::vector<uint8_t> && rawdata, const animnamelst_t * animnames )
        :m_rawdata(rawdata), m_pANameList(animnames)
    {
    }

    Parse_WAN::Parse_WAN( const std::vector<uint8_t> &rawdata, const animnamelst_t * animnames )
        :m_rawdata(rawdata), m_pANameList(animnames)
    {
    }

    Parse_WAN::eSpriteType Parse_WAN::getSpriteType()const
    {
        auto        itRead   = m_rawdata.begin();
        sir0_header sir0head;
        sir0head.ReadFromContainer(itRead);

        //Jump to WAN header
        wan_sub_header wanhead;
        wanhead.ReadFromContainer( itRead + sir0head.subheaderptr );

        //Get the info on the pixel type
        wan_img_data_info frmdat;
        frmdat.ReadFromContainer( itRead + wanhead.ptr_imginfo );

        return (frmdat.is256Colors == 1 )? eSpriteType::spr8bpp : eSpriteType::spr4bpp;
    }

    void Parse_WAN::DoParse( vector<gimg::colorRGB24>     & out_pal, 
                             SprInfo                      & out_sprinf,
                             vector<MetaFrame>            & out_mfrms,
                             vector<SpriteAnimationGroup> & out_anims )
    {
        ReadSir0Header();
        ReadWanHeader();

        out_pal                     = ReadPalette();
        out_sprinf.m_nbColorsPerRow = m_paletteInfo.nbcolorsperrow;
        out_sprinf.m_Unk3           = m_paletteInfo.unk3;
        out_sprinf.m_Unk4           = m_paletteInfo.unk4;
        out_sprinf.m_Unk5           = m_paletteInfo.unk5;

        if( m_paletteInfo.nullbytes != 0 )
        {
            cerr << "\nUm.. Woops? Null bytes at the end of the palette info weren't null ???\n";
            assert(false); //The null bytes at the end of the palette info weren't null! WTF do we do now ?
        }

        out_mfrms = ReadMetaFrames();
        out_anims = ReadAnimations();
    }


    void Parse_WAN::ReadSir0Header()
    {
        m_sir0Header.ReadFromContainer(m_rawdata.begin());
    }

    void Parse_WAN::ReadWanHeader()
    {
        m_wanHeader     .ReadFromContainer( m_rawdata.begin() + m_sir0Header.subheaderptr );
        m_wanAnimInfo   .ReadFromContainer( m_rawdata.begin() + m_wanHeader.ptr_animinfo );
        m_wanImgDataInfo.ReadFromContainer( m_rawdata.begin() + m_wanHeader.ptr_imginfo );
    }

    vector<gimg::colorRGB24> Parse_WAN::ReadPalette()
    {
        m_paletteInfo.ReadFromContainer( m_rawdata.begin() + m_wanImgDataInfo.ptr_palette );

        unsigned int             nbcolors = (m_wanImgDataInfo.ptr_palette - m_paletteInfo.ptrpal) / 4;
        vector<gimg::colorRGB24> palettecolors(nbcolors);
        auto                     itcolread = m_rawdata.begin() + m_paletteInfo.ptrpal;

        for( unsigned int i = 0; i < nbcolors; ++i )
            itcolread = palettecolors[i].ReadAsRawByte( itcolread );

        return std::move(palettecolors);
    }

    graphics::MetaFrame Parse_WAN::ReadAMetaFrame( vector<uint8_t>::const_iterator itread )
    {
        graphics::MetaFrame result;

        //Read the raw values first
        result.image_index = utils::ReadIntFromByteVector<uint16_t>(itread); //itread is incremented automatically!
        result.unk0        = utils::ReadIntFromByteVector<uint16_t>(itread);
        uint16_t offyfl    = utils::ReadIntFromByteVector<uint16_t>(itread);
        uint16_t offxfl    = utils::ReadIntFromByteVector<uint16_t>(itread);
        result.unk1        = utils::ReadIntFromByteVector<uint16_t>(itread);

        //Set the cleaned offsets
        result.offset_y = 0x03FF & offyfl; //keep the 10 lowest bits
        result.offset_x = 0x01FF & offxfl; //Keep the 9  lowest bits

        //Get the resolution
        result.resolution = MetaFrame::GetResolutionFromOffset_uint16( offxfl, offyfl );
        
        //x offset flags
        result.vFlip    = utils::GetBit( offxfl, 13u ) > 0;
        result.hFlip    = utils::GetBit( offxfl, 12u ) > 0;
        result.XOffbit5 = utils::GetBit( offxfl, 11u ) > 0;
        result.XOffbit6 = utils::GetBit( offxfl, 10u ) > 0;
        result.XOffbit7 = utils::GetBit( offxfl,  9u ) > 0;
        
        //y offset flags
        result.YOffbit3   = utils::GetBit( offyfl, 13u ) > 0;
        result.Mosaic     = utils::GetBit( offyfl, 12u ) > 0;
        result.YOffbit5   = utils::GetBit( offyfl, 11u ) > 0;
        result.YOffbit6   = utils::GetBit( offyfl, 10u ) > 0;

        //result.vFlip      = (0x2000 & offxfl) > 0;
        //result.hFlip      = (0x1000 & offxfl) > 0;
        //result.XOffbit5   = (0x0800 & offxfl) > 0;
        //result.XOffbit6   = (0x0400 & offxfl) > 0;
        //result.XOffbit7   = (0x0200 & offxfl) > 0;

        //y offset flags
        //result.YOffbit3   = (0x2000 & offyfl) > 0;
        //result.Mosaic     = (0x1000 & offyfl) > 0;
        //result.YOffbit5   = (0x0800 & offyfl) > 0;
        //result.YOffbit6   = (0x0400 & offyfl) > 0;

        return result;
    }

    vector<graphics::MetaFrame> Parse_WAN::ReadMetaFrames()
    {
        vector<graphics::MetaFrame> metaframes;
        uint32_t                    offsetEndRefTable = 0;

        if( m_wanAnimInfo.ptr_pOffsetsTable != 0 )
        {
            //If the Particle Offset Table exists, use its offset as end of the ref table
            offsetEndRefTable = m_wanAnimInfo.ptr_pOffsetsTable;
        }
        else
        {
            //If the Particle Offset Table DOESN'T EXIST, use the AnimSequenceTable's first entry as end of the ref table
            offsetEndRefTable = utils::ReadIntFromByteVector<uint32_t>( m_wanAnimInfo.ptr_animGrpTable + m_rawdata.begin() );
        }

        //Get the amount of meta frames by counting the pointers
        auto           itReadOffsetMF = m_rawdata.begin() + m_wanAnimInfo.ptr_metaFrmTable;
        const uint32_t nbMetaFrames   = (offsetEndRefTable - m_wanAnimInfo.ptr_metaFrmTable) / sizeof(uint32_t);
        metaframes.resize(nbMetaFrames);

        for( unsigned int cptfrm = 0; cptfrm < nbMetaFrames; ++cptfrm )
        {
            uint32_t curptr = utils::ReadIntFromByteVector<uint32_t>( itReadOffsetMF ); //Iterator is incremented automatically!
            metaframes[cptfrm] = ReadAMetaFrame( curptr + m_rawdata.begin() );
        }

        return std::move(metaframes);
    }

    //void Parse_WAN::ReadFramesAs4bpp( vector<gimg::tiled_image_i4bpp>   & out_imgs, 
    //                                  const vector<graphics::MetaFrame> & metafrms, 
    //                                  const map<uint32_t,uint32_t>      & metarefs )
    //{
    //    out_imgs.resize( m_wanImgDataInfo.nb_ptrs_frm_ptrs_table ); //ensure capacity
    //    vector<uint8_t>::const_iterator itfrmptr = m_rawdata.begin() + m_wanImgDataInfo.ptr_img_table;

    //    //Read ptrs in place
    //    for( unsigned int i = 0; i < m_wanImgDataInfo.nb_ptrs_frm_ptrs_table; ++i )
    //    {
    //        uint32_t ptrtoimg = utils::ReadIntFromByteVector<uint32_t>( itfrmptr ); //iter is incremented automatically
    //        //Move construtor involved!
    //        out_imgs[i] = ParseZeroStrippedTImg<gimg::tiled_image_i4bpp>( m_rawdata.begin() + ptrtoimg, 
    //                                                                      m_rawdata.begin(), 
    //                                                                      MetaFrame::eResToResolution( metafrms[metarefs.at( i )].resolution ) );
    //    }
    //}

    //void Parse_WAN::ReadFramesAs8bpp( vector<gimg::tiled_image_i8bpp>   & out_imgs, 
    //                                  const vector<graphics::MetaFrame> & metafrms, 
    //                                  const map<uint32_t,uint32_t>      & metarefs )
    //{
    //    out_imgs.resize( m_wanImgDataInfo.nb_ptrs_frm_ptrs_table ); //ensure capacity
    //    vector<uint8_t>::const_iterator itfrmptr = m_rawdata.begin() + m_wanImgDataInfo.ptr_img_table;

    //    //Read ptrs in place
    //    for( unsigned int i = 0; i < m_wanImgDataInfo.nb_ptrs_frm_ptrs_table; ++i )
    //    {
    //        uint32_t ptrtoimg = utils::ReadIntFromByteVector<uint32_t>( itfrmptr ); //iter is incremented automatically
    //        //Move construtor involved!
    //        out_imgs[i] = ParseZeroStrippedTImg<gimg::tiled_image_i8bpp>( m_rawdata.begin() + ptrtoimg, 
    //                                                                      m_rawdata.begin(), 
    //                                                                      MetaFrame::eResToResolution( metafrms[metarefs.at( i )].resolution ) );
    //    }
    //}

    graphics::AnimationSequence Parse_WAN::ReadASequence( vector<uint8_t>::const_iterator itwhere )
    {
        AnimationSequence asequence;
        AnimFrame         animframe;
        asequence.reserve(16); //Reserve some frames

        do
        {
            animframe = ReadAnimFrameFromContainer( itwhere );
            if( !(animframe.isNull()) ) //ignore the null frame
            {
                asequence.insertFrame( animframe );
            }
        }
        while( !(animframe.isNull()) );

        return std::move( asequence );
    }

    vector<graphics::AnimationSequence> Parse_WAN::ReadAnimSequences( vector<uint8_t>::const_iterator itwhere, unsigned int nbsequences, unsigned int parentgroupindex )
    {
        vector<AnimationSequence> mysequences;
        mysequences.resize(nbsequences);

        //grab the sequences
        for( unsigned int cpseqs = 0; cpseqs < nbsequences; ++cpseqs )
        {
            //Get the pointer
            uint32_t ptrsequence = utils::ReadIntFromByteVector<uint32_t>( itwhere );

            mysequences[cpseqs] = ReadASequence( m_rawdata.begin() + ptrsequence );

            //Set sequence name if possible
            if( m_pANameList != nullptr && m_pANameList->size() > parentgroupindex && (m_pANameList->at(parentgroupindex).size() - 1) > cpseqs )
                mysequences[cpseqs].setName( m_pANameList->at(parentgroupindex)[cpseqs+1] );
        }

        return std::move( mysequences );
    }

    vector<graphics::SpriteAnimationGroup> Parse_WAN::ReadAnimations()
    {
        vector<uint8_t>::const_iterator        itCurGrp = m_rawdata.begin() + m_wanAnimInfo.ptr_animGrpTable;
        vector<graphics::SpriteAnimationGroup> anims( m_wanAnimInfo.nb_anim_groups );
        
        //Each group is made of a pointer, and the nb of sequences over there! Some can be null!
        spr_anim_grp_entry entry;

        for( unsigned int cpgrp = 0; cpgrp < anims.size(); ++cpgrp )
        {
            auto & curseqs = anims[cpgrp].sequences;
            itCurGrp = entry.ReadFromContainer(itCurGrp);

            //
            if( entry.ptrgrp != 0 && entry.nbseqs != 0 )
            {
                //make an iterator over there !
                vector<uint8_t>::const_iterator itseqs = m_rawdata.begin() + entry.ptrgrp;
                anims[cpgrp].sequences = ReadAnimSequences( itseqs, entry.nbseqs, cpgrp );
            }
            else
            {
                //If is null, we have to give the group a single zero length sequence!
                curseqs.resize(1);
            }

            //Set the group name from the list if applicable!
            if( m_pANameList != nullptr && m_pANameList->size() < cpgrp )
                anims[cpgrp].group_name = m_pANameList->at(cpgrp).front();
        }
        

        return std::move(anims);
    }

//=============================================================================================
//  WAN Identification Rules
//=============================================================================================
    /*
        wan_rule
            Rule for identifying WAN content. With the ContentTypeHandler!
    */
    class wan_rule : public IContentHandlingRule
    {
    public:
        wan_rule(){}
        ~wan_rule(){}

        //Returns the value from the content type enum to represent what this container contains!
        virtual e_ContentType getContentType()const;

        //Returns an ID number identifying the rule. Its not the index in the storage array,
        // because rules can me added and removed during exec. Thus the need for unique IDs.
        //IDs are assigned on registration of the rule by the handler.
        content_rule_id_t getRuleID()const;
        void              setRuleID( content_rule_id_t id );

        //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
        //## This method will call "CContentHandler::AnalyseContent()" for each sub-content container found! ##
        virtual ContentBlock Analyse( const filetypes::analysis_parameter & parameters );

        //This method is a quick boolean test to determine quickly if this content handling
        // rule matches, without in-depth analysis.
        virtual bool isMatch(  types::constitbyte_t   itdatabeg, 
                               types::constitbyte_t   itdataend,
                               const std::string    & filext);

    private:
        content_rule_id_t m_myID;
    };


    //Returns the value from the content type enum to represent what this container contains!
    e_ContentType wan_rule::getContentType()const
    {
        return e_ContentType::WAN_SPRITE_CONTAINER;
    }

    //Returns an ID number identifying the rule. Its not the index in the storage array,
    // because rules can me added and removed during exec. Thus the need for unique IDs.
    //IDs are assigned on registration of the rule by the handler.
    content_rule_id_t wan_rule::getRuleID()const
    {
        return m_myID;
    }
    void wan_rule::setRuleID( content_rule_id_t id )
    {
        m_myID = id;
    }

    //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
    ContentBlock wan_rule::Analyse( const filetypes::analysis_parameter & parameters )
    {
        sir0_header headr;
        ContentBlock cb;

        //Attempt reading the SIR0 header
        headr.ReadFromContainer( parameters._itdatabeg );

        //build our content block info
        cb._startoffset          = 0;
        cb._endoffset            = headr.eofptr;
        cb._rule_id_that_matched = getRuleID();
        cb._type                 = getContentType();

        return cb;
    }

    //This method is a quick boolean test to determine quickly if this content handling
    // rule matches, without in-depth analysis.
    bool wan_rule::isMatch( types::constitbyte_t itdatabeg, types::constitbyte_t itdataend, const std::string & filext )
    {
        using namespace magicnumbers;
        static const unsigned int MaxSubHeaderLen = 27u; //The maximum length of the sub-header + Padding
        sir0_header headr;

        //Check header
        headr.ReadFromContainer( itdatabeg );

        //Check magic number and make sure ptrs aren't null, or invalid.
        if( headr.magic != SIR0_MAGIC_NUMBER_INT || headr.eofptr <= 0x10 || headr.subheaderptr <= 0x10 )
            return false;

        //Literally the best check we can do ^^;
        unsigned int lengthsofar = 0;
        for( auto itcount = itdatabeg + headr.subheaderptr; itcount != itdataend && lengthsofar <= MaxSubHeaderLen; ++lengthsofar, ++itcount )

        //It can't be longer than 26, if the last field ends up on the line below..
        // -- -- -- -- -- -- 01 01 01 01 02 02 02 02 03 03
        // 04 04 AA AA AA AA AA AA AA AA AA AA AA AA AA AA
        if( lengthsofar > MaxSubHeaderLen ) //set to 27, in the very unlikely case that it wouldn't be aligned on the field's size..
        {
            return false;
        }
        else if( lengthsofar == wan_sub_header::DATA_LEN )
        {
            return true;
        }
        else if( lengthsofar > wan_sub_header::DATA_LEN )
        {
            types::constitbyte_t itsearch = itdatabeg;
            std::advance( itsearch, wan_sub_header::DATA_LEN );
            return std::all_of( itsearch, itdataend, []( uint8_t val ){ return val == pmd2::filetypes::COMMON_PADDING_BYTE; } );
        }

        return false;
    }

//=============================================================================================
//  WAN Identification Rules Registration
//=============================================================================================
    RuleRegistrator<wan_rule> RuleRegistrator<wan_rule>::s_instance;
};};