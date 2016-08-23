#ifndef SPRITE_RLE_HPP
#define SPRITE_RLE_HPP
/*
sprite_rle.hpp
2014/09/22
psycommando@gmail.com
Description: Utilites for handling the RLE compression used on sprites in PMD2.

#TODO: put the sprite RLE stuff in here !

*/
#include <ppmdu/pmd2/pmd2_image_formats.hpp>
#include <utils/utility.hpp>

namespace pmd2 { namespace compression
{

//====================================================================================================
// Struct
//====================================================================================================
    //This is a single entry from an RLE table
    struct rle_table_entry /*: public utils::data_array_struct*/
    {
        static const unsigned int LENGTH = 12u;
        uint32_t pixelsrc,  //The source of the pixels to use to rebuild the image. Either an address, or 0
                 pixamt,    //The amount of pixels to copy / insert
                 unknown;   //Not sure what this does..

        unsigned int    size()const   { return LENGTH; }
        bool            isNull()const { return (!pixelsrc && !pixamt && !unknown); } //Whether its a null entry or not 

        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( pixelsrc, itwriteto );
            itwriteto = utils::WriteIntToBytes( pixamt,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unknown,  itwriteto );
            return itwriteto;
        }

        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itPastEnd )
        {
            itReadfrom = utils::ReadIntFromBytes( pixelsrc, itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( pixamt,   itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( unknown,  itReadfrom, itPastEnd );
            return itReadfrom;
        }
    };

//====================================================================================================
// Functors
//====================================================================================================
    /*
        rle_encoder
            Functor for encoding raw, tiled 8bpp image data into the 4bpp RLE encoded format found in
            PMD2 sprites.
    */
    class rle_encoder
    {
    public:

    private:
    };

    /*
        rle_decoder
            Functor for handling decompression of RLE encoded 4bpp sprite image data.
    */
    class rle_decoder
    {
    public:

        /*
            Construct with either a single image to write the output to, or a range of images to
            write to. Each calls to the () operator makes the code write to the next image within 
            the list.
        */
        rle_decoder( std::vector<uint8_t> & out_decoded );
        rle_decoder( std::vector<std::vector<uint8_t>>::iterator itbegout, 
                     std::vector<std::vector<uint8_t>>::iterator itendout );

        /* 
           - itbeg   : The beginning of the container. Or the position against which the offsets stored in the RLE table are
                       calculated against.
           - itfrmin : The position at which the pointer in the pointer table points at for the frame to decode! In other words
                       its the beginning of the RLE table.
           - Returns the index of the beginning of the frame data. FRM_BEG for the current frame!
        */
        uint32_t operator()( std::vector<uint8_t>::const_iterator itbeg, 
                             std::vector<uint8_t>::const_iterator itend,
                             std::vector<uint8_t>::const_iterator itfrmin );

    private:
        std::vector<uint8_t>                        * m_pOutput; //What we currently output to.
        std::vector<std::vector<uint8_t>>::iterator   m_itcurout, //an output iterator to a data structure of images.
                                                      m_itendout;
        bool                                          m_bUsingIterator; //Whether we should care about our vector iterator
   
    };
};};

#endif