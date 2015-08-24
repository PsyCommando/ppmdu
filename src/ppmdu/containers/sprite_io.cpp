#include "sprite_io.hpp"
#include <ppmdu/containers/sprite_data.hpp>
#include <ppmdu/containers/tiled_image.hpp>
#include <ext_fmts/png_io.hpp>
#include <ext_fmts/riff_palette.hpp>
#include <utils/poco_wrapper.hpp>
#include <utils/library_wide.hpp>
#include <vector>
#include <string>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <fstream>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/Exception.h>
using namespace std;
using utils::io::eSUPPORT_IMG_IO;

namespace pmd2{ namespace graphics
{
    ////Filesnames
    //const std::string pmd2::graphics::SPRITE_Properties_fname  = "spriteinfo.xml";
    //const std::string pmd2::graphics::SPRITE_Palette_XML_fname = "palette.xml";
    //const std::string pmd2::graphics::SPRITE_Animations_fname  = "animations.xml";
    //const std::string pmd2::graphics::SPRITE_Frames_fname      = "frames.xml";
    //const std::string pmd2::graphics::SPRITE_Offsets_fname     = "offsets.xml";
    //const std::string pmd2::graphics::SPRITE_IMGs_DIR          = "imgs";         //Name of the sub-folder for the images
    //const std::string pmd2::graphics::SPRITE_Palette_fname     = "palette.pal";

    /*
        Definitions the list of required files for building a sprite from a folder !
    */
    const std::vector<std::string> SpriteBuildingRequiredFiles =
    {{
        SPRITE_Properties_fname,
        SPRITE_Animations_fname,
        SPRITE_Frames_fname,
        SPRITE_Offsets_fname,
        SPRITE_ImgsInfo_fname,
        SPRITE_IMGs_DIR+"/",
    }};

//=============================================================================================
// Utility Functions
//=============================================================================================

    /**************************************************************
    **************************************************************/
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

    /**************************************************************
    **************************************************************/
    bool AreReqFilesPresent_Sprite( const std::string & dirpath )
    {
        vector<string> dircontent = utils::ListDirContent_FilesAndDirs( dirpath, true );
        return AreReqFilesPresent_Sprite(dircontent);
    }

    /**************************************************************
    **************************************************************/
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

    /**************************************************************
    **************************************************************/
    std::vector<string> GetMissingRequiredFiles_Sprite( const std::string & dirpath )
    {
        vector<string> dircontent = utils::ListDirContent_FilesAndDirs( dirpath, true );
        return GetMissingRequiredFiles_Sprite( dircontent );
    }

    /**************************************************************
    **************************************************************/
    bool Sprite_IsResolutionValid( uint8_t width, uint8_t height )
    {
        return ( MetaFrame::IntegerResTo_eRes( width, height ) != MetaFrame::eRes::_INVALID );
    }


//=============================================================================================
//  Sprite to SpriteToDirectory Writer
//=============================================================================================
    template<class _SPRITE_t>
        class SpriteToDirectory
    {
    public:
        typedef _SPRITE_t sprite_t;

        /**************************************************************
        **************************************************************/
        SpriteToDirectory( const sprite_t & myspr )
            :m_inSprite(myspr)/*,m_pProgress(nullptr)*/
        {}

        /**************************************************************
        **************************************************************/
        void WriteSpriteToDir( const string          & folderpath, 
                               eSUPPORT_IMG_IO         imgty, 
                               bool                    xmlcolorpal = false/*, 
                               std::atomic<uint32_t> * progresscnt = nullptr*/ ) 
        {
            //Create Root Folder
            m_outDirPath = Poco::Path(folderpath);
            Poco::File directory(m_outDirPath);

            if( !directory.exists() )
                directory.createDirectory();

            if( utils::LibWide().isLogOn() )
                clog << "Output directory " <<directory.path() <<". Ok!\n";

            /*m_pProgress = progresscnt;*/

            //Count the ammount of entries for calculating work
            // and for adding statistics to the exported files.
            uint32_t totalnbseqs  = m_inSprite.getAnimSequences().size();
            uint32_t totalnbfrms  = 0;

            for( const auto & aseq : m_inSprite.getAnimSequences() )
                totalnbfrms += aseq.getNbFrames();
            
            spriteWorkStats stats;
            stats.totalAnimFrms = totalnbfrms;
            stats.totalAnimSeqs = totalnbseqs;

            ExportFrames(imgty, stats.propFrames );

            if( !xmlcolorpal )
                ExportPalette();

            ExportXMLData(xmlcolorpal, stats);
        }

    private:

        /**************************************************************
        **************************************************************/
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
#if 0
                case eSUPPORT_IMG_IO::RAW:
                {
                    ExportFramesAsRawImgs(imgdir, proportionofwork);
                    break;
                }
#endif
                case eSUPPORT_IMG_IO::PNG:
                default:
                {
                    ExportFramesAsPNGs(imgdir, proportionofwork);
                }
            };
        }

        /**************************************************************
        **************************************************************/
        void ExportFramesAsPNGs( const Poco::Path & outdirpath, uint32_t proportionofwork )
        {
            //uint32_t     progressBefore = 0; //Save a little snapshot of the progress
            const auto & frames         = m_inSprite.getFrames();
            Poco::Path   outimg(outdirpath);

            //if( m_pProgress != nullptr )
            //    progressBefore = m_pProgress->load();

            for( unsigned int i = 0; i < frames.size(); ++i )
            {
                std::stringstream sstrname;
                //Build filenmame
                sstrname <<setw(4) <<setfill('0') <<i <<"." <<utils::io::PNG_FileExtension;
                //Export
                utils::io::ExportToPNG( frames[i], Poco::Path(outimg).append(sstrname.str()).toString() );

                if( utils::LibWide().isLogOn() )
                    clog << "Exported frame " <<i <<": " <<frames[i].getNbPixelWidth() <<"x" <<frames[i].getNbPixelHeight() <<", to " <<Poco::Path(outimg).append(sstrname.str()).toString() <<"\n";

                //if( m_pProgress != nullptr )
                //    m_pProgress->store( progressBefore + (proportionofwork * (i+1) ) / frames.size() ); 
            }
        }

        /**************************************************************
        **************************************************************/
        void ExportFramesAsBMPs( const Poco::Path & outdirpath, uint32_t proportionofwork )
        {
            //uint32_t     progressBefore = 0; //Save a little snapshot of the progress
            const auto & frames         = m_inSprite.getFrames();
            Poco::Path   outimg(outdirpath);
            
            //if( m_pProgress != nullptr )
            //    progressBefore = m_pProgress->load();

            for( unsigned int i = 0; i < frames.size(); ++i )
            {
                std::stringstream sstrname;
                //Build filenmame
                sstrname <<setw(4) <<setfill('0') <<i <<"." <<utils::io::BMP_FileExtension;
                //Export
                utils::io::ExportToBMP( frames[i], Poco::Path(outimg).append(sstrname.str()).toString() );

                if( utils::LibWide().isLogOn() )
                    clog << "Exported frame " <<i <<": " <<frames[i].getNbPixelWidth() <<"x" <<frames[i].getNbPixelHeight() <<", to " <<Poco::Path(outimg).append(sstrname.str()).toString() <<"\n";

                //if( m_pProgress != nullptr )
                //    m_pProgress->store( progressBefore + (proportionofwork * (i+1)) / frames.size() );
            }
        }

#if 0
        /**************************************************************
        **************************************************************/
        void ExportFramesAsRawImgs( const Poco::Path & outdirpath, uint32_t proportionofwork )
        {
            //uint32_t     progressBefore = 0; 
            const auto & frames         = m_inSprite.getFrames();
            Poco::Path   outimg(outdirpath);
            
            //if( m_pProgress != nullptr )
            //    progressBefore = m_pProgress->load(); //Save a little snapshot of the progress

            for( unsigned int i = 0; i < frames.size(); ++i )
            {
                std::stringstream sstrname;
                //Build filenmame
                sstrname <<setw(4) <<setfill('0') <<i <<"." <<utils::io::RawImg_FileExtension;
                //Export
                utils::io::ExportRawImg_NoPal( frames[i], Poco::Path(outimg).append(sstrname.str()).toString() );

                if( utils::LibWide().isLogOn() )
                    clog << "Exported frame " <<i <<": " <<frames[i].getNbPixelWidth() <<"x" <<frames[i].getNbPixelHeight() <<", to " <<Poco::Path(outimg).append(sstrname.str()).toString() <<"\n";

                //if( m_pProgress != nullptr )
                //    m_pProgress->store( progressBefore + (proportionofwork * (i + 1)) / frames.size() );
            }
        }
#endif

        /**************************************************************
        **************************************************************/
        inline void ExportXMLData(bool xmlcolpal, const spriteWorkStats & wstats)
        {
            WriteSpriteDataToXML( &m_inSprite, m_outDirPath.toString(), wstats, xmlcolpal/*, m_pProgress*/ );
        }

        /**************************************************************
        **************************************************************/
        inline void ExportPalette()
        {
            utils::io::ExportTo_RIFF_Palette( m_inSprite.getPalette(), 
                                              Poco::Path(m_outDirPath).append(SPRITE_Palette_fname).toString() );
        }


    private:
        Poco::Path              m_outDirPath;
        const sprite_t        & m_inSprite;
        /*std::atomic<uint32_t> * m_pProgress;*/
    };

//=============================================================================================
//  Sprite Builder
//=============================================================================================
    template<class _SPRITE_t>
        class DirectoryToSprite
    {
    public:
        typedef _SPRITE_t sprite_t;

        /**************************************************************
        **************************************************************/
        DirectoryToSprite( sprite_t & out_sprite )
            :/*m_pProgress( nullptr ),*/ m_outSprite( out_sprite )
        {
        }

        /**************************************************************
        **************************************************************/
        void ParseSpriteFromDirectory( const std::string     & directorypath, 
                                       bool                    readImgByIndex, //If true, the images are read by index number in their names. If false, by alphanumeric order!
                                       /*std::atomic<uint32_t> * pProgress   = nullptr,*/
                                       bool                    parsexmlpal   = false,
                                       bool                    bNoResAutoFix = false )
        {
            //!! This must run first !!
            m_inDirPath = Poco::Path( directorypath );
            /*m_pProgress = pProgress;*/
            auto validimgslist = ListValidImages(readImgByIndex);

            //Parse the xml first to help with reading image with some formats
            ParseXML(parsexmlpal, validimgslist.size() );
            ReadImages(validimgslist);

            //Check and fix missing/differing resolution between meta-frames and images
            if( !bNoResAutoFix )
                CheckForMissingResolution();

            if( !parsexmlpal )
            {
                //See if we have a palette
                Poco::File palettef( (Poco::Path(directorypath).append(SPRITE_Palette_fname)) );

                if( palettef.exists() && palettef.isFile() )
                {
                    m_outSprite.m_palette = utils::io::ImportFrom_RIFF_Palette( palettef.path() );
                }
                else if( !( m_outSprite.m_frames.empty() ) )
                {
                    //If not, use the first image's palette 
                    m_outSprite.m_palette = m_outSprite.m_frames.front().getPalette();
                }
                else
                {
                    //No color source, warn the user!
                    cerr << "Warning: While parsing \""
                         << directorypath
                         << "\"! Could not obtain color palette from either the images, or from the expected palette file in the sprite's directory!"
                         << "";
                }
            }

            //End with rebuilding the references
            m_outSprite.RebuildAllReferences();
        }

    private:

        /*
            Check if a meta-frame points to an image with a resolution that doesn't match
            its internal resolution and fix it.
        */
        void CheckForMissingResolution()
        {
            if( utils::LibWide().isLogOn() )
                clog<<"Checking assembled sprite for resolution mismatch..\n";

            for( unsigned int i = 0; i < m_outSprite.m_metaframes.size(); ++i )
            {
                MetaFrame & curmf = m_outSprite.m_metaframes[i];
                if( curmf.HasValidImageIndex() )
                    CheckMetaFrameRes( curmf, i );
            }
        }

        void CheckMetaFrameRes( MetaFrame & curmf, unsigned int ctmf )
        {
            utils::Resolution mfres = MetaFrame::eResToResolution(curmf.resolution);
            auto            & img   = m_outSprite.m_frames.at(curmf.imageIndex);
            MetaFrame::eRes   imgres= MetaFrame::IntegerResTo_eRes( img.getNbPixelWidth(), img.getNbPixelHeight() );

            if( curmf.resolution == MetaFrame::eRes::_INVALID || 
                curmf.resolution != imgres )
            {
                stringstream sstr;
                sstr <<"WARNING: Meta-frame #" <<ctmf <<", and image#" <<curmf.imageIndex <<", have a resolution mismatch!\n"
                     <<"MF: " <<mfres <<", Img: " <<img.getNbPixelWidth() <<"x" <<img.getNbPixelHeight() <<" !\n";

                //If we got a resolution mismatch fix it!
                if( imgres != MetaFrame::eRes::_INVALID )
                {
                    sstr << "Meta-frame #" <<ctmf <<"'s resolution was changed to match image #" <<curmf.imageIndex
                         << "'s resolution successfully!\n";
                    curmf.resolution = imgres;
                }
                else
                {
                    stringstream sstrerr;
                    sstrerr << "ERROR: image #" <<curmf.imageIndex << " has an unsupported resolution of " 
                            <<img.getNbPixelWidth() <<"x" <<img.getNbPixelHeight() <<" !";
                    throw runtime_error(sstrerr.str());
                }

                string message = sstr.str();
                if( utils::LibWide().isLogOn() )
                    clog << message;
                cerr << message;
            }
        }



        /**************************************************************
        **************************************************************/
        void ParseXML( bool parsexmlpal, uint32_t nbimgs )
        { 
            ParseXMLDataToSprite( &m_outSprite, m_inDirPath.toString(), nbimgs, parsexmlpal );
        }

        vector<Poco::File> ListValidImages( bool OrderByIndex )
        {
            Poco::Path              imgsDir( m_inDirPath );
            imgsDir.append( SPRITE_IMGs_DIR );
            Poco::File              fimginfo(imgsDir);
            Poco::DirectoryIterator itdirend;
            vector<Poco::File>      list;

            for( Poco::DirectoryIterator itdir(fimginfo); itdir != itdirend; ++itdir )
            {
                if( itdir->isFile() && !itdir->isHidden() && utils::io::IsSupportedImageType( itdir->path() ) )
                    list.push_back( *itdir );
            }

            if( OrderByIndex )
            {
                //sort images
                std::sort( list.begin(), 
                           list.end(), 
                           []( const Poco::File & first, const Poco::File & second )->bool
                           {
                               stringstream strsFirst(first.path());
                               stringstream strsSecond(second.path());
                               uint32_t     indexfirst  = 0;
                               uint32_t     indexsecond = 0;
                               strsFirst  >> indexfirst;
                               strsSecond >> indexsecond;
                               return indexfirst < indexsecond;
                           });
            }

            return std::move(list);
        }

        void ReadImages( const vector<Poco::File> & filelst )
        {
            m_outSprite.m_frames.reserve( filelst.size() );

            for( auto & animg : filelst )
            {
                try
                {
                    ReadAnImage( animg );
                }
                catch( Poco::Exception & e )
                {
                    cerr << "\n<!>-Warning: Failure reading image " <<animg.path() <<":\n"
                         <<e.message() <<"\n"
                         <<"Skipping !\n";
                }
                catch( exception & e )
                {
                    cerr << "\n<!>-Warning: Failure reading image " <<animg.path() <<":\n"
                         <<e.what() <<"\n"
                         <<"Skipping !\n";
                }
            }
        }

        /**************************************************************
        **************************************************************/
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
#if 0
                case eSUPPORT_IMG_IO::RAW:
                {
                    utils::Resolution res{0,0};
                    const uint32_t    indextofind = m_outSprite.m_frames.size(); //This is where we'll insert this image.

                    //We have to find a metaframe with the right index. Or a 0xFFFF meta-frame.
                    for( unsigned int i = 0; i < m_outSprite.m_metaframes.size(); ++i )
                    {
                        res = RES_INVALID;

                        if( m_outSprite.m_metaframes[i].imageIndex == indextofind )
                        {
                            res = MetaFrame::eResToResolution(m_outSprite.m_metaframes[i].resolution);
                        }
                        else if( m_outSprite.m_metaframes[i].HasSpecialImageIndex() &&
                                 m_outSprite.m_metaframes[i].unk15 == indextofind )
                        {
                            //#EXPERIMENTAL
                            if( utils::LibWide().isLogOn() )
                            {
                                std::clog << "Reading a Raw image #" <<i <<", meta-frame has special img index value, and unk15 value is " 
                                          <<static_cast<unsigned short>(m_outSprite.m_metaframes[i].unk15) <<"!\n";
                            }
                            res = MetaFrame::eResToResolution(m_outSprite.m_metaframes[i].resolution);
                        }

                        if( res == RES_INVALID )
                        {
                            stringstream sstrerr;
                            sstrerr << "Image \"" << imgfile.path() <<"\" has an invalid resolution specified in one or more meta-frames refereing to it!";
                            throw std::runtime_error( sstrerr.str() );
                        }
                    }

                    stringstream pathtoraw;
                    pathtoraw << imgpath.parent().toString() << imgpath.getBaseName();
                    utils::io::ImportRawImg_NoPal( curfrm, pathtoraw.str(), res );
                    break;
                }
#endif
                default:
                {
                    stringstream strserror;
                    strserror<< "Image " <<imgpath.toString() <<" doesn't look like a supported image type !";
                    throw std::runtime_error(strserror.str());
                }
            };

            //if( Sprite_IsResolutionValid( curfrm.getNbPixelWidth(), curfrm.getNbPixelHeight() )
            //{
            //    stringstream sstrerr;
            //    sstrerr << "Image \"" << imgfile.path() <<"\" has an invalid resolution specified in one or more meta-frames refereing to it!";
            //    throw std::runtime_error( sstrerr.str() );
            //}
            //#TODO: Check is the resolution is valid !!!!
            //assert(false);

            m_outSprite.m_frames.push_back( std::move(curfrm) );
        }

    private:
        Poco::Path              m_inDirPath;
        sprite_t              & m_outSprite;
        /*std::atomic<uint32_t> * m_pProgress;*/
    };


//=============================================================================================
//  Type Camoflage Functions
//=============================================================================================

    /**************************************************************
    **************************************************************/
    template<>
        void ExportSpriteToDirectory( const SpriteData<gimg::tiled_image_i4bpp> & srcspr, 
                                      const std::string                         & outpath, 
                                      utils::io::eSUPPORT_IMG_IO                  imgtype,
                                      bool                                        usexmlpal,
                                      std::atomic<uint32_t>                     * progresscnt ) 
    {
        SpriteToDirectory<SpriteData<gimg::tiled_image_i4bpp>> mywriter(srcspr);
        mywriter.WriteSpriteToDir( outpath, imgtype, usexmlpal/*, progresscnt*/ ); 
    }

    /**************************************************************
    **************************************************************/
    template<>
       void ExportSpriteToDirectory( const SpriteData<gimg::tiled_image_i8bpp> & srcspr, 
                                     const std::string                         & outpath, 
                                     utils::io::eSUPPORT_IMG_IO                  imgtype,
                                     bool                                        usexmlpal,
                                     std::atomic<uint32_t>                     * progresscnt )
    {
        SpriteToDirectory<SpriteData<gimg::tiled_image_i8bpp>> mywriter(srcspr);
        mywriter.WriteSpriteToDir( outpath, imgtype, usexmlpal/*, progresscnt*/ ); 
    }


    /**************************************************************
    **************************************************************/
    void ExportSpriteToDirectoryPtr( const graphics::BaseSprite * srcspr, 
                                      const std::string          & outpath, 
                                      utils::io::eSUPPORT_IMG_IO   imgtype,
                                      bool                         usexmlpal,
                                      std::atomic<uint32_t>      * progresscnt )
    {
        //
        auto spritety = srcspr->getSpriteType();

        if( spritety == eSpriteImgType::spr4bpp )
        {
            const SpriteData<gimg::tiled_image_i4bpp>* ptr = dynamic_cast<const SpriteData<gimg::tiled_image_i4bpp>*>(srcspr);
            ExportSpriteToDirectory( (*ptr), outpath, imgtype, usexmlpal/*, progresscnt*/ );
        }
        else if( spritety == eSpriteImgType::spr8bpp )
        {
            const SpriteData<gimg::tiled_image_i8bpp>* ptr = dynamic_cast<const SpriteData<gimg::tiled_image_i8bpp>*>(srcspr);
            ExportSpriteToDirectory( (*ptr), outpath, imgtype, usexmlpal/*, progresscnt*/ );
        }
    }

    /**************************************************************
    **************************************************************/
    template<>
        SpriteData<gimg::tiled_image_i4bpp> ImportSpriteFromDirectory( const std::string     & inpath, 
                                                                       bool                    bReadImgByIndex,
                                                                       bool                    bParseXmlPal,
                                                                       std::atomic<uint32_t> * progresscnt,
                                                                       bool                    bNoResAutoFix ) 
    { 
        SpriteData<gimg::tiled_image_i4bpp>                    result;
        DirectoryToSprite<SpriteData<gimg::tiled_image_i4bpp>> myreader(result);
        myreader.ParseSpriteFromDirectory( inpath, bReadImgByIndex, /*progresscnt,*/ bParseXmlPal, bNoResAutoFix );
        return std::move( result );
    }

    /**************************************************************
    **************************************************************/
    template<>
       SpriteData<gimg::tiled_image_i8bpp> ImportSpriteFromDirectory( const std::string     & inpath, 
                                                                       bool                    bReadImgByIndex,
                                                                       bool                    bParseXmlPal,
                                                                       std::atomic<uint32_t> * progresscnt,
                                                                       bool                    bNoResAutoFix ) 
    { 
        SpriteData<gimg::tiled_image_i8bpp>                    result;
        DirectoryToSprite<SpriteData<gimg::tiled_image_i8bpp>> myreader(result);
        myreader.ParseSpriteFromDirectory( inpath, bReadImgByIndex, /*progresscnt,*/ bParseXmlPal, bNoResAutoFix );
        return std::move( result );
    }

};};