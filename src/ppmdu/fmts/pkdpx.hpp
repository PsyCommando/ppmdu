#ifndef PKDPX_HPP
#define PKDPX_HPP
/*
pkdpx.hpp
2014/09/18/
psycommando@gmail.com
Description: Tools for handling PKDPX files.

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P
*/
#include <vector>
#include <cstdint>
#include <types/content_type_analyser.hpp>
#include <utils/utility.hpp>
#include <ppmdu/fmts/px_compression.hpp>


namespace filetypes
{
    static const unsigned int                              MagicNumber_PKDPX_Len = 5;
    static const std::array<uint8_t,MagicNumber_PKDPX_Len> MagicNumber_PKDPX{{ 0x50, 0x4B, 0x44, 0x50, 0x58 }}; //"PKDPX"

    //Content ID db handles.
    extern const ContentTy                                 CnTy_PKDPX;
    extern const ContentTy                                 CnTy_SIR0_PKDPX;

//==================================================================
// Structs
//==================================================================
    /*
        pkdpx_header
            Structure of the header for an PKDPX file
    */
    struct pkdpx_header /*: public utils::data_array_struct*/
    {
        static const unsigned int HEADER_SZ        = 20u;
        static const unsigned int NB_FLAGS         = 9u;
        //static const unsigned int MAGIC_NUMBER_LEN = MagicNumber_PKDPX_Len;

        std::array<uint8_t,MagicNumber_PKDPX_Len>  magicn;       //"PKDPX"
        uint16_t                              compressedsz; //The total size of the file compressed
        std::array<uint8_t,NB_FLAGS>          flaglist;     //list of flags used in the file
        uint32_t                              decompsz;     //The file size decompressed

        unsigned int size()const {return HEADER_SZ;}


        //std::vector<uint8_t>::iterator       WriteToContainer (  std::vector<uint8_t>::iterator       itwriteto  )const;
        //std::vector<uint8_t>::const_iterator ReadFromContainer(  std::vector<uint8_t>::const_iterator itReadfrom );

        //Implementations specific to pkdpx_header
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            for( const uint8_t & abyte : magicn )
            {
                *itwriteto = abyte; 
                ++itwriteto;
            }

            itwriteto = utils::WriteIntToBytes( compressedsz, itwriteto );

            for( const uint8_t & aflag : flaglist )
            {
                *itwriteto = aflag; 
                ++itwriteto;
            }

            itwriteto = utils::WriteIntToBytes( decompsz, itwriteto );

            return itwriteto;
        }

        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itPastEnd )
        {
            for( uint8_t & abyte : magicn )
            {
                abyte = utils::ReadIntFromBytes<uint8_t>(itReadfrom,itPastEnd);
                //abyte = *itReadfrom; 
                //++itReadfrom;
            }

            compressedsz = utils::ReadIntFromBytes<decltype(compressedsz)>(itReadfrom,itPastEnd); //iterator is incremented

            for( uint8_t & aflag : flaglist )
            {
                aflag = *itReadfrom; 
                ++itReadfrom;
            }

            decompsz = utils::ReadIntFromBytes<decltype(decompsz)>(itReadfrom,itPastEnd); //iterator is incremented

            return itReadfrom;
        }

        operator compression::px_info_header()const 
        {
            compression::px_info_header pxinf;
            pxinf.compressedsz   = compressedsz;
            pxinf.controlflags   = flaglist;
            pxinf.decompressedsz = decompsz;
            return pxinf;
        }

        pkdpx_header & operator=( const compression::px_info_header & other )
        {
            compressedsz = other.compressedsz;
            flaglist     = other.controlflags;
            decompsz     = other.decompressedsz;
            return *this;
        }
    };


//==================================================================
// Classes
//==================================================================

    //#TODO: Update this to be actually useful as a functor and be able to be used on data structure
    //       that support iterators!

    /*
        pkdpx_handler
            A short lived object meant to exist only for the duration of one operation.
            The arguments to the constructor are the source for either compression or
            decompression.

            After construction, one may run either Compress or Decompress. 
            But afterwards the object should be discarded.

            NOTE:
                The pkdpx object wasn't treated as a specific data container class, like other formats, 
                given data stored inside PKDPX files vary greatly! Thus, storing the decompressed data
                inside of it would have required from users more copies and processing time.
    */
    //class pkdpx_handler
    //{
    //public:

    //    //Constructor
    //    // Take the source data, and destination!
    //    pkdpx_handler( vector<uint8_t>::const_iterator itindatabeg, vector<uint8_t>::const_iterator itindataend, types::bytevec_t & out_data ) throw();

    //    //Read and decompress PKDPX header and data
    //    void Decompress( bool blogenabled = false );

    //    //Compress the data passed as input to the contructor as PKDPX format data
    //    void Compress( bool blogenabled = false ); //#TODO: Handle compression

    //private:
    //    //Methods
    //    void ReadHeader();

    //    //Vars
    //    pkdpx_header           m_lastheader;

    //    vector<uint8_t>::const_iterator   m_itInCur,
    //                           m_itInEnd;

    //    types::bytevec_t &     m_outvec;
    //    bool                   m_blogenabled;
    //};


    

//==================================================================
// Functions
//==================================================================

    /*******************************************************
        CompressToPKDPX
            Function to compress a range into another range.
            The output range must be at least 
            itinputend - itinputbeg in size !

            Also write the PKDPX header. 

            The returned px_info_header is just for 
            reference.
    *******************************************************/
    compression::px_info_header CompressToPKDPX( std::vector<uint8_t>::const_iterator             itinputbeg, 
                                                 std::vector<uint8_t>::const_iterator             itinputend, 
                                                 std::back_insert_iterator<std::vector<uint8_t> > itoutputbeg,
                                                 compression::ePXCompLevel                        complvl          = compression::ePXCompLevel::LEVEL_3,
                                                 bool                                             bzealous         = true,
                                                 bool                                             bdisplayProgress = false,
                                                 bool                                             blogenable       = false );

    compression::px_info_header CompressToPKDPX( std::vector<uint8_t>::const_iterator             itinputbeg, 
                                                 std::vector<uint8_t>::const_iterator             itinputend, 
                                                 std::vector<uint8_t> &                           out_compressed,
                                                 compression::ePXCompLevel                        complvl          = compression::ePXCompLevel::LEVEL_3,
                                                 bool                                             bzealous         = true,
                                                 bool                                             bdisplayProgress = false,
                                                 bool                                             blogenable       = false );


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
                              bool                                             bdisplayProgress = false,
                              bool                                             blogenable       = false );

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
                              bool                                             bdisplayProgress = false,
                              bool                                             blogenable       = false );


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
                              bool                                             bdisplayProgress = false,
                              bool                                             blogenable       = false );


    /*
        PKDPXHeaderToPxinfo
            Convert the pkdpx_header to a px_info_header. 
            It also subtract the pkdpx_header's length from the compressed size!
    */
    compression::px_info_header PKDPXHeaderToPXinfo( const pkdpx_header & head );


    /*
        PxinfoToPKDPXHeader
            Convert the pxinfo data to an pkdpx header. It also adds the pkdpx header
            length to the compressed data length !
    */
    pkdpx_header PXinfoToPKDPXHeader( const compression::px_info_header & pxinf );


};

#endif