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
#include <map>

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
        uint32_t ptrPtrOffsetLst;        //#TODO: Rename !!!
        uint32_t endzero;

        std::string toString()const;
        static unsigned int   size() { return HEADER_LEN; }

        sir0_header( uint32_t magicnumber = 0u, uint32_t subhdroffset = 0u, uint32_t offptrlst = 0u )
            :magic(magicnumber), subheaderptr(subhdroffset), ptrPtrOffsetLst(offptrlst), endzero(0u)
        {}

        //std::vector<uint8_t>::iterator       WriteToContainer(  std::vector<uint8_t>::iterator       itwriteto )const;
        //std::vector<uint8_t>::const_iterator ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom );


        //Implementations specific to sir0_header
        template<class _outit>
            _outit WriteToContainer( _outit itwriteto )const
        {
            itwriteto = utils::WriteIntToBytes( MAGIC_NUMBER,    itwriteto, false ); //Force this, to avoid bad surprises
            itwriteto = utils::WriteIntToBytes( subheaderptr,    itwriteto );
            itwriteto = utils::WriteIntToBytes( ptrPtrOffsetLst, itwriteto );
            itwriteto = utils::WriteIntToBytes( uint32_t(0),     itwriteto ); //Force this, to avoid bad surprises
            return itwriteto;
        }

        //Reading the magic number, and endzero value is solely for validating on read.
        template<class _init>
            _init ReadFromContainer( _init itReadfrom, _init itpastend )
        {
            magic           = utils::ReadIntFromBytes<decltype(magic)>          (itReadfrom, itpastend, false ); //iterator is incremented
            subheaderptr    = utils::ReadIntFromBytes<decltype(subheaderptr)>   (itReadfrom, itpastend ); //iterator is incremented
            ptrPtrOffsetLst = utils::ReadIntFromBytes<decltype(ptrPtrOffsetLst)>(itReadfrom, itpastend ); //iterator is incremented
            endzero         = utils::ReadIntFromBytes<decltype(endzero)>        (itReadfrom, itpastend ); //iterator is incremented
            return itReadfrom;
        }
    };

    /*
        sir0_head_and_list
            This is used to contain the sir0 data computed for a given block of data.
    */
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
                                        uint32_t                     offsetendofdata );

    sir0_head_and_list MakeSIR0ForData( uint32_t                     offsetsubheader,
                                        uint32_t                     offsetendofdata );

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
                                       uint8_t                       padchar = 0 );
    std::vector<uint8_t> MakeSIR0Wrap( const std::vector<uint8_t>  & data,
                                       uint32_t                      offsetsubheader, 
                                       const std::vector<uint32_t> & ptroffsetlst,
                                       uint8_t                       padchar = 0 );


    /**************************************************************************
        EncodeSIR0PtrOffsetList
            Description:
                Encode a list of offsets into a 
                null-terminated string of bytes,
                to be placed at the end of a SIR0 container.
    ***************************************************************************/
    std::vector<uint8_t>  EncodeSIR0PtrOffsetList( const std::vector<uint32_t> &listoffsetptrs );
    void                  EncodeSIR0PtrOffsetList( const std::vector<uint32_t> &listoffsetptrs, std::vector<uint8_t> & out_encoded );

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
    //inline bool IsValid_SIR0_Header( const sir0_header & header, filesize_t filesizetotal )
    //{
    //    using namespace magicnumbers;
    //    return ( 
    //                header.magic        == SIR0_MAGIC_NUMBER_INT && 
    //                header.endzero        == 0x0                   && 
    //                header.subheaderptr <  filesizetotal         && header.subheaderptr > 0x0 &&
    //                header.ptrPtrOffsetLst       <= filesizetotal         && header.subheaderptr > 0x0
    //           );
    //}
    
 //#TODO: maybe add some functors or something in here ? Or a wrapper or something


//
//
//
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