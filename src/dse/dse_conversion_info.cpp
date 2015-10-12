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

//==============================================================================
//  SMDLConvInfoDBXMLParser
//==============================================================================
    namespace cvinfoXML
    {
        const string ROOT_ConvData   = "DSEConversionData";   //Root node for the XML file.

        const string NODE_Track      = "Track";               //Conversion data for a single track.
        const string PROP_PairName   = "PairName";            //Name of the DSE SMD and SWD pair.
        
        const string NODE_Programs   = "Programs";            //List of presets in the file.
        const string NODE_Program    = "Program";             //A single program in the list of program.
        const string PROP_DSEID      = "DSEProgramID";        //The original Program ID in the DSE file.
        const string PROP_MIDIPrg    = "MIDIPreset";          //The ID to convert the DSE preset to during conversion.
        const string PROP_MIDIBnk    = "MIDIBank";            //The ID of the MIDI bank to use when changing the midi preset.
        const string PROP_MaxKeyDown = "MaxKeyDownDuration";  //Specify a maximum duration a key can be held down. Used for keys held indefinetly in some tracks causing issues with using different samples.
        const string PROP_Transpose  = "Transpose";           //Specify a number of octaves to transpose notes played by the instrument. Value is signed!!!!

        const string NODE_KeyRemaps  = "KeyRemaps";           //Key conversion table.
        const string NODE_KeyRemap   = "Remap";               //A single key remap entry
        const string PROP_InKey      = "OriginalKey";         //The original key number to remap.
        const string PROP_OutKey     = "RemapedKey";          //The key to remap to.

        
    };

    /*
    */
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
                else if( propnode.name() == NODE_Programs )
                    ParsePrograms( propnode, info );
            }

            if( !pairname.empty() )
                m_curdb.AddConversionInfo( pairname, move(info) );
            else
                clog <<"<!>- Ignored a remap entry while parsing the XML conversion info, because the name of the pair was empty or not specified!\n";
        }

        void ParsePrograms( pugi::xml_node curnode, SMDLPresetConversionInfo & convinf )
        {
            using namespace pugi;
            using namespace cvinfoXML;

            for( auto & prgnode : curnode.children() )
            {
                if( prgnode.name() == NODE_Program )
                    ParseAProgram( prgnode, convinf );
            }
        }

        void ParseAProgram( pugi::xml_node curnode, SMDLPresetConversionInfo & convinf )
        {
            using namespace pugi;
            using namespace cvinfoXML;

            SMDLPresetConversionInfo::PresetConvData pconv;
            stringstream                             sstrparse;
            int8_t                                   dseid = -1;

            for( auto & progprop : curnode.children() )
            {
                if( progprop.name() == PROP_DSEID )
                    dseid = utils::parseByte( progprop.child_value() );
                else if( progprop.name() == PROP_MIDIPrg )
                    pconv.midipres = utils::parseByte( progprop.child_value() ) - 1; //Bring back onto 0-127
                else if( progprop.name() == PROP_MIDIBnk )
                    pconv.midibank = utils::parseByte( progprop.child_value() );
                else if( progprop.name() == PROP_MaxKeyDown )
                    utils::parseHexaValToValue( progprop.child_value(), pconv.maxkeydowndur );
                else if( progprop.name() == PROP_Transpose )
                    pconv.transpose = utils::parseSignedByte( progprop.child_value() );
                else if( progprop.name() == NODE_KeyRemaps )
                {
                    ParseKeyRemapData( progprop, pconv );
                }
            }

            if( dseid != -1 )
                convinf.AddPresetConvInfo( dseid, move(pconv) );
            else 
                clog << "<!>- Ignored a " <<NODE_Program <<" node because there was no DSE program ID specified!\n";
        }

        void ParseKeyRemapData( pugi::xml_node curnode, SMDLPresetConversionInfo::PresetConvData & pconv )
        {
            using namespace pugi;
            using namespace cvinfoXML;

            for( auto & keyremap : curnode.children() )
            {
                if( keyremap.name() == NODE_KeyRemap )
                    ParseAKeyRemap( keyremap, pconv );
            }
        }

        void ParseAKeyRemap( pugi::xml_node curnode, SMDLPresetConversionInfo::PresetConvData & pconv )
        {
            using namespace pugi;
            using namespace cvinfoXML;
            uint8_t inkey  = 0xFF;
            uint8_t outkey = 0xFF;
            uint8_t midprg = 0xFF;
            uint8_t midbnk = 0xFF;

            for( auto & keyprop : curnode.children() )
            {
                if( keyprop.name() == PROP_InKey )
                    inkey = utils::parseByte( keyprop.child_value() );
                else if( keyprop.name() == PROP_OutKey )
                    outkey = utils::parseByte( keyprop.child_value() );
                else if( keyprop.name() == PROP_MIDIPrg )
                    midprg = utils::parseByte( keyprop.child_value() ) - 1; //Bring back onto 0-127
                else if( keyprop.name() == PROP_MIDIBnk )
                    midbnk = utils::parseByte( keyprop.child_value() );
            }

            if( inkey != 0xFF && outkey != 0xFF )
            {
                SMDLPresetConversionInfo::NoteRemapData rmap;
                rmap.destnote = outkey;

                if( midprg != 0xFF )
                    rmap.destpreset = midprg;

                if( midbnk != 0xFF )
                    rmap.destbank = midbnk;

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

    /*
        Parse
            Triggers parsing of the specified xml file!
    */
    void SMDLConvInfoDB::Parse( const std::string & cvinfxml )
    {
        SMDLConvInfoDBXMLParser(*this).Parse(cvinfxml);
    }


    void SMDLConvInfoDB::AddConversionInfo( const std::string & name, SMDLPresetConversionInfo && info )
    {
        m_convdata.insert( move( make_pair( name, move(info) ) ) );
    }
};