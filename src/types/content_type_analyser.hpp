#ifndef CONTENT_TYPE_ANALYSER_HPP
#define CONTENT_TYPE_ANALYSER_HPP
/*
content_type_analyser.hpp
10/07/2014
psycommando@gmail.com
Description: Handler to analyse data content and return the type of content it is.
             New handling rules and new handlers can be added by registering them into the singleton.

License: Creative Common 0 ( Public Domain ) https://creativecommons.org/publicdomain/zero/1.0/
All wrongs reversed, no crappyrights :P
*/
#include <vector>
#include <types/contentid_generator.hpp>
#include <memory>

namespace filetypes
{

//==================================================================
// Typedefs
//==================================================================
    typedef unsigned int cntRID_t;  //Registered content index
    //typedef unsigned int cnt_t;     //Content Type ID. 0 is reserved!

//==================================================================
// Structs
//==================================================================

    struct analysis_parameter
    {
        typedef std::vector<uint8_t>::const_iterator iterator_t;

        //Init with only two iterators
        analysis_parameter( iterator_t itbegdata, iterator_t itenddata, const std::string & filext = "" )
            :_itdatabeg(itbegdata), _itdataend(itenddata), _itparentbeg(itbegdata), _itparentend(itenddata), _filextension(filext)
        {}

        //Init with 4 parameters
        analysis_parameter( iterator_t itbegdata,   iterator_t itenddata, 
                            iterator_t itparentbeg, iterator_t itparentend )
            :_itdatabeg(itbegdata), _itdataend(itenddata), _itparentbeg(itparentbeg), _itparentend(itparentend)
        {}


        iterator_t  _itparentbeg, //Beginning and End of the parent container, if there is a parent container 
                    _itparentend;
        iterator_t  _itdatabeg, //Beginning and End of the container to analyse
                    _itdataend;

        std::string _filextension; //The filextension of the containing file, if available
    };

    /*
        Parameter struct passed to the content analysis method.
    */
    //template< class _init > 
    //    struct AnalzParam : public analysis_parameter
    //{
    //    typedef _init iterator_t;

    //    //Init with only two iterators
    //    AnalzParam( _init itbegdata, _init itenddata, const std::string & filext = "" )
    //        :_itdatabeg(itbegdata), _itdataend(itenddata), _itparentbeg(itbegdata), _itparentend(itenddata), _filextension(filext)
    //    {}

    //    //Init with 4 parameters
    //    AnalzParam( _init itbegdata,   _init itenddata, 
    //                _init itparentbeg, _init itparentend )
    //        :_itdatabeg(itbegdata), _itdataend(itenddata), _itparentbeg(itparentbeg), _itparentend(itparentend)
    //    {}


    //    iterator_t  _itparentbeg, //Beginning and End of the parent container, if there is a parent container 
    //                _itparentend;
    //    iterator_t  _itdatabeg, //Beginning and End of the container to analyse
    //                _itdataend;

    //    std::string _filextension; //The filextension of the containing file, if available
    //};


    /*************************************************************************************
        ContentBlock
            This represents a node I guess, from the hierarchical structure of
            the containter. Contains the container type+info, and the sub
            containers's types+info.
    *************************************************************************************/
    struct ContentBlock
    {
        ContentBlock( cnt_t  type      = 0,
                      size_t begoffset = 0,
                      size_t endoffset = 0 )
        :_type(type), _startoffset(begoffset), _endoffset(endoffset), _rule_id_that_matched(0)
        {
        }

        //Take all the info in the struct and put it in a string to be read by humans(debug output / console screen)
        std::string printToHumanReadable();

        cnt_t                     _type;        //The type of the ContentBlock
        std::vector<ContentBlock> _hierarchy; //#TODO: DEPRECATE THIS!   //Sub-containers / Sub-content. If not containing sub-elements, main contain data from the
                                                // other same level elements if its the first analysed!
        size_t                    _startoffset; //The position the content begins at, including the header
        size_t                    _endoffset;   //The end of the content block, or the end of the whole container if not applicable
        cntRID_t                  _rule_id_that_matched;
    };

//==================================================================
// Classes
//==================================================================
    /*************************************************************************************
        IContentHandlingRule
            This is a prototype class that all new rules for deciding 
            what to do with content having its own header + content type.
    *************************************************************************************/
    class IContentHandlingRule
    {
    public:
        virtual ~IContentHandlingRule(){}

        //Returns the value from the content type enum to represent what this container contains!
        virtual cnt_t getContentType()const = 0;

        //Returns an ID number identifying the rule. Its not the index in the storage array,
        // because rules can me added and removed during exec. Thus the need for unique IDs.
        //IDs are assigned on registration of the rule by the handler.
        virtual cntRID_t getRuleID()const             = 0;
        virtual void              setRuleID( cntRID_t id ) = 0;

        //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
        //## This method will call "CContentHandler::AnalyseContent()" for each sub-content container found! ##
        virtual ContentBlock Analyse( const analysis_parameter & parameters ) = 0; //#TODO: improve this to give actually useful data!

        //This method is a quick boolean test to determine quickly if this content handling
        // rule matches, without in-depth analysis.
        virtual bool isMatch(  std::vector<uint8_t>::const_iterator   itdatabeg, 
                               std::vector<uint8_t>::const_iterator   itdataend,
                               const std::string                     & filext ) = 0;
    };


    /*************************************************************************************
        CContentHandler
            Singleton handling content analysis.
    *************************************************************************************/
    class CContentHandler
    {
    public:
        static const cntRID_t INVALID_RULE_ID = 0u; //The ruleid for invalid rules. Is used when no valid rules were found

        ~CContentHandler();

        //Use this to get the instance of the Content handler
        //  It also instantiate the singleton, guarantying that the object 
        //  exists when called!
        static CContentHandler & GetInstance();

        //Rule registration handling
        cntRID_t RegisterRule  ( IContentHandlingRule * rule );
        bool     UnregisterRule( cntRID_t ruleid );
        bool     isValidRule   ( cntRID_t theid )const;

        /*
            Use this to increment the internal ruleid counter in order to
            hand out unique rule ids to sub-formats.

            Returns the value of the internal ruleid counter before being incremented!
        */
       // cntRID_t ReserveRuleIDs( unsigned int nbToReserve );

        //Content analysis
        //ContentBlock AnalyseContent( vector<uint8_t>::const_iterator itdatabeg, vector<uint8_t>::const_iterator itdataend );
        ContentBlock AnalyseContent( const analysis_parameter & parameters );

    private:
        CContentHandler(); //no contruction for outsiders
        CContentHandler( const CContentHandler & ); //no copy

        //The list of rules 
        std::vector< std::unique_ptr<IContentHandlingRule> > m_vRules;

        //The current rule id counter, for assigning ruleids
        cntRID_t m_current_ruleid;
    };

    /*************************************************************************************
        RuleRegistrator
            A small singleton that has for only task to register the rule.
            Just call the constructor in your cpp files, with the type of
            the rule as parameter!

            Example:
                RuleRegistrator<ruletypename> RuleRegistrator<ruletypename>::s_instance;
    *************************************************************************************/
    template<class RULE_T> class RuleRegistrator
    {
    public:
        RuleRegistrator()
        {
            CContentHandler::GetInstance().RegisterRule( new RULE_T );
        }

    private:
        static RuleRegistrator<RULE_T> s_instance;
    };

//==================================================================
// Functions
//==================================================================

    /*
        Use this function to avoid having to type this everytimes:
            CContentHandler::GetInstance().AnalyseContent(analysis_parameter(...))
    */
    inline ContentBlock DetermineCntTy( std::vector<uint8_t>::const_iterator  itbegdata, 
                                        std::vector<uint8_t>::const_iterator  itenddata, 
                                        const std::string                    &filext = "" )
    {
        return CContentHandler::GetInstance().AnalyseContent( analysis_parameter(itbegdata,itenddata,filext) );
    }

};

#endif