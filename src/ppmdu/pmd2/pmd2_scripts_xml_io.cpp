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
using namespace std;
using namespace pugi;
using namespace pugixmlutils;

namespace pmd2
{
//==============================================================================
//  Constants
//==============================================================================
    namespace scriptXML
    {
        const string ROOT_ScripDir      = "Event"s;      
        const string ATTR_GVersion      = "gameversion"s; 
        const string ATTR_GRegion       = "gameregion"s;
        const string ATTR_DirName       = "eventname"s;           //Event name/filename with no extension
        const string ATTR_Name          = "name"s;

        const string NODE_LSDTbl        = "LSDTable"s;
        const string NODE_GrpNameRef    = "Ref"s;            //AKA sub-acting 
        const string ATTR_GrpName       = ATTR_Name;        //AKA sub-acting names

        const string NODE_ScriptGroup   = "ScriptGroup"s;
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

        const string NODE_Group         = "Group"s;
        const string ATTR_GroupType     = "type"s;
        const string ATTR_GroupParam2   = "unk2"s;


        const string NODE_Instruction   = "Instruction"s;
        const string NODE_Data          = "Data"s;               ///For data words

        const string ATTR_Param         = "param"s;


        //META
        const string NODE_MetaLabel     = "Label";
        const string ATTR_LblID         = "id";


        //Parameters

        const array<string, 9> ATTR_Params =
        {
            "param1",
            "param2",
            "param3",
            "param4",
            "param5",
            "param6",
            "param7",
            "param8",
            "param9",
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
            :m_version(version), m_region(region)
        {}

        ScriptedSequence operator()( const xml_node & seqn)
        {
            using namespace scriptXML;
            xml_attribute xname = seqn.attribute(ATTR_ScrSeqName.c_str());

            if(!xname)
            {
                throw std::runtime_error("SSBXMLParser::operator(): Sequence is missing its \"name\" attribute!!");
            }
            m_out = std::move( ScriptedSequence(xname.value()) );
            xml_node xcode  = seqn.child(NODE_Code.c_str());
            xml_node xconst = seqn.child(NODE_Constants.c_str());

            ParseCode(xcode);

            CheckLabelReferences();

            //ParseConsts(xconst);
            for( const auto & strblk : seqn.children(NODE_Strings.c_str()) )
                ParseStringBlock(strblk);

            return std::move(m_out);
        }

    private:

        void CheckLabelReferences()
        {
            for( const auto & aconstref : m_labelchecker )
            {
                if( !aconstref.second.bexists )
                {
                    stringstream sstr;
                    sstr <<"SSBXMLParser::CheckLabelReferences(): Reference to non-existant label ID " <<aconstref.first <<" found!!";
#ifdef  _DEBUG
                    assert(false);
#endif //  _DEBUG

                    throw std::runtime_error(sstr.str());
                }
            }
        }


        void ParseCode( const xml_node & coden )
        {
            using namespace scriptXML;
            ScriptedSequence::grptbl_t & outtbl = m_out.Groups();

            for( const xml_node & group : coden.children(NODE_Group.c_str()) )
            {
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
                grpout.type = xtype.as_uint();
                grpout.unk2 = xunk2.as_uint();

                for( const xml_node & inst : group )
                {
                    ParseInstruction(inst,grpout);
                }
                outtbl.push_back(std::move(grpout));
            }
        }

        void ParseInstruction( const xml_node & instn, ScriptInstrGrp & outgrp )
        {
            using namespace scriptXML;
            
            if( instn.name() == NODE_Instruction )
            {
                ParseCommand(instn,outgrp);
            }
            else if( instn.name() == NODE_Data)
            {
                xml_attribute     xval = instn.attribute(ATTR_Value.c_str());
                ScriptInstruction outinstr;
                outinstr.type = eInstructionType::Data;
                outinstr.value = static_cast<uint16_t>(xval.as_uint());
                outgrp.instructions.push_back(std::move(outinstr));
            }
            else if( instn.name() == NODE_MetaLabel )
            {
                ParseMetaLabel(instn,outgrp);
            }
        }

        void ParseMetaLabel( const xml_node & instn, ScriptInstrGrp & outgrp )
        {
            using namespace scriptXML;
            xml_attribute xid = instn.attribute(ATTR_LblID.c_str());

            if( !xid )
                throw std::runtime_error("SSBXMLParser::ParseInstruction(): Label with invalid ID found!");

            uint16_t labelid = static_cast<uint16_t>(xid.as_int());
            m_labelchecker[labelid].bexists = true; //We know this label exists

            ScriptInstruction outinstr;
            outinstr.type  = eInstructionType::MetaLabel;
            outinstr.value = static_cast<uint16_t>(xid.as_uint());
            outgrp.instructions.push_back(std::move(outinstr));
        }

        template<class _OPCodeFinderT>
            auto GetOpcodeNum( const char * name, size_t nbparams )->typename _OPCodeFinderT::opcode_t
        {
            const _OPCodeFinderT                     finder;
            const typename _OPCodeFinderT::opcode_t  op = finder(name,nbparams);
            if( static_cast<uint16_t>(op) != InvalidOpCode )
                return op;
            else
                throw std::runtime_error("SSBXMLParser::GetOpcodeNum(): No matching opcode found!");
        }

        void ParseCommand(const xml_node & instn, ScriptInstrGrp & outgrp)
        {
            using namespace scriptXML;
            xml_attribute name = instn.attribute(ATTR_Name.c_str());
            if(!name)
            {
                stringstream sstrer;
                sstrer <<"SSBXMLParser::ParseInstruction(): Script \"" <<m_out.Name() <<"\", instruction group #" << m_out.Groups().size() 
                    <<", in group instruction #" <<outgrp.instructions.size() <<" doesn't have a \"" <<ATTR_Name <<"\" attribute!!";
                throw std::runtime_error(sstrer.str());
            }
            ScriptInstruction outinstr;
            outinstr.type = eInstructionType::Command;

            //!#TODO: Handle meta, and differently named parameters!!


            //#1 - Get parameters
            ParseCommandParameters(instn, outinstr);
            //for( const auto & param : instn.attributes() )
            //{
            //    if( param.name() == ATTR_Param )
            //        outinstr.parameters.push_back( static_cast<uint16_t>(param.as_int()) );
            //}

            //#2 - Read opcode
            if(m_version == eGameVersion::EoS)
                outinstr.value = static_cast<uint16_t>( GetOpcodeNum<OpCodeFinderPicker<eOpCodeVersion::EoS>>(name.value(),outinstr.parameters.size()) );
            else if( m_version == eGameVersion::EoT || m_version == eGameVersion::EoD )
                outinstr.value = static_cast<uint16_t>( GetOpcodeNum<OpCodeFinderPicker<eOpCodeVersion::EoTD>>(name.value(),outinstr.parameters.size()) );
            else
                assert(false); //This shouldn't happen at all!

            outgrp.instructions.push_back(std::move(outinstr));            
        }


        void ParseCommandParameters( const xml_node & instn, ScriptInstruction & outinst )
        {
            using namespace scriptXML;
            for( const auto & param : instn.attributes() )
            {
                eOpParamTypes pty = eOpParamTypes::Invalid;
                if( param.name() == ATTR_Param )
                    outinst.parameters.push_back( static_cast<uint16_t>(param.as_int()) );
                else if( (pty = FindOpParamTypesByName(param.name()) ) != eOpParamTypes::Invalid )
                {
                    ParseTypedCommandParameterAttribute( param, pty, outinst );
                }
                else
                    throw std::runtime_error("SSBXMLParser::ParseCommandParameters(): Invalid parameter found!");
            }
        }

        void ParseTypedCommandParameterAttribute( const xml_attribute & param, eOpParamTypes pty, ScriptInstruction & outinst )
        {
            switch(pty)
            {
                case eOpParamTypes::Constant:
                {
                    m_constqueue.push_back( param.value() );
                    break;
                }
                case eOpParamTypes::String:
                {
                    if( m_region == eGameRegion::Japan )
                        m_constqueue.push_back( param.value() ); //Japanese version just puts strings in the constant table
                    else
                        outinst.parameters.push_back( static_cast<uint16_t>( param.as_uint() ) );
                    break;
                }
                case eOpParamTypes::InstructionOffset:
                {
                    //Labels IDs
                    uint16_t lblid = static_cast<uint16_t>( param.as_uint() );
                    auto itf = m_labelchecker.find( lblid );
                    
                    if( itf != m_labelchecker.end() )
                        itf->second.nbref += 1;
                    else
                        m_labelchecker.emplace( std::make_pair( lblid, labelRefInf{ false, 1 } ) ); //We found a ref to this label, but didn't check it

                    outinst.parameters.push_back( lblid );
                    break;
                }
                default:
                {
                    outinst.parameters.push_back( static_cast<uint16_t>( param.as_uint() ) );
                }
            }
        }

        void ParseConsts( const xml_node & constn )
        {
            using namespace scriptXML;
            m_out.ConstTbl() = std::move( ScriptedSequence::consttbl_t( m_constqueue.begin(), m_constqueue.end() ) );

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
            //m_out.ConstTbl() = std::move( ScriptedSequence::consttbl_t( constantsout.begin(), constantsout.end() ) );
        }


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
            m_out.InsertStrLanguage( glang, std::move(ScriptedSequence::strtbl_t( langstr.begin(), langstr.end() )) );
        }

    private:
        struct labelRefInf
        {
            bool    bexists;
            size_t  nbref  ;
        };


        unordered_map<uint16_t, labelRefInf> m_labelchecker;
        deque<string>                        m_constqueue;
        
        ScriptedSequence m_out;
        eGameVersion     m_version;
        eGameRegion      m_region;
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

        ScriptEntityData operator()(xml_node & seqn)
        {
            return ScriptEntityData();
        }

    private:

    private:
        eGameVersion m_version;
        eGameRegion m_region;
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

        ScriptSet Parse( const std::string & file )
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

            return std::move( ScriptSet( utils::GetBaseNameOnly(file), 
                             std::move(ParseGroups(parentn)),
                             std::move(ParseLSD   (parentn))));
        }

    private:

        ScriptSet::scriptgrps_t ParseGroups( xml_node & parentn )
        {
            using namespace scriptXML;
            ScriptSet::scriptgrps_t groups;

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

                ScriptSet::scriptgrps_t::value_type agroup( xname.value() );
                for( auto & comp : grp )
                {
                    if( comp.name() == NODE_ScriptSeq )
                        HandleSequence( comp, agroup );
                    else if( comp.name() == NODE_ScriptData )
                        HandleData(comp, agroup);
                }
                groups.push_back(std::forward<ScriptSet::scriptgrps_t::value_type>(agroup));
            }

            return std::move(groups);
        }

        ScriptSet::lsdtbl_t ParseLSD( xml_node & parentn )
        {
            using namespace scriptXML;
            ScriptSet::lsdtbl_t table;
            xml_node xlsd = parentn.child(NODE_LSDTbl.c_str());

            for( auto & lsde : xlsd.children(NODE_GrpNameRef.c_str()) )
            {
                xml_attribute xnameref = lsde.attribute(ATTR_GrpName.c_str());
                if(xnameref)
                {
                    ScriptSet::lsdtbl_t::value_type val;
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
        void HandleSequence(xml_node & seqn, ScriptSet::scriptgrps_t::value_type & destgrp)
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
                destgrp.Type(eScriptGroupType::UNK_unionall);

            //If we're unionall.ssb, change the type accordingly
            destgrp.Sequences().emplace( std::forward<string>(name), 
                                         std::forward<ScriptedSequence>( SSBXMLParser(m_out_gver, m_out_reg)(seqn) ) );
        }

        /*
            Return a type based on the kind of data it is
        */
        void HandleData(xml_node & datan, ScriptSet::scriptgrps_t::value_type & destgrp)
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
                    destgrp.Type(eScriptGroupType::UNK_sub);
                    break;
                }
                case eScrDataTy::SSE:
                {
                    destgrp.Type(eScriptGroupType::UNK_enter);
                    break;
                }
                case eScrDataTy::SSA:
                {
                    destgrp.Type(eScriptGroupType::UNK_fromlsd);
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
        SSBXMLWriter( const ScriptedSequence & seq, eGameVersion version, eGameRegion region )
            :m_seq(seq), m_version(version), m_region(region),m_opinfo( (version == eGameVersion::EoS)? eOpCodeVersion::EoS : eOpCodeVersion::EoTD )
        {}
        
        void operator()( xml_node & parentn )
        {
            using namespace scriptXML;
            //WriteCommentNode( parentn, "" );
            xml_node ssbnode = parentn.append_child( NODE_ScriptSeq.c_str() );
            AppendAttribute( ssbnode, ATTR_Name, m_seq.Name() );

            WriteCode     (ssbnode);
            WriteConstants(ssbnode);
            WriteStrings  (ssbnode);
        }

    private:

        void WriteCode( xml_node & parentn )
        {
            using namespace scriptXML;
            xml_node xcode = parentn.append_child( NODE_Code.c_str() );

            for( const auto & grp : m_seq.Groups() )
            {
                xml_node xgroup = xcode.append_child( NODE_Group.c_str() );
                AppendAttribute( xgroup, ATTR_GroupType,   grp.type );
                AppendAttribute( xgroup, ATTR_GroupParam2, grp.unk2 );

                for( const auto & instr : grp.instructions )
                    HandleInstruction(xgroup, instr);
            }
        }

        inline void HandleInstruction( xml_node & groupn, const pmd2::ScriptInstruction & intr )
        {
            switch(intr.type)
            {
                case eInstructionType::Data:
                {
                    WriteData(groupn,intr);
                    break;
                }
                case eInstructionType::Command:
                {
                    WriteInstruction(groupn,intr);
                    break;
                }
                case eInstructionType::MetaLabel:
                {
                    WriteMetaLabel(groupn,intr);
                    break;
                }
                case eInstructionType::MetaSwitch:
                {
                    WriteMetaSwitch(groupn,intr);
                    break;
                }
                case eInstructionType::MetaAccessor:
                {
                    WriteMetaAccessor(groupn,intr);
                    break;
                }
                case eInstructionType::MetaProcSpecRet:
                {
                    WriteMetaSpecRet(groupn,intr);
                    break;
                }
                default:
                {
                    clog<<"SSBXMLWriter::HandleInstruction(): Encountered invalid instruction type!! Skipping!\n";
                }
            };                
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
            OpCodeInfoWrapper curinf  = m_opinfo(intr.value);
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
                OpCodeInfoWrapper curinf = m_opinfo(subinst.value);
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

        inline void WriteData(xml_node & groupn, const pmd2::ScriptInstruction & instr)
        {
            using namespace scriptXML;
            xml_node        datan = AppendChildNode(groupn, NODE_Data);
            xml_attribute   dval  = datan.append_attribute(ATTR_Value.c_str());
            stringstream    sstr;
            sstr <<"0x" <<hex <<uppercase <<instr.value;
            string str = sstr.str();    //Just in case the string gets deleted out of scope when we pass the c_str..
            dval.set_value( str.c_str() );
        }

        void WriteInstruction(xml_node & groupn, const pmd2::ScriptInstruction & instr)
        {
            using namespace scriptXML;
            xml_node          xinstr = groupn.append_child( NODE_Instruction.c_str() );
            OpCodeInfoWrapper opinfo = m_opinfo(instr.value);

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
            if( (opinfo.ParamInfo().size() > cntparam) && ( (pname = OpParamTypesToStr( opinfo.ParamInfo()[cntparam].ptype )) != nullptr) )
            {
                eOpParamTypes ptype      = opinfo.ParamInfo()[cntparam].ptype;
                size_t        ptypeindex = static_cast<size_t>(ptype);
                int16_t       pval       = intr.parameters[cntparam];
                stringstream  deststr;
                deststr << *pname;

                if(cntparam > 0)
                    deststr <<cntparam;

                switch(ptype)
                {
                    case eOpParamTypes::Constant:
                    {
                        if( isConstIdInRange(pval) )
                        {
                            AppendAttribute( instn, deststr.str(), m_seq.ConstTbl()[pval] );
                            return;
                        }
                        else
                        {
                            clog << "SSBXMLWriter::WriteInstructionParam(): Constant ID " <<pval <<" out of range!";
                            //throw std::runtime_error( "SSBXMLWriter::WriteInstructionParam(): Constant ID out of range!" );
                        }
                        break;
                    }
                    case eOpParamTypes::String:
                    {
                        if( m_region == eGameRegion::Japan && isConstIdInRange(pval) ) //Japan ignores string block completely
                        {
                            AppendAttribute( instn, OpParamTypesNames[static_cast<size_t>(eOpParamTypes::Constant)], m_seq.ConstTbl()[pval] );
                            return;
                        }
                        else if( isStringIdInRange(pval) )
                        {
                            //We place a string ID, not the string itself, because multiples refs and multi-lingual issues
                            AppendAttribute( instn, deststr.str(), pval );
                            for( const auto & lang : m_seq.StrTblSet() )
                            {
                                xml_node xlang = AppendChildNode(instn, NODE_String);
                                AppendAttribute( xlang, ATTR_Language, GetGameLangName(lang.first) );
                                AppendAttribute( xlang, ATTR_Value,    lang.second.at(pval) );
                            }
                            return;
                        }
                        else
                        {
                            clog << "SSBXMLWriter::WriteInstructionParam(): String ID " <<pval <<" out of range!";
                            //throw std::runtime_error( "SSBXMLWriter::WriteInstructionParam(): String ID out of range!" );
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
                        //!#TODO:
                    }
                    case eOpParamTypes::Unk_LivesRef:
                    {
                        //!#TODO:
                    }
                    default:
                    {
                        AppendAttribute( instn, deststr.str(), pval );
                        return;
                    }
                };

            }
            //!#TODO: we could probably make this a bit more sophisticated later on 
            if( cntparam < ATTR_Params.size() )
            {
                AppendAttribute( instn, ATTR_Params[cntparam], static_cast<int16_t>(intr.parameters[cntparam]) );
            }
            else
            {
                stringstream  deststr;
                deststr<<ATTR_Param <<cntparam;
                AppendAttribute( instn, deststr.str(), static_cast<int16_t>(intr.parameters[cntparam]) );
            }
        }

        inline bool isConstIdInRange( uint16_t id )const
        {
            return id < m_seq.ConstTbl().size();
        }

        inline bool isStringIdInRange( uint16_t id )const
        {
            return !(m_seq.StrTblSet().empty()) && (id < m_seq.StrTblSet().begin()->second.size());
        }

        void WriteConstants( xml_node & parentn )
        {
            using namespace scriptXML;
            if( m_seq.ConstTbl().empty() )
                return;
            xml_node xconsts = parentn.append_child( NODE_Constants.c_str() );

#ifdef _DEBUG
            size_t cntc = 0;
#endif
            for( const auto & aconst : m_seq.ConstTbl() )
            {
#ifdef _DEBUG
                stringstream sstr;
                sstr<<"Const #" <<cntc;
                WriteCommentNode( xconsts, sstr.str() );
                ++cntc;
#endif
                xml_node xcst = xconsts.append_child( NODE_Constant.c_str() );
                AppendAttribute( xcst, ATTR_Value, aconst );
            }
        }

        void WriteStrings( xml_node & parentn )
        {
            using namespace scriptXML;
            if( m_seq.StrTblSet().empty() )
                return;
            xml_node xstrings = parentn.append_child( NODE_Strings.c_str() );

            for( const auto & alang : m_seq.StrTblSet() )
            {
                AppendAttribute( xstrings, ATTR_Language, GetGameLangName(alang.first) );
#ifdef _DEBUG
                size_t cnts = 0;
#endif
                for( const auto & astring : alang.second )
                {
#ifdef _DEBUG
                    stringstream sstr;
                    sstr<<"String #" <<cnts;
                    WriteCommentNode( xstrings, sstr.str() );
                    ++cnts;
#endif
                    AppendAttribute( xstrings.append_child( NODE_String.c_str() ), ATTR_Value, astring );
                }
            }
        }

    private:
        const ScriptedSequence & m_seq;
        eGameVersion             m_version;
        eGameRegion              m_region;
        OpCodeClassifier         m_opinfo;
    };

    /*****************************************************************************************
        GameScriptsXMLWriter
            Write XML for the content of a single script directory.
    *****************************************************************************************/
    class GameScriptsXMLWriter
    {
    public:
        GameScriptsXMLWriter( const ScriptSet & set, eGameRegion greg, eGameVersion gver )
            :m_scrset(set), m_region(greg), m_version(gver)
        {}

        void Write(const std::string & destdir)
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

            //Write doc
            if( ! doc.save_file( sstrfname.str().c_str() ) )
                throw std::runtime_error("GameScriptsXMLWriter::Write(): Can't write xml file " + sstrfname.str());
        }

    private:

        void WriteGrp( xml_node & parentn, const ScriptGroup & grp )
        {
            using namespace scriptXML;
            WriteCommentNode(parentn, grp.Identifier() );
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

        inline void WriteSSBContent( xml_node & parentn, const ScriptedSequence & seq )
        {
            using namespace scriptXML;
            WriteCommentNode(parentn, seq.Name() );
            SSBXMLWriter(seq,  m_version, m_region)(parentn);
        }

        inline void WriteSSDataContent( xml_node & parentn, const ScriptEntityData & dat )
        {
            using namespace scriptXML;
            WriteCommentNode(parentn, dat.Name() );
//#ifndef _DEBUG
//            assert(false);
//#endif
        }



    private:
        const ScriptSet & m_scrset;
        eGameRegion       m_region;
        eGameVersion      m_version;
    };

//==============================================================================
//  GameScripts
//==============================================================================

    bool RunImport( const ScrSetLoader & ldr, string fname, eGameRegion reg, eGameVersion ver, atomic<uint32_t> & completed )
    {
        eGameRegion  tempregion  = eGameRegion::Invalid;
        eGameVersion tempversion = eGameVersion::Invalid;
        ldr( std::move( GameScriptsXMLParser(tempregion,tempversion).Parse(fname) ) );
        if( tempregion != reg || tempversion != ver )
            throw std::runtime_error("GameScripts::ImportXML(): Event " + fname + " from the wrong region or game version was loaded!! Ensure the version and region attributes are set properly!!");
        ++completed;
        return true;
    }

    bool RunExport( const ScrSetLoader & entry, const string & dir, eGameRegion reg, eGameVersion ver, atomic<uint32_t> & completed )
    {
        GameScriptsXMLWriter(entry(), reg, ver).Write(dir);
        ++completed;
        return true;
    }

    void PrintProgressLoop( atomic<uint32_t> & completed, uint32_t total, atomic<bool> & bDoUpdate )
    {
        static const chrono::milliseconds ProgressUpdThWait = chrono::milliseconds(100);
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

    void ImportXMLGameScripts(const std::string & dir, GameScripts & out_dest, bool bprintprogress )
    {
        decltype(out_dest.Region())   tempregion;
        decltype(out_dest.Version()) tempversion;
        atomic_bool                  shouldUpdtProgress = true;
        future<void>                 updtProgress;
        atomic<uint32_t>             completed = 0;
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


        multitask::CMultiTaskHandler taskhandler;
        //
        for( const auto & entry : out_dest.m_setsindex )
        {
            stringstream currentfname;
            currentfname << utils::TryAppendSlash(dir) <<entry.first <<".xml";

            taskhandler.AddTask( multitask::pktask_t( std::bind( RunImport, std::cref(entry.second), currentfname.str(), out_dest.m_scrRegion, out_dest.m_gameVersion, std::ref(completed) ) ) );

            /*taskhandler.AddTask( multitask::pktask_t(
                                 std::bind( RunImport, std::addressof(entry.second), 
                                               currentfname.str(), 
                                                                eGameRegion(out_dest.m_scrRegion), 
                                                                eGameVersion(out_dest.m_gameVersion) ) )  );*/

            //if(bprintprogress)
            //    cout<<"\r\t- Parsing " <<setw(14) <<setfill(' ') <<right <<entry.first + ".xml..";

            //entry.second( std::move( GameScriptsXMLParser(tempregion, tempversion).Parse(currentfname.str()) ) );

            //if( tempregion != out_dest.m_scrRegion || tempversion != out_dest.m_gameVersion )
            //    throw std::runtime_error("GameScripts::ImportXML(): Event " + entry.first + " from the wrong region or game version was loaded!! Ensure the version and region attributes are set properly!!");
        }

        try
        {
            cout<<"\nImporting..\n";
            updtProgress = std::async( std::launch::async, PrintProgressLoop, std::ref(completed), out_dest.m_setsindex.size(), std::ref(shouldUpdtProgress) );
            taskhandler.Execute();
            taskhandler.BlockUntilTaskQueueEmpty();
            taskhandler.StopExecute();

            shouldUpdtProgress = false;
            if( updtProgress.valid() )
                updtProgress.get();
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

    void ExportGameScriptsXML(const std::string & dir, const GameScripts & gs, bool bprintprogress )
    {
        //Export COMMON first
        if(bprintprogress)
            cout<<"\t- Writing COMMOM.xml..";
        GameScriptsXMLWriter(gs.m_common, gs.m_scrRegion, gs.m_gameVersion ).Write(dir);

        atomic_bool                  shouldUpdtProgress = true;
        future<void>                 updtProgress;
        atomic<uint32_t>             completed = 0;
        multitask::CMultiTaskHandler taskhandler;
        //Export everything else
        for( const auto & entry : gs.m_setsindex )
        {
            //if(bprintprogress)
            //    cout<<"\r\t- Writing " <<setw(14) <<setfill(' ') <<right <<entry.first + ".xml..";
            //GameScriptsXMLWriter( entry.second(), gs.m_scrRegion, gs.m_gameVersion ).Write(dir);
            taskhandler.AddTask( multitask::pktask_t( std::bind( RunExport, std::cref(entry.second), std::cref(dir), gs.m_scrRegion, gs.m_gameVersion, std::ref(completed) ) ) );
        }

        try
        {
            cout<<"\nExporting..\n";
            updtProgress = std::async( std::launch::async, PrintProgressLoop, std::ref(completed), gs.m_setsindex.size(), std::ref(shouldUpdtProgress) );
            taskhandler.Execute();
            taskhandler.BlockUntilTaskQueueEmpty();
            taskhandler.StopExecute();

            shouldUpdtProgress = false;
            if( updtProgress.valid() )
                updtProgress.get();
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

    void ScriptSetToXML( const ScriptSet & set, eGameRegion gloc, eGameVersion gver, const std::string & destdir )
    {
        if( gver < eGameVersion::NBGameVers && gloc < eGameRegion::NBRegions )
            GameScriptsXMLWriter(set, gloc, gver).Write(destdir);
        else
            throw std::runtime_error("ScriptSetToXML() : Error, invalid version or region!");
    }

    ScriptSet XMLToScriptSet( const std::string & srcdir, eGameRegion & out_reg, eGameVersion & out_gver )
    {
        return std::move( GameScriptsXMLParser(out_reg, out_gver).Parse(srcdir) );
    }

};