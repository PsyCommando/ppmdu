#include "pkdpx.hpp"
#include <iterator>
#include <ppmdu/fmts/px_compression.hpp>
#include <types/content_type_analyser.hpp>
#include <ppmdu/fmts/sir0.hpp>
#include <utils/utility.hpp>
#include <cassert>
using namespace std;
using namespace filetypes;

//using namespace pmd2::filetypes;
using compression::px_info_header;
using namespace utils;

namespace filetypes
{
    //Content ID db handles.
    const ContentTy CnTy_PKDPX      {"pkdpx"};
    const ContentTy CnTy_SIR0_PKDPX {"sir0pkdpx"};
    //const std::array<uint8_t,5> MagicNumber_PKDPX {{ 0x50, 0x4B, 0x44, 0x50, 0x58 }};

//==================================================================
// Utility:
//==================================================================
    px_info_header PKDPXHeaderToPXinfo( const pkdpx_header & head )
    {
        px_info_header px;
        px.compressedsz   = head.compressedsz - pkdpx_header::HEADER_SZ;
        px.decompressedsz = head.decompsz;
        std::copy_n( head.flaglist.begin(), px_info_header::NB_FLAGS, px.controlflags.begin() );
        return px;
    }

    pkdpx_header PXinfoToPKDPXHeader( const px_info_header & pxinf )
    {
        pkdpx_header head;

        head.magicn       = MagicNumber_PKDPX;
        head.compressedsz = pxinf.compressedsz + pkdpx_header::HEADER_SZ;
        head.flaglist     = pxinf.controlflags;
        head.decompsz     = pxinf.decompressedsz;

        return head;
    }

//
//pkdpx_header
//
    //uint8_t& pkdpx_header::operator[]( unsigned int index )
    //{
    //    if(index < 5)
    //        return magicn[index];
    //    else if( index < 7 )
    //        return reinterpret_cast<uint8_t*>(&compressedsz)[(index-5)]; //lol
    //    else if( index < 16 )
    //        return flaglist[(index-7)];
    //    else if( index < HEADER_SZ )
    //        return reinterpret_cast<uint8_t*>(&decompsz)[(index-16)]; //lol
    //    else
    //        return *((uint8_t*)0); //out of bound, derefence a pointer to 0 to simulate out-of-bound error
    //}

    //const uint8_t & pkdpx_header::operator[](unsigned int index)const
    //{
    //    return (*const_cast<pkdpx_header*>(this))[index];
    //}

    //std::vector<uint8_t>::iterator pkdpx_header::WriteToContainer( std::vector<uint8_t>::iterator itwriteto )const
    //{
    //    for( const uint8_t & abyte : magicn )
    //    {
    //        *itwriteto = abyte; 
    //        ++itwriteto;
    //    }

    //    itwriteto = utils::WriteIntToBytes( compressedsz, itwriteto );

    //    for( const uint8_t & aflag : flaglist )
    //    {
    //        *itwriteto = aflag; 
    //        ++itwriteto;
    //    }

    //    itwriteto = utils::WriteIntToBytes( decompsz, itwriteto );

    //    return itwriteto;
    //}

    //std::vector<uint8_t>::const_iterator pkdpx_header::ReadFromContainer(  std::vector<uint8_t>::const_iterator itReadfrom )
    //{
    //    for( uint8_t & abyte : magicn )
    //    {
    //        abyte = *itReadfrom; 
    //        ++itReadfrom;
    //    }

    //    compressedsz = utils::ReadIntFromBytes<decltype(compressedsz)>(itReadfrom); //iterator is incremented

    //    for( uint8_t & aflag : flaglist )
    //    {
    //        aflag = *itReadfrom; 
    //        ++itReadfrom;
    //    }

    //    decompsz = utils::ReadIntFromBytes<decltype(decompsz)>(itReadfrom); //iterator is incremented

    //    return itReadfrom;
    //}
   
//==================================================================
// PKDPX Handler
//==================================================================
    //pkdpx_handler::pkdpx_handler( constitbyte_t itindatabeg, constitbyte_t itindataend, bytevec_t & out_data )
    //    :m_outvec(out_data),m_itInCur(itindatabeg),m_itInEnd(itindataend)
    //{}

    //void pkdpx_handler::ReadHeader()
    //{
    //    //for( unsigned int i = 0; i < pkdpx_header::HEADER_SZ; ++i, ++m_itInCur )
    //    //    m_lastheader[i] = (*m_itInCur);
    //    m_itInCur = m_lastheader.ReadFromContainer( m_itInCur );
    //}

    //void pkdpx_handler::Decompress( bool blogenabled )
    //{
    //    
    //    //Get the header
    //    ReadHeader();

    //    //Fill a pxinfo struct
    //    px_info_header pxinf = PKDPXHeaderToPXinfo( m_lastheader );

    //    //resize
    //    m_outvec.resize(pxinf.decompressedsz);

    //    //Run the decompressor
    //    compression::DecompressPX( pxinf,
    //                               m_itInCur,
    //                               m_itInEnd,
    //                               m_outvec,
    //                               blogenabled );
    //}

    //void pkdpx_handler::Compress( bool blogenabled )
    //{
    //    assert(false);
    //    //#TODO : Handle compression!
    //    throw std::exception("Not implemented");
    //}


//==================================================================
// PKDPX Compress Functions
//==================================================================
    /*******************************************************
        CompressToPKDPX
            Function to compress a range into another range.
            The output range must be at least 
            itinputend - itinputbeg in size !
    *******************************************************/
    compression::px_info_header CompressToPKDPX( std::vector<uint8_t>::const_iterator             itinputbeg, 
                                                 std::vector<uint8_t>::const_iterator             itinputend, 
                                                 std::back_insert_iterator<std::vector<uint8_t> > itoutputbeg,
                                                 compression::ePXCompLevel                        complvl,
                                                 bool                                             bzealous,
                                                 bool                                             bdisplayProgress,
                                                 bool                                             blogenable )
    {
        px_info_header  pxinf;
        vector<uint8_t> buffer;
        unsigned int    inputsz  = std::distance(itinputbeg, itinputend);
        auto            itbuffer = std::back_inserter(buffer);

        //Allocate
        buffer.reserve( pkdpx_header::HEADER_SZ + inputsz + (inputsz / 8u) );
        
        //Run compression
        //Compress data
        pxinf = compression::CompressPX( itinputbeg, 
                                         itinputend, 
                                         itoutputbeg, 
                                         complvl, 
                                         bzealous, 
                                         bdisplayProgress,
                                         blogenable );

        //Write header, before the compressed data
        pkdpx_header headr = PXinfoToPKDPXHeader( pxinf );
        headr.WriteToContainer( itoutputbeg );

        //Copy compression buffer
        std::copy( buffer.begin(), buffer.end(), itoutputbeg );

        return pxinf;
    }


    compression::px_info_header CompressToPKDPX( std::vector<uint8_t>::const_iterator             itinputbeg, 
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
        buffer.reserve( pkdpx_header::HEADER_SZ + inputsz + (inputsz / 8u) );
        buffer.resize(pkdpx_header::HEADER_SZ); //Reserve the header size, to come back and write it later
        
        //Compress data
        pxinf = compression::CompressPX( itinputbeg, 
                                         itinputend, 
                                         itbuffer, 
                                         complvl, 
                                         bzealous, 
                                         bdisplayProgress,
                                         blogenable );

        //Write header, before the compressed data
        pkdpx_header headr = PXinfoToPKDPXHeader( pxinf );
        headr.WriteToContainer( buffer.begin() );

        out_compressed = std::move(buffer);
        return pxinf;
    }

    /*******************************************************
        DecompressPKDPX
            Decompress an PKDPX file.
            Returns the size of the decompressed data!

            Params:
                - itinputbeg : beginning of the PX compressed data, right BEFORE the PKDPX header!

            NOTE:
            This version needs to create an internal 
            buffer because of the way compression works.
            
            Use the version taking 2 output iterators or
            the one with the vector instead to avoid this!
    *******************************************************/
    uint16_t DecompressPKDPX( std::vector<uint8_t>::const_iterator             itinputbeg, 
                              std::vector<uint8_t>::const_iterator             itinputend, 
                              std::back_insert_iterator<std::vector<uint8_t> > itoutwhere, 
                              bool                                             bdisplayProgres,
                              bool                                             blogenable )
    {
        //Get header
        pkdpx_header myhdr;
        itinputbeg = myhdr.ReadFromContainer( itinputbeg, itinputend );
        px_info_header pxinf = PKDPXHeaderToPXinfo( myhdr );

        //1 - make buffer
        vector<uint8_t> buffer;
        buffer.reserve( pxinf.decompressedsz );

        //2 - decompress
        compression::DecompressPX( pxinf,
                                   itinputbeg,
                                   itinputend,
                                   buffer,
                                   blogenable );

        //3 - copy buffer
        std::copy( buffer.begin(), buffer.end(), itoutwhere );

        return static_cast<uint16_t>(buffer.size());
    }

    /*******************************************************
        DecompressPKDPX
            Decompress an PKDPX file.
            Returns the size of the decompressed data!

            Params:
                - itinputbeg : beginning of the PX compressed data, right BEFORE the PKDPX header!

            Takes a range to output to ! 
            The output range must be as large as the
            decompressedsz specified in the header!
    *******************************************************/
    uint16_t DecompressPKDPX( std::vector<uint8_t>::const_iterator             itinputbeg, 
                              std::vector<uint8_t>::const_iterator             itinputend, 
                              std::vector<uint8_t>::iterator                   itoutputbeg, 
                              std::vector<uint8_t>::iterator                   itoutputend,
                              bool                                             bdisplayProgress,
                              bool                                             blogenable )
    {
        pkdpx_header   hdr;
        itinputbeg = hdr.ReadFromContainer( itinputbeg, itinputend );

        px_info_header pxinf = PKDPXHeaderToPXinfo( hdr );

        //1 - check iterators
        unsigned int outputsz = std::distance( itoutputbeg, itoutputend );

        if( outputsz > hdr.decompsz )
        {
            //move the end iterator exactly at the right place
            itoutputend = itoutputbeg; 
            std::advance( itoutputend,  hdr.decompsz );
        }
        else if( outputsz < hdr.decompsz )
        {
            throw std::length_error("DecompressPKDPX(): Not enough space to output decompressed file !");
            return 0;
        }

        //2 - decompress
        compression::DecompressPX( pxinf, itinputbeg, itinputend, itoutputbeg, itoutputend, blogenable );

        return hdr.decompsz;
    }


    /*******************************************************
        DecompressPKDPX
            Decompress a PKDPX file.
            Returns the size of the decompressed data!

            Params:
                - itinputbeg : beginning of the PX compressed data, right BEFORE the PKDPX header!
    *******************************************************/
    uint16_t DecompressPKDPX( std::vector<uint8_t>::const_iterator             itinputbeg, 
                              std::vector<uint8_t>::const_iterator             itinputend, 
                              std::vector<uint8_t> &                           out_decompressed,
                              bool                                             bdisplayProgress,
                              bool                                             blogenable )
    {
        pkdpx_header   hdr;
        itinputbeg = hdr.ReadFromContainer( itinputbeg, itinputend );

        px_info_header pxinf = PKDPXHeaderToPXinfo( hdr );

        //1 - make buffer
        out_decompressed.resize(hdr.decompsz);

        //2 - decompress
        compression::DecompressPX( pxinf, itinputbeg, itinputend, out_decompressed, blogenable );

        return static_cast<uint16_t>(out_decompressed.size());
    }

//========================================================================================================
//  pkdpx_rule
//========================================================================================================
    /*
        pkdpx_rule
            Rule for identifying PKDPX content. With the ContentTypeHandler!
    */
    class pkdpx_rule : public IContentHandlingRule
    {
    public:
        pkdpx_rule(){}
        ~pkdpx_rule(){}

        //Returns the value from the content type enum to represent what this container contains!
        virtual cnt_t getContentType()const;

        //Returns an ID number identifying the rule. Its not the index in the storage array,
        // because rules can me added and removed during exec. Thus the need for unique IDs.
        //IDs are assigned on registration of the rule by the handler.
        virtual cntRID_t getRuleID()const;
        virtual void              setRuleID( cntRID_t id );

        //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
        //## This method will call "CContentHandler::AnalyseContent()" for each sub-content container found! ##
        //virtual ContentBlock Analyse( vector<uint8_t>::const_iterator   itdatabeg, 
        //                              vector<uint8_t>::const_iterator   itdataend );
        virtual ContentBlock Analyse( const analysis_parameter & parameters );

        //This method is a quick boolean test to determine quickly if this content handling
        // rule matches, without in-depth analysis.
        virtual bool isMatch(  vector<uint8_t>::const_iterator   itdatabeg, 
                               vector<uint8_t>::const_iterator   itdataend,
                               const std::string    & filext );

    private:
        cntRID_t m_myID;
    };


    //Returns the value from the content type enum to represent what this container contains!
    cnt_t pkdpx_rule::getContentType()const
    {
        return CnTy_PKDPX;
    }

    //Returns an ID number identifying the rule. Its not the index in the storage array,
    // because rules can me added and removed during exec. Thus the need for unique IDs.
    //IDs are assigned on registration of the rule by the handler.
    cntRID_t pkdpx_rule::getRuleID()const
    {
        return m_myID;
    }
    void pkdpx_rule::setRuleID( cntRID_t id )
    {
        m_myID = id;
    }

    //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
    //## This method will call "CContentHandler::AnalyseContent()" for each sub-content container found! ##
    //ContentBlock pkdpx_rule::Analyse( vector<uint8_t>::const_iterator itdatabeg, vector<uint8_t>::const_iterator itdataend )
    ContentBlock pkdpx_rule::Analyse( const analysis_parameter & parameters )
    {
        pkdpx_header headr;
        ContentBlock cb;
        auto itdatabeg = parameters._itdatabeg;

        //Read the header
        headr.ReadFromContainer( itdatabeg, parameters._itdataend );

        //build our content block info
        cb._startoffset          = 0;
        cb._endoffset            = headr.compressedsz;
        cb._rule_id_that_matched = getRuleID();
        cb._type                 = getContentType();

        //Handle sub-containers!
        //Since we don't want to decompress everything just to tell what it is
        // we'll tag it as "CompressedData"
        //cb._hierarchy.push_back( ContentBlock( static_cast<unsigned int>(e_ContentType::COMPRESSED_DATA), headr.HEADER_SZ, cb._endoffset ) );

        return cb;
    }

    //This method is a quick boolean test to determine quickly if this content handling
    // rule matches, without in-depth analysis.
    bool pkdpx_rule::isMatch(  vector<uint8_t>::const_iterator itdatabeg, vector<uint8_t>::const_iterator itdataend,const std::string & filext )
    {
        return std::equal( MagicNumber_PKDPX.begin(), MagicNumber_PKDPX.end(), itdatabeg );
    }


    /*
        sir0pkdpx_rule
            Rule for identifying PKDPX content wrapped by a SIR0 container. With the ContentTypeHandler!
    */
    class sir0pkdpx_rule : public IContentHandlingRule
    {
    public:
        sir0pkdpx_rule(){}
        ~sir0pkdpx_rule(){}

        //Returns the value from the content type enum to represent what this container contains!
        virtual cnt_t getContentType()const
        {
            return CnTy_SIR0_PKDPX;
        }

        //Returns an ID number identifying the rule. Its not the index in the storage array,
        // because rules can me added and removed during exec. Thus the need for unique IDs.
        //IDs are assigned on registration of the rule by the handler.
        virtual cntRID_t getRuleID()const
        {
            return m_myID;
        }
        virtual void              setRuleID( cntRID_t id )
        {
            m_myID = id;
        }

        //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
        virtual ContentBlock Analyse( const analysis_parameter & parameters )
        {
            //#TODO: Seriously get rid of this method, its completely useless...
            sir0_header  sir0hdr; 
            pkdpx_header headr;
            ContentBlock cb;
            auto         itdatabeg = parameters._itdatabeg;

            //Read the header
            sir0hdr.ReadFromContainer( itdatabeg, parameters._itdataend );
            headr.ReadFromContainer( (parameters._itdatabeg + sir0hdr.subheaderptr), parameters._itdataend );

            //build our content block info
            cb._startoffset          = 0;
            cb._endoffset            = std::distance( parameters._itdatabeg, parameters._itdataend );
            cb._rule_id_that_matched = getRuleID();
            cb._type                 = getContentType();

            //Handle sub-containers!
            //Since we don't want to decompress everything just to tell what it is
            // we'll tag it as "CompressedData"
            //cb._hierarchy.push_back( ContentBlock( e_ContentType::COMPRESSED_DATA, headr.HEADER_SZ, cb._endoffset ) );

            return cb;
        }

        //This method is a quick boolean test to determine quickly if this content handling
        // rule matches, without in-depth analysis.
        virtual bool isMatch(  vector<uint8_t>::const_iterator   itdatabeg, 
                               vector<uint8_t>::const_iterator   itdataend,
                               const std::string    & filext )
        {

            sir0_header  mysir0hdr;
            pkdpx_header mypkdpxhdr;
            try
            {
                mysir0hdr.ReadFromContainer( itdatabeg, itdataend );
                if( mysir0hdr.magic == MagicNumber_SIR0 )
                {
                    mypkdpxhdr.ReadFromContainer( (itdatabeg + mysir0hdr.subheaderptr), itdataend );
                    return std::equal( MagicNumber_PKDPX.begin(), MagicNumber_PKDPX.end(), mypkdpxhdr.magicn.begin() );
                }
            }
            catch(...)
            {
                return false;
            }
            return false;
        }

    private:
        cntRID_t m_myID;
    };

//========================================================================================================
//  pkdpx_rule_registrator
//========================================================================================================
    /*
        pkdpx_rule_registrator
            A small singleton that has for only task to register the pkdpx_rule!
    */
    RuleRegistrator<pkdpx_rule>         RuleRegistrator<pkdpx_rule>        ::s_instance;
    SIR0RuleRegistrator<sir0pkdpx_rule> SIR0RuleRegistrator<sir0pkdpx_rule>::s_instance;

};