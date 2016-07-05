#include "pmd2_scripts.hpp"
#include <string>
#include <cassert>
#include <deque>
#include <iostream>
#include <sstream>
#include <utils/utility.hpp>
#include <ppmdu/pmd2/pmd2_scripts_opcodes.hpp>
#include <utils/pugixml_utils.hpp>
#include <utils/library_wide.hpp>
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
        const string ROOT_ScripDir      = "Event";      
        const string ATTR_GVersion      = "gameversion"; 
        const string ATTR_GRegion       = "gameregion";
        const string ATTR_DirName       = "eventname";           //Event name/filename with no extension
        const string ATTR_Name          = "name";

        const string NODE_LSDTbl        = "LSDTable";
        const string NODE_GrpNameRef    = "Ref";            //AKA sub-acting 
        const string ATTR_GrpName       = ATTR_Name;        //AKA sub-acting names

        const string NODE_ScriptGroup   = "ScriptGroup";
        const string ATTR_ScrGrpName    = ATTR_Name;
        
        const string NODE_ScriptData    = "ScriptData";
        const string ATTR_ScriptType    = "type";           // ssa, sse, sss
        const string ATTR_ScrDatName    = ATTR_Name;

        const string NODE_ScriptSeq     = "ScriptSequence";
        const string ATTR_ScrSeqName    = ATTR_Name;

        const string NODE_Constants     = "Constants";
        const string NODE_Constant      = "Constant";

        const string NODE_Strings       = "Strings";
        const string NODE_String        = "String";
        const string ATTR_Value         = "value";
        const string ATTR_Language      = "language";

        const string NODE_Code          = "Code";

        //!#TODO: add various group nodes depending on the group type 

        const string NODE_Group         = "Group";
        const string ATTR_GroupType     = "type";
        const string ATTR_GroupParam2   = "unk2";


        const string NODE_Instruction   = "Instruction"s;
        const string NODE_Data          = "Data";               ///For data words

        const string ATTR_Param         = "param";

        const array<string, 6> ATTR_Params =
        {
            "param1",
            "param2",
            "param3",
            "param4",
            "param5",
            "param6",

        };

    };

//==============================================================================
//  GameScriptsXMLParser
//==============================================================================

    /*****************************************************************************************
        SSBParser
            
    *****************************************************************************************/
    class SSBParser
    {
    public:
        SSBParser( eGameVersion version, eGameRegion region )
            :m_version(version), m_region(region)
        {}

        ScriptedSequence operator()(xml_node & seqn)
        {
            assert(false);
            return ScriptedSequence();
        }

    private:

    private:
        eGameVersion m_version;
        eGameRegion m_region;
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
            assert(false);
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
            xml_attribute xeventname = parentn.attribute(ATTR_DirName.c_str());

            m_out_gver = StrToGameVersion(xversion.value());
            m_out_reg  = StrToGameRegion(xregion.value());

            return ScriptSet( xeventname.value(), 
                             std::forward<ScriptSet::scriptgrps_t>(ParseGroups(parentn)),
                             std::forward<ScriptSet::lsdtbl_t>    (ParseLSD   (parentn)));
        }

    private:

        ScriptSet::scriptgrps_t ParseGroups( xml_node & parentn )
        {
            using namespace scriptXML;
            ScriptSet::scriptgrps_t groups;
            xml_node xgrp = parentn.child(NODE_ScriptGroup.c_str());

            for( auto & grp : xgrp )
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
                    if( grp.name() == NODE_ScriptSeq )
                        HandleSequence( comp, agroup );
                    else if( grp.name() == NODE_ScriptData )
                        HandleData(comp, agroup);
                }
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
                    std::copy_n(xnameref.value(), val.size(), val.begin());
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
                                         std::forward<ScriptedSequence>( SSBParser(m_out_gver, m_out_reg)(seqn) ) );
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
            AppendAttribute( AppendChildNode(parentn, NODE_Instruction), ATTR_Value, intr.opcode );
        }

        //template<eGameVersion _VERS>
            void WriteInstruction( xml_node & parentn, const pmd2::ScriptInstruction & intr )
        {
            //typedef typename std::conditional<_VERS == eGameVersion::EoS, 
            //                                  OpCodeFinderPicker<eOpCodeVersion::EoS>,
            //                                  typename std::conditional<_VERS == eGameVersion::EoT || _VERS == eGameVersion::EoD,
            //                                                            OpCodeFinderPicker<eOpCodeVersion::EoTD>,
            //                                                            OpCodeFinderPicker<eOpCodeVersion::Invalid>,
            //                                                           >::type
            //                                 >::type 
            //                 OpPicker_t;    //This determines at compile time how to get the opcode info
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
                    //if( cntparam > ATTR_Params.size() )
                      //  assert(false); //#REMOVEME: Just for testing something

                    //#TODO: we could probably make this a bit more sophisticated later on 
                    stringstream sstrpname;
                    sstrpname <<ATTR_Param <<cntparam;
                    AppendAttribute( xinstr, sstrpname.str(), intr.parameters[cntparam] );
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
            xml_node xgroup = AppendChildNode( parentn, NODE_ScriptGroup );
            WriteCommentNode(parentn, grp.Identifier() );

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
            SSBXMLWriter(seq,  m_version, m_region)(parentn);
        }

        inline void WriteSSDataContent( xml_node & parentn, const ScriptEntityData & dat )
        {
            using namespace scriptXML;
#ifndef _DEBUG
            assert(false);
#endif
        }



    private:
        const ScriptSet & m_scrset;
        eGameRegion       m_region;
        eGameVersion      m_version;
    };

//==============================================================================
//  GameScripts
//==============================================================================

    void ImportXMLGameScripts(const std::string & dir, GameScripts & out_dest)
    {
        decltype(out_dest.Region())   tempregion;
        decltype(out_dest.Version()) tempversion;

        //Grab our version and region from the 
        out_dest.m_common = std::move( GameScriptsXMLParser(tempregion, tempversion).Parse(dir) );

        if( tempregion != out_dest.m_scrRegion || tempversion != out_dest.m_gameVersion )
            throw std::runtime_error("GameScripts::ImportXML(): The COMMON event from the wrong region or game version was loaded!! Ensure the version and region attributes are set properly!!");

        //
        for( const auto entry : out_dest.m_setsindex )
        {
            entry.second( std::move( GameScriptsXMLParser(tempregion, tempversion).Parse(dir) ) );

            if( tempregion != out_dest.m_scrRegion || tempversion != out_dest.m_gameVersion )
                throw std::runtime_error("GameScripts::ImportXML(): Event " + entry.first + " from the wrong region or game version was loaded!! Ensure the version and region attributes are set properly!!");
        }
    }

    void ExportGameScriptsXML(const std::string & dir, const GameScripts & gs )
    {
        //Export COMMON first
        GameScriptsXMLWriter(gs.m_common, gs.m_scrRegion, gs.m_gameVersion ).Write(dir);

        //Export everything else
        for( const auto & entry : gs.m_setsindex )
            GameScriptsXMLWriter( entry.second(), gs.m_scrRegion, gs.m_gameVersion ).Write(dir);
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