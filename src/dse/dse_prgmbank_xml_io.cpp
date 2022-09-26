#include "dse_conversion.hpp"
#include <pugixml.hpp>
#include <utils/parse_utils.hpp>
#include <utils/pugixml_utils.hpp>
#include <ext_fmts/adpcm.hpp>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <Poco/DirectoryIterator.h>
#include <Poco/File.h>
#include <Poco/Path.h>

using namespace pugi;
using namespace pugixmlutils;
using namespace std;

namespace DSE
{
//====================================================================================================
//  Constants
//====================================================================================================
    const string DEF_ProgramsFname = "programs.xml";
    const string DEF_WavInfoFname  = "samples.xml";
    const string DEF_KeygroupFname = "keygroups.xml";



    namespace PrgmXML
    {
        //Root nodes
        const string ROOT_Programs     = "Programs";
        const string ROOT_WavInfo      = "WavInfo";
        const string ROOT_KeyGroups    = "KeyGroups";

        //Common
        const string PROP_Volume       = "Volume";
        const string PROP_Pan          = "Pan";
        const string PROP_ID           = "ID";
        const string PROP_FTune        = "FineTune";
        const string PROP_CTune        = "CoarseTune";
        const string PROP_RootKey      = "RootKey";
        const string PROP_KeyTrans     = "KeyTransposition";

        //Nodes
        const string NODE_Program      = "Program";
        const string PROP_PrgmUnk3     = "Unk3";
        const string PROP_PrgmUnkPoly  = "UnkPoly";

        const string NODE_LFOTable     = "LFOTable";

        const string NODE_LFOEntry     = "LFOEntry";
        const string PROP_LFOEnabled   = "Enabled";
        const string PROP_LFOModDest   = "ModulationDestination";
        const string PROP_LFOWaveShape = "WaveformShape";
        const string PROP_LFORate      = "Rate";
        const string PROP_LFOUnk29     = "Unk29";
        const string PROP_LFODepth     = "Depth";
        const string PROP_LFODelay     = "Delay";
        const string PROP_LFOUnk32     = "Unk32";
        const string PROP_LFOUnk33     = "Unk33";

        const string NODE_SplitTable   = "SplitTable";

        const string NODE_Split        = "Split";
        const string PROP_SplitUnk11   = "PitchBendRange";
        const string PROP_SplitUnk25   = "Unk25";
        const string PROP_SplitLowKey  = "LowestKey";
        const string PROP_SplitHighKey = "HighestKey";
        const string PROP_SplitLowVel  = "LowestVelocity";
        const string PROP_SplitHighVel = "HighestVelocity";
        const string PROP_SplitSmplID  = "SampleID";
        const string PROP_SplitKGrp    = "KeyGroupID";
        const string PROP_SplitUnk22   = "Unk22";
        const string PROP_SplitUnk23   = "Unk23";
        
        const string NODE_KGrp         = "KeyGroup";
        const string NODE_KGrpPoly     = "Polyphony";
        const string NODE_KGrpPrio     = "Priority";
        const string NODE_KGrVcLow     = "LowestVoiceChannel";
        const string NODE_KGrVcHi      = "HighestVoiceChannel";


        const string NODE_Sample       = "Sample";
        const string PROP_SmplUnk5     = "Unk5";
        const string PROP_SmplUnk58    = "Unk58";
        const string PROP_SmplUnk6     = "Unk6";
        const string PROP_SmplUnk7     = "Unk7";
        const string PROP_SmplUnk59    = "Unk59";
        //const string PROP_SmplFmt = "SampleFormat";
        const string PROP_SmplUnk9     = "Unk9";
        const string PROP_SmplLoop     = "LoopOn";
        const string PROP_SmplUnk10    = "Unk10";
        //const string PROP_Smplsperdw = "NbSamplesPerDWord";
        const string PROP_SmplUnk11    = "Unk11";
        //const string PROP_BitsPerSmpl = "BitsPerSample";
        const string PROP_SmplUnk12    = "Unk12";
        const string PROP_SmplUnk62    = "Unk62";
        const string PROP_SmplUnk13    = "Unk13";
        //const string PROP_SmplRate = "SampleRate";
        const string PROP_LoopBeg      = "LoopBegin";
        const string PROP_LoopLen      = "LoopLen";

        const string NODE_Envelope     = "Envelope";
        const string PROP_EnvOn        = "EnableEnveloppe";
        const string PROP_EnvMulti     = "EnveloppeDurationMultiplier";
        const string PROP_EnvUnk19     = "Unk19";
        const string PROP_EnvUnk20     = "Unk20";
        const string PROP_EnvUnk21     = "Unk21";
        const string PROP_EnvUnk22     = "Unk22";
        const string PROP_EnvAtkVol    = "AttackVolume";
        const string PROP_EnvAtk       = "Attack";
        const string PROP_EnvHold      = "Hold";
        const string PROP_EnvDecay     = "Decay";
        const string PROP_EnvSustain   = "Sustain";
        const string PROP_EnvDecay2    = "FadeOut";
        const string PROP_EnvRelease   = "Release";
        const string PROP_EnvUnk57     = "Unk57";
    };

//====================================================================================================
//  ProgramBankXMLParser
//====================================================================================================
class ProgramBankXMLParser
{
public:
    ProgramBankXMLParser( const string & dirpath )
        :m_path(dirpath)
    {}

    PresetBank Parse()
    {
        vector< unique_ptr<ProgramInfo> > prgmbank;
        vector<KeyGroup>                  kgrp;
        vector<SampleBank::smpldata_t>    sampledata;

        ParsePrograms(prgmbank);
        ParseKeygroups(kgrp);
        ParseSampleInfos(sampledata);

        return move( PresetBank( move(DSE_MetaDataSWDL()), 
                                 move(unique_ptr<ProgramBank>(new ProgramBank(move(prgmbank), 
                                                                              move(kgrp)))),
                                 move(unique_ptr<SampleBank>(new SampleBank(move(sampledata)))) 
                                 ) );
    }

private:

    void ParsePrograms( vector< unique_ptr<ProgramInfo> > & prgmbank )
    {
        ParseAProgram();
    }

    void ParseAProgram()
    {
        ParseALFO();
        ParseASplit();
    }

    void ParseALFO()
    {
    }

    void ParseASplit()
    {
    }


    void ParseSampleInfos( vector<SampleBank::smpldata_t> & sampledata )
    {
        ParseASample();
    }

    void ParseASample()
    {
    }

    void ParseKeygroups( vector<KeyGroup> & kgrp )
    {
        ParseAKeygroup();
    }

    void ParseAKeygroup()
    {
    }

private:
    string m_path;

};

//====================================================================================================
//  ProgramBankXMLWriter
//====================================================================================================
class ProgramBankXMLWriter
{
public:
    ProgramBankXMLWriter( const PresetBank & presbnk )
        :m_presbnk(presbnk)
    {
    }

    void Write( const std::string & destdir )
    {
        WriteWavInfo(destdir);
        WritePrograms(destdir);
        WriteKeyGroups(destdir);
    }

private:

    void WritePrograms( const std::string & destdir )
    {
        using namespace PrgmXML;
        xml_document doc;
        xml_node     prgmsnode = doc.append_child( ROOT_Programs.c_str() );
        auto         ptrprgms  = m_presbnk.prgmbank().lock(); 


        if( ptrprgms != nullptr )
        {
            for( const auto & aprog : ptrprgms->PrgmInfo() )
            {
                if( aprog != nullptr )
                    WriteAProgram( prgmsnode, *aprog );
            }

            stringstream sstrfname;
            sstrfname << utils::TryAppendSlash(destdir) << DEF_ProgramsFname;
            if( ! doc.save_file( sstrfname.str().c_str() ) )
                throw std::runtime_error("Can't write xml file " + sstrfname.str());
        }
        else
            clog << "ProgramBankXMLWriter::WritePrograms(): no program data available!";
    }

    void WriteAProgram( xml_node & parent, const ProgramInfo & curprog )
    {
        using namespace PrgmXML;

        WriteCommentNode( parent, "ProgramID : " + std::to_string( curprog.id ) );
        xml_node prgnode = parent.append_child( NODE_Program.c_str() );

        //Write program header stuff
        WriteNodeWithValue( prgnode, PROP_ID,           curprog.id );
        WriteNodeWithValue( prgnode, PROP_Volume,       curprog.prgvol );
        WriteNodeWithValue( prgnode, PROP_Pan,          curprog.prgpan );
        //WriteNodeWithValue( prgnode, PROP_PrgmUnk3,     curprog.unk3 );
        WriteNodeWithValue( prgnode, PROP_PrgmUnkPoly,  curprog.unkpoly );

        WriteCommentNode( prgnode, "LFO Settings" );
        {
            xml_node lfotblnode = prgnode.append_child( NODE_LFOTable.c_str() );
            for( const auto & lfoentry : curprog.m_lfotbl )
                WriteALFO( lfotblnode, lfoentry );
        }

        WriteCommentNode( prgnode, "Splits" );
        {
            xml_node splittblnode = prgnode.append_child( NODE_SplitTable.c_str() );
            for( const auto & splitentry : curprog.m_splitstbl )
                WriteASplit( splittblnode, splitentry );
        }
        
    }

    void WriteALFO( xml_node & parent, const LFOTblEntry & curlfo )
    {
        using namespace PrgmXML;

        if( curlfo.isLFONonDefault() )
        {
            stringstream sstrunkcv;

            WriteNodeWithValue( parent, PROP_LFOEnabled,    curlfo.unk52  );
            WriteNodeWithValue( parent, PROP_LFOModDest,    curlfo.dest   );
            WriteNodeWithValue( parent, PROP_LFOWaveShape,  curlfo.wshape );
            WriteNodeWithValue( parent, PROP_LFORate,       curlfo.rate   );

            //unk29 is different
            sstrunkcv <<hex <<showbase <<curlfo.unk29;
            parent.append_child(PROP_LFOUnk29.c_str()).append_child(node_pcdata).set_value( sstrunkcv.str().c_str() );

            WriteNodeWithValue( parent, PROP_LFODepth,      curlfo.depth  );
            WriteNodeWithValue( parent, PROP_LFODelay,      curlfo.delay  );
            WriteNodeWithValue( parent, PROP_LFOUnk32,      curlfo.unk32  );
            WriteNodeWithValue( parent, PROP_LFOUnk33,      curlfo.unk33  );
        }
    }

    void WriteASplit( xml_node & parent, const SplitEntry & cursplit )
    {
        using namespace PrgmXML;
        WriteCommentNode( parent, "Split Sample " + to_string(cursplit.smplid) );

        WriteNodeWithValue( parent, PROP_ID,                cursplit.id );
        WriteNodeWithValue( parent, PROP_SplitUnk11,        cursplit.unk11 );
        WriteNodeWithValue( parent, PROP_SplitUnk25,        cursplit.unk25 );
        WriteNodeWithValue( parent, PROP_SplitLowKey,       cursplit.lowkey );
        WriteNodeWithValue( parent, PROP_SplitHighKey,      cursplit.hikey );
        WriteNodeWithValue( parent, PROP_SplitLowVel,       cursplit.lovel );
        WriteNodeWithValue( parent, PROP_SplitHighVel,      cursplit.hivel );
        WriteNodeWithValue( parent, PROP_SplitSmplID,       cursplit.smplid );
        WriteNodeWithValue( parent, PROP_FTune,             cursplit.ftune );
        WriteNodeWithValue( parent, PROP_CTune,             cursplit.ctune );
        WriteNodeWithValue( parent, PROP_RootKey,           cursplit.rootkey );
        WriteNodeWithValue( parent, PROP_KeyTrans,          cursplit.ktps );
        WriteNodeWithValue( parent, PROP_Volume,            cursplit.smplvol );
        WriteNodeWithValue( parent, PROP_Pan,               cursplit.smplpan );
        WriteNodeWithValue( parent, PROP_SplitKGrp,         cursplit.kgrpid );
        //WriteNodeWithValue( parent, PROP_SplitUnk22,        cursplit.unk22 );
        //WriteNodeWithValue( parent, PROP_SplitUnk23,        cursplit.unk23 );
        WriteNodeWithValue( parent, PROP_EnvOn,             cursplit.envon );
        WriteNodeWithValue( parent, PROP_EnvMulti,          cursplit.env.envmulti );

        //stringstream sstrunkcv;

        //sstrunkcv <<hex <<showbase <<static_cast<uint16_t>(cursplit.unk37);
        //parent.append_child(PROP_EnvUnk19.c_str()).append_child(node_pcdata).set_value( sstrunkcv.str().c_str() );
        //sstrunkcv.str(string());

        //sstrunkcv <<hex <<showbase <<static_cast<uint16_t>(cursplit.unk38);
        //parent.append_child(PROP_EnvUnk20.c_str()).append_child(node_pcdata).set_value( sstrunkcv.str().c_str() );
        //sstrunkcv.str(string());

        //sstrunkcv <<hex <<showbase <<static_cast<uint16_t>(cursplit.unk39);
        //parent.append_child(PROP_EnvUnk21.c_str()).append_child(node_pcdata).set_value( sstrunkcv.str().c_str() );
        //sstrunkcv.str(string());

        //sstrunkcv <<hex <<showbase <<static_cast<uint16_t>(cursplit.unk40);
        //parent.append_child(PROP_EnvUnk22.c_str()).append_child(node_pcdata).set_value( sstrunkcv.str().c_str() );
        //sstrunkcv.str(string());
        
        WriteNodeWithValue( parent, PROP_EnvAtkVol,     cursplit.env.atkvol );
        WriteNodeWithValue( parent, PROP_EnvAtk,        cursplit.env.attack );
        WriteNodeWithValue( parent, PROP_EnvDecay,      cursplit.env.decay );
        WriteNodeWithValue( parent, PROP_EnvSustain,    cursplit.env.sustain );
        WriteNodeWithValue( parent, PROP_EnvHold,       cursplit.env.hold );
        WriteNodeWithValue( parent, PROP_EnvDecay2,     cursplit.env.decay2 );
        WriteNodeWithValue( parent, PROP_EnvRelease,    cursplit.env.release );
        //WriteNodeWithValue( parent, PROP_EnvUnk57,      cursplit.env.rx );
    }


    void WriteKeyGroups( const std::string & destdir )
    {
        using namespace PrgmXML;
        xml_document doc;
        xml_node     kgrpsnode = doc.append_child( ROOT_KeyGroups.c_str() );
        auto         ptrprgms  = m_presbnk.prgmbank().lock(); 

        if( ptrprgms != nullptr )
        {
            for( const auto & grp : ptrprgms->Keygrps() )
                WriteKeyGroup( kgrpsnode, grp );

            stringstream sstrfname;
            sstrfname << utils::TryAppendSlash(destdir) << DEF_KeygroupFname;
            if( ! doc.save_file( sstrfname.str().c_str() ) )
                throw std::runtime_error("Can't write xml file " + sstrfname.str());
        }
        else
            clog << "ProgramBankXMLWriter::WriteKeyGroups(): no program/keygroup data available!";
    }

    void WriteKeyGroup( xml_node & parent, const KeyGroup & grp )
    {
        using namespace PrgmXML;

        WriteCommentNode( parent, "Keygroup ID : " + std::to_string( grp.id ) );
        xml_node kgrpnode = parent.append_child( NODE_KGrp.c_str() );

        WriteNodeWithValue( kgrpnode, PROP_ID,        grp.id );
        WriteNodeWithValue( kgrpnode, NODE_KGrpPoly,  grp.poly );
        WriteNodeWithValue( kgrpnode, NODE_KGrpPrio,  grp.priority );
        WriteNodeWithValue( kgrpnode, NODE_KGrVcLow,  grp.vclow );
        WriteNodeWithValue( kgrpnode, NODE_KGrVcHi,   grp.vchigh );
    }

    void WriteWavInfo( const std::string & destdir )
    {
        using namespace PrgmXML;
        auto ptrwavs = m_presbnk.smplbank().lock();

        if( ptrwavs != nullptr )
        {
            xml_document doc;
            xml_node     infonode = doc.append_child( ROOT_WavInfo.c_str() );

            for( size_t cptwav = 0; cptwav < ptrwavs->NbSlots(); ++cptwav )
            {
                auto ptrwinf = ptrwavs->sampleInfo(cptwav);
                if( ptrwinf != nullptr )
                    WriteAWav( infonode, *ptrwinf );
            }

            stringstream sstrfname;
            sstrfname << utils::TryAppendSlash(destdir) << DEF_WavInfoFname;
            if( ! doc.save_file( sstrfname.str().c_str() ) )
                throw std::runtime_error("Can't write xml file " + sstrfname.str());
        }
        else
            clog << "ProgramBankXMLWriter::WriteWavInfo(): No Sample Data";
    }

    void WriteAWav( xml_node & parent, const WavInfo & winfo )
    {
        using namespace PrgmXML;
        WriteCommentNode( parent, "Sample #" + std::to_string( winfo.id ) );

        xml_node infonode = parent.append_child( NODE_Sample.c_str() );
        WriteNodeWithValue( infonode, PROP_ID,          winfo.id ); 
        WriteCommentNode( infonode, "Tuning Data" );
        WriteNodeWithValue( infonode, PROP_FTune,       winfo.ftune );
        WriteNodeWithValue( infonode, PROP_CTune,       winfo.ctune );
        WriteNodeWithValue( infonode, PROP_RootKey,     winfo.rootkey );
        WriteNodeWithValue( infonode, PROP_KeyTrans,    winfo.ktps );
        WriteCommentNode( infonode, "Misc" );
        WriteNodeWithValue( infonode, PROP_Volume,      winfo.vol );
        WriteNodeWithValue( infonode, PROP_Pan,         winfo.pan );
        //WriteNodeWithValue( infonode, PROP_SmplUnk5,    winfo.unk5 );
        //WriteNodeWithValue( infonode, PROP_SmplUnk58,   winfo.unk58 );
        //WriteNodeWithValue( infonode, PROP_SmplUnk6,    winfo.unk6 );
        //WriteNodeWithValue( infonode, PROP_SmplUnk59,   winfo.unk59 );

        //WriteNodeWithValue( infonode, PROP_SmplUnk9,    winfo.unk9 );
        WriteNodeWithValue( infonode, PROP_SmplLoop,    winfo.smplloop );

        //WriteNodeWithValue( infonode, PROP_SmplUnk11,   winfo.unk11 );
        //WriteNodeWithValue( infonode, PROP_SmplUnk12,   winfo.unk12 );
        //WriteNodeWithValue( infonode, PROP_SmplUnk62,   winfo.unk62 );
        //WriteNodeWithValue( infonode, PROP_SmplUnk13,   winfo.unk13 );

        WriteCommentNode( infonode, "Loop Data (in PCM16 sample points)" );
        //Correct the sample loop points because the samples were exported to PCM16
        if( winfo.smplfmt == eDSESmplFmt::pcm8 )
        {
            WriteNodeWithValue( infonode, PROP_LoopBeg,     winfo.loopbeg * 2 );
            WriteNodeWithValue( infonode, PROP_LoopLen,     winfo.looplen * 2 );
        }
        else if( winfo.smplfmt == eDSESmplFmt::ima_adpcm )
        {
            WriteNodeWithValue( infonode, PROP_LoopBeg,     (winfo.loopbeg * 4) + ::audio::IMA_ADPCM_PreambleLen );
            WriteNodeWithValue( infonode, PROP_LoopLen,     (winfo.looplen * 4) + ::audio::IMA_ADPCM_PreambleLen );
        }
        else
        {
            WriteNodeWithValue( infonode, PROP_LoopBeg,     winfo.loopbeg );
            WriteNodeWithValue( infonode, PROP_LoopLen,     winfo.looplen );
        }

        WriteCommentNode( infonode, "Volume Envelope" );
        WriteNodeWithValue( infonode, PROP_EnvOn,         winfo.envon );
        WriteNodeWithValue( infonode, PROP_EnvMulti,      winfo.envmult );

        //stringstream sstrunkcv;

        //sstrunkcv <<hex <<showbase <<static_cast<uint16_t>(winfo.unk19);
        //infonode.append_child(PROP_EnvUnk19.c_str()).append_child(node_pcdata).set_value( sstrunkcv.str().c_str() );
        //sstrunkcv.str(string());

        //sstrunkcv <<hex <<showbase <<static_cast<uint16_t>(winfo.unk20);
        //infonode.append_child(PROP_EnvUnk20.c_str()).append_child(node_pcdata).set_value( sstrunkcv.str().c_str() );
        //sstrunkcv.str(string());

        //sstrunkcv <<hex <<showbase <<static_cast<uint16_t>(winfo.unk21);
        //infonode.append_child(PROP_EnvUnk21.c_str()).append_child(node_pcdata).set_value( sstrunkcv.str().c_str() );
        //sstrunkcv.str(string());

        //sstrunkcv <<hex <<showbase <<static_cast<uint16_t>(winfo.unk22);
        //infonode.append_child(PROP_EnvUnk22.c_str()).append_child(node_pcdata).set_value( sstrunkcv.str().c_str() );
        //sstrunkcv.str(string());

        WriteNodeWithValue( infonode, PROP_EnvAtkVol,     winfo.atkvol );
        WriteNodeWithValue( infonode, PROP_EnvAtk,        winfo.attack );
        WriteNodeWithValue( infonode, PROP_EnvDecay,      winfo.decay );
        WriteNodeWithValue( infonode, PROP_EnvSustain,    winfo.sustain );
        WriteNodeWithValue( infonode, PROP_EnvHold,       winfo.hold );
        WriteNodeWithValue( infonode, PROP_EnvDecay2,     winfo.decay2 );
        WriteNodeWithValue( infonode, PROP_EnvRelease,    winfo.release );
        //WriteNodeWithValue( infonode, PROP_EnvUnk57,      winfo.unk57 );

    }

private:
    const PresetBank & m_presbnk;
};

//====================================================================================================
//  
//====================================================================================================

    /*
        PresetBankToXML
            Write the 3 XML files for a given set of presets and samples.
    */
    void PresetBankToXML( const DSE::PresetBank & srcbnk, const std::string & destdir )
    {
        ProgramBankXMLWriter(srcbnk).Write(destdir);
    }

    /*
        XMLToPresetBank
            Read the 3 XML files for a given set of presets and samples.
    */
    DSE::PresetBank XMLToPresetBank( const std::string & srcdir )
    {
        return ProgramBankXMLParser(srcdir).Parse();
    }
};
