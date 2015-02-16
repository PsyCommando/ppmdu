/*
    Implementation of the XML Parser/Writer for reading/writing the sprites to disk.
*/
#include "sprite_io.hpp"
#include <ppmdu/utils/poco_wrapper.hpp>
#include <ppmdu/containers/sprite_data.hpp>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <iomanip>
#include <array>
#include <pugixml.hpp>
#include <Poco/DirectoryIterator.h>
#include <Poco/File.h>
#include <Poco/Path.h>

using namespace std;
using utils::io::eSUPPORT_IMG_IO;

namespace pmd2 { namespace graphics
{

//=============================================================================================
// Constants
//=============================================================================================
    /*
        All the strings used to parse and write the xml for the sprite!
    */
    namespace SpriteXMLStrings
    {
        //Generic Attributes
        static const string XML_ATTR_NAME       = "name";

        //Animation Stuff
        static const string XML_ROOT_ANIMDAT    = "AnimData";    //Root
        static const string XML_NODE_ANIMGRPTBL = "AnimGroupTable"; 
        static const string XML_NODE_ANIMGRP    = "AnimGroup";
        static const string XML_PROP_ANIMSEQIND = "AnimSequenceIndex";
        static const string XML_NODE_ANIMSEQ    = "AnimSequence";
        static const string XML_NODE_ANIMSEQTBL = "AnimSequenceTable"; 
        static const string XML_NODE_ANIMFRM    = "AnimFrame";

        //Meta-Frames Stuff
        static const string XML_ROOT_FRMLST    = "FrameList";
        static const string XML_NODE_FRMGRP    = "FrameGroup";
        static const string XML_NODE_FRMFRM    = "Frame";

        //OffsetList Stuff
        static const string XML_ROOT_OFFLST    = "OffsetList";
        static const string XML_NODE_OFFSET    = "Offset";

        //Sprite Property Stuff
        static const string XML_ROOT_SPRPROPS  = "SpriteProperties";

        //Images info
        static const string XML_ROOT_IMGINFO   = "ImagesInfo";
        static const string XML_NODE_IMAGE     = "ImageProperty";
        static const string XML_PROP_ZINDEX    = "ZIndex";

        //Other nodes
        static const string XML_NODE_PALLETTE  = "Palette";
        static const string XML_NODE_SHADOW    = "Shadow";
        static const string XML_NODE_SPRITE    = "Sprite";
        static const string XML_NODE_RES       = "Resolution";
        static const string XML_NODE_COLOR     = "Color";

        //Properties Stuff (names of all the properties we'll write)
        static const string XML_PROP_IMGINDEX  = "ImageIndex";
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
        static const string XML_PROP_LASTMFRM  = "IsLastMFrmInGrp";
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

        static const string XML_PROP_SPRTY      = "Is8Ways";
        static const string XML_PROP_IS256COL  = "Is256Colors";
        static const string XML_PROP_UNK13 = "IsMosaicSprite";
        static const string XML_PROP_UNK11     = "Unk11";
        static const string XML_PROP_UNK12     = "Unk12";

        static const string XML_PROP_RED       = "R";
        static const string XML_PROP_GREEN     = "G";
        static const string XML_PROP_BLUE      = "B";

        //Special chars
        static const string PARSE_HEX_NUMBER   = "0x";
    };

//---------------------------------------------------------------------------------------------
//  Descriptions for the various parameters
//---------------------------------------------------------------------------------------------
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

    const std::string SprInfo::DESC_spriteType    = "If 1 character sprite. If 0, other sprite. This messes with what anim groups are used for!";
    const std::string SprInfo::DESC_Is256Sprite     = "If 1, the game draw the sprite as a 8bpp 256 color sprite from memory!(You need to specify it in the palette info too for it to work!)\n"
                                                      "       If 0, images are drawn as 4bpp !";
    const std::string SprInfo::DESC_Unk13     = "If 1, load the first row of tiles of each images one after the other, the the second, and so on. Seems to be for very large animated sprites!";
    const std::string SprInfo::DESC_Unk11           = "This far 0, 1, 3(d79p41a1.wan), 4(as001.wan).. Seems to deal with the palette slot in-game.";
    const std::string SprInfo::DESC_Unk12           = "unknown";

//=============================================================================================
// Utility Functions
//=============================================================================================
    /**************************************************************
        Parses a value directly as is.
        Return value is a parameter, to avoid having to specify
        a template parameter.
    **************************************************************/
    template<class _Ty>
        void _parseXMLValToValue( const string & str, _Ty out_val )
    {
        stringstream sstr(str);
        sstr <<dec << str;
        sstr >> out_val;
    }

    /**************************************************************
        Parse a value that might be an hex number or not.
    **************************************************************/
    template<class _Ty>
        void _parseXMLHexaValToValue( const string & str, _Ty & out_val )
    {
        stringstream sstr;
        std::size_t  foundprefix = str.find( SpriteXMLStrings::PARSE_HEX_NUMBER );

        if( foundprefix != string::npos )
            sstr <<hex <<string( foundprefix + str.begin(), str.end() ).substr(2); //make sure the string begins at "0x" and skip "0x"
        else
            sstr <<dec <<str;

        sstr >> out_val;
    }

    /**************************************************************
        Parse the "name" attribute from the attribute map sprcified.
        Return empty string if the attribute wasn't found!
    **************************************************************/
    //string _parseNameAttribute( const pugi::xml_object_range<pugi::xml_attribute_iterator> & attributes )
    //{
    //    for( auto attr : attributes )
    //    {
    //        if( attr.name() == SpriteXMLStrings::XML_ATTR_NAME )
    //            return attr.value();
    //    }
    //    return string();
    //}

//=============================================================================================
//  Sprite XML Templates Reader
//=============================================================================================

    /**************************************************************
        A class that reads the xml template files for naming animation groups properly
    **************************************************************/
    class SpriteTemplateReader
    {
        //#TODO
    };


//=============================================================================================
//  SpriteXMLParser
//=============================================================================================
    /*
        SpriteXMLParser
            Reads the xml files and put them in the appropriate data structures.
    */
    class SpriteXMLParser
    {
        //static const unsigned int MY_NODE_FILTER = NodeFilter::SHOW_ATTRIBUTE | NodeFilter::SHOW_ELEMENT;
    public:
        //typedef _SPRITE_t sprite_t;

        /**************************************************************
        **************************************************************/
        SpriteXMLParser(BaseSprite * out_spr)
            :m_pOutSprite(out_spr)
        {
        }

        /**************************************************************
        **************************************************************/
        void ParseXML( Poco::Path sprrootfolder, uint32_t nbimgs, bool parsexmlpal )
        {
            using namespace SpriteXMLStrings;
            std::ifstream inProperties( Poco::Path(sprrootfolder).append(SPRITE_Properties_fname).toString() );
            std::ifstream inAnims     ( Poco::Path(sprrootfolder).append(SPRITE_Animations_fname).toString() );
            std::ifstream inMFrames   ( Poco::Path(sprrootfolder).append(SPRITE_Frames_fname    ).toString() );
            std::ifstream inOffsets   ( Poco::Path(sprrootfolder).append(SPRITE_Offsets_fname   ).toString() );
            std::ifstream inImgsInfo  ( Poco::Path(sprrootfolder).append(SPRITE_ImgsInfo_fname  ).toString() );

            if( inProperties.bad() || inAnims.bad() || inMFrames.bad() || inOffsets.bad() )
            {
                stringstream sstr;
                sstr << "Error: One of the XML files in the sprite directory \""
                    <<sprrootfolder.toString() << "\" cannot be opened for some reasons..";
                throw std::runtime_error( sstr.str() );
            }

            {
                //utils::MrChronometer chrono("ParseXMLTotal");
                ParseSpriteInfo(inProperties);
                ParseAnimations(inAnims     );
                ParseMetaFrames(inMFrames   );
                ParseOffsets   (inOffsets   );

                //Pre-alloc
                m_pOutSprite->getImgsInfo().resize(nbimgs);
                ParseImagesInfo(inImgsInfo  );
            }
            if(parsexmlpal)
            {
                std::ifstream inPal( Poco::Path(sprrootfolder).append(SPRITE_Palette_fname).toString() );
                ParsePalette(inPal);
            }

        }

    private:

        //A simple little struct used very briefly to make things more readable.
        struct tmpoffset
        {
            uint16_t x,y;
        };

        /**************************************************************
            Used to parse the 2 offsets contained in an anim frame's 
            xml data's child node!
        **************************************************************/
        tmpoffset ParseAnimFrmOffsets( const pugi::xml_node & aoffnode )
        {
            tmpoffset res = {0,0};

            for( auto & compnode : aoffnode.children() )
            {
                if( compnode.name() == SpriteXMLStrings::XML_PROP_OFFSETX )
                        _parseXMLHexaValToValue( compnode.child_value(), res.x );
                else if( compnode.name() == SpriteXMLStrings::XML_PROP_OFFSETY )
                    _parseXMLHexaValToValue( compnode.child_value(), res.y );
            }

            return std::move(res);
        }

        /**************************************************************
        **************************************************************/
        AnimFrame ParseAnimationFrame( const pugi::xml_node & frmnode )
        {
            AnimFrame afrm;
            
            for( auto & propnode : frmnode.children() )
            {
                if( propnode.name() == SpriteXMLStrings::XML_PROP_DURATION )
                {
                    _parseXMLHexaValToValue( propnode.child_value(), afrm.frameDuration );
                }
                else if( propnode.name() == SpriteXMLStrings::XML_PROP_METAINDEX )
                    _parseXMLHexaValToValue( propnode.child_value(), afrm.metaFrmGrpIndex );
                else if( propnode.name() == SpriteXMLStrings::XML_NODE_SPRITE )
                {
                    tmpoffset result = ParseAnimFrmOffsets( propnode );
                    afrm.sprOffsetX = result.x;
                    afrm.sprOffsetY = result.y;
                }
                else if( propnode.name() == SpriteXMLStrings::XML_NODE_SHADOW )
                {
                    tmpoffset result = ParseAnimFrmOffsets( propnode );
                    afrm.shadowOffsetX = result.x;
                    afrm.shadowOffsetY = result.y;
                }
            }

            return std::move(afrm);
        }

        /**************************************************************
        **************************************************************/
        AnimationSequence ParseAnimationSequence( const pugi::xml_node & seqnode )
        {
            AnimationSequence aseq;

            //Get the name if applicable. If not in the attributes will set empty string
            aseq.setName( seqnode.attribute(SpriteXMLStrings::XML_ATTR_NAME.c_str()).as_string() );

            for( auto & frmnode : seqnode.children( SpriteXMLStrings::XML_NODE_ANIMFRM.c_str() ) )
                 aseq.insertFrame( ParseAnimationFrame( frmnode ) );

            return std::move(aseq);
        }

        /**************************************************************
        **************************************************************/
        vector<AnimationSequence> ParseAnimationSequences( const pugi::xml_node & seqstblnode )
        {
            vector<AnimationSequence> seqs;

            for( auto & seqnode : seqstblnode.children(SpriteXMLStrings::XML_NODE_ANIMSEQ.c_str()) )
                seqs.push_back( ParseAnimationSequence( seqnode ) );

            return std::move(seqs);
        }

        /**************************************************************
        **************************************************************/
        vector<SpriteAnimationGroup> ParseAnimationGroups( const pugi::xml_node & agtblnode )
        {
            vector<SpriteAnimationGroup> grps;

            auto & animgrpsrange = agtblnode.children(SpriteXMLStrings::XML_NODE_ANIMGRP.c_str());
            for( auto & animgrpnode : animgrpsrange )
            {
                SpriteAnimationGroup agrp;

                //Get the name
                if( animgrpnode.attributes_begin() != animgrpnode.attributes_end() )
                    agrp.group_name = animgrpnode.attribute(SpriteXMLStrings::XML_ATTR_NAME.c_str()).as_string();


                //Parse the sequences indexes
                for( auto & seqindnode : animgrpnode.children( SpriteXMLStrings::XML_PROP_ANIMSEQIND.c_str() ) )
                {
                    uint32_t index = 0;
                    _parseXMLHexaValToValue( seqindnode.child_value(), index );
                    agrp.seqsIndexes.push_back(index);
                }

                //Add an empty group if we don't have any sequences !
                grps.push_back( std::move(agrp) );
            }
            return std::move( grps );
        }

        /**************************************************************
        **************************************************************/
        void ParseAnimations(std::ifstream & in)
        {
            //utils::MrChronometer chronoxmlanims("ParseXMLAnims");
            using namespace SpriteXMLStrings;
            using namespace pugi;
            xml_document mydoc;

            if( ! mydoc.load(in) )
                throw std::runtime_error("Failed to create xml_document for parsing Sprite Meta-Frames!");

            auto & animsrange = mydoc.child(XML_ROOT_ANIMDAT.c_str()).children();
            //Read every elements
            for( auto & curnode : animsrange )
            {
                if( curnode.name() == SpriteXMLStrings::XML_NODE_ANIMGRPTBL )
                    m_pOutSprite->getAnimGroups() = ParseAnimationGroups( curnode );
                else if( curnode.name() == SpriteXMLStrings::XML_NODE_ANIMSEQTBL )
                    m_pOutSprite->getAnimSequences() = ParseAnimationSequences( curnode );
            }
        }

        /**************************************************************
        **************************************************************/
        void ParseOffsets(std::ifstream & in)
        {
            //utils::MrChronometer chrono("ParseXMLSpriteOffsets");
            using namespace SpriteXMLStrings;
            using namespace pugi;
            xml_document mydoc;

            if( ! mydoc.load(in) )
                throw std::runtime_error("Failed to create xml_document for parsing Sprite Meta-Frames!");

            auto & offsetsrange = mydoc.child(XML_ROOT_OFFLST.c_str()).children(SpriteXMLStrings::XML_NODE_OFFSET.c_str());
            //Read every elements
            for( auto & offnode : offsetsrange )
            {
                sprOffParticle    anoffs;
                for( auto & compnode : offnode.children() )
                {
                    if( compnode.name() == SpriteXMLStrings::XML_PROP_X )
                        _parseXMLHexaValToValue( compnode.child_value(), anoffs.offx );
                    else if( compnode.name() == SpriteXMLStrings::XML_PROP_Y )
                        _parseXMLHexaValToValue( compnode.child_value(), anoffs.offy );
                }
                m_pOutSprite->getPartOffsets().push_back(anoffs);
            }
        }

        /**************************************************************
        **************************************************************/
        MetaFrame::eRes ParseMetaFrameRes( const pugi::xml_node & resnode )
        {
            utils::Resolution myres      = {0,0};
            MetaFrame::eRes   result     = MetaFrame::eRes::_INVALID;

            for( auto & compnode : resnode.children() )
            {
                if( compnode.name() == SpriteXMLStrings::XML_PROP_WIDTH )
                    _parseXMLHexaValToValue( compnode.child_value(), myres.width );
                else if( compnode.name() == SpriteXMLStrings::XML_PROP_HEIGTH )
                    _parseXMLHexaValToValue( compnode.child_value(), myres.height );
            }

            result = MetaFrame::IntegerResTo_eRes( myres.width, myres.height );

            if( result == MetaFrame::eRes::_INVALID )
            {
                stringstream sstr;
                sstr << "Error: invalid resolution specified for a meta-frame!\n"
                        << "Got " <<myres.width << "x" <<myres.height << " for meta-frame #" <<m_pOutSprite->getMetaFrames().size()
                        << ", referred to in meta-frame group #" <<m_pOutSprite->getMetaFrmsGrps().size() <<" !\n"
                        << "Valid resolutions are any of the following :\n";

                for( const auto & theentry : graphics::MetaFrame::ResEquiv )
                    sstr << "-> " <<theentry.x <<"x" <<theentry.y <<"\n";

                throw std::runtime_error( sstr.str() );
            }

            return result;
        }
        
        /**************************************************************
        **************************************************************/
        MetaFrame ParseAMetaFrame( const pugi::xml_node & mfnode )
        {
            MetaFrame         mf;

            for( auto & propnode : mfnode.children() )
            {
                if( propnode.name() == SpriteXMLStrings::XML_PROP_IMGINDEX )
                {
                    _parseXMLHexaValToValue( propnode.child_value(), mf.imageIndex );
                }
                else if( propnode.name()  == SpriteXMLStrings::XML_PROP_UNK0 )
                    _parseXMLHexaValToValue( propnode.child_value(), mf.unk0 );
                else if( propnode.name()  == SpriteXMLStrings::XML_PROP_OFFSETX )
                    _parseXMLHexaValToValue( propnode.child_value(), mf.offsetX );
                else if( propnode.name()  == SpriteXMLStrings::XML_PROP_OFFSETY )
                    _parseXMLHexaValToValue( propnode.child_value(), mf.offsetY );
                else if( propnode.name()  == SpriteXMLStrings::XML_PROP_UNK1 )
                    _parseXMLHexaValToValue( propnode.child_value(), mf.unk1 );
                else if( propnode.name()  == SpriteXMLStrings::XML_PROP_VFLIP )
                    _parseXMLHexaValToValue( propnode.child_value(), mf.vFlip );
                else if( propnode.name()  == SpriteXMLStrings::XML_PROP_HFLIP )
                    _parseXMLHexaValToValue( propnode.child_value(), mf.hFlip );
                else if( propnode.name()  == SpriteXMLStrings::XML_PROP_MOSAIC )
                    _parseXMLHexaValToValue( propnode.child_value(), mf.Mosaic );
                else if( propnode.name()  == SpriteXMLStrings::XML_PROP_OFFXBIT6 )
                    _parseXMLHexaValToValue( propnode.child_value(), mf.XOffbit6 );
                else if( propnode.name()  == SpriteXMLStrings::XML_PROP_OFFXBIT7 )
                    _parseXMLHexaValToValue( propnode.child_value(), mf.XOffbit7 );
                else if( propnode.name()  == SpriteXMLStrings::XML_PROP_OFFYBIT3 )
                    _parseXMLHexaValToValue( propnode.child_value(), mf.YOffbit3 );
                else if( propnode.name()  == SpriteXMLStrings::XML_PROP_OFFYBIT5 )
                    _parseXMLHexaValToValue( propnode.child_value(), mf.YOffbit5 );
                else if( propnode.name()  == SpriteXMLStrings::XML_PROP_OFFYBIT6 )
                    _parseXMLHexaValToValue( propnode.child_value(), mf.YOffbit6 );
                else if( propnode.name()  == SpriteXMLStrings::XML_NODE_RES )
                {
                    mf.resolution = ParseMetaFrameRes( propnode );
                }
            }

            return std::move(mf);
        }

        /**************************************************************
        **************************************************************/
        void ParseMetaFrames(std::ifstream & in)
        {
            //utils::MrChronometer chrono("ParseXMLSpriteMetaFrames");
            using namespace SpriteXMLStrings;
            using namespace pugi;
            xml_document mydoc;

            if( ! mydoc.load(in) )
                throw std::runtime_error("Failed to create xml_document for parsing Sprite Meta-Frames!");

            //Read every groups
            for( auto & mfgnode : mydoc.child(XML_ROOT_FRMLST.c_str()).children( SpriteXMLStrings::XML_NODE_FRMGRP.c_str() ) )
            {
                auto           mfgnodechilds = mfgnode.children( SpriteXMLStrings::XML_NODE_FRMFRM.c_str() );
                MetaFrameGroup mfg; //Create the meta-frame group
                mfg.metaframes.reserve( distance( mfgnodechilds.begin(), mfgnodechilds.end() ) );

                //Read all frames in the group
                for( auto & mfnode : mfgnodechilds )
                {
                    mfg.metaframes.push_back( m_pOutSprite->getMetaFrames().size() ); //put the frame's offset in the meta frame group table
                    m_pOutSprite->getMetaFrames().push_back( ParseAMetaFrame( mfnode ) );
                }

                m_pOutSprite->getMetaFrmsGrps().push_back( std::move(mfg) );
            }
        }

        /**************************************************************
        **************************************************************/
        void ParseSpriteInfo(std::ifstream & in)
        {
            //utils::MrChronometer chrono("ParseXMLSpriteInfo");
            using namespace SpriteXMLStrings;
            using namespace pugi;
            xml_document mydoc;

            if( ! mydoc.load(in) )
                throw std::runtime_error("Failed to create xml_document for parsing Sprite info!");

            //Read every elements
            for( auto & curnode : mydoc.child(XML_ROOT_SPRPROPS.c_str()).children() )
            {
                if( curnode.name() == XML_PROP_UNK3 )
                {
                    _parseXMLHexaValToValue( curnode.child_value(), m_pOutSprite->getSprInfo().Unk3 );
                }
                else if( curnode.name() == XML_PROP_COLPERROW )
                    _parseXMLHexaValToValue( curnode.child_value(), m_pOutSprite->getSprInfo().nbColorsPerRow );
                else if( curnode.name() == XML_PROP_UNK4 )
                    _parseXMLHexaValToValue( curnode.child_value(), m_pOutSprite->getSprInfo().Unk4 );
                else if( curnode.name() == XML_PROP_UNK5 )
                    _parseXMLHexaValToValue( curnode.child_value(), m_pOutSprite->getSprInfo().Unk5 );
                else if( curnode.name() == XML_PROP_UNK6 )
                    _parseXMLHexaValToValue( curnode.child_value(), m_pOutSprite->getSprInfo().Unk6 );
                else if( curnode.name() == XML_PROP_UNK7 )
                    _parseXMLHexaValToValue( curnode.child_value(), m_pOutSprite->getSprInfo().Unk7 );
                else if( curnode.name() == XML_PROP_UNK8 )
                    _parseXMLHexaValToValue( curnode.child_value(), m_pOutSprite->getSprInfo().Unk8 );
                else if( curnode.name() == XML_PROP_UNK9 )
                    _parseXMLHexaValToValue( curnode.child_value(), m_pOutSprite->getSprInfo().Unk9 );
                else if( curnode.name() == XML_PROP_UNK10 )
                    _parseXMLHexaValToValue( curnode.child_value(), m_pOutSprite->getSprInfo().Unk10 );
                else if( curnode.name() == XML_PROP_SPRTY )
                {
                    uint16_t srty = 0;
                    _parseXMLHexaValToValue( curnode.child_value(), srty );
                    m_pOutSprite->getSprInfo().spriteType = static_cast<graphics::eSprTy>(srty);
                }
                else if( curnode.name() == XML_PROP_IS256COL )
                    _parseXMLHexaValToValue( curnode.child_value(), m_pOutSprite->getSprInfo().m_is256Sprite );
                else if( curnode.name() == XML_PROP_UNK13 )
                    _parseXMLHexaValToValue( curnode.child_value(), m_pOutSprite->getSprInfo().Unk13 );
                else if( curnode.name() == XML_PROP_UNK11 )
                    _parseXMLHexaValToValue( curnode.child_value(), m_pOutSprite->getSprInfo().Unk11 );
                else if( curnode.name() == XML_PROP_UNK12 )
                    _parseXMLHexaValToValue( curnode.child_value(), m_pOutSprite->getSprInfo().Unk12 );

                //pCurNode = itnode.nextNode();
            }
        }

        /**************************************************************
        **************************************************************/
        void ParsePalette(std::ifstream & in)
        {
            using namespace SpriteXMLStrings;
            using namespace pugi;
            xml_document mydoc;

            if( ! mydoc.load(in) )
                throw std::runtime_error("Failed to create xml_document for parsing Sprite xml palette!");

            //Read every elements
            for( auto & colornode : mydoc.child(XML_NODE_PALLETTE.c_str()).children(XML_NODE_COLOR.c_str()) )
            {
                graphics::colRGB24 mycolor;

                //Copy the content of the color
                for( auto & acomponent : colornode.children() )
                {
                    if( acomponent.name() == SpriteXMLStrings::XML_PROP_RED )
                        _parseXMLHexaValToValue( acomponent.child_value(), mycolor.red ); 
                    else if( acomponent.name() == SpriteXMLStrings::XML_PROP_GREEN )
                        _parseXMLHexaValToValue( acomponent.child_value(), mycolor.green ); 
                    else if( acomponent.name() == SpriteXMLStrings::XML_PROP_BLUE )
                        _parseXMLHexaValToValue( acomponent.child_value(), mycolor.blue ); 
                }
                m_pOutSprite->getPalette().push_back(mycolor);
            }
        }

        void ParseImagesInfo(std::ifstream & in)
        {
            using namespace SpriteXMLStrings;
            using namespace pugi;
            xml_document doc;

            if( ! doc.load(in) )
                throw std::runtime_error("Failed to create xml_document for parsing the images info xml file!");

            //Read every elements
            for( auto & imagenode : doc.child(XML_ROOT_IMGINFO.c_str()).children(XML_NODE_IMAGE.c_str()) )
            {
                int32_t   imgindex = -1;
                ImageInfo imginf;

                for( auto & props : imagenode.children() )
                {
                    if( props.name() == SpriteXMLStrings::XML_PROP_IMGINDEX )
                        _parseXMLHexaValToValue( props.child_value(), imgindex ); 
                    else if( props.name() == SpriteXMLStrings::XML_PROP_ZINDEX )
                        _parseXMLHexaValToValue( props.child_value(), imginf.zindex ); 
                }

                if( imgindex != -1 )
                {
                    auto & imginfvector = m_pOutSprite->getImgsInfo(); //We already pre-allocated the vector earlier
                    imginfvector[imgindex] = std::move(imginf); //Add the image data
                }
            }
        }

        //Variables
        BaseSprite * m_pOutSprite;
    };



//=============================================================================================
//  Sprite to XML writer
//=============================================================================================

    class SpriteXMLWriter
    {
        //static const unsigned int MY_WRITER_FLAGS = XMLWriter::WRITE_XML_DECLARATION | XMLWriter::PRETTY_PRINT; 
    public:

        /**************************************************************
        **************************************************************/
        SpriteXMLWriter( const BaseSprite * myspr)
            :m_pInSprite(myspr), m_pProgresscnt(nullptr)
        {
            m_convBuff.fill(0);
            m_secConvbuffer.fill(0);
        }


        /**************************************************************
            Write all the data for the Sprite as XML in the
            specified folder  under ! 
                -colorpalasxml : if true, will print an xml palette 
                                 alongside the rest!
        **************************************************************/
        void WriteXMLFiles( const string & folderpath, const spriteWorkStats & stats, bool xmlcolorpal = false, std::atomic<uint32_t> * progresscnt = nullptr ) 
        {
            m_outPath = Poco::Path(folderpath);
            m_pProgresscnt = progresscnt;

            WriteProperties();
            WriteAnimations     ( stats.propAnims, stats.totalAnimSeqs, stats.totalAnimFrms );
            WriteMetaFrameGroups( stats.propMFrames );
            WriteOffsets        ( stats.propOffsets );
            WriteImgInfo        ( stats.propImgInfo );

            //Write xml palette if needed
            if( xmlcolorpal )
                WritePalette();
        }

    private:
        //
        //  Common Stuff
        //

        //Returns a pointer to the buffer passed as argument
        inline const char * FastTurnIntToHexCStr( unsigned int value )
        {
            sprintf_s( m_convBuff.data(), CBuffSZ, "0x%s", itoa( value, m_secConvbuffer.data(), 16 ) );
            return m_convBuff.data();
        }

        //Returns a pointer to the buffer passed as argument
        inline const char * FastTurnIntToCStr( unsigned int value )
        {
            return itoa( value, m_convBuff.data(), 10 );
        }

        /**************************************************************
        **************************************************************/
        inline void writeComment( pugi::xml_node & node, const string & str )
        {
            using namespace pugi;
            node.append_child( xml_node_type::node_comment ).set_value( str.c_str() );
        }

        inline void WriteNodeWithValue( pugi::xml_node & parentnode, const string & name, const char * value )
        {
            parentnode.append_child(name.c_str()).append_child(pugi::node_pcdata).set_value(value);
        }

        /*
            This clears the instance's string stream, and set it back to the beginning, ready for 
            converting more stuff.
        */
        inline void resetStrs()
        {
            m_strs.str(string());
        }

    private:
        //
        //  Specific Stuff
        //

        /**************************************************************
        **************************************************************/
        void WriteProperties()
        {
            using namespace SpriteXMLStrings;
            using namespace pugi;
            string          outpath = Poco::Path(m_outPath).append(SPRITE_Properties_fname).toString();
            xml_document    doc;
            xml_node        rootnode = doc.append_child( XML_ROOT_SPRPROPS.c_str() );

            //Color stuff
            writeComment( rootnode, SprInfo::DESC_Unk3 );
            WriteNodeWithValue( rootnode, XML_PROP_UNK3, FastTurnIntToHexCStr( m_pInSprite->getSprInfo().Unk3 ) );

            writeComment( rootnode, SprInfo::DESC_nbColorsPerRow );
            WriteNodeWithValue( rootnode, XML_PROP_COLPERROW, FastTurnIntToCStr( m_pInSprite->getSprInfo().nbColorsPerRow ) );

            writeComment( rootnode, SprInfo::DESC_Unk4 );
            WriteNodeWithValue( rootnode, XML_PROP_UNK4, FastTurnIntToHexCStr( m_pInSprite->getSprInfo().Unk4 ) );

            writeComment( rootnode, SprInfo::DESC_Unk5 );
            WriteNodeWithValue( rootnode, XML_PROP_UNK5, FastTurnIntToHexCStr( m_pInSprite->getSprInfo().Unk5 ) );

            //Anim Stuff
            writeComment( rootnode, SprInfo::DESC_Unk6 );
            WriteNodeWithValue( rootnode, XML_PROP_UNK6, FastTurnIntToHexCStr( m_pInSprite->getSprInfo().Unk6 ) );

            writeComment( rootnode, SprInfo::DESC_Unk7 );
            WriteNodeWithValue( rootnode, XML_PROP_UNK7, FastTurnIntToHexCStr( m_pInSprite->getSprInfo().Unk7 ) );

            writeComment( rootnode, SprInfo::DESC_Unk8 );
            WriteNodeWithValue( rootnode, XML_PROP_UNK8, FastTurnIntToHexCStr( m_pInSprite->getSprInfo().Unk8 ) );

            writeComment( rootnode, SprInfo::DESC_Unk9 );
            WriteNodeWithValue( rootnode, XML_PROP_UNK9, FastTurnIntToHexCStr( m_pInSprite->getSprInfo().Unk9 ) );

            writeComment( rootnode, SprInfo::DESC_Unk10 );
            WriteNodeWithValue( rootnode, XML_PROP_UNK10, FastTurnIntToHexCStr( m_pInSprite->getSprInfo().Unk10 ) );

            //Other properties
            writeComment( rootnode, SprInfo::DESC_spriteType );
            WriteNodeWithValue( rootnode, XML_PROP_SPRTY, FastTurnIntToHexCStr( static_cast<uint16_t>(m_pInSprite->getSprInfo().spriteType) ) );

            writeComment( rootnode, SprInfo::DESC_Is256Sprite );
            WriteNodeWithValue( rootnode, XML_PROP_IS256COL, FastTurnIntToHexCStr( m_pInSprite->getSprInfo().m_is256Sprite ) );

            writeComment( rootnode, SprInfo::DESC_Unk13 );
            WriteNodeWithValue( rootnode, XML_PROP_UNK13, FastTurnIntToHexCStr( m_pInSprite->getSprInfo().Unk13 ) );

            writeComment( rootnode, SprInfo::DESC_Unk11 );
            WriteNodeWithValue( rootnode, XML_PROP_UNK11, FastTurnIntToHexCStr( m_pInSprite->getSprInfo().Unk11 ) );

            writeComment( rootnode, SprInfo::DESC_Unk12 );
            WriteNodeWithValue( rootnode, XML_PROP_UNK12, FastTurnIntToHexCStr( m_pInSprite->getSprInfo().Unk12 ) );

            if( ! doc.save_file( outpath.c_str() ) )
                throw std::runtime_error("Error, can't write sprite info xml file!");
        }

        /**************************************************************
        **************************************************************/
        void WritePalette()
        {
            using namespace SpriteXMLStrings;
            using namespace pugi;
            xml_document doc;
            string       outpath = Poco::Path(m_outPath).append(SPRITE_Palette_fname).toString();
            xml_node     rootnode = doc.append_child( XML_NODE_PALLETTE.c_str() );

            resetStrs();
            m_strs <<"Total nb of color(s) : " <<setw(4) <<setfill(' ') <<m_pInSprite->getPalette().size();
            writeComment( rootnode, m_strs.str() );

            for( unsigned int i = 0; i < m_pInSprite->getPalette().size(); ++i )
            {
                const auto & acolor = m_pInSprite->getPalette()[i];
                resetStrs();
                m_strs <<"#" <<i;
                writeComment( rootnode, m_strs.str() );

                xml_node curcolor = rootnode.append_child( XML_NODE_COLOR.c_str() );
                WriteNodeWithValue( curcolor, XML_PROP_RED, FastTurnIntToCStr( acolor.red ) );
                WriteNodeWithValue( curcolor, XML_PROP_GREEN, FastTurnIntToCStr( acolor.green ) );
                WriteNodeWithValue( curcolor, XML_PROP_BLUE, FastTurnIntToCStr( acolor.blue ) );
            }

            if( ! doc.save_file( outpath.c_str() ) )
                throw std::runtime_error("Error, can't write palette xml file!");
        }

        /**************************************************************
        **************************************************************/
        void WriteAnimFrame( pugi::xml_node & parentnode, const AnimFrame & curfrm )
        {
            using namespace SpriteXMLStrings;
            using namespace pugi;

            xml_node anifrmnode = parentnode.append_child( XML_NODE_ANIMFRM.c_str() );
            {
                WriteNodeWithValue( anifrmnode, XML_PROP_DURATION,  FastTurnIntToCStr( curfrm.frameDuration   ) );
                WriteNodeWithValue( anifrmnode, XML_PROP_METAINDEX, FastTurnIntToCStr( curfrm.metaFrmGrpIndex ) );

                xml_node sprnode = anifrmnode.append_child( XML_NODE_SPRITE.c_str() );
                {
                    WriteNodeWithValue(sprnode, XML_PROP_OFFSETX, FastTurnIntToCStr( curfrm.sprOffsetX ) );
                    WriteNodeWithValue(sprnode, XML_PROP_OFFSETY, FastTurnIntToCStr( curfrm.sprOffsetY ) );
                }

                xml_node shadnode = anifrmnode.append_child( XML_NODE_SHADOW.c_str() );
                {
                    WriteNodeWithValue(shadnode, XML_PROP_OFFSETX, FastTurnIntToCStr( curfrm.shadowOffsetX ) );
                    WriteNodeWithValue(shadnode, XML_PROP_OFFSETY, FastTurnIntToCStr( curfrm.shadowOffsetY ) );
                }
            }
        }

        /**************************************************************
        **************************************************************/
        void WriteAnimSequence( pugi::xml_node & parentnode, const AnimationSequence & aseq )
        {
            using namespace SpriteXMLStrings;
            using namespace pugi;
            
            xml_node seqnode = parentnode.append_child( XML_NODE_ANIMSEQ.c_str() );

            //Give this sequence a name 
            seqnode.append_attribute(XML_ATTR_NAME.c_str()).set_value(aseq.getName().c_str());

            //Write the content of each frame in that sequence
            for( unsigned int cptfrms = 0; cptfrms < aseq.getNbFrames(); ++cptfrms )
            {
                resetStrs();
                m_strs << "frm " <<cptfrms;
                writeComment( seqnode, m_strs.str() );
                WriteAnimFrame(seqnode, aseq.getFrame(cptfrms));
            }
        }

        /**************************************************************
            -pAnimNameList : First entry of heach sub-vector is the 
                             anim group name, anything after is anim 
                             sequence names!
        **************************************************************/
        void WriteAnimations( uint32_t proportionofwork, uint32_t totalnbseqs, uint32_t totalnbfrms )
        {
            using namespace SpriteXMLStrings;
            using namespace pugi;
            xml_document doc;
            string       outpath = Poco::Path(m_outPath).append(SPRITE_Animations_fname).toString();
            uint32_t     saveprogress = 0;

            if( m_pProgresscnt != nullptr )
                saveprogress = m_pProgresscnt->load();

            xml_node agnode = doc.append_child(XML_ROOT_ANIMDAT.c_str());

            //Add some comments to help fellow humans 
            resetStrs();
            m_strs <<"Total nb of animation group(s) : " <<setw(4) <<setfill(' ') <<m_pInSprite->getAnimGroups().size();
            writeComment( agnode, m_strs.str() );

            resetStrs();
            m_strs <<"Total nb of sequence(s)        : " <<setw(4) <<setfill(' ') <<totalnbseqs;
            writeComment( agnode, m_strs.str() );

            resetStrs();
            m_strs <<"Total nb of animation frame(s) : " <<setw(4) <<setfill(' ') <<totalnbfrms;
            writeComment( agnode, m_strs.str() );

            //First Write the GroupRefTable
            xml_node     grpreftblnode = agnode.append_child(XML_NODE_ANIMGRPTBL.c_str());
            unsigned int cptgrp = 0;
            for( const auto & animgrp : m_pInSprite->getAnimGroups() )
            {
                resetStrs();
                unsigned int cptseq = 0;
                m_strs <<"Group #" <<cptgrp <<" contains " << animgrp.seqsIndexes.size() << " sequence(s)";
                writeComment( grpreftblnode, m_strs.str() );
                        
                xml_node grpnode = grpreftblnode.append_child(XML_NODE_ANIMGRP.c_str());

                //Give this group a name
                grpnode.append_attribute(XML_ATTR_NAME.c_str()).set_value(animgrp.group_name.c_str());

                //Write the content of each sequences in that group
                for( const auto & aseq : animgrp.seqsIndexes )
                {
                    //m_strs.clear();
                    //m_strs <<cptseq;
                    WriteNodeWithValue( grpnode, XML_PROP_ANIMSEQIND, FastTurnIntToCStr(aseq) );
                    ++cptseq;
                }
                ++cptgrp;

                if( m_pProgresscnt != nullptr )
                {
                    uint32_t prog = ( ( proportionofwork * cptgrp ) / m_pInSprite->getAnimGroups().size() );
                    m_pProgresscnt->store( saveprogress + prog );
                }
            }


            //Second Write the SequenceTable
            xml_node     seqreftblnode = agnode.append_child(XML_NODE_ANIMSEQTBL.c_str());
            unsigned int cptaseqs = 0;

            //Write the content of each group
            for( const auto & animseq : m_pInSprite->getAnimSequences() )
            {
                //Write the content of each sequences in that group
                //strs = stringstream();
                resetStrs();
                m_strs <<"Seq #" <<cptaseqs  <<" contains " <<animseq.getNbFrames()  << " frame(s)";
                writeComment( seqreftblnode, m_strs.str() );
                WriteAnimSequence( seqreftblnode, animseq );

                ++cptaseqs;
            }


            if( ! doc.save_file( outpath.c_str() ) )
                throw std::runtime_error("Error, can't write animation xml file!");
        }

        /**************************************************************
        **************************************************************/
        void WriteMetaFrameGroups( uint32_t proportionofwork )
        {
            using namespace SpriteXMLStrings;
            using namespace pugi;
            xml_document doc;
            string       outpath = Poco::Path(m_outPath).append(SPRITE_Frames_fname).toString();
            uint32_t     saveprogress = 0;

            if( m_pProgresscnt != nullptr )
                saveprogress = m_pProgresscnt->load();

            xml_node frmlstnode = doc.append_child( XML_ROOT_FRMLST.c_str() );

            resetStrs();
            m_strs <<"Total nb of group(s)      : " <<setw(4) <<setfill(' ') <<m_pInSprite->getMetaFrmsGrps().size();
            writeComment( frmlstnode, m_strs.str() );
            resetStrs();
            m_strs <<"Total nb of meta-frame(s) : " <<setw(4) <<setfill(' ') <<m_pInSprite->getMetaFrames().size();
            writeComment( frmlstnode, m_strs.str() );

            for( unsigned int i = 0; i < m_pInSprite->getMetaFrmsGrps().size(); ++i )
            {
                const auto & grp = m_pInSprite->getMetaFrmsGrps()[i];
                resetStrs();
                m_strs <<"MFG #" <<i <<" contains " <<grp.metaframes.size() <<" meta-frame(s)";
                writeComment( frmlstnode, m_strs.str() );

                WriteAMetaFrameGroup( frmlstnode, grp );

                if( m_pProgresscnt != nullptr )
                    m_pProgresscnt->store( saveprogress + ( proportionofwork * i ) / m_pInSprite->getMetaFrmsGrps().size() );
            }

            if( ! doc.save_file( outpath.c_str() ) )
                throw std::runtime_error("Error, can't write meta-frames xml file!");
        }

        /**************************************************************
        **************************************************************/
        inline void WriteAMetaFrameGroup( pugi::xml_node & parentnode, const MetaFrameGroup & grp )
        {
            using namespace SpriteXMLStrings;
            using namespace pugi;
            xml_node        frmgrpnode = parentnode.append_child(XML_NODE_FRMGRP.c_str());

            for( unsigned int i = 0; i < grp.metaframes.size(); ++i )
            {
                writeComment( frmgrpnode, FastTurnIntToCStr(i) );
                WriteMetaFrame( frmgrpnode, grp.metaframes[i] );
            }
        }

        /**************************************************************
        **************************************************************/
        void WriteMetaFrame( pugi::xml_node & parentnode, unsigned int index )
        {
            using namespace pugi;
            using namespace SpriteXMLStrings;
            xml_node     mfnode = parentnode.append_child( XML_NODE_FRMFRM.c_str() );
            const auto & aframe = m_pInSprite->getMetaFrames()[index];

            WriteNodeWithValue( mfnode, XML_PROP_IMGINDEX, FastTurnIntToCStr(aframe.imageIndex) );
            WriteNodeWithValue( mfnode, XML_PROP_UNK0,     FastTurnIntToHexCStr(aframe.unk0) );
            WriteNodeWithValue( mfnode, XML_PROP_OFFSETY,  FastTurnIntToCStr(aframe.offsetY   ) );
            WriteNodeWithValue( mfnode, XML_PROP_OFFSETX,  FastTurnIntToCStr(aframe.offsetX   ) );
            WriteNodeWithValue( mfnode, XML_PROP_UNK1,     FastTurnIntToHexCStr(aframe.unk1) );

            xml_node resnode = mfnode.append_child(XML_NODE_RES.c_str());
            auto resolution = MetaFrame::eResToResolution(aframe.resolution);
            WriteNodeWithValue( resnode, XML_PROP_WIDTH,     FastTurnIntToCStr( resolution.width  ) );
            WriteNodeWithValue( resnode, XML_PROP_HEIGTH,    FastTurnIntToCStr( resolution.height ) );

            WriteNodeWithValue( mfnode, XML_PROP_VFLIP,    FastTurnIntToCStr( aframe.vFlip ) );
            WriteNodeWithValue( mfnode, XML_PROP_HFLIP,    FastTurnIntToCStr( aframe.hFlip ) );
            WriteNodeWithValue( mfnode, XML_PROP_MOSAIC,   FastTurnIntToCStr( aframe.Mosaic) );

            WriteNodeWithValue( mfnode, XML_PROP_OFFXBIT6,   FastTurnIntToCStr( aframe.XOffbit6) );
            WriteNodeWithValue( mfnode, XML_PROP_OFFXBIT7,   FastTurnIntToCStr( aframe.XOffbit7) );

            WriteNodeWithValue( mfnode, XML_PROP_OFFYBIT3,   FastTurnIntToCStr( aframe.YOffbit3) );
            WriteNodeWithValue( mfnode, XML_PROP_OFFYBIT5,   FastTurnIntToCStr( aframe.YOffbit5) );
            WriteNodeWithValue( mfnode, XML_PROP_OFFYBIT6,   FastTurnIntToCStr( aframe.YOffbit6) );
        }

        /**************************************************************
        **************************************************************/
        void WriteOffsets( uint32_t proportionofwork )
        {
            using namespace SpriteXMLStrings;
            using namespace pugi;
            xml_document doc;
            string       outpath = Poco::Path(m_outPath).append(SPRITE_Offsets_fname).toString();

            xml_node offlstnode = doc.append_child(XML_ROOT_OFFLST.c_str());

            //stringstream strs;
            resetStrs();
            m_strs <<"Total nb of offset(s) : " <<setw(4) <<setfill(' ') <<m_pInSprite->getPartOffsets().size();
            writeComment( offlstnode, m_strs.str() );

            uint32_t saveprogress = 0;

            if( m_pProgresscnt != nullptr )
                saveprogress = m_pProgresscnt->load();

            for( unsigned int i = 0; i < m_pInSprite->getPartOffsets().size(); ++i )
            {
                const auto & anoffset = m_pInSprite->getPartOffsets()[i];
                //strs.clear();
                //strs <<"#" <<i;
                writeComment( offlstnode, FastTurnIntToCStr(i) );

                xml_node offnode = offlstnode.append_child(XML_NODE_OFFSET.c_str());
                WriteNodeWithValue( offnode, XML_PROP_X, FastTurnIntToCStr( anoffset.offx ));
                WriteNodeWithValue( offnode, XML_PROP_Y, FastTurnIntToCStr( anoffset.offy ));

                if( m_pProgresscnt != nullptr )
                    m_pProgresscnt->store( saveprogress + ( proportionofwork * i ) / m_pInSprite->getPartOffsets().size() );
            }


            if( ! doc.save_file( outpath.c_str() ) )
                throw std::runtime_error("Error, can't write offset list xml file!");
        }

        void WriteImgInfo( uint32_t proportionofwork  )
        {
            using namespace SpriteXMLStrings;
            using namespace pugi;
            xml_document doc;
            string       outpath      = Poco::Path(m_outPath).append(SPRITE_ImgsInfo_fname).toString();
            xml_node     imginfonode  = doc.append_child(XML_ROOT_IMGINFO.c_str());
            uint32_t     saveprogress = 0;

            if( m_pProgresscnt != nullptr )
                saveprogress = m_pProgresscnt->load();

            unsigned int cpt = 0;
            for( auto & imginfo : m_pInSprite->getImgsInfo() )
            {
                if( imginfo.zindex != 0 )   //Only write non-zero entries
                {
                    xml_node imgnode = imginfonode.append_child(XML_NODE_IMAGE.c_str());
                    WriteNodeWithValue( imgnode, XML_PROP_IMGINDEX, FastTurnIntToCStr( cpt ));
                    WriteNodeWithValue( imgnode, XML_PROP_ZINDEX,   FastTurnIntToCStr( imginfo.zindex ));
                }

                ++cpt;
                if( m_pProgresscnt != nullptr )
                    m_pProgresscnt->store( saveprogress + ( proportionofwork * cpt ) / m_pInSprite->getImgsInfo().size() );
            }

            if( ! doc.save_file( outpath.c_str() ) )
                throw std::runtime_error("Error, can't write image info xml file!");
        }

    private:
        static const int         CBuffSZ = (sizeof(int)*8+1);
        Poco::Path               m_outPath;
        const BaseSprite       * m_pInSprite;
        std::atomic<uint32_t>  * m_pProgresscnt;
        uint32_t                 m_progressProportion; //The percentage of the entire work attributed to this
        std::array<char,CBuffSZ> m_convBuff;           //A buffer for executing convertions to c-strings. Pretty ugly, but less constructor calls, for simple, non-localised integer convertions.
        std::array<char,CBuffSZ> m_secConvbuffer;      //A Secondary convertion buffer
        std::stringstream        m_strs;               //Instance-wide stringstream to handle conversions. Reduces overhead a little.
    };

//=============================================================================================
// XML Functions
//=============================================================================================

    /******************************************************************************************
    ParseXMLDataToSprite
        Use this to read the expected XML files into the specified sprite container.
    ******************************************************************************************/
    void ParseXMLDataToSprite( BaseSprite        * out_spr, 
                               const std::string & spriteFolderPath, 
                               uint32_t            nbimgs,
                               bool                parsexmlpal )
    {
        SpriteXMLParser(out_spr).ParseXML(spriteFolderPath, nbimgs, parsexmlpal);
    }

    /******************************************************************************************
    WriteSpriteDataToXML
        Use this to write to a set of XML files the data from the specified sprite container.
    ******************************************************************************************/
    void WriteSpriteDataToXML( const BaseSprite      * spr, 
                               const std::string     & spriteFolderPath, 
                               const spriteWorkStats & stats, 
                               bool                    writexmlpal, 
                               std::atomic<uint32_t> * progresscnt )
    {
        SpriteXMLWriter(spr).WriteXMLFiles(spriteFolderPath, stats, writexmlpal, progresscnt);
    }


    /**************************************************************
    **************************************************************/
    eSpriteImgType QuerySpriteImgTypeFromDirectory( const std::string & dirpath )
    {
        using namespace pugi;
        ifstream          in( Poco::Path(dirpath).append( SPRITE_Properties_fname ).toString() );

        if( in.bad() || !in.is_open() )
            throw std::runtime_error( SPRITE_Properties_fname + " cannot be opened !" );

        xml_document doc;
        if( ! doc.load(in) )
            throw std::runtime_error( SPRITE_Properties_fname + " cannot be opened !" );

        bool bIs256Colors = false;
        
        xml_node typenode = doc.find_node( [](xml_node & anode){ return anode.name() == SpriteXMLStrings::XML_PROP_IS256COL; });

        if( typenode.begin() != typenode.end() )
        {
            _parseXMLHexaValToValue(typenode.child_value(), bIs256Colors );
        }
        else
            throw std::runtime_error("The " +SpriteXMLStrings::XML_PROP_IS256COL+ " element is mising from the xml data, impossible to determine type!" );


        return (bIs256Colors)? eSpriteImgType::spr8bpp : eSpriteImgType::spr4bpp;
    }

};};