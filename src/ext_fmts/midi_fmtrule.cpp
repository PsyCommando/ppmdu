#include "midi_fmtrule.hpp"
#include <cstdint>
#include <utils/gbyteutils.hpp>
using namespace std;

static const uint32_t MIDI_MagicNum = 0x4D546864;

#ifdef USE_PPMDU_CONTENT_TYPE_ANALYSER
    #include <types/content_type_analyser.hpp>

    namespace filetypes
    {
        const ContentTy CnTy_MIDI{"midi"}; //Content ID db handle

    //========================================================================================================
    //  midi_rule
    //========================================================================================================
        /*
            midi_rule
                Rule for identifying a SMDL file. With the ContentTypeHandler!
        */
        class midi_rule : public IContentHandlingRule
        {
        public:
            midi_rule(){}
            ~midi_rule(){}

            //Returns the value from the content type enum to represent what this container contains!
            virtual cnt_t getContentType()const
            {
                return CnTy_MIDI;
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
                size_t       fsize = std::distance( parameters._itparentbeg, parameters._itparentend );
                ContentBlock cb;
                //build our content block info 
                cb._startoffset          = 0;
                cb._endoffset            = fsize;
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
                return (utils::ReadIntFromBytes<uint32_t>(itdatabeg, itdataend, false) == MIDI_MagicNum);
            }

        private:
            cntRID_t m_myID;
        };

    //========================================================================================================
    //  midi_rule_registrator
    //========================================================================================================
        /*
            midi_rule_registrator
                A small singleton that has for only task to register the midi_rule!
        */
        RuleRegistrator<midi_rule> RuleRegistrator<midi_rule>::s_instance;
    };
#endif