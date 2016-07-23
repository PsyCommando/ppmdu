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
#include <atomic>
#include <thread>
#include <unordered_set>
using namespace std;
using namespace pugi;
using namespace pugixmlutils;

namespace pmd2
{
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


//==============================================================================
//  Constants
//==============================================================================
    namespace scriptXML
    {
        const string ROOT_ScripDir      = "Level"s;      
        const string ATTR_GVersion      = "gameversion"s; 
        const string ATTR_GRegion       = "gameregion"s;
        const string ATTR_DirName       = "eventname"s;           //Event name/filename with no extension
        const string ATTR_Name          = "name"s;

        const string NODE_LSDTbl        = "LSDTable"s;
        const string NODE_GrpNameRef    = "Ref"s;            //AKA sub-acting 
        const string ATTR_GrpName       = ATTR_Name;        //AKA sub-acting names

        const string NODE_ScriptGroup   = "ScriptSet"s;
        const string ATTR_ScrGrpName    = ATTR_Name;
        
        const string NODE_ScriptData    = "ScriptData"s;
        const string ATTR_ScriptType    = "type"s;           // ssa, sse, sss
        const string ATTR_ScrDatName    = ATTR_Name;

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

        const string NODE_Group         = "Routine"s;
        const string ATTR_GroupID       = "id"s;
        const string ATTR_GroupType     = "type"s;
        const string ATTR_GroupParam2   = "unk2"s;

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
        SSBXMLParser( eGameVersion version, eGameRegion region )
            :m_version(version), m_region(region), m_opinfo(GameVersionToOpCodeVersion(version))
        {}

        Script operator()( const xml_node & seqn)
        {
            using namespace scriptXML;
            xml_attribute xname = seqn.attribute(ATTR_ScrSeqName.c_str());

            if(!xname)
            {
                throw std::runtime_error("SSBXMLParser::operator(): Sequence is missing its \"name\" attribute!!");
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
                    stringstream sstr;
                    sstr <<"SSBXMLParser::CheckLabelReferences(): "<<aconstref.second.nbref 
                         <<" reference(s) to non-existant label ID " <<aconstref.first <<" found!!";
#ifdef _DEBUG
                    assert(false);
#endif
                    throw std::runtime_error(sstr.str());
                }
            }
        }


        /*****************************************************************************************
        *****************************************************************************************/
        void ParseCode( const xml_node & coden )
        {
            using namespace scriptXML;
            Script::grptbl_t & outtbl = m_out.Groups();

            for( const xml_node & group : coden.children() )
            {
                bool isalias = false;
                if( NODE_Group.compare(group.name()) == 0 || (isalias = (NODE_RoutineAlias.compare(group.name()) == 0)) )
                {
                    //We ignore the id on load!
                    //!#TODO: Maybe we could use routine ids to refer to routines through the "Call" commands?
                    xml_attribute  xtype = group.attribute(ATTR_GroupType.c_str());
                    xml_attribute  xunk2 = group.attribute(ATTR_GroupParam2.c_str());
                    ScriptInstrGrp grpout;
                    if(!xtype)
                    {
                        stringstream sstrer;
                        sstrer <<"SSBXMLParser::ParseCode(): Script \"" <<m_out.Name() <<"\", instruction group #" << outtbl.size() 
                               <<"doesn't have a \"" <<ATTR_GroupType <<"\" attribute!!";
                        throw std::runtime_error(sstrer.str());
                    }
                
                    grpout.type     = ToWord(xtype.as_uint());
                    grpout.unk2     = ToWord(xunk2.as_uint());
                    grpout.isalias  = isalias;

                    if( !isalias )
                    {
                        for( const xml_node & inst : group )
                            ParseInstruction(inst,grpout.instructions);
                    }

                    outtbl.push_back(std::move(grpout));
                }
            }
        }

        /*****************************************************************************************
        *****************************************************************************************/
        template<typename _InstContainer>
            void ParseInstruction( const xml_node & instn, _InstContainer & outcnt )
        {
            using namespace scriptXML;
            
            if( instn.name() == NODE_Instruction )
            {
                ParseCommand(instn, outcnt);
            }
            else if( instn.name() == NODE_MetaLabel )
            {
                ParseMetaLabel(instn, outcnt);
            }
            else
            {
                TryParseCommandNode(instn, outcnt);
            }            
            //else if( instn.name() == NODE_Data)
            //{
            //    xml_attribute     xval = instn.attribute(ATTR_Value.c_str());
            //    ScriptInstruction outinstr;
            //    outinstr.type = eInstructionType::Data;
            //    outinstr.value = static_cast<uint16_t>(xval.as_uint());
            //    outcnt.push_back(std::move(outinstr));
            //}
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
                sstrer <<"SSBXMLParser::TryParseCommandNode(): Script \"" <<m_out.Name() <<"\", instruction group #" << m_out.Groups().size() 
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
                sstrer <<"SSBXMLParser::ParseInstruction(): Script \"" <<m_out.Name() <<"\", instruction group #" << m_out.Groups().size() 
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
                throw std::runtime_error("SSBXMLParser::ParseCommand(): No matching opcode found!");

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
                        sstrer <<"SSBXMLParser::ParseCommand(): Command \"" <<instn.path() <<"\" had less parameters specified than expected!!";
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
                sstrer <<"SSBXMLParser::ParseCommand(): Command \"" <<instn.path() <<"\" had less parameters specified than expected!!";
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
                            throw std::runtime_error("SSBXMLParser::ParseDefinedParameters(): Expected String node or constref, but neither were found! " + instn.path() );
                    }
                    else
                        throw std::runtime_error("SSBXMLParser::ParseDefinedParameters(): Unexpected instruction! " + instn.path() );
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
                        throw std::runtime_error("SSBXMLParser::ParseDefinedParameters(): Expected String node, but none were found! " + instn.path() );
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
                    xml_attribute xval  = strs.attribute(ATTR_Value.c_str());

                    if( xval && xlang )
                    {
                        eGameLanguages lang = StrToGameLang(xlang.value());

                        if( lang == eGameLanguages::Invalid )
                            throw std::runtime_error("SSBXMLParser::ParseTypedCommandParameterAttribute(): Encountered unknown language "s + xlang.value() + "for string!");
                        m_strqueues[lang].push_back(xval.value());
                    }
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
            static const regex ParamNameCleaner(R"((\b\S+(?=_\d)))");
            smatch             matches;
            //First, clean the name of the parameter of any appended number if needed
            if( regex_search( name, matches, ParamNameCleaner ) && matches.size() > 1 )
                name = matches[1].str();
            return std::move(name);
        }

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
                        itf->second.nbref += 1;
                    else
                        m_labelchecker.emplace( std::make_pair( lblid, labelRefInf{ false, 1 } ) ); //We found a ref to this label, but didn't check it

                    outinst.parameters.push_back( lblid );
                    break;
                }
                case eOpParamTypes::Unk_FaceType:
                {
                    uint16_t faceid = FindFaceIDByName(param.value());
                    //if(faceid != InvalidFaceID)
                        outinst.parameters.push_back(faceid);
                    //else
                    //    outinst.parameters.push_back( ToWord(param.as_int()) );
                    break;
                }
                case eOpParamTypes::Unk_ScriptVariable:
                {
                    uint16_t varid = GameVarInfoNameToId(param.value());
                    if(varid != InvalidGameVariableID )
                        outinst.parameters.push_back(varid);
                    else
                        outinst.parameters.push_back( ToWord(param.as_int()) );
                    break;
                }
                case eOpParamTypes::Unk_LivesRef:
                {
                    uint16_t livesid = 0;

                    if( m_version == eGameVersion::EoS )
                        livesid = FindLivesIdByName_EoS(param.value());
                    else
                        livesid = FindLivesIdByName_EoTD(param.value());

                    if(livesid != InvalidLivesID )
                        outinst.parameters.push_back(livesid);
                    else
                        outinst.parameters.push_back( ToWord(param.as_int()) );
                    break;
                }
                case eOpParamTypes::Unk_PerformerRef:
                {
                    //!#TODO
                }
                case eOpParamTypes::Unk_ObjectRef:
                {
                    //!#TODO
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
            bool    bexists;
            size_t  nbref  ;
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

    };


    /*****************************************************************************************
        SSDataParser
            
    *****************************************************************************************/
    class SSDataParser
    {
    public:
        SSDataParser( eGameVersion version, eGameRegion region )
            :m_version(version), m_region(region)
        {}

        ScriptData operator()(xml_node & seqn)
        {
            return ScriptData();
        }

    private:

    private:
        eGameVersion    m_version;
        eGameRegion     m_region;
    };

    /*****************************************************************************************
        GameScriptsXMLParser
            
    *****************************************************************************************/
    class GameScriptsXMLParser
    {
    public:
        GameScriptsXMLParser(eGameRegion & out_reg, eGameVersion & out_gver)
            :m_out_reg(out_reg), m_out_gver(out_gver)
        {}

        LevelScript Parse( const std::string & file )
        {
            using namespace scriptXML;
            xml_document     doc;
            xml_parse_result loadres = doc.load_file(file.c_str());

            if( ! loadres )
            {
                stringstream sstr;
                sstr <<"GameScriptsXMLParser::Parse():Can't load XML document \"" 
                     <<file <<"\"! Pugixml returned an error : \"" << loadres.description() <<"\"";
                throw std::runtime_error(sstr.str());
            }

            xml_node      parentn    = doc.child(ROOT_ScripDir.c_str());
            xml_attribute xversion   = parentn.attribute(ATTR_GVersion.c_str());
            xml_attribute xregion    = parentn.attribute(ATTR_GRegion.c_str());
            //xml_attribute xeventname = parentn.attribute(ATTR_DirName.c_str());

            m_out_gver = StrToGameVersion(xversion.value());
            m_out_reg  = StrToGameRegion (xregion.value());

            return std::move( LevelScript( utils::GetBaseNameOnly(file), 
                             std::move(ParseGroups(parentn)),
                             std::move(ParseLSD   (parentn))));
        }

    private:

        LevelScript::scriptgrps_t ParseGroups( xml_node & parentn )
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
                                         std::forward<Script>( SSBXMLParser(m_out_gver, m_out_reg)(seqn) ) );
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
                    destgrp.Type(eScriptSetType::UNK_sub);
                    break;
                }
                case eScrDataTy::SSE:
                {
                    destgrp.Type(eScriptSetType::UNK_enter);
                    break;
                }
                case eScrDataTy::SSA:
                {
                    destgrp.Type(eScriptSetType::UNK_fromlsd);
                    break;
                }
                default:
                {
                    if( utils::LibWide().isLogOn() )
                        clog << "\t*Couldn't determine script type. Skipping!\n";
                    return;
                }
            };

            //If we're unionall.ssb, change the type accordingly
            destgrp.SetData( SSDataParser(m_out_gver, m_out_reg)(datan) );
        }

    private:
        eGameRegion  & m_out_reg;
        eGameVersion & m_out_gver;
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
        SSBXMLWriter( const Script & seq, eGameVersion version, eGameRegion region )
            :m_seq(seq), m_version(version), m_region(region),m_opinfo(GameVersionToOpCodeVersion(version))
        {}
        
        void operator()( xml_node & parentn )
        {
            using namespace scriptXML;
            stringstream sstrstats;
            sstrstats << "File contained " << m_seq.ConstTbl().size() << " constant(s), ";
            if( !m_seq.StrTblSet().empty() )
                sstrstats <<m_seq.StrTblSet().begin()->second.size() <<" string(s), ";
            else
                sstrstats <<"0 string(s), ";
            sstrstats <<m_seq.Groups().size() <<" routine(s),";
            size_t nbaliases = 0;
            for( const auto & grpa : m_seq.Groups() )
                if(grpa.IsAliasOfPrevGroup()) ++nbaliases;
            sstrstats <<" of which " <<nbaliases <<" are aliases to their previous routine." ;
            WriteCommentNode( parentn, sstrstats.str() );

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

        void WriteCode( xml_node & parentn )
        {
            using namespace scriptXML;
            xml_node xcode = parentn.append_child( NODE_Code.c_str() );

            size_t grpcnt = 0;
            for( const auto & grp : m_seq.Groups() )
            {
                WriteCommentNode( xcode, "************************" );
                stringstream sstr;
                sstr << std::right <<std::setw(15) <<std::setfill(' ') <<"Routine #" <<grpcnt;
                WriteCommentNode( xcode, sstr.str() );
                WriteCommentNode( xcode, "************************" );
                xml_node xgroup;
                if( grp.IsAliasOfPrevGroup() )
                    xgroup = xcode.append_child( NODE_RoutineAlias.c_str() );
                else
                    xgroup = xcode.append_child( NODE_Group.c_str() );
                AppendAttribute( xgroup, ATTR_GroupID,     grpcnt );
                AppendAttribute( xgroup, ATTR_GroupType,   grp.type );
                AppendAttribute( xgroup, ATTR_GroupParam2, grp.unk2 );

                if( !grp.IsAliasOfPrevGroup() )
                {
                    for( const auto & instr : grp.instructions )
                        HandleInstruction(xgroup, instr);
                }
                ++grpcnt;
            }
        }

        inline void HandleInstruction( xml_node & groupn, const pmd2::ScriptInstruction & instr )
        {
            using namespace scriptXML;
#ifdef _DEBUG
            stringstream sstr;
            sstr << "Offset : 0x" <<std::hex <<std::uppercase <<instr.dbg_origoffset;
            WriteCommentNode( groupn, sstr.str() );
#endif
            switch(instr.type)
            {
                //case eInstructionType::Data:
                //{
                //    WriteData(groupn,instr);
                //    break;
                //}
                case eInstructionType::Command:
                {
                    WriteInstruction(groupn,instr);
                    break;
                }
                case eInstructionType::MetaCaseLabel:
                //{
                //    WriteMetaCaseLabel(groupn,instr);
                //    break;
                //}
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
                case eInstructionType::MetaProcSpecRet:
                {
                    WriteMetaSpecRet(groupn,instr);
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
                throw std::runtime_error("WriteInstructionWithSubInst(): Unknown opcode!!");
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

        //inline void WriteData(xml_node & groupn, const pmd2::ScriptInstruction & instr)
        //{
        //    using namespace scriptXML;
        //    xml_node        datan = AppendChildNode(groupn, NODE_Data);
        //    xml_attribute   dval  = datan.append_attribute(ATTR_Value.c_str());
        //    stringstream    sstr;
        //    sstr <<"0x" <<hex <<uppercase <<instr.value;
        //    string str = sstr.str();    //Just in case the string gets deleted out of scope when we pass the c_str..
        //    dval.set_value( str.c_str() );
        //}

        void WriteInstruction(xml_node & groupn, const pmd2::ScriptInstruction & instr)
        {
            using namespace scriptXML;
            xml_node          xinstr = groupn.append_child( NODE_Instruction.c_str() );
            OpCodeInfoWrapper opinfo = m_opinfo.Info(instr.value);

            if(opinfo)
            {
                AppendAttribute( xinstr, ATTR_Name, opinfo.Name() );

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

        template<typename _Inst_ty>
            void WriteInstructionParam( xml_node & instn, const OpCodeInfoWrapper & opinfo, const _Inst_ty & intr, size_t cntparam )
        {
            using namespace scriptXML;
            const string * pname = nullptr;

            //Check for unique parameter names 
            if( ( cntparam < opinfo.ParamInfo().size() ) && ( (pname = OpParamTypesToStr( opinfo.ParamInfo()[cntparam].ptype )) != nullptr) )
            {
                eOpParamTypes ptype      = opinfo.ParamInfo()[cntparam].ptype;
                size_t        ptypeindex = static_cast<size_t>(ptype);
                int16_t       pval       = intr.parameters[cntparam];
                stringstream  deststr;
                deststr << *pname;

                if(cntparam > 0)
                    deststr <<"_" <<cntparam;

                switch(ptype)
                {
                    case eOpParamTypes::Constant:
                    //{
                    //    if( isConstIdInRange(pval) )
                    //    {
                    //        m_referedconstids.insert(pval);
                    //        AppendAttribute( instn, deststr.str(), m_seq.ConstTbl()[pval] );
                    //        return;
                    //    }
                    //    else
                    //    {
                    //        clog << "SSBXMLWriter::WriteInstructionParam(): Constant ID " <<pval <<" out of range!";
                    //        //throw std::runtime_error( "SSBXMLWriter::WriteInstructionParam(): Constant ID out of range!" );
                    //    }
                    //    break;
                    //}
                    case eOpParamTypes::String:
                    {
                        const int16_t stroffset = pval - m_seq.ConstTbl().size(); //The string ids in the instructions include the length of the const table if there's one!
                        if( (pval < m_seq.ConstTbl().size() || m_region == eGameRegion::Japan) && isConstIdInRange(pval) ) //Japan ignores string block completely
                        {
                            auto res = m_referedconstids.insert(pval);
                            if( !res.second )
                                cerr << "\nConstant duplicate reference to CID# " <<pval <<", \"" <<m_seq.ConstTbl()[pval] <<"\"!!\n";
                            AppendAttribute( instn, OpParamTypesNames[static_cast<size_t>(eOpParamTypes::Constant)], m_seq.ConstTbl()[pval] );
                            return;
                        }
                        else if( isStringIdInRange(stroffset) )
                        {
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
                                AppendAttribute( xlang, ATTR_Value,    lang.second.at(stroffset) );
                                if(!res.second)
                                    cerr <<", \"" <<lang.second.at(stroffset) <<"\" ";
                            }
                            if(!res.second)
                                cerr <<"\n";
                            return;
                        }
                        else
                        {
                            assert(false);
                            throw std::runtime_error( "SSBXMLWriter::WriteInstructionParam(): String ID out of range!" );
                        }
                        break;
                    }
                    case eOpParamTypes::Unk_ScriptVariable:
                    {
                        //!#TODO: Append the name of the script varaible!!
                        const gamevariableinfo * pinf = FindGameVarInfo(pval);
                        if( pinf )
                        {
                            AppendAttribute( instn, deststr.str(), pinf->str );
                            return;
                        }
                        break;
                    }
                    case eOpParamTypes::Unk_FaceType:
                    {
                        string facename = GetFaceNameByID(pval);
                        if(!facename.empty())
                            AppendAttribute( instn, deststr.str(), facename);
                        else
                            break; //Output as regular nameless param otherwise
                        return;
                    }
                    case eOpParamTypes::Unk_LivesRef:
                    {
                        if( m_version == eGameVersion::EoS )
                        {
                            const livesinfo_EoS * pinf = FindLivesInfo_EoS(pval);
                            if(pinf)
                                AppendAttribute( instn, deststr.str(), pinf->name );
                            else
                                break;
                        }
                        else if( m_version == eGameVersion::EoT || m_version == eGameVersion::EoD )
                        {
                            cout <<"\nNEED TO IMPLEMENT Unk_LivesRef support for EoTD!!!\n";
                            assert(false);
                        }
                        return;
                    }
                    case eOpParamTypes::Boolean:   //!#TODO
                    {
                        AppendAttribute( instn, deststr.str(), static_cast<uint16_t>(pval) );
                        return;
                    }
                    case eOpParamTypes::InstructionOffset: //This is actually converted to a label id by the program
                    {
                        AppendAttribute( instn, deststr.str(), static_cast<uint16_t>(pval) );
                        return;
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

            //!#TODO: we could probably make this a bit more sophisticated later on 
            stringstream sstrval;
            sstrval <<"0x" <<hex <<uppercase <<static_cast<int16_t>(intr.parameters[cntparam]);
            if( cntparam < ATTR_Params.size() )
            {
                AppendAttribute( instn, ATTR_Params[cntparam], sstrval.str() );
            }
            else
            {
                stringstream  deststr;
                deststr<<ATTR_Param <<cntparam;
                AppendAttribute( instn, deststr.str(), sstrval.str() );
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
        eGameVersion                m_version;
        eGameRegion                 m_region;
        OpCodeClassifier            m_opinfo;
        std::unordered_set<size_t>  m_referedstrids;   //String ids that have been referred to by an instruction
        std::unordered_set<size_t>  m_referedconstids; //constant ids that have been referred to by an instruction
    };

    /*****************************************************************************************
        GameScriptsXMLWriter
            Write XML for the content of a single script directory.
    *****************************************************************************************/
    class GameScriptsXMLWriter
    {
    public:
        GameScriptsXMLWriter( const LevelScript & set, eGameRegion greg, eGameVersion gver )
            :m_scrset(set), m_region(greg), m_version(gver)
        {}

        void Write(const std::string & destdir, bool bautoescape)
        {
            using namespace scriptXML;
            stringstream sstrfname;
            sstrfname << utils::TryAppendSlash(destdir) <<m_scrset.Name() <<".xml";
            xml_document doc;
            xml_node     xroot = doc.append_child( ROOT_ScripDir.c_str() );
            AppendAttribute( xroot, ATTR_GVersion, GetGameVersionName(m_version) );
            AppendAttribute( xroot, ATTR_GRegion,  GetGameRegionNames(m_region) );

            //Write stuff
            WriteLSDTable(xroot);

            for( const auto & entry : m_scrset.Components() )
                WriteGrp(xroot,entry);

            const unsigned int flag = (bautoescape)? pugi::format_default  : 
                                        pugi::format_indent | pugi::format_no_escapes;
            //Write doc
            if( ! doc.save_file( sstrfname.str().c_str(), "\t", flag ) )
                throw std::runtime_error("GameScriptsXMLWriter::Write(): Can't write xml file " + sstrfname.str());
        }

    private:

        void WriteGrp( xml_node & parentn, const ScriptSet & grp )
        {
            using namespace scriptXML;
            WriteCommentNode(parentn, "##########################################" );
            stringstream sstr;
            sstr <<std::right <<std::setw(10) <<setfill(' ') <<grp.Identifier() <<" Set";
            WriteCommentNode(parentn, sstr.str() );
            WriteCommentNode(parentn, "##########################################" );
            xml_node xgroup = AppendChildNode( parentn, NODE_ScriptGroup );

            AppendAttribute( xgroup, ATTR_GrpName, grp.Identifier() );

            if( grp.Data() )
                WriteSSDataContent( xgroup, *grp.Data() );

            for( const auto & seq : grp.Sequences() )
                WriteSSBContent(xgroup, seq.second);
        }

        void WriteLSDTable( xml_node & parentn )
        {
            using namespace scriptXML;
            xml_node xlsd = AppendChildNode( parentn, NODE_LSDTbl );

            for( const auto & entry : m_scrset.LSDTable() )
                AppendAttribute( AppendChildNode(xlsd,NODE_GrpNameRef), ATTR_GrpName, string(entry.begin(), entry.end()) );
        }

        inline void WriteSSBContent( xml_node & parentn, const Script & seq )
        {
            using namespace scriptXML;
            WriteCommentNode(parentn, "++++++++++++++++++++++" );
            stringstream sstr;
            sstr <<std::right <<std::setw(10) <<setfill(' ') <<seq.Name() <<" Script";
            WriteCommentNode(parentn, sstr.str() );
            WriteCommentNode(parentn, "++++++++++++++++++++++" );
            SSBXMLWriter(seq,  m_version, m_region)(parentn);
        }

        inline void WriteSSDataContent( xml_node & parentn, const ScriptData & dat )
        {
            using namespace scriptXML;
            WriteCommentNode(parentn, "======================" );
            stringstream sstr;
            sstr <<std::right <<std::setw(10) <<setfill(' ') <<dat.Name() <<" Data";
            WriteCommentNode(parentn, sstr.str() );
            WriteCommentNode(parentn, "======================" );
//#ifndef _DEBUG
//            assert(false);
//#endif
        }



    private:
        const LevelScript & m_scrset;
        eGameRegion       m_region;
        eGameVersion      m_version;
    };

//==============================================================================
//  GameScripts
//==============================================================================

    /*
        RunLevelXMLImport
            Helper for importing script data from XML.
            Is used in packaged tasks to be handled by the thread pool.
    */
    bool RunLevelXMLImport( const ScrSetLoader & ldr, string fname, eGameRegion reg, eGameVersion ver, atomic<uint32_t> & completed )
    {
#ifdef _DEBUG
if( fname == "ExplorersOfSky_Stats\\scripts/D00P01.xml")
    cout<<"lol";
#else
        assert(false);
#endif

        eGameRegion  tempregion  = eGameRegion::Invalid;
        eGameVersion tempversion = eGameVersion::Invalid;
        ldr( std::move( GameScriptsXMLParser(tempregion,tempversion).Parse(fname) ) );
        if( tempregion != reg || tempversion != ver )
            throw std::runtime_error("GameScripts::ImportXML(): Event " + fname + " from the wrong region or game version was loaded!! Ensure the version and region attributes are set properly!!");
        ++completed;
        return true;
    }

    /*
        RunLevelXMLExport
            Helper for exporting script data as XML.
            Is used in packaged tasks to be handled by the thread pool.
    */
    bool RunLevelXMLExport( const ScrSetLoader & entry, const string & dir, eGameRegion reg, eGameVersion ver, bool autoescape, atomic<uint32_t> & completed )
    {
        GameScriptsXMLWriter(entry(), reg, ver).Write(dir, autoescape);
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
    void ImportXMLGameScripts(const std::string & dir, GameScripts & out_dest, bool bprintprogress )
    {
        if(out_dest.m_setsindex.empty())
            throw std::runtime_error("ImportXMLGameScripts(): No script data to load to!!");

        decltype(out_dest.Region())   tempregion;
        decltype(out_dest.Version()) tempversion;
        atomic_bool                  shouldUpdtProgress = true;
        //future<void>                 updtProgress;
        atomic<uint32_t>             completed = 0;
        thread                       updatethread;
        //Grab our version and region from the 
        if(bprintprogress)
            cout<<"- Parsing COMON.xml..\n";

        stringstream commonfilename;
        commonfilename <<utils::TryAppendSlash(dir) <<DirNameScriptCommon <<".xml";
        out_dest.m_common = std::move( GameScriptsXMLParser(tempregion, tempversion).Parse(commonfilename.str()) );

        if( tempregion != out_dest.m_scrRegion || tempversion != out_dest.m_gameVersion )
            throw std::runtime_error("GameScripts::ImportXML(): The COMMON event from the wrong region or game version was loaded!! Ensure the version and region attributes are set properly!!");

        if(bprintprogress)
        {
            cout<<"!- Detected game region \"" <<GetGameRegionNames(out_dest.m_scrRegion) 
                <<"\", and version \"" <<GetGameVersionName(out_dest.m_gameVersion)<<"!\n";
        }
        //Write out common
        out_dest.WriteScriptSet(out_dest.m_common);


        //Prepare import of everything else!
        multitask::CMultiTaskHandler taskhandler;
        for( const auto & entry : out_dest.m_setsindex )
        {
            stringstream currentfname;
            currentfname << utils::TryAppendSlash(dir) <<entry.first <<".xml";
            const string fname = currentfname.str();

            //If a xml file in the import directory matches one of the level in the target game, load it. Otherwise ignore it!
            if( utils::isFile(fname) )
            {
                taskhandler.AddTask( multitask::pktask_t( std::bind( RunLevelXMLImport, 
                                                                     std::cref(entry.second), 
                                                                     fname, 
                                                                     out_dest.m_scrRegion, 
                                                                     out_dest.m_gameVersion, 
                                                                     std::ref(completed) ) ) );
            }
        }

        try
        {
            if(bprintprogress)
            {
                assert(!out_dest.m_setsindex.empty());
                cout<<"\nImporting..\n";
                updatethread = std::thread( PrintProgressLoop, 
                                            std::ref(completed), 
                                            out_dest.m_setsindex.size(), 
                                            std::ref(shouldUpdtProgress) );
                updatethread.detach();
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
            //if( updtProgress.valid() )
            //    updtProgress.get();
            if(updatethread.joinable())
                updatethread.join();
            if(bprintprogress)
                cout<<"\r100%"; //Can't be bothered to make another drawing update
        }
        catch(...)
        {
            shouldUpdtProgress = false;
            //if( updtProgress.valid() )
            //    updtProgress.get();
            if(updatethread.joinable())
                updatethread.join();
            std::rethrow_exception( std::current_exception() );
        }


        if(bprintprogress)
            cout<<"\n";
    }

    /*
        ExportGameScriptsXML
            
    */
    void ExportGameScriptsXML(const std::string & dir, const GameScripts & gs, bool bautoescapexml, bool bprintprogress )
    {
        //Export COMMON first
        if(bprintprogress)
            cout<<"\t- Writing COMMOM.xml..";
        GameScriptsXMLWriter(gs.m_common, gs.m_scrRegion, gs.m_gameVersion ).Write(dir, bautoescapexml);

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
                                                                 gs.m_scrRegion, 
                                                                 gs.m_gameVersion, 
                                                                 bautoescapexml,
                                                                std::ref(completed) ) ) );
        }

        try
        {
            if(bprintprogress)
            {
                cout<<"\nExporting..\n";
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
            if(bprintprogress)
                cout<<"\r100%"; //Can't be bothered to make another drawing update
        }
        catch(...)
        {
            shouldUpdtProgress = false;
            if( updtProgress.valid() )
                updtProgress.get();
            std::rethrow_exception( std::current_exception() );
        }

        if(bprintprogress)
            cout<<"\n";
    }

    /*
    */
    void ScriptSetToXML( const LevelScript & set, eGameRegion gloc, eGameVersion gver, bool bautoescapexml, const std::string & destdir )
    {
        if( gver < eGameVersion::NBGameVers && gloc < eGameRegion::NBRegions )
            GameScriptsXMLWriter(set, gloc, gver).Write(destdir, bautoescapexml);
        else
            throw std::runtime_error("ScriptSetToXML() : Error, invalid version or region!");
    }

    /*
    */
    LevelScript XMLToScriptSet( const std::string & srcdir, eGameRegion & out_reg, eGameVersion & out_gver )
    {
        return std::move( GameScriptsXMLParser(out_reg, out_gver).Parse(srcdir) );
    }

};