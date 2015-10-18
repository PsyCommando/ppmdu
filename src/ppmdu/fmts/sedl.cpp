#include "sedl.hpp"
#include <ppmdu/pmd2/pmd2_filetypes.hpp>
#include <types/content_type_analyser.hpp>


using namespace filetypes;
using namespace std;

namespace filetypes
{
    const ContentTy CnTy_SEDL {"sedl"}; //Content ID db handle
};


namespace DSE
{
};

//========================================================================================================
//  sedl_rule
//========================================================================================================
    /*
        sedl_rule
            Rule for identifying a SMDL file. With the ContentTypeHandler!
    */
    class sedl_rule : public IContentHandlingRule
    {
    public:
        sedl_rule(){}
        ~sedl_rule(){}

        //Returns the value from the content type enum to represent what this container contains!
        virtual cnt_t getContentType()const
        {
            return filetypes::CnTy_SEDL;
        }

        //Returns an ID number identifying the rule. Its not the index in the storage array,
        // because rules can me added and removed during exec. Thus the need for unique IDs.
        //IDs are assigned on registration of the rule by the handler.
        virtual cntRID_t getRuleID()const                          { return m_myID; }
        virtual void                      setRuleID( cntRID_t id ) { m_myID = id; }

        //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
        //## This method will call "CContentHandler::AnalyseContent()" for each sub-content container found! ##
        //virtual ContentBlock Analyse( vector<uint8_t>::const_iterator   itdatabeg, 
        //                              vector<uint8_t>::const_iterator   itdataend );
        virtual ContentBlock Analyse( const analysis_parameter & parameters )
        {
            using namespace pmd2::filetypes;
            DSE::SEDL_Header headr;
            ContentBlock cb;

            //Read the header
            headr.ReadFromContainer( parameters._itdatabeg );

            //build our content block info 
            cb._startoffset          = 0;
            cb._endoffset            = headr.flen;
            cb._rule_id_that_matched = getRuleID();
            cb._type                 = getContentType();

            return cb;
        }

        //This method is a quick boolean test to determine quickly if this content handling
        // rule matches, without in-depth analysis.
        virtual bool isMatch(  vector<uint8_t>::const_iterator   itdatabeg, 
                               vector<uint8_t>::const_iterator   itdataend,
                               const std::string & filext)
        {
            using namespace pmd2::filetypes;
            return (utils::ReadIntFromByteVector<uint32_t>(itdatabeg,false) == DSE::SEDL_MagicNumber);
        }

    private:
        cntRID_t m_myID;
    };

//========================================================================================================
//  sedl_rule_registrator
//========================================================================================================
    /*
        sedl_rule_registrator
            A small singleton that has for only task to register the sedl_rule!
    */
    RuleRegistrator<sedl_rule> RuleRegistrator<sedl_rule>::s_instance;
