#include "wtu.hpp"
#include <ppmdu/pmd2/pmd2_filetypes.hpp>
#include <types/content_type_analyser.hpp>
using namespace filetypes;
using namespace std;

namespace filetypes
{
    const ContentTy CnTy_WTU{"wtu"}; //Content ID handle
//

//========================================================================================================
//  wtu_rule
//========================================================================================================
    /*
        wtu_rule
            Rule for identifying WTU content. With the ContentTypeHandler!
    */
    class wtu_rule : public IContentHandlingRule
    {
    public:
        wtu_rule(){}
        ~wtu_rule(){}

        //Returns the value from the content type enum to represent what this container contains!
        virtual cnt_t getContentType()const { return CnTy_WTU; }

        //Returns an ID number identifying the rule. Its not the index in the storage array,
        // because rules can me added and removed during exec. Thus the need for unique IDs.
        //IDs are assigned on registration of the rule by the handler.
        virtual cntRID_t getRuleID()const                  { return m_myID; }
        virtual void              setRuleID( cntRID_t id ) { m_myID = id; }

        //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
        virtual ContentBlock Analyse( const analysis_parameter & parameters )
        {
            //#TODO: Do something with this method, its just dumb and not accomplishing much right now! 
            ContentBlock cb;

            cb._startoffset          = 0;
            cb._endoffset            = std::distance( parameters._itdatabeg, parameters._itdataend );
            cb._rule_id_that_matched = getRuleID();
            cb._type                 = getContentType();

            return cb;
        }

        //This method is a quick boolean test to determine quickly if this content handling
        // rule matches, without in-depth analysis.
        virtual bool isMatch(  vector<uint8_t>::const_iterator   itdatabeg, 
                               vector<uint8_t>::const_iterator   itdataend,
                               const std::string    & filext )
        {
            WTU_header myhead;
            myhead.ReadFromContainer( itdatabeg, itdataend );
            return myhead.magic == WTU_MAGIC_NUMBER_INT;
        }

    private:
        cntRID_t m_myID;
    };

//========================================================================================================
//  wtu_rule_registrator
//========================================================================================================
    /*
        wtu_rule_registrator
            A small singleton that has for only task to register the wtu_rule!
    */
    RuleRegistrator<wtu_rule> RuleRegistrator<wtu_rule>::s_instance;
};

