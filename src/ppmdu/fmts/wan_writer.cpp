/*
    Another cpp file to make the wan.cpp file a little less cluttered!
*/
#include "wan.hpp"
#include <ppmdu/containers/sprite_data.hpp>
#include <vector>
#include <algorithm>
#include <atomic>
#include <string>
#include <iostream>
#include <Poco/Path.h>
using namespace std;
using namespace pmd2::graphics;

namespace pmd2 { namespace filetypes
{
//==================================================================================================
//  WAN_Writer
//==================================================================================================

    /**************************************************************
    **************************************************************/
    WAN_Writer::WAN_Writer( BaseSprite * pSprite )
        :m_pSprite(pSprite), m_itbackins(m_outBuffer)
    {}

    std::vector<uint8_t> WAN_Writer::write( std::atomic<uint32_t> * pProgress )
    {
        utils::MrChronometer chronoTotal("WriteWanTotal");
        //Don't forget to build the SIR0 pointer offset table !
        // We must gather the offset of ALL pointers!
        m_pProgress = pProgress;

        //Fill Structures
        FillFileInfoStructures(); //Fill up the structs
            
        //Allocate
        AllocateAndEstimateResultLength();

        //Reserve 16 bytes at the begining for the SIR0 header we'll write at the end !
        m_outBuffer.resize( filetypes::sir0_header::HEADER_LEN, 0 );

        //Write the file content
        WriteMetaFramesBlock();
        WriteAnimationSequencesBlock();
        WritePaddingBytes(4); //Some 4 bytes padding is needed here

        if( m_pSprite->getSpriteType() == eSpriteType::spr4bpp )
            WriteFramesBlock( *(m_pSprite->getFramesAs4bpp()) );
        else if( m_pSprite->getSpriteType() == eSpriteType::spr8bpp )
            WriteFramesBlock( *(m_pSprite->getFramesAs8bpp()) );

        WritePaletteBlock();
        WriteMetaFrameGroupPtrTable();
        WriteParticleOffsetsBlock();
        WriteAnimSequencePtrTable();
        WriteAnimGroupPtrTable();
        WriteCompImagePtrTable();

        //Write info blocks and header
        WriteAnimInfoHeadr();
        WriteImgInfoHeadr();
        WriteWANHeadr();
        WritePaddingBytes(16); //add some padding

        //SIR0 pointer offset list + SIR0 header
        WriteSIR0HeaderAndEncodedPtrList();
        WritePaddingBytes(16); //Add end of file padding bytes !


        vector<uint8_t> swaped;
        std::swap( swaped, m_outBuffer );//do this to avoid screwing up our internal state
        return std::move(swaped);
    }

    /**************************************************************
    **************************************************************/
    void WAN_Writer::write( const std::string     & outputpath, 
                           std::atomic<uint32_t> * pProgress )
    {
        vector<uint8_t> outvec = write( pProgress );

        Poco::Path outptutwan(outputpath);
        outptutwan.makeFile().setExtension(WAN_FILEX);
        utils::io::WriteByteVectorToFile( outptutwan.toString(), outvec );
    }

    /**************************************************************
    **************************************************************/
    void WAN_Writer::FillFileInfoStructures()
    {
        m_wanHeadr     .FillFromSprite( m_pSprite );
        m_wanHeadr_anim.FillFromSprite( m_pSprite );
        m_wanHeadr_img .FillFromSprite( m_pSprite );
        m_wanPalInfo   .FillFromSprite( m_pSprite );
    }

    /**************************************************************
    **************************************************************/
    void WAN_Writer::AllocateAndEstimateResultLength()
    {
        static const uint32_t MINIMUM_REQUIRED_NB_POINTERS = 10;

        //Calculate lengths
        uint32_t nbAnimFrms              = 0;
        uint32_t nbAnimSequences         = m_pSprite->getAnimSequences().size();
        uint32_t nbAnimSequencePtrTables = 0;
        uint32_t nbEmptyAnimGrps         = 0;
        for( const auto & agrp : m_pSprite->getAnimGroups() )
        {
            uint32_t szcur = agrp.seqsIndexes.size();
            if( szcur != 0 ) //Don't count empty groups+sequences
            {
                for( const auto & aptr : agrp.seqsIndexes )
                {
                    nbAnimFrms += m_pSprite->getAnimSequences()[aptr].getNbFrames();
                }
                //nbAnimSequences += szcur;
                ++nbAnimSequencePtrTables;
            }
            else
                ++nbEmptyAnimGrps;
        }

        //Allocate
        m_MFramesGrpOffsets      .reserve( m_pSprite->getMetaFrmsGrps().size() );
        //m_AnimSequenceOffsets    .reserve( nbAnimSequences                     );
        m_AnimSequencesListOffset.reserve( nbAnimSequencePtrTables             );
        m_CompImagesTblOffsets   .reserve( m_pSprite->getNbFrames()            );
        m_ptrOffsetTblToEncode     .reserve( MINIMUM_REQUIRED_NB_POINTERS + 
                                            m_pSprite->getMetaFrmsGrps().size() + 
                                            nbAnimSequences + 
                                            nbAnimSequencePtrTables + 
                                            m_pSprite->getNbFrames() );

        //Estimate final size to allocate output buffer
        uint32_t totalSize = filetypes::sir0_header::HEADER_LEN;

        //SIR0 header 16 bytes
        //Metafrms are 10 bytes each
        totalSize += m_pSprite->getMetaFrames().size() * WAN_LENGTH_META_FRM;

        //Animframes are 12 bytes each. Each sequences adds an extra 12 for the null ending frame.
        totalSize += nbAnimFrms + (nbAnimSequences * WAN_LENGTH_ANIM_FRM);

        // if the value isn't divisible by 4 without leftovers add 2 bytes for padding!
        totalSize = CalcClosestHighestDenominator( totalSize, 4 );

        //Add worst case scenario size for each images, which is a 2 entry zero-strip table( 24 bytes ), with all bytes of the image above the table.
        totalSize += ( m_pSprite->getNbFrames() * ( 24 + (64*64) ) ); //This is the worst case possible!

        //Palette is nbcolors * 4 bytes + 16 bytes for the palette info!
        totalSize += ( m_pSprite->getPalette().size() * 4 ) + 16;

        //MetaFrmRefTable is nbentries * 4 bytes
        totalSize += ( m_pSprite->getMetaFrames().size() * 4 );

        //ParticleOffsetstable is nboffsets * 4 bytes
        totalSize += ( m_pSprite->getPartOffsets().size() * 4 );

        //Animsequencetable is nbentriesforeachgroups * 4 bytes. Add 4 bytes for each empty anim groups.
        totalSize += ( nbAnimSequences * 4 ) + ( nbEmptyAnimGrps * 4 );

        //AnimGrpTable is literally nbanimgrp * 8 bytes.
        totalSize += ( m_pSprite->getAnimGroups().size() * 8 );
            
        //Img data table is nbimages * 4 bytes
        totalSize += (m_pSprite->getNbFrames() * 4);

        //AnimInfo is 24 bytes
        //ImgDataInfo is 16 bytes
        //Wan reader is 12 bytes
        totalSize += wan_sub_header::DATA_LEN + 
                        wan_anim_info::DATA_LEN + 
                        wan_img_data_info::DATA_LEN;

        // if the size at this point isn't divisible by 16, factor in padding bytes to make it so. 
        totalSize = CalcClosestHighestDenominator( totalSize, 16 );

        //Worst case scenario size is m_ptrOffsetTblToEncode.capacity() * 4 bytes
        totalSize += (m_ptrOffsetTblToEncode.capacity() * 4);

        // if the size at this point isn't divisible by 16, factor in padding bytes to make it so. 
        totalSize = CalcClosestHighestDenominator( totalSize, 16 );

        m_outBuffer.reserve( totalSize );
    }

    /**************************************************************
        WriteAPointer
            To make things simpler, always use this method to 
            write the value of a pointer to the buffer at the 
            current pos !
            It will automatically add an entry to the pointer 
            offset table !
    **************************************************************/
    void WAN_Writer::WriteAPointer( uint32_t val )
    {
        if( val != 0 )  //We ignore null pointers !
            m_ptrOffsetTblToEncode.push_back( m_outBuffer.size() );

        utils::WriteIntToByteVector( val, m_itbackins );
    }

    /**************************************************************
    **************************************************************/
    void WAN_Writer::WriteMetaFramesBlock()
    {
        utils::MrChronometer chronoTotal("WriteWanMetaFrames");
        const auto & metafrms = m_pSprite->getMetaFrames();

        for( const auto & agrp : m_pSprite->getMetaFrmsGrps() )
        {
        //# Note the offset where each groups begins at
            m_MFramesGrpOffsets.push_back( m_outBuffer.size() );

            //Write meta frames group
            for( unsigned int ctfrms = 0; ctfrms < agrp.metaframes.size(); ++ctfrms )
            {
                WriteAMetaFrame( metafrms[ agrp.metaframes[ctfrms] ], ( ctfrms == (agrp.metaframes.size() - 1) ) );
            }
        }
    }

    /**************************************************************
    **************************************************************/
    void WAN_Writer::WriteAMetaFrame( const MetaFrame & cur, bool setLastBit ) //setLastBit set this to true for the last frame in a group !
    {
        utils::WriteIntToByteVector( cur.imageIndex, m_itbackins );
        utils::WriteIntToByteVector( cur.unk0,       m_itbackins );

        //Get the value of the resolution as a byte
        uint8_t resval    = static_cast<uint8_t>(cur.resolution);
        uint8_t EndbitVal = /*((cur.isLastMFrmInGrp)?1:0 ) | */( (setLastBit)?1:0 ); //Force it to one if is last!

        uint16_t YOffset = ( ( resval << 8 ) & 0xC000 ) | (cur.YOffbit3 << 13) | ((cur.Mosaic)?1:0) << 12 | (cur.YOffbit5 << 11) | (cur.YOffbit5 << 10) | cur.offsetY;
        utils::WriteIntToByteVector( YOffset,       m_itbackins );

    //# Don't forget to make sure XOffset bit 5 is set to 1 for the last meta-frame in a group !
        uint16_t XOffset = ( ( resval << 12 ) & 0xC000 ) | ( ((cur.vFlip)?1:0 ) << 13) | ( ((cur.hFlip)?1:0 ) << 12) | ( EndbitVal << 11) | (cur.XOffbit6 << 10) | (cur.XOffbit7 << 9) | cur.offsetX;
        utils::WriteIntToByteVector( XOffset,       m_itbackins );

        utils::WriteIntToByteVector( cur.unk1, m_itbackins );
    }

    /**************************************************************
    **************************************************************/
    void WAN_Writer::WriteAnimationSequencesBlock()
    {
        const auto & animgroups = m_pSprite->getAnimGroups();

        for( const auto & agrp : animgroups )
        {
            for( const auto & aptr : agrp.seqsIndexes )
            {
                auto result = m_AnimSequenceOffsets.insert( make_pair( aptr, m_outBuffer.size() ) );

                if( result.second ) //If the sequence was not already written!
                {
                    auto & aseq = m_pSprite->getAnimSequences()[aptr];

                    for( unsigned int ctfrm = 0; ctfrm < m_pSprite->getAnimSequences()[aptr].getNbFrames(); ++ctfrm )
                    {
                        const auto & curfrm = aseq.getFrame(ctfrm);
                        WriteAnAnimFrame( curfrm );
                        if( ctfrm == (aseq.getNbFrames() - 1) )
                        {
                        //# Don't forget the null frame at the end of all sequence!
                            WriteAnAnimFrame( AnimFrame() );
                        }
                    }
                }


                //if( aptr.getNbFrames() > 0 ) //Ignore empty sequences
                //{
                ////# Write the offsets where each sequences begins at ! (except null ones)
                //    m_AnimSequenceOffsets.push_back( m_outBuffer.size() );
                //    //Write the sequence
                //    for( unsigned int ctfrm = 0; ctfrm < aseq.getNbFrames(); ++ctfrm )
                //    {
                //        const auto & curfrm = aseq.getFrame(ctfrm);
                //        WriteAnAnimFrame( curfrm );
                //        if( ctfrm == (aseq.getNbFrames() - 1) )
                //        {
                //        //# Don't forget the null frame at the end of all sequence!
                //            WriteAnAnimFrame( AnimFrame() );
                //        }
                //    }
                //}
            }
        }
    }

    /**************************************************************
    **************************************************************/
    void WAN_Writer::WriteAnAnimFrame( const AnimFrame & curfrm )
    {
        utils::WriteIntToByteVector( curfrm.frameDuration,   m_itbackins );
        utils::WriteIntToByteVector( curfrm.metaFrmGrpIndex, m_itbackins );
        utils::WriteIntToByteVector( curfrm.sprOffsetX,      m_itbackins );
        utils::WriteIntToByteVector( curfrm.sprOffsetY,      m_itbackins );
        utils::WriteIntToByteVector( curfrm.shadowOffsetX,   m_itbackins );
        utils::WriteIntToByteVector( curfrm.shadowOffsetY,   m_itbackins );
    }

    /**************************************************************
    **************************************************************/
    void WAN_Writer::WritePaddingBytes( uint32_t alignon )
    {
        const uint32_t bufflen = m_outBuffer.size();
    //# Insert padding at the current write position, to align the next entry on "alignon" bytes
        if( (bufflen % alignon) != 0 )
        {
            uint32_t lenpadding = ( CalcClosestHighestDenominator( bufflen, alignon ) -  bufflen );

            for( unsigned int ctpad = 0; ctpad < lenpadding; ++ctpad )
                utils::WriteIntToByteVector( filetypes::COMMON_PADDING_BYTE, m_itbackins );
        }
    }

    /**************************************************************
        This insert the next sequence into the zero strip table.
        If its a sequence of zero, it won't write into the pixel 
        strip table. If it is, it will.    
    **************************************************************/
    WAN_Writer::zeroStripTableTempEntry WAN_Writer::MakeZeroStripTableEntry( std::vector<uint8_t>::const_iterator & itReadAt, 
                                                        std::vector<uint8_t>::const_iterator   itEnd,
                                                        std::vector<uint8_t>                 & pixStrips,
                                                        uint32_t                             & totalbytecnt) 
    {
        static const unsigned int MIN_Increments = 0x20; //Minimum increments of 32 bytes
        auto lambdaIsZero    =  []( uint8_t val )->bool{ return (val == 0); };
        auto itReadAtBefore  = itReadAt; //This is to hold the last read position for the duration of this function
        auto itCurPos        = itReadAt; //This is to iterate over the data, without modifying itReadAt just yet! 
        std::advance( itCurPos, MIN_Increments );
            
        //See if we're dealing with a sequence of zeroes or not!
        bool isZeroSeq = std::all_of( itReadAtBefore, itCurPos, lambdaIsZero );

        //This is to indicate where the last sequence of 16 bytes we analyzed began at!
        auto itLastSeqBeg = itCurPos;
        uint32_t countAmtBytes = MIN_Increments;    //we already got 16 bytes in our sequence

        //Try to get other blocks of the same kind !
        while( itCurPos != itEnd )
        {
            std::advance( itCurPos, MIN_Increments ); //This will throw an exception if the pixel count isn't divisible by our ammount of increment

            //Check if the newly added bytes are the same kind of bytes we're looking for!
            if( isZeroSeq == std::all_of( itLastSeqBeg, itCurPos, lambdaIsZero ) )
            {
                countAmtBytes += MIN_Increments;
                itLastSeqBeg = itCurPos;
            }
            else
            {
                break;
            }
        }
        zeroStripTableTempEntry myentry;
        myentry.pixelsrc    = ( (isZeroSeq)? 0 : pixStrips.size() );
        myentry.isZeroEntry = isZeroSeq;
        myentry.pixamt      = countAmtBytes;
        myentry.unknown     = 0; //#TODO: figure out what this thing does !

        if( !isZeroSeq )
            std::copy( itReadAtBefore, itLastSeqBeg, std::back_inserter(pixStrips) );

        //auto dist = std::distance( itReadAtBefore, itLastSeqBeg );

        //Get only the actual sequence's data we're sure of between itReadAtBefore and itLastSeqBeg
        

        totalbytecnt += myentry.pixamt;

        //Increment the iterator now!
        itReadAt = itLastSeqBeg;

        return std::move( myentry );

        //1. Read 16 bytes
        //2. If we got 16 zeroes, we're reading a zero sequence to strip!!! Try to read as much groups of 16 zeroes as possible
        //3. If we don't have 16 zeroes, this is a pixel strip!! Try to read another block of 16 bytes, and make sure its not 16 zeroes!
        // Once we can't read any of our resprective sequence type, return entry!
    }


    /**************************************************************
    **************************************************************/
    WAN_Writer::zeroStripTableTempEntry WAN_Writer::MakeZeroStripTableEntryNoStripping( vector<uint8_t>::const_iterator & itReadAt, 
                                                                                      vector<uint8_t>::const_iterator   itEnd,
                                                                                      vector<uint8_t>                 & pixStrips,
                                                                                      uint32_t                        & totalbytecnt )
    {
        zeroStripTableTempEntry myentry;
        myentry.isZeroEntry = false;
        myentry.pixamt      = std::distance( itReadAt, itEnd );
        myentry.pixelsrc    = pixStrips.size();
        myentry.unknown     = 0;                                //#TODO: figure out what to do with this !

        std::copy( itReadAt, itEnd, std::back_inserter( pixStrips ) );
        totalbytecnt += myentry.pixamt;

        itReadAt = itEnd;
        return std::move( myentry );
    }

    /**************************************************************
    **************************************************************/
    void WAN_Writer::WriteACompressedFrm( const std::vector<uint8_t> & frm, bool dontStripZeros )
    {
        uint32_t imgbegoffset = m_outBuffer.size(); //Keep the offset before to offset the entries in the zerostrip table!

        vector<uint8_t>                 pixelstrips;
        vector<zeroStripTableTempEntry> zerostriptable; 
        auto                            itLaststrip  = frm.begin();
        uint32_t                        totalbytecnt = 0;

        //Encode image
        auto itCurPos = frm.begin();
        auto itEnd    = frm.end();

        while( itCurPos != itEnd )
        {
            if( dontStripZeros )
                zerostriptable.push_back( MakeZeroStripTableEntryNoStripping(itCurPos,itEnd,pixelstrips, totalbytecnt ) );
            else
                zerostriptable.push_back( MakeZeroStripTableEntry( itCurPos, itEnd, pixelstrips, totalbytecnt ) );
        }

        //Write pixel strips
        std::copy( pixelstrips.begin(), pixelstrips.end(), m_itbackins );

        //Save the offset of the upcoming zero strip table
        m_CompImagesTblOffsets.push_back( m_outBuffer.size() );

        //Write table
        for( auto & entry : zerostriptable )
        {
            //Offset the pointers correctly + Register ptr if non-zero
            if( !entry.isZeroEntry )
            {
                entry.pixelsrc += imgbegoffset;
                m_ptrOffsetTblToEncode.push_back( m_outBuffer.size() );
            }
            //Write entry
            entry.WriteToContainer( m_itbackins );
        }

        //Write a final null entry
        zeroStripTableTempEntry().WriteToContainer( m_itbackins );
    }


    /**************************************************************
    **************************************************************/
    void WAN_Writer::WritePaletteBlock()
    {
        //# Note the position of the first color !
        m_wanPalInfo.ptrpal = m_outBuffer.size();

        //Write colors
        graphics::WriteRawPalette_RGB24_As_RGBX32( m_itbackins, m_pSprite->getPalette().begin(), m_pSprite->getPalette().end() );

        //# Note the position the palette info block is written at !
        m_wanHeadr_img.ptr_palette = m_outBuffer.size();

        //Add pointer to colors, to the ptr offset list
        m_ptrOffsetTblToEncode.push_back( m_outBuffer.size() );

        //Write the palette info
        m_wanPalInfo.WriteToContainer(m_itbackins);
    }


    /**************************************************************
    **************************************************************/
    void WAN_Writer::WriteMetaFrameGroupPtrTable()
    {
        //# Note the position it begins at !
        m_wanHeadr_anim.ptr_metaFrmTable = m_outBuffer.size();

        for( const auto & aptr : m_MFramesGrpOffsets )
            WriteAPointer( aptr );
    }

    /**************************************************************
    **************************************************************/
    void WAN_Writer::WriteParticleOffsetsBlock()
    {
        utils::MrChronometer chronoTotal("WriteWanParticleOffsetsBlock");
        //# Write starting offset
        m_wanHeadr_anim.ptr_pOffsetsTable = m_outBuffer.size();

        for( const auto & anoffset : m_pSprite->getPartOffsets() )
        {
            utils::WriteIntToByteVector( anoffset.offx, m_itbackins );
            utils::WriteIntToByteVector( anoffset.offy, m_itbackins );
        }
    }

    /**************************************************************
    **************************************************************/
    void WAN_Writer::WriteAnimSequencePtrTable()
    {
        //# Note the position where all sequences for a group begins at !
        //m_AnimSequencesListOffset;

        //auto itCurPtr = m_AnimSequenceOffsets.begin(); //This is the current location where to get another pointer for buildign our sequence

        //# Don't forget that, there must be a single 4 bytes null entry for a corresponding empty groups in here !
        for( const auto & agrp : m_pSprite->getAnimGroups() )
        {
            if(  agrp.seqsIndexes.empty() )
            {
                //# Note the position where all sequences for a group begins at !
                //m_AnimSequencesListOffset.push_back( 0 ); //Null for null groups !

                //Handle null entries differently
                WriteAPointer( 0 );
            }
            else
            {
                //# Note the position where all sequences for a group begins at !
                m_AnimSequencesListOffset.push_back( m_outBuffer.size() );

                //Write out the offsets to all the non-null sequences we wrote earlier in the file !
                for( unsigned int ctseq = 0; ctseq < agrp.seqsIndexes.size(); ++ctseq/*, ++itCurPtr*/ )
                {
                    uint32_t existingSeq = m_AnimSequenceOffsets.at( agrp.seqsIndexes[ctseq] );
                    WriteAPointer( existingSeq );
                }
            }
                
                
        }
    }


    /**************************************************************
    **************************************************************/
    void WAN_Writer::WriteAnimGroupPtrTable()
    {
        //# Write start pos !
        m_wanHeadr_anim.ptr_animGrpTable = m_outBuffer.size();

        auto itCurPtr = m_AnimSequencesListOffset.begin();

        for( const auto & agrp : m_pSprite->getAnimGroups() )
        {
            //
            if( agrp.seqsIndexes.empty() )
            {
                utils::WriteIntToByteVector( static_cast<uint64_t>(0), m_itbackins ); //Write 8 bytes of 0
            }
            else
            {
                WriteAPointer( *itCurPtr );
                utils::WriteIntToByteVector( agrp.seqsIndexes.size(), m_itbackins );
                ++itCurPtr; //Only increment when we have a non-null entry!
            }
        }
    }

    /**************************************************************
    **************************************************************/
    void WAN_Writer::WriteCompImagePtrTable()
    {
        //Note the position it begins at
        m_wanHeadr_img.ptr_img_table = m_outBuffer.size();
        m_wanHeadr_img.nb_ptrs_frm_ptrs_table = m_pSprite->getNbFrames();

        for( const auto & ptr : m_CompImagesTblOffsets )
            WriteAPointer( ptr );
    }

    /**************************************************************
    **************************************************************/
    void WAN_Writer::WriteAnimInfoHeadr()
    {
        //# Write down starting offset
        m_wanHeadr.ptr_animinfo = m_outBuffer.size();

        //Write anim info
        m_wanHeadr_anim.WriteToWanContainer( m_outBuffer, m_ptrOffsetTblToEncode );
    }

    /**************************************************************
    **************************************************************/
    void WAN_Writer::WriteImgInfoHeadr()
    {
        //# Save location of img info 
        m_wanHeadr.ptr_imginfo = m_outBuffer.size();

        m_wanHeadr_img.WriteToWanContainer( m_outBuffer, m_ptrOffsetTblToEncode );
    }

    /**************************************************************
    **************************************************************/
    void WAN_Writer::WriteWANHeadr()
    {
        //Put offset in sir0 header
        m_sir0Header.subheaderptr = m_outBuffer.size();

        m_wanHeadr.WriteToWanContainer( m_outBuffer, m_ptrOffsetTblToEncode );
    }

    /**************************************************************
    **************************************************************/
    void WAN_Writer::WriteSIR0HeaderAndEncodedPtrList()
    {
        m_sir0Header.eofptr = m_outBuffer.size();
        //Don't forget the 2 pointers of the sir0 header! in first.
        //Add padding after !

        filetypes::sir0_head_and_list result = filetypes::MakeSIR0ForData( m_ptrOffsetTblToEncode, 
                                                                            (m_sir0Header.subheaderptr-16), 
                                                                            (m_sir0Header.eofptr-16) );

        //Write encoded ptr offset list
        for( const auto & encptr : result.ptroffsetslst )
        {
            utils::WriteIntToByteVector( encptr, m_itbackins );
        }

        //Then write SIR0 at the begining !
        result.hdr.WriteToContainer( m_outBuffer.begin() );
    }


};};