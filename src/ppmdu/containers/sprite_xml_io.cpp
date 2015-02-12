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
#include <Poco/File.h>
#include <Poco/Path.h>

using namespace std;
using utils::io::eSUPPORT_IMG_IO;
using namespace Poco::XML;

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

    const std::string SprInfo::DESC_Is8WaySprite    = "If 1 character sprite. If 0, other sprite. This messes with what anim groups are used for!";
    const std::string SprInfo::DESC_Is256Sprite     = "If 1, the game draw the sprite as a 8bpp 256 color sprite from memory!(You need to specify it in the palette info too for it to work!)\n"
                                                      "       If 0, images are drawn as 4bpp !";
    const std::string SprInfo::DESC_IsMosaicSpr     = "If 1, load the first row of tiles of each images one after the other, the the second, and so on. Seems to be for very large animated sprites!";
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
        void _parseXMLValToValue( const Poco::XML::XMLString & str, _Ty out_val )
    {
        stringstream sstr(str);
        sstr <<dec << str;
        sstr >> out_val;
    }

    /**************************************************************
        Parse a value that might be an hex number or not.
    **************************************************************/
    template<class _Ty>
        void _parseXMLHexaValToValue( const Poco::XML::XMLString & str, _Ty & out_val )
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
    string _parseNameAttribute( Poco::XML::NamedNodeMap * attribmap )
    {
        AutoPtr<NamedNodeMap> nmap = attribmap;
        for( unsigned long ctatt = 0; ctatt < nmap->length(); ++ctatt )
        {
            Node * pAtt = nmap->item(ctatt);
            if( pAtt->nodeName() == SpriteXMLStrings::XML_ATTR_NAME )
                return pAtt->innerText();
        }
        return string();
    }

//=============================================================================================
//  Sprite XML Templates Reader
//=============================================================================================

    /**************************************************************
        A class that reads the xml template files for naming animation groups properly
    **************************************************************/
    class SpriteTemplateReader
    {
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
        static const unsigned int MY_NODE_FILTER = NodeFilter::SHOW_ATTRIBUTE | NodeFilter::SHOW_ELEMENT;
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
        void ParseXML( Poco::Path sprrootfolder, bool parsexmlpal )
        {
            using namespace SpriteXMLStrings;
            std::ifstream inProperties( Poco::Path(sprrootfolder).append(SPRITE_Properties_fname).toString() );
            std::ifstream inAnims     ( Poco::Path(sprrootfolder).append(SPRITE_Animations_fname).toString() );
            std::ifstream inMFrames   ( Poco::Path(sprrootfolder).append(SPRITE_Frames_fname    ).toString() );
            std::ifstream inOffsets   ( Poco::Path(sprrootfolder).append(SPRITE_Offsets_fname   ).toString() );

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
        tmpoffset ParseAnimFrmOffsets( Poco::XML::NodeList * pNodeLst )
        {
            tmpoffset         res       = {0,0};
            AutoPtr<NodeList> pOffNodes = pNodeLst;

            for( unsigned long ctoff = 0; ctoff < pOffNodes->length(); ++ctoff )
            {
                Node * pCurOffnode = pOffNodes->item(ctoff);

                if( pCurOffnode->nodeName() == SpriteXMLStrings::XML_PROP_OFFSETX )
                        _parseXMLHexaValToValue( pCurOffnode->innerText(), res.x );
                else if( pCurOffnode->nodeName() == SpriteXMLStrings::XML_PROP_OFFSETY )
                    _parseXMLHexaValToValue( pCurOffnode->innerText(), res.y );
            }

            return std::move(res);
        }

        /**************************************************************
        **************************************************************/
        AnimFrame ParseAnimationFrame( Poco::XML::Node * pFrmNode )
        {
            AnimFrame         afrm;
            AutoPtr<NodeList> pFrmProp = pFrmNode->childNodes();
            
            for( unsigned long ctfrmprop = 0; ctfrmprop < pFrmProp->length(); ++ctfrmprop )
            {
                Node * pCurProp = pFrmProp->item( ctfrmprop );

                if( pCurProp->nodeName() == SpriteXMLStrings::XML_PROP_DURATION )
                {
                    _parseXMLHexaValToValue( pCurProp->innerText(), afrm.frameDuration );
                }
                else if( pCurProp->nodeName() == SpriteXMLStrings::XML_PROP_METAINDEX )
                    _parseXMLHexaValToValue( pCurProp->innerText(), afrm.metaFrmGrpIndex );
                else if( pCurProp->nodeName() == SpriteXMLStrings::XML_NODE_SPRITE && pCurProp->hasChildNodes() )
                {
                    tmpoffset result = ParseAnimFrmOffsets( pCurProp->childNodes() );
                    afrm.sprOffsetX = result.x;
                    afrm.sprOffsetY = result.y;
                }
                else if( pCurProp->nodeName() == SpriteXMLStrings::XML_NODE_SHADOW && pCurProp->hasChildNodes() )
                {
                    tmpoffset result = ParseAnimFrmOffsets( pCurProp->childNodes() );
                    afrm.shadowOffsetX = result.x;
                    afrm.shadowOffsetY = result.y;
                }
            }

            return std::move(afrm);
        }

        /**************************************************************
        **************************************************************/
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

                    if( pCurFrm->nodeName() == SpriteXMLStrings::XML_NODE_ANIMFRM && pCurFrm->hasChildNodes() )
                    {   
                        aseq.insertFrame( ParseAnimationFrame( pCurFrm ) );
                    }
                }
            }

            return std::move(aseq);
        }

        /**************************************************************
        **************************************************************/
        vector<AnimationSequence> ParseAnimationSequences( Poco::XML::NodeList * pNodeLst )
        {
            vector<AnimationSequence> seqs;
            AutoPtr<NodeList>         pSeqs = pNodeLst;

            for( unsigned long ctseqs = 0; ctseqs < pNodeLst->length(); ++ctseqs )
            {
                Node * pCurSeq = pNodeLst->item(ctseqs);

                if( pCurSeq->nodeName() == SpriteXMLStrings::XML_NODE_ANIMSEQ )
                {
                    seqs.push_back( ParseAnimationSequence( pCurSeq ) );
                }
            }

            return std::move(seqs);
        }

        /**************************************************************
        **************************************************************/
        vector<SpriteAnimationGroup> ParseAnimationGroups( Poco::XML::NodeList * pNodeLst )
        {
            vector<SpriteAnimationGroup> grps;
            AutoPtr<NodeList>            pGrps = pNodeLst;

            for( unsigned long ctgrp = 0; ctgrp < pGrps->length(); ++ctgrp )
            {
                auto * pCurNode = pGrps->item(ctgrp);

                if( pCurNode->nodeName() == SpriteXMLStrings::XML_NODE_ANIMGRP )
                {
                    SpriteAnimationGroup agrp;

                    //Get the name
                    if( pCurNode->hasAttributes() )
                        agrp.group_name = _parseNameAttribute( pCurNode->attributes() );

                    //If we got anim sequences ref get them !
                    if( pCurNode->hasChildNodes() )
                    {
                        //Parse the sequences indexes
                        AutoPtr<NodeList>  pSeqs = pCurNode->childNodes();

                        for( unsigned long ctseqs = 0; ctseqs < pSeqs->length(); ++ctseqs )
                        {
                            if( pSeqs->item(ctseqs)->nodeName() == SpriteXMLStrings::XML_PROP_ANIMSEQIND )
                            {
                                uint32_t index = 0;
                                _parseXMLHexaValToValue( pSeqs->item(ctseqs)->innerText(), index );
                                agrp.seqsIndexes.push_back(index);
                            }
                        }
                    }
                    //Add an empty group if we don't have any sequences !
                    grps.push_back( std::move(agrp) );
                }
            }
            return std::move( grps );
        }

        /**************************************************************
        **************************************************************/
        void ParseAnimations(std::ifstream & in)
        {
            //utils::MrChronometer chronoxmlanims("ParseXMLAnims");
            using namespace Poco::XML;
            InputSource       src(in);
            DOMParser         parser;
            AutoPtr<Document> pAnimGrps = parser.parse(&src);
            NodeIterator      itnode( pAnimGrps, MY_NODE_FILTER );
            Node             *pCurNode = itnode.nextNode(); //Get the first node loaded

            //Read every elements
            while( pCurNode != nullptr )
            {
                if( pCurNode->nodeName() == SpriteXMLStrings::XML_NODE_ANIMGRPTBL && pCurNode->hasChildNodes() )
                {
                    m_pOutSprite->getAnimGroups() = ParseAnimationGroups( pCurNode->childNodes() );
                }
                else if( pCurNode->nodeName() == SpriteXMLStrings::XML_NODE_ANIMSEQTBL && pCurNode->hasChildNodes() )
                {
                    m_pOutSprite->getAnimSequences() = ParseAnimationSequences( pCurNode->childNodes() );
                }

                pCurNode = itnode.nextNode();
            }
        }

        /**************************************************************
        **************************************************************/
        void ParseOffsets(std::ifstream & in)
        {
            //utils::MrChronometer chrono("ParseXMLSpriteOffsets");
            using namespace Poco::XML;
            InputSource       src(in);
            DOMParser         parser;
            AutoPtr<Document> pOffsets = parser.parse(&src);
            NodeIterator      itnode( pOffsets, MY_NODE_FILTER );
            Node             *pCurNode = itnode.nextNode(); //Get the first node loaded

            //Read every elements
            while( pCurNode != nullptr )
            {
                if( pCurNode->nodeName() == SpriteXMLStrings::XML_NODE_OFFSET && pCurNode->hasChildNodes() )
                {
                    AutoPtr<NodeList> offsetsndl = pCurNode->childNodes();
                    sprOffParticle    anoffs;

                    for( unsigned long ctoffc = 0; ctoffc < offsetsndl->length(); ++ctoffc )
                    {
                        Node * pOffNode = offsetsndl->item(ctoffc);
                        if( pOffNode->nodeName() == SpriteXMLStrings::XML_PROP_X )
                            _parseXMLHexaValToValue( pOffNode->innerText(), anoffs.offx );
                        else if( pOffNode->nodeName() == SpriteXMLStrings::XML_PROP_Y )
                            _parseXMLHexaValToValue( pOffNode->innerText(), anoffs.offy );
                    }
                    m_pOutSprite->getPartOffsets().push_back(anoffs);
                }
                pCurNode = itnode.nextNode();
            }
        }

        /**************************************************************
        **************************************************************/
        MetaFrame::eRes ParseMetaFrameRes( NodeList * nodelst )
        {
            AutoPtr<NodeList> resnodelst = nodelst;
            utils::Resolution myres      = {0,0};
            MetaFrame::eRes   result     = MetaFrame::eRes::_INVALID;

            for( unsigned long ctres = 0; ctres < resnodelst->length(); ++ctres  )
            {
                Node * pResChild = resnodelst->item(ctres);
                if( pResChild->nodeName() == SpriteXMLStrings::XML_PROP_WIDTH )
                    _parseXMLHexaValToValue( pResChild->innerText(), myres.width );
                else if( pResChild->nodeName() == SpriteXMLStrings::XML_PROP_HEIGTH )
                    _parseXMLHexaValToValue( pResChild->innerText(), myres.height );
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
        MetaFrame ParseAMetaFrame( Poco::XML::Node * pFrmNode )
        {
            MetaFrame         mf;
            AutoPtr<NodeList> nodelst = pFrmNode->childNodes();

            for( unsigned long ctmfprop = 0; ctmfprop < nodelst->length(); ++ctmfprop )
            {
                Node * pPropNode = nodelst->item(ctmfprop);

                if( pPropNode->nodeName() == SpriteXMLStrings::XML_PROP_FRMINDEX )
                {
                    _parseXMLHexaValToValue( pPropNode->innerText(), mf.imageIndex );
                }
                else if( pPropNode->nodeName() == SpriteXMLStrings::XML_PROP_UNK0 )
                    _parseXMLHexaValToValue( pPropNode->innerText(), mf.unk0 );
                else if( pPropNode->nodeName() == SpriteXMLStrings::XML_PROP_OFFSETX )
                    _parseXMLHexaValToValue( pPropNode->innerText(), mf.offsetX );
                else if( pPropNode->nodeName() == SpriteXMLStrings::XML_PROP_OFFSETY )
                    _parseXMLHexaValToValue( pPropNode->innerText(), mf.offsetY );
                else if( pPropNode->nodeName() == SpriteXMLStrings::XML_PROP_UNK1 )
                    _parseXMLHexaValToValue( pPropNode->innerText(), mf.unk1 );
                else if( pPropNode->nodeName() == SpriteXMLStrings::XML_PROP_VFLIP )
                    _parseXMLHexaValToValue( pPropNode->innerText(), mf.vFlip );
                else if( pPropNode->nodeName() == SpriteXMLStrings::XML_PROP_HFLIP )
                    _parseXMLHexaValToValue( pPropNode->innerText(), mf.hFlip );
                else if( pPropNode->nodeName() == SpriteXMLStrings::XML_PROP_MOSAIC )
                    _parseXMLHexaValToValue( pPropNode->innerText(), mf.Mosaic );
                //else if( pPropNode->nodeName() == SpriteXMLStrings::XML_PROP_LASTMFRM )
                //    _parseXMLHexaValToValue( pPropNode->innerText(), mf.isLastMFrmInGrp );
                else if( pPropNode->nodeName() == SpriteXMLStrings::XML_PROP_OFFXBIT6 )
                    _parseXMLHexaValToValue( pPropNode->innerText(), mf.XOffbit6 );
                else if( pPropNode->nodeName() == SpriteXMLStrings::XML_PROP_OFFXBIT7 )
                    _parseXMLHexaValToValue( pPropNode->innerText(), mf.XOffbit7 );
                else if( pPropNode->nodeName() == SpriteXMLStrings::XML_PROP_OFFYBIT3 )
                    _parseXMLHexaValToValue( pPropNode->innerText(), mf.YOffbit3 );
                else if( pPropNode->nodeName() == SpriteXMLStrings::XML_PROP_OFFYBIT5 )
                    _parseXMLHexaValToValue( pPropNode->innerText(), mf.YOffbit5 );
                else if( pPropNode->nodeName() == SpriteXMLStrings::XML_PROP_OFFYBIT6 )
                    _parseXMLHexaValToValue( pPropNode->innerText(), mf.YOffbit6 );
                else if( pPropNode->nodeName() == SpriteXMLStrings::XML_NODE_RES && pPropNode->hasChildNodes() )
                {
                    mf.resolution = ParseMetaFrameRes( pPropNode->childNodes() );
                    //AutoPtr<NodeList> resnodelst = pPropNode->childNodes();
                    //utils::Resolution myres={0,0};

                    //for( unsigned long ctres = 0; ctres < resnodelst->length(); ++ctres  )
                    //{
                    //    Node * pResChild = resnodelst->item(ctres);
                    //    if( pResChild->nodeName() == SpriteXMLStrings::XML_PROP_WIDTH )
                    //        _parseXMLHexaValToValue( pResChild->innerText(), myres.width );
                    //    else if( pResChild->nodeName() == SpriteXMLStrings::XML_PROP_HEIGTH )
                    //        _parseXMLHexaValToValue( pResChild->innerText(), myres.height );
                    //}

                    //mf.resolution = MetaFrame::IntegerResTo_eRes( myres.width, myres.height );

                    //if( mf.resolution == MetaFrame::eRes::_INVALID )
                    //{
                    //    stringstream sstr;
                    //    sstr << "Error: invalid resolution specified for a meta-frame!\n"
                    //         << "Got " <<myres.width << "x" <<myres.height << " for meta-frame #" <<m_outSprite.m_metaframes.size()
                    //         << ", referred to in meta-frame group #" <<m_outSprite.m_metafrmsgroups.size() <<" !\n"
                    //         << "Valid resolutions are any of the following :\n";

                    //    for( const auto & theentry : graphics::MetaFrame::ResEquiv )
                    //        sstr << "-> " <<theentry.x <<"x" <<theentry.y <<"\n";

                    //    throw std::runtime_error( sstr.str() );
                    //}
                }
            }

            return std::move(mf);
        }

        /**************************************************************
        **************************************************************/
        void ParseMetaFrames(std::ifstream & in)
        {
            //utils::MrChronometer chrono("ParseXMLSpriteMetaFrames");
            using namespace Poco::XML;
            InputSource       src(in);
            DOMParser         parser;
            AutoPtr<Document> pMetaFrms = parser.parse(&src);
            NodeIterator      itnode( pMetaFrms, MY_NODE_FILTER );
            Node             *pCurNode = itnode.nextNode(); //Get the first node loaded

            //Read every elements
            while( pCurNode != nullptr )
            {
                //Get a frame group first !
                if( pCurNode->nodeName() == SpriteXMLStrings::XML_NODE_FRMGRP )
                {
                    if( pCurNode->hasChildNodes() )
                    {
                        AutoPtr<NodeList> nodelst = pCurNode->childNodes();
                        MetaFrameGroup mfg; //Create the meta-frame group
                        mfg.metaframes.reserve( nodelst->length() );

                        //Read all frames in the group
                        for( unsigned long ctmf = 0; ctmf < nodelst->length(); ++ctmf )
                        {
                            if( nodelst->item(ctmf)->nodeName() == SpriteXMLStrings::XML_NODE_FRMFRM && nodelst->item(ctmf)->hasChildNodes() )
                            {
                                mfg.metaframes.push_back( m_pOutSprite->getMetaFrames().size() ); //put the frame's offset in the meta frame group table
                                m_pOutSprite->getMetaFrames().push_back( ParseAMetaFrame( nodelst->item(ctmf) ) );
                            }
                        }

                        m_pOutSprite->getMetaFrmsGrps().push_back( std::move(mfg) );
                        /*std::swap( mfg, MetaFrameGroup() );*/
                    }
                    else
                        assert(false); //What to do with frm groups with no childs ? //#TODO: do something with this !!
                }
                pCurNode = itnode.nextNode();
            }
        }

        /**************************************************************
        **************************************************************/
        void ParseSpriteInfo(std::ifstream & in)
        {
            //utils::MrChronometer chrono("ParseXMLSpriteInfo");
            using namespace SpriteXMLStrings;
            using namespace Poco::XML;
            InputSource       src(in);
            DOMParser         parser;
            AutoPtr<Document> pSprinfo = parser.parse(&src);
            NodeIterator      itnode( pSprinfo, MY_NODE_FILTER /*| NodeFilter::SHOW_TEXT*/ );
            Node             *pCurNode = itnode.nextNode(); //Get the first node loaded

            //Read every elements
            while( pCurNode != nullptr )
            {
                if( pCurNode->nodeName() == XML_PROP_UNK3 )
                {
                    _parseXMLHexaValToValue( pCurNode->innerText(), m_pOutSprite->getSprInfo().m_Unk3 );
                }
                else if( pCurNode->nodeName() == XML_PROP_COLPERROW )
                    _parseXMLHexaValToValue( pCurNode->innerText(), m_pOutSprite->getSprInfo().m_nbColorsPerRow );
                else if( pCurNode->nodeName() == XML_PROP_UNK4 )
                    _parseXMLHexaValToValue( pCurNode->innerText(), m_pOutSprite->getSprInfo().m_Unk4 );
                else if( pCurNode->nodeName() == XML_PROP_UNK5 )
                    _parseXMLHexaValToValue( pCurNode->innerText(), m_pOutSprite->getSprInfo().m_Unk5 );
                else if( pCurNode->nodeName() == XML_PROP_UNK6 )
                    _parseXMLHexaValToValue( pCurNode->innerText(), m_pOutSprite->getSprInfo().m_Unk6 );
                else if( pCurNode->nodeName() == XML_PROP_UNK7 )
                    _parseXMLHexaValToValue( pCurNode->innerText(), m_pOutSprite->getSprInfo().m_Unk7 );
                else if( pCurNode->nodeName() == XML_PROP_UNK8 )
                    _parseXMLHexaValToValue( pCurNode->innerText(), m_pOutSprite->getSprInfo().m_Unk8 );
                else if( pCurNode->nodeName() == XML_PROP_UNK9 )
                    _parseXMLHexaValToValue( pCurNode->innerText(), m_pOutSprite->getSprInfo().m_Unk9 );
                else if( pCurNode->nodeName() == XML_PROP_UNK10 )
                    _parseXMLHexaValToValue( pCurNode->innerText(), m_pOutSprite->getSprInfo().m_Unk10 );
                else if( pCurNode->nodeName() == XML_PROP_IS8W )
                    _parseXMLHexaValToValue( pCurNode->innerText(), m_pOutSprite->getSprInfo().m_is8WaySprite );
                else if( pCurNode->nodeName() == XML_PROP_IS256COL )
                    _parseXMLHexaValToValue( pCurNode->innerText(), m_pOutSprite->getSprInfo().m_is256Sprite );
                else if( pCurNode->nodeName() == XML_PROP_ISMOSAICS )
                    _parseXMLHexaValToValue( pCurNode->innerText(), m_pOutSprite->getSprInfo().m_IsMosaicSpr );
                else if( pCurNode->nodeName() == XML_PROP_UNK11 )
                    _parseXMLHexaValToValue( pCurNode->innerText(), m_pOutSprite->getSprInfo().m_Unk11 );
                else if( pCurNode->nodeName() == XML_PROP_UNK12 )
                    _parseXMLHexaValToValue( pCurNode->innerText(), m_pOutSprite->getSprInfo().m_Unk12 );

                pCurNode = itnode.nextNode();
            }
        }

        /**************************************************************
        **************************************************************/
        void ParsePalette(std::ifstream & in)
        {
            using namespace Poco::XML;
            InputSource       src(in);
            DOMParser         parser;
            AutoPtr<Document> pPalette = parser.parse(&src);
            NodeIterator      itnode( pPalette, MY_NODE_FILTER );
            Node             *pCurNode = itnode.nextNode(); //Get the first node loaded

            //Read every elements
            while( pCurNode != nullptr )
            {
                if( pCurNode->nodeName() == SpriteXMLStrings::XML_NODE_COLOR )
                {
                    graphics::colRGB24 mycolor;
                    if( pCurNode->hasChildNodes() )
                    {
                        AutoPtr<NodeList> nodelst = pCurNode->childNodes();

                        //Copy the content of the color
                        for( unsigned long ctchn = 0; ctchn < nodelst->length(); ++ctchn )
                        {
                            Node * pChild = nodelst->item(ctchn);
                            if( pChild->nodeName() == SpriteXMLStrings::XML_PROP_RED )
                                _parseXMLHexaValToValue( pChild->innerText(), mycolor.red ); 
                            else if( pChild->nodeName() == SpriteXMLStrings::XML_PROP_GREEN )
                                _parseXMLHexaValToValue( pChild->innerText(), mycolor.green ); 
                            else if( pChild->nodeName() == SpriteXMLStrings::XML_PROP_BLUE )
                                _parseXMLHexaValToValue( pChild->innerText(), mycolor.blue ); 
                        }
                    }
                    m_pOutSprite->getPalette().push_back(mycolor);
                }
                pCurNode = itnode.nextNode();
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
        static const unsigned int MY_WRITER_FLAGS = XMLWriter::WRITE_XML_DECLARATION | XMLWriter::PRETTY_PRINT; 
    public:

        /**************************************************************
        **************************************************************/
        SpriteXMLWriter( const BaseSprite * myspr)
            :m_pInSprite(myspr), m_pProgresscnt(nullptr)
        {
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

            //Write xml palette if needed
            if( xmlcolorpal )
                WritePalette();
        }

    private:
        //
        //  Common Stuff
        //
        /**************************************************************
        **************************************************************/
        inline void InitWriter( Poco::XML::XMLWriter & writer, const std::string & rootnodename )
        {
            writer.setNewLine("\n");
            writer.startDocument();
            writer.startElement( "", "", rootnodename );
        }

        /**************************************************************
        **************************************************************/
        inline void DeinitWriter( Poco::XML::XMLWriter& writer, const std::string & rootnodename )
        {
            writer.endElement( "", "", rootnodename );
            writer.endDocument();
        }

        /**************************************************************
        **************************************************************/
        template<typename _myintt>
            std::string turnIntToHexStr( _myintt anint )
        {
            using SpriteXMLStrings::PARSE_HEX_NUMBER;
            stringstream sstr;
            sstr <<PARSE_HEX_NUMBER << hex <<anint;
            return sstr.str();
        }

        /**************************************************************
        **************************************************************/
        inline void writeComment( XMLWriter & writer, const string & str )
        {
            writer.comment( str.c_str(), 0, str.size() );
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
            Poco::Path outpath = Poco::Path(m_outPath).append(SPRITE_Properties_fname);
            ofstream   outfile( outpath.toString() );
            XMLWriter  writer(outfile, MY_WRITER_FLAGS );
                
            InitWriter( writer, XML_ROOT_SPRPROPS );
            {
                //Color stuff
                writeComment( writer, SprInfo::DESC_Unk3 );
                writer.dataElement( "", "", XML_PROP_UNK3,      turnIntToHexStr( m_pInSprite->getSprInfo().m_Unk3 ) );

                writeComment( writer, SprInfo::DESC_nbColorsPerRow );
                writer.dataElement( "", "", XML_PROP_COLPERROW, std::to_string( m_pInSprite->getSprInfo().m_nbColorsPerRow ) );

                writeComment( writer, SprInfo::DESC_Unk4 );
                writer.dataElement( "", "", XML_PROP_UNK4,      turnIntToHexStr( m_pInSprite->getSprInfo().m_Unk4 ) );

                writeComment( writer, SprInfo::DESC_Unk5 );
                writer.dataElement( "", "", XML_PROP_UNK5,      turnIntToHexStr( m_pInSprite->getSprInfo().m_Unk5 ) );

                //Anim Stuff
                writeComment( writer, SprInfo::DESC_Unk6 );
                writer.dataElement( "", "", XML_PROP_UNK6,      turnIntToHexStr( m_pInSprite->getSprInfo().m_Unk6 ) );

                writeComment( writer, SprInfo::DESC_Unk7 );
                writer.dataElement( "", "", XML_PROP_UNK7,      turnIntToHexStr( m_pInSprite->getSprInfo().m_Unk7 ) );

                writeComment( writer, SprInfo::DESC_Unk8 );
                writer.dataElement( "", "", XML_PROP_UNK8,      turnIntToHexStr( m_pInSprite->getSprInfo().m_Unk8 ) );

                writeComment( writer, SprInfo::DESC_Unk9 );
                writer.dataElement( "", "", XML_PROP_UNK9,      turnIntToHexStr( m_pInSprite->getSprInfo().m_Unk9 ) );

                writeComment( writer, SprInfo::DESC_Unk10 );
                writer.dataElement( "", "", XML_PROP_UNK10,      turnIntToHexStr( m_pInSprite->getSprInfo().m_Unk10 ) );

                //Other properties
                writeComment( writer, SprInfo::DESC_Is8WaySprite );
                writer.dataElement( "", "", XML_PROP_IS8W,      std::to_string( m_pInSprite->getSprInfo().m_is8WaySprite ) );

                writeComment( writer, SprInfo::DESC_Is256Sprite );
                writer.dataElement( "", "", XML_PROP_IS256COL,  std::to_string( m_pInSprite->getSprInfo().m_is256Sprite ) );

                writeComment( writer, SprInfo::DESC_IsMosaicSpr );
                writer.dataElement( "", "", XML_PROP_ISMOSAICS, std::to_string( m_pInSprite->getSprInfo().m_IsMosaicSpr ) );

                writeComment( writer, SprInfo::DESC_Unk11 );
                writer.dataElement( "", "", XML_PROP_UNK11,     turnIntToHexStr( m_pInSprite->getSprInfo().m_Unk11 ) );

                writeComment( writer, SprInfo::DESC_Unk12 );
                writer.dataElement( "", "", XML_PROP_UNK12,     turnIntToHexStr( m_pInSprite->getSprInfo().m_Unk12 ) );
            }
            DeinitWriter( writer, XML_ROOT_SPRPROPS );
        }

        /**************************************************************
        **************************************************************/
        void WritePalette()
        {
            using namespace SpriteXMLStrings;
            Poco::Path outpath = Poco::Path(m_outPath).append(SPRITE_Palette_fname);
            ofstream   outfile( outpath.toString() );
            XMLWriter writer(outfile, MY_WRITER_FLAGS );
            
            InitWriter( writer, XML_NODE_PALLETTE );

            stringstream strs;
            strs <<"Total nb of color(s) : " <<setw(4) <<setfill(' ') <<m_pInSprite->getPalette().size();
            writeComment( writer, strs.str() );
            strs = stringstream();

            for( unsigned int i = 0; i < m_pInSprite->getPalette().size(); ++i )
            {
                const auto & acolor = m_pInSprite->getPalette()[i];
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

        /**************************************************************
        **************************************************************/
        void WriteAnimFrame( Poco::XML::XMLWriter & writer, const AnimFrame & curfrm )
        {
            using namespace SpriteXMLStrings;
            //stringstream sstrconv;
            //string result;

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

        /**************************************************************
        **************************************************************/
        void WriteAnimSequence( Poco::XML::XMLWriter & writer, const AnimationSequence & aseq )
        {
            using namespace SpriteXMLStrings;

            //stringstream strs;

            //Give this sequence a name 
            AttributesImpl attr;
            attr.addAttribute("","", XML_ATTR_NAME, "string", aseq.getName() );
            writer.startElement("","", XML_NODE_ANIMSEQ, attr );

            //Write the content of each frame in that sequence
            for( unsigned int cptfrms = 0; cptfrms < aseq.getNbFrames(); ++cptfrms )
            {
                //strs <<"frm " <<cptfrms;
                writeComment( writer, ("frm " + to_string(cptfrms)) /*strs.str()*/ );
                //strs = stringstream();
                WriteAnimFrame(writer, aseq.getFrame(cptfrms));
            }

            writer.endElement("","", XML_NODE_ANIMSEQ );
        }

        /**************************************************************
            -pAnimNameList : First entry of heach sub-vector is the 
                             anim group name, anything after is anim 
                             sequence names!
        **************************************************************/
        void WriteAnimations( uint32_t proportionofwork, uint32_t totalnbseqs, uint32_t totalnbfrms )
        {
            using namespace SpriteXMLStrings;
            Poco::Path outpath = Poco::Path(m_outPath).append(SPRITE_Animations_fname);
            ofstream   outfile( outpath.toString() );
            XMLWriter  writer(outfile, MY_WRITER_FLAGS );
            uint32_t   saveprogress = 0;

            if( m_pProgresscnt != nullptr )
                saveprogress = m_pProgresscnt->load();

            InitWriter( writer, XML_ROOT_ANIMDAT );
            //Add some comments to help fellow humans 
            stringstream strs;
            strs <<"Total nb of animation group(s) : " <<setw(4) <<setfill(' ') <<m_pInSprite->getAnimGroups().size();
            writeComment( writer, strs.str() );

            strs = stringstream();
            strs <<"Total nb of sequence(s)        : " <<setw(4) <<setfill(' ') <<totalnbseqs;
            writeComment( writer, strs.str() );

            strs = stringstream();
            strs <<"Total nb of animation frame(s) : " <<setw(4) <<setfill(' ') <<totalnbfrms;
            writeComment( writer, strs.str() );

            {
                //First Write the GroupRefTable
                writer.startElement("","", XML_NODE_ANIMGRPTBL );
                {
                    unsigned int cptgrp = 0;
                    for( const auto & animgrp : m_pInSprite->getAnimGroups() )
                    {
                        strs = stringstream();
                        unsigned int cptseq = 0;
                        strs <<"Group #" <<cptgrp <<" contains " << animgrp.seqsIndexes.size() << " sequence(s)";
                        writeComment( writer, strs.str() );
                        

                        //Give this group a name
                        AttributesImpl attrname;
                        attrname.addAttribute("", "", XML_ATTR_NAME, "string", animgrp.group_name);
                        writer.startElement("","", XML_NODE_ANIMGRP, attrname );

                        //Write the content of each sequences in that group
                        for( const auto & aseq : animgrp.seqsIndexes )
                        {
                            strs = stringstream();
                            strs <<cptseq;
                            writer.dataElement("","", XML_PROP_ANIMSEQIND, std::to_string(aseq) );
                            ++cptseq;
                        }

                        writer.endElement("","", XML_NODE_ANIMGRP );
                        ++cptgrp;

                        if( m_pProgresscnt != nullptr )
                        {
                            uint32_t prog = ( ( proportionofwork * cptgrp ) / m_pInSprite->getAnimGroups().size() );
                            m_pProgresscnt->store( saveprogress + prog );
                        }
                    }
                }
                writer.endElement( "", "", XML_NODE_ANIMGRPTBL );

                //Second Write the SequenceTable
                writer.startElement("","", XML_NODE_ANIMSEQTBL );
                {
                    unsigned int cptaseqs = 0;

                    //Write the content of each group
                    for( const auto & animseq : m_pInSprite->getAnimSequences() )
                    {
                        //Write the content of each sequences in that group
                        strs = stringstream();
                        strs <<"Seq #" <<cptaseqs  <<" contains " <<animseq.getNbFrames()  << " frame(s)";
                        writeComment( writer, strs.str() );
                        WriteAnimSequence( writer, animseq );

                        ++cptaseqs;
                    }
                }
                writer.endElement( "", "", XML_NODE_ANIMSEQTBL );
            }
            DeinitWriter( writer, XML_ROOT_ANIMDAT );
        }

        /**************************************************************
        **************************************************************/
        void WriteMetaFrameGroups( uint32_t proportionofwork )
        {
            using namespace SpriteXMLStrings;
            Poco::Path outpath = Poco::Path(m_outPath).append(SPRITE_Frames_fname);
            ofstream   outfile( outpath.toString() );
            XMLWriter  writer(outfile, MY_WRITER_FLAGS );

            uint32_t saveprogress = 0;

            if( m_pProgresscnt != nullptr )
                saveprogress = m_pProgresscnt->load();

            InitWriter( writer, XML_ROOT_FRMLST );

            stringstream strs;
            strs <<"Total nb of group(s)      : " <<setw(4) <<setfill(' ') <<m_pInSprite->getMetaFrmsGrps().size();
            writeComment( writer, strs.str() );
            strs = stringstream();
            strs <<"Total nb of meta-frame(s) : " <<setw(4) <<setfill(' ') <<m_pInSprite->getMetaFrames().size();
            writeComment( writer, strs.str() );
            strs = stringstream();

            for( unsigned int i = 0; i < m_pInSprite->getMetaFrmsGrps().size(); ++i )
            {
                const auto & grp = m_pInSprite->getMetaFrmsGrps()[i];
                strs <<"MFG #" <<i <<" contains " <<grp.metaframes.size() <<" meta-frame(s)";
                writeComment( writer, strs.str() );
                strs = stringstream();

                WriteAMetaFrameGroup( writer, grp );

                if( m_pProgresscnt != nullptr )
                    m_pProgresscnt->store( saveprogress + ( proportionofwork * i ) / m_pInSprite->getMetaFrmsGrps().size() );
            }

            DeinitWriter( writer, XML_ROOT_FRMLST );
        }

        /**************************************************************
        **************************************************************/
        void WriteAMetaFrameGroup( XMLWriter & writer, const MetaFrameGroup & grp )
        {
            using namespace SpriteXMLStrings;
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

        /**************************************************************
        **************************************************************/
        void WriteMetaFrame( XMLWriter & writer, unsigned int index )
        {
            using namespace SpriteXMLStrings;
            const auto & aframe = m_pInSprite->getMetaFrames()[index];

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
                //writer.dataElement( "", "", XML_PROP_LASTMFRM, to_string( aframe.isLastMFrmInGrp ) );
                writer.dataElement( "", "", XML_PROP_OFFXBIT6, to_string( aframe.XOffbit6 ) );
                writer.dataElement( "", "", XML_PROP_OFFXBIT7, to_string( aframe.XOffbit7 ) );

                writer.dataElement( "", "", XML_PROP_OFFYBIT3, to_string( aframe.YOffbit3 ) );
                writer.dataElement( "", "", XML_PROP_OFFYBIT5, to_string( aframe.YOffbit5 ) );
                writer.dataElement( "", "", XML_PROP_OFFYBIT6, to_string( aframe.YOffbit6 ) );
            }
            writer.endElement("","", XML_NODE_FRMFRM );
        }

        /**************************************************************
        **************************************************************/
        void WriteOffsets( uint32_t proportionofwork )
        {
            using namespace Poco::XML;
            using namespace SpriteXMLStrings;
            Poco::Path outpath = Poco::Path(m_outPath).append(SPRITE_Offsets_fname);
            ofstream   outfile( outpath.toString() );
            XMLWriter writer(outfile, MY_WRITER_FLAGS );

            InitWriter( writer, XML_ROOT_OFFLST );

            stringstream strs;
            strs <<"Total nb of offset(s) : " <<setw(4) <<setfill(' ') <<m_pInSprite->getPartOffsets().size();
            writeComment( writer, strs.str() );

            uint32_t saveprogress = 0;

            if( m_pProgresscnt != nullptr )
                saveprogress = m_pProgresscnt->load();

            for( unsigned int i = 0; i < m_pInSprite->getPartOffsets().size(); ++i )
            {
                const auto & anoffset = m_pInSprite->getPartOffsets()[i];
                strs = stringstream();
                strs <<"#" <<i;
                writeComment( writer, strs.str() );
                

                writer.startElement("","", XML_NODE_OFFSET );
                {
                    writer.dataElement("","", XML_PROP_X, to_string( anoffset.offx ) );
                    writer.dataElement("","", XML_PROP_Y, to_string( anoffset.offy ) );
                }
                writer.endElement("","", XML_NODE_OFFSET );


                if( m_pProgresscnt != nullptr )
                    m_pProgresscnt->store( saveprogress + ( proportionofwork * i ) / m_pInSprite->getPartOffsets().size() );
            }

            DeinitWriter( writer, XML_ROOT_OFFLST );
        }

    private:
        Poco::Path              m_outPath;
        const BaseSprite      * m_pInSprite;
        std::atomic<uint32_t> * m_pProgresscnt;
        uint32_t                m_progressProportion; //The percentage of the entire work attributed to this
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
                               bool                parsexmlpal )
    {
        SpriteXMLParser(out_spr).ParseXML(spriteFolderPath, parsexmlpal);
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
    eSpriteType QuerySpriteTypeFromDirectory( const std::string & dirpath )
    {
        using namespace Poco::XML;
        ifstream          in( Poco::Path(dirpath).append( SPRITE_Properties_fname ).toString() );

        if( in.bad() || !in.is_open() )
            throw std::runtime_error( SPRITE_Properties_fname + " cannot be opened !" );

        InputSource       src(in);
        DOMParser         parser;
        AutoPtr<Document> pSprinfo = parser.parse(&src);
        NodeIterator      itnode( pSprinfo, NodeFilter::SHOW_ATTRIBUTE | NodeFilter::SHOW_ELEMENT | NodeFilter::SHOW_TEXT );
        Node             *pCurNode = itnode.nextNode(); //Get the first node loaded
        bool              bIs256Colors = false;


        //Read every elements
        while( pCurNode != nullptr )
        {
            if( pCurNode->nodeName() == SpriteXMLStrings::XML_PROP_IS256COL )
            {
                _parseXMLHexaValToValue(pCurNode->innerText(), bIs256Colors );
                break;
            }

            pCurNode = itnode.nextNode();
        }

        return (bIs256Colors)? eSpriteType::spr8bpp : eSpriteType::spr4bpp;
    }

};};