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
//            WAN_AnimGrpEntry
//============================================
    struct WAN_AnimGrpEntry
    {
        uint32_t ptrgrp,
                 nbseqs;

        /**************************************************************
        **************************************************************/
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToByteVector( ptrgrp, itwriteto );
            itwriteto = utils::WriteIntToByteVector( nbseqs, itwriteto );
            return itwriteto;
        }

        /**************************************************************
        **************************************************************/
        template<class _init>
            _init ReadFromContainer( _init itReadfrom )
        {
            ptrgrp = utils::ReadIntFromByteVector<decltype(ptrgrp)>(itReadfrom);
            nbseqs = utils::ReadIntFromByteVector<decltype(nbseqs)>(itReadfrom);
            return itReadfrom;
        }
    };
//============================================
//             
//============================================

    /**************************************************************
    **************************************************************/
    AnimFrame ReadAnimFrameFromContainer( vector<uint8_t>::const_iterator & itread )
    {
        AnimFrame myfrm;

        myfrm.frameDuration   = utils::ReadIntFromByteVector<decltype(myfrm.frameDuration)>  (itread);
        myfrm.metaFrmGrpIndex = utils::ReadIntFromByteVector<decltype(myfrm.metaFrmGrpIndex)>(itread);
        myfrm.sprOffsetX     = utils::ReadIntFromByteVector<decltype(myfrm.sprOffsetX)>    (itread);
        myfrm.sprOffsetY     = utils::ReadIntFromByteVector<decltype(myfrm.sprOffsetY)>    (itread);
        myfrm.shadowOffsetX  = utils::ReadIntFromByteVector<decltype(myfrm.shadowOffsetX)> (itread);
        myfrm.shadowOffsetY  = utils::ReadIntFromByteVector<decltype(myfrm.shadowOffsetY)> (itread);

        return std::move(myfrm);
    }

    /**************************************************************
    **************************************************************/
    vector<uint8_t>::iterator WriteAnimFrameToContainer( const AnimFrame & frame, vector<uint8_t>::iterator itwrite )
    {
        itwrite = utils::WriteIntToByteVector( frame.frameDuration,    itwrite );
        itwrite = utils::WriteIntToByteVector( frame.metaFrmGrpIndex,  itwrite );
        itwrite = utils::WriteIntToByteVector( frame.sprOffsetX,      itwrite );
        itwrite = utils::WriteIntToByteVector( frame.sprOffsetY,      itwrite );
        itwrite = utils::WriteIntToByteVector( frame.shadowOffsetX,   itwrite );
        itwrite = utils::WriteIntToByteVector( frame.shadowOffsetY,   itwrite );

        return itwrite;
    }

//=============================================================================================
//  Utility Functions
//=============================================================================================

    /**************************************************************
    Gets the pointer to the sequence table of the first non-null 
    group!
    **************************************************************/
    template<class _randit>
        uint32_t GetOffsetFirstAnimSeqPtr( _randit itReadAnimGrp, uint32_t nbAnimGrps, uint32_t offImgBlock )
    {
        //Search for the first non-null group
        for( unsigned int cntanimgrp = 0; cntanimgrp < nbAnimGrps; ++cntanimgrp )
        {
            WAN_AnimGrpEntry entry;
            itReadAnimGrp = entry.ReadFromContainer( itReadAnimGrp );
            if( entry.ptrgrp != 0 && entry.nbseqs != 0 )
                return entry.ptrgrp;
        }

        return offImgBlock; //Return the offset of the next block after anims if all fails
    }

//=============================================================================================
//  WAN_Parser
//=============================================================================================

    /**************************************************************
    **************************************************************/
    WAN_Parser::WAN_Parser( std::vector<uint8_t> && rawdata, const animnamelst_t * animnames )
        :m_rawdata(rawdata), m_pANameList(animnames), m_pProgress(nullptr)
    {
    }

    /**************************************************************
    **************************************************************/
    WAN_Parser::WAN_Parser( const std::vector<uint8_t> &rawdata, const animnamelst_t * animnames )
        :m_rawdata(rawdata), m_pANameList(animnames), m_pProgress(nullptr)
    {
    }

    /**************************************************************
    **************************************************************/
    graphics::eSpriteType WAN_Parser::getSpriteType()const
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

    /**************************************************************
    **************************************************************/
    void WAN_Parser::DoParse( vector<gimg::colorRGB24>     & out_pal, 
                             SprInfo                       & out_sprinf,
                             vector<MetaFrame>             & out_mfrms,
                             vector<MetaFrameGroup>        & out_mtfgrps,
                             vector<SpriteAnimationGroup>  & out_anims,
                             vector<AnimationSequence>     & out_animseqs,
                             vector<sprOffParticle>        & out_offsets )
    {
        ReadSir0Header();
        ReadWanHeader();

        //Get Palette
        out_pal                     = ReadPalette();

        //Named Properties
        out_sprinf.m_nbColorsPerRow = m_paletteInfo.nbcolorsperrow;
        out_sprinf.m_is256Sprite    = m_wanImgDataInfo.is256Colors;
        out_sprinf.m_is8WaySprite   = m_wanHeader.is8DirectionSprite;
        out_sprinf.m_IsMosaicSpr    = m_wanImgDataInfo.isMosaic;

        //Unknowns
        out_sprinf.m_Unk3           = m_paletteInfo.unk3;
        out_sprinf.m_Unk4           = m_paletteInfo.unk4;
        out_sprinf.m_Unk5           = m_paletteInfo.unk5;
        out_sprinf.m_Unk6           = m_wanAnimInfo.unk6;
        out_sprinf.m_Unk7           = m_wanAnimInfo.unk7;
        out_sprinf.m_Unk8           = m_wanAnimInfo.unk8;
        out_sprinf.m_Unk9           = m_wanAnimInfo.unk9;
        out_sprinf.m_Unk10          = m_wanAnimInfo.unk10;
        out_sprinf.m_Unk11          = m_wanImgDataInfo.unk11;
        out_sprinf.m_Unk12          = m_wanHeader.unk12;

        if( m_paletteInfo.nullbytes != 0 )
        {
            cerr << "\nUm.. Woops? Null bytes at the end of the palette info weren't null ???\n";
            assert(false); //The null bytes at the end of the palette info weren't null! WTF do we do now ?
        }

        //Get meta-frames + meta-frame groups
        out_mfrms = ReadMetaFrameGroups( out_mtfgrps );

        //Get anims
        out_anims = ReadAnimGroups();

        //Get the sequences refered to by the groups
        out_animseqs = ReadAnimSequences( out_anims );

        //Get Offsets
        out_offsets = ReadParticleOffsets(); 

        if( m_pProgress != nullptr )
        {
            m_pProgress->store( m_pProgress->load() + ProgressProp_Other );
        }
    }

    /**************************************************************
    **************************************************************/
    void WAN_Parser::ReadSir0Header()
    {
        m_sir0Header.ReadFromContainer(m_rawdata.begin());
    }

    /**************************************************************
    **************************************************************/
    void WAN_Parser::ReadWanHeader()
    {
        m_wanHeader     .ReadFromContainer( m_rawdata.begin() + m_sir0Header.subheaderptr );
        m_wanAnimInfo   .ReadFromContainer( m_rawdata.begin() + m_wanHeader.ptr_animinfo );
        m_wanImgDataInfo.ReadFromContainer( m_rawdata.begin() + m_wanHeader.ptr_imginfo );
    }

    /**************************************************************
    **************************************************************/
    vector<gimg::colorRGB24> WAN_Parser::ReadPalette()
    {
        m_paletteInfo.ReadFromContainer( m_rawdata.begin() + m_wanImgDataInfo.ptr_palette );
        unsigned int             nbcolors = (m_wanImgDataInfo.ptr_palette - m_paletteInfo.ptrpal) / 4;
        vector<gimg::colorRGB24> palettecolors( nbcolors );
        rgbx32_parser            myparser( palettecolors.begin() );

        //2 - Read it
        std::for_each( (m_rawdata.begin() + m_paletteInfo.ptrpal), 
                       (m_rawdata.begin() + m_wanImgDataInfo.ptr_palette), //The pointer points at the end of the palette
                        myparser );

        return std::move(palettecolors);
    }

    /**************************************************************
    **************************************************************/
    graphics::MetaFrame WAN_Parser::ReadAMetaFrame( vector<uint8_t>::const_iterator & itread )
    {
        graphics::MetaFrame result;

        //Read the raw values first
        result.imageIndex = utils::ReadIntFromByteVector<uint16_t>(itread); //itread is incremented automatically!
        result.unk0        = utils::ReadIntFromByteVector<uint16_t>(itread);
        uint16_t offyfl    = utils::ReadIntFromByteVector<uint16_t>(itread);
        uint16_t offxfl    = utils::ReadIntFromByteVector<uint16_t>(itread);
        result.unk1        = utils::ReadIntFromByteVector<uint16_t>(itread);

        //Set the cleaned offsets
        result.offsetY = 0x03FF & offyfl; //keep the 10 lowest bits
        result.offsetX = 0x01FF & offxfl; //Keep the 9  lowest bits

        //Get the resolution
        result.resolution = MetaFrame::GetResolutionFromOffset_uint16( offxfl, offyfl );
        
        //x offset flags
        result.vFlip    = utils::GetBit( offxfl, 13u ) > 0;
        result.hFlip    = utils::GetBit( offxfl, 12u ) > 0;
        //result.isLastMFrmInGrp = utils::GetBit( offxfl, 11u ) > 0;
        result.XOffbit6 = utils::GetBit( offxfl, 10u ) > 0;
        result.XOffbit7 = utils::GetBit( offxfl,  9u ) > 0;
        
        //y offset flags
        result.YOffbit3   = utils::GetBit( offyfl, 13u ) > 0;
        result.Mosaic     = utils::GetBit( offyfl, 12u ) > 0;
        result.YOffbit5   = utils::GetBit( offyfl, 11u ) > 0;
        result.YOffbit6   = utils::GetBit( offyfl, 10u ) > 0;

        return result;
    }

    /**************************************************************
    **************************************************************/
    void WAN_Parser::ReadAMetaFrameGroup( vector<MetaFrameGroup> & out_metafrmgrps,
                                         vector<MetaFrame>      & out_metafrms,
                                         uint32_t                 grpbeg,
                                         uint32_t                 grpend )
    {
        if( ( grpend - grpbeg ) == WAN_LENGTH_META_FRM )
        {
            //Handle a single frame
            uint32_t frmindex = out_metafrms.size(); //get the size before pushback as frm index
            out_metafrms.push_back( ReadAMetaFrame( grpbeg + m_rawdata.begin() ) );
            out_metafrmgrps.push_back( MetaFrameGroup{ vector<size_t>{ frmindex } } ); //create a group with a single entry refering to our new frame!
        }
        else
        {
            //Handle several frames
            auto           itreadgrp = grpbeg + m_rawdata.begin();
            auto           itendgrp  = grpend + m_rawdata.begin();
            MetaFrameGroup curgrp;
            curgrp.metaframes.reserve( ( grpend - grpbeg ) / WAN_LENGTH_META_FRM );

            while( itreadgrp != itendgrp )
            {
                curgrp.metaframes.push_back( out_metafrms.size() );
                out_metafrms.push_back( ReadAMetaFrame(itreadgrp) );
            }
            out_metafrmgrps.push_back( std::move( curgrp ) );
        }
    }

    /**************************************************************
    Fill the Meta-Frame ptr table, and calculate the nb of 
    meta-frames.
        - endofblockoffset : the offset of the end of the 
                             meta-frame block. One after the 
                             last meta-frame.
    **************************************************************/

    uint32_t FillMFTblAndCalcNbMFs( std::vector<uint32_t>               & out_ptrs, 
                                    std::vector<uint8_t>::const_iterator  itbeg, 
                                    std::vector<uint8_t>::const_iterator  itend,
                                    uint32_t                              endofblockoffset )
    {
        uint32_t totalnbMF      = 0;
        uint32_t lastoffsetread = 0;

        for( auto & entry : out_ptrs )
        {
            entry = utils::ReadIntFromByteVector<uint32_t>( itbeg ); //Iterator auto-incremented

            if( lastoffsetread != 0 ) //skip the first one
                totalnbMF += (entry - lastoffsetread) / WAN_LENGTH_META_FRM;

            lastoffsetread = entry;
        }

        //Add the last offset's length
        totalnbMF += ( endofblockoffset - lastoffsetread) / WAN_LENGTH_META_FRM;

        return totalnbMF;
    }

    /**************************************************************
    **************************************************************/
    vector<MetaFrame> WAN_Parser::ReadMetaFrameGroups( vector<MetaFrameGroup> & out_metafrmgrps )
    {
        //Compute the end of the meta-frame ref table, to get its length.
        uint32_t endMFPtrTbl = 0;

        //If the Particle Offset Table exists, use its offset as end of the ref table
        //If the Particle Offset Table DOESN'T EXIST, use the AnimSequenceTable's first entry as end of the ref table
        if( m_wanAnimInfo.ptr_pOffsetsTable != 0 )
            endMFPtrTbl = m_wanAnimInfo.ptr_pOffsetsTable; 
        else 
            endMFPtrTbl = ReadOff<uint32_t>(m_wanAnimInfo.ptr_animGrpTable);

        //First, build our offset list
        uint32_t         nbPtrMFGtbl = (endMFPtrTbl       - (m_wanAnimInfo.ptr_metaFrmTable)) / sizeof(uint32_t);
        auto             itreadtbl   = (m_rawdata.begin() + m_wanAnimInfo.ptr_metaFrmTable);
        vector<uint32_t> MFptrTbl( nbPtrMFGtbl); 


        //Read the offset of one-past-the-last-meta-frame
        auto     itReadAnimGrp   = ( m_rawdata.begin() + m_wanAnimInfo.ptr_animGrpTable );
        uint32_t offsetseqptrtbl = GetOffsetFirstAnimSeqPtr( itReadAnimGrp, 
                                                             m_wanAnimInfo.nb_anim_groups, 
                                                             ReadOff<uint32_t>( m_wanImgDataInfo.ptr_img_table ) ); 
        

        if( offsetseqptrtbl == 0 )
        {
            cerr<< "It seems this sprite's first animation group is null.. This is unxexpected\n";
            assert(false); //This should never happen
            throw std::runtime_error("<!>- FATAL ERROR: Something is wrong with this sprite. Its animation group table, begins with a null entry!");
        }
        uint32_t offsetlast = ReadOff<uint32_t>( offsetseqptrtbl ); //Get the offset of the block after the meta-frames

        //Fill it up & calculate nb of meta-frames in-between
        uint32_t totalnbMF  = FillMFTblAndCalcNbMFs( MFptrTbl, itreadtbl, endMFPtrTbl + m_rawdata.begin(), offsetlast );

        //Create data + alloc
        vector<MetaFrame> mymetaframes;
        uint32_t          progressBefore = 0;

        out_metafrmgrps.reserve( nbPtrMFGtbl );
        mymetaframes   .reserve( totalnbMF );

        if(m_pProgress != nullptr)
            progressBefore = m_pProgress->load();  //Save a little snapshot of the progress

        for( unsigned int i = 0; i < MFptrTbl.size(); ++i ) 
        {
            if( i == (MFptrTbl.size() - 1) ) //Avoid reading next value on last entry!
                ReadAMetaFrameGroup( out_metafrmgrps, mymetaframes, MFptrTbl[i], offsetlast );
            else
                ReadAMetaFrameGroup( out_metafrmgrps, mymetaframes, MFptrTbl[i], MFptrTbl[i+1] );

            if( m_pProgress != nullptr )
                m_pProgress->store( progressBefore + ( (ProgressProp_MetaFrames * (i+1)) / MFptrTbl.size() ) );
        }

        return std::move( mymetaframes );
    }

    /**************************************************************
    **************************************************************/
    graphics::AnimationSequence WAN_Parser::ReadASequence( vector<uint8_t>::const_iterator itwhere )
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

    /**************************************************************
    **************************************************************/
    vector<uint32_t> WAN_Parser::ReadAnimGroupSeqRefs( vector<uint8_t>::const_iterator itwhere, unsigned int nbsequences/*, unsigned int parentgroupindex*/ )
    {
        vector<uint32_t> mysequences;
        mysequences.resize(nbsequences);

        //grab the sequences
        for( unsigned int cpseqs = 0; cpseqs < nbsequences; ++cpseqs )
        {
            //Get the pointer
            uint32_t ptrsequence = utils::ReadIntFromByteVector<uint32_t>( itwhere );

            mysequences[cpseqs] = ptrsequence; //ReadASequence( m_rawdata.begin() + ptrsequence );
            
            //Set sequence name if possible
            //if( m_pANameList != nullptr && m_pANameList->size() > parentgroupindex && (m_pANameList->at(parentgroupindex).size() - 1) > cpseqs )
            //    mysequences[cpseqs].setName( m_pANameList->at(parentgroupindex)[cpseqs+1] );
        }

        return std::move( mysequences );
    }

    /**************************************************************
    **************************************************************/
    std::vector<graphics::AnimationSequence> WAN_Parser::ReadAnimSequences( std::vector<graphics::SpriteAnimationGroup> & groupsWPtr )
    {
        vector<AnimationSequence>    myanimseqs;
        std::map<uint32_t, uint32_t> sequencesLocations; //This is used to check if we already parsed a sequence at a specific address, and if so, at what index in the sequence table is it!

        for( auto & agrp : groupsWPtr )
        {
            for( auto & aptr : agrp.seqsIndexes )
            {
                uint32_t indexInSeqTbl = myanimseqs.size();
                auto     result        = sequencesLocations.insert( make_pair( aptr, indexInSeqTbl ) ); //Add the address to the map

                if( !(result.second) )
                {
                    //The entry was already in the map !
                    indexInSeqTbl = result.first->second;
                }
                else
                {
                    //The entry wasn't there already
                    myanimseqs.push_back( ReadASequence( aptr + m_rawdata.begin() ) );
                }

                aptr = indexInSeqTbl;  //Swap the file offset for a vector index to the sequence!
            }
        }

        return std::move(myanimseqs);
    }

    /**************************************************************
    **************************************************************/
    vector<graphics::SpriteAnimationGroup> WAN_Parser::ReadAnimGroups()
    {
        vector<uint8_t>::const_iterator        itCurGrp = m_rawdata.begin() + m_wanAnimInfo.ptr_animGrpTable;
        vector<graphics::SpriteAnimationGroup> anims( m_wanAnimInfo.nb_anim_groups );
        
        //Each group is made of a pointer, and the nb of sequences over there! Some can be null!
        WAN_AnimGrpEntry entry;
        uint32_t           progressBefore = 0; //Save a little snapshot of the progress
        if(m_pProgress!=nullptr)
            progressBefore = m_pProgress->load();

        for( unsigned int cpgrp = 0; cpgrp < anims.size(); ++cpgrp )
        {
            auto & curseqs = anims[cpgrp].seqsIndexes;
            itCurGrp = entry.ReadFromContainer(itCurGrp);

            //
            if( entry.ptrgrp != 0 && entry.nbseqs != 0 )
            {
                //make an iterator over there !
                vector<uint8_t>::const_iterator itseqs = m_rawdata.begin() + entry.ptrgrp;
                anims[cpgrp].seqsIndexes = ReadAnimGroupSeqRefs( itseqs, entry.nbseqs/*, cpgrp */);
            }
            else
            {
                //If is null, make sure the vector is empty
                curseqs.resize(0);
            }

            //Set the group name from the list if applicable!
            if( m_pANameList != nullptr && m_pANameList->size() < cpgrp )
                anims[cpgrp].group_name = m_pANameList->at(cpgrp).front();

            if( m_pProgress != nullptr )
            {
                m_pProgress->store( progressBefore + ( ( ProgressProp_Animations * (cpgrp+1)) / anims.size() ) );
            }
        }
        
        return std::move(anims);
    }

    /**************************************************************
    **************************************************************/
    vector<sprOffParticle> WAN_Parser::ReadParticleOffsets()
    {
        //First, check if we do have a particle offset block ! It can be omitted!
        if( m_wanAnimInfo.ptr_pOffsetsTable == 0 )
        {
            if( m_pProgress != nullptr )
                m_pProgress->store( m_pProgress->load() + ProgressProp_Offsets );
            return vector<sprOffParticle>();
        }

        //Get the size of the block
        uint32_t                        ptrFirstSeq = GetOffsetFirstAnimSeqPtr( (m_rawdata.begin() + m_wanAnimInfo.ptr_animGrpTable), 
                                                                                m_wanAnimInfo.nb_anim_groups, 
                                                                                ReadOff<uint32_t>( m_wanImgDataInfo.ptr_img_table ) ); 
        uint32_t                        offsetblocklen = ptrFirstSeq - m_wanAnimInfo.ptr_pOffsetsTable;
        vector<sprOffParticle>          offsets( offsetblocklen / 4u ); //2x 16 bit ints per entry
        vector<uint8_t>::const_iterator itCuroffset  = m_rawdata.begin() + m_wanAnimInfo.ptr_pOffsetsTable;
        vector<uint8_t>::const_iterator itEndOffsets = m_rawdata.begin() + ptrFirstSeq;
        uint32_t progressBefore = 0; //Save a little snapshot of the progress

        if(m_pProgress!=nullptr)
            progressBefore = m_pProgress->load();

        for( unsigned int i = 0; i < offsets.size() && itCuroffset != itEndOffsets; ++i )
        {
            offsets[i].offx = utils::ReadIntFromByteVector<uint16_t>(itCuroffset); //Incremented automatically by the function
            offsets[i].offy = utils::ReadIntFromByteVector<uint16_t>(itCuroffset);

            if( m_pProgress != nullptr )
            {
                m_pProgress->store( progressBefore + ( ( ProgressProp_Offsets * (i + 1) ) / offsets.size() ) );
            }
        }

        return std::move(offsets);
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
        cb._endoffset            = headr.ptrPtrOffsetLst;
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
        sir0_header    headr;
        wan_sub_header wanheadr;

        //Check header
        headr.ReadFromContainer( itdatabeg );

        //Check magic number and make sure ptrs aren't null, or invalid.
        if( headr.magic != SIR0_MAGIC_NUMBER_INT || headr.ptrPtrOffsetLst <= 0x10 || headr.subheaderptr <= 0x10 )
            return false;

        //READ WAN SUB-HEADer and check if pointers fit within file!!
        wanheadr.ReadFromContainer( itdatabeg + headr.subheaderptr );

        //Check if the wan header pointers are invalid
        if( wanheadr.is8DirectionSprite >= 2 || 
            wanheadr.ptr_animinfo >= headr.subheaderptr || 
            wanheadr.ptr_imginfo >= headr.subheaderptr )
            return false;

        //Make sure what comes next is either 0xAA 0xAA or 0x04 0x04
        // This covers the 2 possible cases of what might follow the subheader!
        // 1- 0xAA padding bytes
        // 2- The beginning of the SIR0 pointer offset list. Which always begins with 0x0404 ! 
        auto iterafterwan = itdatabeg + (headr.subheaderptr + wan_sub_header::DATA_LEN);
        uint16_t bytesAfterWan = utils::ReadIntFromByteVector<uint16_t>(iterafterwan);
        if( bytesAfterWan != 0xAAAA && bytesAfterWan != 0x0404 )
            return false;

        return true;
    }

//=============================================================================================
//  WAN Identification Rules Registration
//=============================================================================================
    RuleRegistrator<wan_rule> RuleRegistrator<wan_rule>::s_instance;
};};