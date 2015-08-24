#include "wan.hpp"
#include <ppmdu/fmts/content_type_analyser.hpp>
#include <ppmdu/basetypes.hpp>
#include <ppmdu/containers/sprite_data.hpp>
#include <utils/utility.hpp>
#include <ppmdu/containers/index_iterator.hpp>
#include <utils/library_wide.hpp>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <iostream>

using namespace std;
using namespace pmd2::graphics;

namespace pmd2{ namespace filetypes
{

    template<class T>
        uint32_t CountNbAdjacentNullValues( std::vector<uint8_t>::const_iterator itbeg, std::vector<uint8_t>::const_iterator itend )
    {
        uint32_t cntNullPtrs = 0;
        for(; (itbeg != itend) && (utils::ReadIntFromByteVector<T>(itbeg) == 0); ++cntNullPtrs );
        return cntNullPtrs;
    }

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

//
//
//

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
    graphics::eSpriteImgType WAN_Parser::getSpriteType()const
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

        return (frmdat.is256Colors == 1 )? eSpriteImgType::spr8bpp : eSpriteImgType::spr4bpp;
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
        out_sprinf.nbColorsPerRow  = m_paletteInfo.nbcolorsperrow;
        out_sprinf.is256Sprite     = m_wanImgDataInfo.is256Colors;
        out_sprinf.spriteType      = static_cast<graphics::eSprTy>(m_wanHeader.spriteType);
        out_sprinf.Unk13           = m_wanImgDataInfo.unk13;

        //Unknowns
        out_sprinf.Unk3            = m_paletteInfo.unk3;
        out_sprinf.Unk4            = m_paletteInfo.unk4;
        out_sprinf.Unk5            = m_paletteInfo.unk5;
        out_sprinf.Unk6            = m_wanAnimInfo.unk6;
        out_sprinf.Unk7            = m_wanAnimInfo.unk7;
        out_sprinf.Unk8            = m_wanAnimInfo.unk8;
        out_sprinf.Unk9            = m_wanAnimInfo.unk9;
        out_sprinf.Unk10           = m_wanAnimInfo.unk10;
        out_sprinf.Unk11           = m_wanImgDataInfo.unk11;
        out_sprinf.Unk12           = m_wanHeader.unk12;

        if( m_paletteInfo.nullbytes != 0 )
        {
            cerr << "\nUm.. Woops? Null bytes at the end of the palette info weren't null ???\n";
            //The null bytes at the end of the palette info weren't null! WTF do we do now ?
            throw std::runtime_error("WAN_Parser::DoParse(): Null bytes at the end of the palette info weren't null ???");
        }

        //Get meta-frames + meta-frame groups
        out_mfrms = ReadMetaFrameGroups( out_mtfgrps );

        //Get anims
        out_anims = ReadAnimGroups();

        //Get the sequences refered to by the groups
        out_animseqs = ReadAnimSequences( out_anims );

        //Get Offsets
        out_offsets = ReadParticleOffsets( out_anims ); 
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
        m_paletteInfo.ReadFromContainer( m_rawdata.begin() + m_wanImgDataInfo.ptrPal );
        unsigned int             nbcolors = (m_wanImgDataInfo.ptrPal - m_paletteInfo.ptrpal) / 4;
        vector<gimg::colorRGB24> palettecolors( nbcolors );
        rgbx32_parser            myparser( palettecolors.begin() );

        //2 - Read it
        std::for_each( (m_rawdata.begin() + m_paletteInfo.ptrpal), 
                       (m_rawdata.begin() + m_wanImgDataInfo.ptrPal), //The pointer points at the end of the palette
                        myparser );

        return std::move(palettecolors);
    }

    /**************************************************************
    **************************************************************/
    graphics::MetaFrame WAN_Parser::ReadAMetaFrame( vector<uint8_t>::const_iterator & itread, bool & out_isLastFrm )
    {
        MetaFrame mf;
        itread = mf.ReadFromWANContainer( itread, out_isLastFrm );
        return std::move(mf);
    }

    /**************************************************************
    **************************************************************/
    void WAN_Parser::ReadAMetaFrameGroup( vector<MetaFrameGroup> & out_metafrmgrps,
                                         vector<MetaFrame>       & out_metafrms,
                                         uint32_t                  grpbeg )
    {
        //Handle several frames
        auto           itreadgrp   = grpbeg + m_rawdata.begin();
        auto           itSanityEnd = m_rawdata.end(); //Sanity ends here
        MetaFrameGroup curgrp;
        curgrp.metaframes.reserve(1); //We know we at least have one

        bool islast = false;
        do
        {
            if( itreadgrp >= itSanityEnd )
                throw std::runtime_error( "A meta-frame group continues past the end of the input data!" );

            islast = false;
            curgrp.metaframes.push_back( out_metafrms.size() ); //Write down at what offset the meta-frame is stored at
            out_metafrms     .push_back( ReadAMetaFrame(itreadgrp, islast) );
        }while( !islast );

        out_metafrmgrps.push_back( std::move( curgrp ) );
    }

    /**************************************************************
    Fill the Meta-Frame ptr table, and calculate the nb of 
    meta-frames.
        - endofblockoffset : the offset of the end of the 
                             meta-frame block. One after the 
                             last meta-frame.
    **************************************************************/

    //uint32_t FillMFTblAndCalcNbMFs( std::vector<uint32_t>               & out_ptrs, 
    //                                std::vector<uint8_t>::const_iterator  itbeg, 
    //                                std::vector<uint8_t>::const_iterator  itend,
    //                                uint32_t                              endofblockoffset )
    //{
    //    uint32_t totalnbMF      = 0;
    //    uint32_t lastoffsetread = 0;

    //    for( auto & entry : out_ptrs )
    //    {
    //        entry = utils::ReadIntFromByteVector<uint32_t>( itbeg ); //Iterator auto-incremented

    //        if( lastoffsetread != 0 ) //skip the first one
    //            totalnbMF += (entry - lastoffsetread) / WAN_LENGTH_META_FRM;

    //        lastoffsetread = entry;
    //    }

    //    //Add the last offset's length
    //    totalnbMF += ( endofblockoffset - lastoffsetread) / WAN_LENGTH_META_FRM;

    //    return totalnbMF;
    //}

    /**************************************************************
    **************************************************************/
    vector<MetaFrame> WAN_Parser::ReadMetaFrameGroups( vector<MetaFrameGroup> & out_metafrmgrps )
    {
    //#0 - Compute the end of the meta-frame ref table, to get its length.
        uint32_t endMFPtrTbl = 0;
        const uint32_t begSeqTbl   = CalcFileOffsetBegSeqTable();
        
        if( m_wanAnimInfo.ptr_pOffsetsTable != 0 )
            endMFPtrTbl = m_wanAnimInfo.ptr_pOffsetsTable; //If the Particle Offset Table exists, use its offset as end of the ref table
        else 
            endMFPtrTbl = begSeqTbl; //If the Particle Offset Table DOESN'T EXIST, use the AnimSequenceTable's first entry as end of the ref table

        uint32_t nbPtrMFGtbl = (endMFPtrTbl - (m_wanAnimInfo.ptr_metaFrmTable)) / sizeof(uint32_t);

        //If we don't have any, just skip
        if( nbPtrMFGtbl == 0 )
            return vector<MetaFrame>();

    //#2 - Then, first pass, read all the meta-frame groups
        vector<uint32_t> MFptrTbl( nbPtrMFGtbl ); 
        auto             itreadtbl   = (m_rawdata.begin() + m_wanAnimInfo.ptr_metaFrmTable);
        uint32_t         lastPtrRead = ReadOff<uint32_t>( m_wanAnimInfo.ptr_metaFrmTable );
        uint32_t         nbMetaF     = 0; 

        for( auto & entry : MFptrTbl )
        {
            uint32_t curPtr = utils::ReadIntFromByteVector<uint32_t>( itreadtbl ); //Iterator auto-incremented
            entry = curPtr;
            nbMetaF += ( (curPtr - lastPtrRead) / WAN_LENGTH_ANIM_FRM );
            lastPtrRead = curPtr;
        }

    //#3 - Second pass parse all the meta-frames
        vector<MetaFrame> mymetaframes;
        uint32_t          progressBefore = 0;

        if(m_pProgress != nullptr)
            progressBefore = m_pProgress->load();  //Save a little snapshot of the progress

        out_metafrmgrps.reserve( nbPtrMFGtbl );
        mymetaframes   .reserve( nbMetaF );

        for( unsigned int i = 0; i < MFptrTbl.size(); ++i )
            ReadAMetaFrameGroup( out_metafrmgrps, mymetaframes, MFptrTbl[i] );

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
        }
        
        return std::move(anims);
    }


    /**************************************************************
    **************************************************************/
    uint32_t WAN_Parser::CalcFileOffsetBegSeqTable()
    {
        //Count leading null group entries
        uint32_t nbNullGroups = CountNbAdjacentNullValues<uint64_t>( (m_rawdata.begin() + m_wanAnimInfo.ptr_animGrpTable),
                                                                     (m_rawdata.begin() + m_wanImgDataInfo.ptrImgsTbl) 
                                                                   ); //Each entries is 64 bits

        uint32_t firstNonNullGrp      = m_wanAnimInfo.ptr_animGrpTable + (nbNullGroups * WAN_LENGTH_ANIM_GRP);
        uint32_t nbBytesBefNonNullSeq = ( nbNullGroups * sizeof(uint32_t) );

        //In the case all groups are null
        if( firstNonNullGrp == m_wanImgDataInfo.ptrImgsTbl )
        {
            assert( m_wanAnimInfo.ptr_animGrpTable > nbBytesBefNonNullSeq );
            return ( m_wanAnimInfo.ptr_animGrpTable - nbBytesBefNonNullSeq );
        }
        else
        {
            uint32_t firstNonNullSeq = ReadOff<uint32_t>( firstNonNullGrp );
            assert( firstNonNullSeq > nbBytesBefNonNullSeq );
            return ( firstNonNullSeq - nbBytesBefNonNullSeq );
        }
    }

    /**************************************************************
    **************************************************************/
    vector<sprOffParticle> WAN_Parser::ReadParticleOffsets( const std::vector<graphics::SpriteAnimationGroup> & groupsPtr )
    {
        //First, check if we do have a particle offset block ! It can be omitted!
        if( m_wanAnimInfo.ptr_pOffsetsTable == 0 && 
            m_wanAnimInfo.ptr_pOffsetsTable != m_wanAnimInfo.ptr_metaFrmTable ) //In a few files, instead of being a nullptr the particle offset table has its pointer set to the same offset as the meta-frame table's. File #43 in m_attack.bin for example.
        {
            return vector<sprOffParticle>();
        }

        //Get the begining of the sequence table by subtracting the null sequences before the first non-null one!
        uint32_t                        offsetBegSeqTable = CalcFileOffsetBegSeqTable(); 

        uint32_t                        offsetblocklen = offsetBegSeqTable - m_wanAnimInfo.ptr_pOffsetsTable;
        vector<sprOffParticle>          offsets( offsetblocklen / 4u ); //2x 16 bit ints per entry
        vector<uint8_t>::const_iterator itCuroffset  = m_rawdata.begin() + m_wanAnimInfo.ptr_pOffsetsTable;
        vector<uint8_t>::const_iterator itEndOffsets = m_rawdata.begin() + offsetBegSeqTable;
        //uint32_t                        progressBefore = 0; //Save a little snapshot of the progress

        //if(m_pProgress!=nullptr)
        //    progressBefore = m_pProgress->load();

        for( unsigned int i = 0; i < offsets.size() && itCuroffset != itEndOffsets; ++i )
        {
            offsets[i].offx = utils::ReadIntFromByteVector<uint16_t>(itCuroffset); //Incremented automatically by the function
            offsets[i].offy = utils::ReadIntFromByteVector<uint16_t>(itCuroffset);
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
        cntRID_t getRuleID()const;
        void              setRuleID( cntRID_t id );

        //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
        //## This method will call "CContentHandler::AnalyseContent()" for each sub-content container found! ##
        virtual ContentBlock Analyse( const filetypes::analysis_parameter & parameters );

        //This method is a quick boolean test to determine quickly if this content handling
        // rule matches, without in-depth analysis.
        virtual bool isMatch(  types::constitbyte_t   itdatabeg, 
                               types::constitbyte_t   itdataend,
                               const std::string    & filext);

    private:
        cntRID_t m_myID;
    };


    //Returns the value from the content type enum to represent what this container contains!
    e_ContentType wan_rule::getContentType()const
    {
        return e_ContentType::WAN_SPRITE_CONTAINER;
    }

    //Returns an ID number identifying the rule. Its not the index in the storage array,
    // because rules can me added and removed during exec. Thus the need for unique IDs.
    //IDs are assigned on registration of the rule by the handler.
    cntRID_t wan_rule::getRuleID()const
    {
        return m_myID;
    }
    void wan_rule::setRuleID( cntRID_t id )
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
        if( wanheadr.spriteType >= 2 || 
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
    SIR0RuleRegistrator<wan_rule> SIR0RuleRegistrator<wan_rule>::s_instance;
};};