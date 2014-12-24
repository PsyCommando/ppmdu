#ifndef AT4PX_HPP
#define AT4PX_HPP
/*
at4px.hpp
2014/09/14
psycommando@gmail.com
Description: stuff for handling and building AT4PX files.
*/
#include <ppmdu/basetypes.hpp>
#include <ppmdu/fmts/content_type_analyser.hpp>
#include <ppmdu/pmd2/pmd2_palettes.hpp>
#include <ppmdu/pmd2/pmd2_image_formats.hpp>
#include <ppmdu/utils/utility.hpp>
#include <ppmdu/fmts/px_compression.hpp>
#include <array>


namespace pmd2{ namespace filetypes
{

//==================================================================
// Structs
//==================================================================

    /*******************************************************
        at4px_header
        Structure of the header for an AT4PX file
    *******************************************************/
    struct at4px_header : public utils::data_array_struct
    {
        static const unsigned int HEADER_SZ        = 18u;
        static const unsigned int NB_FLAGS         = 9u;
        static const unsigned int MAGIC_NUMBER_LEN = 5u;

        std::array<uint8_t,MAGIC_NUMBER_LEN>  magicn;       //"AT4PX"
        uint16_t                              compressedsz; //The total size of the file compressed
        std::array<uint8_t,NB_FLAGS>          flaglist;     //list of flags used in the file
        uint16_t                              decompsz;    //The image size when decompressed as 4 bpp

        //Overrides
        unsigned int                         size()const {return HEADER_SZ;}
        std::vector<uint8_t>::iterator       WriteToContainer( std::vector<uint8_t>::iterator       itwriteto )const;
        std::vector<uint8_t>::const_iterator ReadFromContainer(  std::vector<uint8_t>::const_iterator itReadfrom );

        //Implementations specific to at4px_header
        template<class _outit>
            _outit WriteToContainerT( _outit itwriteto )const
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

        template<class _init>
            _init ReadFromContainerT(  _init itReadfrom )
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
    };

//==================================================================
// Functions
//==================================================================

    /*
        AT4PXHeaderToPXinfo
            Convert the at4px_header to a px_info_header. 
            It also substract the at4px_header's length from the compressed size !
    */
    compression::px_info_header AT4PXHeaderToPXinfo( const at4px_header & head );

    /*
        PXinfoToAT4PXHeader
            Convert the pxinfo data to an at4px header. It also adds the at4px header
            length to the compressed data length !
    */
    at4px_header PXinfoToAT4PXHeader( const compression::px_info_header & pxinf );

//==================================================================
// Classes
//==================================================================

    //#TODO: Think of something to make those, actually usable as functors..
    //       The main issue right now is that because some AT4PX come with a palette
    //       before their header, the best way to pass AT4PX with good perf, is 
    //       by using iterators. But, to use a functor that takes an iterator in a 
    //       for_each loop, we'd need to build an array of them first hand.. Which
    //       isn't such a great idea performance wise.. 
    //       Not to mention, we'd need to actually put pairs of both the AT4PX start
    //       iterator and end iterator into that vector..

    /*******************************************************
        at4px_decompress
            Functor for handling at4px decompression!
    *******************************************************/
    class at4px_decompress
    {
    public:

        //With this constructor only a single call to the class' 
        // () operator will work. Any subsequent calls will 
        // overwrite the content of the vector with the new
        // decompressed output! The byte vector will
        // have its size adjusted accordingly to the decompressend output
        // size !
        at4px_decompress( types::bytevec_t & out_decompimg );

        //With this constructor, the functor will output the result of each calls
        // to its () operator to a new entry in the vector. 
        // The topmost vector must have the correct size! The byte vectors will
        // have their sizes adjusted accordingly to the decompressend output
        // size !
        at4px_decompress( std::vector<types::bytevec_t>::iterator & out_itdecompimg );

        //Read and decompress AT4PX header and data
        // **If a vector of bytevec was passed as parameter, we expect the
        // calling code to give us as much elements in the output vector 
        // as how many times they'll call this method!**
        // If a single bytevec was passed, this method will simply overwrite 
        // the content the bytevec!
        void operator()( types::constitbyte_t itindatabeg, types::constitbyte_t itindataend, bool blogenabled );

    protected:
        //Read and decompress AT4PX header and data
        void Decompress( bool blogenabled );
        //Methods
        void ReadHeader();

        at4px_decompress( const at4px_decompress& ); //no copy
        at4px_decompress& operator=( const at4px_decompress& ); //no copy

        //Vars
        at4px_header                            m_lastheader;
        types::constitbyte_t                    m_itInCur,
                                                m_itInEnd;

        types::bytevec_t                      * m_outvec;
        std::vector<types::bytevec_t>::iterator m_itOutContainers;
    };


    /*******************************************************
        at4px_compress
            Functor for handling at4px compression!
    *******************************************************/
    //class at4px_compress
    //{
    //public:
    //    at4px_compress( types::bytevec_t & out_compressedimg );
    //    at4px_compress( std::vector<types::bytevec_t>::iterator itoutimgbeg, std::vector<types::bytevec_t>::iterator itoutimgend );

    //    //Compress the data passed as input to the contructor as AT4PX format data
    //    void operator()( types::constitbyte_t itindatabeg, types::constitbyte_t itindataend );

    //protected:
    //    //Compress the data passed as input to the contructor as AT4PX format data
    //    void Compress();

    //    //Make the header
    //    void WriteHeader();

    //    //Vars
    //    at4px_header                m_header;
    //    compression::px_info_header m_compinfo;
    //    types::constitbyte_t        m_itInBeg,
    //                                m_itInCur,
    //                                m_itInEnd;

    //    //Ouput iterators
    //    bool                               m_isUsingOutputIterators; //Whether we need to use the output iterators
    //    std::vector<types::bytevec_t>::iterator m_itOutImgBeg,
    //                                            m_itOutImgCur,
    //                                            m_itOutImgEnd;

    //    types::bytevec_t * m_outvec;

    //};


    //template<class _init, class _randit>
    //    compression::px_info_header CompressToAT4PX( _init itinputbeg, _init itinputend, _randit itoutputbeg, _randit itouputend )
    //{
    //    compression::px_info_header pxinf;
    //    //compress data
    //    auto itwriteat = itoutputbeg + at4px_header::HEADER_SZ;
    //    pxinf = compression::CompressPX( itinputbeg, itinputend, itwriteat, itouputend, compression::ePXCompLevel::LEVEL_3, true );

    //    //write header, before the compressed data
    //    PXinfoToAT4PXHeader( pxinf ).WriteToContainerT( itoutputbeg );
    //    return pxinf;
    //}
    //template<class _init, class _randit>
    //    compression::px_info_header CompressToAT4PX( _init itinputbeg, _init itinputend, _randit itoutputbeg, _randit itouputend )
    //{
    //    static_assert(false, "Not supported yet !");
    //}

    /*******************************************************
        CompressToAT4PX
            Function to compress a range into another range.
            The output range must be at least 
            itinputend - itinputbeg in size !
    *******************************************************/
    compression::px_info_header CompressToAT4PX( std::vector<uint8_t>::const_iterator             itinputbeg, 
                                                 std::vector<uint8_t>::const_iterator             itinputend, 
                                                 std::back_insert_iterator<std::vector<uint8_t> > itoutputbeg,
                                                 compression::ePXCompLevel                        complvl          = compression::ePXCompLevel::LEVEL_3,
                                                 bool                                             bzealous         = true,
                                                 bool                                             bdisplayProgress = false,
                                                 bool                                             blogenable       = false );

    compression::px_info_header CompressToAT4PX( std::vector<uint8_t>::const_iterator             itinputbeg, 
                                                 std::vector<uint8_t>::const_iterator             itinputend, 
                                                 std::vector<uint8_t> &                           out_compressed,
                                                 compression::ePXCompLevel                        complvl          = compression::ePXCompLevel::LEVEL_3,
                                                 bool                                             bzealous         = true,
                                                 bool                                             bdisplayProgress = false,
                                                 bool                                             blogenable       = false );


    /*******************************************************
        DecompressAT4PX
            Decompress an at4px file.
            Returns the size of the decompressed data!

            Params:
                - itinputbeg : beginning of the PX compressed data, right BEFORE the at4px header!

            NOTE:
            This version needs to create an internal 
            buffer because of the way compression works.
            
            Use the version taking 2 output iterators or
            the one with the vector instead to avoid this!
    *******************************************************/
    uint16_t DecompressAT4PX( std::vector<uint8_t>::const_iterator             itinputbeg, 
                              std::vector<uint8_t>::const_iterator             itinputend, 
                              std::back_insert_iterator<std::vector<uint8_t> > itoutwhere, 
                              bool                                             bdisplayProgress = false,
                              bool                                             blogenable       = false );

    /*******************************************************
        DecompressAT4PX
            Decompress an at4px file.
            Returns the size of the decompressed data!

            Params:
                - itinputbeg : beginning of the PX compressed data, right BEFORE the at4px header!

            Takes a range to output to ! 
            The output range must be as large as the
            decompressedsz specified in the header!
    *******************************************************/
    uint16_t DecompressAT4PX( std::vector<uint8_t>::const_iterator             itinputbeg, 
                              std::vector<uint8_t>::const_iterator             itinputend, 
                              std::vector<uint8_t>::iterator                   itoutputbeg, 
                              std::vector<uint8_t>::iterator                   itoutputend,
                              bool                                             bdisplayProgress = false,
                              bool                                             blogenable       = false );


    /*******************************************************
        DecompressAT4PX
            Decompress an at4px file.
            Returns the size of the decompressed data!

            Params:
                - itinputbeg : beginning of the PX compressed data, right BEFORE the at4px header!
    *******************************************************/
    uint16_t DecompressAT4PX( std::vector<uint8_t>::const_iterator             itinputbeg, 
                              std::vector<uint8_t>::const_iterator             itinputend, 
                              std::vector<uint8_t> &                           out_decompressed,
                              bool                                             bdisplayProgress = false,
                              bool                                             blogenable       = false );


    /*******************************************************
        palette_and_at4px_decompress
            Handles the decompression of a 3 bytes per color 
            16 color palette, followed by an AT4PX
            container. Mainly used with the "kaomado.kao" file.

            The image format returned is 8bpp
    *******************************************************/
    //class palette_and_at4px_decompress : public at4px_decompress
    //{
    //public:

    //    //With this constructor only a single call to the class' 
    //    // () operator will work. Any subsequent calls will 
    //    // overwrite the content of the vector with the new
    //    // decompressed output! The byte vector will
    //    // have its size adjusted accordingly to the decompressend output
    //    // size !
    //    palette_and_at4px_decompress( types::bytevec_t & out_decompimg, graphics::rgb24palette_t & out_palette );

    //    //Read the palette, and decompress the at4px right after.
    //    void operator()( types::constitbyte_t itindatabeg, types::constitbyte_t itindataend, bool benablelog );

    //protected:

    //    //Parse the palette to the palette vector
    //    void HandlePalette();

    //    graphics::rgb24palette_t & m_outpal;
    //};

    /*******************************************************
        palette_and_at4px_decompress_struct
            The same functor as above, but it takes a vector
            of rgb24pal_and_8bpp_tiled instead
            of a single byte vector and a single palette.
    *******************************************************/
    //class palette_and_at4px_decompress_struct : public at4px_decompress
    //{
    //public:

    //    //With this constructor, the functor will output the result of each calls
    //    // to its () operator to a new entry in the vector. 
    //    // The topmost vector must have the correct size! The byte and palette vectors will
    //    // have their sizes adjusted accordingly to the decompressend output
    //    // size !
    //    palette_and_at4px_decompress_struct( std::vector<graphics::rgb24pal_and_8bpp_tiled>::iterator itoutimgpal );

    //    //Read the palette, and decompress the at4px right after.
    //    // We expect the calling code to give us as much elements in the output vector 
    //    // as how many times they'll call this!
    //    void operator()( types::constitbyte_t itindatabeg, types::constitbyte_t itindataend );

    //    std::vector<graphics::rgb24pal_and_8bpp_tiled>::iterator GetCurrentOutputPos()const
    //    {
    //        return m_itOutContainers;
    //    }

    //protected:

    //    //Parse the palette to the palette vector
    //    void HandlePalette();

    //    std::vector<graphics::rgb24pal_and_8bpp_tiled>::iterator m_itOutContainers;
    //};


};};

#endif