#include "wte.hpp"
#include <ppmdu/fmts/content_type_analyser.hpp>

using namespace std;

namespace pmd2 {namespace filetypes
{

//========================================================================================================
//  wte_rule
//========================================================================================================
    /*
        wte_rule
            Rule for identifying WTE content. With the ContentTypeHandler!
    */
    class wte_rule : public IContentHandlingRule
    {
    public:
        wte_rule(){}
        ~wte_rule(){}

        //Returns the value from the content type enum to represent what this container contains!
        virtual e_ContentType getContentType()const { return e_ContentType::WTE_FILE; }

        //Returns an ID number identifying the rule. Its not the index in the storage array,
        // because rules can me added and removed during exec. Thus the need for unique IDs.
        //IDs are assigned on registration of the rule by the handler.
        virtual content_rule_id_t getRuleID()const                  { return m_myID; }
        virtual void              setRuleID( content_rule_id_t id ) { m_myID = id; }

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
        virtual bool isMatch(  types::constitbyte_t   itdatabeg, 
                               types::constitbyte_t   itdataend,
                               const std::string    & filext )
        {
            sir0_header mysir0hdr;
            WTE_header myhead;
            try
            {
                mysir0hdr.ReadFromContainer( itdatabeg );
                if( mysir0hdr.magic == magicnumbers::SIR0_MAGIC_NUMBER_INT )
                {
                    myhead.ReadFromContainer( (itdatabeg + mysir0hdr.subheaderptr) );
                    return myhead.magic == WTE_MAGIC_NUMBER_INT;
                }
            }
            catch(...)
            {
                return false;
            }

            return false;
        }

    private:
        content_rule_id_t m_myID;
    };

//========================================================================================================
//  wte_rule_registrator
//========================================================================================================
    /*
        wte_rule_registrator
            A small singleton that has for only task to register the wte_rule!
    */
    RuleRegistrator<wte_rule> RuleRegistrator<wte_rule>::s_instance;
};};