#include "sprite_data.hpp"
#include <string>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <atomic>
#include <ppmdu/containers/tiled_image.hpp>
#include <ppmdu/ext_fmts/png_io.hpp>
#include <ppmdu/ext_fmts/riff_palette.hpp>
#include <ppmdu/utils/poco_wrapper.hpp>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/SAX/InputSource.h>
#include <Poco/XML/XMLWriter.h>
#include <Poco/SAX/AttributesImpl.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NamedNodeMap.h>
#include <Poco/DirectoryIterator.h>

using namespace std;
//using namespace utils;
using utils::io::eSUPPORT_IMG_IO;
using namespace Poco::XML;


namespace pmd2{ namespace graphics
{
    namespace xmlstrings
    {
        //Filesnames
        static const string SPRITE_Properties  = "spriteinfo.xml";
        //static const string SPRITE_Colorinf    = "colorinfo.xml";
        static const string SPRITE_Palette     = "palette.xml";
        static const string SPRITE_Animations  = "animations.xml";
        static const string SPRITE_Frames      = "frames.xml";
        static const string SPRITE_Offsets     = "offsets.xml";

        //Generic Attributes
        static const string XML_ATTR_NAME      = "name";

        //Animation Stuff
        static const string XML_NODE_ANIMDAT   = "AnimData";
        static const string XML_NODE_ANIMGRP   = "AnimGroup";
        static const string XML_NODE_ANIMSEQ   = "AnimSequence";
        static const string XML_NODE_ANIMFRM   = "AnimFrame";

        //Meta-Frames Stuff
        static const string XML_NODE_FRMLST    = "FrameList";
        static const string XML_NODE_FRMGRP    = "FrameGroup";
        static const string XML_NODE_FRMFRM    = "Frame";

        //OffsetList Stuff
        static const string XML_NODE_OFFLST    = "OffsetList";
        static const string XML_NODE_OFFSET    = "Offset";

        //Sprite Property Stuff
        static const string XML_NODE_SPRPROPS  = "SpriteProperties";

        //Other nodes
        static const string XML_NODE_PALLETTE  = "Palette";
        static const string XML_NODE_SHADOW    = "Shadow";
        static const string XML_NODE_SPRITE    = "Sprite";
        static const string XML_NODE_RES       = "Resolution";
        static const string XML_NODE_COLOR     = "Color";

        //Properties Stuff (names of all the properties we'll write)
        static const string XML_PROP_FRMINDEX  = "ImageIndex";
        static const string XML_PROP_UNK0      = "Unk0";
        static const string XML_PROP_OFFSETY   = "YOffset";
        static const string XML_PROP_OFFSETX   = "XOffset";
        static const string XML_PROP_UNK1      = "Unk1";
    
        static const string XML_PROP_X         = "X";
        static const string XML_PROP_Y         = "Y";
        static const string XML_PROP_WIDTH     = "Width";
        static const string XML_PROP_HEIGTH    = "Heigth";
        static const string XML_PROP_HFLIP     = "HFlip";
        static const string XML_PROP_VFLIP     = "VFlip";
        static const string XML_PROP_MOSAIC    = "Mosaic";
        static const string XML_PROP_OFFXBIT5  = "XOffsetBit5";
        static const string XML_PROP_OFFXBIT6  = "XOffsetBit6";
        static const string XML_PROP_OFFXBIT7  = "XOffsetBit7";
        static const string XML_PROP_OFFYBIT3  = "YOffsetBit3";
        static const string XML_PROP_OFFYBIT5  = "YOffsetBit5";
        static const string XML_PROP_OFFYBIT6  = "YOffsetBit6";

        static const string XML_PROP_DURATION  = "Duration";
        static const string XML_PROP_METAINDEX = "MetaFrameGroupIndex";

        static const string XML_PROP_UNK3      = "Unk3";
        static const string XML_PROP_COLPERROW = "ColorsPerRow";
        static const string XML_PROP_UNK4      = "Unk4";
        static const string XML_PROP_UNK5      = "Unk5";

        static const string XML_PROP_UNK6      = "Unk6";
        static const string XML_PROP_UNK7      = "Unk7";
        static const string XML_PROP_UNK8      = "Unk8";
        static const string XML_PROP_UNK9      = "Unk9";
        static const string XML_PROP_UNK10     = "Unk10";

        static const string XML_PROP_IS8W      = "Is8Ways";
        static const string XML_PROP_IS256COL  = "Is256Colors";
        static const string XML_PROP_ISMOSAICS = "IsMosaicSprite";
        static const string XML_PROP_UNK11     = "Unk11";
        static const string XML_PROP_UNK12     = "Unk12";

        static const string XML_PROP_RED       = "R";
        static const string XML_PROP_GREEN     = "G";
        static const string XML_PROP_BLUE      = "B";

        //Special chars
        static const string PARSE_HEX_NUMBER   = "0x";


        //Elements to parse for Sprite Info
        //static const vector<const string> XML_ParseList_SpriteInfoProps=
        //{{
        //        XML_PROP_UNK3,
        //        XML_PROP_COLPERROW,
        //        XML_PROP_UNK4,
        //        XML_PROP_UNK5,
        //        XML_PROP_UNK6,
        //        XML_PROP_UNK7,
        //        XML_PROP_UNK8,
        //        XML_PROP_UNK9,
        //        XML_PROP_UNK10,
        //        XML_PROP_IS8W,
        //        XML_PROP_IS256COL,
        //        XML_PROP_ISMOSAICS,
        //        XML_PROP_UNK11,
        //        XML_PROP_UNK12,
        //}};

        ////Elements to parse for Offsets
        //static const vector<const string&> XML_ParseList_Offsets=
        //{{
        //        XML_NODE_OFFSET,
        //        XML_PROP_X,
        //        XML_PROP_Y,
        //}};

        ////Elements to parse for Animations
        //static const vector<const string> XML_ParseList_Animations=
        //{{
        //        XML_NODE_ANIMGRP,
        //        XML_NODE_ANIMSEQ,
        //        XML_NODE_ANIMFRM,
        //        XML_NODE_SPRITE,
        //        XML_NODE_SHADOW,
        //        XML_PROP_DURATION,
        //        XML_PROP_METAINDEX,
        //        XML_PROP_OFFSETY,
        //        XML_PROP_OFFSETX,
        //}};

        ////Elements to parse for MetaFrames
        //static const vector<const string> XML_ParseList_MetaFrames=
        //{{
        //        XML_NODE_FRMGRP,
        //        XML_NODE_FRMFRM,
        //        XML_PROP_FRMINDEX,
        //        XML_PROP_UNK0,
        //        XML_PROP_OFFSETY,
        //        XML_PROP_OFFSETX,
        //        XML_PROP_UNK1,
        //        XML_NODE_RES,
        //        XML_PROP_WIDTH,
        //        XML_PROP_HEIGTH,
        //        XML_PROP_HFLIP,
        //        XML_PROP_VFLIP,
        //        XML_PROP_MOSAIC,
        //        XML_PROP_OFFXBIT5,
        //        XML_PROP_OFFXBIT6,
        //        XML_PROP_OFFXBIT7,
        //        XML_PROP_OFFYBIT3,
        //        XML_PROP_OFFYBIT5,
        //        XML_PROP_OFFYBIT6,
        //}};

        ////Elements to parse for MetaFrames
        //static const vector<const string> XML_ParseList_Palette=
        //{{
        //        XML_NODE_COLOR,
        //        XML_PROP_RED,
        //        XML_PROP_GREEN,
        //        XML_PROP_BLUE,
        //}};

    };
    static const string SPRITE_IMGs_DIR   = "imgs"; //Name of the sub-folder for the images
    static const string Palette_Filename  = "palette.pal";

    /*
        Definitions the list of required files for building a sprite from a folder !
    */
    const vector<string> SpriteBuildingRequiredFiles =
    {{
        xmlstrings::SPRITE_Properties,
        xmlstrings::SPRITE_Animations,
        xmlstrings::SPRITE_Frames,
        xmlstrings::SPRITE_Offsets,
        SPRITE_IMGs_DIR,
    }};

//
//  Descriptions for the various parameters
//
    const std::string SprInfo::DESC_Unk3            = "If set to 1, write palette in \"Main Ext Spr 0\" runtime palette! Additionally, force sprite to be read to memory as 4 bpp!";
    const std::string SprInfo::DESC_nbColorsPerRow  = "0 to 16.. Changes the amount of colors loaded on a single row in the runtime palette sheet.";
    const std::string SprInfo::DESC_Unk4            = "In extended palette mode, seems to change the location of the palette..";
    const std::string SprInfo::DESC_Unk5            = "(1111 0000)If not 0xF or 0, makes the image data be written to memory as 8 bpp, from 4 bpp! Also seem to force palette as 256 colors extended pal?\n"
                                                      "   Otherwise, the image is loaded as-is?\n"
                                                      "(0000 1111) Set in which slot in a palette sheet in memory the palette will be drawn at!\n";

    const std::string SprInfo::DESC_Unk6            = "Nb of \"blocks\" the image takes up in tile memory. Affects where things like the relic fragment will be loaded.\n" 
                                                      "       If too low, item not loaded and replaced with parts of character sprite.";
    const std::string SprInfo::DESC_Unk7            = "unknown";
    const std::string SprInfo::DESC_Unk8            = "unknown";
    const std::string SprInfo::DESC_Unk9            = "unknown";
    const std::string SprInfo::DESC_Unk10           = "unknown";

    const std::string SprInfo::DESC_Is8WaySprite    = "If 1 character sprite. If 0, other sprite. This messes with what anim groups are used for!";
    const std::string SprInfo::DESC_Is256Sprite     = "If 1, the game draw the sprite as a 8bpp 256 color sprite from memory!(You need to specify it in the palette info too for it to work!)\n"
                                                      "       If 0, images are drawn as 4bpp !";
    const std::string SprInfo::DESC_IsMosaicSpr     = "If 1, load the first row of tiles of each images one after the other, the the second, and so on. Seems to be for very large animated sprites!";
    const std::string SprInfo::DESC_Unk11           = "This far 0, 1, 3(d79p41a1.wan), 4(as001.wan).. Seems to deal with the palette slot in-game.";
    const std::string SprInfo::DESC_Unk12           = "unknown";

//=============================================================================================
//  Utility
//=============================================================================================

    /*
        Used to hold a couple of useful statistics to determine task completion.
    */
    struct workstatistics
    {
        uint32_t propFrames;
        uint32_t propAnims;
        uint32_t propMFrames;
        uint32_t propOffsets;

        uint32_t totalAnimFrms;
        uint32_t totalAnimSeqs;
    };

    /*
    */
    bool AreReqFilesPresent_Sprite( const std::vector<std::string> & filelist )
    {
        for( const auto & filename : SpriteBuildingRequiredFiles )
        {
            auto itfound = std::find( filelist.begin(), filelist.end(), filename );

            if( itfound == filelist.end() )
                return false;
        }

        //The palette isn't required in most cases
        return true;
    }

    bool AreReqFilesPresent_Sprite( const std::string & dirpath )
    {
        vector<string> dircontent = utils::ListDirContent_FilesAndDirectories( dirpath, true );
        return AreReqFilesPresent_Sprite(dircontent);
    }

    /*
    */
    std::vector<string> GetMissingRequiredFiles_Sprite( const std::vector<std::string> & filelist )
    {
        vector<string> missingf;

        for( const auto & filename : SpriteBuildingRequiredFiles )
        {
            auto itfound = std::find( filelist.begin(), filelist.end(), filename );

            if( itfound == filelist.end() )
                missingf.push_back( filename );
        }

        return std::move( missingf );
    }

    std::vector<string> GetMissingRequiredFiles_Sprite( const std::string & dirpath )
    {
        vector<string> dircontent = utils::ListDirContent_FilesAndDirectories( dirpath, true );
        return GetMissingRequiredFiles_Sprite( dircontent );
    }

    /*
        Parses a value directly as is.
    */
    template<class _Ty>
        void _parseXMLValToValue( const Poco::XML::XMLString & str, _Ty out_val )
    {
        stringstream sstr(str);
        sstr <<dec << str;
        sstr >> out_val;
    }

    /*
        Parse a value that might be an hex number or not.
    */
    template<class _Ty>
        void _parseXMLHexaValToValue( const Poco::XML::XMLString & str, _Ty out_val )
    {
        stringstream sstr;
        std::size_t  foundprefix = str.find( xmlstrings::PARSE_HEX_NUMBER );

        if( foundprefix != string::npos )
            sstr <<hex <<string( foundprefix + str.begin(), str.end() ).substr(2); //make sure the string begins at "0x" and skip "0x"
        else
            sstr <<dec <<str;

        sstr >> out_val;
    }

    /*
        Parse the "name" attribute from the attribute map sprcified.
        Return empty string if the attribute wasn't found!
    */
    string _parseNameAttribute( Poco::XML::NamedNodeMap * attribmap )
    {
        AutoPtr<NamedNodeMap> nmap = attribmap;
        for( unsigned long ctatt = 0; ctatt < nmap->length(); ++ctatt )
        {
            Node * pAtt = nmap->item(ctatt);
            if( pAtt->nodeName() == xmlstrings::XML_ATTR_NAME )
                return pAtt->nodeValue();
        }
        return string();
    }

//=============================================================================================
//  Sprite XML Templates Reader
//=============================================================================================
    /*
        A class that reads the xml template files for naming animation groups properly
    */
    class SpriteTemplateReader
    {
    };

//=============================================================================================
//  Sprite to XML writer
//=============================================================================================
    template<class _SPRITE_t>
        class SpriteToXML
    {
    public:
        typedef _SPRITE_t sprite_t;

        SpriteToXML( const sprite_t & myspr)
            :m_inSprite(myspr), m_pProgresscnt(nullptr)
        {
        }

        //Write all the data for the Sprite as XML in the
        // specified folder  under ! 
        // -colorpalasxml : if true, will print an xml palette alongside the rest!
        void WriteXMLFiles( const string & folderpath, workstatistics stats, bool xmlcolorpal = false, std::atomic<uint32_t> * progresscnt = nullptr ) 
        {
            m_outPath = Poco::Path(folderpath);
            m_pProgresscnt = progresscnt;

            WriteProperties();
            WriteAnimations     ( stats.propAnims, stats.totalAnimSeqs, stats.totalAnimFrms );
            WriteMetaFrameGroups( stats.propMFrames );
            WriteOffsets        ( stats.propOffsets );

            //Write xml palette if needed
            if( xmlcolorpal )
                WritePalette();
        }

    private:
        //
        //  Common Stuff
        //
        void InitWriter( Poco::XML::XMLWriter & writer, const std::string & rootnodename )
        {
            writer.setNewLine("\n");
            writer.startDocument();
            writer.startElement( "", "", rootnodename );
        }

        void DeinitWriter( Poco::XML::XMLWriter& writer, const std::string & rootnodename )
        {
            writer.endElement( "", "", rootnodename );
            writer.endDocument();
        }


        template<typename _myintt>
            std::string turnIntToHexStr( _myintt anint )
        {
            using xmlstrings::PARSE_HEX_NUMBER;
            stringstream sstr;
            sstr <<PARSE_HEX_NUMBER << hex <<anint;
            return sstr.str();
        }

        void writeComment( XMLWriter & writer, const string & str )
        {
            writer.comment( str.c_str(), 0, str.size() );
        }

    private:
        //
        //  Specific Stuff
        //

        void WriteProperties()
        {
            using namespace xmlstrings;
            Poco::Path outpath = Poco::Path(m_outPath).append(SPRITE_Properties);
            ofstream   outfile( outpath.toString() );
            XMLWriter  writer(outfile, XMLWriter::WRITE_XML_DECLARATION | XMLWriter::PRETTY_PRINT );
                
            InitWriter( writer, XML_NODE_SPRPROPS );
            {
                //Color stuff
                writeComment( writer, SprInfo::DESC_Unk3 );
                writer.dataElement( "", "", XML_PROP_UNK3,      turnIntToHexStr( m_inSprite.getSprInfo().m_Unk3 ) );

                writeComment( writer, SprInfo::DESC_nbColorsPerRow );
                writer.dataElement( "", "", XML_PROP_COLPERROW, std::to_string( m_inSprite.getSprInfo().m_nbColorsPerRow ) );

                writeComment( writer, SprInfo::DESC_Unk4 );
                writer.dataElement( "", "", XML_PROP_UNK4,      turnIntToHexStr( m_inSprite.getSprInfo().m_Unk4 ) );

                writeComment( writer, SprInfo::DESC_Unk5 );
                writer.dataElement( "", "", XML_PROP_UNK5,      turnIntToHexStr( m_inSprite.getSprInfo().m_Unk5 ) );

                //Anim Stuff
                writeComment( writer, SprInfo::DESC_Unk6 );
                writer.dataElement( "", "", XML_PROP_UNK6,      turnIntToHexStr( m_inSprite.getSprInfo().m_Unk6 ) );

                writeComment( writer, SprInfo::DESC_Unk7 );
                writer.dataElement( "", "", XML_PROP_UNK7,      turnIntToHexStr( m_inSprite.getSprInfo().m_Unk7 ) );

                writeComment( writer, SprInfo::DESC_Unk8 );
                writer.dataElement( "", "", XML_PROP_UNK8,      turnIntToHexStr( m_inSprite.getSprInfo().m_Unk8 ) );

                writeComment( writer, SprInfo::DESC_Unk9 );
                writer.dataElement( "", "", XML_PROP_UNK9,      turnIntToHexStr( m_inSprite.getSprInfo().m_Unk9 ) );

                writeComment( writer, SprInfo::DESC_Unk10 );
                writer.dataElement( "", "", XML_PROP_UNK10,      turnIntToHexStr( m_inSprite.getSprInfo().m_Unk10 ) );

                //Other properties
                writeComment( writer, SprInfo::DESC_Is8WaySprite );
                writer.dataElement( "", "", XML_PROP_IS8W,      std::to_string( m_inSprite.getSprInfo().m_is8WaySprite ) );

                writeComment( writer, SprInfo::DESC_Is256Sprite );
                writer.dataElement( "", "", XML_PROP_IS256COL,  std::to_string( m_inSprite.getSprInfo().m_is256Sprite ) );

                writeComment( writer, SprInfo::DESC_IsMosaicSpr );
                writer.dataElement( "", "", XML_PROP_ISMOSAICS, std::to_string( m_inSprite.getSprInfo().m_IsMosaicSpr ) );

                writeComment( writer, SprInfo::DESC_Unk11 );
                writer.dataElement( "", "", XML_PROP_UNK11,     turnIntToHexStr( m_inSprite.getSprInfo().m_Unk11 ) );

                writeComment( writer, SprInfo::DESC_Unk12 );
                writer.dataElement( "", "", XML_PROP_UNK12,     turnIntToHexStr( m_inSprite.getSprInfo().m_Unk12 ) );
            }
            DeinitWriter( writer, XML_NODE_SPRPROPS );
        }

        void WritePalette()
        {
            using namespace xmlstrings;
            Poco::Path outpath = Poco::Path(m_outPath).append(SPRITE_Palette);
            ofstream   outfile( outpath.toString() );
            XMLWriter writer(outfile, XMLWriter::WRITE_XML_DECLARATION | XMLWriter::PRETTY_PRINT );
            
            InitWriter( writer, XML_NODE_PALLETTE );

            stringstream strs;
            strs <<"Total nb of color(s) : " <<setw(4) <<setfill(' ') <<m_inSprite.getPalette().size();
            writeComment( writer, strs.str() );
            strs = stringstream();

            for( unsigned int i = 0; i < m_inSprite.getPalette().size(); ++i )
            {
                const auto & acolor = m_inSprite.getPalette()[i];
                strs <<"#" <<i;
                writeComment( writer, strs.str() );
                strs = stringstream();

                writer.startElement("","", XML_NODE_COLOR );
                writer.dataElement ("","", XML_PROP_RED,   to_string( acolor.red )   );
                writer.dataElement ("","", XML_PROP_GREEN, to_string( acolor.green ) );
                writer.dataElement ("","", XML_PROP_BLUE,  to_string( acolor.blue )  );
                writer.endElement  ("","", XML_NODE_COLOR );
            }
            DeinitWriter( writer, XML_NODE_PALLETTE );
        }

        void WriteAnimFrame( Poco::XML::XMLWriter & writer, const AnimFrame & curfrm )
        {
            using namespace xmlstrings;

            writer.startElement("","", XML_NODE_ANIMFRM );
            {
                writer.dataElement ("","", XML_PROP_DURATION,  to_string( curfrm.frameDuration ) );
                writer.dataElement ("","", XML_PROP_METAINDEX, to_string( curfrm.metaFrmGrpIndex ) );

                writer.startElement("","",XML_NODE_SPRITE);
                {
                    writer.dataElement ("","", XML_PROP_OFFSETX, to_string( curfrm.sprOffsetX ) );
                    writer.dataElement ("","", XML_PROP_OFFSETY, to_string( curfrm.sprOffsetY ) );
                }
                writer.endElement("","",XML_NODE_SPRITE);

                writer.startElement("","",XML_NODE_SHADOW);
                {
                    writer.dataElement ("","", XML_PROP_OFFSETX, to_string( curfrm.shadowOffsetX ) );
                    writer.dataElement ("","", XML_PROP_OFFSETY, to_string( curfrm.shadowOffsetY ) );
                }
                writer.endElement("","",XML_NODE_SHADOW);
            }
            writer.endElement("","", XML_NODE_ANIMFRM );
        }

        void WriteAnimSequece( Poco::XML::XMLWriter & writer, const AnimationSequence & aseq )
        {
            using namespace xmlstrings;

            stringstream strs;

            //Give this sequence a name 
            AttributesImpl attr;
            attr.addAttribute("","", XML_ATTR_NAME, "string", aseq.getName() );
            writer.startElement("","", XML_NODE_ANIMSEQ, attr );

            //Write the content of each frame in that sequence
            for( unsigned int cptfrms = 0; cptfrms < aseq.getNbFrames(); ++cptfrms )
            {
                strs <<"frm " <<cptfrms;
                writeComment( writer, strs.str() );
                strs = stringstream();
                WriteAnimFrame(writer, aseq.getFrame(cptfrms));
            }

            writer.endElement("","", XML_NODE_ANIMSEQ );
        }

        //-pAnimNameList : First entry of heach sub-vector is the anim group name, anything after is anim sequence names!
        void WriteAnimations( uint32_t proportionofwork, uint32_t totalnbseqs, uint32_t totalnbfrms )
        {
            using namespace xmlstrings;
            Poco::Path outpath = Poco::Path(m_outPath).append(SPRITE_Animations);
            ofstream   outfile( outpath.toString() );
            XMLWriter  writer(outfile, XMLWriter::WRITE_XML_DECLARATION | XMLWriter::PRETTY_PRINT );
            uint32_t   saveprogress = 0;

            if( m_pProgresscnt != nullptr )
                saveprogress = m_pProgresscnt->load();



            InitWriter( writer, XML_NODE_ANIMDAT );

            stringstream strs;
            strs <<"Total nb of animation group(s) : " <<setw(4) <<setfill(' ') <<m_inSprite.getAnimGroups().size();
            writeComment( writer, strs.str() );
            strs = stringstream();

            strs <<"Total nb of sequence(s)        : " <<setw(4) <<setfill(' ') <<totalnbseqs;
            writeComment( writer, strs.str() );
            strs = stringstream();

            strs <<"Total nb of animation frame(s) : " <<setw(4) <<setfill(' ') <<totalnbfrms;
            writeComment( writer, strs.str() );
            strs = stringstream();

            {
                unsigned int cptgrp = 0;

                //Write the content of each group
                for( const auto & animgrp : m_inSprite.getAnimGroups() )
                {
                    unsigned int cptseq = 0;
                    strs <<"Group #" <<cptgrp <<" contains " << animgrp.sequences.size() << " sequence(s)";
                    writeComment( writer, strs.str() );
                    strs = stringstream();

                    //Give this group a name
                    AttributesImpl attrname;
                    attrname.addAttribute("", "", XML_ATTR_NAME, "string", animgrp.group_name);
                    writer.startElement("","", XML_NODE_ANIMGRP, attrname );

                    //Write the content of each sequences in that group
                    for( const auto & aseq : animgrp.sequences )
                    {
                        strs <<"Seq #" <<cptseq  <<" contains " <<aseq.getNbFrames()  << " frame(s)";
                        writeComment( writer, strs.str() );
                        strs = stringstream();
                        WriteAnimSequece( writer, aseq );
                        ++cptseq;
                    }

                    writer.endElement("","", XML_NODE_ANIMGRP );
                    ++cptgrp;

                    if( m_pProgresscnt != nullptr )
                    {
                        uint32_t prog = ( ( proportionofwork * cptgrp ) / m_inSprite.getAnimGroups().size() );
                        m_pProgresscnt->store( saveprogress + prog );
                    }
                }
            }
            DeinitWriter( writer, XML_NODE_ANIMDAT );
        }

        void WriteMetaFrameGroups( uint32_t proportionofwork )
        {
            using namespace xmlstrings;
            Poco::Path outpath = Poco::Path(m_outPath).append(SPRITE_Frames);
            ofstream   outfile( outpath.toString() );
            XMLWriter writer(outfile, XMLWriter::WRITE_XML_DECLARATION | XMLWriter::PRETTY_PRINT );

            uint32_t saveprogress = 0;

            if( m_pProgresscnt != nullptr )
                saveprogress = m_pProgresscnt->load();

            InitWriter( writer, XML_NODE_FRMLST );

            stringstream strs;
            strs <<"Total nb of group(s)      : " <<setw(4) <<setfill(' ') <<m_inSprite.getMetaFrmsGrps().size();
            writeComment( writer, strs.str() );
            strs = stringstream();
            strs <<"Total nb of meta-frame(s) : " <<setw(4) <<setfill(' ') <<m_inSprite.getMetaFrames().size();
            writeComment( writer, strs.str() );
            strs = stringstream();

            for( unsigned int i = 0; i < m_inSprite.getMetaFrmsGrps().size(); ++i )
            {
                const auto & grp = m_inSprite.getMetaFrmsGrps()[i];
                strs <<"MFG #" <<i <<" contains " <<grp.metaframes.size() <<" meta-frame(s)";
                writeComment( writer, strs.str() );
                strs = stringstream();

                WriteAMetaFrameGroup( writer, grp );

                if( m_pProgresscnt != nullptr )
                    m_pProgresscnt->store( saveprogress + ( proportionofwork * i ) / m_inSprite.getMetaFrmsGrps().size() );
            }

            DeinitWriter( writer, XML_NODE_FRMLST );
        }

        void WriteAMetaFrameGroup( XMLWriter & writer, const MetaFrameGroup & grp )
        {
            using namespace xmlstrings;
            writer.startElement("","", XML_NODE_FRMGRP );
            stringstream strs;


            for( unsigned int i = 0; i < grp.metaframes.size(); ++i )
            {
                const auto & aframe = grp.metaframes[i];
                strs <<i;
                writeComment( writer, strs.str() );
                strs = stringstream();

                WriteMetaFrame( writer, aframe );
            }

            writer.endElement("","", XML_NODE_FRMGRP );
        }

        void WriteMetaFrame( XMLWriter & writer, unsigned int index )
        {
            using namespace xmlstrings;
            const auto & aframe = m_inSprite.getMetaFrames()[index];

            writer.startElement("","", XML_NODE_FRMFRM );
            {
                writer.dataElement( "", "", XML_PROP_FRMINDEX, to_string( aframe.imageIndex ) );
                writer.dataElement( "", "", XML_PROP_UNK0,     turnIntToHexStr( aframe.unk0 ) );
                writer.dataElement( "", "", XML_PROP_OFFSETY,  to_string( aframe.offsetY ) );
                writer.dataElement( "", "", XML_PROP_OFFSETX,  to_string( aframe.offsetX ) );
                writer.dataElement( "", "", XML_PROP_UNK1,     turnIntToHexStr( aframe.unk1 ) );

                writer.startElement("","", XML_NODE_RES );
                {
                    auto resolution = MetaFrame::eResToResolution(aframe.resolution);
                    writer.dataElement( "", "", XML_PROP_WIDTH,  to_string( resolution.width ) );
                    writer.dataElement( "", "", XML_PROP_HEIGTH, to_string( resolution.height ) );
                }
                writer.endElement("","", XML_NODE_RES );

                writer.dataElement( "", "", XML_PROP_VFLIP,  to_string( aframe.vFlip ) );
                writer.dataElement( "", "", XML_PROP_HFLIP,  to_string( aframe.hFlip ) );
                writer.dataElement( "", "", XML_PROP_MOSAIC, to_string( aframe.Mosaic ) );
                writer.dataElement( "", "", XML_PROP_OFFXBIT5, to_string( aframe.XOffbit5 ) );
                writer.dataElement( "", "", XML_PROP_OFFXBIT6, to_string( aframe.XOffbit6 ) );
                writer.dataElement( "", "", XML_PROP_OFFXBIT7, to_string( aframe.XOffbit7 ) );

                writer.dataElement( "", "", XML_PROP_OFFYBIT3, to_string( aframe.YOffbit3 ) );
                writer.dataElement( "", "", XML_PROP_OFFYBIT5, to_string( aframe.YOffbit5 ) );
                writer.dataElement( "", "", XML_PROP_OFFYBIT6, to_string( aframe.YOffbit6 ) );
            }
            writer.endElement("","", XML_NODE_FRMFRM );
        }

        void WriteOffsets( uint32_t proportionofwork )
        {
            using namespace Poco::XML;
            using namespace xmlstrings;
            Poco::Path outpath = Poco::Path(m_outPath).append(SPRITE_Offsets);
            ofstream   outfile( outpath.toString() );
            XMLWriter writer(outfile, XMLWriter::WRITE_XML_DECLARATION | XMLWriter::PRETTY_PRINT );

            InitWriter( writer, XML_NODE_OFFLST );

            stringstream strs;
            strs <<"Total nb of offset(s) : " <<setw(4) <<setfill(' ') <<m_inSprite.getPartOffsets().size();
            writeComment( writer, strs.str() );
            strs = stringstream();

            uint32_t saveprogress = 0;

            if( m_pProgresscnt != nullptr )
                saveprogress = m_pProgresscnt->load();

            for( unsigned int i = 0; i < m_inSprite.getPartOffsets().size(); ++i )
            {
                const auto & anoffset = m_inSprite.getPartOffsets()[i];
                strs <<"#" <<i;
                writeComment( writer, strs.str() );
                strs = stringstream();

                writer.startElement("","", XML_NODE_OFFSET );
                {
                    writer.dataElement("","", XML_PROP_X, to_string( anoffset.offx ) );
                    writer.dataElement("","", XML_PROP_Y, to_string( anoffset.offy ) );
                }
                writer.endElement("","", XML_NODE_OFFSET );


                if( m_pProgresscnt != nullptr )
                    m_pProgresscnt->store( saveprogress + ( proportionofwork * i ) / m_inSprite.getPartOffsets().size() );
            }

            DeinitWriter( writer, XML_NODE_OFFLST );
        }

    private:
        Poco::Path              m_outPath;
        const sprite_t        & m_inSprite;
        std::atomic<uint32_t> * m_pProgresscnt;
        uint32_t                m_progressProportion; //The percentage of the entire work attributed to this
    };

//=============================================================================================
//  Sprite to SpriteToDirectory Writer
//=============================================================================================
    template<class _SPRITE_t>
        class SpriteToDirectory
    {
    public:
        typedef _SPRITE_t sprite_t;


        

        SpriteToDirectory( const sprite_t & myspr )
            :m_inSprite(myspr),m_pProgress(nullptr)
        {
        }

        void WriteSpriteToDir( const string          & folderpath, 
                               eSUPPORT_IMG_IO         imgty, 
                               bool                    xmlcolorpal = false, 
                               std::atomic<uint32_t> * progresscnt = nullptr ) 
        {
            //Create Root Folder
            m_outDirPath = Poco::Path(folderpath);
            Poco::File directory(m_outDirPath);

            if( !directory.exists() )
                directory.createDirectory();

            m_pProgress = progresscnt;

            //Count the ammount of entries for calculating work
            // and for adding statistics to the exported files.
            uint32_t totalnbseqs  = 0;
            uint32_t totalnbfrms  = 0;

            for( const auto & agrp : m_inSprite.getAnimGroups() )
            {
                if( ! ( agrp.sequences.empty() ) )
                {
                    totalnbseqs += agrp.sequences.size();
                    for( const auto & aseq : agrp.sequences )
                        totalnbfrms += aseq.getNbFrames();
                }
            }
            
            //Gather stats for computing progress proportionally at runtime
            const uint32_t amtWorkFrames  = m_inSprite.getFrames().size();
            const uint32_t amtWorkAnims   = m_inSprite.getAnimGroups().size() + totalnbseqs + totalnbfrms;
            const uint32_t amtWorkFrmGrps = m_inSprite.getMetaFrames().size() + m_inSprite.getMetaFrmsGrps().size();
            const uint32_t amtWorkOffs    = m_inSprite.getPartOffsets().size();
            const uint32_t totalwork      = amtWorkAnims + amtWorkFrmGrps + amtWorkOffs + amtWorkFrames;

            //Get the percentages of work, relative to the total, for each
            const float percentFrames  = ( (static_cast<double>(amtWorkFrames)  * 100.0) / static_cast<double>(totalwork) );
            const float percentAnims   = ( (static_cast<double>(amtWorkAnims)   * 100.0) / static_cast<double>(totalwork) );
            const float percentFrmGrps = ( (static_cast<double>(amtWorkFrmGrps) * 100.0) / static_cast<double>(totalwork) );
            const float percentOffsets = ( (static_cast<double>(amtWorkOffs)    * 100.0) / static_cast<double>(totalwork) );

            workstatistics stats
            {
                static_cast<uint32_t>(percentFrames),   //uint32_t propFrames;
                static_cast<uint32_t>(percentAnims),    //uint32_t propAnims;
                static_cast<uint32_t>(percentFrmGrps),  //uint32_t propMFrames;
                static_cast<uint32_t>(percentOffsets),  //uint32_t propOffsets;
                totalnbfrms,                            //uint32_t totalAnimFrms;
                totalnbseqs,                            //uint32_t totalAnimSeqs;
            };

            ExportFrames(imgty, stats.propFrames );

            if( !xmlcolorpal )
                ExportPalette();

            ExportXMLData(xmlcolorpal, stats);

            if( m_pProgress != nullptr )
                m_pProgress->store( 100 ); //fill the remaining percentage (the palette and etc are nearly instantenous anyways)
        }

    private:

        void ExportFrames( eSUPPORT_IMG_IO imgty, uint32_t proportionofwork )
        {
            Poco::Path imgdir = Poco::Path(m_outDirPath);
            imgdir.append(SPRITE_IMGs_DIR);
            Poco::File directory(imgdir);

            if( !directory.exists() )
                directory.createDirectory();

            switch( imgty )
            {
                case eSUPPORT_IMG_IO::BMP:
                {
                    ExportFramesAsBMPs(imgdir, proportionofwork);
                    break;
                }
                case eSUPPORT_IMG_IO::RAW:
                {
                    ExportFramesAsRawImgs(imgdir, proportionofwork);
                    break;
                }
                case eSUPPORT_IMG_IO::PNG:
                default:
                {
                    ExportFramesAsPNGs(imgdir, proportionofwork);
                }
            };
        }

        void ExportFramesAsPNGs( const Poco::Path & outdirpath, uint32_t proportionofwork )
        {
            uint32_t     progressBefore = 0; //Save a little snapshot of the progress
            const auto & frames         = m_inSprite.getFrames();
            Poco::Path   outimg(outdirpath);

            if(m_pProgress!=nullptr)
                progressBefore = m_pProgress->load();

            for( unsigned int i = 0; i < frames.size(); ++i )
            {
                std::stringstream sstrname;
                //Build filenmame
                sstrname <<setw(4) <<setfill('0') <<i <<"." <<utils::io::PNG_FileExtension;
                //Export
                utils::io::ExportToPNG( frames[i], Poco::Path(outimg).append(sstrname.str()).toString() );

                if( m_pProgress != nullptr )
                    m_pProgress->store( progressBefore + (proportionofwork * (i+1) ) / frames.size() ); 
            }
        }

        void ExportFramesAsBMPs( const Poco::Path & outdirpath, uint32_t proportionofwork )
        {
            uint32_t     progressBefore = 0; //Save a little snapshot of the progress
            const auto & frames         = m_inSprite.getFrames();
            Poco::Path   outimg(outdirpath);
            

            if(m_pProgress!=nullptr)
                progressBefore = m_pProgress->load();

            for( unsigned int i = 0; i < frames.size(); ++i )
            {
                std::stringstream sstrname;
                //Build filenmame
                sstrname <<setw(4) <<setfill('0') <<i <<"." <<utils::io::BMP_FileExtension;
                //Export
                utils::io::ExportToBMP( frames[i], Poco::Path(outimg).append(sstrname.str()).toString() );

                if( m_pProgress != nullptr )
                    m_pProgress->store( progressBefore + (proportionofwork * (i+1)) / frames.size() );
            }
        }

        void ExportFramesAsRawImgs( const Poco::Path & outdirpath, uint32_t proportionofwork )
        {
            uint32_t     progressBefore = 0; 
            const auto & frames         = m_inSprite.getFrames();
            Poco::Path   outimg(outdirpath);
            

            if(m_pProgress!=nullptr)
                progressBefore = m_pProgress->load(); //Save a little snapshot of the progress

            for( unsigned int i = 0; i < frames.size(); ++i )
            {
                std::stringstream sstrname;
                //Build filenmame
                sstrname <<setw(4) <<setfill('0') <<i <<"." <<utils::io::RawImg_FileExtension;
                //Export
                utils::io::ExportRawImg_NoPal( frames[i], Poco::Path(outimg).append(sstrname.str()).toString() );

                if( m_pProgress != nullptr )
                    m_pProgress->store( progressBefore + (proportionofwork * (i + 1)) / frames.size() );
            }
        }

        void ExportXMLData(bool xmlcolpal, const workstatistics & wstats)
        {
            SpriteToXML<sprite_t> mywriter(m_inSprite);
            mywriter.WriteXMLFiles( m_outDirPath.toString(), wstats, xmlcolpal, m_pProgress );
        }

        void ExportPalette()
        {
            utils::io::ExportTo_RIFF_Palette( m_inSprite.getPalette(), 
                                              Poco::Path(m_outDirPath).append(Palette_Filename).toString() );
        }


    private:
        Poco::Path              m_outDirPath;
        const sprite_t        & m_inSprite;
        std::atomic<uint32_t> * m_pProgress;
    };

//=============================================================================================
//  SpriteXMLParser
//=============================================================================================

    /*
        SpriteXMLParser
            Reads the xml files and put them in the appropriate data structures.
    */
    template<class _SPRITE_t>
        class SpriteXMLParser
    {
    public:
        typedef _SPRITE_t sprite_t;

        SpriteXMLParser(sprite_t & out_spr)
            :m_outSprite(out_spr)
        {
        }

        /*
            ParseXML
        */
        void ParseXML( Poco::Path sprrootfolder, bool parsexmlpal )
        {
            using namespace xmlstrings;
            std::ifstream inProperties( Poco::Path(sprrootfolder).append(SPRITE_Properties).toString() );
            std::ifstream inAnims     ( Poco::Path(sprrootfolder).append(SPRITE_Animations).toString() );
            std::ifstream inMFrames   ( Poco::Path(sprrootfolder).append(SPRITE_Frames    ).toString() );
            std::ifstream inOffsets   ( Poco::Path(sprrootfolder).append(SPRITE_Offsets   ).toString() );

            ParseSpriteInfo(inProperties);
            ParseAnimations(inAnims     );
            ParseMetaFrames(inMFrames   );
            ParseOffsets   (inOffsets   );

            if(parsexmlpal)
            {
                std::ifstream inPal( Poco::Path(sprrootfolder).append(SPRITE_Palette).toString() );
                ParsePalette(inPal);
            }

        }

    private:

        //A simple little struct used very briefly to make things more readable.
        struct tmpoffset
        {
            uint16_t x,y;
        };

        /*
            Used to parse the 2 offsets contained in an anim frame's xml data's child node!
        */
        tmpoffset ParseAnimaFrmOffsets( Poco::XML::NodeList * pNodeLst )
        {
            tmpoffset         res       = {0,0};
            AutoPtr<NodeList> pOffNodes = pNodeLst;

            for( unsigned long ctoff = 0; ctoff < pOffNodes->length(); ++ctoff )
            {
                Node * pCurOffnode = pOffNodes->item(ctoff);

                if( pCurOffnode->nodeName() == xmlstrings::XML_PROP_OFFSETX )
                        _parseXMLHexaValToValue( pCurOffnode->nodeValue(), res.x );
                else if( pCurOffnode->nodeName() == xmlstrings::XML_PROP_OFFSETY )
                    _parseXMLHexaValToValue( pCurOffnode->nodeValue(), res.y );
            }

            return std::move(res);
        }

        /*
            ParseAnimationFrame
        */
        AnimFrame ParseAnimationFrame( Poco::XML::Node * pFrmNode )
        {
            AnimFrame         afrm;
            AutoPtr<NodeList> pFrmProp = pFrmNode->childNodes();
            
            for( unsigned long ctfrmprop = 0; ctfrmprop < pFrmProp->length(); ++ctfrmprop )
            {
                Node * pCurProp = pFrmProp->item( ctfrmprop );

                if( pCurProp->nodeName() == xmlstrings::XML_PROP_DURATION )
                {
                    _parseXMLHexaValToValue( pCurProp->nodeValue(), afrm.frameDuration );
                }
                else if( pCurProp->nodeName() == xmlstrings::XML_PROP_METAINDEX )
                    _parseXMLHexaValToValue( pCurProp->nodeValue(), afrm.metaFrmGrpIndex );
                else if( pCurProp->nodeName() == xmlstrings::XML_NODE_SPRITE && pCurProp->hasChildNodes() )
                {
                    tmpoffset result = ParseAnimaFrmOffsets( pCurProp->childNodes() );
                    afrm.sprOffsetX = result.x;
                    afrm.sprOffsetY = result.y;
                }
                else if( pCurProp->nodeName() == xmlstrings::XML_NODE_SHADOW && pCurProp->hasChildNodes() )
                {
                    tmpoffset result = ParseAnimaFrmOffsets( pCurProp->childNodes() );
                    afrm.shadowOffsetX = result.x;
                    afrm.shadowOffsetY = result.y;
                }
            }

            return std::move(afrm);
        }

        /*
            ParseAnimationSequence
        */
        AnimationSequence ParseAnimationSequence( Poco::XML::Node * pASeq )
        {
            AnimationSequence aseq;

            //Get the name if applicable. If not in the attributes will set empty string
            if( pASeq->hasAttributes() )
            {
                aseq.setName( _parseNameAttribute( pASeq->attributes() ) );
            }

            if( pASeq->hasChildNodes() )
            {
                AutoPtr<NodeList> pSeqFrms = pASeq->childNodes();
                for( unsigned long ctfrms = 0; ctfrms < pSeqFrms->length(); ++ctfrms )
                {
                    Node * pCurFrm = pSeqFrms->item(ctfrms);

                    if( pCurFrm->nodeName() == xmlstrings::XML_NODE_ANIMFRM && pCurFrm->hasChildNodes() )
                    {   
                        aseq.insertFrame( ParseAnimationFrame( pCurFrm ) );
                    }
                }
            }

            return std::move(aseq);
        }

        /*
            ParseAnimationSequences
        */
        vector<AnimationSequence> ParseAnimationSequences( Poco::XML::NodeList * pNodeLst )
        {
            vector<AnimationSequence> seqs;
            AutoPtr<NodeList>         pSeqs = pNodeLst;

            for( unsigned long ctseqs = 0; ctseqs < pNodeLst->length(); ++ctseqs )
            {
                Node * pCurSeq = pNodeLst->item(ctseqs);

                if( pCurSeq->nodeName() == xmlstrings::XML_NODE_ANIMSEQ )
                {
                    seqs.push_back( ParseAnimationSequence( pCurSeq ) );
                }
            }

            if( seqs.empty() )
                seqs.resize(1); //We must have at least a single empty sequence per groups

            return std::move(seqs);
        }

        /*
            ParseAnimations
        */
        void ParseAnimations(std::ifstream & in)
        {
            using namespace Poco::XML;
            InputSource       src(in);
            DOMParser         parser;
            AutoPtr<Document> pAnimGrps = parser.parse(&src);
            NodeIterator      itnode( pAnimGrps, NodeFilter::SHOW_ATTRIBUTE | NodeFilter::SHOW_ELEMENT | NodeFilter::SHOW_TEXT );
            Node             *pCurNode = itnode.nextNode(); //Get the first node loaded

            //Read every elements
            while( pCurNode != nullptr )
            {
                if( pCurNode->nodeName() == xmlstrings::XML_NODE_ANIMGRP && pCurNode->hasChildNodes() )
                {
                    SpriteAnimationGroup agrp;

                    //Get the name
                    if( pCurNode->hasAttributes() )
                        agrp.group_name = _parseNameAttribute( pCurNode->attributes() );

                    //Parse the sequences
                    agrp.sequences = ParseAnimationSequences( pCurNode->childNodes() );

                    m_outSprite.m_animgroups.push_back( agrp );
                }
                pCurNode = itnode.nextNode();
            }
        }

        /*
            ParseOffsets
        */
        void ParseOffsets(std::ifstream & in)
        {
            using namespace Poco::XML;
            InputSource       src(in);
            DOMParser         parser;
            AutoPtr<Document> pOffsets = parser.parse(&src);
            NodeIterator      itnode( pOffsets, NodeFilter::SHOW_ATTRIBUTE | NodeFilter::SHOW_ELEMENT | NodeFilter::SHOW_TEXT );
            Node             *pCurNode = itnode.nextNode(); //Get the first node loaded

            //Read every elements
            while( pCurNode != nullptr )
            {
                if( pCurNode->nodeName() == xmlstrings::XML_NODE_OFFSET && pCurNode->hasChildNodes() )
                {
                    AutoPtr<NodeList> offsetsndl = pCurNode->childNodes();
                    sprOffParticle    anoffs;

                    for( unsigned long ctoffc = 0; ctoffc < offsetsndl->length(); ++ctoffc )
                    {
                        Node * pOffNode = offsetsndl->item(ctoffc);
                        if( pOffNode->nodeName() == xmlstrings::XML_PROP_X )
                            _parseXMLHexaValToValue( pOffNode->nodeValue(), anoffs.offx );
                        else if( pOffNode->nodeName() == xmlstrings::XML_PROP_Y )
                            _parseXMLHexaValToValue( pOffNode->nodeValue(), anoffs.offy );
                    }
                    m_outSprite.m_partOffsets.push_back(anoffs);
                }
                pCurNode = itnode.nextNode();
            }
        }
        
        /*
            ParseAMetaFrame
        */
        MetaFrame ParseAMetaFrame( Poco::XML::Node * pFrmNode )
        {
            MetaFrame         mf;
            AutoPtr<NodeList> nodelst = pFrmNode->childNodes();

            for( unsigned long ctmfprop = 0; ctmfprop < nodelst->length(); ++ctmfprop )
            {
                Node * pPropNode = nodelst->item(ctmfprop);

                if( pPropNode->nodeName() == xmlstrings::XML_PROP_FRMINDEX )
                {
                    _parseXMLHexaValToValue( pPropNode->nodeValue(), mf.imageIndex );
                }
                else if( pPropNode->nodeName() == xmlstrings::XML_PROP_UNK0 )
                    _parseXMLHexaValToValue( pPropNode->nodeValue(), mf.unk0 );
                else if( pPropNode->nodeName() == xmlstrings::XML_PROP_OFFSETX )
                    _parseXMLHexaValToValue( pPropNode->nodeValue(), mf.offsetX );
                else if( pPropNode->nodeName() == xmlstrings::XML_PROP_OFFSETY )
                    _parseXMLHexaValToValue( pPropNode->nodeValue(), mf.offsetY );
                else if( pPropNode->nodeName() == xmlstrings::XML_PROP_UNK1 )
                    _parseXMLHexaValToValue( pPropNode->nodeValue(), mf.unk1 );
                else if( pPropNode->nodeName() == xmlstrings::XML_PROP_VFLIP )
                    _parseXMLHexaValToValue( pPropNode->nodeValue(), mf.vFlip );
                else if( pPropNode->nodeName() == xmlstrings::XML_PROP_HFLIP )
                    _parseXMLHexaValToValue( pPropNode->nodeValue(), mf.hFlip );
                else if( pPropNode->nodeName() == xmlstrings::XML_PROP_MOSAIC )
                    _parseXMLHexaValToValue( pPropNode->nodeValue(), mf.Mosaic );
                else if( pPropNode->nodeName() == xmlstrings::XML_PROP_OFFXBIT5 )
                    _parseXMLHexaValToValue( pPropNode->nodeValue(), mf.XOffbit5 );
                else if( pPropNode->nodeName() == xmlstrings::XML_PROP_OFFXBIT6 )
                    _parseXMLHexaValToValue( pPropNode->nodeValue(), mf.XOffbit6 );
                else if( pPropNode->nodeName() == xmlstrings::XML_PROP_OFFXBIT7 )
                    _parseXMLHexaValToValue( pPropNode->nodeValue(), mf.XOffbit7 );
                else if( pPropNode->nodeName() == xmlstrings::XML_PROP_OFFYBIT3 )
                    _parseXMLHexaValToValue( pPropNode->nodeValue(), mf.YOffbit3 );
                else if( pPropNode->nodeName() == xmlstrings::XML_PROP_OFFYBIT5 )
                    _parseXMLHexaValToValue( pPropNode->nodeValue(), mf.YOffbit5 );
                else if( pPropNode->nodeName() == xmlstrings::XML_PROP_OFFYBIT6 )
                    _parseXMLHexaValToValue( pPropNode->nodeValue(), mf.YOffbit6 );
                else if( pPropNode->nodeName() == xmlstrings::XML_NODE_RES && pPropNode->hasChildNodes() )
                {
                    AutoPtr<NodeList> resnodelst = pPropNode->childNodes();
                    utils::Resolution myres={0,0};

                    for( unsigned long ctres = 0; ctres < resnodelst->length(); ++ctres  )
                    {
                        Node * pResChild = resnodelst->item(ctres);
                        if( pResChild->nodeName() == xmlstrings::XML_PROP_WIDTH )
                            _parseXMLHexaValToValue( pResChild->nodeValue(), myres.width );
                        else if( pResChild->nodeName() == xmlstrings::XML_PROP_HEIGTH )
                            _parseXMLHexaValToValue( pResChild->nodeValue(), myres.height );
                    }

                    mf.resolution = MetaFrame::IntegerResTo_eRes( myres.width, myres.height );
                }
            }

            return std::move(mf);
        }

        /*
            ParseMetaFrames
        */
        void ParseMetaFrames(std::ifstream & in)
        {
            using namespace Poco::XML;
            InputSource       src(in);
            DOMParser         parser;
            AutoPtr<Document> pMetaFrms = parser.parse(&src);
            NodeIterator      itnode( pMetaFrms, NodeFilter::SHOW_ATTRIBUTE | NodeFilter::SHOW_ELEMENT | NodeFilter::SHOW_TEXT );
            Node             *pCurNode = itnode.nextNode(); //Get the first node loaded

            //Read every elements
            while( pCurNode != nullptr )
            {
                //Get a frame group first !
                if( pCurNode->nodeName() == xmlstrings::XML_NODE_FRMGRP )
                {
                    if( pCurNode->hasChildNodes() )
                    {
                        AutoPtr<NodeList> nodelst = pCurNode->childNodes();
                        MetaFrameGroup mfg; //Create the meta-frame group
                        mfg.metaframes.reserve( nodelst->length() );

                        //Read all frames in the group
                        for( unsigned long ctmf = 0; ctmf < nodelst->length(); ++ctmf )
                        {
                            if( nodelst->item(ctmf)->nodeName() == xmlstrings::XML_NODE_FRMFRM && nodelst->item(ctmf)->hasChildNodes() )
                            {
                                mfg.metaframes.push_back( m_outSprite.m_metaframes.size() ); //put the frame's offset in the meta frame group table
                                m_outSprite.m_metaframes.push_back( ParseAMetaFrame( nodelst->item(ctmf) ) );
                            }
                        }

                        m_outSprite.m_metafrmsgroups.push_back(mfg);
                    }
                    else
                        assert(false); //What to do with frm groups with no childs ? //#TODO: do something with this !!
                }
                pCurNode = itnode.nextNode();
            }
        }

        /*
            ParseSpriteInfo
        */
        void ParseSpriteInfo(std::ifstream & in)
        {
            using namespace xmlstrings;
            using namespace Poco::XML;
            InputSource       src(in);
            DOMParser         parser;
            AutoPtr<Document> pSprinfo = parser.parse(&src);
            NodeIterator      itnode( pSprinfo, NodeFilter::SHOW_ATTRIBUTE | NodeFilter::SHOW_ELEMENT | NodeFilter::SHOW_TEXT );
            Node             *pCurNode = itnode.nextNode(); //Get the first node loaded

            //Read every elements
            while( pCurNode != nullptr )
            {
                if( pCurNode->nodeName() == XML_PROP_UNK3 )
                {
                    _parseXMLHexaValToValue( pCurNode->getNodeValue(), m_outSprite.m_common.m_Unk3 );
                }
                else if( pCurNode->nodeName() == XML_PROP_COLPERROW )
                    _parseXMLHexaValToValue( pCurNode->getNodeValue(), m_outSprite.m_common.m_nbColorsPerRow );
                else if( pCurNode->nodeName() == XML_PROP_UNK4 )
                    _parseXMLHexaValToValue( pCurNode->getNodeValue(), m_outSprite.m_common.m_Unk4 );
                else if( pCurNode->nodeName() == XML_PROP_UNK5 )
                    _parseXMLHexaValToValue( pCurNode->getNodeValue(), m_outSprite.m_common.m_Unk5 );
                else if( pCurNode->nodeName() == XML_PROP_UNK6 )
                    _parseXMLHexaValToValue( pCurNode->getNodeValue(), m_outSprite.m_common.m_Unk6 );
                else if( pCurNode->nodeName() == XML_PROP_UNK7 )
                    _parseXMLHexaValToValue( pCurNode->getNodeValue(), m_outSprite.m_common.m_Unk7 );
                else if( pCurNode->nodeName() == XML_PROP_UNK8 )
                    _parseXMLHexaValToValue( pCurNode->getNodeValue(), m_outSprite.m_common.m_Unk8 );
                else if( pCurNode->nodeName() == XML_PROP_UNK9 )
                    _parseXMLHexaValToValue( pCurNode->getNodeValue(), m_outSprite.m_common.m_Unk9 );
                else if( pCurNode->nodeName() == XML_PROP_UNK10 )
                    _parseXMLHexaValToValue( pCurNode->getNodeValue(), m_outSprite.m_common.m_Unk10 );
                else if( pCurNode->nodeName() == XML_PROP_IS8W )
                    _parseXMLHexaValToValue( pCurNode->getNodeValue(), m_outSprite.m_common.m_is8WaySprite );
                else if( pCurNode->nodeName() == XML_PROP_IS256COL )
                    _parseXMLHexaValToValue( pCurNode->getNodeValue(), m_outSprite.m_common.m_is256Sprite );
                else if( pCurNode->nodeName() == XML_PROP_ISMOSAICS )
                    _parseXMLHexaValToValue( pCurNode->getNodeValue(), m_outSprite.m_common.m_IsMosaicSpr );
                else if( pCurNode->nodeName() == XML_PROP_UNK11 )
                    _parseXMLHexaValToValue( pCurNode->getNodeValue(), m_outSprite.m_common.m_Unk11 );
                else if( pCurNode->nodeName() == XML_PROP_UNK12 )
                    _parseXMLHexaValToValue( pCurNode->getNodeValue(), m_outSprite.m_common.m_Unk11 );

                pCurNode = itnode.nextNode();
            }
        }

        /*
            ParsePalette
        */
        void ParsePalette(std::ifstream & in)
        {
            using namespace Poco::XML;
            InputSource       src(in);
            DOMParser         parser;
            AutoPtr<Document> pPalette = parser.parse(&src);
            NodeIterator      itnode( pPalette, NodeFilter::SHOW_ATTRIBUTE | NodeFilter::SHOW_ELEMENT | NodeFilter::SHOW_TEXT );
            Node             *pCurNode = itnode.nextNode(); //Get the first node loaded

            //Read every elements
            while( pCurNode != nullptr )
            {
                if( pCurNode->nodeName() == xmlstrings::XML_NODE_COLOR )
                {
                    graphics::colRGB24 mycolor;
                    if( pCurNode->hasChildNodes() )
                    {
                        AutoPtr<NodeList> nodelst = pCurNode->childNodes();

                        //Copy the content of the color
                        for( unsigned long ctchn = 0; ctchn < nodelst->length(); ++ctchn )
                        {
                            Node * pChild = nodelst->item(ctchn);
                            if( pChild->nodeName() == xmlstrings::XML_PROP_RED )
                                _parseXMLHexaValToValue( pChild->nodeValue(), mycolor.red ); 
                            else if( pChild->nodeName() == xmlstrings::XML_PROP_GREEN )
                                _parseXMLHexaValToValue( pChild->nodeValue(), mycolor.green ); 
                            else if( pChild->nodeName() == xmlstrings::XML_PROP_BLUE )
                                _parseXMLHexaValToValue( pChild->nodeValue(), mycolor.blue ); 
                        }
                    }
                    m_outSprite.m_palette.push_back(mycolor);
                }
                pCurNode = itnode.nextNode();
            }
        }

        //Variables
        sprite_t & m_outSprite;
    };




//=============================================================================================
//  Sprite Builder
//=============================================================================================
    template<class _SPRITE_t>
        class DirectoryToSprite
    {
    public:
        typedef _SPRITE_t sprite_t;

        /*
            DirectoryToSprite
        */
        DirectoryToSprite( sprite_t & out_sprite )
            :m_pProgress( nullptr ), m_outSprite( out_sprite )
        {
        }

        /*
            ParseSpriteFromDirectory
        */
        void ParseSpriteFromDirectory( const std::string     & directorypath, 
                                       bool                    readImgByIndex, //If true, the images are read by index number in their names. If false, by alphanumeric order!
                                       std::atomic<uint32_t> * pProgress   = nullptr,
                                       bool                    parsexmlpal = false )
        {
            m_inDirPath = Poco::Path( directorypath );
            m_pProgress = pProgress;

            //Parse the xml first to help with reading image with some formats
            ParseXML(parsexmlpal);

            if( readImgByIndex )
                ReadImagesByIndex();
            else
                ReadImagesSorted();

            //End with rebuilding the references
            m_outSprite.RebuildAllReferences();
        }

    private:

        /*
            ParseXML
        */
        void ParseXML( bool parsexmlpal )
        {
            SpriteXMLParser<sprite_t> myparser( m_outSprite );
            myparser.ParseXML( m_inDirPath, parsexmlpal );
        }

        /*
            ReadImages
        */
        void ReadImagesByIndex()
        {
            //assert(false);
            //**************************************************************************
            //#TODO: Use the meta-frame table to read image files by index/filenmae. Then change the meta-frame index in-memory to 
            //       refer to the index the image data was actually inserted at!
            //**************************************************************************
            Poco::Path                imgsDir( m_inDirPath );
            map<uint32_t, Poco::File> validImages;
            uint32_t                  nbvalidimgs  = 0;
            imgsDir.append( SPRITE_IMGs_DIR );

            Poco::DirectoryIterator itdir(imgsDir);
            Poco::DirectoryIterator itdircount(imgsDir);
            Poco::DirectoryIterator itdirend;

            //count imgs
            //while( itdircount != itdirend )
            //{
            //    if( utils::io::IsSupportedImageType( itdircount->path() ) )
            //        ++nbvalidimgs;
            //    ++itdircount;
            //}

            //
            //validImages.resize(nbvalidimgs);

            //We want to load images into the right indexes

            //
            //#1 - Count the images in the imgs directory + Find all our valid images
            for(; itdir != itdirend; ++itdir ) 
            {
                if( utils::io::IsSupportedImageType( itdir->path() ) )
                {
                    stringstream sstrparseindex(itdir->path());
                    uint32_t     curindex = 0;
                    sstrparseindex >> curindex;

                    validImages.insert( make_pair( curindex, *itdir ) );
                }
            }


            //#2 - Parse the images after allocating
            m_outSprite.m_frames.reserve( validImages.size() );

            for( uint32_t i = 0; i < validImages.size(); ++i )
            {
                auto & animg = validImages.at(i);
                try
                {
                    ReadAnImage( animg );
                }
                catch( std::out_of_range ore ) //In this case it means the std::map didn't find our entry
                {
                    cerr << "\n<!>-Warning: Image #" <<i <<" was expected, but not found !\n"
                        << "The next image read will end up with that image# ! This might result in unforseen consequences!\n";
                }
                catch( Poco::Exception e )
                {
                    cerr << "\n<!>-Warning: Failure reading image " <<animg.path() <<":\n"
                         <<e.message() <<"\n"
                         <<"Skipping !\n";
                }
                catch( exception e )
                {
                    cerr << "\n<!>-Warning: Failure reading image " <<animg.path() <<":\n"
                         <<e.what() <<"\n"
                         <<"Skipping !\n";
                }
            }
        }


        void ReadImagesSorted()
        {
            Poco::Path         imgsDir( m_inDirPath );
            uint32_t           nbvalidimgs  = 0;
            imgsDir.append( SPRITE_IMGs_DIR );

            Poco::DirectoryIterator itdir(imgsDir);
            Poco::DirectoryIterator itdircount(imgsDir);
            Poco::DirectoryIterator itdirend;

            //count imgs
            while( itdircount != itdirend )
            {
                if( utils::io::IsSupportedImageType( itdircount->path() ) )
                    ++nbvalidimgs;
                ++itdircount;
            }

            //Allocate
            m_outSprite.m_frames.reserve( nbvalidimgs );

            //Grab the images in order
            for(; itdir != itdirend; ++itdir )  
            {
                if( utils::io::IsSupportedImageType( itdir->path() ) )
                {
                    try
                    {
                        ReadAnImage( *itdir );
                    }
                    catch( Poco::Exception e )
                    {
                        cerr << "\n<!>-Warning: Failure reading image " <<itdir->path() <<":\n"
                             <<e.message() <<"(POCO)\n"
                             <<"Skipping !\n";
                    }
                    catch( exception e )
                    {
                        cerr << "\n<!>-Warning: Failure reading image " <<itdir->path() <<":\n"
                             <<e.what() <<"\n"
                             <<"Skipping !\n";
                    }
                }
            }
        }

        /*
            ReadAnImage
        */
        void ReadAnImage( const Poco::File & imgfile )
        {
            Poco::Path               imgpath(imgfile.path());
            typename sprite_t::img_t curfrm;

            //Proceed to validate the file and find out what to use to handle it!
            switch( utils::io::GetSupportedImageType( imgpath.getFileName() ) )
            {
                case eSUPPORT_IMG_IO::PNG:
                {
                    utils::io::ImportFromPNG( curfrm, imgfile.path() );
                    break;
                }
                case eSUPPORT_IMG_IO::BMP:
                {
                    utils::io::ImportFromBMP( curfrm, imgfile.path() );
                    break;
                }
                case eSUPPORT_IMG_IO::RAW:
                {
                    utils::Resolution res{0,0};
                    const uint32_t    indextofind = m_outSprite.m_frames.size(); //This is where we'll insert this image.

                    //We have to find a metaframe with the right index. Or a 0xFFFF meta-frame.
                    for( unsigned int i = 0; i < m_outSprite.m_metaframes.size(); ++i )
                    {
                        if( m_outSprite.m_metaframes[i].imageIndex == indextofind )
                        {
                            res = MetaFrame::eResToResolution(m_outSprite.m_metaframes[i].resolution);
                        }
                        else if( m_outSprite.m_metaframes[i].imageIndex == SPRITE_SPECIAL_METAFRM_INDEX ) //For ui sprites just get the 0xFFFF meta frame
                        {
                            res = MetaFrame::eResToResolution(m_outSprite.m_metaframes[i].resolution);
                        }
                    }

                    stringstream pathtoraw;
                    pathtoraw << imgpath.parent().toString() << imgpath.getBaseName();
                    utils::io::ImportRawImg_NoPal( curfrm, pathtoraw.str(), res );
                    break;
                }
                default:
                {
                    stringstream strserror;
                    strserror<< "Image " <<imgpath.toString() <<" doesn't look like a BMP, RAW or PNG image !";
                    throw std::runtime_error(strserror.str());
                }
            };

            m_outSprite.m_frames.push_back( std::move(curfrm) );
        }

    private:
        Poco::Path              m_inDirPath;
        sprite_t              & m_outSprite;
        std::atomic<uint32_t> * m_pProgress;
    };




//=============================================================================================
//  Type Camoflage Functions
//=============================================================================================

    template<>
        void ExportSpriteToDirectory( const SpriteData<gimg::tiled_image_i4bpp> & srcspr, 
                                      const std::string                         & outpath, 
                                      utils::io::eSUPPORT_IMG_IO                  imgtype,
                                      bool                                        usexmlpal,
                                      std::atomic<uint32_t>                     * progresscnt ) 
    {
        SpriteToDirectory<SpriteData<gimg::tiled_image_i4bpp>> mywriter(srcspr);
        mywriter.WriteSpriteToDir( outpath, imgtype, usexmlpal, progresscnt ); 
    }

    template<>
       void ExportSpriteToDirectory( const SpriteData<gimg::tiled_image_i8bpp> & srcspr, 
                                     const std::string                         & outpath, 
                                     utils::io::eSUPPORT_IMG_IO                  imgtype,
                                     bool                                        usexmlpal,
                                     std::atomic<uint32_t>                     * progresscnt )
    {
        SpriteToDirectory<SpriteData<gimg::tiled_image_i8bpp>> mywriter(srcspr);
        mywriter.WriteSpriteToDir( outpath, imgtype, usexmlpal, progresscnt ); 
    }


    template<>
        SpriteData<gimg::tiled_image_i4bpp> ImportSpriteFromDirectory( const std::string     & inpath, 
                                                                       bool                    bReadImgByIndex,
                                                                       bool                    bParseXmlPal,
                                                                       std::atomic<uint32_t> * progresscnt ) 
    { 
        SpriteData<gimg::tiled_image_i4bpp>                    result;
        DirectoryToSprite<SpriteData<gimg::tiled_image_i4bpp>> myreader(result);
        myreader.ParseSpriteFromDirectory( inpath, bReadImgByIndex, progresscnt, bParseXmlPal );
        return result;
    }

    template<>
       SpriteData<gimg::tiled_image_i8bpp> ImportSpriteFromDirectory( const std::string     & inpath, 
                                                                       bool                    bReadImgByIndex,
                                                                       bool                    bParseXmlPal,
                                                                       std::atomic<uint32_t> * progresscnt ) 
    { 
        SpriteData<gimg::tiled_image_i8bpp>                    result;
        DirectoryToSprite<SpriteData<gimg::tiled_image_i8bpp>> myreader(result);
        myreader.ParseSpriteFromDirectory( inpath, bReadImgByIndex, progresscnt, bParseXmlPal );
        return result;
    }


    eSpriteType QuerySpriteTypeFromDirectory( const std::string & dirpath )throw(std::runtime_error)
    {
        using namespace Poco::XML;
        ifstream          in( Poco::Path(dirpath).append( xmlstrings::SPRITE_Properties ).toString() );

        if( in.bad() || !in.is_open() )
            throw std::runtime_error( xmlstrings::SPRITE_Properties + " cannot be opened !" );

        InputSource       src(in);
        DOMParser         parser;
        AutoPtr<Document> pSprinfo = parser.parse(&src);
        NodeIterator      itnode( pSprinfo, NodeFilter::SHOW_ATTRIBUTE | NodeFilter::SHOW_ELEMENT | NodeFilter::SHOW_TEXT );
        Node             *pCurNode = itnode.nextNode(); //Get the first node loaded
        bool              bIs256Colors = false;


        //Read every elements
        while( pCurNode != nullptr )
        {
            if( pCurNode->nodeName() == xmlstrings::XML_PROP_IS256COL )
                _parseXMLHexaValToValue(pCurNode->getNodeValue(), bIs256Colors );

            pCurNode = itnode.nextNode();
        }

        return (bIs256Colors)? eSpriteType::spr8bpp : eSpriteType::spr4bpp;
    }


};};