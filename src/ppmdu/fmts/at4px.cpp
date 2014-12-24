#include "at4px.hpp"
#include <ppmdu/pmd2/pmd2_filetypes.hpp>
#include <ppmdu/fmts/px_compression.hpp>
#include <ppmdu/pmd2/pmd2_palettes.hpp>
#include <ppmdu/pmd2/pmd2_image_formats.hpp>
#include <exception>
#include <algorithm>
#include <cassert>
#include <array>
using namespace std;
using pmd2::compression::px_info_header;
using pmd2::graphics::rgb15_parser;
using pmd2::compression::ePXCompLevel;

namespace pmd2{ namespace filetypes
{


//==================================================================
// Utility:
//==================================================================
    px_info_header AT4PXHeaderToPXinfo( const at4px_header & head )
    {
        px_info_header px;

        px.compressedsz   = head.compressedsz - at4px_header::HEADER_SZ; //subtract at4px header lenght
        px.decompressedsz = head.decompsz;
        std::copy_n( head.flaglist.begin(), px_info_header::NB_FLAGS, px.controlflags.begin());

        return px;
    }

    at4px_header PXinfoToAT4PXHeader( const px_info_header & pxinf )
    {
        at4px_header myhead;

        myhead.magicn       = filetypes::magicnumbers::AT4PX_MAGIC_NUMBER;
        myhead.compressedsz = pxinf.compressedsz + at4px_header::HEADER_SZ; //Add the header's length
        myhead.flaglist     = pxinf.controlflags;
        myhead.decompsz     = pxinf.decompressedsz;

        return myhead;
    }

//========================================================================================================
//  at4px_header
//========================================================================================================
    
    std::vector<uint8_t>::iterator at4px_header::WriteToContainer( std::vector<uint8_t>::iterator itwriteto )const
    {
        for( const uint8_t & abyte : magicn )
        {
            *itwriteto = abyte; 
            ++itwriteto;
        }

        itwriteto = utils::WriteIntToByteVector( compressedsz, itwriteto );

        for( const uint8_t & aflag : flaglist )
        {
            *itwriteto = aflag; 
            ++itwriteto;
        }

        itwriteto = utils::WriteIntToByteVector( decompsz, itwriteto );

        return itwriteto;
    }

    std::vector<uint8_t>::const_iterator at4px_header::ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom )
    {
        for( uint8_t & abyte : magicn )
        {
            abyte = *itReadfrom; 
            ++itReadfrom;
        }

        compressedsz = utils::ReadIntFromByteVector<decltype(compressedsz)>(itReadfrom); //iterator is incremented

        for( uint8_t & aflag : flaglist )
        {
            aflag = *itReadfrom; 
            ++itReadfrom;
        }

        decompsz = utils::ReadIntFromByteVector<decltype(decompsz)>(itReadfrom); //iterator is incremented

        return itReadfrom;
    }

//========================================================================================================
//  at4px_decompress
//========================================================================================================

    at4px_decompress::at4px_decompress( types::bytevec_t & out_decompimg )
        :m_outvec(&out_decompimg)
    {}

    at4px_decompress::at4px_decompress(  vector<types::bytevec_t>::iterator & out_itdecompimg )
        :m_outvec(nullptr),m_itOutContainers(out_itdecompimg)
    {}

    void at4px_decompress::operator()( types::constitbyte_t itindatabeg, types::constitbyte_t itindataend, bool blogenabled )
    {
        m_itInCur = itindatabeg; 
        m_itInEnd = itindataend;
        Decompress(blogenabled);
    }

    void at4px_decompress::ReadHeader()
    {
        m_itInCur = m_lastheader.ReadFromContainer( m_itInCur );
    }

    void at4px_decompress::Decompress( bool blogenabled )
    {
        //Get the header
        ReadHeader();

        //fill pxinfo
        px_info_header pxinf = AT4PXHeaderToPXinfo( m_lastheader );

        //If m_outvec == nullptr, we output to an array of bytevecs.
        // If its not null we output to a single bytevec!
        if( m_outvec == nullptr )
        {
            //resize
            m_itOutContainers->resize( pxinf.decompressedsz );

            //Decompress
            //Run the decompressor
            compression::DecompressPX( pxinf,
                                       m_itInCur,
                                       m_itInEnd,
                                       *m_itOutContainers );

            ++m_itOutContainers; //Increment the output iterator if applicable
        }
        else
        {
            //resize
            m_outvec->resize( pxinf.decompressedsz );

            //Decompress
            //Run the decompressor
            compression::DecompressPX( pxinf,
                                       m_itInCur,
                                       m_itInEnd,
                                       *m_outvec );
        }

    }

//========================================================================================================
//  at4px_compress
//========================================================================================================

    //at4px_compress::at4px_compress( vector<types::bytevec_t>::iterator itoutimgbeg, vector<types::bytevec_t>::iterator itoutimgend )
    //    :m_outvec(nullptr), m_itOutImgBeg(itoutimgbeg),m_itOutImgCur(itoutimgbeg),m_itOutImgEnd(itoutimgend), m_isUsingOutputIterators(true)
    //{
    //    m_outvec = &(*m_itOutImgBeg);
    //}


    //at4px_compress::at4px_compress( types::bytevec_t & out_compressedimg )
    //    :m_outvec(&out_compressedimg), m_isUsingOutputIterators(false)
    //{}


    //void at4px_compress::operator()( types::constitbyte_t itindatabeg, types::constitbyte_t itindataend)
    //{
    //    assert(false);
    //    if(m_outvec == nullptr)
    //    {
    //        assert(false);
    //        throw runtime_error("at4px_compress: Error: Tried to call () operator with no output images left !");
    //    }

    //    m_itInBeg = itindatabeg;
    //    m_itInCur = itindatabeg; 
    //    m_itInEnd = itindataend;
    //    Compress();

    //    //Increment our output iterator if we are using those ! And affect the value to our current output pointer!
    //    if( m_isUsingOutputIterators )
    //    {
    //        if( m_itOutImgCur != m_itOutImgEnd )
    //        {
    //            ++m_itOutImgCur;
    //            if(m_itOutImgCur != m_itOutImgEnd)
    //                m_outvec = &(*m_itOutImgCur);
    //            else
    //                m_outvec = nullptr;
    //        }
    //        else
    //            m_outvec = nullptr;
    //    }
    //}

    //void at4px_compress::WriteHeader()
    //{
    //    assert(false);
    //    m_header = PXinfoToAT4PXHeader(m_compinfo);

    //    //Write the header in the reserved slot
    //    auto itwriteat = m_outvec->begin();
    //    m_header.WriteToContainer( itwriteat );
    //}

    //void at4px_compress::Compress()
    //{
    //    //Make some space 

    //    assert(false);

    //    //Leave some space at the begining for the header!
    //    auto itwriteat = m_outvec->begin() + filetypes::at4px_header::HEADER_SZ;

    //    
    //}


    compression::px_info_header CompressToAT4PX( vector<uint8_t>::const_iterator        itinputbeg, 
                                                 vector<uint8_t>::const_iterator        itinputend, 
                                                 back_insert_iterator<vector<uint8_t> > itoutputbeg, 
                                                 ePXCompLevel                           complvl,
                                                 bool                                   bzealous,
                                                 bool                                   bdisplayProgress,
                                                 bool                                   blogenable )
    {
        compression::px_info_header pxinf;
        std::vector<uint8_t>        buffer;
        unsigned int                inputsz  = std::distance(itinputbeg, itinputend);
        auto                        itbuffer = std::back_inserter(buffer);

        //Allocate
        buffer.reserve( at4px_header::HEADER_SZ + inputsz + (inputsz / 8u) );

        ////Compress data
        pxinf = compression::CompressPX( itinputbeg, 
                                         itinputend, 
                                         itbuffer, 
                                         complvl, 
                                         bzealous, 
                                         bdisplayProgress,
                                         blogenable );

        //Write header, before the compressed data
        at4px_header headr = PXinfoToAT4PXHeader( pxinf );
        headr.WriteToContainerT( itoutputbeg );

        //Copy compression buffer
        std::copy( buffer.begin(), buffer.end(), itoutputbeg );

        return pxinf;
    }

    compression::px_info_header CompressToAT4PX( std::vector<uint8_t>::const_iterator             itinputbeg, 
                                                 std::vector<uint8_t>::const_iterator             itinputend, 
                                                 std::vector<uint8_t> &                           out_compressed,
                                                 compression::ePXCompLevel                        complvl,
                                                 bool                                             bzealous,
                                                 bool                                             bdisplayProgress,
                                                 bool                                             blogenable )
    {
        compression::px_info_header pxinf;
        std::vector<uint8_t>        buffer;
        unsigned int                inputsz  = std::distance(itinputbeg, itinputend);
        auto                        itbuffer = std::back_inserter(buffer);

        //Allocate
        buffer.reserve( at4px_header::HEADER_SZ + inputsz + (inputsz / 8u) );
        buffer.resize(at4px_header::HEADER_SZ); //Reserve the header size, to come back and write it later
        
        ////Compress data
        pxinf = compression::CompressPX( itinputbeg, 
                                         itinputend, 
                                         itbuffer, 
                                         complvl, 
                                         bzealous, 
                                         bdisplayProgress,
                                         blogenable );

        //Write header, before the compressed data
        at4px_header headr = PXinfoToAT4PXHeader( pxinf );
        headr.WriteToContainer( buffer.begin() );

        out_compressed = std::move(buffer);
        return pxinf;
    }

    /*******************************************************
        DecompressAT4PX
            Decompress an at4px file.
            Returns the size of the decompressed data!

            NOTE:
            This version needs to create an internal 
            buffer because of the way compression works.
            
            Use the version taking 2 output iterators or
            the one with the vector instead to avoid this!
    *******************************************************/
    uint16_t DecompressAT4PX( std::vector<uint8_t>::const_iterator             itinputbeg, 
                              std::vector<uint8_t>::const_iterator             itinputend, 
                              std::back_insert_iterator<std::vector<uint8_t> > itoutwhere, 
                              bool                                             bdisplayProgress,
                              bool                                             blogenable  )
    {
        at4px_header   hdr;
        itinputbeg = hdr.ReadFromContainer( itinputbeg );

        px_info_header pxinf = AT4PXHeaderToPXinfo( hdr );

        //1 - make buffer
        std::vector<uint8_t> buff(hdr.decompsz);

        //2 - decompress
        compression::DecompressPX( pxinf, itinputbeg, itinputend, buff, blogenable );

        //3 - copy buffer
        std::copy( buff.begin(), buff.end(), itoutwhere );

        return buff.size();
    }

    /*******************************************************
        DecompressAT4PX
            Decompress an at4px file.
            Returns the size of the decompressed data!

            Takes a range to output to ! 
            The output range must be as large as the
            decompressedsz specified in the header!
    *******************************************************/
    uint16_t DecompressAT4PX( std::vector<uint8_t>::const_iterator             itinputbeg, 
                              std::vector<uint8_t>::const_iterator             itinputend, 
                              std::vector<uint8_t>::iterator                   itoutputbeg, 
                              std::vector<uint8_t>::iterator                   itoutputend,
                              bool                                             bdisplayProgress,
                              bool                                             blogenable )
    {
        at4px_header   hdr;
        itinputbeg = hdr.ReadFromContainer( itinputbeg );

        px_info_header pxinf = AT4PXHeaderToPXinfo( hdr );

        //1 - check iterators
        auto outputsz = std::distance( itoutputbeg, itoutputend );

        if( outputsz > hdr.decompsz )
        {
            //move the end iterator exactly at the right place
            itoutputend = itoutputbeg; 
            std::advance( itoutputend,  hdr.decompsz );
        }
        else if( outputsz < hdr.decompsz )
        {
            throw std::length_error("DecompressAT4PX(): Not enough space to output decompressed file !");
            return 0;
        }

        //2 - decompress
        compression::DecompressPX( pxinf, itinputbeg, itinputend, itoutputbeg, itoutputend, blogenable );

        return hdr.decompsz;
    }


    /*******************************************************
        DecompressAT4PX
            Decompress an at4px file.
            Returns the size of the decompressed data!
    *******************************************************/
    uint16_t DecompressAT4PX( std::vector<uint8_t>::const_iterator             itinputbeg, 
                              std::vector<uint8_t>::const_iterator             itinputend, 
                              std::vector<uint8_t> &                           out_decompressed,
                              bool                                             bdisplayProgress,
                              bool                                             blogenable )
    {
        at4px_header   hdr;
        itinputbeg = hdr.ReadFromContainer( itinputbeg );

        px_info_header pxinf = AT4PXHeaderToPXinfo( hdr );

        //1 - make buffer
        out_decompressed.resize(hdr.decompsz);

        //2 - decompress
        compression::DecompressPX( pxinf, itinputbeg, itinputend, out_decompressed, blogenable );

        return out_decompressed.size();
    }

//========================================================================================================
//  palette_and_at4px_decompress
//========================================================================================================

    //palette_and_at4px_decompress::palette_and_at4px_decompress( types::bytevec_t & out_decompimg, graphics::rgb24palette_t & out_palette )
    //    :at4px_decompress(out_decompimg), m_outpal(out_palette)
    //{}

    ////Read the palette, and decompress the at4px right after.
    //void palette_and_at4px_decompress::operator()( types::constitbyte_t itindatabeg, types::constitbyte_t itindataend, bool benablelog )
    //{
    //    m_itInCur = itindatabeg; 
    //    m_itInEnd = itindataend;
    //    HandlePalette();
    //    Decompress(benablelog);
    //}

    ////Parse the palette to the palette vector
    //void palette_and_at4px_decompress::HandlePalette()
    //{
    //    using namespace graphics;

    //    m_outpal.resize( 16 ); //Make sure the palette is the right size 
    //    rgb15_parser palparser( m_outpal.begin() );
    //    std::for_each( m_itInCur, m_itInCur + PALETTE_15_BPC_1BPCHAN_AT4PX_SZ, palparser );

    //    std::advance( m_itInCur, PALETTE_15_BPC_1BPCHAN_AT4PX_SZ ); //Advance the iterator, by the size of the palette
    //}

//========================================================================================================
//  palette_and_at4px_decompress_pairs
//========================================================================================================
    //palette_and_at4px_decompress_struct::palette_and_at4px_decompress_struct( vector<graphics::rgb24pal_and_8bpp_tiled>::iterator itoutimgpal )
    //    :at4px_decompress(itoutimgpal->_8bpp_timg), m_itOutContainers(itoutimgpal)
    //{
    //}

    ////Read the palette, and decompress the at4px right after.
    //void palette_and_at4px_decompress_struct::operator()( types::constitbyte_t itindatabeg, types::constitbyte_t itindataend )
    //{
    //    m_itInCur = itindatabeg; 
    //    m_itInEnd = itindataend;
    //    HandlePalette();
    //    Decompress();
    //    
    //    //Set the next vector to write into. 
    //    // We expect the calling code to give us as much elements in that vector as how many times they'll call this!
    //    ++m_itOutContainers;
    //    m_outvec = &(m_itOutContainers->_8bpp_timg);
    //}

    ////Parse the palette to the palette vector
    //void palette_and_at4px_decompress_struct::HandlePalette()
    //{
    //    using namespace graphics;

    //    m_itOutContainers->_palette.resize( 16 ); //Make sure the palette is the right size 
    //    rgb15_parser palparser( m_itOutContainers->_palette.begin() );
    //    std::for_each( m_itInCur, m_itInCur + PALETTE_15_BPC_1BPCHAN_AT4PX_SZ, palparser );

    //    std::advance( m_itInCur, PALETTE_15_BPC_1BPCHAN_AT4PX_SZ ); //Advance the iterator, by the size of the palette
    //}

//========================================================================================================
//  at4px_rule
//========================================================================================================
    /*
        at4px_rule
            Rule for identifying AT4PX content. With the ContentTypeHandler!
    */
    class at4px_rule : public IContentHandlingRule
    {
    public:
        at4px_rule(){}
        ~at4px_rule(){}

        //Returns the value from the content type enum to represent what this container contains!
        virtual e_ContentType getContentType()const;

        //Returns an ID number identifying the rule. Its not the index in the storage array,
        // because rules can me added and removed during exec. Thus the need for unique IDs.
        //IDs are assigned on registration of the rule by the handler.
        virtual content_rule_id_t getRuleID()const;
        virtual void              setRuleID( content_rule_id_t id );

        //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
        //## This method will call "CContentHandler::AnalyseContent()" for each sub-content container found! ##
        //virtual ContentBlock Analyse( types::constitbyte_t   itdatabeg, 
        //                              types::constitbyte_t   itdataend );
        virtual ContentBlock Analyse( const filetypes::analysis_parameter & parameters );

        //This method is a quick boolean test to determine quickly if this content handling
        // rule matches, without in-depth analysis.
        virtual bool isMatch(  types::constitbyte_t   itdatabeg, 
                               types::constitbyte_t   itdataend,
                               const std::string & filext);

    private:
        content_rule_id_t m_myID;
    };


    //Returns the value from the content type enum to represent what this container contains!
    e_ContentType at4px_rule::getContentType()const
    {
        return e_ContentType::AT4PX_CONTAINER;
    }

    //Returns an ID number identifying the rule. Its not the index in the storage array,
    // because rules can me added and removed during exec. Thus the need for unique IDs.
    //IDs are assigned on registration of the rule by the handler.
    content_rule_id_t at4px_rule::getRuleID()const
    {
        return m_myID;
    }
    void at4px_rule::setRuleID( content_rule_id_t id )
    {
        m_myID = id;
    }

    //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
    //## This method will call "CContentHandler::AnalyseContent()" for each sub-content container found! ##
    //ContentBlock at4px_rule::Analyse( types::constitbyte_t itdatabeg, types::constitbyte_t itdataend )
    ContentBlock at4px_rule::Analyse( const filetypes::analysis_parameter & parameters )
    {
        at4px_header headr;
        ContentBlock cb;

        //Read the header
        //ReadTheAT4PXHeader( parameters._itdatabeg, headr );
        headr.ReadFromContainer( parameters._itdatabeg );

        //build our content block info
        cb._startoffset          = 0;
        cb._endoffset            = headr.compressedsz;
        cb._rule_id_that_matched = getRuleID();
        cb._type                 = getContentType();

        //AT4PX files have no sub-containers! They're always containing image data.

        return cb;
    }

    //This method is a quick boolean test to determine quickly if this content handling
    // rule matches, without in-depth analysis.
    bool at4px_rule::isMatch(  types::constitbyte_t itdatabeg, types::constitbyte_t itdataend, const std::string & filext )
    {
        using namespace magicnumbers;
        return std::equal( AT4PX_MAGIC_NUMBER.begin(), AT4PX_MAGIC_NUMBER.end(), itdatabeg );
    }

//========================================================================================================
//  at4px_rule_registrator
//========================================================================================================
    /*
        at4px_rule_registrator
            A small singleton that has for only task to register the at4px_rule!
    */
    RuleRegistrator<at4px_rule> RuleRegistrator<at4px_rule>::s_instance;
};};