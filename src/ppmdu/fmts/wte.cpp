#include "wte.hpp"
#include <ppmdu/pmd2/pmd2_filetypes.hpp>
#include <types/content_type_analyser.hpp>
#include <ppmdu/fmts/sir0.hpp>
using namespace filetypes;
using namespace std;

namespace filetypes
{
    const ContentTy CnTy_WTE {"wte"};
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
        virtual cnt_t getContentType()const { return CnTy_WTE; }

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
            sir0_header mysir0hdr;
            WTE_header myhead;
            try
            {
                mysir0hdr.ReadFromContainer( itdatabeg );
                if( mysir0hdr.magic == MagicNumber_SIR0 )
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
        cntRID_t m_myID;
    };

//========================================================================================================
//  wte_rule_registrator
//========================================================================================================
    /*
        wte_rule_registrator
            A small singleton that has for only task to register the wte_rule!
    */
    SIR0RuleRegistrator<wte_rule> SIR0RuleRegistrator<wte_rule>::s_instance;
};

