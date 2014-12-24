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
using namespace std;

namespace pmd2 { namespace filetypes
{

//========================================================================================================
// sir0_header
//========================================================================================================
    //uint8_t& sir0_header::operator[](unsigned int pos)
    //{
    //    if( pos < 4 )
    //        return reinterpret_cast<uint8_t*>(&magic)[pos];
    //    else if( pos < 8 )
    //        return reinterpret_cast<uint8_t*>(&subheaderptr)[pos-4];
    //    else if( pos < 12 )
    //        return reinterpret_cast<uint8_t*>(&eofptr)[pos-8];
    //    else if( pos < 16 )
    //        return reinterpret_cast<uint8_t*>(&null)[pos-12];
    //    else
    //        return *reinterpret_cast<uint8_t*>(0); //Crash please
    //}

    //const uint8_t & sir0_header::operator[](unsigned int index)const
    //{
    //    return (*const_cast<sir0_header*>(this))[index];
    //}

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
        virtual e_ContentType getContentType()const;

        //Returns an ID number identifying the rule. Its not the index in the storage array,
        // because rules can me added and removed during exec. Thus the need for unique IDs.
        //IDs are assigned on registration of the rule by the handler.
        content_rule_id_t getRuleID()const;
        void              setRuleID( content_rule_id_t id );

        //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
        //## This method will call "CContentHandler::AnalyseContent()" for each sub-content container found! ##
        //virtual ContentBlock Analyse( types::constitbyte_t   itdatabeg, 
        //                              types::constitbyte_t   itdataend );
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
    e_ContentType sir0_rule::getContentType()const
    {
        return e_ContentType::SIR0_CONTAINER;
    }

    //Returns an ID number identifying the rule. Its not the index in the storage array,
    // because rules can me added and removed during exec. Thus the need for unique IDs.
    //IDs are assigned on registration of the rule by the handler.
    content_rule_id_t sir0_rule::getRuleID()const
    {
        return m_myID;
    }
    void sir0_rule::setRuleID( content_rule_id_t id )
    {
        m_myID = id;
    }

    //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
    //## This method will call "CContentHandler::AnalyseContent()" for each sub-content container found! ##
    ContentBlock sir0_rule::Analyse( const filetypes::analysis_parameter & parameters )
    {
        sir0_header headr;
        ContentBlock cb;

        //#FIXME: we should do something about other formats that have an SIR0 header ! So they don't get SIR0-ed !

        //Read the header
        headr.ReadFromContainer( parameters._itdatabeg );

        //build our content block info
        cb._startoffset          = 0;
        cb._endoffset            = headr.eofptr;
        cb._rule_id_that_matched = getRuleID();
        cb._type                 = getContentType();

        //Try to guess what is the subcontent
        analysis_parameter paramtopass( parameters._itparentbeg + headr.subheaderptr,  
                                        parameters._itparentbeg + headr.eofptr, 
                                        parameters._itparentbeg, 
                                        parameters._itparentend );

        ContentBlock subcontent = CContentHandler::GetInstance().AnalyseContent( paramtopass );
        subcontent._startoffset = headr.subheaderptr; //Set the actual start offset!
        cb._hierarchy.push_back( subcontent );

        return cb;
    }

    //This method is a quick boolean test to determine quickly if this content handling
    // rule matches, without in-depth analysis.
    bool sir0_rule::isMatch( types::constitbyte_t itdatabeg, types::constitbyte_t itdataend, const std::string & filext )
    {
        using namespace magicnumbers;
        return std::equal( SIR0_MAGIC_NUMBER.begin(), SIR0_MAGIC_NUMBER.end(), itdatabeg );
    }

//========================================================================================================
//  at4px_rule_registrator
//========================================================================================================
    /*
        at4px_rule_registrator
            A small singleton that has for only task to register the at4px_rule!
    */
    RuleRegistrator<sir0_rule> RuleRegistrator<sir0_rule>::s_instance;

};};