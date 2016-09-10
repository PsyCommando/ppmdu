#ifndef AT4PX_HPP
#define AT4PX_HPP
/*
at4px.hpp
2014/09/14
psycommando@gmail.com
Description: stuff for handling and building AT4PX files.

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P
*/
#include <types/content_type_analyser.hpp>
#include <utils/utility.hpp>
#include <ppmdu/fmts/px_compression.hpp>
#include <array>
#include <vector>
#include <cstdint>


namespace filetypes
{
    static const unsigned int                              MagicNumber_AT4PX_Len = 5; 
    static const std::array<uint8_t,MagicNumber_AT4PX_Len> MagicNumber_AT4PX{{ 0x41, 0x54, 0x34, 0x50, 0x58 }};//"AT4PX"

    extern const ContentTy CnTy_AT4PX;      //Contains the ID for the content type in the content type DB. Also contains the file extension
    extern const ContentTy CnTy_SIR0_AT4PX; //Contains the ID for the content type in the content type DB. Also contains the file extension

//==================================================================
// Structs
//==================================================================

    /*******************************************************
        at4px_header
        Structure of the header for an AT4PX file
    *******************************************************/
    struct at4px_header
    {
        static const unsigned int HEADER_SZ        = 18u; //Bytes
        static const unsigned int NB_FLAGS         = 9u;  //Nb of PX compression flags
        //static const unsigned int MAGIC_NUMBER_LEN = 5u;  //Bytes

        std::array<uint8_t,MagicNumber_AT4PX_Len>  magicn;       //"AT4PX"
        uint16_t                              compressedsz; //The total size of the file compressed
        std::array<uint8_t,NB_FLAGS>          flaglist;     //list of flags used in the file
        uint16_t                              decompsz;    //The image size when decompressed as 4 bpp

        //Overrides
        inline unsigned int                  size()const {return HEADER_SZ;}

        //Implementations specific to at4px_header
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
            _init ReadFromContainer(  _init itReadfrom, _init itpastend )
        {
            for( uint8_t & abyte : magicn )
            {
                abyte = utils::ReadIntFromBytes<uint8_t>(itReadfrom, itpastend);
                //abyte = *itReadfrom; 
                //++itReadfrom;
            }

            compressedsz = utils::ReadIntFromBytes<decltype(compressedsz)>(itReadfrom, itpastend); //iterator is incremented

            for( uint8_t & aflag : flaglist )
            {
                aflag = *itReadfrom; 
                ++itReadfrom;
            }

            decompsz = utils::ReadIntFromBytes<decltype(decompsz)>(itReadfrom, itpastend); //iterator is incremented

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

        at4px_header & operator=( const compression::px_info_header & other )
        {
            compressedsz = other.compressedsz;
            flaglist     = other.controlflags;
            decompsz     = static_cast<uint16_t>(other.decompressedsz);
            if( other.decompressedsz > std::numeric_limits<uint16_t>::max() )
            {
                assert(false); //This should never be bigger than a int16 with a AT4PX
                throw std::overflow_error("at4px_header::operator=(): decompressedsz is bigger than a int16!");
            }
            return *this;
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
        at4px_decompress( std::vector<uint8_t> & out_decompimg );

        //With this constructor, the functor will output the result of each calls
        // to its () operator to a new entry in the vector. 
        // The topmost vector must have the correct size! The byte vectors will
        // have their sizes adjusted accordingly to the decompressend output
        // size !
        at4px_decompress( std::vector<std::vector<uint8_t>>::iterator & out_itdecompimg );

        //Read and decompress AT4PX header and data
        // **If a vector of bytevec was passed as parameter, we expect the
        // calling code to give us as much elements in the output vector 
        // as how many times they'll call this method!**
        // If a single bytevec was passed, this method will simply overwrite 
        // the content the bytevec!
        void operator()( std::vector<uint8_t>::const_iterator itindatabeg, std::vector<uint8_t>::const_iterator itindataend, bool blogenabled );

    protected:
        //Read and decompress AT4PX header and data
        void Decompress( bool blogenabled );
        //Methods
        void ReadHeader();

        at4px_decompress( const at4px_decompress& ); //no copy
        at4px_decompress& operator=( const at4px_decompress& ); //no copy

        //Vars
        at4px_header                            m_lastheader;
        std::vector<uint8_t>::const_iterator    m_itInCur,
                                                m_itInEnd;

        std::vector<uint8_t>                      * m_outvec;
        std::vector<std::vector<uint8_t>>::iterator m_itOutContainers;
    };

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
};

#endif