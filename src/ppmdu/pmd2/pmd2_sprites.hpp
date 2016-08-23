#ifndef PMD2_SPRITES_HPP
#define PMD2_SPRITES_HPP
/*
pmd2_sprites.hpp
19/05/2014
psycommando@gmail.com

Description: Various utilities for dealing with the sprite format in PMD2.

#TODO: Its about time to do a good cleanup here ! Lots of unused functions, and methods!
       Plus, some are just not in the right place.

No crappyrights. All wrong reversed!
*/
#include <ppmdu/pmd2/pmd2_palettes.hpp>
#include <utils/handymath.hpp>
#include <utils/utility.hpp>
#include <ppmdu/pmd2/pmd2_image_formats.hpp>
#include <ppmdu/pmd2/pmd2_filetypes.hpp>
#include <ppmdu/fmts/sir0.hpp>
#include <utility>

namespace pmd2{ namespace graphics
{
//=====================================================================================
// Constants
//=====================================================================================
    //#TODO : Are those still of any use ?
    static const unsigned int DBG_DEF_NB_ENTRIES =  35u; //Taken from Eevee's sprite.
    static const unsigned int DBH_DEF_NB_ENTRIES = 280u; //Taken from Eevee's sprite.
    static const unsigned int DBI_DEF_NB_ENTRIES = 878u; //Taken from Eevee's sprite.

//=====================================================================================
// Structs
//=====================================================================================
    /*
        The part of the header containing the pointers to unidentified kind of info for the sprite
    */
    struct sprite_info_data /*: public utils::data_array_struct*/
    {
        static const unsigned int DATA_LEN = 24u;

        uint32_t ptr_ptrstable_e, //pointer to the pointers table at OFF_E
                 ptr_offset_f,
                 ptr_offset_g;
        uint16_t nb_blocks_in_offset_g,
                 nb_entries_offset_e,  //<-- The name for this is wrong, we don't know what this value does
                 unknown1,
                 unknown2,
                 unknown3,
                 unknown4;

        //uint8_t       & operator[](unsigned int index); //access data as an array of bytes
        //const uint8_t & operator[](unsigned int index)const;
        unsigned int size()const{return DATA_LEN;}
        std::string toString()const;


        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( ptr_ptrstable_e,       itwriteto );
            itwriteto = utils::WriteIntToBytes( ptr_offset_f,          itwriteto );
            itwriteto = utils::WriteIntToBytes( ptr_offset_g,          itwriteto );
            itwriteto = utils::WriteIntToBytes( nb_blocks_in_offset_g, itwriteto );
            itwriteto = utils::WriteIntToBytes( nb_entries_offset_e,   itwriteto );
            itwriteto = utils::WriteIntToBytes( unknown1,              itwriteto );
            itwriteto = utils::WriteIntToBytes( unknown2,              itwriteto );
            itwriteto = utils::WriteIntToBytes( unknown3,              itwriteto );
            itwriteto = utils::WriteIntToBytes( unknown4,              itwriteto );
            return itwriteto;
        }

        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itPastEnd )
        {
            itReadfrom = utils::ReadIntFromBytes( ptr_ptrstable_e,       itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( ptr_offset_f,          itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( ptr_offset_g,          itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( nb_blocks_in_offset_g, itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( nb_entries_offset_e,   itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( unknown1,              itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( unknown2,              itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( unknown3,              itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( unknown4,              itReadfrom, itPastEnd );
            return itReadfrom;
        }

    };

    /*
        The part of the header containing the pointers to the actual frames of the sprite,
        and the palette.
    */
    struct sprite_frame_data /*: public utils::data_array_struct*/
    {
        static const unsigned int DATA_LEN = 16u;

        uint32_t ptr_frm_ptrs_table,        // Pointer to the the table of pointer to the individual frames
                 ptrPal;               // Pointer to the pointer to the beginning of the palette
        uint16_t unkn_1,                    // Unknown, seems to range between 0 and 1..
                 unkn_2,                    // Unknown, seems to range between 0 and 1..
                 unkn_3,                    // Unknown, seems to range between 0 and 1..
                 nbImgsTblPtr;    // Number of entries in the table of pointers to each frames.

        //uint8_t & operator[](unsigned int index); //access data as an array of bytes
        //const uint8_t & operator[](unsigned int index)const;
        unsigned int size()const{return DATA_LEN;}
        std::string toString()const;

        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( ptr_frm_ptrs_table, itwriteto );
            itwriteto = utils::WriteIntToBytes( ptrPal,             itwriteto );
            itwriteto = utils::WriteIntToBytes( unkn_1,             itwriteto );
            itwriteto = utils::WriteIntToBytes( unkn_2,             itwriteto );
            itwriteto = utils::WriteIntToBytes( unkn_3,             itwriteto );
            itwriteto = utils::WriteIntToBytes( nbImgsTblPtr,       itwriteto );
            return itwriteto;
        }

        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itPastEnd )
        {
            itReadfrom = utils::ReadIntFromBytes( ptr_frm_ptrs_table, itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( ptrPal,             itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( unkn_1,             itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( unkn_2,             itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( unkn_3,             itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( nbImgsTblPtr,       itReadfrom, itPastEnd );
            return itReadfrom;
        }

        //std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const;
        //std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom );
    };


    /*
        The 12 bytes sub-header that is linked to by an SIR0 header.
        Contains pointers to important sprite information.
    */
    struct sprite_data_header /*: public utils::data_array_struct*/
    {
        static const unsigned int DATA_LEN = 12u;

        uint32_t spr_ptr_info,
                 spr_ptr_frames;
        uint16_t unknown0,
                 unknown1;

        unsigned int size()const{return DATA_LEN;}
        std::string toString()const;

        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( spr_ptr_info,   itwriteto );
            itwriteto = utils::WriteIntToBytes( spr_ptr_frames, itwriteto );
            itwriteto = utils::WriteIntToBytes( unknown0,       itwriteto );
            itwriteto = utils::WriteIntToBytes( unknown1,       itwriteto );
            return itwriteto;
        }

        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itPastEnd )
        {
            itReadfrom = utils::ReadIntFromBytes( spr_ptr_info,   itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes( spr_ptr_frames, itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes( unknown0,       itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes( unknown1,       itReadfrom, itPastEnd);
            return itReadfrom;
        }

        //std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const;
        //std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom );
    };

//=====================================================================================
// Structs For Research Purpose Mainly
//=====================================================================================

    struct datablock_i_entry /*: public utils::data_array_struct*/
    {
        static const unsigned int MY_SIZE = 12u;
        uint16_t Unk0,
                 Index;
        int16_t  Val0,
                 Val1,
                 Val2,
                 Val3;

        datablock_i_entry()
        {
            Reset();
        }

        //uint8_t & operator[](unsigned int index); 
        //const uint8_t & operator[](unsigned int index)const;
        std::string toString(unsigned int indent=0)const;

        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( Unk0,  itwriteto );
            itwriteto = utils::WriteIntToBytes( Index, itwriteto );
            itwriteto = utils::WriteIntToBytes( Val0,  itwriteto );
            itwriteto = utils::WriteIntToBytes( Val1,  itwriteto );
            itwriteto = utils::WriteIntToBytes( Val2,  itwriteto );
            itwriteto = utils::WriteIntToBytes( Val3,  itwriteto );
            return itwriteto;
        }

        template<class _init>
            _init ReadFromContainer( _init itReadfrom,  _init itPastEnd )
        {
            itReadfrom = utils::ReadIntFromBytes (Unk0,  itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes (Index, itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes (Val0,  itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes (Val1,  itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes (Val2,  itReadfrom, itPastEnd);
            itReadfrom = utils::ReadIntFromBytes (Val3,  itReadfrom, itPastEnd);
            return itReadfrom;
        }

        //std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const;
        //std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom );

        //Assign the members of "other" to their counterpart in the current instance. Return this.
        datablock_i_entry & operator+=( const datablock_i_entry & other );

        //Divide each members by the integer specified
        datablock_i_entry operator/( int16_t other )const;

        //Assign the members of "other" that are bigger than their counterpart in the current instance. Return this.
        datablock_i_entry & AssignMembersIfGreaterThan( const datablock_i_entry & other );

        //Assign the members of "other" that are smaller than their counterpart in the current instance. Return this.
        datablock_i_entry & AssignMembersIfSmallerThan( const datablock_i_entry & other );

        unsigned int size()const{return MY_SIZE;}
        bool isNullEntry()const { return !Unk0 && !Index && !Val0 && !Val1 && !Val2 && !Val3; }
        void Reset()
        {
            Index = 0;
            Unk0 = 0;
            Val0 = 0;
            Val1 = 0;
            Val2 = 0;
            Val3 = 0;
        }
    };

    /*
        Contains a ptr to an array in datablockH and a size for the said array. 
    */
    struct datablock_g_entry/* : public utils::data_array_struct*/
    {
        static const unsigned int MY_SIZE = 8u;

        datablock_g_entry( uint32_t ptr = 0, uint32_t sz = 0 )
            :ptrtoarray(ptr), szofarray(sz)
        {}

        uint32_t ptrtoarray,
                 szofarray;

        //uint8_t & operator[](unsigned int index); 
        //const uint8_t & operator[](unsigned int index)const;
        std::string toString()const;

        unsigned int size()const{return MY_SIZE;}
        bool isNullEntry()const { return !ptrtoarray && !szofarray; }

        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( ptrtoarray, itwriteto );
            itwriteto = utils::WriteIntToBytes( szofarray,  itwriteto );

            return itwriteto;
        }

        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itPastEnd )
        {
            itReadfrom = utils::ReadIntFromBytes( ptrtoarray, itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( szofarray,  itReadfrom, itPastEnd );
            return itReadfrom;
        }

        //std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const;
        //std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom );
    };

    /*
        
    */
    struct datablock_s_entry/* : utils::data_array_struct*/
    {
        static const unsigned int MY_SIZE = 10u;
        uint32_t id;
        uint8_t  val0,
                 val1,
                 val2,
                 val3;
        uint16_t endofentry;

        datablock_s_entry() { reset(); }
                 
        unsigned int size()const { return MY_SIZE; }
        //uint8_t & operator[](unsigned int index);
        //const uint8_t & operator[](unsigned int index)const;

        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( id,         itwriteto );
            itwriteto = utils::WriteIntToBytes( val0,       itwriteto );
            itwriteto = utils::WriteIntToBytes( val1,       itwriteto );
            itwriteto = utils::WriteIntToBytes( val2,       itwriteto );
            itwriteto = utils::WriteIntToBytes( val3,       itwriteto );
            itwriteto = utils::WriteIntToBytes( endofentry, itwriteto );
            return itwriteto;
        }

        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itPastEnd )
        {
            itReadfrom = utils::ReadIntFromBytes( id,         itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( val0,       itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( val1,       itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( val2,       itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( val3,       itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( endofentry, itReadfrom, itPastEnd );
            return itReadfrom;
        }

        //std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const;
        //std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom );

        //Assign the members of "other" to their counterpart in the current instance. Return this.
        datablock_s_entry & operator+=( const datablock_s_entry & other );

        //Divide each members by the integer specified
        datablock_s_entry operator/( int16_t other )const;

        //Assign the members of "other" that are bigger than their counterpart in the current instance. Return this.
        datablock_s_entry & AssignMembersIfGreaterThan( const datablock_s_entry & other );

        //Assign the members of "other" that are smaller than their counterpart in the current instance. Return this.
        datablock_s_entry & AssignMembersIfSmallerThan( const datablock_s_entry & other );

        std::string toString( unsigned int indent = 0 )const;
        void reset();
    };

    /*
    */
    struct datablock_f_entry/* : utils::data_array_struct*/
    {
        static const unsigned int MY_SIZE = 4u;
        int16_t val0,
                val1;

        datablock_f_entry() { reset(); }
                 
        unsigned int size()const { return MY_SIZE; }
        //uint8_t & operator[](unsigned int index);
        //const uint8_t & operator[](unsigned int index)const;

        std::string toString( unsigned int indent = 0 )const;
        void reset();

        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( val0,  itwriteto );
            itwriteto = utils::WriteIntToBytes( val1,  itwriteto );
            return itwriteto;
        }

        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itPastEnd )
        {
            itReadfrom = utils::ReadIntFromBytes( val0, itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( val1, itReadfrom, itPastEnd );
            return itReadfrom;
        }

        //std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const;
        //std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom );
    };

    /*
        Struct for the extra data located right after the palette!
        16 bytes long.
    */
    struct palette_fmtinf /*: utils::data_array_struct*/
    {
        static const unsigned int MY_SIZE = 16u;

        uint32_t ptrpalbeg;
        uint16_t unknown0,
                 unknown1,
                 unknown2,
                 unknown3;
        uint32_t endofdata;

        palette_fmtinf(){reset();}

        unsigned int    size()const { return MY_SIZE; }
        //uint8_t       & operator[](unsigned int index);
        //const uint8_t & operator[](unsigned int index)const;
        std::string     toString( unsigned int indent = 0 )const;
        void            reset(); //Set all values to 0

        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( ptrpalbeg, itwriteto );
            itwriteto = utils::WriteIntToBytes( unknown0,  itwriteto );
            itwriteto = utils::WriteIntToBytes( unknown1,  itwriteto );
            itwriteto = utils::WriteIntToBytes( unknown2,  itwriteto );
            itwriteto = utils::WriteIntToBytes( unknown3,  itwriteto );
            itwriteto = utils::WriteIntToBytes( endofdata, itwriteto );
            return itwriteto;
        }

        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itPastEnd )
        {
            itReadfrom = utils::ReadIntFromBytes( ptrpalbeg, itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( unknown0,  itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( unknown1,  itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( unknown2,  itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( unknown3,  itReadfrom, itPastEnd );
            itReadfrom = utils::ReadIntFromBytes( endofdata, itReadfrom, itPastEnd );
            return itReadfrom;
        }

        //std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const;
        //std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom );
    };


//=====================================================================================
// Sprite Unknown Reference Table
//=====================================================================================
    
    //Forward declare
    class sprite_parser;

    /*
        Utility class to encapsulate and maintain the proper state 
        of the 2 reference tables and the data table they point at.

        NOTE:
            This is going to be very slow to edit sprite content right now.
            But as research progress we might find ways to make it faster!
            We're keeping a lot of data that might be unecessary right now!
    */
    class SpriteUnkRefTable
    {
        friend class sprite_parser;
    public:

        SpriteUnkRefTable( unsigned int toreserveG = DBG_DEF_NB_ENTRIES, 
                           unsigned int toreserveH = DBH_DEF_NB_ENTRIES, 
                           unsigned int toreserveI = DBI_DEF_NB_ENTRIES );


        //Push Null Entry
        // Appends a null entry properly represented in both Datablock G and H
        unsigned int pushnullrefs();

        //PushEntry
        // Appends a vector of indexes passed in paramters. The indexes have to point to
        // entries in the current datablock I data content.
        // returns index of entry in datablock G
        unsigned int pushrefs( const std::vector<uint32_t> & datablockH_Array );

        //Move a vector containing datablockI entries into the container
        // -> This makes all existing references inaccessible, and the class will behave
        //    as if it was empty!
        void moveInData( std::vector<std::vector<datablock_i_entry> > && datablocki );

        void moveInReferences( std::vector<std::vector<uint32_t> > && datablockh );


        const datablock_g_entry              & getDatablockGEntry( unsigned int index )const;
        const std::vector<uint32_t>          & getDatablockHArray( unsigned int index )const;
        const std::vector<datablock_i_entry> & getDatablockIArray( unsigned int index )const;

        unsigned int getSizeDatablockG()const;
        unsigned int getNbArraysDatablockH()const;
        unsigned int getNbArraysDatablockI()const;

        //Write a full report on how the 3 datablocks interact, and what they contain!
        std::string WriteReport()const;

    private:

        std::vector<datablock_g_entry>               m_datablockG;
        std::vector< std::vector<uint32_t> >         m_datablockH;
        std::vector<std::vector<datablock_i_entry> > m_datablockI;  //2 vectors to make it easier to grow and shrink
    };

//=====================================================================================
// Sprite Data 
//=====================================================================================
   
    /*
        CCharSpriteData
            Contain parsed data from a character sprite file.
    */
    class CCharSpriteData
    {
        friend class sprite_parser;
    public:
        CCharSpriteData(){}

        //-------------------------------------------------
        //                Manipulation
        //-------------------------------------------------
        const std::vector<colRGB24> & getPalette()const;
        void setPalette( const std::vector<colRGB24> & pal );

        //TODO: Remove this one of these days..
        std::vector<indexed8bppimg_t>& getAllFrames();

        //
        const indexed8bppimg_t& getFrame( std::vector<indexed8bppimg_t>::size_type frmno );
        void                    setFrame( std::vector<indexed8bppimg_t>::size_type frmno, 
                                          const indexed8bppimg_t                 & frmdata );

        std::string WriteReport();
         
    private:
        //-------------------------------------------------
        //                      Data
        //-------------------------------------------------
        std::vector<indexed8bppimg_t>  m_frames8bpp; //Decoded frame data, always stored as 8bpp indexed, for faster interop
        std::vector<colRGB24>        m_palette;    //Color palette for the sprite, just stored as RGB24, because its easier

        //Sprite data
        std::vector<datablock_s_entry> m_datablockE; //Block E's entries points to block S entries..
        std::vector<datablock_f_entry> m_datablockF;

        //Datablock G, H, and I. Their states are closely linked and need to be managed by another object
        SpriteUnkRefTable              m_datablocksGHI; 

        //Temporary stuff, to remove/merge enventually
        palette_fmtinf                 m_palfmt;
    };

//=====================================================================================
// Sprite Parser
//=====================================================================================

    /*
        sprite_parser
            A functor to parse sprites into a CCharSpriteData structure!
    */
    class sprite_parser
    {
    public:

        /*
            This constructor takes a single CCharSpriteData to output to.
        */
        sprite_parser( CCharSpriteData & out_asprite );

        /*
            This constructor takes an iterator to a structure with several CCharSpriteData to output to.
            The calling code is expected to have set the correct size for the vector, and to only invoke 
            the "()" operator as many times as there are elements in the target vector. In other words,
            guarantee we won't dereference an invalid iterator at one point after incrementing it!
        */
        sprite_parser( std::vector<CCharSpriteData>::iterator itspritesbeg, std::vector<CCharSpriteData>::iterator itspritesend );

        /*
            This takes in parameter the bgining and the end of the sprite data.

            -> report is a string where to output data about the headers of the sprite file.

            -> itbegdata MUST be at the very first byte of the SIR0 header, or the place
               from where the pointers in the files are calculated from.

            -> itenddata MUST be at either the same position as the eof pointer in the
               SIR0 header points to, or anywhere afterwrds.
        */
        void operator()( std::vector<uint8_t>::const_iterator itbegdata, std::vector<uint8_t>::const_iterator itenddata );
        void operator()( std::vector<uint8_t>::const_iterator itbegdata, std::vector<uint8_t>::const_iterator itenddata, std::string & report );
        void operator()( const std::pair<std::vector<uint8_t>::const_iterator, std::vector<uint8_t>::const_iterator> & apair );

    private:

        //Methods
        void ReadSIR0Header();
        void ReadEntireSpriteSubHeader();
        void ReadPalette();
        void ReadAllFramePointers();
        void ReadAndDecodeAllFrames();

        void ReadDatablockG();
        void ReadDatablockE();
        void ReadDatablockF();

        //Output
        CCharSpriteData                         *m_pCurSpriteOut;  //Pointer to the current sprite to output to. 
        std::vector<CCharSpriteData>::iterator  m_itCurSprite,  //Used only when outputing to several sprites.
                                                m_itSpritesEnd;
        bool                                    m_isUsingIterator; //Whether we've received an iterator at construction

        //Input
        std::vector<uint8_t>::const_iterator                    m_itbegdata,
                                                m_itcurdata,
                                                m_itenddata;

        //Headers / Sub-Headers
        ::filetypes::sir0_header                m_sir0header;
        sprite_data_header                      m_subheader;
        sprite_info_data                        m_sprinf;
        sprite_frame_data                       m_sprfrmdat;

        //Temporary data
        std::vector<uint32_t>                   m_framepointers;
        uint32_t                                m_offsetH;
        uint32_t                                m_offsetFirstFrameBeg;

        std::string                            *m_pReport;
    };


//=====================================================================================
//                               Sprite Import/Export
//=====================================================================================
};};

#endif