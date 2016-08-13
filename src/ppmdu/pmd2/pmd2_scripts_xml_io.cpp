#include "pmd2_scripts.hpp"
#include <string>
#include <cassert>
#include <deque>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <utils/utility.hpp>
#include <ppmdu/pmd2/pmd2_scripts_opcodes.hpp>
#include <utils/pugixml_utils.hpp>
#include <utils/library_wide.hpp>
#include <utils/multiple_task_handler.hpp>
#include <ppmdu/fmts/ssb.hpp>
#include <atomic>
#include <thread>
#include <unordered_set>
#include <Poco/DirectoryIterator.h>
#include <Poco/Path.h>
#include <Poco/File.h>
using namespace std;
using namespace pugi;
using namespace pugixmlutils;

//#define PMD2XML_STRING_AS_CDATA
#define PMD2XML_STRING_AS_PCDATA

namespace pmd2
{

    //Return is signed!!!
    inline int16_t Convert14bTo16b(uint16_t val)
    {
        return (val & 0x4000u)? (val | 0xFFFF8000u) : (val & 0x3FFFu);
    }

    //!#TODO: Double check this!!
    inline uint16_t Convert16bTo14b(uint16_t val)
    {
        if(val == 0)
            return 0;
        else if( static_cast<int16_t>(val) < 0)
            return (val & 0x3FFFu) | 0x4000u;     //If the value was negative, set the negative bit
        else
            return val & 0x3FFFu;
    }


    /*
        ToWord
            Checked cast of a 32 bits integer into a 16 bits integer.
    */
    inline uint16_t ToWord( size_t val )
    {
        if( val > std::numeric_limits<uint16_t>::max() )
            throw std::overflow_error("ToWord(): Value is larger than a 16bits integer!!");
        return static_cast<uint16_t>(val);
    }

    inline int16_t ToSWord( long val )
    {
        if( val <= std::numeric_limits<int16_t>::max() && val >= std::numeric_limits<int16_t>::lowest() )
        //if( (val & 0xFFFF0000) == 0 )
            return static_cast<int16_t>(val);
        else
            throw std::overflow_error("ToWord(): Value is larger or smaller than a 16bits integer!!");
    }


    //! #TODO: Make a synchronised text output stack !



//==============================================================================
//  Constants
//==============================================================================
    namespace scriptXML
    {
        const string ROOT_SingleScript  = ScriptXMLRoot_SingleScript; 
        const string ROOT_ScripDir      = ScriptXMLRoot_Level;      
        const string ATTR_GVersion      = CommonXMLGameVersionAttrStr; 
        const string ATTR_GRegion       = CommonXMLGameRegionAttrStr;
        const string ATTR_DirName       = "eventname"s;           //Event name/filename with no extension
        const string ATTR_Name          = "name"s;

        const string NODE_LSDTbl        = "LSDTable"s;
        const string NODE_GrpNameRef    = "Ref"s;            //AKA sub-acting 
        const string ATTR_GrpName       = ATTR_Name;        //AKA sub-acting names

        const string NODE_ScriptGroup   = "ScriptSet"s;
        const string ATTR_ScrGrpName    = ATTR_Name;
        
        // ----------------------------------------------
        // Script
        // ----------------------------------------------
        const string NODE_ScriptSeq     = "ScriptSequence"s;
        const string ATTR_ScrSeqName    = ATTR_Name;

        const string NODE_Constants     = "Constants"s;
        const string NODE_Constant      = "Constant"s;

        const string NODE_Strings       = "Strings"s;
        const string NODE_String        = "String"s;
        const string ATTR_Value         = "value"s;
        const string ATTR_Language      = "language"s;

        const string NODE_Code          = "Code"s;

        //!#TODO: add various group nodes depending on the group type 

        const string NODE_Routine       = "Routine"s;
        const string ATTR_RoutineID     = "_id"s;
        const string ATTR_RoutineType   = "type"s;
        const string ATTR_RoutineParam2 = "unk2"s;

        const string NODE_RoutineAlias  = "AliasPrevRoutine";       //For multiple groups refering to the same memory offset!


        const string NODE_Instruction   = "Instruction"s;
        const string NODE_Data          = "Data"s;               ///For data words

        const string ATTR_Param         = "param"s;


        //META
        const string NODE_MetaLabel     = "Label";
        const string ATTR_LblID         = "id";
        const string NODE_MetaCaseLabel = "CaseLabel";


        //Parameters

        const array<string, 10> ATTR_Params =
        {
            "param",
            "param_1",
            "param_2",
            "param_3",
            "param_4",
            "param_5",
            "param_6",
            "param_7",
            "param_8",
            "param_9",
        };

        // ----------------------------------------------
        // Script Data
        // ----------------------------------------------
        const string ROOT_SingleData        = ScriptDataXMLRoot_SingleDat;
        const string NODE_ScriptData        = "ScriptData"s;
        const string ATTR_ScriptType        = "type"s;           // ssa, sse, sss
        const string ATTR_ScrDatName        = ATTR_Name;

        const string NODE_Layers            = "Layers"s;
        const string NODE_Layer             = "Layer"s;
        const string ATTR_DummyID           = "_id"s;

        const string NODE_Actors            = "Actors"s;
        const string NODE_Actor             = "Actor"s;

        const string NODE_Objects           = "Objects"s;
        const string NODE_Object            = "Object"s;

        const string NODE_Performers        = "Performers"s;
        const string NODE_Performer         = "Performer"s;

        const string NODE_Events            = "Events"s;
        const string NODE_Event             = "Event"s;

        const string NODE_UnkTable3         = "UnkTable3"s;
        const string NODE_UnkTable3Entry    = "Entry"s;

        const string NODE_UnkTable1         = "UnkTable1"s;
        const string NODE_UnkTable1Entry    = "Entry"s;

        const string NODE_PositionMarkers   = "PositionMarkers"s;
        const string NODE_Marker            = "Marker"s;

        const string ATTR_XOffset           = "x"s;
        const string ATTR_YOffset           = "y"s;
        const string ATTR_Direction         = "facing"s;
        const string ATTR_ScriptID          = "act_scriptid"s;

        const string ATTR_UnknownPrintf     = "unk_%d"s;

        const string ATTR_Unk0  = "unk0"s;
        const string ATTR_Unk1  = "unk1"s;
        const string ATTR_Unk2  = "unk2"s;
        const string ATTR_Unk3  = "unk3"s;
        const string ATTR_Unk4  = "unk4"s;
        const string ATTR_Unk5  = "unk5"s;
        const string ATTR_Unk6  = "unk6"s;
        const string ATTR_Unk7  = "unk7"s;
        const string ATTR_Unk8  = "unk8"s;
        const string ATTR_Unk9  = "unk9"s;
        const string ATTR_Unk10 = "unk10"s;

        

    };

//==============================================================================
//  GameScriptsXMLParser
//==============================================================================

    /*****************************************************************************************
        SSBParser
            
    *****************************************************************************************/
    class SSBXMLParser
    {
    public:
        /*
        */
        SSBXMLParser( eGameVersion version, eGameRegion region, const ConfigLoader & conf )
            :m_version(version), 
             m_region(region), 
             m_opinfo(version),
             m_gconf(conf), 
             m_paraminf(conf)
        {}

        /*
        */
        Script operator()( const xml_node & seqn)
        {
            using namespace scriptXML;
            xml_attribute xname = seqn.attribute(ATTR_ScrSeqName.c_str());

            if(!xname)
            {
                stringstream sstr;
                PrintErrorPos(sstr, seqn) << "SSBXMLParser::operator(): Sequence is missing its \"name\" attribute!!";
                throw std::runtime_error(sstr.str());
            }
            xml_node xcode = seqn.child(NODE_Code.c_str());

            m_out = std::move( Script(xname.value()) );
            ParseCode(xcode);
            CheckLabelReferences();
            IncrementAllStringReferencesParameters(); //Increment all strings values by the nb of entries in the const table.

            //Move strings
            m_out.ConstTbl() = std::move( Script::consttbl_t( m_constqueue.begin(), m_constqueue.end() ) );
            for( auto & aq : m_strqueues )
            {
                m_out.StrTblSet().emplace( aq.first, std::move(Script::strtbl_t(aq.second.begin(), aq.second.end())) );
            }
            return std::move(m_out);
        }

    private:


        inline stringstream & PrintErrorPos( stringstream & sstr, const xml_node & errornode )const
        {
            sstr <<m_out.Name() <<", file offset : " <<dec <<nouppercase <<errornode.offset_debug() <<" -> ";
            return sstr;
        }

        /*****************************************************************************************
            IncrementAllStringReferencesParameters
                This increments the value of every single parameters referring to a string id
                we collected so far. It adds the nb of constants to the index, just as the 
                ssb format expects!
        *****************************************************************************************/
        inline void IncrementAllStringReferencesParameters()
        {
            const size_t constblocksz = m_constqueue.size();
            for( const auto & entry : m_stringrefparam )
            {
                (*entry) += ToWord(constblocksz);
            }
        }

        /*****************************************************************************************
            CheckLabelReferences 
                Checks to see if any instruction refers to a non-existing label.
        *****************************************************************************************/
        void CheckLabelReferences()
        {
            for( const auto & aconstref : m_labelchecker )
            {
                if( !aconstref.second.bexists )
                {
                    const string & filename = m_out.Name();
                    stringstream sstr;
                    sstr <<"SSBXMLParser::CheckLabelReferences(): "<<aconstref.second.referersoffsets.size() 
                         <<" reference(s) to non-existant label ID " <<aconstref.first <<" found!!\n";
                    size_t cnt = 0;
                    for(const auto & ref : aconstref.second.referersoffsets)
                        sstr <<"\t*Reference " <<cnt++ <<", \"" <<filename <<"\" offset: " <<ref <<"\n";
                    assert(false);
                    throw std::runtime_error(sstr.str());
                }
            }
        }

        /*****************************************************************************************
        *****************************************************************************************/
        void ParseTypedRoutine(const xml_node & routinen, uint16_t routinety, ScriptRoutine & grpout, bool isalias)
        {
            using namespace scriptXML;
            eOpParamTypes       ptype   = RoutineParameterType(routinety);
            const std::string * ppname  = OpParamTypesToStr(ptype);
            if(!ppname)
            {
                stringstream sstrer;
                PrintErrorPos(sstrer,routinen) <<"SSBXMLParser::ParseTypedRoutine(): Routine has unknown parameter type \"" 
                                               <<static_cast<uint16_t>(ptype) <<"\"!!";
                throw std::runtime_error(sstrer.str());
            }

            xml_attribute attr = routinen.attribute(ppname->c_str());
            grpout.isalias = isalias;
            grpout.type    = routinety;

            if(!attr && RoutineHasParameter(routinety) )
            {
                //If its an unexpected routine type, we'll have the def argument type
                attr = routinen.attribute(ATTR_RoutineParam2.c_str());
                if(!attr)
                {
                    stringstream sstrer;
                    PrintErrorPos(sstrer,routinen) <<"SSBXMLParser::ParseTypedRoutine(): Routine is missing its expected parameter \"" 
                                                   <<*ppname <<"\"!!";
                    throw std::runtime_error(sstrer.str());
                }
                ptype = eOpParamTypes::UNK_Placeholder; //We want to handle it as a regular value
            }
            else if(!RoutineHasParameter(routinety))
            {
                //We don't have a parameter value, standard functions never usually have a parameter, so default it to 0.
                grpout.unk2 = 0;
                return;
            }
            
            //! #TODO: this is redundant, better find a smart way to deal with this..
            switch(ptype)
            {
                case eOpParamTypes::Unk_LivesRef:
                {
                    uint16_t livesid = m_paraminf.LivesInfo(attr.value());
                    if(livesid != InvalidLivesID )
                        grpout.unk2 = livesid;
                    else
                    {
                        clog <<routinen.path() <<", " <<routinen.offset_debug() 
                                <<" : used invalid \"lives\" id value as a raw integer.\n"; 
                        grpout.unk2 = ToWord(attr.as_int());
                    }
                    break;
                }
                case eOpParamTypes::Unk_PerformerRef:
                {
                    //!#TODO
                    grpout.unk2 = ToWord(attr.as_uint());
                    break;
                }
                case eOpParamTypes::Unk_ObjectRef:
                {
                    try
                    {
                        grpout.unk2 = m_paraminf.StrToObjectID(attr.value()); //! #FIXME: Verify it or somthing?
                    }
                    catch( const std::exception & )
                    {
                        stringstream sstrer;
                        PrintErrorPos(sstrer,routinen) 
                            << "SSBXMLParser::ParseTypedCommandParameterAttribute(): Object id " <<attr.value() 
                            <<", is missing object number! Can't reliably pinpoint the correct object instance!";
                        throw_with_nested( std::runtime_error(sstrer.str()) );
                    }
                    break;
                }
                case eOpParamTypes::UNK_Placeholder:
                {
                    grpout.unk2 = ToWord(attr.as_uint());
                    break;
                }
            };
        }

        /*****************************************************************************************
        *****************************************************************************************/
        void ParseCode( const xml_node & coden )
        {
            using namespace scriptXML;
            Script::grptbl_t & outtbl = m_out.Routines();

            uint16_t lastroutinetype = 0;
            for( const xml_node & group : coden.children() )
            {
                uint16_t routinety = 0;
                bool     isalias   = NODE_RoutineAlias == group.name();

                if(isalias)
                {
                    if(m_out.Routines().empty()) 
                    {
                        stringstream sstrer;
                        PrintErrorPos(sstrer,group) 
                            << "SSBXMLParser::ParseCode(): The first routine cannot be an alias!";
                        throw std::runtime_error(sstrer.str());
                    }
                    //If its an alias, the type is held in the type attribute, not the name of the node
                    //xml_attribute xtype = group.attribute(ATTR_RoutineType.c_str());
                    //if(!xtype)
                    //{
                    //    stringstream sstrer;
                    //    PrintErrorPos(sstrer,group) 
                    //        << "SSBXMLParser::ParseCode(): Routine alias is missing its type!";
                    //    throw std::runtime_error(sstrer.str());
                    //}
                    routinety = lastroutinetype;
                }
                else
                    routinety = StrToRoutineTyInt(group.name());
                

                //Parse the attributes and instructions if the routine is a valid type!
                if(routinety != 0)
                {
                    ScriptRoutine grpout;
                    ParseTypedRoutine(group, routinety, grpout, isalias);
                    if(!isalias)
                    {
                        for( const xml_node & inst : group )
                            ParseInstruction(inst,grpout.instructions);
                    }
                    outtbl.push_back(std::move(grpout));
                }
                lastroutinetype = routinety; //Set the type of the last routine
            }
        }

        /*****************************************************************************************
        *****************************************************************************************/
        template<typename _InstContainer>
            void ParseInstruction( const xml_node & instn, _InstContainer & outcnt )
        {
            using namespace scriptXML;
            //static const unordered_map<std::string, eInstructionType> Ref 
            //{{
            //    {NODE_Instruction, eInstructionType::Command},
            //    {NODE_MetaLabel,   eInstructionType::MetaLabel},
            //}};

            //auto itf = Ref.find(instn.name());
            //if( itf != Ref.end() )
            //{
            //    if(itf->second == eInstructionType::MetaLabel)
            //        ParseMetaLabel(instn, outcnt);
            //    else
            //        ParseCommand(instn, outcnt);
            //}
            //else
            //    TryParseCommandNode(instn, outcnt);
            
            if( instn.name() == NODE_Instruction )
                ParseCommand(instn, outcnt);
            else if( instn.name() == NODE_MetaLabel )
                ParseMetaLabel(instn, outcnt);
            else
                TryParseCommandNode(instn, outcnt);
        }

        /*****************************************************************************************
        *****************************************************************************************/
        template<typename _InstContainer>
            void ParseMetaLabel( const xml_node & instn, _InstContainer & outcnt )
        {
            using namespace scriptXML;
            xml_attribute xid = instn.attribute(ATTR_LblID.c_str());

            if( !xid )
                throw std::runtime_error("SSBXMLParser::ParseInstruction(): Label with invalid ID found! " + instn.path());

            uint16_t labelid = ToWord(xid.as_int());
            m_labelchecker[labelid].bexists = true; //We know this label exists

            ScriptInstruction outinstr;
            outinstr.type  = eInstructionType::MetaLabel;
            outinstr.value = labelid;
            outcnt.push_back(std::move(outinstr));
        }


        /*****************************************************************************************
            Cases :

            		<Switch svar="DUNGEON_ENTER">
						<Case int="24" tolabel_1="121" />
						<Case int="25" tolabel_1="122" />
						<Case int="26" tolabel_1="123" />
					</Switch>
        *****************************************************************************************/
        template<typename _InstContainer>
            void TryParseCommandNode(const xml_node & instn, _InstContainer & outcnt)
        {
            using namespace scriptXML;
            xml_attribute_iterator  itat     = instn.attributes_begin();
            xml_attribute_iterator  itatend  = instn.attributes_end();
            const std::string       nodename = instn.name();
            size_t                  nbparams = std::distance( itat, itatend );

            if( instn.child(NODE_String.c_str()) )
                ++nbparams; //If we got a string, add it to the parameter count!

            uint16_t foundop  = m_opinfo.Code( nodename, nbparams );
            if( foundop == InvalidOpCode )
            {
                stringstream sstrer;
                PrintErrorPos(sstrer,instn) <<"SSBXMLParser::TryParseCommandNode(): Script \"" <<m_out.Name() <<"\", instruction group #" << m_out.Routines().size() 
                    <<", in group instruction #" <<outcnt.size() <<" Node name doesn't match any known meta instructions or command!!";
                throw std::runtime_error(sstrer.str());
            }

            OpCodeInfoWrapper opinfo = m_opinfo.Info(foundop);
            ScriptInstruction outinstr;
            outinstr.value = foundop;
            outinstr.type  = opinfo.GetMyInstructionType();

            //Read parameters
            DecideHowParseParams( instn, itat, itatend, nbparams, opinfo, instn.child(NODE_String.c_str()), outinstr, outcnt );

            //Parse child instructions, if required
            if( outinstr.type  != eInstructionType::Command )
            {
                ParseSubInstructions(instn, outinstr);
            }
            //Insert
            outcnt.push_back(std::move(outinstr));
        }
        
        /*****************************************************************************************
        *****************************************************************************************/
        void ParseSubInstructions(const xml_node & parentinstn, ScriptInstruction & dest )
        {
            using namespace scriptXML;
            for( const auto & entry : parentinstn.children() )
                ParseInstruction(entry, dest.subinst);
        }

        /*****************************************************************************************
        *****************************************************************************************/
        template<typename _InstContainer>
            void ParseCommand(const xml_node & instn, _InstContainer & outcnt)
        {
            using namespace scriptXML;
            xml_attribute name = instn.attribute(ATTR_Name.c_str());
            string        instname;
            if(!name)
            {
                //!#TODO: Merge the other method for parsing parameters with sub-instructions with this one!!
                stringstream sstrer;
               PrintErrorPos(sstrer,instn) <<"SSBXMLParser::ParseInstruction(): Script \"" <<m_out.Name() <<"\", instruction group #" << m_out.Routines().size() 
                    <<", in group instruction #" <<outcnt.size() <<" doesn't have a \"" <<ATTR_Name <<"\" attribute!!";
                throw std::runtime_error(sstrer.str());
            }
            else
                instname = name.value();

            
            ScriptInstruction outinstr;
            outinstr.type = eInstructionType::Command;

            //#1 - Try to count parameters
            bool                    hasstrings  = false;
            xml_attribute_iterator  itat        = instn.attributes_begin();
            xml_attribute_iterator  itatend     = instn.attributes_end();
            size_t                  nbparams    = 0;

            if( itat != itatend  && name /*&& strcmp(itat->name(), ATTR_Name.c_str()) == 0*/ )
                ++itat; //Skip name attribute

            nbparams = std::distance( itat, itatend );

            if( instn.child(NODE_String.c_str()) ) //Look for string nodes, so we can count it as a parameter
            {
                hasstrings = true;
                ++nbparams;
            }

            //#2 - Read opcode
            outinstr.value = m_opinfo.Code(instname, nbparams );
            if( outinstr.value == InvalidOpCode )
            {
                stringstream sstrer;

                PrintErrorPos(sstrer, instn) << "SSBXMLParser::ParseCommand(): No matching command for " 
                    <<instname <<", taking " <<nbparams <<" parameter(s) found!";;
                throw std::runtime_error(sstrer.str());
            }

            //#3 - Parse parameters
            OpCodeInfoWrapper oinf = m_opinfo.Info(outinstr.value);
            if(nbparams != 0)
                DecideHowParseParams( instn, itat, itatend, nbparams, oinf, hasstrings,outinstr, outcnt );

            //!#TODO: Merge the other method for parsing parameters with sub-instructions with this one!!

            outcnt.push_back(std::move(outinstr));            
        }


        /*****************************************************************************************
            DecideHowParseParams
                Depending on the data will pick the best method to parse the parameters 
                for a given instruction.
        *****************************************************************************************/
        template<typename _InstContainer>
            void DecideHowParseParams( const xml_node         & instn,
                                       xml_attribute_iterator & itat, 
                                       xml_attribute_iterator & itatend, 
                                       size_t                   nbparams, 
                                       const OpCodeInfoWrapper & oinf,
                                       bool                     hasstrings,
                                       ScriptInstruction      & outinstr,
                                       _InstContainer         & outcnt )
        {
            using namespace scriptXML;
            //!#TODO: Handle meta, and differently named parameters!!
            if(nbparams == 0)
            {
                return;
            }
            else if( nbparams >= oinf.NbParams() )
            {
                //We should have enough parameters to parse!
                if( oinf.ParamInfo().size() == oinf.NbParams() )
                {
                    if( ParseDefinedParameters( instn, itat, itatend, oinf, hasstrings, outinstr ) != oinf.NbParams() )
                    {
                        assert(false); //should never happen!!
                    }
                }
                else if( oinf.ParamInfo().size() < oinf.NbParams() &&   //If not all parameters were defined
                        nbparams >= oinf.NbParams() )                   //AND if the nb of param attributes is larger or equal to the ammount expected.
                {
                    //Handle when **not** all parameters are defined
                    //We do not handle string nodes in this case!
                    if( ParsePartiallyDefinedParameters(instn, itat, itatend, oinf, hasstrings, outinstr) != oinf.NbParams() )
                    {
#ifdef _DEBUG
                        assert(false);
#endif
                        //Error, lacks required parameters!!
                        stringstream sstrer;
                        PrintErrorPos(sstrer,instn) <<"SSBXMLParser::ParseCommand(): Command \"" <<instn.path() <<"\" had less parameters specified than expected!!";
                        throw std::runtime_error(sstrer.str()); //Error!!
                    }
                }
            }
            else if( oinf.NbParams() == -1 && itat != itatend )
            {
                //Special case
                ParseRawParameters(itat, itatend, oinf, outinstr);
            }
            else if( nbparams < oinf.NbParams() || itat == itatend )
            {
#ifdef _DEBUG
                        assert(false);
#endif
                //Error, lacks required parameters!!
                stringstream sstrer;
                PrintErrorPos(sstrer,instn) <<"SSBXMLParser::ParseCommand(): Command \"" <<instn.path() <<"\" had less parameters specified than expected!!";
                throw std::runtime_error(sstrer.str()); //Error!!
            }

        }


        /*****************************************************************************************
            ParseRawParameters
                Parses any parameters named "param", and push them into the
                parameter list of the function!
        *****************************************************************************************/
        size_t ParseRawParameters(  xml_attribute_iterator & itat, 
                                    xml_attribute_iterator & itatend,
                                    const OpCodeInfoWrapper& oinf,
                                    ScriptInstruction      & outinstr,
                                    size_t                   begat = 0 )
        {
            using namespace scriptXML;

            size_t nbtoparse = 0;
            if(oinf.NbParams() != -1)
                nbtoparse = oinf.NbParams();
            else 
                nbtoparse = std::distance( itat, itatend );

            for( ;begat < nbtoparse && itat != itatend; ++itat )
            {
                const string attrname = CleanAttributeName(itat->name());
                if( attrname == ATTR_Param )
                {
                    outinstr.parameters.push_back( ToWord(itat->as_uint()) );
                    ++begat;
                }
            }
            return begat;
        }

        /*****************************************************************************************
            ParsePartiallyDefinedParameters
                Handles the case when not all parameters have a defined type.

                **#TODO: Will eventually be phased out!!**
        *****************************************************************************************/
        size_t ParsePartiallyDefinedParameters( const xml_node         & instn, 
                                                xml_attribute_iterator & itat, 
                                                xml_attribute_iterator & itatend,
                                                const OpCodeInfoWrapper& oinf,
                                                bool                     hasstrings,     //Whether has string subnodes or not
                                                ScriptInstruction      & outinstr )
        {
            using namespace scriptXML;
            size_t nbparamparsed = ParseDefinedParameters(instn, itat, itatend, oinf, hasstrings, outinstr);

            //handle any remaining parameters
            return ParseRawParameters( itat, itatend, oinf, outinstr, nbparamparsed );
        }

        /*****************************************************************************************
            ParseDefinedParameters
                Will parse all defined parameters until hitting the end of the attribute list, 
                or the defined param list!
                Returns the nb of parameters parsed.
        *****************************************************************************************/
        size_t ParseDefinedParameters( const xml_node         & instn, 
                                       xml_attribute_iterator & itat, 
                                       xml_attribute_iterator & itatend,
                                       const OpCodeInfoWrapper& oinf,
                                       bool                     hasstrings,     //Whether has string subnodes or not
                                       ScriptInstruction      & outinstr ) 
        {
            using namespace scriptXML;
            size_t nbparamread = 0;
            //Handle when **all** parameters are defined
            for( const auto & pinf : oinf.ParamInfo() )
            {
                if( itat != itatend )   
                {
                    //**When we still have parameters attributes to parse**
                    const string  attrname      = CleanAttributeName(itat->name());
                    eOpParamTypes attrparamtype = FindOpParamTypesByName(attrname);
                    if( pinf.ptype == attrparamtype )
                    {
                        ParseTypedCommandParameterAttribute(*itat, instn, attrparamtype, outinstr );
                    }
                    else if( pinf.ptype == eOpParamTypes::String ) //Strings will never match 
                    {
                        if( attrparamtype == eOpParamTypes::Constant && itat != itatend )
                        {
                            //Special case for constants and strings, since they can be replaced by one another
                            HandleAConstref( *itat, outinstr );
                        }
                        else if(hasstrings)
                        {
                            //If we have strings nodes parse them
                            HandleStringNodes( instn, outinstr );
                            ++nbparamread;  //Increment and skip incrementing the iterator!!
                            continue;       //Don't increment itat, since we're not even using the current parameter attribute !!
                        }
                        else
                        {
                            stringstream ss;
                            PrintErrorPos(ss,instn) <<"SSBXMLParser::ParseDefinedParameters(): Expected String node or constref, but neither were found! " 
                                                    << instn.path();
                            throw std::runtime_error(ss.str());
                        }
                    }
                    else
                    {
                        stringstream ss;
                        PrintErrorPos(ss,instn) <<"SSBXMLParser::ParseDefinedParameters(): Unexpected parameter " <<itat->name() <<"\", " <<attrname <<"\"! "
                                                << instn.path();
                        throw std::runtime_error( ss.str() );
                    }
                }
                else if( pinf.ptype == eOpParamTypes::String ) 
                {
                    //**If we don't have attributes left to parse, but are looking for strings**
                    //This is the only valid case where we'd have no more attributes to parse!
                    if(hasstrings)
                    {
                        //If we have strings nodes parse them
                        HandleStringNodes( instn, outinstr );
                        ++nbparamread;  //Increment and skip incrementing the iterator!!
                        
                    }
                    else
                    {
                        stringstream ss;
                        PrintErrorPos(ss,instn) <<"SSBXMLParser::ParseDefinedParameters(): Expected String node, but none were found! "
                                                << instn.path();
                        throw std::runtime_error( ss.str() );
                    }
                    break; //Break immediatetly, since, we don't have any attributes left!!
                }
                else //if( itat == itatend )
                {
                    break; //After we looked for possible strings nodes, we can break safely!
                }
                ++nbparamread;
                ++itat;
            }

            return nbparamread;
        }

        /*****************************************************************************************
            HandleStringNodes
        *****************************************************************************************/
        void HandleStringNodes(const xml_node & instn, ScriptInstruction & outinstr)
        {
            using namespace scriptXML;
            auto subn = instn.children(NODE_String.c_str());

            if( subn.begin() != subn.end() )
            {
                uint16_t strindex = ToWord(m_stringrefparam.size());
                //If we got subnodes containing strings!
                for( const auto & strs: subn)
                {
                    xml_attribute xlang = strs.attribute(ATTR_Language.c_str());
#if defined(PMD2XML_STRING_AS_CDATA) || defined(PMD2XML_STRING_AS_PCDATA)
                    xml_text cdatatext  = strs.text();

                    if( /*cdatatext && */xlang )
                    {
                        eGameLanguages lang = StrToGameLang(xlang.value());

                        if( lang == eGameLanguages::Invalid )
                        {
                            throw std::runtime_error("SSBXMLParser::ParseTypedCommandParameterAttribute(): Encountered unknown language "s + xlang.value() + "for string!");
                        }
                        m_strqueues[lang].push_back(cdatatext.get());
                    }
#else
                    xml_attribute xval  = strs.attribute(ATTR_Value.c_str());

                    if( xval && xlang )
                    {
                        eGameLanguages lang = StrToGameLang(xlang.value());

                        if( lang == eGameLanguages::Invalid )
                        {
                            throw std::runtime_error("SSBXMLParser::ParseTypedCommandParameterAttribute(): Encountered unknown language "s + xlang.value() + "for string!");
                        }
                        m_strqueues[lang].push_back(xval.value());
                    }
#endif
                }

                //Add to the list of parameter only once per string, not per language!!!
                outinstr.parameters.push_back( strindex );//save the idex it was inserted at
                m_stringrefparam.push_back( std::addressof(outinstr.parameters.back()) ); //Then add this parameter value to the list of strings id to increment by the constant tbl len later on!
            }
        }


        /*****************************************************************************************
            CleanAttributeName
        *****************************************************************************************/
        inline string CleanAttributeName( const pugi::char_t * cname )
        {
            string name(cname);
            for( size_t i = 0; i < name.size(); ++i )
            {
                if( name[i] == '_' && (i + 1 < name.size()) && std::isdigit(name[i+1], locale::classic()) )
                    return name.substr(0,i);
            }
            return std::move(name);
        }
        //{
        //    string name(cname);
        //    static const regex ParamNameCleaner(R"((\b\S+(?=_\d)))");
        //    smatch             matches;
        //    //First, clean the name of the parameter of any appended number if needed
        //    if( regex_search( name, matches, ParamNameCleaner ) && matches.size() > 1 )
        //        name = matches[1].str();
        //    return std::move(name);
        //}


        /*****************************************************************************************
            HandleAConstref
        *****************************************************************************************/
        inline void HandleAConstref(const xml_attribute & param, ScriptInstruction & outinst )
        {
            outinst.parameters.push_back( ToWord(m_constqueue.size()) );
            m_constqueue.push_back( param.value() );
        }

        /*****************************************************************************************
            ParseTypedCommandParameterAttribute
        *****************************************************************************************/
        void ParseTypedCommandParameterAttribute( const xml_attribute & param, const xml_node & parentinstn, eOpParamTypes pty, ScriptInstruction & outinst )
        {
            using namespace scriptXML;
            switch(pty)
            {
                case eOpParamTypes::Constant:
                {
                    HandleAConstref( param, outinst );
                    break;
                }
                case eOpParamTypes::String:
                {
                    if( m_region == eGameRegion::Japan )
                    {
                        HandleAConstref( param, outinst );
                    }
                    break;
                }
                case eOpParamTypes::InstructionOffset:
                {
                    //Labels IDs
                    uint16_t lblid = ToWord( param.as_uint() );
                    auto itf = m_labelchecker.find( lblid );
                    
                    if( itf != m_labelchecker.end() )
                        itf->second.referersoffsets.push_back(parentinstn.offset_debug());
                        //itf->second.nbref += 1;
                    else
                        m_labelchecker.emplace( std::make_pair( lblid, labelRefInf{ false, {{parentinstn.offset_debug()}} } ) ); //We found a ref to this label, but didn't check it

                    outinst.parameters.push_back( lblid );
                    break;
                }
                case eOpParamTypes::Unk_CRoutineId:
                {
                    uint16_t croutineid = m_paraminf.CRoutine(param.value());

                    //! #TODO: When the parameter info gets encapsulated and abstracted in its own class, remove this!
                    //if( m_version == eGameVersion::EoS )
                    //    croutineid = FindCommonRoutineIDByName_EoS(param.value());
                    //else
                    //    croutineid = FindCommonRoutineIDByName_EoTD(param.value());

                    if(croutineid != InvalidCRoutineID )
                        outinst.parameters.push_back(croutineid);
                    else
                    {
                        clog <<parentinstn.path() <<", " <<parentinstn.offset_debug() 
                             <<" : used invalid \"common routine\" id value as a raw integer.\n"; 
                        outinst.parameters.push_back( ToWord(param.as_int()) );
                    }

                    break;
                }
                case eOpParamTypes::Unk_FaceType:
                {
                    uint16_t faceid = m_paraminf.Face(param.value());
                    outinst.parameters.push_back(faceid);
                    break;
                }
                case eOpParamTypes::Unk_ScriptVariable:
                {
                    uint16_t varid = m_paraminf.GameVarInfo(param.value());
                    if(varid != InvalidGameVariableID )
                        outinst.parameters.push_back(varid);
                    else
                    {
                        clog <<parentinstn.path() <<", " <<parentinstn.offset_debug() 
                             <<" : used invalid game variable id value as a raw integer.\n";
                        outinst.parameters.push_back( ToWord(param.as_int()) );
                    }
                    break;
                }
                case eOpParamTypes::Unk_LivesRef:
                {
                    outinst.parameters.push_back(m_paraminf.LivesInfo(param.value()));
                    break;
                }
                case eOpParamTypes::Unk_PerformerRef:
                {
                    //!#TODO
                    outinst.parameters.push_back( ToWord( param.as_uint() ) );
                    break;
                }
                case eOpParamTypes::Unk_ObjectRef:
                {
                    try
                    {
                        outinst.parameters.push_back(m_paraminf.StrToObjectID(param.value())); //! #FIXME: Verify it or somthing?
                    }
                    catch( const std::exception & )
                    {
                        stringstream sstrer;
                        PrintErrorPos(sstrer,parentinstn) 
                            << "SSBXMLParser::ParseTypedCommandParameterAttribute(): Object id " <<param.value() 
                            <<", is missing object number! Can't reliably pinpoint the correct object instance!";
                        throw_with_nested( std::runtime_error(sstrer.str()) );
                    }
                    break;
                }
                case eOpParamTypes::Unk_LevelId:
                {
                    uint16_t lvlid = m_paraminf.LevelInfo(param.value());
                    if( lvlid == InvalidLevelID && param.value() != NullLevelId)
                    {
                        clog <<parentinstn.path() <<", " <<parentinstn.offset_debug() 
                             <<" : invalid level id value, using it as a raw integer.\n"; 
                        outinst.parameters.push_back(ToSWord(param.as_int()));
                    }

                    outinst.parameters.push_back(lvlid);
                    break;
                }
                case eOpParamTypes::Unk_FacePosMode:
                {
                    uint16_t faceposmode = m_paraminf.FacePosMode(param.value());
                    if( faceposmode != InvalidFaceModeID )
                        outinst.parameters.push_back(faceposmode);
                    else
                    {
                        clog <<parentinstn.path() <<", " <<parentinstn.offset_debug() 
                             <<" : used invalid face position mode value as a raw integer.\n"; 
                        outinst.parameters.push_back(ToSWord(param.as_int()));
                    }
                    break;
                }
                case eOpParamTypes::Direction:
                {
                    outinst.parameters.push_back(m_paraminf.Direction(param.value()));
                    break;
                }
                case eOpParamTypes::ItemID:
                {
                    outinst.parameters.push_back(m_paraminf.Item(param.value()));
                    return;
                }
                case eOpParamTypes::Integer:
                case eOpParamTypes::Duration:
                case eOpParamTypes::CoordinateY:
                case eOpParamTypes::CoordinateX:
                {
                    outinst.parameters.push_back( Convert16bTo14b(ToSWord(param.as_int())) );
                    break;
                }
                case eOpParamTypes::StationId:
                case eOpParamTypes::ActingLayerID:
                {
                    outinst.parameters.push_back( Convert16bTo14b(ToSWord(param.as_int())) );
                    break;
                }
                default:
                {
                    outinst.parameters.push_back( ToWord( param.as_uint() ) );
                }
            }
        }

        /*****************************************************************************************
        *****************************************************************************************/
        void ParseConsts( const xml_node & constn )
        {
            using namespace scriptXML;
            m_out.ConstTbl() = std::move( Script::consttbl_t( m_constqueue.begin(), m_constqueue.end() ) );

            //deque<string> constantsout; //The deque avoids re-allocating a vector constantly
            //for( const auto & conststr : constn.children(NODE_Constant.c_str()) )
            //{
            //    xml_attribute xval = conststr.attribute(ATTR_Value.c_str());
            //    if(!xval)
            //    {
            //        stringstream sstrer;
            //        sstrer << "SSBXMLParser::ParseConsts(): Script \"" <<m_out.Name() <<"\", Constant #"  <<constantsout.size()
            //               <<" is missing its \"" <<ATTR_Value <<"\" attribute!";
            //        throw std::runtime_error( sstrer.str() );
            //    }
            //    constantsout.push_back(xval.value());
            //}
            //m_out.ConstTbl() = std::move( Script::consttbl_t( constantsout.begin(), constantsout.end() ) );
        }

        /*****************************************************************************************
        *****************************************************************************************/
        void ParseStringBlock( const xml_node & strblkn )
        {
            using namespace scriptXML;
            xml_attribute  xlang = strblkn.attribute(ATTR_Language.c_str());
            eGameLanguages glang = eGameLanguages::Invalid;

            if( !xlang )
            {
                    stringstream sstrer;
                    sstrer << "SSBXMLParser::ParseConsts(): Script \"" <<m_out.Name() <<"\", String block #"  <<m_out.StrTblSet().size()
                           <<" is missing its \"" <<ATTR_Language <<"\" attribute!";
                    throw std::runtime_error( sstrer.str() );
            }
            if( (glang = StrToGameLang(xlang.value())) == eGameLanguages::Invalid )
            {
                    stringstream sstrer;
                    sstrer << "SSBXMLParser::ParseConsts(): Script \"" <<m_out.Name() <<"\", String block #"  <<m_out.StrTblSet().size()
                           <<" has an invalid value\"" <<xlang.value() <<"\" as its \"" <<ATTR_Language <<"\" attribute value!";
                    throw std::runtime_error( sstrer.str() );
            }

            deque<string> langstr; //save on realloc each times on a vector
            for( const auto & str : strblkn.children(NODE_String.c_str()) )
            {
                xml_attribute xval = str.attribute( ATTR_Value.c_str() );
                if(xval)
                    langstr.push_back(xval.value());
                else
                {
                    stringstream sstrer;
                    sstrer << "SSBXMLParser::ParseConsts(): Script \"" <<m_out.Name() <<"\", String block #"  <<m_out.StrTblSet().size()
                           <<", language \"" <<xlang.value() <<"\", string #" <<langstr.size() 
                           <<", is missing its \"" <<ATTR_Value <<"\" attribute!";
                    throw std::runtime_error( sstrer.str() );
                }
            }
            m_out.InsertStrLanguage( glang, std::move(Script::strtbl_t( langstr.begin(), langstr.end() )) );
        }

    private:
        struct labelRefInf
        {
            bool                    bexists;            //Whether the label's metainstruction has been found
            std::deque<ptrdiff_t>   referersoffsets;    //The offset in the xml of the references to this label
        };


        unordered_map<uint16_t, labelRefInf>         m_labelchecker;
        deque<string>                                m_constqueue;
        unordered_map<eGameLanguages, deque<string>> m_strqueues;

        //A list of parameters containing a string index reference to be incremented after the size of the const table is known!
        deque<uint16_t *>                            m_stringrefparam;  

        Script           m_out;
        eGameVersion     m_version;
        eGameRegion      m_region;
        OpCodeClassifier m_opinfo;
        //LevelEntryInfoWrapper m_lvlentryinf;
        ParameterReferences  m_paraminf;
        const ConfigLoader & m_gconf;
    };


    /*****************************************************************************************
        SSDataXMLParser
            
    *****************************************************************************************/
    class SSDataXMLParser
    {
        inline stringstream & PrintErrorPos( stringstream & sstr, const xml_node & errornode )const
        {
            sstr <<m_out.Name() <<", file offset : " <<dec <<nouppercase <<errornode.offset_debug() <<" -> ";
            return sstr;
        }

    public:
        SSDataXMLParser( const ConfigLoader & conf )
            :m_gconf(conf), m_paraminf(conf)
        {}

        ScriptData operator()(xml_node & datan)
        {
            using namespace scriptXML;
            xml_attribute xname = datan.attribute(ATTR_ScrDatName.c_str());
            if(!xname)
            {
                stringstream sstrer;
                PrintErrorPos(sstrer,datan) << "SSDataXMLParser::operator(): Script data is missing its \"" 
                                            << ATTR_ScrDatName << "\" attribute!!";
                throw std::runtime_error(sstrer.str());
            }
            xml_attribute xtype = datan.attribute(ATTR_ScriptType.c_str());
            if(!xtype)
            {
                stringstream sstrer;
                PrintErrorPos(sstrer,datan) << "SSDataXMLParser::operator(): Script data is missing its \"" 
                                            << ATTR_ScriptType <<"\" attribute!!";
                throw std::runtime_error(sstrer.str());
            }

            eScrDataTy dataty = StrToScriptDataType(xtype.value());
            if( dataty == eScrDataTy::Invalid )
            {
                stringstream sstrer;
                PrintErrorPos(sstrer,datan) << "SSDataXMLParser::operator(): Invalid script data type!!";
                throw std::runtime_error(sstrer.str());
            }

            //Init output data
            m_out = std::move( ScriptData(xname.value(), dataty) );

            ParseUnkTable1(datan);
            ParsePosMarkers(datan);
            ParseLayers(datan);

            return std::move(m_out);
        }

    private:
        void ParseUnkTable1(xml_node & datan)
        {
            using namespace scriptXML;
            const string & AttrID  = *OpParamTypesToStr(eOpParamTypes::Unk_CRoutineId);
            xml_node       unktbl1 = datan.child(NODE_UnkTable1.c_str());

            for(const auto & curunk1entry : unktbl1.children(NODE_UnkTable1Entry.c_str()) )
            {
                UnkTbl1DataEntry entry;
                xml_attribute  xcrtnid = curunk1entry.attribute(AttrID.c_str());
                if(!xcrtnid)
                {
                    stringstream sstrer;
                    PrintErrorPos(sstrer,curunk1entry) << "SSDataXMLParser::ParseUnkTable1(): Missing " <<AttrID <<" attribute!!";
                    throw std::runtime_error(sstrer.str());
                }

                entry.croutineid = m_paraminf.CRoutine(xcrtnid.value());
                if(entry.croutineid == InvalidCRoutineID)
                {
                    cerr << "SSDataXMLParser::ParseUnkTable1(), offset: " <<curunk1entry.offset_debug() <<": Got common routine id " 
                         <<entry.croutineid <<"! Interpreting as number instead!\n";
                    entry.croutineid = ToSWord(xcrtnid.as_int());
                }

                //UnkTbl1DataEntry entry;
               // xml_attribute xcrtnid = curunk1entry.attribute(ATTR_.c_str());
                xml_attribute xunk1   = curunk1entry.attribute(ATTR_Unk1.c_str());
                xml_attribute xunk2   = curunk1entry.attribute(ATTR_Unk2.c_str());
                xml_attribute xscrid  = curunk1entry.attribute(ATTR_ScriptID.c_str());

                if(xcrtnid && xunk1 && xunk2 && xscrid)
                {
                    //entry.croutineid    = ToWord(xcrtnid.as_uint());
                    entry.unk1          = ToWord(xunk1.as_uint());
                    entry.unk2          = ToWord(xunk2.as_uint());
                    entry.scrid         = ToWord(xscrid.as_uint());
                    m_out.UnkTbl1().push_back(std::move(entry));
                }
                else
                {
                    stringstream sstrer;
                    PrintErrorPos(sstrer,curunk1entry) << "SSDataXMLParser::ParseUnkTable1(): Missing one or more of the 4 attributes for Unk Table 1 entry!";
                    throw std::runtime_error(sstrer.str());
                }
            }
        }

        void ParsePosMarkers(xml_node & datan)
        {
            using namespace scriptXML;
            xml_node posmarkers = datan.child(NODE_PositionMarkers.c_str());

            for(const auto & marker : posmarkers.children(NODE_Marker.c_str()) )
            {
                PosMarkDataEntry entry;

                for( const auto & attr : marker.attributes() )
                {
                    if( attr.name() == ATTR_XOffset )
                        entry.xoff = ToSWord(attr.as_uint());
                    else if( attr.name() == ATTR_YOffset )
                        entry.yoff = ToSWord(attr.as_uint());
                    else if( attr.name() == ATTR_Unk2 )
                        entry.unk2 = ToWord(attr.as_uint());
                    else if( attr.name() == ATTR_Unk3 )
                        entry.unk3 = ToWord(attr.as_uint());
                    else if( attr.name() == ATTR_Unk4 )
                        entry.unk4 = ToWord(attr.as_uint());
                    else if( attr.name() == ATTR_Unk5 )
                        entry.unk5 = ToWord(attr.as_uint());
                    else if( attr.name() == ATTR_Unk6 )
                        entry.unk6 = ToWord(attr.as_uint());
                    else if( attr.name() == ATTR_Unk7 )
                        entry.unk7 = ToWord(attr.as_uint());
                }

                m_out.PosMarkers().push_back(std::move(entry));
            }
        }

        void ParseLayers(xml_node & datan)
        {
            using namespace scriptXML;
            xml_node    layers = datan.child(NODE_Layers.c_str());

            for( const auto & layer : layers.children(NODE_Layer.c_str()) )
            {
                ScriptLayer curlay;
                ParseActors     (layer.child(NODE_Actors.c_str()),      curlay);
                ParseObjects    (layer.child(NODE_Objects.c_str()),     curlay);
                ParsePerformers (layer.child(NODE_Performers.c_str()),  curlay);
                ParseEvents     (layer.child(NODE_Events.c_str()),      curlay);
                m_out.Layers().push_back(curlay); //We add even empty layers!!
            }
        }

        void ParseActors(xml_node & actorn, ScriptLayer & outlay)
        {
            using namespace scriptXML;
            const string & AttrID = *OpParamTypesToStr(eOpParamTypes::Unk_LivesRef);
            for( const auto & actor : actorn )
            {
                LivesDataEntry entry;
                xml_attribute xid = actor.attribute(AttrID.c_str());
                if(!xid)
                {
                    stringstream sstrer;
                    PrintErrorPos(sstrer,actor) << "SSDataXMLParser::ParseActors(): Missing actor id attribute!!";
                    throw std::runtime_error(sstrer.str());
                }

                entry.livesid = m_paraminf.LivesInfo(xid.value());
                if(entry.livesid == InvalidLivesID)
                {
                    cerr << "SSDataXMLParser::ParseActors(), offset: " <<actor.offset_debug() <<": Got Invalid actor id " 
                         <<entry.livesid <<"! Interpreting as number instead!\n";
                    entry.livesid = ToSWord(xid.as_int());
                }

                for( const auto & attr : actor.attributes() )
                {
                    if( attr.name() == ATTR_Direction )
                        entry.facing = m_paraminf.DirectionData(attr.value());
                    else if( attr.name() == ATTR_XOffset )
                        entry.xoff = ToSWord(attr.as_uint());
                    else if( attr.name() == ATTR_YOffset )
                        entry.yoff = ToSWord(attr.as_uint());
                    else if( attr.name() == ATTR_Unk4 )
                        entry.unk4 = ToWord(attr.as_uint());
                    else if( attr.name() == ATTR_Unk5 )
                        entry.unk5 = ToWord(attr.as_uint());
                    else if( attr.name() == ATTR_ScriptID )
                        entry.scrid = ToSWord(attr.as_uint());
                }
                outlay.lives.push_back(std::move(entry));
            }
        }

        void ParseObjects(xml_node & objn, ScriptLayer & outlay)
        {
            using namespace scriptXML;
            stringstream   sstr;
            const string & AttrID = *OpParamTypesToStr(eOpParamTypes::Unk_ObjectRef);
            for( const auto & object : objn )
            {
                ObjectDataEntry entry;
                xml_attribute xid = object.attribute(AttrID.c_str());
                if(!xid)
                {
                    stringstream sstrer;
                    PrintErrorPos(sstrer,object) << "SSDataXMLParser::ParseObjects(): Missing " <<AttrID <<" attribute!!";
                    throw std::runtime_error(sstrer.str());
                }

                const string objid = xid.value();
                try
                {
                    entry.objid = m_paraminf.StrToObjectID(objid); //! #FIXME: Verify it or somthing?
                }
                catch( const std::exception & )
                {
                    stringstream sstrer;
                    PrintErrorPos(sstrer,object) 
                        << "SSDataXMLParser::ParseObjects(): Object id " <<objid 
                        <<", is missing object number! Can't reliably pinpoint the correct object instance!";
                    throw_with_nested(std::runtime_error(sstrer.str()));
                }

                for( const auto & attr : object.attributes() )
                {
                    if( attr.name() == ATTR_Direction )
                        entry.facing = m_paraminf.DirectionData(attr.value());
                    else if( attr.name() == ATTR_Unk2 )
                        entry.unk2 = ToWord(attr.as_uint());
                    else if( attr.name() == ATTR_Unk3 )
                        entry.unk3 = ToWord(attr.as_uint());
                    else if( attr.name() == ATTR_XOffset )
                        entry.xoff = ToSWord(attr.as_uint());
                    else if( attr.name() == ATTR_YOffset )
                        entry.yoff = ToSWord(attr.as_uint());
                    else if( attr.name() == ATTR_Unk6 )
                        entry.unk6 = ToWord(attr.as_uint());
                    else if( attr.name() == ATTR_Unk7 )
                        entry.unk7 = ToWord(attr.as_uint());
                    else if( attr.name() == ATTR_ScriptID )
                        entry.scrid = ToSWord(attr.as_uint());
                }
                outlay.objects.push_back(std::move(entry));
            }
        }

        void ParsePerformers(xml_node & perfn, ScriptLayer & outlay)
        {
            using namespace scriptXML;

            for( const auto & performer : perfn )
            {
                PerformerDataEntry entry;
                for( const auto & attr : performer.attributes() )
                {
                    if( attr.name() == ATTR_Unk0 )
                        entry.unk0 = ToWord(attr.as_uint());
                    else if( attr.name() == ATTR_Unk1 )
                        entry.unk1 = ToWord(attr.as_uint());
                    else if( attr.name() == ATTR_Unk2 )
                        entry.unk2 = ToWord(attr.as_uint());
                    else if( attr.name() == ATTR_Unk3 )
                        entry.unk3 = ToWord(attr.as_uint());
                    else if( attr.name() == ATTR_Unk4 )
                        entry.unk4 = ToWord(attr.as_uint());
                    else if( attr.name() == ATTR_Unk5 )
                        entry.unk5 = ToWord(attr.as_uint());
                    else if( attr.name() == ATTR_Unk6 )
                        entry.unk6 = ToWord(attr.as_uint());
                    else if( attr.name() == ATTR_Unk7 )
                        entry.unk7 = ToWord(attr.as_uint());
                    else if( attr.name() == ATTR_Unk8 )
                        entry.unk8 = ToWord(attr.as_uint());
                    else if( attr.name() == ATTR_Unk9 )
                        entry.unk9 = ToWord(attr.as_uint());
                }
                outlay.performers.push_back(std::move(entry));
            }
        }

        void ParseEvents(xml_node & evn, ScriptLayer & outlay)
        {
            using namespace scriptXML;

            const string & AttrID = *OpParamTypesToStr(eOpParamTypes::Unk_CRoutineId);

            for( const auto & aevent : evn )
            {
                EventDataEntry entry;
                xml_attribute  xevid = aevent.attribute(AttrID.c_str());
                if(!xevid)
                {
                    stringstream sstrer;
                    PrintErrorPos(sstrer,aevent) << "SSDataXMLParser::ParseEvents(): Missing " <<AttrID <<" attribute!!";
                    throw std::runtime_error(sstrer.str());
                }

                entry.unk0 = m_paraminf.CRoutine(xevid.value());
                if(entry.unk0 == InvalidCRoutineID)
                {
                    cerr << "SSDataXMLParser::ParseEvents(), offset: " <<aevent.offset_debug() <<": Got common routine id " 
                         <<entry.unk0 <<"! Interpreting as number instead!\n";
                    entry.unk0 = ToSWord(xevid.as_int());
                }

                for( const auto & attr : aevent.attributes() )
                {

                    if( attr.name() == ATTR_Unk1 )
                        entry.unk1 = ToWord(attr.as_uint());
                    else if( attr.name() == ATTR_Unk2 )
                        entry.unk2 = ToWord(attr.as_uint());
                    else if( attr.name() == ATTR_Unk3 )
                        entry.unk3 = ToWord(attr.as_uint());
                    else if( attr.name() == ATTR_Unk4 )
                        entry.unk4 = ToWord(attr.as_uint());
                    else if( attr.name() == ATTR_Unk5 )
                        entry.unk5 = ToWord(attr.as_uint());
                    else if( attr.name() == ATTR_Unk6 )
                        entry.unk6 = ToWord(attr.as_uint());
                }
                outlay.events.push_back(std::move(entry));
            }
        }

    private:
        const ConfigLoader & m_gconf;
        ScriptData           m_out;
        ParameterReferences  m_paraminf;
    };

    /*****************************************************************************************
        GameScriptsXMLParser
            
    *****************************************************************************************/
    class GameScriptsXMLParser
    {
    public:
        GameScriptsXMLParser(eGameRegion & out_reg, eGameVersion & out_gver, const ConfigLoader & conf)
            :m_out_reg(out_reg), m_out_gver(out_gver), m_gconf(conf)
        {}

        LevelScript Parse( const std::string & file )
        {
            using namespace scriptXML;
            xml_document doc;

            try
            {
                HandleParsingError( doc.load_file(file.c_str(), pugi::parse_default | pugi::parse_ws_pcdata_single), file);
            }
            catch(const std::exception & )
            {
                throw_with_nested(std::runtime_error("GameScriptsXMLParser::Parse() : Pugixml failed loading file!"));
            }

            xml_node      parentn    = doc.child(ROOT_ScripDir.c_str());
            xml_attribute xversion   = parentn.attribute(ATTR_GVersion.c_str());
            xml_attribute xregion    = parentn.attribute(ATTR_GRegion.c_str());
            //xml_attribute xeventname = parentn.attribute(ATTR_DirName.c_str());

            m_out_gver = StrToGameVersion(xversion.value());
            m_out_reg  = StrToGameRegion (xregion.value());

            return std::move( LevelScript( utils::GetBaseNameOnly(file), 
                             std::move(ParseRoutines(parentn)),
                             std::move(ParseLSD   (parentn))));
        }

    private:

        LevelScript::scriptgrps_t ParseRoutines( xml_node & parentn )
        {
            using namespace scriptXML;
            LevelScript::scriptgrps_t groups;

            for( auto & grp : parentn.children(NODE_ScriptGroup.c_str()) )
            {
                xml_attribute xname = grp.attribute(ATTR_GrpName.c_str());
                if( !xname )
                {
                    if(utils::LibWide().isLogOn())
                        clog<<"->Skipped unnamed group!\n";
                    continue;
                }
                else if( utils::LibWide().isLogOn() )
                    clog<<"->Parsing Script Group " <<xname.value() <<"\n";

                LevelScript::scriptgrps_t::value_type agroup( xname.value() );
                for( auto & comp : grp )
                {
                    if( comp.name() == NODE_ScriptSeq )
                        HandleSequence( comp, agroup );
                    else if( comp.name() == NODE_ScriptData )
                        HandleData(comp, agroup);
                }
                groups.push_back(std::forward<LevelScript::scriptgrps_t::value_type>(agroup));
            }

            return std::move(groups);
        }

        LevelScript::lsdtbl_t ParseLSD( xml_node & parentn )
        {
            using namespace scriptXML;
            LevelScript::lsdtbl_t table;
            xml_node xlsd = parentn.child(NODE_LSDTbl.c_str());

            for( auto & lsde : xlsd.children(NODE_GrpNameRef.c_str()) )
            {
                xml_attribute xnameref = lsde.attribute(ATTR_GrpName.c_str());
                if(xnameref)
                {
                    LevelScript::lsdtbl_t::value_type val;
                    std::fill(val.begin(), val.end(), 0);
                    std::copy_n(xnameref.value(), strnlen_s(xnameref.value(), val.size()) , val.begin());
                    table.push_back(val);
                }
            }
            return std::move(table);
        }

        /*
            Returns if the sequence was unionall.ssb
        */
        void HandleSequence(xml_node & seqn, LevelScript::scriptgrps_t::value_type & destgrp)
        {
            using namespace scriptXML;
            xml_attribute xname = seqn.attribute( ATTR_ScrSeqName.c_str() );
            string        name  = xname.value();

            if( !xname )
            {
                if( utils::LibWide().isLogOn() )
                    clog << "\t*Unamed sequence, skipping!\n";
                return;
            }
            else if( utils::LibWide().isLogOn() )
                clog << "\t*Parsing " <<name <<"\n";

            //If we're unionall.ssb, change the type accordingly
            if( name == ScriptPrefix_unionall )
                destgrp.Type(eScriptSetType::UNK_unionall);

            //If we're unionall.ssb, change the type accordingly
            destgrp.Sequences().emplace( std::forward<string>(name), 
                                         std::forward<Script>( SSBXMLParser(m_out_gver, m_out_reg, m_gconf)(seqn) ) );
        }

        /*
            Return a type based on the kind of data it is
        */
        void HandleData(xml_node & datan, LevelScript::scriptgrps_t::value_type & destgrp)
        {
            using namespace scriptXML;
            xml_attribute    xname = datan.attribute( ATTR_ScrDatName.c_str() );
            xml_attribute    xtype = datan.attribute( ATTR_ScriptType.c_str() );
            if(!xtype)
            {
                if( utils::LibWide().isLogOn() )
                    clog << "\t*Untyped script data, skipping!\n";
                return;
            }
            eScrDataTy scrty = StrToScriptDataType(xtype.value());

            if(!xname)
            {
                if( utils::LibWide().isLogOn() )
                    clog << "\t*Unamed script data, skipping!\n";
                return;
            }
            else if( utils::LibWide().isLogOn() )
                clog << "\t*Parsing " <<xname.value() <<", type : " <<xtype.value() <<"\n";

            //Set Appropriate type
            switch(scrty)
            {
                case eScrDataTy::SSS:
                {
                    destgrp.Type(eScriptSetType::UNK_station);
                    break;
                }
                case eScrDataTy::SSE:
                {
                    destgrp.Type(eScriptSetType::UNK_enter);
                    break;
                }
                case eScrDataTy::SSA:
                {
                    destgrp.Type(eScriptSetType::UNK_acting);
                    break;
                }
                default:
                {
                    if( utils::LibWide().isLogOn() )
                        clog << "\t*Couldn't determine script type. Skipping!\n";
                    return;
                }
            };

            destgrp.SetData( SSDataXMLParser(m_gconf)(datan) );
        }

    private:
        eGameRegion         & m_out_reg;
        eGameVersion        & m_out_gver;
        const ConfigLoader  & m_gconf;
    };


//==============================================================================
//  GameScriptsXMLWriter
//==============================================================================

    /*****************************************************************************************
        SSBXMLWriter
            Writes the content of a SSB file.
    *****************************************************************************************/
    class SSBXMLWriter
    {
    public:
        SSBXMLWriter( const Script & seq, const ConfigLoader & conf, const scriptprocoptions & options )
            :m_gconf(conf),
            m_seq(seq), 
            //m_version(version), 
            //m_region(region),
            m_opinfo(conf.GetGameVersion().version), 
            // m_lvlinf(version), 
            m_paraminf(conf),
            m_options(options)
            //m_commentoffsets(bprintcmdoffsets)
        {}
        
        void operator()( xml_node & parentn )
        {
            using namespace scriptXML;
            WriteSSBCommentHeader(parentn);
            xml_node ssbnode = parentn.append_child( NODE_ScriptSeq.c_str() );
            AppendAttribute( ssbnode, ATTR_Name, m_seq.Name() );

            //prepare for counting references to strings
            if(!m_seq.ConstTbl().empty())
                m_referedconstids.reserve(m_seq.ConstTbl().size());
            if( (!m_seq.StrTblSet().empty()) && (!m_seq.StrTblSet().begin()->second.empty()) )
                m_referedstrids.reserve(m_seq.StrTblSet().begin()->second.size());

            WriteCode             (ssbnode);
            WriteOrphanedConstants(ssbnode);
            WriteOrphanedStrings  (ssbnode);
        }

    private:

        inline void WriteSSBCommentHeader(xml_node & parentn)
        {
            stringstream sstrstats;
            size_t       nbaliases = 0;
            sstrstats << "File contained " << m_seq.ConstTbl().size() << " constant(s), ";
            if( !m_seq.StrTblSet().empty() )
                sstrstats <<m_seq.StrTblSet().begin()->second.size() <<" string(s), ";
            else
                sstrstats <<"0 string(s), ";
            sstrstats <<m_seq.Routines().size() <<" routine(s),";
            for( const auto & grpa : m_seq.Routines() )
                if(grpa.IsAliasOfPrevGroup()) ++nbaliases;
            sstrstats <<" of which " <<nbaliases <<" are aliases to their previous routine." ;
            WriteCommentNode( parentn, sstrstats.str() );
        }

        /*
        */
        xml_node SetupRoutine(xml_node & parent, const ScriptRoutine & cur, size_t routinecnt, bool isUnionall)
        {
            using namespace scriptXML;
            xml_node     xroutine;
            string       nameid;
            string       routinetype = RoutineTyToStr(cur.type);
            stringstream sstr;
            WriteCommentNode( parent, "*******************************************************" );
            
            sstr << std::right <<std::setw(15) <<std::setfill(' ') <<routinetype <<" #" <<routinecnt;
                
            if( isUnionall )
            {
                const auto * pinfo = m_paraminf.CRoutine(routinecnt);
                if(pinfo)
                {
                    nameid = pinfo->name;
                    sstr<<" - " <<nameid;
                }
            }
 
            WriteCommentNode( parent, sstr.str() );
            WriteCommentNode( parent, "*******************************************************" );

            if( cur.IsAliasOfPrevGroup() )
                xroutine = parent.append_child( NODE_RoutineAlias.c_str() );
            else
                xroutine = parent.append_child( routinetype.c_str() );

            if( isUnionall && !nameid.empty() )
                AppendAttribute( xroutine, ATTR_RoutineID, nameid );
            else
                AppendAttribute( xroutine, ATTR_RoutineID, routinecnt );

            //if( cur.IsAliasOfPrevGroup() )
            //    AppendAttribute( xroutine, ATTR_RoutineType, routinetype );
            
            //If we don't have a paramter normally, and the parameter is 0, just skip over this and return!
            if( !RoutineHasParameter(cur.type) && cur.unk2 == 0 )
                return xroutine;
            
            eOpParamTypes  rparamty  = RoutineParameterType(cur.type);
            const string * paramname = OpParamTypesToStr(rparamty);

            if(paramname)
            {
                switch(rparamty)
                {
                    case eOpParamTypes::Unk_LivesRef:
                    {
                        const livesent_info * pinf = m_paraminf.LivesInfo(cur.unk2);
                        if(pinf)
                            AppendAttribute( xroutine, *paramname, pinf->name );
                        else
                            AppendAttribute( xroutine, *paramname, cur.unk2 );
                        break;
                    }
                    case eOpParamTypes::Unk_ObjectRef:
                    {
                        AppendAttribute(xroutine, *paramname, m_paraminf.ObjectIDToStr(cur.unk2) );
                        break;
                    }
                    case eOpParamTypes::Unk_PerformerRef:
                    default:
                    { 
                        AppendAttribute( xroutine, *paramname, cur.unk2 );
                    }
                };
            }
            else
                AppendAttribute( xroutine, ATTR_RoutineParam2, cur.unk2 );

            return xroutine;
        }

        void WriteCode( xml_node & parentn )
        {
            using namespace scriptXML;
            xml_node xcode = parentn.append_child( NODE_Code.c_str() );
            const bool isUnionall = m_seq.Name() == ScriptPrefix_unionall;

            size_t routinecnt = 0;
            
            for( const auto & routine : m_seq.Routines() )
            {
                xml_node xroutine = SetupRoutine(xcode, routine, routinecnt, isUnionall );

                if( !routine.IsAliasOfPrevGroup() )
                {
                    for( const auto & instr : routine.instructions )
                        HandleInstruction(xroutine, instr);
                }
                ++routinecnt;
            }
        }

        inline void HandleInstruction( xml_node & groupn, const pmd2::ScriptInstruction & instr )
        {
            using namespace scriptXML;
            if(m_options.bmarkoffsets)
            {
                stringstream sstr;
                sstr << "Offset : 0x" <<std::hex <<std::uppercase <<instr.dbg_origoffset;
                WriteCommentNode( groupn, sstr.str() );
            }
            switch(instr.type)
            {
                case eInstructionType::Command:
                {
                    WriteInstruction(groupn,instr);
                    break;
                }
                case eInstructionType::MetaCaseLabel:
                case eInstructionType::MetaLabel:
                {
                    WriteMetaLabel(groupn,instr);
                    break;
                }
                case eInstructionType::MetaSwitch:
                {
                    WriteMetaSwitch(groupn,instr);
                    break;
                }
                case eInstructionType::MetaAccessor:
                {
                    WriteMetaAccessor(groupn,instr);
                    break;
                }
                case eInstructionType::MetaReturnCases:
                {
                    WriteMetaReturnCases(groupn,instr);
                    break;
                }
                default:
                {
                    clog<<"SSBXMLWriter::HandleInstruction(): Encountered invalid instruction type!! Skipping!\n";
                }
            };                
        }

        inline void WriteMetaReturnCases(xml_node & groupn, const pmd2::ScriptInstruction & intr)
        {
            WriteInstructionWithSubInst(groupn,intr);
        }

        inline void WriteMetaSpecRet(xml_node & groupn, const pmd2::ScriptInstruction & intr)
        {
            WriteInstructionWithSubInst(groupn,intr);
        }

        inline void WriteMetaSwitch(xml_node & groupn, const pmd2::ScriptInstruction & intr)
        {
            WriteInstructionWithSubInst(groupn,intr);
        }

        inline void WriteMetaAccessor(xml_node & groupn, const pmd2::ScriptInstruction & intr)
        {
            if( m_options.bnodeisinst )
                WriteInstructionWithSubInst<true>(groupn,intr);
            else
                WriteInstructionWithSubInst<false>(groupn,intr);
        }

        template<bool _UseInstNameAsNodeName=true>
            void WriteInstructionWithSubInst(xml_node & groupn, const pmd2::ScriptInstruction & intr)
        {
            using namespace scriptXML;
            OpCodeInfoWrapper curinf  = m_opinfo.Info(intr.value);
            if(curinf)
            {
                xml_node xparent = AppendChildNode(groupn, curinf.Name() );

                //Write the parent's params
                for( size_t cntparam= 0; cntparam < intr.parameters.size(); ++cntparam )
                    WriteInstructionParam(xparent, curinf, intr, cntparam);

                //Write the sub-nodes
                WriteSubInstructions<_UseInstNameAsNodeName>(xparent, intr);
            }
            else
            {
                stringstream ss;
                ss <<"WriteInstructionWithSubInst(): Unknown opcode!! " <<groupn.path();
                throw std::runtime_error(ss.str());
            }
        }

        template<bool _UseInstNameAsNodeName>
            void WriteSubInstructions(xml_node & parentinstn, const pmd2::ScriptInstruction & intr);

        template<>
            void WriteSubInstructions<true>(xml_node & parentinstn, const pmd2::ScriptInstruction & instr)
        {
            using namespace scriptXML;
            for(const auto & subinst : instr.subinst)
            {
                OpCodeInfoWrapper curinf = m_opinfo.Info(subinst.value);
                xml_node          xcase  = AppendChildNode(parentinstn, curinf.Name());
                
                //Write parameters
                for( size_t cntparam= 0; cntparam < subinst.parameters.size(); ++cntparam )
                    WriteInstructionParam(xcase, curinf, subinst, cntparam);
            }
        }

        template<>
            void WriteSubInstructions<false>(xml_node & parentinstn, const pmd2::ScriptInstruction & instr)
        {
            using namespace scriptXML;
            for(const auto & subinst : instr.subinst)
            {
                WriteInstruction(parentinstn,subinst);
            }
        }

        inline void WriteMetaLabel(xml_node & groupn, const pmd2::ScriptInstruction & instr)
        {
            using namespace scriptXML;
            AppendAttribute( AppendChildNode(groupn, NODE_MetaLabel), ATTR_LblID, instr.value );
        }

        inline void WriteMetaCaseLabel(xml_node & groupn, const pmd2::ScriptInstruction & instr)
        {
            using namespace scriptXML;
            AppendAttribute( AppendChildNode(groupn, NODE_MetaCaseLabel), ATTR_LblID, instr.value );
        }

        void WriteInstruction(xml_node & groupn, const pmd2::ScriptInstruction & instr)
        {
            using namespace scriptXML;
            OpCodeInfoWrapper opinfo = m_opinfo.Info(instr.value);
            xml_node          xinstr;

            if(opinfo)
            {
                if(m_options.bnodeisinst)
                    xinstr = groupn.append_child( opinfo.Name().c_str() );
                else
                {
                    xinstr = groupn.append_child( NODE_Instruction.c_str() );
                    AppendAttribute( xinstr, ATTR_Name, opinfo.Name() );
                }

                for( size_t cntparam= 0; cntparam < instr.parameters.size(); ++cntparam )
                {
                    WriteInstructionParam( xinstr, opinfo, instr, cntparam );
                }
            }
            else
            {
                cout<<"\n<!>- SSBXMLWriter::WriteInstruction(): A bad opcode was found in the scripted sequece!! This shouldn't happen here!\n";
                assert(false);
            }
        }

        //! #TODO: This will need to be better handled. We only really have strings that need a very special
        //!         treatment. All other parameters could be easily handled by a dedicated object.
        template<typename _Inst_ty>
            void WriteInstructionParam( xml_node & instn, const OpCodeInfoWrapper & opinfo, const _Inst_ty & intr, size_t cntparam )
        {
            using namespace scriptXML;
            const string * pname = nullptr;

            //Check for unique parameter names 
            if( ( cntparam < opinfo.ParamInfo().size() ) && 
                ( (pname = OpParamTypesToStr( opinfo.ParamInfo()[cntparam].ptype )) != nullptr) )
            {
                eOpParamTypes ptype      = opinfo.ParamInfo()[cntparam].ptype;
                size_t        ptypeindex = static_cast<size_t>(ptype);
                uint16_t      pval       = intr.parameters[cntparam];
                stringstream  deststr;
                deststr << *pname;

#if 0
                if(cntparam > 0)
                    deststr <<"_" <<cntparam;
#endif
                switch(ptype)
                {
                    case eOpParamTypes::Boolean:   //!#TODO
                    {
                        AppendAttribute( instn, deststr.str(), static_cast<uint16_t>(pval) );
                        return;
                    }
                    //Simple integers
                    case eOpParamTypes::Integer:
                    case eOpParamTypes::Duration:
                    case eOpParamTypes::Volume:
                    case eOpParamTypes::CoordinateY:
                    case eOpParamTypes::CoordinateX:
                    {
                        int16_t val = static_cast<int16_t>(pval);
                        assert((val & 0x8000) == 0);
                        AppendAttribute( instn, deststr.str(), Convert14bTo16b(val) );
                        return;
                    }
                    case eOpParamTypes::Constant:
                    case eOpParamTypes::String:
                    {
                        WriteStringParameter(instn, pval);
                        return;
                    }
                    case eOpParamTypes::Unk_ScriptVariable:
                    {
                        const gamevariable_info * pinf = m_paraminf.GameVarInfo(pval);
                        if(pinf)
                        {
                            AppendAttribute( instn, deststr.str(), pinf->name );
                            return;
                        }
                        else
                        {
                            clog << "<!>- Got a script variable id out of range for instruction at script file offset 0x" 
                                    <<hex <<uppercase <<intr.dbg_origoffset <<dec <<nouppercase <<"!\n";
                            AppendAttribute( instn, deststr.str(), pval );
                        }
                        break;
                    }
                    case eOpParamTypes::Unk_FaceType:
                    {
                        string facename = m_paraminf.Face(pval);
                        if(!facename.empty())
                            AppendAttribute( instn, deststr.str(), facename);
                        else
                        {
                            clog << "<!>- Got an face type out of range for instruction at script file offset 0x" 
                                    <<hex <<uppercase <<intr.dbg_origoffset <<dec <<nouppercase <<"!\n";
                            AppendAttribute( instn, deststr.str(),  pval );
                        }
                        return;
                    }
                    case eOpParamTypes::Unk_LivesRef:
                    {
                        const livesent_info * pinf = m_paraminf.LivesInfo(pval);
                        if(pinf)
                            AppendAttribute( instn, deststr.str(), pinf->name );
                        else
                        {
                            clog << "<!>- Got an actor id out of range for instruction at script file offset 0x" 
                                    <<hex <<uppercase <<intr.dbg_origoffset <<dec <<nouppercase <<"!\n";
                            AppendAttribute( instn, deststr.str(), pval );
                        }
                        return;
                    }
                    case eOpParamTypes::Unk_CRoutineId:
                    {
                        const commonroutine_info * pinf = m_paraminf.CRoutine(pval);
                        if(pinf)
                            AppendAttribute( instn, deststr.str(), pinf->name );
                        else
                            AppendAttribute( instn, deststr.str(), static_cast<uint16_t>(pval) );
                        return;
                    }
                    case eOpParamTypes::InstructionOffset: //This is actually converted to a label id by the program
                    {
                        AppendAttribute( instn, deststr.str(), static_cast<uint16_t>(pval) );
                        return;
                    }
                    case eOpParamTypes::Unk_FacePosMode:
                    {
                        const string * pstr = m_paraminf.FacePosMode(pval);
                        if(pstr)
                            AppendAttribute( instn, deststr.str(), *pstr );
                        else
                        {
                            clog << "<!>- Got a face position mode out of range for instruction at script file offset 0x" 
                                    <<hex <<uppercase <<intr.dbg_origoffset <<dec <<nouppercase <<"!\n";
                            AppendAttribute( instn, deststr.str(), static_cast<uint16_t>(pval) );
                        }
                        return;
                    }
                    case eOpParamTypes::Unk_ObjectRef:
                    {
                        AppendAttribute(instn, deststr.str(), m_paraminf.ObjectIDToStr(pval) );
                        return;
                    }
                    case eOpParamTypes::Unk_LevelId:
                    {
                        if( pval == InvalidLevelID )
                            AppendAttribute( instn, deststr.str(), NullLevelId );
                        else
                        {
                            const level_info * lvlinf = m_paraminf.LevelInfo(pval);
                            if(lvlinf)
                            {
                                AppendAttribute( instn, deststr.str(), lvlinf->name );
                            }
                            else
                            {
                                //! #TODO: Log this
                                clog << "<!>- Got a level id out of range for instruction at script file offset 0x" 
                                     <<hex <<uppercase <<intr.dbg_origoffset <<dec <<nouppercase <<"!\n";
                                //assert(false);
                                stringstream sstrid;
                                sstrid <<"0x" <<hex <<uppercase <<pval; 
                                AppendAttribute( instn, deststr.str(),  sstrid.str() );
                            }
                        }
                        return;
                    }
                    case eOpParamTypes::MenuID:
                    {
                        //! #TODO: Handle references!
                        AppendAttribute( instn, deststr.str(), Convert14bTo16b(pval) );
                        return;
                    }
                    case eOpParamTypes::StationId:
                    {
                        //! #TODO: Handle stations a bit better
                        AppendAttribute( instn, deststr.str(), Convert14bTo16b(pval) );
                        return;
                    }
                    case eOpParamTypes::ActingLayerID:
                    {
                        //! #TODO: Handle "acting" layers a bit better
                        AppendAttribute( instn, deststr.str(), Convert14bTo16b(pval) );
                        return;
                    }
                    case eOpParamTypes::Unk_BgmTrack:
                    {
                        AppendAttribute( instn, deststr.str(), Convert14bTo16b(pval) );
                        return;
                    }
                    case eOpParamTypes::Direction:
                    {
                        AppendAttribute( instn, deststr.str(), m_paraminf.Direction(pval) );
                        return;
                    }
                    case eOpParamTypes::ItemID:
                    {
                        AppendAttribute( instn, deststr.str(), m_paraminf.Item(pval) );
                        return;
                    }
                    case eOpParamTypes::UNK_Placeholder:
                    {
                        break; //Just break and handle as nameless parameters
                    }
                    case eOpParamTypes::BitsFlag:   //!#TODO
                    default:
                    {
                        stringstream sstrp;
                        sstrp <<"0x" <<hex <<uppercase <<pval; 
                        AppendAttribute( instn, deststr.str(), sstrp.str() );
                        return;
                    }
                };

            }
            //When we end up here, we use generic parameter names!! 
            //! #TODO: Should be phased out once we fully define all parameters!!
            stringstream sstrval;
            sstrval <<"0x" <<hex <<uppercase <<intr.parameters[cntparam];
            if( cntparam < ATTR_Params.size() )
            {
                AppendAttribute( instn, ATTR_Params[cntparam], sstrval.str() );
            }
            else
            {
                stringstream  paramnamestr;
                paramnamestr<<ATTR_Param <<"_" <<cntparam;
                AppendAttribute( instn, paramnamestr.str(), sstrval.str() );
            }
        }

        /*
            WriteStringParameter
                Return whether it was a string to parse or not.
        */
        void WriteStringParameter( xml_node & instn, uint16_t pval )
        {
            using namespace scriptXML;
            
            //Strings and constants use the same indices. Constants first, then strings 
            if( isConstIdInRange(pval) )
            {
                auto res = m_referedconstids.insert(pval);
                if( !res.second )
                    cerr << "\nConstant duplicate reference to CID# " <<pval <<", \"" <<m_seq.ConstTbl()[pval] <<"\"!!\n";
                AppendAttribute( instn, OpParamTypesNames[static_cast<size_t>(eOpParamTypes::Constant)], m_seq.ConstTbl()[pval] );

                return;
            }
            else if( isStringIdInRange(pval - m_seq.ConstTbl().size()) ) //The string ids in the instructions include the length of the const table if there's one!
            {
                const int16_t stroffset = ToSWord(pval - m_seq.ConstTbl().size());
                auto res = m_referedstrids.insert(stroffset);
                if( !res.second )
                    cerr << "\nString duplicate reference to SID# " <<pval; 
                //Place the strings for each languages
#ifdef _DEBUG
                WriteCommentNode( instn, std::to_string(pval) );
#endif
                for( const auto & lang : m_seq.StrTblSet() )
                {
                    xml_node xlang = AppendChildNode(instn, NODE_String);
                    AppendAttribute( xlang, ATTR_Language, GetGameLangName(lang.first) );
#ifdef PMD2XML_STRING_AS_CDATA
                    AppendCData(xlang, lang.second.at(stroffset) );
#elif defined(PMD2XML_STRING_AS_PCDATA)
                    AppendPCData(xlang, lang.second.at(stroffset) );
#else
                    AppendAttribute( xlang, ATTR_Value,    lang.second.at(stroffset) );
#endif
                    if(!res.second)
                        cerr <<", \"" <<lang.second.at(stroffset) <<"\" ";
                }
                if(!res.second)
                    cerr <<"\n";
                return;
            }
            else
            {
                throw std::runtime_error( "SSBXMLWriter::WriteInstructionParam(): String ID out of range! " + instn.path() );
            }
        }

        /*
        */
        inline bool isConstIdInRange( uint16_t id )const
        {
            return id < m_seq.ConstTbl().size();
        }

        /*
        */
        inline bool isStringIdInRange( uint16_t id )const
        {
            return !(m_seq.StrTblSet().empty()) && (id < m_seq.StrTblSet().begin()->second.size());
        }

        /*
        */
        void WriteOrphanedConstants( xml_node & parentn )
        {
            using namespace scriptXML;
            if( m_seq.ConstTbl().empty() || 
               m_seq.ConstTbl().size() == m_referedconstids.size() ) //If all our consts were referenced, don't bother!
                return;

            xml_node xconsts = parentn.append_child( NODE_Constants.c_str() );

            size_t cntc = 0;
            for( const auto & aconst : m_seq.ConstTbl() )
            {
                if( m_referedconstids.find(cntc) == m_referedconstids.end() )
                {
                    stringstream sstr;
                    sstr<<"Unreferenced Const ID#" <<cntc;
                    WriteCommentNode( xconsts, sstr.str() );
                    xml_node xcst = xconsts.append_child( NODE_Constant.c_str() );
                    AppendAttribute( xcst, ATTR_Value, aconst );
                }
                ++cntc;
            }
        }

        /*
        */
        void WriteOrphanedStrings( xml_node & parentn )
        {
            using namespace scriptXML;
            if( m_seq.StrTblSet().empty() || 
                (m_referedstrids.size() == m_seq.StrTblSet().begin()->second.size()) ) //If all our strings were referenced, don't bother!
                return;

            xml_node xstrings = parentn.append_child( NODE_Strings.c_str() );

            for( const auto & alang : m_seq.StrTblSet() )
            {
                AppendAttribute( xstrings, ATTR_Language, GetGameLangName(alang.first) );

                size_t cnts = 0;
                for( const auto & astring : alang.second )
                {
                    if( m_referedstrids.find(cnts) == m_referedstrids.end() )
                    {
                        stringstream sstr;
                        sstr<<"Unreferenced String ID#" <<cnts;
                        WriteCommentNode( xstrings, sstr.str() );
                        AppendAttribute( xstrings.append_child( NODE_String.c_str() ), ATTR_Value, astring );
                    }
                    ++cnts;
                }
            }
        }

    private:
        const Script              & m_seq;
        //eGameVersion                m_version;
        //eGameRegion                 m_region;
        OpCodeClassifier            m_opinfo;
        ParameterReferences         m_paraminf;

        std::unordered_set<size_t>  m_referedstrids;   //String ids that have been referred to by an instruction
        std::unordered_set<size_t>  m_referedconstids; //constant ids that have been referred to by an instruction
        //bool                        m_commentoffsets;
        const ConfigLoader        & m_gconf;
        scriptprocoptions           m_options;

    };

    /*****************************************************************************************
        SSDataXMLWriter
            
    *****************************************************************************************/
    class SSDataXMLWriter
    {
    public:
        SSDataXMLWriter(const ScriptData & data, const ConfigLoader & conf)
            :m_data(data), m_paraminf(conf), m_gconf(conf)
        {}

        void operator()(xml_node & parentn)
        {
            using namespace scriptXML;
            xml_node xdata = AppendChildNode(parentn, NODE_ScriptData);
            AppendAttribute( xdata, ATTR_ScrDatName, m_data.Name());
            AppendAttribute( xdata, ATTR_ScriptType, ScriptDataTypeToStr(m_data.Type()));

            WriteUnkTable1(xdata);
            WritePositionMarkers(xdata);
            WriteLayers(xdata);
        }

    private:

        inline char * MakeHexa( uint16_t value, char * buffer )
        {
            std::sprintf(buffer, "0x%X", value);
            return buffer;
        }

        void WriteUnkTable1(xml_node & parentn)
        {
            using namespace scriptXML;
            if(m_data.UnkTbl1().empty())
                return;
            xml_node       xunktbl1 = AppendChildNode(parentn, NODE_UnkTable1);
            const string   IDAttrName  = *OpParamTypesToStr(eOpParamTypes::Unk_CRoutineId);
            array<char,32> buf{0};

            size_t cnt = 0;
            for( const auto & unk1ent : m_data.UnkTbl1() )
            {
                WriteCommentNode( xunktbl1, to_string(cnt) );
                xml_node xentry = AppendChildNode(xunktbl1, NODE_UnkTable1Entry);
                const auto * inf = m_paraminf.CRoutine(unk1ent.croutineid);

                if(inf)
                    AppendAttribute(xentry, IDAttrName, inf->name);
                else
                    AppendAttribute(xentry, IDAttrName, unk1ent.croutineid);

                AppendAttribute(xentry, ATTR_Unk1,      MakeHexa(unk1ent.unk1, buf.data()) );
                AppendAttribute(xentry, ATTR_Unk2,      MakeHexa(unk1ent.unk2, buf.data()) );
                AppendAttribute(xentry, ATTR_ScriptID,  unk1ent.scrid );
                ++cnt;
            }
        }

        void WritePositionMarkers(xml_node & parentn)
        {
            using namespace scriptXML;
            if(m_data.PosMarkers().empty())
                return;
            xml_node        xposmark = AppendChildNode(parentn, NODE_PositionMarkers);
            array<char,32>  buf{0};

            size_t cnt = 0;
            for( const auto & marker : m_data.PosMarkers() )
            {
                WriteCommentNode( xposmark, to_string(cnt) );
                xml_node xentry = AppendChildNode(xposmark, NODE_Marker);
                AppendAttribute(xentry, ATTR_XOffset, marker.xoff );
                AppendAttribute(xentry, ATTR_YOffset, marker.yoff );
                AppendAttribute(xentry, ATTR_Unk2, MakeHexa(marker.unk2, buf.data()) );
                AppendAttribute(xentry, ATTR_Unk3, MakeHexa(marker.unk3, buf.data()) );
                AppendAttribute(xentry, ATTR_Unk4, MakeHexa(marker.unk4, buf.data()) );
                AppendAttribute(xentry, ATTR_Unk5, MakeHexa(marker.unk5, buf.data()) );
                AppendAttribute(xentry, ATTR_Unk6, MakeHexa(marker.unk6, buf.data()) );
                AppendAttribute(xentry, ATTR_Unk7, MakeHexa(marker.unk7, buf.data()) );
                ++cnt;
            }
        }


        void WriteLayers(xml_node & parentn)
        {
            using namespace scriptXML;
            xml_node xlayers = AppendChildNode(parentn, NODE_Layers);

            size_t cntlayer = 0;
            for( const auto & layer : m_data.Layers() )
            {
                //WriteCommentNode( xlayers, "Layer " + to_string(cntlayer) );
                xml_node xlayer = AppendChildNode(xlayers, NODE_Layer);
                AppendAttribute(xlayer, ATTR_DummyID, to_string(cntlayer) );
                WriteLayerActors    (xlayer, layer);
                WriteLayerObjects   (xlayer, layer);
                WriteLayerPerformers(xlayer, layer);
                WriteLayerEvents    (xlayer, layer);
                //WriteLayerUnkTable3 (xlayer, layer);
                ++cntlayer;
            }
        }

        void WriteLayerActors(xml_node & parentn, const ScriptLayer & layer )
        {
            using namespace scriptXML;
            if(layer.lives.empty())
                return;

            xml_node        xactors     = AppendChildNode( parentn, NODE_Actors );
            const string    IDAttrName  = *OpParamTypesToStr(eOpParamTypes::Unk_LivesRef);
            size_t          cntact      = 0;
            array<char,32>  buf{0};

            for( const auto & actor : layer.lives )
            {
                WriteCommentNode( xactors, to_string(cntact) );
                xml_node              xactor = AppendChildNode( xactors, NODE_Actor );
                const livesent_info * inf    = m_paraminf.LivesInfo(actor.livesid);
                assert(inf);

                if(inf)
                    AppendAttribute(xactor, IDAttrName, inf->name);
                else
                    AppendAttribute(xactor, IDAttrName, actor.livesid);

                AppendAttribute(xactor, ATTR_Direction, m_paraminf.DirectionData(actor.facing) );
                AppendAttribute(xactor, ATTR_XOffset,   actor.xoff );
                AppendAttribute(xactor, ATTR_YOffset,   actor.yoff );
                AppendAttribute(xactor, ATTR_Unk4,      MakeHexa(actor.unk4,buf.data()) );
                AppendAttribute(xactor, ATTR_Unk5,      MakeHexa(actor.unk5,buf.data()) );
                AppendAttribute(xactor, ATTR_ScriptID,  actor.scrid );
                ++cntact;
            }
        }

        void WriteLayerObjects(xml_node & parentn, const ScriptLayer & layer )
        {
            using namespace scriptXML;
            if(layer.objects.empty())
                return;
            //WriteCommentNode( parentn, to_string(layer.objects.size()) + " object(s)" );

            xml_node        xobjects    = AppendChildNode( parentn, NODE_Objects );
            size_t          cnt         = 0;
            const string    IDAttrName  = *OpParamTypesToStr(eOpParamTypes::Unk_ObjectRef);
            array<char,32>  buf{0};
            //stringstream    sstr;

            for( const auto & entry : layer.objects )
            {
                WriteCommentNode( xobjects, to_string(cnt) );
                xml_node     xobject = AppendChildNode( xobjects, NODE_Object );

                AppendAttribute(xobject, IDAttrName, m_paraminf.ObjectIDToStr(entry.objid) );
                AppendAttribute(xobject, ATTR_Direction, m_paraminf.DirectionData(entry.facing) );
                AppendAttribute(xobject, ATTR_Unk2, MakeHexa(entry.unk2,buf.data()) );
                AppendAttribute(xobject, ATTR_Unk3, MakeHexa(entry.unk3,buf.data()) );
                AppendAttribute(xobject, ATTR_XOffset, entry.xoff );
                AppendAttribute(xobject, ATTR_YOffset, entry.yoff );
                AppendAttribute(xobject, ATTR_Unk6, MakeHexa(entry.unk6,buf.data()) );
                AppendAttribute(xobject, ATTR_Unk7, MakeHexa(entry.unk7,buf.data()) );
                AppendAttribute(xobject, ATTR_ScriptID, entry.scrid );
                ++cnt;
            }
        }

        void WriteLayerPerformers(xml_node & parentn, const ScriptLayer & layer )
        {
            using namespace scriptXML;
            if(layer.performers.empty())
                return;
            //WriteCommentNode( parentn, to_string(layer.performers.size()) + " performer(s)" );

            xml_node        xperfs      = AppendChildNode( parentn, NODE_Performers );
            size_t          cnt         = 0;
            const string    IDAttrName  = *OpParamTypesToStr(eOpParamTypes::Unk_PerformerRef);
            array<char,32>  buf{0};

            for( const auto & entry : layer.performers )
            {
                WriteCommentNode( xperfs, to_string(cnt) );
                xml_node xperf = AppendChildNode( xperfs, NODE_Performer );

                //const livesent_info * inf    = m_paraminf.LivesInfo(actor.livesid);
                //assert(inf);

                //if(inf)
                //    AppendAttribute(xactor, IDAttrName, inf->name);
                //else
                //    AppendAttribute(xactor, IDAttrName, actor.livesid);

                AppendAttribute(xperf, ATTR_Unk0, MakeHexa(entry.unk0,buf.data()) );
                AppendAttribute(xperf, ATTR_Unk1, MakeHexa(entry.unk1,buf.data()) );
                AppendAttribute(xperf, ATTR_Unk2, MakeHexa(entry.unk2,buf.data()) );
                AppendAttribute(xperf, ATTR_Unk3, MakeHexa(entry.unk3,buf.data()) );
                AppendAttribute(xperf, ATTR_Unk4, MakeHexa(entry.unk4,buf.data()) );
                AppendAttribute(xperf, ATTR_Unk5, MakeHexa(entry.unk5,buf.data()) );
                AppendAttribute(xperf, ATTR_Unk6, MakeHexa(entry.unk6,buf.data()) );
                AppendAttribute(xperf, ATTR_Unk7, MakeHexa(entry.unk7,buf.data()) );
                AppendAttribute(xperf, ATTR_Unk8, MakeHexa(entry.unk8,buf.data()) );
                AppendAttribute(xperf, ATTR_Unk9, MakeHexa(entry.unk9,buf.data()) );
                ++cnt;
            }
        }

        void WriteLayerEvents(xml_node & parentn, const ScriptLayer & layer )
        {
            using namespace scriptXML;
            if(layer.events.empty())
                return;
            //WriteCommentNode( parentn, to_string(layer.events.size()) + " event(s)" );

            xml_node        xevents     = AppendChildNode( parentn, NODE_Events );
            size_t          cnt         = 0;
            const string    IDAttrName  = *OpParamTypesToStr(eOpParamTypes::Unk_CRoutineId);
            array<char,32>  buf{0};

            for( const auto & entry : layer.events )
            {
                WriteCommentNode( xevents, to_string(cnt) );
                xml_node     xevent = AppendChildNode( xevents, NODE_Event );
                const auto * inf = m_paraminf.CRoutine(entry.unk0);

                if(inf)
                    AppendAttribute(xevent, IDAttrName, inf->name);
                else
                    AppendAttribute(xevent, IDAttrName, entry.unk0);

                AppendAttribute(xevent, ATTR_Unk1, MakeHexa(entry.unk1,buf.data()) );
                AppendAttribute(xevent, ATTR_Unk2, MakeHexa(entry.unk2,buf.data()) );
                AppendAttribute(xevent, ATTR_Unk3, MakeHexa(entry.unk3,buf.data()) );
                AppendAttribute(xevent, ATTR_Unk4, MakeHexa(entry.unk4,buf.data()) );
                AppendAttribute(xevent, ATTR_Unk5, MakeHexa(entry.unk5,buf.data()) );
                AppendAttribute(xevent, ATTR_Unk6, MakeHexa(entry.unk6,buf.data()) );
                ++cnt;
            }
        }

    private:
        const ScriptData   & m_data;
        ParameterReferences  m_paraminf;
        const ConfigLoader & m_gconf;
    };

    /*****************************************************************************************
        GameScriptsXMLWriter
            Write XML for the content of a single script directory.
    *****************************************************************************************/
    class GameScriptsXMLWriter
    {
    public:
        GameScriptsXMLWriter( const LevelScript & set, const ConfigLoader & conf )
            :m_scrset(set), m_gconf(conf)
        {}

        void Write(const std::string & destdir, const scriptprocoptions & options )
        {
            using namespace scriptXML;
            m_options = options;
            stringstream sstrfname;
            sstrfname << utils::TryAppendSlash(destdir) <<m_scrset.Name() <<".xml";
            xml_document doc;
            xml_node     xroot = doc.append_child( ROOT_ScripDir.c_str() );
            AppendAttribute( xroot, ATTR_GVersion, GetGameVersionName(m_gconf.GetGameVersion().version) );
            AppendAttribute( xroot, ATTR_GRegion,  GetGameRegionNames(m_gconf.GetGameVersion().region) );

            //Write stuff
            WriteLSDTable(xroot);

            for( const auto & entry : m_scrset.Components() )
                WriteSet(xroot,entry);

            const unsigned int flag = (m_options.bescapepcdata)? pugi::format_default  :
                                        pugi::format_indent | pugi::format_no_escapes;
            //Write doc
            if( ! doc.save_file( sstrfname.str().c_str(), "\t", flag, pugi::encoding_utf8 ) )
                throw std::runtime_error("GameScriptsXMLWriter::Write(): PugiXML can't write xml file " + sstrfname.str());
        }

    private:

        void WriteSet( xml_node & parentn, const ScriptSet & set )
        {
            using namespace scriptXML;
            WriteCommentNode(parentn, "##########################################" );
            stringstream sstr;
            sstr <<std::right <<std::setw(10) <<setfill(' ') <<set.Identifier() <<" Set";
            WriteCommentNode(parentn, sstr.str() );
            WriteCommentNode(parentn, "##########################################" );
            sstr.str(string());
            sstr.clear();
            sstr << "Has " <<((set.Data() != nullptr)? "1 data file, and ": "") <<set.Sequences().size() <<" associated script(s)";
            WriteCommentNode(parentn, sstr.str() );
            xml_node xgroup = AppendChildNode( parentn, NODE_ScriptGroup );

            AppendAttribute( xgroup, ATTR_GrpName, set.Identifier() );

            if( set.Data() )
                WriteSSDataContent( xgroup, *set.Data() );

            for( const auto & seq : set.Sequences() )
                WriteSSBContent(xgroup, seq.second);
        }

        void WriteLSDTable( xml_node & parentn )
        {
            using namespace scriptXML;
            xml_node xlsd = AppendChildNode( parentn, NODE_LSDTbl );

            size_t cntlsd = 0;
            for( const auto & entry : m_scrset.LSDTable() )
            {
                WriteCommentNode(xlsd, to_string(cntlsd));
                AppendAttribute( AppendChildNode(xlsd,NODE_GrpNameRef), ATTR_GrpName, string(entry.begin(), entry.end()) );
                ++cntlsd;
            }
        }

        inline void WriteSSBContent( xml_node & parentn, const Script & seq )
        {
            using namespace scriptXML;
            WriteCommentNode(parentn, "++++++++++++++++++++++" );
            stringstream sstr;
            sstr <<std::right <<std::setw(10) <<setfill(' ') <<seq.Name() <<" Script";
            WriteCommentNode(parentn, sstr.str() );
            WriteCommentNode(parentn, "++++++++++++++++++++++" );
            SSBXMLWriter(seq,  m_gconf, m_options)(parentn);
        }

        inline void WriteSSDataContent( xml_node & parentn, const ScriptData & dat )
        {
            using namespace scriptXML;
            WriteCommentNode(parentn, "======================" );
            stringstream sstr;
            sstr <<std::right <<std::setw(10) <<setfill(' '); 
            switch(dat.Type())
            {
                case eScrDataTy::SSA:
                {
                    sstr <<dat.Name() <<" \"Acting\"";
                    break;
                }
                case eScrDataTy::SSE:
                {
                    sstr <<dat.Name() <<" \"Enter\"";
                    break;
                }
                case eScrDataTy::SSS:
                {
                    sstr <<dat.Name() <<" \"Station\"";
                    break;
                }
                default:
                    sstr <<dat.Name() <<" Data";
            };
            WriteCommentNode(parentn, sstr.str() );
            WriteCommentNode(parentn, "======================" );
            SSDataXMLWriter(dat, m_gconf)(parentn);
            //! #TODO: DO SOMETHING HERE
        }



    private:
        const LevelScript       & m_scrset;
        const ConfigLoader      & m_gconf;
        scriptprocoptions         m_options;
    };

//==============================================================================
//  GameScripts
//==============================================================================

    /*
        RunLevelXMLImport
            Helper for importing script data from XML.
            Is used in packaged tasks to be handled by the thread pool.
    */
    //bool RunLevelXMLImport( const ScrSetLoader & ldr, string fname, eGameRegion reg, eGameVersion ver, atomic<uint32_t> & completed )
    bool RunLevelXMLImport( GameScripts      & gs, 
                            string             fname, 
                            string             dest, 
                            atomic<uint32_t> & completed )
    {
        try
        {
            eGameRegion  tempregion  = eGameRegion::Invalid;
            eGameVersion tempversion = eGameVersion::Invalid;
            gs.WriteScriptSet( std::move( GameScriptsXMLParser(tempregion,tempversion, gs.GetConfig()).Parse(fname) ) );
            if( tempregion != gs.Region() || tempversion != gs.Version() )
                throw std::runtime_error("GameScripts::ImportXML(): Event " + fname + " from the wrong region or game version was loaded!! Ensure the version and region attributes are set properly!!");
        }
        catch(const std::exception &)
        {
            throw_with_nested(std::runtime_error("RunLevelXMLImport(): Error in file " + fname));
        }
        ++completed;
        return true;
    }

    /*
        RunLevelXMLExport
            Helper for exporting script data as XML.
            Is used in packaged tasks to be handled by the thread pool.
    */
    bool RunLevelXMLExport( const ScrSetLoader      & entry, 
                            const string            & dir, 
                            const ConfigLoader      & gs, 
                            const scriptprocoptions & options,
                            atomic<uint32_t>        & completed )
    {
        try
        {
            GameScriptsXMLWriter(entry(), gs).Write(dir, options);
        }
        catch(const std::exception & )
        {
            throw_with_nested(std::runtime_error("RunLevelXMLExport(): Error processing " + entry.path()));
        }
        ++completed;
        return true;
    }

    /*
        PrintProgressLoop
            To be run on a separate thread. Displays a percentage of "completed" on "total".
            Stops looping when "bDoUpdate" is false.
    */
    void PrintProgressLoop( atomic<uint32_t> & completed, uint32_t total, atomic<bool> & bDoUpdate )
    {
        static const chrono::milliseconds ProgressUpdThWait = chrono::milliseconds(200);
        while( bDoUpdate )
        {
            if( completed.load() <= total )
            {
                uint32_t percent = ( (100 * completed.load()) / total );
                cout << "\r" <<setw(3) <<setfill(' ') <<percent <<"%" <<setw(15) <<setfill(' ') <<" ";
            }
            this_thread::sleep_for(ProgressUpdThWait);
        }
    }

    /*
    */
    void ImportXMLGameScripts(const std::string & dir, 
                              GameScripts & out_dest, const 
                              scriptprocoptions & options )
    {
        if(out_dest.m_setsindex.empty())
            throw std::runtime_error("ImportXMLGameScripts(): No script data to load to!!");

        decltype(out_dest.Region())   tempregion;
        decltype(out_dest.Version()) tempversion;
        atomic_bool                  shouldUpdtProgress = true;
        atomic<uint32_t>             completed = 0;
        future<void>                 updatethread;
        //Grab our version and region from the 
        if(utils::LibWide().ShouldDisplayProgress())
            cout<<"<*>- Compiling COMON.xml..\n";

        stringstream commonfilename;
        commonfilename <<utils::TryAppendSlash(dir) <<DirNameScriptCommon <<".xml";
        out_dest.m_common = std::move( GameScriptsXMLParser(tempregion, tempversion, out_dest.GetConfig()).Parse(commonfilename.str()) );

        if( tempregion != out_dest.Region() || tempversion != out_dest.Version() )
            throw std::runtime_error("GameScripts::ImportXML(): The COMMON event from the wrong region or game version was loaded!! Ensure the version and region attributes are set properly!!");

        if(utils::LibWide().ShouldDisplayProgress())
        {
            cout<<"<!>- Detected game region \"" <<GetGameRegionNames(out_dest.Region()) 
                <<"\", and version \"" <<GetGameVersionName(out_dest.Version())<<"!\n";
        }
        //Write out common
        out_dest.WriteScriptSet(out_dest.m_common);

        //Prepare import of everything else!
        multitask::CMultiTaskHandler taskhandler;
        Poco::DirectoryIterator dirit(dir);
        Poco::DirectoryIterator dirend;
        while( dirit != dirend )
        {
            if( dirit->isFile() && dirit.path().getExtension() == "xml" )
            {

                Poco::Path destination(out_dest.GetScriptDir());
                destination.append(dirit.path().getBaseName());
                taskhandler.AddTask( multitask::pktask_t( std::bind( RunLevelXMLImport, 
                                                                     std::ref(out_dest), 
                                                                     dirit->path(), 
                                                                     destination.toString(),
                                                                     std::ref(completed) ) ) );
            }
            ++dirit;
        }

        try
        {
            if(utils::LibWide().ShouldDisplayProgress())
            {
                assert(!out_dest.m_setsindex.empty());
                cout<<"\n<*>- Compiling Scripts..\n";
                std::packaged_task<void()> task( std::bind(&PrintProgressLoop, 
                                                           std::ref(completed), 
                                                           out_dest.m_setsindex.size(), 
                                                           std::ref(shouldUpdtProgress)) );
                updatethread = std::move(task.get_future());
                std::thread(std::move(task)).detach();
                //updtProgress = std::async( std::launch::async, 
                //                           PrintProgressLoop, 
                //                           std::ref(completed), 
                //                           out_dest.m_setsindex.size(), 
                //                           std::ref(shouldUpdtProgress) );
            }
            taskhandler.Execute();
            taskhandler.BlockUntilTaskQueueEmpty();
            taskhandler.StopExecute();

            shouldUpdtProgress = false;
            if(updatethread.valid())
                updatethread.get();
            if(utils::LibWide().ShouldDisplayProgress())
                cout<<"\r100%"; //Can't be bothered to make another drawing update

        }
        catch(...)
        {
            shouldUpdtProgress = false;
            if(updatethread.valid())
                updatethread.get();
            std::rethrow_exception( std::current_exception() );
        }


        if(utils::LibWide().ShouldDisplayProgress())
            cout<<"\n";
    }

    /*
        ExportGameScriptsXML
            
    */
    void ExportGameScriptsXML(const std::string         & dir, 
                              const GameScripts         & gs, 
                              const scriptprocoptions   & options )
    {
        //Export COMMON first
        if(utils::LibWide().ShouldDisplayProgress())
            cout<<"<*>- Writing COMMOM.xml..";
        GameScriptsXMLWriter(gs.m_common, gs.GetConfig() ).Write(dir, options);

        atomic_bool                  shouldUpdtProgress = true;
        future<void>                 updtProgress;
        atomic<uint32_t>             completed = 0;
        multitask::CMultiTaskHandler taskhandler;
        //Export everything else
        for( const auto & entry : gs.m_setsindex )
        {
            taskhandler.AddTask( multitask::pktask_t( std::bind( RunLevelXMLExport, 
                                                                 std::cref(entry.second), 
                                                                 std::cref(dir), 
                                                                 std::cref(gs.GetConfig()),
                                                                 std::cref(options),
                                                                 std::ref(completed) ) ) );
        }

        try
        {
            if(utils::LibWide().ShouldDisplayProgress())
            {
                cout<<"\n<*>- Exporting the rest..\n";
                updtProgress = std::async( std::launch::async, 
                                           PrintProgressLoop, 
                                           std::ref(completed), 
                                           gs.m_setsindex.size(), 
                                           std::ref(shouldUpdtProgress) );
            }
            taskhandler.Execute();
            taskhandler.BlockUntilTaskQueueEmpty();
            taskhandler.StopExecute();

            shouldUpdtProgress = false;
            if( updtProgress.valid() )
                updtProgress.get();
            if(utils::LibWide().ShouldDisplayProgress())
                cout<<"\r100%"; //Can't be bothered to make another drawing update
        }
        catch(...)
        {
            shouldUpdtProgress = false;
            if( updtProgress.valid() )
                updtProgress.get();
            std::rethrow_exception( std::current_exception() );
        }

        if(utils::LibWide().ShouldDisplayProgress())
            cout<<"\n";
    }

    /*
    */
    void ScriptSetToXML( const LevelScript          & set, 
                         const ConfigLoader         & gconf, 
                         const scriptprocoptions    & options, 
                         const std::string          & destdir )
    {
        if( gconf.GetGameVersion().version < eGameVersion::NBGameVers && gconf.GetGameVersion().region < eGameRegion::NBRegions )
            GameScriptsXMLWriter(set, gconf).Write(destdir, options);
        else
            throw std::runtime_error("ScriptSetToXML() : Error, invalid version or region!");
    }

    /*
    */
    LevelScript XMLToScriptSet( const std::string   & srcdir, 
                                eGameRegion         & out_reg, 
                                eGameVersion        & out_gver, 
                                const ConfigLoader  & gconf  )
    {
        return std::move( GameScriptsXMLParser(out_reg, out_gver, gconf).Parse(srcdir) );
    }

    /*
    */
    void ScriptToXML( const Script        & scr, 
                      const ConfigLoader  & gconf, 
                      const scriptprocoptions & options,
                      const std::string   & destdir )
    {
        using namespace scriptXML;
        stringstream sstrfname;
        sstrfname << utils::TryAppendSlash(destdir) <<scr.Name() <<".xml";
        xml_document doc;
        xml_node     xroot = doc.append_child( ROOT_SingleScript.c_str() );
        AppendAttribute( xroot, ATTR_GVersion, GetGameVersionName(gconf.GetGameVersion().version) );
        AppendAttribute( xroot, ATTR_GRegion,  GetGameRegionNames(gconf.GetGameVersion().region) );

        SSBXMLWriter(scr, gconf, options)(xroot);

        //Write stuff
        const unsigned int flag = (options.bescapepcdata)? pugi::format_default  : 
                                    pugi::format_indent | pugi::format_no_escapes;
        //Write doc
        if( ! doc.save_file( sstrfname.str().c_str(), "\t", flag ) )
            throw std::runtime_error("ScriptToXML(): Can't write xml file " + sstrfname.str());
    }

    /*
    */
    Script XMLToScript( const std::string & srcfile, 
                        eGameRegion & out_greg, 
                       eGameVersion & out_gver, 
                       const ConfigLoader  & gconf )
    {
        using namespace scriptXML;
        xml_document     doc;
        try
        {
            HandleParsingError( doc.load_file(srcfile.c_str()), srcfile);
        }
        catch(const std::exception & )
        {
            throw_with_nested(std::runtime_error("XMLToScript() : Pugixml failed loading file!"));
        }

        xml_node      parentn    = doc.child(ROOT_SingleScript.c_str());
        xml_attribute xversion   = parentn.attribute(ATTR_GVersion.c_str());
        xml_attribute xregion    = parentn.attribute(ATTR_GRegion.c_str());
        xml_node      seqn       = parentn.child(NODE_ScriptSeq.c_str());

        out_gver = StrToGameVersion(xversion.value());
        out_greg = StrToGameRegion (xregion.value());

        //! #FIXME: possibly load the right config? Or allow the user of the function to probe the xml first and load the correct config?
        if( out_gver != gconf.GetGameVersion().version || out_greg != gconf.GetGameVersion().region )
            throw std::runtime_error("XMLToScript(): The file \""s+srcfile+"\" is not for the version/region the program is set to expect!!");

        if(seqn)
            return std::move( SSBXMLParser(out_gver, out_greg, gconf)(seqn) );
        else
        {
            throw std::runtime_error("XMLToScript(): Couldn't find the \""+NODE_ScriptSeq+"\" node!!");
            return std::move( Script() );
        }
    }


    /*
    */
    void ScriptDataToXML( const ScriptData          & dat, 
                          const ConfigLoader        & gconf, 
                          const scriptprocoptions   & options, 
                          const std::string         & destdir )
    {
        using namespace scriptXML;
        stringstream sstrfname;
        sstrfname << utils::TryAppendSlash(destdir) <<dat.Name() <<".xml";
        xml_document doc;
        xml_node     xroot = doc.append_child( ROOT_SingleData.c_str() );
        AppendAttribute( xroot, ATTR_GVersion, GetGameVersionName(gconf.GetGameVersion().version) );
        AppendAttribute( xroot, ATTR_GRegion,  GetGameRegionNames(gconf.GetGameVersion().region) );

        SSDataXMLWriter(dat, gconf)(xroot);

        //Write stuff
        const unsigned int flag = (options.bescapepcdata)? pugi::format_default  : 
                                    pugi::format_indent | pugi::format_no_escapes;
        //Write doc
        if( ! doc.save_file( sstrfname.str().c_str(), "\t", flag ) )
            throw std::runtime_error("ScriptDataToXML(): Can't write xml file " + sstrfname.str());
    }

    /*
    */
    ScriptData XMLToScriptData( const std::string   & srcfile, 
                                eGameRegion         & out_greg, 
                                eGameVersion        & out_gver, 
                                const ConfigLoader  & gconf )
    {
        using namespace scriptXML;
        xml_document doc;

        try
        {
            HandleParsingError( doc.load_file(srcfile.c_str()), srcfile);
        }
        catch(const std::exception & )
        {
            throw_with_nested(std::runtime_error("XMLToScriptData() : Pugixml failed loading file!"));
        }

        xml_node      parentn    = doc.child(ROOT_SingleData.c_str());
        xml_attribute xversion   = parentn.attribute(ATTR_GVersion.c_str());
        xml_attribute xregion    = parentn.attribute(ATTR_GRegion.c_str());
        xml_node      datan      = parentn.child(NODE_ScriptData.c_str());

        out_gver = StrToGameVersion(xversion.value());
        out_greg = StrToGameRegion (xregion.value());

        //! #FIXME: possibly load the right config? Or allow the user of the function to probe the xml first and load the correct config?
        if( out_gver != gconf.GetGameVersion().version || out_greg != gconf.GetGameVersion().region )
            throw std::runtime_error("XMLToScriptData(): The file \""s+srcfile+"\" is not for the version/region the program is set to expect!!");

        if(datan)
            return std::move( SSDataXMLParser(gconf)(datan) );
        else
            throw std::runtime_error("XMLToScriptData(): Couldn't find the \""+NODE_ScriptData+"\" node!!");
        return ScriptData();
    }


};