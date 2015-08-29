#ifndef PX_COMPRESSION_HPP
#define PX_COMPRESSION_HPP
/*
px_compression.hpp
2014/09/17
psycommando@gmail.com
Description: Utilities for dealing with what we've dubbed "PX" compression. A custom
             compression format used on several files used by the PMD2 and PMD1 games.

             Thanks to Zhorken for reversing most of the format!
             https://github.com/Zhorken
*/
#include <ppmdu/basetypes.hpp>


namespace compression
{

//=========================================
// Constants
//=========================================
    static const unsigned int PX_MINIMUM_COMPRESSED_SIZE   = 9u; //bytes
    static const unsigned int PX_NB_POSSIBLE_SEQUENCES_LEN = 7u; //The amount of possible lengths a sequence to lookup can have, considering
                                                                 // there are 9 ctrl flags, and only 0 to 15 as range to contain all that info!
                                                                 // 9 + 7 = 16
    static const std::string PX_COMPRESSION_LOGFILE_NAME   = "./px_compression.log";
    static const std::string PX_DECOMPRESSION_LOGFILE_NAME = "./px_decompression.log";


    enum struct ePXCompLevel : unsigned int
    {
        LEVEL_0,    // No compression     - All command bytes are 0xFF, and values are stored uncompressed. Filesize is actually increased !
        LEVEL_1,    // Low compression    - We handle 4 byte patterns, using only ctrl flag 0 
        LEVEL_2,    // Medium compression - We handle 4 byte patterns, using all control flags
        LEVEL_3,    // Full compression   - We handle everything above, along with repeating sequences of bytes already decompressed.
    };

//=========================================
// Structs
//=========================================
    /*
        px_info_header
            Structure used to contain some critical details needed to decompress the compressed data.
            Namely:
            - The compressed size
            - The control flags
            - The decompressed size
            Also used for building the header of PX compressed data !
    */
    struct px_info_header
    {
        static const unsigned int     NB_FLAGS = 9; 
        uint16_t                      compressedsz;
        std::array<uint8_t,NB_FLAGS>  controlflags;
        uint32_t                      decompressedsz;
    };

//=========================================
// Classes
//=========================================

    /*
        ExCompressedSzOverflow
            Exception thrown when the data's expected compressed size 
            exceeds the capacity of the uint16 that contains it in PX
            compressed files headers!
    */
    struct ExCompressedSzOverflow : public std::length_error
    { 
        ExCompressedSzOverflow(const std::string & s ):std::length_error(s){} 

        static void throwme( uint32_t sizegot );
    };

    /*
        ExDecompressedSzOverflow
            Exception thrown when the data's decompressed size 
            exceeds the capacity of the uint32 that contains it in PX
            compressed files headers!
    */
    struct ExDecompressedSzOverflow : public std::length_error
    { 
        ExDecompressedSzOverflow(const std::string & s ):std::length_error(s){} 

        static void throwme( uint64_t sizegot );
    };

//=========================================
// Functions
//=========================================
    /*
        DecompressPX
            Function used to decompress PX compressed data.

            Parameters:
                - info                 : A filled px_info_header struct to properly do its job!
                - itdatabeg            : An iterator positioned right after the AT4PX/PKDPX header of the compressed data.
                - itdataend            : An iterator after the end of the input data vector.
                - out_decompresseddata : A vector where to output the decompressed data. 
                                         It needs to be the same size specified in the "info" struct as decompressedsz.
                                         It needs to have the method size().
                - blogenabled          : Whether a log for the decompression should be written.

            It may throw exceptions.
    */
    //template<class _init, class _outstdcontainer>
    //    void DecompressPX( px_info_header             info, 
    //                       _init                      itdatabeg, 
    //                       _init                      itdataend, 
    //                       _outstdcontainer         & out_decompresseddata,
    //                       bool                       blogenabled = false );
    void DecompressPX( px_info_header                         info, 
                       std::vector<uint8_t>::const_iterator   itdatabeg, 
                       std::vector<uint8_t>::const_iterator   itdataend, 
                       std::vector<uint8_t>                 & out_decompresseddata,
                       bool                                   blogenabled = false );

    void DecompressPX( px_info_header                         info, 
                       std::vector<uint8_t>::const_iterator   itdatabeg, 
                       std::vector<uint8_t>::const_iterator   itdataend, 
                       std::vector<uint8_t>::iterator         itoutbeg, 
                       std::vector<uint8_t>::iterator         itoutend, 
                       bool                                   blogenabled = false );

    /*
        CompressPX
            Function used to compress data into PX compressed data.

            Parameters:
                - itdatabeg           : An iterator positioned at the start of the data to compress.
                - itdataend           : An iterator after the end of the data to compress.
                - out_compresseddata  : A vector where to output the compressed data. It will be resized.
                - compressionlvl      : The compression level to apply. Mostly here for research purpose.
                - bZealousSearch      : Whether to prioritize compression efficiency over speed basically..
                - displayprogress     : Whether should display the progress at the console!
                - blogenabled          : Whether a log for the compression should be written.

            Returns:
                A px_info_header structure with details about this specific compression. 
                This struct is essential when exporting the compressed data to a AT4PX or PKDPX file!

            It may throw exceptions.
    */
    px_info_header CompressPX( std::vector<uint8_t>::const_iterator   itdatabeg,
                               std::vector<uint8_t>::const_iterator   itdataend,
                               std::vector<uint8_t>                 & out_compresseddata,
                               ePXCompLevel                           compressionlvl  = ePXCompLevel::LEVEL_3,
                               bool                                   bZealousSearch  = false,
                               bool                                   displayprogress = true,
                               bool                                   blogenabled     = false );

    //template<class _init, class _randit>
    //    px_info_header CompressPX( _init        itdatabeg,
    //                               _init        itdataend,
    //                               _randit      itoutbeg,
    //                               _randit      itoutend,
    //                               ePXCompLevel compressionlvl  = ePXCompLevel::LEVEL_3,
    //                               bool         bZealousSearch  = false,
    //                               bool         displayprogress = true,
    //                               bool         blogenabled     = false )
    //    {
    //        static_assert( false, "Not supported yet!" );
    //    }


    //px_info_header CompressPX( std::vector<uint8_t>::const_iterator itdatabeg,
    //                           std::vector<uint8_t>::const_iterator itdataend,
    //                           std::vector<uint8_t>::iterator       itoutbeg,
    //                           ePXCompLevel compressionlvl  = ePXCompLevel::LEVEL_3,
    //                           bool         bZealousSearch  = false,
    //                           bool         displayprogress = true,
    //                           bool         blogenabled     = false );


    px_info_header CompressPX( std::vector<uint8_t>::const_iterator            itdatabeg,
                               std::vector<uint8_t>::const_iterator            itdataend,
                               std::back_insert_iterator<std::vector<uint8_t>> itoutbeg,
                               ePXCompLevel                                    compressionlvl  = ePXCompLevel::LEVEL_3,
                               bool                                            bZealousSearch  = false,
                               bool                                            displayprogress = true,
                               bool                                            blogenabled     = false );



    /*
        CleanExistingCompressionLogs
            A little utility function to delete the existing logs. 
            Handy to remove logs from a previous session, and avoid them getting 
            very large in size !
    */
    void CleanExistingCompressionLogs();

};

//Include the definition so that templates can properly instantiate 
//#include "px_compression.cpp"

#endif