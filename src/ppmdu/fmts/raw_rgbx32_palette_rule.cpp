#include <types/content_type_analyser.hpp>
#include <utils/utility.hpp>
#include <ppmdu/pmd2/pmd2_palettes.hpp>
using namespace std;

namespace filetypes
{

//========================================================================================================
//  Loose Palette files rules
//========================================================================================================
    /*
        rgbx32_raw_pal_rule
            Rule for identifying WTE content. With the ContentTypeHandler!
    */
    class rgbx32_raw_pal_rule : public filetypes::IContentHandlingRule
    {
    public:
        rgbx32_raw_pal_rule(){}
        ~rgbx32_raw_pal_rule(){}

        //Returns the value from the content type enum to represent what this container contains!
        virtual cnt_t getContentType()const { return CntTy_RawRGBX32; }

        //Returns an ID number identifying the rule. Its not the index in the storage array,
        // because rules can me added and removed during exec. Thus the need for unique IDs.
        //IDs are assigned on registration of the rule by the handler.
        virtual cntRID_t getRuleID()const         { return m_myID; }
        virtual void     setRuleID( cntRID_t id ) { m_myID = id; }

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
            try
            {
                while( itdatabeg != itdataend )
                {
                    uint32_t acolor = utils::ReadIntFromBytes<uint32_t>( itdatabeg, itdataend, false );
                    if( (acolor & pmd2::graphics::RGBX_UNUSED_BYTE_VALUE) == 0 )
                        return false;
                }
            }
            catch(...)
            {
                return false;
            }

            return true;
        }

    private:
        cntRID_t m_myID;
    };

    //========================================================================================================
    //  rgbx32_raw_pal_rule_registrator
    //========================================================================================================
    /*
        wte_rule_registrator
            A small singleton that has for only task to register the wte_rule!
    */
    RuleRegistrator<rgbx32_raw_pal_rule> RuleRegistrator<rgbx32_raw_pal_rule>::s_instance;

};