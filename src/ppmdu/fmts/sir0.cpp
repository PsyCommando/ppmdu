/*
sir0.cpp
20/05/2014
psycommando@gmail.com

Description:

No crappyrights. All wrongs reversed ! 
*/
#include <ppmdu/fmts/sir0.hpp>

#include <types/content_type_analyser.hpp>
#include <utils/utility.hpp>
#include <utils/gbyteutils.hpp>
#include <utils/handymath.hpp>
#include <string>
#include <sstream>
#include <iomanip>
#include <array>
using namespace std;
using namespace utils;

namespace filetypes
{
    static const uint8_t SIR0_EncodedOffsetsHeader = 0x04u;
    const ContentTy      CnTy_SIR0 {"sir0"}; 
//========================================================================================================
// sir0_header
//========================================================================================================

    string sir0_header::toString()const
    {
        stringstream strs;
        strs <<setfill('.') <<setw(22) <<left 
             <<"Sprite header offset" <<": 0x" <<setfill('0') <<setw(8) <<uppercase <<right <<hex <<subheaderptr <<nouppercase <<"\n"
             <<setfill('.') <<setw(22) <<left 
             <<"End of data offset"   <<": 0x" <<setfill('0') <<setw(8) <<uppercase <<right <<hex <<ptrPtrOffsetLst       <<nouppercase <<"\n";
        return strs.str();
    }

    //std::vector<uint8_t>::iterator sir0_header::WriteToContainer( std::vector<uint8_t>::iterator itwriteto )const
    //{
    //    itwriteto = utils::WriteIntToByteVector( magic,        itwriteto, false );
    //    itwriteto = utils::WriteIntToByteVector( subheaderptr, itwriteto );
    //    itwriteto = utils::WriteIntToByteVector( ptrPtrOffsetLst,       itwriteto );
    //    itwriteto = utils::WriteIntToByteVector( endzero,        itwriteto );
    //    return itwriteto;
    //}

    //std::vector<uint8_t>::const_iterator sir0_header::ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom )
    //{
    //    magic        = utils::ReadIntFromByteVector<decltype(magic)>       (itReadfrom, false );
    //    subheaderptr = utils::ReadIntFromByteVector<decltype(subheaderptr)>(itReadfrom);
    //    ptrPtrOffsetLst       = utils::ReadIntFromByteVector<decltype(ptrPtrOffsetLst)>      (itReadfrom);
    //    endzero        = utils::ReadIntFromByteVector<decltype(endzero)>       (itReadfrom);
    //    return itReadfrom;
    //}

//========================================================================================================
// Utility:
//========================================================================================================

    sir0_head_and_list MakeSIR0ForData( const std::vector<uint32_t> &listoffsetptrs,
                                        uint32_t                     offsetsubheader,
                                        uint32_t                     offsetendofdata )
    {
        sir0_header hdr( MagicNumber_SIR0, 
                         offsetsubheader + sir0_header::HEADER_LEN,
                         offsetendofdata + sir0_header::HEADER_LEN );

        //#TODO: This could be done much better. There's a lot of allocation that could possibly be avoided.
        vector<uint8_t> encodedptroffsets( 2 + listoffsetptrs.size() ); //Worst case scenario allocation
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

    sir0_head_and_list MakeSIR0ForData( uint32_t                     offsetsubheader,
                                        uint32_t                     offsetendofdata )
    {
        static const vector<uint8_t> MinimalEncPtrOffsets = 
        { 
            SIR0_EncodedOffsetsHeader, 
            SIR0_EncodedOffsetsHeader,
            0, //We have to put the zero here manually, since we're entirely skipping encoding!
        };
        sir0_header hdr( MagicNumber_SIR0, 
                         offsetsubheader + sir0_header::HEADER_LEN,
                         offsetendofdata + sir0_header::HEADER_LEN );
        return sir0_head_and_list{ hdr, MinimalEncPtrOffsets };
    }

    //#TODO: Should use a back_inserter here !
    void EncodeSIR0PtrOffsetList( const std::vector<uint32_t> &listoffsetptrs, std::vector<uint8_t> & out_encoded )
    {
        uint32_t offsetSoFar = 0; //used to add up the sum of all the offsets up to the current one

        for( const auto & anoffset : listoffsetptrs )
        {
            uint32_t offsetToEncode        = anoffset - offsetSoFar;
            bool     hasHigherNonZero      = false; //This tells the loop whether it needs to encode null bytes, if at least one higher byte was non-zero
            offsetSoFar = anoffset; //set the value to the latest offset, so we can properly subtract it from the next offset.

            //Encode every bytes of the 4 bytes integer we have to
            for( int32_t i = sizeof(int32_t); i > 0; --i )
            {
                uint8_t currentbyte = ( offsetToEncode >> (7 * (i - 1)) ) & 0x7Fu;
                
                if( i == 1 ) //the lowest byte to encode is special
                {
                    //If its the last byte to append, leave the highest bit to 0 !
                    out_encoded.push_back( currentbyte );
                }
                else if( currentbyte != 0 || hasHigherNonZero ) //if any bytes but the lowest one! If not null OR if we have encoded a higher non-null byte before!
                {
                    //Set the highest bit to 1, to signifie that the next byte must be appended
                    out_encoded.push_back( currentbyte | 0x80u ); 
                    hasHigherNonZero = true;
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
        bool    LastHadBitFlag = false; //This contains whether the byte read on the previous turn of the loop had the bit flag indicating to append the next byte!

        while( itcurbyte != itlastbyte && ( LastHadBitFlag || (*itcurbyte) != 0 ) )
        {
            curbyte = *itcurbyte;
            buffer |= curbyte & 0x7Fu;
        
            if( (0x80u & curbyte) != 0 )
            {
                LastHadBitFlag = true;
                buffer <<= 7u;
            }
            else
            {
                LastHadBitFlag = false;
                offsetsum += buffer;
                decodedptroffsets.push_back(offsetsum);
                buffer = 0;
            }
        
            ++itcurbyte;
        }

        return std::move(decodedptroffsets);
    }

    vector<uint8_t> MakeSIR0Wrap( const vector<uint8_t>    & data, 
                                  const sir0_head_and_list & sir0data, 
                                  uint8_t                    padchar  )
    {
        vector<uint8_t> wrap;
        auto            itbackins = back_inserter(wrap);

        //Write SIR0 header
        sir0data.hdr.WriteToContainer(itbackins);

        //Write data
        wrap.insert( wrap.end(), data.begin(), data.end() );

        //Add padding before the ptr offset list
        AppendPaddingBytes( itbackins, wrap.size(), 16, padchar );

        //Write ptr offset list
        wrap.insert( wrap.end(), sir0data.ptroffsetslst.begin(), sir0data.ptroffsetslst.end() );

        //Add end of file padding
        AppendPaddingBytes( itbackins, wrap.size(), 16, padchar );

        return std::move(wrap);
    }

    vector<uint8_t> MakeSIR0Wrap( const std::vector<uint8_t> & data, uint8_t padchar )
    {
        //Calculate padding first, to ensure the end offset is valid
        uint32_t lenpadding = data.size() % 16;

        //Make the SIR0 data
        sir0_head_and_list sir0data = MakeSIR0ForData( 0, data.size() + lenpadding );

        //Call the function handling everything else
        return MakeSIR0Wrap( data, sir0data, padchar );
    }

    //vector<uint8_t> MakeSIR0Wrap( const std::vector<uint8_t>  & data, 
    //                              uint32_t                      offsetsubheader, 
    //                              const std::vector<uint32_t> & ptroffsetlst )
    //{
    //    //Calculate padding first, to ensure the end offset is valid
    //    uint32_t lenpadding = ( CalcClosestHighestDenominator( data.size(), 16 ) -  data.size() );

    //    //Make the SIR0 data
    //    sir0_head_and_list sir0data = MakeSIR0ForData( ptroffsetlst, offsetsubheader, data.size() + lenpadding );

    //    //Call the function handling everything else
    //    return MakeSIR0Wrap( data, sir0data );
    //}



//========================================================================================================
//  SIR0DerivHandler
//========================================================================================================
    SIR0DerivHandler & SIR0DerivHandler::GetInstance()
    {
        static SIR0DerivHandler s_instance;
        return s_instance;
    }

    //Rule registration handling
    cntRID_t SIR0DerivHandler::RegisterRule( IContentHandlingRule * rule )
    {
        if( rule != nullptr )
        {
            //Check if rule already in!
            for( auto &arule : this->m_rules )
            {
                if( arule.second.get() == rule )
                    return arule.second->getRuleID();
            }

            //If not already in, put it in the rule list!
            //Set the rule id
            
            cntRID_t ridbefore = m_currentRID;
            rule->setRuleID( m_currentRID );
            m_rules.insert( make_pair( m_currentRID, std::unique_ptr<IContentHandlingRule>(rule) ) );
            ++m_currentRID;
            //m_rules.push_back( std::unique_ptr<IContentHandlingRule>( rule ) );

            return ridbefore;
        }

        return std::numeric_limits<unsigned int>::max();// -1;
    }

    bool SIR0DerivHandler::UnregisterRule( cntRID_t ruleid )
    {
        auto itfound = m_rules.find( ruleid );

        if( itfound != m_rules.end() )
        {
            m_rules.erase( itfound );
            return true;
        }

        //for( auto &arule : m_rules )
        //{
        //    if( arule.second->getRuleID() == ruleid )
        //    {
        //        delete arule.second.release();
        //        return true;
        //    }
        //}
        return false;
    }
   
    ContentBlock SIR0DerivHandler::AnalyseContent( const analysis_parameter & parameters )
    {
        ContentBlock contentdetails;

        //Feed the data through all the rules we have and check which one returns true
        for( auto &rule : m_rules )
        {
            if( rule.second->isMatch( parameters._itdatabeg, parameters._itdataend, parameters._filextension ) )
            {
                contentdetails = rule.second->Analyse( parameters );
                break;
            }
        }

        return contentdetails;
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
    class sir0_rule : public IContentHandlingRule
    {
    public:
        sir0_rule(){}
        ~sir0_rule(){}

        //Returns the value from the content type enum to represent what this container contains!
        virtual cnt_t getContentType()const;

        //Returns an ID number identifying the rule. Its not the index in the storage array,
        // because rules can me added and removed during exec. Thus the need for unique IDs.
        //IDs are assigned on registration of the rule by the handler.
        cntRID_t getRuleID()const;
        void              setRuleID( cntRID_t id );

        //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
        //## This method will call "CContentHandler::AnalyseContent()" for each sub-content container found! ##
        //virtual ContentBlock Analyse( vector<uint8_t>::const_iterator   itdatabeg, 
        //                              vector<uint8_t>::const_iterator   itdataend );
        virtual ContentBlock Analyse( const filetypes::analysis_parameter & parameters );

        //This method is a quick boolean test to determine quickly if this content handling
        // rule matches, without in-depth analysis.
        virtual bool isMatch(  vector<uint8_t>::const_iterator   itdatabeg, 
                               vector<uint8_t>::const_iterator   itdataend,
                               const std::string    & filext);

    private:
        cntRID_t m_myID;
    };


    //Returns the value from the content type enum to represent what this container contains!
    cnt_t sir0_rule::getContentType()const
    {
        return CnTy_SIR0;
    }

    //Returns an ID number identifying the rule. Its not the index in the storage array,
    // because rules can me added and removed during exec. Thus the need for unique IDs.
    //IDs are assigned on registration of the rule by the handler.
    cntRID_t sir0_rule::getRuleID()const
    {
        return m_myID;
    }
    void sir0_rule::setRuleID( cntRID_t id )
    {
        m_myID = id;
    }

    //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
    //## This method will call "CContentHandler::AnalyseContent()" for each sub-content container found! ##
    ContentBlock sir0_rule::Analyse( const filetypes::analysis_parameter & parameters )
    {
        sir0_header headr;
        ContentBlock cb;

        //Read the header
        headr.ReadFromContainer( parameters._itdatabeg );

        //build our content block info
        cb._startoffset          = 0;
        cb._endoffset            = headr.ptrPtrOffsetLst;
        cb._rule_id_that_matched = getRuleID();
        cb._type                 = getContentType();

        //Try to guess what is the subcontent
        analysis_parameter paramtopass( parameters._itparentbeg,
                                        parameters._itparentbeg, 
                                        parameters._itparentbeg, 
                                        parameters._itparentend );

        //Try all the rules that are wrapped by an SIR0 !
        ContentBlock subcontent = SIR0DerivHandler::GetInstance().AnalyseContent( paramtopass );
        subcontent._startoffset = headr.subheaderptr; //Set the actual start offset!
        //cb._hierarchy.push_back( subcontent );

        
        if( subcontent._type == CnTy_Invalid )
        {
            return cb; //If we didn't find a valid derivative, just return the standard SIR0 type!
        }
        else
            return subcontent; //If we found a valid derivative, return that!
    }

    //This method is a quick boolean test to determine quickly if this content handling
    // rule matches, without in-depth analysis.
    bool sir0_rule::isMatch( vector<uint8_t>::const_iterator itdatabeg, vector<uint8_t>::const_iterator itdataend, const std::string & filext )
    {
        return ReadIntFromByteVector<uint32_t>(itdatabeg,false) == MagicNumber_SIR0;
    }

//========================================================================================================
//  sir0_rule_rule_registrator
//========================================================================================================
    RuleRegistrator<sir0_rule> RuleRegistrator<sir0_rule>::s_instance;

};