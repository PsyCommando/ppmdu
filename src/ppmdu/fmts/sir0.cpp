/*
sir0.cpp
20/05/2014
psycommando@gmail.com

Description:

No crappyrights. All wrongs reversed ! 
*/
#include <ppmdu/fmts/sir0.hpp>
#include <ppmdu/fmts/content_type_analyser.hpp>
#include <ppmdu/utils/utility.hpp>
#include <string>
#include <sstream>
#include <iomanip>
#include <array>
using namespace std;

namespace pmd2 { namespace filetypes
{
//========================================================================================================
// sir0_header
//========================================================================================================

    string sir0_header::toString()const
    {
        stringstream strs;
        strs <<setfill('.') <<setw(22) <<left 
             <<"Sprite header offset" <<": 0x" <<setfill('0') <<setw(8) <<uppercase <<right <<hex <<subheaderptr <<nouppercase <<"\n"
             <<setfill('.') <<setw(22) <<left 
             <<"End of data offset"   <<": 0x" <<setfill('0') <<setw(8) <<uppercase <<right <<hex <<eofptr       <<nouppercase <<"\n";
        return strs.str();
    }

    std::vector<uint8_t>::iterator sir0_header::WriteToContainer(  std::vector<uint8_t>::iterator itwriteto )const
    {
        itwriteto = utils::WriteIntToByteVector( magic,        itwriteto );
        itwriteto = utils::WriteIntToByteVector( subheaderptr, itwriteto );
        itwriteto = utils::WriteIntToByteVector( eofptr,       itwriteto );
        itwriteto = utils::WriteIntToByteVector( _null,        itwriteto );
        return itwriteto;
    }

    std::vector<uint8_t>::const_iterator sir0_header::ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom )
    {
        magic        = utils::ReadIntFromByteVector<decltype(magic)>       (itReadfrom);
        subheaderptr = utils::ReadIntFromByteVector<decltype(subheaderptr)>(itReadfrom);
        eofptr       = utils::ReadIntFromByteVector<decltype(eofptr)>      (itReadfrom);
        _null        = utils::ReadIntFromByteVector<decltype(_null)>       (itReadfrom);
        return itReadfrom;
    }

//========================================================================================================
// Utility:
//========================================================================================================

    sir0_head_and_list MakeSIR0ForData( const std::vector<uint32_t> &listoffsetptrs,
                                        uint32_t                     offsetsubheader,
                                        uint32_t                     offsetendofdata )
    {
        static const uint8_t SIR0_EncodedOffsetsHeader = 0x04u;
        sir0_header hdr( magicnumbers::SIR0_MAGIC_NUMBER_INT, 
                         offsetsubheader + sir0_header::HEADER_LEN,
                         offsetendofdata + sir0_header::HEADER_LEN );

        vector<uint8_t> encodedptroffsets( 2u + listoffsetptrs.size() ); //Worst case scenario allocation
        encodedptroffsets.resize(0); //preserve alloc, allow push backs

        //Encode the pointers

        // 0000 0000 0000 0000 0000 0000 0111 1111
        //                                     <<7
        // 0000 0000 0000 0000 0011 1111 1000 0000
        //                                     <<7
        // 0000 0000 0001 1111 1100 0000 0000 0000
        //                                     <<7
        // 0000 1111 1110 0000 0000 0000 0000 0000

        std::vector<uint32_t> offsetsToEncode(listoffsetptrs.size() + 2);
        offsetsToEncode.resize(0);

        offsetsToEncode.push_back(SIR0_EncodedOffsetsHeader);
        offsetsToEncode.push_back(SIR0_EncodedOffsetsHeader + SIR0_EncodedOffsetsHeader);
        offsetsToEncode.insert( offsetsToEncode.end(), listoffsetptrs.begin(), listoffsetptrs.end() );

        EncodeSIR0PtrOffsetList( offsetsToEncode, encodedptroffsets );
        return sir0_head_and_list{ hdr, std::move(encodedptroffsets) };
    }

    void EncodeSIR0PtrOffsetList( const std::vector<uint32_t> &listoffsetptrs, std::vector<uint8_t> & out_encoded )
    {
        uint32_t offsetSoFar = 0; //used to add up the sum of all the offsets up to the current one

        for( const auto & anoffset : listoffsetptrs )
        {
            uint32_t offsetToEncode = anoffset - offsetSoFar;
            offsetSoFar = anoffset; //set the value to the latest offset, so we can properly subtract it from the next offset.

            //Encode every bytes of the 4 bytes integer we have to
            for( int32_t i = 4; i > 0; --i )
            {
                uint8_t currentbyte = ( offsetToEncode >> (7 * (i - 1)) ) & 0x7Fu;
                if( currentbyte != 0 )
                {
                    //If its the last byte to chain, leave the highest bit to 0 !
                    if( i == 1 )
                        out_encoded.push_back( currentbyte );
                    else
                        out_encoded.push_back( currentbyte | 0x80u ); //Set the highest bit to 1, to signifie that the next byte must be chained
                }
            }
        }

        //Append the closing 0
        out_encoded.push_back(0);
    }

    std::vector<uint8_t> EncodeSIR0PtrOffsetList( const std::vector<uint32_t> &listoffsetptrs )
    {
        vector<uint8_t> encodedptroffsets( 2u + listoffsetptrs.size() );  //Worst case scenario allocation
        encodedptroffsets.resize(0); //preserve alloc, allow push backs
        EncodeSIR0PtrOffsetList(listoffsetptrs,encodedptroffsets);
        return std::move(encodedptroffsets);
    }

    std::vector<uint32_t> DecodeSIR0PtrOffsetList( const std::vector<uint8_t>  &ptroffsetslst )
    {
        vector<uint32_t> decodedptroffsets( ptroffsetslst.size() ); //worst case scenario
        decodedptroffsets.resize(0);

        auto itcurbyte  = ptroffsetslst.begin();
        auto itlastbyte = ptroffsetslst.end();

        uint32_t offsetsum = 0; //This is used to sum up all offsets and obtain the offset relative to the file, and not the last offset
        uint32_t buffer    = 0; //temp buffer to assemble longer offsets
        uint8_t curbyte    = *itcurbyte;

        while( itcurbyte != itlastbyte && (curbyte = *itcurbyte) != 0 )
        {
            buffer |= curbyte & 0x7Fu;
        
            if( (0x80u & curbyte) != 0 )
            {
                buffer <<= 7u;
            }
            else
            {
                offsetsum += buffer;
                decodedptroffsets.push_back(offsetsum);
                buffer = 0;
            }
        
            ++itcurbyte;
        }

        return std::move(decodedptroffsets);
    }

//========================================================================================================
//  sir0_rule
//========================================================================================================

    /*
        sir0_rule
            Rule for identifying SIR0 content. With the ContentTypeHandler!

            Inherit from this in any file structures contained within a 
            SIR0 container.
    */
    //class sir0_rule : public IContentHandlingRule
    //{
    //public:
    //    sir0_rule(){}
    //    ~sir0_rule(){}

    //    //Returns the value from the content type enum to represent what this container contains!
    //    virtual e_ContentType getContentType()const;

    //    //Returns an ID number identifying the rule. Its not the index in the storage array,
    //    // because rules can me added and removed during exec. Thus the need for unique IDs.
    //    //IDs are assigned on registration of the rule by the handler.
    //    content_rule_id_t getRuleID()const;
    //    void              setRuleID( content_rule_id_t id );

    //    //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
    //    //## This method will call "CContentHandler::AnalyseContent()" for each sub-content container found! ##
    //    //virtual ContentBlock Analyse( types::constitbyte_t   itdatabeg, 
    //    //                              types::constitbyte_t   itdataend );
    //    virtual ContentBlock Analyse( const filetypes::analysis_parameter & parameters );

    //    //This method is a quick boolean test to determine quickly if this content handling
    //    // rule matches, without in-depth analysis.
    //    virtual bool isMatch(  types::constitbyte_t   itdatabeg, 
    //                           types::constitbyte_t   itdataend,
    //                           const std::string    & filext);

    //private:
    //    content_rule_id_t m_myID;
    //};


    ////Returns the value from the content type enum to represent what this container contains!
    //e_ContentType sir0_rule::getContentType()const
    //{
    //    return e_ContentType::SIR0_CONTAINER;
    //}

    ////Returns an ID number identifying the rule. Its not the index in the storage array,
    //// because rules can me added and removed during exec. Thus the need for unique IDs.
    ////IDs are assigned on registration of the rule by the handler.
    //content_rule_id_t sir0_rule::getRuleID()const
    //{
    //    return m_myID;
    //}
    //void sir0_rule::setRuleID( content_rule_id_t id )
    //{
    //    m_myID = id;
    //}

    ////This method returns the content details about what is in-between "itdatabeg" and "itdataend".
    ////## This method will call "CContentHandler::AnalyseContent()" for each sub-content container found! ##
    //ContentBlock sir0_rule::Analyse( const filetypes::analysis_parameter & parameters )
    //{
    //    sir0_header headr;
    //    ContentBlock cb;

    //    //#FIXME: we should do something about other formats that have an SIR0 header ! So they don't get SIR0-ed !

    //    //Read the header
    //    headr.ReadFromContainer( parameters._itdatabeg );

    //    //build our content block info
    //    cb._startoffset          = 0;
    //    cb._endoffset            = headr.eofptr;
    //    cb._rule_id_that_matched = getRuleID();
    //    cb._type                 = getContentType();

    //    //Try to guess what is the subcontent
    //    analysis_parameter paramtopass( parameters._itparentbeg + headr.subheaderptr,  
    //                                    parameters._itparentbeg + headr.eofptr, 
    //                                    parameters._itparentbeg, 
    //                                    parameters._itparentend );

    //    ContentBlock subcontent = CContentHandler::GetInstance().AnalyseContent( paramtopass );
    //    subcontent._startoffset = headr.subheaderptr; //Set the actual start offset!
    //    cb._hierarchy.push_back( subcontent );

    //    return cb;
    //}

    ////This method is a quick boolean test to determine quickly if this content handling
    //// rule matches, without in-depth analysis.
    //bool sir0_rule::isMatch( types::constitbyte_t itdatabeg, types::constitbyte_t itdataend, const std::string & filext )
    //{
    //    using namespace magicnumbers;
    //    return std::equal( SIR0_MAGIC_NUMBER.begin(), SIR0_MAGIC_NUMBER.end(), itdatabeg );
    //}

//========================================================================================================
//  sir0_rule_rule_registrator
//========================================================================================================
    //RuleRegistrator<sir0_rule> RuleRegistrator<sir0_rule>::s_instance;

};};