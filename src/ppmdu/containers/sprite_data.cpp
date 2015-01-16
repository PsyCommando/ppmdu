#include "sprite_data.hpp"
#include <string>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <atomic>
#include <ppmdu/containers/tiled_image.hpp>
#include <ppmdu/ext_fmts/png_io.hpp>
#include <ppmdu/ext_fmts/riff_palette.hpp>
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

using namespace std;
//using namespace utils;
using utils::io::eSUPPORT_IMG_IO;


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
        static const string XML_PROP_METAINDEX = "MetaFrameIndex";

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
    static const string SPRITE_IMGs_DIR   = "imgs"; //Name of the sub-folder for the images
    static const string Palette_Filename  = "palette.pal";

    const vector<string> SpriteBuildingRequiredFiles =
    {{
        xmlstrings::SPRITE_Properties,
        xmlstrings::SPRITE_Animations,
        xmlstrings::SPRITE_Frames,
        xmlstrings::SPRITE_Offsets,
        SPRITE_IMGs_DIR,
    }};


//=============================================================================================
//  Sprite to XML writer
//=============================================================================================
    template<class _SPRITE_t>
        class SpriteToXML
    {
    public:
        typedef _SPRITE_t sprite_t;

        SpriteToXML( const sprite_t & myspr )
            :m_inSprite(myspr), m_pProgresscnt(nullptr)
        {
        }

        //Write all the data for the Sprite as XML in the
        // specified folder  under ! 
        // -colorpalasxml : if true, will print an xml palette alongside the rest!
        void WriteXMLFiles( const string & folderpath, bool xmlcolorpal = false, std::atomic<uint32_t> * progresscnt = nullptr ) 
        {
            m_outPath = Poco::Path(folderpath);
            m_pProgresscnt = progresscnt;

            WriteProperties();
            if(m_pProgresscnt != nullptr )
                (*m_pProgresscnt) += 5;
            WriteAnimations();
            if(m_pProgresscnt != nullptr )
                (*m_pProgresscnt) += 5;
            WriteMetaFrames();
            if(m_pProgresscnt != nullptr )
                (*m_pProgresscnt) += 5;
            WriteOffsets();
            if(m_pProgresscnt != nullptr )
                (*m_pProgresscnt) += 5;

            //Write palette if needed
            if( xmlcolorpal )
            {
                WritePalette();
                if(m_pProgresscnt != nullptr )
                    (*m_pProgresscnt) += 5;
            }
        }

    private:
        //
        //  Common Stuff
        //
        void InitWriter( Poco::XML::XMLWriter & writer, const std::string & rootnodename )
        {
            using namespace Poco::XML;
            //writer = XMLWriter(strout, XMLWriter::WRITE_XML_DECLARATION | XMLWriter::PRETTY_PRINT );
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

    private:
        //
        //  Specific Stuff
        //

        void WriteProperties()
        {
            using namespace Poco::XML;
            using namespace xmlstrings;
            ofstream  outfile( m_outPath.setFileName(SPRITE_Properties).toString() );
            XMLWriter writer(outfile, XMLWriter::WRITE_XML_DECLARATION | XMLWriter::PRETTY_PRINT );
                
            InitWriter( writer, XML_NODE_SPRPROPS );
            {
                //Color stuff
                writer.comment( SprInfo::DESC_Unk3.c_str(), 0, SprInfo::DESC_Unk3.size() );
                writer.dataElement( "", "", XML_PROP_UNK3,      turnIntToHexStr( m_inSprite.getSprInfo().m_Unk3 ) );
                writer.comment( SprInfo::DESC_nbColorsPerRow.c_str(), 0, SprInfo::DESC_nbColorsPerRow.size() );
                writer.dataElement( "", "", XML_PROP_COLPERROW, std::to_string( m_inSprite.getSprInfo().m_nbColorsPerRow ) );
                writer.comment( SprInfo::DESC_Unk4.c_str(), 0, SprInfo::DESC_Unk4.size() );
                writer.dataElement( "", "", XML_PROP_UNK4,      turnIntToHexStr( m_inSprite.getSprInfo().m_Unk4 ) );
                writer.comment( SprInfo::DESC_Unk5.c_str(), 0, SprInfo::DESC_Unk5.size() );
                writer.dataElement( "", "", XML_PROP_UNK5,      turnIntToHexStr( m_inSprite.getSprInfo().m_Unk5 ) );

                //Anim Stuff
                writer.comment( SprInfo::DESC_Unk6.c_str(), 0, SprInfo::DESC_Unk6.size() );
                writer.dataElement( "", "", XML_PROP_UNK6,      turnIntToHexStr( m_inSprite.getSprInfo().m_Unk6 ) );
                writer.comment( SprInfo::DESC_Unk7.c_str(), 0, SprInfo::DESC_Unk7.size() );
                writer.dataElement( "", "", XML_PROP_UNK7, std::to_string( m_inSprite.getSprInfo().m_Unk7 ) );
                writer.comment( SprInfo::DESC_Unk8.c_str(), 0, SprInfo::DESC_Unk8.size() );
                writer.dataElement( "", "", XML_PROP_UNK8,      turnIntToHexStr( m_inSprite.getSprInfo().m_Unk8 ) );
                writer.comment( SprInfo::DESC_Unk9.c_str(), 0, SprInfo::DESC_Unk9.size() );
                writer.dataElement( "", "", XML_PROP_UNK9,      turnIntToHexStr( m_inSprite.getSprInfo().m_Unk9 ) );
                writer.comment( SprInfo::DESC_Unk10.c_str(), 0, SprInfo::DESC_Unk10.size() );
                writer.dataElement( "", "", XML_PROP_UNK10,      turnIntToHexStr( m_inSprite.getSprInfo().m_Unk10 ) );

                //Other properties
                writer.comment( SprInfo::DESC_Is8WaySprite.c_str(), 0, SprInfo::DESC_Is8WaySprite.size() );
                writer.dataElement( "", "", XML_PROP_IS8W,      std::to_string( m_inSprite.getSprInfo().m_is8WaySprite ) );
                writer.comment( SprInfo::DESC_Is256Sprite.c_str(), 0, SprInfo::DESC_Is256Sprite.size() );
                writer.dataElement( "", "", XML_PROP_IS256COL,  std::to_string( m_inSprite.getSprInfo().m_is256Sprite ) );
                writer.comment( SprInfo::DESC_IsMosaicSpr.c_str(), 0, SprInfo::DESC_IsMosaicSpr.size() );
                writer.dataElement( "", "", XML_PROP_ISMOSAICS, std::to_string( m_inSprite.getSprInfo().m_IsMosaicSpr ) );
                writer.comment( SprInfo::DESC_Unk11.c_str(), 0, SprInfo::DESC_Unk11.size() );
                writer.dataElement( "", "", XML_PROP_UNK11,     turnIntToHexStr( m_inSprite.getSprInfo().m_Unk11 ) );
                writer.comment( SprInfo::DESC_Unk12.c_str(), 0, SprInfo::DESC_Unk12.size() );
                writer.dataElement( "", "", XML_PROP_UNK12,     turnIntToHexStr( m_inSprite.getSprInfo().m_Unk12 ) );
            }
            DeinitWriter( writer, XML_NODE_SPRPROPS );
        }

        void WritePalette()
        {
            using namespace Poco::XML;
            using namespace xmlstrings;
            ofstream  outfile( m_outPath.setFileName(SPRITE_Palette).toString() );
            XMLWriter writer(outfile, XMLWriter::WRITE_XML_DECLARATION | XMLWriter::PRETTY_PRINT );
            
            InitWriter( writer, XML_NODE_PALLETTE );
            for( const auto & acolor : m_inSprite.getPalette() )
            {
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
            using namespace Poco::XML;
            using namespace xmlstrings;

            writer.startElement("","", XML_NODE_ANIMFRM );
            {
                writer.dataElement ("","", XML_PROP_DURATION,  to_string( curfrm.frame_duration ) );
                writer.dataElement ("","", XML_PROP_METAINDEX, to_string( curfrm.meta_frame_index ) );

                writer.startElement("","",XML_NODE_SPRITE);
                {
                    writer.dataElement ("","", XML_PROP_OFFSETX, to_string( curfrm.spr_offset_x ) );
                    writer.dataElement ("","", XML_PROP_OFFSETY, to_string( curfrm.spr_offset_y ) );
                }
                writer.endElement("","",XML_NODE_SPRITE);

                writer.startElement("","",XML_NODE_SHADOW);
                {
                    writer.dataElement ("","", XML_PROP_OFFSETX, to_string( curfrm.shadow_offset_x ) );
                    writer.dataElement ("","", XML_PROP_OFFSETY, to_string( curfrm.shadow_offset_y ) );
                }
                writer.endElement("","",XML_NODE_SHADOW);
            }
            writer.endElement("","", XML_NODE_ANIMFRM );
        }

        void WriteAnimSequece( Poco::XML::XMLWriter & writer, const AnimationSequence & aseq )
        {
            using namespace Poco::XML;
            using namespace xmlstrings;

            //Give this sequence a name if we have one to give it!
            //if( !aseq.getName().empty() )
            //{
                AttributesImpl attr;
                attr.addAttribute("","", XML_ATTR_NAME, "string", aseq.getName() );
                writer.startElement("","", XML_NODE_ANIMSEQ, attr );
            //}
            //else
            //    writer.startElement("","", XML_NODE_ANIMSEQ );
                        
            //Write the content of each frame in that sequence
            for( unsigned int cptfrms = 0; cptfrms < aseq.getNbFrames(); ++cptfrms )
                WriteAnimFrame(writer, aseq.getFrame(cptfrms));

            writer.endElement("","", XML_NODE_ANIMSEQ );
        }

        //-pAnimNameList : First entry of heach sub-vector is the anim group name, anything after is anim sequence names!
        void WriteAnimations()
        {
            using namespace Poco::XML;
            using namespace xmlstrings;
            ofstream  outfile( m_outPath.setFileName(SPRITE_Animations).toString() );
            XMLWriter writer(outfile, XMLWriter::WRITE_XML_DECLARATION | XMLWriter::PRETTY_PRINT );

            InitWriter( writer, XML_NODE_ANIMDAT );
            {
                unsigned int   cptgrp = 0;
                //AttributesImpl attrname;
                //attrname.addAttribute("", "", XML_ATTR_NAME, "string", "");

                //Write the content of each group
                for( const auto & animgrp : m_inSprite.getAnimGroups() )
                {
                    unsigned int cptseq = 0;

                    //Give this group a name if we have one to give it!
                    //if( !(animgrp.group_name.empty()) )
                    //{
                        AttributesImpl attrname;
                        attrname.addAttribute("", "", XML_ATTR_NAME, "string", animgrp.group_name);
                        writer.startElement("","", XML_NODE_ANIMGRP, attrname );
                    //}
                    //else
                    //    writer.startElement("","", XML_NODE_ANIMGRP );

                    //Write the content of each sequences in that group
                    for( const auto & aseq : animgrp.sequences )
                    {
                        WriteAnimSequece( writer, aseq );
                        ++cptseq;
                    }

                    writer.endElement("","", XML_NODE_ANIMGRP );
                    ++cptgrp;
                }
            }
            DeinitWriter( writer, XML_NODE_ANIMDAT );
        }



        void WriteMetaFrames()
        {
            using namespace Poco::XML;
            using namespace xmlstrings;
            ofstream  outfile( m_outPath.setFileName(SPRITE_Frames).toString() );
            XMLWriter writer(outfile, XMLWriter::WRITE_XML_DECLARATION | XMLWriter::PRETTY_PRINT );
            
            InitWriter( writer, XML_NODE_FRMLST );
            {
                for( const auto & aframe : m_inSprite.getMetaFrames() )
                {
                    writer.startElement("","", XML_NODE_FRMFRM );
                    {
                        writer.dataElement( "", "", XML_PROP_FRMINDEX, to_string( aframe.image_index ) );
                        writer.dataElement( "", "", XML_PROP_UNK0,     turnIntToHexStr( aframe.unk0 ) );
                        writer.dataElement( "", "", XML_PROP_OFFSETY,  to_string( aframe.offset_y ) );
                        writer.dataElement( "", "", XML_PROP_OFFSETX,  to_string( aframe.offset_x ) );
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
            }
            DeinitWriter( writer, XML_NODE_FRMLST );
        }

        void WriteOffsets()
        {
            using namespace Poco::XML;
            using namespace xmlstrings;
            ofstream  outfile( m_outPath.setFileName(SPRITE_Offsets).toString() );
            XMLWriter writer(outfile, XMLWriter::WRITE_XML_DECLARATION | XMLWriter::PRETTY_PRINT );

            InitWriter( writer, XML_NODE_OFFLST );
            {
                for( const auto & anoffset : m_inSprite.getPartOffsets() )
                {
                    writer.startElement("","", XML_NODE_OFFSET );
                    {
                        writer.dataElement("","", XML_PROP_X, to_string( anoffset.offx ) );
                        writer.dataElement("","", XML_PROP_Y, to_string( anoffset.offy ) );
                    }
                    writer.endElement("","", XML_NODE_OFFSET );
                }
            }
            DeinitWriter( writer, XML_NODE_OFFLST );
        }

    private:
        Poco::Path              m_outPath;
        const sprite_t        & m_inSprite;
        std::atomic<uint32_t> * m_pProgresscnt;
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

            ExportFrames(imgty);

            if( !xmlcolorpal )
                ExportPalette();

            ExportXMLData(xmlcolorpal);
        }

    private:

        void ExportFrames( eSUPPORT_IMG_IO imgty )
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
                    ExportFramesAsBMPs(imgdir);
                    break;
                }
                case eSUPPORT_IMG_IO::RAW:
                {
                    ExportFramesAsRawImgs(imgdir);
                    break;
                }
                case eSUPPORT_IMG_IO::PNG:
                default:
                {
                    ExportFramesAsPNGs(imgdir);
                }
            };
        }

        void ExportFramesAsPNGs( const Poco::Path & outdirpath )
        {
            const auto & frames = m_inSprite.getFrames();
            Poco::Path outimg(outdirpath);
            uint32_t progressBefore = 0; //Save a little snapshot of the progress
            if(m_pProgress!=nullptr)
            {
                progressBefore = m_pProgress->load();
            }

            for( unsigned int i = 0; i < frames.size(); ++i )
            {
                std::stringstream sstrname;
                sstrname <<setw(4) <<setfill('0') <<i <<"." <<utils::io::PNG_FileExtension;
                utils::io::ExportToPNG( frames[i], Poco::Path(outimg).append(sstrname.str()).toString() );

                if( m_pProgress != nullptr )
                {
                    m_pProgress->store( progressBefore + (PropCompl_Frames * (i+1) ) / frames.size() ); 
                }
            }
        }

        void ExportFramesAsBMPs( const Poco::Path & outdirpath )
        {
            const auto & frames = m_inSprite.getFrames();
            Poco::Path outimg(outdirpath);
            uint32_t progressBefore = 0; //Save a little snapshot of the progress
            if(m_pProgress!=nullptr)
            {
                progressBefore = m_pProgress->load();
            }

            for( unsigned int i = 0; i < frames.size(); ++i )
            {
                std::stringstream sstrname;
                sstrname <<setw(4) <<setfill('0') <<i <<"." <<utils::io::BMP_FileExtension;
                utils::io::ExportToBMP( frames[i], Poco::Path(outimg).append(sstrname.str()).toString() );

                if( m_pProgress != nullptr )
                {
                    m_pProgress->store( progressBefore + (PropCompl_Frames * (i+1)) / frames.size() );
                }
            }
        }

        void ExportFramesAsRawImgs( const Poco::Path & outdirpath )
        {
            const auto & frames = m_inSprite.getFrames();
            Poco::Path outimg(outdirpath);
            uint32_t progressBefore = 0; //Save a little snapshot of the progress
            if(m_pProgress!=nullptr)
            {
                progressBefore = m_pProgress->load();
            }

            for( unsigned int i = 0; i < frames.size(); ++i )
            {
                std::stringstream sstrname;
                sstrname <<setw(4) <<setfill('0') <<i <<"." <<utils::io::RawImg_FileExtension;
                utils::io::ExportRawImg( frames[i], Poco::Path(outimg).append(sstrname.str()).toString() );

                if( m_pProgress != nullptr )
                {
                    m_pProgress->store( progressBefore + (PropCompl_Frames * (i + 1)) / frames.size() );
                }
            }
        }

        void ExportXMLData(bool xmlcolpal)
        {
            SpriteToXML<sprite_t> mywriter(m_inSprite);
            mywriter.WriteXMLFiles( m_outDirPath.toString(), xmlcolpal, m_pProgress );
        }

        void ExportPalette()
        {
            using utils::io::ExportTo_RIFF_Palette;
            //Export as RIFF palette
            ExportTo_RIFF_Palette( m_inSprite.getPalette(), Poco::Path(m_outDirPath).setFileName(Palette_Filename).toString() );

            if( m_pProgress != nullptr )
            {
                m_pProgress->store( m_pProgress->load() + PropCompl_PAL );
            }
        }


    private:
        Poco::Path              m_outDirPath;
        const sprite_t        & m_inSprite;
        std::atomic<uint32_t> * m_pProgress;

        static const uint32_t PropCompl_Frames = 75; //75% of the job is writing the frames
        static const uint32_t PropCompl_XML    = 20; //20% of the job is writing the xml
        static const uint32_t PropCompl_PAL    = 5;  //5% of the job is writing the palette
    };

//=============================================================================================
//  Sprite to XML Reader
//=============================================================================================

//=============================================================================================
//  Sprite Builder
//=============================================================================================

//=============================================================================================
//  Type Camoflage Functions
//=============================================================================================
    void Export8bppSpriteToDirectory( const SpriteData<gimg::tiled_image_i8bpp> & srcspr, 
                                      const std::string                         & outpath,
                                      utils::io::eSUPPORT_IMG_IO                  imgtype,
                                      bool                                        usexmlpal)
    {
        SpriteToDirectory<SpriteData<gimg::tiled_image_i8bpp>> mywriter(srcspr);
        mywriter.WriteSpriteToDir(outpath,imgtype,usexmlpal);
    }

    void Export4bppSpriteToDirectory( const SpriteData<gimg::tiled_image_i4bpp> & srcspr, 
                                      const std::string                         & outpath,
                                      utils::io::eSUPPORT_IMG_IO                  imgtype,
                                      bool                                        usexmlpal)
    {
        SpriteToDirectory<SpriteData<gimg::tiled_image_i4bpp>> mywriter(srcspr);
        mywriter.WriteSpriteToDir(outpath,imgtype,usexmlpal);
    }

    //SpriteData<gimg::tiled_image_i8bpp> Import8bppSpriteFromDirectory( const std::string & inpath )
    //{
    //    assert(false);
    //    return SpriteData<gimg::tiled_image_i8bpp>();
    //}

    //SpriteData<gimg::tiled_image_i4bpp> Import4bppSpriteFromDirectory( const std::string & inpath )
    //{

    //}

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
        SpriteData<gimg::tiled_image_i4bpp> ImportSpriteFromDirectory( const std::string & inpath ) 
    { 
        assert(false);
        return SpriteData<gimg::tiled_image_i4bpp>();
        //return Import4bppSpriteFromDirectory(inpath); 
    }

    template<>
       SpriteData<gimg::tiled_image_i8bpp> ImportSpriteFromDirectory( const std::string & inpath )
    { 
        assert(false);
        return SpriteData<gimg::tiled_image_i8bpp>();
        //return Import8bppSpriteFromDirectory(inpath); 
    }

    bool AreReqFilesPresent_Sprite( const std::vector<std::string> & filelist )
    {
        for( const auto & filename : SpriteBuildingRequiredFiles )
        {
            auto itfound = std::find( filelist.begin(), filelist.end(), filename );

            if( itfound == filelist.end() )
                return false;
        }

        //The palette isn't required
        return true;
    }

//
//
//
    const std::string SprInfo::DESC_Unk3            = "If set to 1, write palette in \"Main Ext Spr 0\" runtime palette! Additionally, force sprite to be read to memory as 4 bpp!";
    const std::string SprInfo::DESC_nbColorsPerRow  = "0 to 16.. Changes the amount of colors loaded on a single row in the runtime palette sheet.";
    const std::string SprInfo::DESC_Unk4            = "In extended palette mode, seems to change the location of the palette..";
    const std::string SprInfo::DESC_Unk5            = "1111 0000 0000 0000)If not 0xF or 0, makes the image data be written to memory as 8 bpp, from 4 bpp! Also seem to force palette as 256 colors extended pal?\n"
                                                      "   Otherwise, the image is loaded as-is?\n"
                                                      "(0000 1111 0000 0000) Set in which slot in a palette sheet in memory the palette will be drawn at!\n";

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

};};