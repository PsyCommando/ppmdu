#include "wan.hpp"
#include <ppmdu/fmts/content_type_analyser.hpp>
#include <ppmdu/basetypes.hpp>
#include <algorithm>
#include <sstream>
#include <iomanip>

using namespace std;

namespace pmd2{ namespace filetypes
{
//=============================================================================================
//  WAN File Specifics
//=============================================================================================

//============================================
//              wan_sub_header
//============================================
    std::vector<uint8_t>::iterator wan_sub_header::WriteToContainer(  std::vector<uint8_t>::iterator itwriteto )const
    {
        itwriteto = utils::WriteIntToByteVector( ptr_infochunk,   itwriteto );
        itwriteto = utils::WriteIntToByteVector( ptr_frameschunk, itwriteto );
        itwriteto = utils::WriteIntToByteVector( unknown0,        itwriteto );
        itwriteto = utils::WriteIntToByteVector( unknown1,        itwriteto );
        return itwriteto;
    }

    std::vector<uint8_t>::const_iterator wan_sub_header::ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom )
    {
        ptr_infochunk   = utils::ReadIntFromByteVector<decltype(ptr_infochunk)>  (itReadfrom);
        ptr_frameschunk = utils::ReadIntFromByteVector<decltype(ptr_frameschunk)>(itReadfrom);
        unknown0        = utils::ReadIntFromByteVector<decltype(unknown0)>       (itReadfrom);
        unknown1        = utils::ReadIntFromByteVector<decltype(unknown1)>       (itReadfrom);
        return itReadfrom;
    }

    std::string wan_sub_header::toString()const
    {
        stringstream strs;
        //<<setfill('.') <<setw(24) <<left 
        strs <<setfill('.') <<setw(24) <<left <<"Offset sprite info" <<": 0x" <<setfill('0') <<setw(8) <<right <<uppercase <<hex <<ptr_infochunk   <<nouppercase <<"\n"
             <<setfill('.') <<setw(24) <<left <<"Offset frame data"  <<": 0x" <<setfill('0') <<setw(8) <<right <<uppercase <<hex <<ptr_frameschunk <<nouppercase <<"\n"
             <<setfill('.') <<setw(24) <<left <<"Unknown value0"     <<": 0x" <<setfill('0') <<setw(4) <<right <<uppercase <<hex <<unknown0        <<nouppercase <<"\n"
             <<setfill('.') <<setw(24) <<left <<"Unknown value1"     <<": 0x" <<setfill('0') <<setw(4) <<right <<uppercase <<hex <<unknown1        <<nouppercase <<"\n";
        return strs.str();
    }


//============================================
//              wan_frame_data
//============================================
    std::vector<uint8_t>::iterator wan_frame_data::WriteToContainer(  std::vector<uint8_t>::iterator itwriteto )const
    {
        itwriteto = utils::WriteIntToByteVector( ptr_frm_ptrs_table,     itwriteto );
        itwriteto = utils::WriteIntToByteVector( ptr_palette,            itwriteto );
        itwriteto = utils::WriteIntToByteVector( unkn_1,                 itwriteto );
        itwriteto = utils::WriteIntToByteVector( unkn_2,                 itwriteto );
        itwriteto = utils::WriteIntToByteVector( unkn_3,                 itwriteto );
        itwriteto = utils::WriteIntToByteVector( nb_ptrs_frm_ptrs_table, itwriteto );
        return itwriteto;
    }

    std::vector<uint8_t>::const_iterator wan_frame_data::ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom )
    {
        ptr_frm_ptrs_table     = utils::ReadIntFromByteVector<decltype(ptr_frm_ptrs_table)>    (itReadfrom);
        ptr_palette            = utils::ReadIntFromByteVector<decltype(ptr_palette)>           (itReadfrom);
        unkn_1                 = utils::ReadIntFromByteVector<decltype(unkn_1)>                (itReadfrom);
        unkn_2                 = utils::ReadIntFromByteVector<decltype(unkn_2)>                (itReadfrom);
        unkn_3                 = utils::ReadIntFromByteVector<decltype(unkn_3)>                (itReadfrom);
        nb_ptrs_frm_ptrs_table = utils::ReadIntFromByteVector<decltype(nb_ptrs_frm_ptrs_table)>(itReadfrom);
        return itReadfrom;
    }

    std::string wan_frame_data::toString()const
    {
        stringstream strs;
        strs <<setfill('.') <<setw(24) <<left <<"Offset frame ptr table" <<": 0x" <<setfill('0') <<setw(8) <<right <<uppercase <<hex <<ptr_frm_ptrs_table <<nouppercase <<"\n"
             <<setfill('.') <<setw(24) <<left <<"Offset palette end"     <<": 0x" <<setfill('0') <<setw(8) <<right <<uppercase <<hex <<ptr_palette        <<nouppercase <<"\n"
             <<setfill('.') <<setw(24) <<left <<"Unknown #1"             <<": 0x" <<setfill('0') <<setw(4) <<right <<uppercase <<hex <<unkn_1             <<nouppercase <<"\n"
             <<setfill('.') <<setw(24) <<left <<"Unknown #2"             <<": 0x" <<setfill('0') <<setw(4) <<right <<uppercase <<hex <<unkn_2             <<nouppercase <<"\n"
             <<setfill('.') <<setw(24) <<left <<"Unknown #3"             <<": 0x" <<setfill('0') <<setw(4) <<right <<uppercase <<hex <<unkn_3             <<nouppercase <<"\n"
             <<setfill('.') <<setw(24) <<left <<"Nb frames"              <<": "   <<right        <<dec     <<nb_ptrs_frm_ptrs_table           <<"\n";
        return strs.str();
    }

//============================================
// wan_info_data
//============================================

    std::vector<uint8_t>::iterator wan_info_data::WriteToContainer(  std::vector<uint8_t>::iterator itwriteto )const
    {
        itwriteto = utils::WriteIntToByteVector( ptr_ptrstable_e,       itwriteto );
        itwriteto = utils::WriteIntToByteVector( ptr_offset_f,          itwriteto );
        itwriteto = utils::WriteIntToByteVector( ptr_offset_g,          itwriteto );
        itwriteto = utils::WriteIntToByteVector( nb_blocks_in_offset_g, itwriteto );
        itwriteto = utils::WriteIntToByteVector( nb_entries_offset_e,   itwriteto );
        itwriteto = utils::WriteIntToByteVector( unknown1,              itwriteto );
        itwriteto = utils::WriteIntToByteVector( unknown2,              itwriteto );
        itwriteto = utils::WriteIntToByteVector( unknown3,              itwriteto );
        itwriteto = utils::WriteIntToByteVector( unknown4,              itwriteto );
        return itwriteto;
    }

    std::vector<uint8_t>::const_iterator wan_info_data::ReadFromContainer( std::vector<uint8_t>::const_iterator itReadfrom )
    {
        ptr_ptrstable_e       = utils::ReadIntFromByteVector<decltype(ptr_ptrstable_e)>      (itReadfrom);
        ptr_offset_f          = utils::ReadIntFromByteVector<decltype(ptr_offset_f)>         (itReadfrom);
        ptr_offset_g          = utils::ReadIntFromByteVector<decltype(ptr_offset_g)>         (itReadfrom);
        nb_blocks_in_offset_g = utils::ReadIntFromByteVector<decltype(nb_blocks_in_offset_g)>(itReadfrom);
        nb_entries_offset_e   = utils::ReadIntFromByteVector<decltype(nb_entries_offset_e)>  (itReadfrom);
        unknown1              = utils::ReadIntFromByteVector<decltype(unknown1)>             (itReadfrom);
        unknown2              = utils::ReadIntFromByteVector<decltype(unknown2)>             (itReadfrom);
        unknown3              = utils::ReadIntFromByteVector<decltype(unknown3)>             (itReadfrom);
        unknown4              = utils::ReadIntFromByteVector<decltype(unknown4)>             (itReadfrom);
        return itReadfrom;
    }

    std::string wan_info_data::toString()const
    {
        stringstream strs;
        strs <<setfill('.') <<setw(24) <<left 
                <<"Offset table E"    <<": 0x" <<right <<setfill('0') <<setw(8) <<hex <<uppercase << ptr_ptrstable_e <<nouppercase <<"\n"

                <<setfill('.') <<setw(24) <<left 
                <<"Offset table F"    <<": 0x" <<right <<setfill('0') <<setw(8) <<hex <<uppercase << ptr_offset_f    <<nouppercase <<"\n"

                <<setfill('.') <<setw(24) <<left 
                <<"Offset table G"    <<": 0x" <<right <<setfill('0') <<setw(8) <<hex <<uppercase << ptr_offset_g    <<nouppercase <<"\n"

                <<setfill('.') <<setw(24) <<left 
                <<"NbEntries table G" <<": "   <<right <<dec          << nb_blocks_in_offset_g           <<"\n"

                <<setfill('.') <<setw(24) <<left 
                <<"Unknown"           <<": "   <<right <<dec          << nb_entries_offset_e             <<"\n"

                <<setfill('.') <<setw(24) <<left 
                <<"Unknown #1"        <<": 0x" <<right <<setfill('0') <<setw(4) <<hex <<uppercase << unknown1        <<nouppercase <<"\n"

                <<setfill('.') <<setw(24) <<left 
                <<"Unknown #2"        <<": 0x" <<right <<setfill('0') <<setw(4) <<hex <<uppercase << unknown2        <<nouppercase <<"\n"

                <<setfill('.') <<setw(24) <<left 
                <<"Unknown #3"        <<": 0x" <<right <<setfill('0') <<setw(4) <<hex <<uppercase << unknown3        <<nouppercase <<"\n"

                <<setfill('.') <<setw(24) <<left 
                <<"Unknown #4"        <<": 0x" <<right <<setfill('0') <<setw(4) <<hex <<uppercase << unknown4        <<nouppercase <<"\n";
        return strs.str();
    }

//=============================================================================================
//  WAN Identification Rules
//=============================================================================================
    /*
        wan_rule
            Rule for identifying WAN content. With the ContentTypeHandler!
    */
    class wan_rule : public IContentHandlingRule
    {
    public:
        wan_rule(){}
        ~wan_rule(){}

        //Returns the value from the content type enum to represent what this container contains!
        virtual e_ContentType getContentType()const;

        //Returns an ID number identifying the rule. Its not the index in the storage array,
        // because rules can me added and removed during exec. Thus the need for unique IDs.
        //IDs are assigned on registration of the rule by the handler.
        content_rule_id_t getRuleID()const;
        void              setRuleID( content_rule_id_t id );

        //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
        //## This method will call "CContentHandler::AnalyseContent()" for each sub-content container found! ##
        virtual ContentBlock Analyse( const filetypes::analysis_parameter & parameters );

        //This method is a quick boolean test to determine quickly if this content handling
        // rule matches, without in-depth analysis.
        virtual bool isMatch(  types::constitbyte_t   itdatabeg, 
                               types::constitbyte_t   itdataend,
                               const std::string    & filext);

    private:
        content_rule_id_t m_myID;
    };


    //Returns the value from the content type enum to represent what this container contains!
    e_ContentType wan_rule::getContentType()const
    {
        return e_ContentType::WAN_SPRITE_CONTAINER;
    }

    //Returns an ID number identifying the rule. Its not the index in the storage array,
    // because rules can me added and removed during exec. Thus the need for unique IDs.
    //IDs are assigned on registration of the rule by the handler.
    content_rule_id_t wan_rule::getRuleID()const
    {
        return m_myID;
    }
    void wan_rule::setRuleID( content_rule_id_t id )
    {
        m_myID = id;
    }

    //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
    ContentBlock wan_rule::Analyse( const filetypes::analysis_parameter & parameters )
    {
        sir0_header headr;
        ContentBlock cb;

        //Attempt reading the SIR0 header
        headr.ReadFromContainer( parameters._itdatabeg );

        //build our content block info
        cb._startoffset          = 0;
        cb._endoffset            = headr.eofptr;
        cb._rule_id_that_matched = getRuleID();
        cb._type                 = getContentType();

        return cb;
    }

    //This method is a quick boolean test to determine quickly if this content handling
    // rule matches, without in-depth analysis.
    bool wan_rule::isMatch( types::constitbyte_t itdatabeg, types::constitbyte_t itdataend, const std::string & filext )
    {
        using namespace magicnumbers;
        static const unsigned int MaxSubHeaderLen = 27u; //The maximum length of the sub-header + Padding
        sir0_header headr;

        //Check header
        headr.ReadFromContainer( itdatabeg );

        //Check magic number and make sure ptrs aren't null, or invalid.
        if( headr.magic != SIR0_MAGIC_NUMBER_INT || headr.eofptr <= 0x10 || headr.subheaderptr <= 0x10 )
            return false;

        //Literally the best check we can do ^^;
        unsigned int lengthsofar = 0;
        for( auto itcount = itdatabeg + headr.subheaderptr; itcount != itdataend && lengthsofar <= MaxSubHeaderLen; ++lengthsofar, ++itcount )

        //It can't be longer than 26, if the last field ends up on the line below..
        // -- -- -- -- -- -- 01 01 01 01 02 02 02 02 03 03
        // 04 04 AA AA AA AA AA AA AA AA AA AA AA AA AA AA
        if( lengthsofar > MaxSubHeaderLen ) //set to 27, in the very unlikely case that it wouldn't be aligned on the field's size..
        {
            return false;
        }
        else if( lengthsofar == wan_sub_header::DATA_LEN )
        {
            return true;
        }
        else if( lengthsofar > wan_sub_header::DATA_LEN )
        {
            types::constitbyte_t itsearch = itdatabeg;
            std::advance( itsearch, wan_sub_header::DATA_LEN );
            return std::all_of( itsearch, itdataend, []( uint8_t val ){ return val == pmd2::filetypes::COMMON_PADDING_BYTE; } );
        }

        return false;
    }

//=============================================================================================
//  WAN Identification Rules Registration
//=============================================================================================
    RuleRegistrator<wan_rule> RuleRegistrator<wan_rule>::s_instance;
};};