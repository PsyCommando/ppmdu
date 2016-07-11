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
            ParseConsts(xconst);
            for( const auto & strblk : seqn.children(NODE_Strings.c_str()) )
                ParseStringBlock(strblk);
            return std::move(m_out);
        }

    private:
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
                ParseOpCode(instn,outgrp);
            }
            else if( instn.name() == NODE_Data)
            {
                xml_attribute     xval = instn.attribute(ATTR_Value.c_str());
                ScriptInstruction outinstr;
                outinstr.isdata = true;
                outinstr.opcode = static_cast<uint16_t>(xval.as_uint());
                outgrp.instructions.push_back(std::move(outinstr));
            }
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

        void ParseOpCode(const xml_node & instn, ScriptInstrGrp & outgrp)
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
            outinstr.isdata = false;

            //#1 - Get parameters
            for( const auto & param : instn.attributes() )
            {
                if( param.name() == ATTR_Param )
                    outinstr.parameters.push_back( static_cast<uint16_t>(param.as_uint()) );
            }

            
            //#2 - Read opcode
            if(m_version == eGameVersion::EoS)
            {
                outinstr.opcode = static_cast<uint16_t>( GetOpcodeNum<OpCodeFinderPicker<eOpCodeVersion::EoS>>(name.value(),outinstr.parameters.size()) );
                //const OpCodeFinderPicker<eOpCodeVersion::EoS> finder;
                //const eScriptOpCodesEoS                       op = finder(name.value());
                //if( op != eScriptOpCodesEoS::INVALID )
                //{
                //    outinstr.opcode = static_cast<uint16_t>(op);
                //    const OpCodeInfoEoS * popcode = finder(op);
                //    assert(popcode); //This shouldn't happen at all, since we validated that opcode already!
                //    nbparams = popcode->nbparams;
                //}
                //else
                //{
                //}
            }
            else if( m_version == eGameVersion::EoT || m_version == eGameVersion::EoD )
            {
                outinstr.opcode = static_cast<uint16_t>( GetOpcodeNum<OpCodeFinderPicker<eOpCodeVersion::EoTD>>(name.value(),outinstr.parameters.size()) );
                //const OpCodeFinderPicker<eOpCodeVersion::EoTD> finder;
                //const eScriptOpCodesEoTD                       op = finder(name.value());
                //if( op != eScriptOpCodesEoTD::INVALID )
                //{
                //    outinstr.opcode = static_cast<uint16_t>(op);
                //    const OpCodeInfoEoTD * popcode = finder(op);
                //    assert(popcode); //This shouldn't happen at all, since we validated that opcode already!
                //}
                //else
                //{
                //}
            }
            else
                assert(false); //This shouldn't happen at all!

            outgrp.instructions.push_back(std::move(outinstr));

            ////#2 - Read the parameters
            //if( nbparamsexpected > 0 )
            //{
            //    size_t cntparam = 0;
            //    outinstr.parameters.resize(nbparamsexpected, 0);
            //    for( const auto & attr : instn.attributes() )
            //    {
            //        string pname = attr.name();
            //        const static regex paramname( ATTR_Param + "(\\d)");
            //        smatch sm;

            //        if( regex_match(pname,sm,paramname) && sm.size() == 2 )
            //        {
            //            size_t paramno = 0;
            //            stringstream sstr;
            //            sstr << sm[1].str();
            //            sstr >> paramno;

            //            if( paramno < nbparamsexpected )
            //                outinstr.parameters[paramno] = static_cast<uint16_t>(attr.as_uint());
            //            else
            //                clog <<"<!>- SSBParser: Ignored extra parameter \"param" <<paramno <<"\", for instruction number " <<outgrp.size() <<" in group!\n";
            //        }
            //    }
            //}
            
        }

        void ParseConsts( const xml_node & constn )
        {
            using namespace scriptXML;
            deque<string> constantsout; //The deque avoids re-allocating a vector constantly
            for( const auto & conststr : constn.children(NODE_Constant.c_str()) )
            {
                xml_attribute xval = conststr.attribute(ATTR_Value.c_str());
                if(!xval)
                {
                    stringstream sstrer;
                    sstrer << "SSBXMLParser::ParseConsts(): Script \"" <<m_out.Name() <<"\", Constant #"  <<constantsout.size()
                           <<" is missing its \"" <<ATTR_Value <<"\" attribute!";
                    throw std::runtime_error( sstrer.str() );
                }
                constantsout.push_back(xval.value());
            }
            m_out.ConstTbl() = std::move( ScriptedSequence::consttbl_t( constantsout.begin(), constantsout.end() ) );
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
            :m_seq(seq), m_version(version), m_region(region)
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
                    WriteInstructionOrData(xgroup, instr);
            }
        }

        inline void WriteInstructionOrData(xml_node & parentn, const pmd2::ScriptInstruction & intr)
        {
            if( intr.isdata )
                WriteData(parentn,intr);
            else
                WriteInstruction(parentn,intr);
        }

        inline void WriteData(xml_node & parentn, const pmd2::ScriptInstruction & intr)
        {
            using namespace scriptXML;
            xml_node        datan = AppendChildNode(parentn, NODE_Data);
            xml_attribute   dval  = datan.append_attribute(ATTR_Value.c_str());
            stringstream    sstr;
            sstr <<"0x" <<hex <<uppercase <<intr.opcode;
            string str = sstr.str();    //Just in case the string gets deleted out of scope when we pass the c_str..
            dval.set_value( str.c_str() );
        }

        //template<eGameVersion _VERS>
            void WriteInstruction( xml_node & parentn, const pmd2::ScriptInstruction & intr )
        {
            using namespace scriptXML;
            xml_node        xinstr     = parentn.append_child( NODE_Instruction.c_str() );
            string          opcodename; 

            if( m_version == eGameVersion::EoS )
            {
                const OpCodeInfoEoS * popcode = OpCodeFinderPicker<eOpCodeVersion::EoS>()(intr.opcode);
                if( popcode )
                    opcodename = popcode->name;
            }
            else if( m_version == eGameVersion::EoT || m_version == eGameVersion::EoD )
            {
                const OpCodeInfoEoTD * popcode = OpCodeFinderPicker<eOpCodeVersion::EoTD>()(intr.opcode);
                if( popcode )
                    opcodename = popcode->name;
            }
            else
            { cout<<"<!>- SSBXMLWriter::WriteInstruction(): Bad version. This shouldn't happen here!\n"; assert(false);}


            if( !opcodename.empty() )
            {
                AppendAttribute( xinstr, ATTR_Name, opcodename );
                for( size_t cntparam= 0; cntparam < intr.parameters.size(); ++cntparam )
                {
                    //!#TODO: we could probably make this a bit more sophisticated later on 
                    AppendAttribute( xinstr, ATTR_Param, intr.parameters[cntparam] );
                }
            }
            else
            {
                cout<<"\n<!>- SSBXMLWriter::WriteInstruction(): A bad opcode was found in the scripted sequece!! This shouldn't happen here!\n";
                assert(false);
            }
        }

        void WriteConstants( xml_node & parentn )
        {
            using namespace scriptXML;
            if( m_seq.ConstTbl().empty() )
                return;
            xml_node xconsts = parentn.append_child( NODE_Constants.c_str() );

            for( const auto & aconst : m_seq.ConstTbl() )
            {
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
                for( const auto & astring : alang.second )
                    AppendAttribute( xstrings.append_child( NODE_String.c_str() ), ATTR_Value, astring );
            }
        }

    private:
        const ScriptedSequence & m_seq;
        eGameVersion             m_version;
        eGameRegion              m_region;
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
            cout<<"Importing..\n";
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
            cout<<"Exporting..\n";
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
        GameScriptsXMLWriter(set, gloc, gver).Write(destdir);
    }

    ScriptSet XMLToScriptSet( const std::string & srcdir, eGameRegion & out_reg, eGameVersion & out_gver )
    {
        return std::move( GameScriptsXMLParser(out_reg, out_gver).Parse(srcdir) );
    }

};