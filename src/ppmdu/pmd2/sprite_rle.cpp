#include "sprite_rle.hpp"
#include <cassert>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <iostream>
#include <utils\utility.hpp>
#include <ppmdu\basetypes.hpp>
using namespace std;


namespace pmd2 { namespace compression
{
//================================================================================================
// Utility Functions
//================================================================================================

    /*
        Also increments the output iterator!
    */
    void InsertZerosIntoVector( vector<uint8_t>::iterator & itpos, unsigned int amount )
    {
        for( unsigned int i = 0; i < amount; ++i, ++itpos )
            (*itpos) = 0; 
    }

//================================================================================================
// rle_table_entry
//================================================================================================

    //std::vector<uint8_t>::iterator rle_table_entry::WriteToContainer(  std::vector<uint8_t>::iterator itwriteto )const
    //{
    //    itwriteto = utils::WriteIntToBytes( pixelsrc, itwriteto );
    //    itwriteto = utils::WriteIntToBytes( pixamt,   itwriteto );
    //    itwriteto = utils::WriteIntToBytes( unknown,  itwriteto );
    //    return itwriteto;
    //}
    //std::vector<uint8_t>::const_iterator rle_table_entry::ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom )
    //{
    //    pixelsrc = utils::ReadIntFromBytes<decltype(pixelsrc)>(itReadfrom);
    //    pixamt   = utils::ReadIntFromBytes<decltype(pixamt)>  (itReadfrom);
    //    unknown  = utils::ReadIntFromBytes<decltype(unknown)> (itReadfrom);
    //    return itReadfrom;
    //}

//================================================================================================
// rle_decoder Utility Functions
//================================================================================================

    /*
        Increments the output iterator!
    */
    void HandleRLETableEntry( const rle_table_entry                      &entry, 
                              std::vector<uint8_t>::const_iterator  itbeg, 
                              std::vector<uint8_t>::iterator       &itout )
    {
        //Execute whatever it wants to do
        if( !entry.isNull() )
        {
            if( entry.pixelsrc == 0 )
            {
                InsertZerosIntoVector( itout, (entry.pixamt * 2) ); //Multiplied by 2, because 4bpp to 8bpp
            }
            else
            {
                for( unsigned int i = 0; i < entry.pixamt; ++i )
                {
                    uint8_t twopixels = *(itbeg + entry.pixelsrc + i );
                    (*itout) = twopixels & 0x0F;
                    ++itout;
                    (*itout) = ( twopixels & 0xF0 ) >> 4;
                    ++itout;
                }
            }
        }
    }

//================================================================================================
// rle_decoder
//================================================================================================
    rle_decoder::rle_decoder( std::vector<uint8_t> & out_decoded )
        :m_pOutput(&out_decoded), m_bUsingIterator(false)
    {
    }

    rle_decoder::rle_decoder( std::vector<std::vector<uint8_t>>::iterator itbegout, std::vector<std::vector<uint8_t>>::iterator itendout )
        :m_itcurout(itbegout), m_itendout(itendout),m_pOutput(&(*itbegout)), m_bUsingIterator(true)
    {
    }

    uint32_t rle_decoder::operator()( vector<uint8_t>::const_iterator itbeg, vector<uint8_t>::const_iterator itfrmin )
    {
        if( m_pOutput == nullptr )
        {
            assert(false); //Still calling "operator()" even after the output is out of bounds !
            std::cerr << "!- Error : Tried to call rle_decoder::operator(), even though output is out of bound !\n";
            throw exception();
        }

        unsigned int            sanitycpt   = 0; //A little counter to abort in case we don't find a null entry
        vector<rle_table_entry> entries;
        uint32_t                imagesz8bpp = 0;
        uint32_t                frm_beg     = 0;

        //#1 - Read our RLE table, and calculate our size
        do
        {
            rle_table_entry entry;
            //Read current entry
            itfrmin = entry.ReadFromContainer( itfrmin );
            //advance( itfrmin, entry.size() ); //Increment reading position in the RLE table

            imagesz8bpp += entry.pixamt * 2; //Multiply the bytes per 2, because we're exporting to 8bpp!

            if( frm_beg == 0 && entry.pixelsrc != 0 ) //Get the beginning of the frame data if possible
                frm_beg = entry.pixelsrc;

            entries.push_back(entry);

            ++sanitycpt;
        } while( !entries.back().isNull() && sanitycpt < 1000u ); //We abort when hitting 1000 turns, just in case

        assert( sanitycpt < 1000u ); //If we hit this, the data fed to this functor is very wrong !

        //#2 - Resize the output
        m_pOutput->resize( imagesz8bpp );
        auto itoutpos  = m_pOutput->begin(); //The place in the output image we're writing at!

        //#3 - Fill the image up
        for( auto &entry : entries )
        {
            HandleRLETableEntry( entry, itbeg, itoutpos );
        }

        //Increment to get the next element to output to, the next time this function is called, if applicable
        if(m_bUsingIterator ) 
        {
            ++m_itcurout;
            if( m_itcurout != m_itendout )
            {
                m_pOutput = &(*(m_itcurout));
            }
            else
                m_pOutput = nullptr;
        }

        return frm_beg;
    }


};};