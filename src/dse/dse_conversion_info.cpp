#include "dse_conversion_info.hpp"

#include <pugixml.hpp>
#include <utils/pugixml_utils.hpp>
#include <utils/parse_utils.hpp>
#include <utils/utility.hpp>
#include <utils/library_wide.hpp>

#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;
using namespace pugixmlutils;

namespace DSE
{

//==============================================================================
//  SMDLPresetConversionInfo
//==============================================================================
    SMDLPresetConversionInfo::NoteRemapData SMDLPresetConversionInfo::RemapNote( dsepresetid_t dsep, midinote_t note )const 
    {
        if( !convtbl.empty() )
        {
            auto itfound = convtbl.find(dsep);

            if( itfound !=convtbl.end() )
            {
                auto foundnote = itfound->second.remapnotes.find(note);
                if( foundnote != itfound->second.remapnotes.end() )
                    return foundnote->second;
            }
        }

        //No changes
        NoteRemapData rmap;
        rmap.destnote = note;
        return move(rmap);
    }


    namespace cvinfoXML
    {
        const string ROOT_ConvData   = "DSEConversionData";   //Root node for the XML file.

        const string NODE_Track      = "Track";               //Conversion data for a single track.
        const string PROP_PairName   = "PairName";            //Name of the DSE SMD and SWD pair.
        const string PROP_OutName    = "OutputName";          //Name to give the resulting midi file
        
        const string NODE_Programs   = "Programs";            //List of presets in the file.
        const string NODE_Program    = "Program";             //A single program in the list of program.
        const string PROP_DSEID      = "DSEProgramID";        //The original Program ID in the DSE file.
        const string PROP_MIDIPrg    = "MIDIPreset";          //The ID to convert the DSE preset to during conversion.
        const string PROP_MIDIBnk    = "MIDIBank";            //The ID of the MIDI bank to use when changing the midi preset.
        const string PROP_MaxKeyDown = "MaxKeyDownDuration";  //Specify a maximum duration a key can be held down. Used for keys held indefinetly in some tracks causing issues with using different samples.
        const string PROP_Transpose  = "Transpose";           //Specify a number of octaves to transpose notes played by the instrument. Value is signed!!!!
        const string PROP_ForceChan  = "ForceChannel";        //Specify a MIDI channel on which the notes of the Program have to be played on.


        const string NODE_KeyRemaps  = "KeyRemaps";           //Key conversion table.
        const string NODE_KeyRemap   = "Remap";               //A single key remap entry
        const string PROP_InKey      = "OriginalKey";         //The original key number to remap.
        const string PROP_OutKey     = "RemapedKey";          //The key to remap to.

        
    };

//==============================================================================
//  SMDLConvInfoDBXMLWriter
//==============================================================================

    /*
        SMDLConvInfoDBXMLWriter
            This is meant to write out a "blank" populated conversion info file for a 
            particular DSE track.
    */
    class SMDLConvInfoDBXMLWriter
    {
    public:
        SMDLConvInfoDBXMLWriter( const SMDLConvInfoDB & db )
            :m_curdb(db)
        {}

        void Write(const string & xmlpath)
        {
            using namespace pugi;
            using namespace cvinfoXML;

            xml_document doc;
            xml_node     root = doc.append_child( ROOT_ConvData.c_str() );

            for( const auto & entry : m_curdb )
            {
                WriteATrack( root, entry.first, entry.second );
            }

            if( ! doc.save_file( xmlpath.c_str() ) )
                throw std::runtime_error("Can't write xml file " + xmlpath);
        }

    private:
        void WriteATrack( pugi::xml_node parent, const std::string & trkname, const SMDLPresetConversionInfo & entry )
        {
            using namespace pugi;
            using namespace cvinfoXML;

            pugixmlutils::WriteCommentNode( parent, "==========================================================================" );
            pugixmlutils::WriteCommentNode( parent, trkname );
            pugixmlutils::WriteCommentNode( parent, "==========================================================================" );

            xml_node curtrk = parent.append_child( NODE_Track.c_str() );
            WriteNodeWithValue( curtrk, PROP_PairName, trkname );
            WriteNodeWithValue( curtrk, PROP_OutName,  trkname );

            xml_node curprgms = curtrk.append_child( NODE_Programs.c_str() );
            //Write all programs
            for( const auto & prg : entry )
            {
                WriteAPrgm( curprgms, prg.first, prg.second );
            }
        }

        void WriteAPrgm( pugi::xml_node parent, DSE::dsepresetid_t prgid, const SMDLPresetConversionInfo::PresetConvData & prgm )
        {
            using namespace pugi;
            using namespace cvinfoXML;
           
            stringstream sstrname;

            if( prgid == 0x7F ) //Drum are special
            {
                sstrname <<hex <<"0x" <<uppercase <<prgid <<nouppercase << " Drumkit";
            }
            else
            {
                sstrname <<hex <<"0x" <<uppercase <<prgid <<nouppercase << "   (smpl ";  
                for( const auto & smpl : prgm.origsmplids )
                    sstrname <<right <<setw(6) <<setfill(' ') <<"0x" <<uppercase <<smpl <<nouppercase <<" ";
                sstrname <<")";
            }
            pugixmlutils::WriteCommentNode( parent, sstrname.str() );

            //Write Program content
            xml_node curprgm = parent.append_child( NODE_Program.c_str() );
            WriteNodeWithValue( curprgm, PROP_DSEID,   prgid );

            if( prgid == 0x7F ) //Drum are special
                WriteNodeWithValue( curprgm, PROP_MIDIPrg, 1 );
            else
                WriteNodeWithValue( curprgm, PROP_MIDIPrg, prgm.midipres );

            if( prgm.midibank != 0 )
                WriteNodeWithValue( curprgm, PROP_MIDIBnk, prgm.midibank );

            if( prgm.idealchan != SMDLPresetConversionInfo::PresetConvData::Invalid_Chan )
                WriteNodeWithValue( curprgm, PROP_ForceChan, prgm.idealchan );

            //Write KeyRemaps
            if( ! prgm.remapnotes.empty() )
                WriteKeyRemaps(curprgm, prgm);

        }

        void WriteKeyRemaps( pugi::xml_node parent, const SMDLPresetConversionInfo::PresetConvData & prgm )
        {
            using namespace pugi;
            using namespace cvinfoXML;
            
            xml_node remaps = parent.append_child( NODE_KeyRemaps.c_str() );
            size_t cntremap = 0;
            for( const auto & remap : prgm.remapnotes )
            {
                stringstream sstrname;
                sstrname <<hex <<"0x" <<uppercase <<cntremap <<nouppercase << " -  (smpl 0x"
                         <<uppercase <<remap.second.origsmplid <<nouppercase <<" )";  
                pugixmlutils::WriteCommentNode( remaps, sstrname.str() );
                xml_node curremap =  remaps.append_child( NODE_KeyRemap.c_str() );
                WriteNodeWithValue( curremap, PROP_InKey,  remap.first );
                WriteNodeWithValue( curremap, PROP_OutKey, remap.second.destnote );
                ++cntremap;
            }
        }

    private:
        const SMDLConvInfoDB & m_curdb;
    };

//==============================================================================
//  SMDLConvInfoDBXMLParser
//==============================================================================
    class SMDLConvInfoDBXMLParser
    {
    public:
        SMDLConvInfoDBXMLParser( SMDLConvInfoDB & db )
            :m_curdb(db)
        {}

        void Parse(const string & xmlpath)
        {
            using namespace pugi;
            using namespace cvinfoXML;

            xml_document     indoc;
            xml_parse_result result = indoc.load_file( xmlpath.c_str() );
            HandleParsingError( result, xmlpath );

            auto & cventries = indoc.child(ROOT_ConvData.c_str()).children();

            //Read every elements
            for( auto & curnode : cventries )
            {
                if( curnode.name() == NODE_Track )
                    ParseRemapEntry( curnode );
            }
        }

    private:
        void ParseRemapEntry( pugi::xml_node curnode )
        {
            using namespace pugi;
            using namespace cvinfoXML;
            SMDLPresetConversionInfo info;
            string                   pairname;

            for( auto & propnode : curnode.children() )
            {
                if( propnode.name() == PROP_PairName )
                    pairname = propnode.child_value();
                else if( propnode.name() == PROP_OutName )
                    info.SetOutputName( propnode.child_value() );
                else if( propnode.name() == NODE_Programs )
                    ParsePrograms( propnode, info, pairname );
            }

            if( !pairname.empty() )
                m_curdb.AddConversionInfo( pairname, move(info) );
            else
                clog <<"<!>- Ignored a remap entry while parsing the XML conversion info, because the name of the pair was empty or not specified!\n";
        }

        void ParsePrograms( pugi::xml_node curnode, SMDLPresetConversionInfo & convinf, const string & trkname )
        {
            using namespace pugi;
            using namespace cvinfoXML;

            for( auto & prgnode : curnode.children() )
            {
                if( prgnode.name() == NODE_Program )
                    ParseAProgram( prgnode, convinf, trkname );
            }
        }

        void ParseAProgram( pugi::xml_node curnode, SMDLPresetConversionInfo & convinf, const string & trkname )
        {
            using namespace pugi;
            using namespace cvinfoXML;

            SMDLPresetConversionInfo::PresetConvData pconv;
            stringstream                             sstrparse;
            dsepresetid_t                            dseid = InvalidDSEPresetID;

            for( auto & progprop : curnode.children() )
            {
                if( progprop.name() == PROP_DSEID )
                    utils::parseHexaValToValue(  progprop.child_value(), dseid );
                else if( progprop.name() == PROP_MIDIPrg )
                    pconv.midipres = utils::parseByte( progprop.child_value() ) - 1; //Bring back onto 0-127
                else if( progprop.name() == PROP_MIDIBnk )
                    utils::parseHexaValToValue(  progprop.child_value(), pconv.midibank );
                else if( progprop.name() == PROP_MaxKeyDown )
                    utils::parseHexaValToValue( progprop.child_value(), pconv.maxkeydowndur );
                else if( progprop.name() == PROP_Transpose )
                    pconv.transpose = utils::parseSignedByte( progprop.child_value() );
                else if( progprop.name() == PROP_ForceChan )
                    pconv.idealchan = (utils::parseByte( progprop.child_value() ) - 1); //Bring back to 0-15 from 1-16
                else if( progprop.name() == NODE_KeyRemaps )
                {
                    ParseKeyRemapData( progprop, pconv, dseid, trkname );
                }
            }

            if( dseid != InvalidDSEPresetID )
            {
                if( pconv.midipres != InvalidPresetID && pconv.midipres > CHAR_MAX )
                {
                    clog << "#CVInfoParser : Forced preset for preset " <<dseid << " for track " <<trkname << " was not one of the valid 127 MIDI presets! Ignoring!\n";
                    pconv.midipres = InvalidPresetID;
                }

                if( pconv.idealchan != UCHAR_MAX && pconv.idealchan >= NbMidiChannels )
                {
                    clog << "#CVInfoParser : Forced channel for preset " <<dseid <<" for track " <<trkname << " was not one of the valid 16 MIDI channel! Ignoring!\n";
                    pconv.idealchan = UCHAR_MAX;
                }
            }

            if( dseid != InvalidDSEPresetID )
                convinf.AddPresetConvInfo( dseid, move(pconv) );
            else 
                clog << "<!>- Ignored a " <<NODE_Program <<" node because there was no DSE program ID specified!\n";
        }

        void ParseKeyRemapData( pugi::xml_node curnode, SMDLPresetConversionInfo::PresetConvData & pconv, dsepresetid_t preset, const string & trkname )
        {
            using namespace pugi;
            using namespace cvinfoXML;

            for( auto & keyremap : curnode.children() )
            {
                if( keyremap.name() == NODE_KeyRemap )
                    ParseAKeyRemap( keyremap, pconv, preset, trkname );
            }
        }

        void ParseAKeyRemap( pugi::xml_node curnode, SMDLPresetConversionInfo::PresetConvData & pconv, dsepresetid_t preset, const string & trkname )
        {
            using namespace pugi;
            using namespace cvinfoXML;
            midinote_t inkey     = InvalidMIDIKey;
            midinote_t outkey    = InvalidMIDIKey;
            presetid_t midprg    = InvalidPresetID;
            bankid_t   midbnk    = InvalidBankID;
            uint8_t    idealchan = UCHAR_MAX;

            for( auto & keyprop : curnode.children() )
            {
                if( keyprop.name() == PROP_InKey )
                    inkey = utils::parseByte( keyprop.child_value() );
                else if( keyprop.name() == PROP_OutKey )
                    outkey = utils::parseByte( keyprop.child_value() );
                else if( keyprop.name() == PROP_MIDIPrg )
                    midprg = utils::parseByte( keyprop.child_value() ) - 1; //Bring back onto 0-127
                else if( keyprop.name() == PROP_MIDIBnk )
                    utils::parseHexaValToValue(  keyprop.child_value(), midbnk );
                else if( keyprop.name() == PROP_ForceChan )
                    idealchan = ( utils::parseByte( keyprop.child_value() ) - 1); //Bring back to 0-15
            }

            if( inkey != InvalidMIDIKey && outkey != InvalidMIDIKey )
            {
                SMDLPresetConversionInfo::NoteRemapData rmap;
                rmap.destnote = outkey;

                if( midprg != InvalidPresetID )
                {
                    if( midprg >= 0 && midprg <= CHAR_MAX )
                        rmap.destpreset = midprg;
                    else
                        clog << "#CVInfoParser : Forced preset for preset " <<preset <<" for key " <<inkey << " for track " <<trkname << " was not one of the valid 127 MIDI presets! Ignoring!\n";
                }

                if( midbnk != InvalidBankID )
                {
                    rmap.destbank = midbnk;
                }

                if( idealchan != UCHAR_MAX )
                {
                    if( idealchan < NbMidiChannels )
                        rmap.idealchan = idealchan;
                    else
                        clog << "#CVInfoParser : Forced channel for preset " <<preset <<" for key " <<inkey <<" for track " <<trkname << " was not one of the valid 16 MIDI channel! Ignoring!\n";
                }

                pconv.remapnotes.insert( make_pair( inkey, rmap ) );
            }
        }

    private:
        SMDLConvInfoDB & m_curdb;

    };

//==============================================================================
//  SMDLConvInfoDB
//==============================================================================
    /*
        SMDLConvInfoDB
            -cvinfxml: path to the xml file containing conversion data.
            The file will be parsed on construction.
    */
    SMDLConvInfoDB::SMDLConvInfoDB( const std::string & cvinfxml )
    {
        Parse(cvinfxml);
    }

    SMDLConvInfoDB::SMDLConvInfoDB()
    {}

    /*
        Parse
            Triggers parsing of the specified xml file!
    */
    void SMDLConvInfoDB::Parse( const std::string & cvinfxml )
    {
        SMDLConvInfoDBXMLParser(*this).Parse(cvinfxml);
    }

    /*
        Write
            Writes a "blank" Conversion Info file, with all the default values for each programs and splits
    */
    void SMDLConvInfoDB::Write( const std::string & cvinfxml )
    {
        SMDLConvInfoDBXMLWriter(*this).Write(cvinfxml);
    }


    void SMDLConvInfoDB::AddConversionInfo( const std::string & name, SMDLPresetConversionInfo && info )
    {
        m_convdata.insert( move( make_pair( name, move(info) ) ) );
    }
};