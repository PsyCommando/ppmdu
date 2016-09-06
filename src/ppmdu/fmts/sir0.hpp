#ifndef SIR0_HPP
#define SIR0_HPP
/*
sir0.hpp
20/05/2014
psycommando@gmail.com

Description:
A set of helper tools and data structures for working
with SIR0 containers !

No crappyrights. All wrongs reversed !
*/
//#include <ppmdu/pmd2/pmd2_filetypes.hpp>
#include <types/content_type_analyser.hpp>
#include <utils/utility.hpp>
//#include <map>
#include <deque>

namespace filetypes
{
//====================================================================================================
//  Constants
//====================================================================================================
    //The list of pointer offsets in the file to be wrapped by the SIR0 container.
    //typedef std::vector<uint32_t> sir0_ptr_list_t;
    static const uint32_t  MagicNumber_SIR0 = 0x53495230; //SIR0
    extern const ContentTy CnTy_SIR0; 

//====================================================================================================
// Structs
//====================================================================================================
    /*
        Header structure for a SIR0 file.
    */
    struct sir0_header //: public utils::data_array_struct
    {
        static const unsigned int HEADER_LEN       = 16u; //bytes
        static const uint32_t     MAGIC_NUMBER     = MagicNumber_SIR0;                       
        static const unsigned int MAGIC_NUMBER_LEN = 4u;

        uint32_t magic;
        uint32_t subheaderptr;
        uint32_t ptrPtrOffsetLst;        //! #TODO: Rename !!!

        std::string toString()const;
        static unsigned int   size() { return HEADER_LEN; }

        sir0_header( uint32_t magicnumber = 0u, uint32_t subhdroffset = 0u, uint32_t offptrlst = 0u )
            :magic(magicnumber), subheaderptr(subhdroffset), ptrPtrOffsetLst(offptrlst)
        {}

        //Implementations specific to sir0_header
        template<class _outit>
            _outit WriteToContainer( _outit itw )const    //! #TODO: Shorten name to "Write"
        {
            itw = utils::WriteIntToBytes( MAGIC_NUMBER,    itw, false ); //Force this, to avoid bad surprises
            itw = utils::WriteIntToBytes( subheaderptr,    itw );
            itw = utils::WriteIntToBytes( ptrPtrOffsetLst, itw );
            itw = utils::WriteIntToBytes( uint32_t(0),     itw ); //Force this, to avoid bad surprises
            return itw;
        }

        //Reading the magic number, and endzero value is solely for validating on read. iterator past the end is just to avoid catastrophic overflow.
        template<class _init>
            _init ReadFromContainer( _init itr, _init itpastend ) //! #TODO: Shorten name to "Read"
        {
            magic           = utils::ReadIntFromBytes<decltype(magic)>          (itr, itpastend, false ); //iterator is incremented
            subheaderptr    = utils::ReadIntFromBytes<decltype(subheaderptr)>   (itr, itpastend ); //iterator is incremented
            ptrPtrOffsetLst = utils::ReadIntFromBytes<decltype(ptrPtrOffsetLst)>(itr, itpastend ); //iterator is incremented
            uint32_t endzero= utils::ReadIntFromBytes<decltype(endzero)>        (itr, itpastend ); //iterator is incremented
            if(endzero != 0)
                throw std::logic_error("sir0_header::Read(): The ending zero dword for the header was not 0!!");
            return itr;
        }
    };

    /*
        sir0_head_and_list
            This is used to contain the sir0 data computed for a given block of data.
    */
    //! #REMOVEME: Remove this its deprecated in favor of the SIR0 wrapper object below.
    struct sir0_head_and_list
    {
        sir0_header          hdr;
        std::vector<uint8_t> ptroffsetslst;
    };

//====================================================================================================
// Functions
//====================================================================================================

    /***********************************************************************************
        MakeSIR0ForData
            Description:
                This will compute a SIR0 header, and the pointer offset list 
                to be inserted at the end of the file. 

                It needs a list containing the offsets relative to the 
                beginning of the data to be wrapped in a SIR0 container.
                (not including the length of the SIR0 header, as the 
                function will compensate for that when encoding the list)

                It also needs the offset where the sub-header is. 
                (again not counting the SIR0 header length)

                And finally the offset one past the last byte of 
                data to be wrapped. (again not counting the SIR0 
                header length) This is where the
                list of pointer offsets will be inserted. 
                So, the padding bytes in-between the end of the 
                data and the list must be counted!

                It will return the computed SIR0 header, 
                and the full encoded list of pointer offsets. 
                The list doesn't include the padding bytes however.

                The second version is for data that doesn't contain pointers.

    ***********************************************************************************/
    sir0_head_and_list MakeSIR0ForData( const std::vector<uint32_t> &listoffsetptrs,
                                        uint32_t                     offsetsubheader,
                                        uint32_t                     offsetendofdata );     //! #REMOVEME: This is really clunky, slow, and limited, use the wrapper object below instead.

    sir0_head_and_list MakeSIR0ForData( uint32_t                     offsetsubheader,
                                        uint32_t                     offsetendofdata );     //! #REMOVEME: This is really clunky, slow, and limited, use the wrapper object below instead.

    /**************************************************************************
        MakeSIR0Wrap
            This is a convenience function to wrap a chunk of data into a 
            SIR0. 
            It comes in two version, one for data not containing any pointers,
            and the other for data that contains pointers.

            The first version for data that doesn't contain ptrs will simply have 
            the SIR0 header point to the beginning of the data.

            The second version for data that contains ptr takes the pointer to the
            sub-header, and a pointer offset list! 

            Both versions add the neccessary padding between the data and the 
            SIR0's list of encoded pointers, and at the end of the whole thing.
    **************************************************************************/
    std::vector<uint8_t> MakeSIR0Wrap( const std::vector<uint8_t>  & data,
                                       uint8_t                       padchar = 0 );         //! #REMOVEME: This is really clunky, slow, and limited, use the wrapper object below instead.
    std::vector<uint8_t> MakeSIR0Wrap( const std::vector<uint8_t>  & data,
                                       uint32_t                      offsetsubheader, 
                                       const std::vector<uint32_t> & ptroffsetlst,
                                       uint8_t                       padchar = 0 );         //! #REMOVEME: This is really clunky, slow, and limited, use the wrapper object below instead.

   std::vector<uint8_t> MakeSIR0Wrap( const std::vector<uint8_t>    & data, 
                                const sir0_head_and_list & sir0data, 
                                uint8_t                    padchar = 0 );                   //! #REMOVEME: This is really clunky, slow, and limited, use the wrapper object below instead.
    

    /**************************************************************************
        EncodeSIR0PtrOffsetList
            Description:
                Encode a list of offsets into a 
                null-terminated string of bytes,
                to be placed at the end of a SIR0 container.
    ***************************************************************************/
    std::vector<uint8_t>  EncodeSIR0PtrOffsetList( const std::vector<uint32_t> &listoffsetptrs );   //! #REMOVEME: This is really clunky, slow, and limited, use the template version instead.
    void                  EncodeSIR0PtrOffsetList( const std::vector<uint32_t> &listoffsetptrs, std::vector<uint8_t> & out_encoded ); //! #REMOVEME: This is really clunky, slow, and limited, use the template version instead.


//======================================================================================================================================
//  Functions for encoding pointer offset lists.
//======================================================================================================================================

    /*
        
    */
    template<class _backinsoutit>
        inline _backinsoutit EncodeASIR0Offset( _backinsoutit itw, uint32_t offset )
    {
        bool hasHigherNonZero = false; //This tells the loop whether it needs to encode null bytes, if at least one higher byte was non-zero
        //Encode every bytes of the 4 bytes integer we have to
        for( int32_t i = sizeof(int32_t); i > 0; --i )
        {
            uint8_t currentbyte = ( offset >> (7 * (i - 1)) ) & 0x7Fu;
            if( i == 1 )                                            //the lowest byte to encode is special
                *(itw++) = currentbyte;               //If its the last byte to append, leave the highest bit to 0 !
            else if( currentbyte != 0 || hasHigherNonZero )         //if any bytes but the lowest one! If not null OR if we have encoded a higher non-null byte before!
            {
                *(itw++) = (currentbyte | 0x80u);       //Set the highest bit to 1, to signifie that the next byte must be appended
                hasHigherNonZero = true;
            }
        }
        return itw;
    }

    template<class _backinsoutit, class _fwdinit>
        _backinsoutit EncodeSIR0PtrOffsetList( _fwdinit itbeg, _fwdinit itend, _backinsoutit itw )
    {
        uint32_t offsetSoFar = 0; //used to add up the sum of all the offsets up to the current one
        for( ; itbeg != itend; ++itbeg )
        {
            const auto & anoffset = *itbeg;
            itw                   = EncodeASIR0Offset( itw, (anoffset - offsetSoFar) );
            offsetSoFar           = anoffset; //set the value to the latest offset, so we can properly subtract it from the next offset.
        }
        //Append the closing 0
        *(itw++) = 0;
        return itw;
    }



    /**************************************************************************
        DecodeSIR0PtrOffsetList
            Description:
                Decode the encoded list of pointers 
                offsets and turns them into offsets 
                relative to the beginning of the SIR0.
                The list must only include the first 
                byte of the encoded offsets, and the 
                last one, 0x0.
    ***************************************************************************/
    std::vector<uint32_t> DecodeSIR0PtrOffsetList( const std::vector<uint8_t>  &ptroffsetslst  );

    /*
        Test a sir0_header struct to verify if its from a valid SIR0 container.
    */

 //#TODO: maybe add some functors or something in here ? Or a wrapper or something



    /***************************************************************************
        FixedSIR0DataWrapper
            A temporary object for helping with maintaining a list of offsets 
            to pointers alongside the SIR0's content, and to make writing 
            the whole thing easier and intuitive in the end!
    ***************************************************************************/
    //! #TODO: Make everything use this thing instead of the broken ambiguous as hell functions..
    template<class _CONTAINER_T>
        class FixedSIR0DataWrapper
    {
    public:
        typedef _CONTAINER_T data_t;

        FixedSIR0DataWrapper()
            :m_dataptr(0)
        {
            //Insert the 2 obligatory offsets
            m_ptroffsets.push_back(4);   //SIR0 Data Ptr offset
            m_ptroffsets.push_back(8);   //SIR0 Encoded Ptr List beg offset
        }

        template<typename _backinsoutit>
            void Write( _backinsoutit & itw )
        {
            //! #TODO: this could make a decent function to put in utility!
            static const auto lambdaWriteVector = []( const std::vector<uint8_t> & towrite, _backinsoutit & itw )
            {
                itw = std::copy(std::begin(towrite), std::end(towrite), itw);
            };

            //First, assemble + write header
            sir0_header hdr;
            hdr.ptrPtrOffsetLst = m_data.size() + sir0_header::HEADER_LEN;
            hdr.subheaderptr    = m_dataptr     + sir0_header::HEADER_LEN;
            itw = hdr.WriteToContainer(itw);

            //Then the data
            lambdaWriteVector( Data(), itw );

            //Pad as needed
            const uint32_t nbpadbytes = utils::AppendPaddingBytes( itw, m_data.size() + sir0_header::HEADER_LEN, 16, 0xAA );

            //Encode and write the offset ptr list!
            std::vector<uint8_t> encoded( (m_ptroffsets.size() + 2) * sizeof(uint32_t) );
            encoded.resize(0);  //Preserve allocation, but allows push backs!
            EncodeSIR0PtrOffsetList( m_ptroffsets.begin(), m_ptroffsets.end(), std::back_inserter(encoded) );
            lambdaWriteVector(encoded, itw );

            //Put final padding!
            utils::AppendPaddingBytes( itw, encoded.size() + nbpadbytes +  m_data.size() + sir0_header::HEADER_LEN, 16, 0xAA );
        }

        /*
            This sets the position that the first pointer in the SIR0 header will point at.
            This offset **MUST NOT** include the SIR0 header length!!
        */
        inline void SetDataPointerOffset(uint32_t offset)
        {
            m_dataptr = offset;
        }

        /*
            Adds an offset to a pointer as-is to the list to be encoded.
        */
        inline void pushptroffset(uint32_t offset)
        {
            m_ptroffsets.push_back(offset);
        }

        /*
            Push a pointer to the end of the data vector, and add the offset it was written at to the list of pointers offset to encode automatically.
        */
        inline void pushpointer(uint32_t ptr)
        {
            pushptroffset(m_data.size() + sir0_header::HEADER_LEN);
            ptr += sir0_header::HEADER_LEN;
            utils::WriteIntToBytes( ptr, std::back_inserter(m_data) );
        }

        inline data_t       & Data()        { return m_data; }
        inline const data_t & Data()const   { return m_data; }

    private:
        uint32_t              m_dataptr;
        std::deque<uint32_t>  m_ptroffsets;
        data_t                m_data;
    };


//! #REMOVEME: All this below is completely unused!!
    /*
        SIR0DerivHandler
            This is used to register all rules for filetypes that are wrapped by
            an SIR0 container!

            Registering those formats here will make sure that when the CContentHandler
            scans files to see if they are a SIR0, the formats that are wrapped by an SIR0
            container won't be ignored.
    */
    class SIR0DerivHandler
    {
    public:
        
        static SIR0DerivHandler & GetInstance();

        //Rule registration handling
        cntRID_t RegisterRule  ( IContentHandlingRule * rule );
        bool     UnregisterRule( cntRID_t ruleid );

        ContentBlock AnalyseContent( const analysis_parameter & parameters );

    private:
        typedef std::map<cntRID_t,std::unique_ptr<IContentHandlingRule>> container_t;
        cntRID_t    m_currentRID;
        container_t m_rules;

        static const cntRID_t INVALID_RID = -1;
    private:
        SIR0DerivHandler():m_currentRID(0){}
        SIR0DerivHandler(const SIR0DerivHandler&);
        SIR0DerivHandler(const SIR0DerivHandler&&);
        SIR0DerivHandler& operator=(const SIR0DerivHandler&);
        SIR0DerivHandler& operator=(const SIR0DerivHandler&&);
    };



    /*************************************************************************************
        SIR0RuleRegistrator
            A small singleton that has for only task to register the rule.
            Just call the constructor in your cpp files, with the type of
            the rule as parameter!

            Example:
                RuleRegistrator<ruletypename> RuleRegistrator<ruletypename>::s_instance;
    *************************************************************************************/
    template<class RULE_T> class SIR0RuleRegistrator
    {
    public:
        SIR0RuleRegistrator()
        {
            SIR0DerivHandler::GetInstance().RegisterRule( new RULE_T );
        }

    private:
        static SIR0RuleRegistrator<RULE_T> s_instance;
    };

};
#endif