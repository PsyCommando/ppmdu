#include "sprite_io.hpp"
#include <ppmdu/containers/sprite_data.hpp>
#include <ppmdu/containers/tiled_image.hpp>
#include <ppmdu/ext_fmts/png_io.hpp>
#include <ppmdu/ext_fmts/riff_palette.hpp>
#include <ppmdu/utils/poco_wrapper.hpp>
#include <ppmdu/utils/library_wide.hpp>
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
        vector<string> dircontent = utils::ListDirContent_FilesAndDirectories( dirpath, true );
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
        vector<string> dircontent = utils::ListDirContent_FilesAndDirectories( dirpath, true );
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
            :m_inSprite(myspr),m_pProgress(nullptr)
        {}

        /**************************************************************
        **************************************************************/
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

            if( utils::LibWide().isLogOn() )
                clog << "Output directory " <<directory.path() <<". Ok!\n";

            m_pProgress = progresscnt;

            //Count the ammount of entries for calculating work
            // and for adding statistics to the exported files.
            uint32_t totalnbseqs  = m_inSprite.getAnimSequences().size();
            uint32_t totalnbfrms  = 0;

            for( const auto & aseq : m_inSprite.getAnimSequences() )
                totalnbfrms += aseq.getNbFrames();
            
            //Gather stats for computing progress proportionally at runtime
            //const uint32_t amtWorkFrames  = m_inSprite.getFrames().size();
            //const uint32_t amtWorkAnims   = m_inSprite.getAnimGroups().size() + totalnbseqs + totalnbfrms;
            //const uint32_t amtWorkFrmGrps = m_inSprite.getMetaFrames().size() + m_inSprite.getMetaFrmsGrps().size();
            //const uint32_t amtWorkOffs    = m_inSprite.getPartOffsets().size();
            //const uint32_t totalwork      = amtWorkAnims + amtWorkFrmGrps + amtWorkOffs + amtWorkFrames;

            //Get the percentages of work, relative to the total, for each
            //const float percentFrames  = static_cast<float>( (static_cast<double>(amtWorkFrames)  * 100.0) / static_cast<double>(totalwork) );
            //const float percentAnims   = static_cast<float>( (static_cast<double>(amtWorkAnims)   * 100.0) / static_cast<double>(totalwork) );
            //const float percentFrmGrps = static_cast<float>( (static_cast<double>(amtWorkFrmGrps) * 100.0) / static_cast<double>(totalwork) );
            //const float percentOffsets = static_cast<float>( (static_cast<double>(amtWorkOffs)    * 100.0) / static_cast<double>(totalwork) );

            spriteWorkStats stats;
            //stats.propFrames    = static_cast<uint32_t>(percentFrames);
            //stats.propAnims     = static_cast<uint32_t>(percentAnims);
            //stats.propMFrames   = static_cast<uint32_t>(percentFrmGrps);
            //stats.propOffsets   = static_cast<uint32_t>(percentOffsets);
            stats.totalAnimFrms = totalnbfrms;
            stats.totalAnimSeqs = totalnbseqs;

            ExportFrames(imgty, stats.propFrames );

            if( !xmlcolorpal )
                ExportPalette();

            ExportXMLData(xmlcolorpal, stats);

            if( m_pProgress != nullptr )
                m_pProgress->store( 100 ); //fill the remaining percentage (the palette and etc are nearly instantenous anyways)
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

        /**************************************************************
        **************************************************************/
        void ExportFramesAsPNGs( const Poco::Path & outdirpath, uint32_t proportionofwork )
        {
            uint32_t     progressBefore = 0; //Save a little snapshot of the progress
            const auto & frames         = m_inSprite.getFrames();
            Poco::Path   outimg(outdirpath);

            if( m_pProgress != nullptr )
                progressBefore = m_pProgress->load();

            for( unsigned int i = 0; i < frames.size(); ++i )
            {
                std::stringstream sstrname;
                //Build filenmame
                sstrname <<setw(4) <<setfill('0') <<i <<"." <<utils::io::PNG_FileExtension;
                //Export
                utils::io::ExportToPNG( frames[i], Poco::Path(outimg).append(sstrname.str()).toString() );

                if( utils::LibWide().isLogOn() )
                    clog << "Exported frame " <<i <<": " <<frames[i].getNbPixelWidth() <<"x" <<frames[i].getNbPixelHeight() <<", to " <<Poco::Path(outimg).append(sstrname.str()).toString() <<"\n";

                if( m_pProgress != nullptr )
                    m_pProgress->store( progressBefore + (proportionofwork * (i+1) ) / frames.size() ); 
            }
        }

        /**************************************************************
        **************************************************************/
        void ExportFramesAsBMPs( const Poco::Path & outdirpath, uint32_t proportionofwork )
        {
            uint32_t     progressBefore = 0; //Save a little snapshot of the progress
            const auto & frames         = m_inSprite.getFrames();
            Poco::Path   outimg(outdirpath);
            
            if( m_pProgress != nullptr )
                progressBefore = m_pProgress->load();

            for( unsigned int i = 0; i < frames.size(); ++i )
            {
                std::stringstream sstrname;
                //Build filenmame
                sstrname <<setw(4) <<setfill('0') <<i <<"." <<utils::io::BMP_FileExtension;
                //Export
                utils::io::ExportToBMP( frames[i], Poco::Path(outimg).append(sstrname.str()).toString() );

                if( utils::LibWide().isLogOn() )
                    clog << "Exported frame " <<i <<": " <<frames[i].getNbPixelWidth() <<"x" <<frames[i].getNbPixelHeight() <<", to " <<Poco::Path(outimg).append(sstrname.str()).toString() <<"\n";

                if( m_pProgress != nullptr )
                    m_pProgress->store( progressBefore + (proportionofwork * (i+1)) / frames.size() );
            }
        }

        /**************************************************************
        **************************************************************/
        void ExportFramesAsRawImgs( const Poco::Path & outdirpath, uint32_t proportionofwork )
        {
            uint32_t     progressBefore = 0; 
            const auto & frames         = m_inSprite.getFrames();
            Poco::Path   outimg(outdirpath);
            
            if( m_pProgress != nullptr )
                progressBefore = m_pProgress->load(); //Save a little snapshot of the progress

            for( unsigned int i = 0; i < frames.size(); ++i )
            {
                std::stringstream sstrname;
                //Build filenmame
                sstrname <<setw(4) <<setfill('0') <<i <<"." <<utils::io::RawImg_FileExtension;
                //Export
                utils::io::ExportRawImg_NoPal( frames[i], Poco::Path(outimg).append(sstrname.str()).toString() );

                if( utils::LibWide().isLogOn() )
                    clog << "Exported frame " <<i <<": " <<frames[i].getNbPixelWidth() <<"x" <<frames[i].getNbPixelHeight() <<", to " <<Poco::Path(outimg).append(sstrname.str()).toString() <<"\n";

                if( m_pProgress != nullptr )
                    m_pProgress->store( progressBefore + (proportionofwork * (i + 1)) / frames.size() );
            }
        }

        /**************************************************************
        **************************************************************/
        inline void ExportXMLData(bool xmlcolpal, const spriteWorkStats & wstats)
        {
            WriteSpriteDataToXML( &m_inSprite, m_outDirPath.toString(), wstats, xmlcolpal, m_pProgress );
            //SpriteXMLWriter(&m_inSprite).WriteXMLFiles( m_outDirPath.toString(), wstats, xmlcolpal, m_pProgress );
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
        std::atomic<uint32_t> * m_pProgress;
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
            :m_pProgress( nullptr ), m_outSprite( out_sprite )
        {
        }

        /**************************************************************
        **************************************************************/
        void ParseSpriteFromDirectory( const std::string     & directorypath, 
                                       bool                    readImgByIndex, //If true, the images are read by index number in their names. If false, by alphanumeric order!
                                       std::atomic<uint32_t> * pProgress   = nullptr,
                                       bool                    parsexmlpal = false )
        {
            //!! This must run first !!
            m_inDirPath = Poco::Path( directorypath );
            m_pProgress = pProgress;

            auto validimgslist = ListValidImages(readImgByIndex);

            //Parse the xml first to help with reading image with some formats
            ParseXML(parsexmlpal, validimgslist.size() );

            ReadImages(validimgslist);

            //if( readImgByIndex )
            //    ReadImagesByIndex();
            //else
            //    ReadImagesSorted();

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

        /**************************************************************
        **************************************************************/
        void ParseXML( bool parsexmlpal, uint32_t nbimgs )
        { 
            ParseXMLDataToSprite( &m_outSprite, m_inDirPath.toString(), nbimgs, parsexmlpal );
        }

        vector<Poco::File> ListValidImages( bool OrderByIndex )
        {
            vector<Poco::File> list;
            Poco::Path         imgsDir( m_inDirPath );
            imgsDir.append( SPRITE_IMGs_DIR );
            Poco::File         fimginfo(imgsDir);
            Poco::DirectoryIterator itdirend;

            for( Poco::DirectoryIterator itdir(fimginfo); itdir != itdirend; ++itdir )
            {
                if( itdir->isFile() && !itdir->isHidden() && utils::io::IsSupportedImageType( itdir->path() ) )
                {
                    list.push_back( *itdir );
                }
            }

            if( OrderByIndex )
            {
                //sort images
                std::sort( list.begin(), list.end(), []( const Poco::File & first, const Poco::File & second )->bool
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

            //for( uint32_t i = 0; i < filelst.size(); ++i )
            for( auto & animg : filelst )
            {
                //auto & animg = filelst[i];
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
        //void ReadImagesByIndex()
        //{
        //    //assert(false);
        //    //**************************************************************************
        //    //#TODO: Use the meta-frame table to read image files by index/filenmae. Then change the meta-frame index in-memory to 
        //    //       refer to the index the image data was actually inserted at!
        //    //**************************************************************************
        //    Poco::Path                imgsDir( m_inDirPath );
        //    map<uint32_t, Poco::File> validImages;
        //    uint32_t                  nbvalidimgs  = 0;
        //    imgsDir.append( SPRITE_IMGs_DIR );

        //    Poco::DirectoryIterator itdir(imgsDir);
        //    Poco::DirectoryIterator itdircount(imgsDir);
        //    Poco::DirectoryIterator itdirend;

        //    //We want to load images into the right indexes

        //    //
        //    //#1 - Count the images in the imgs directory + Find all our valid images
        //    for(; itdir != itdirend; ++itdir ) 
        //    {
        //        if( utils::io::IsSupportedImageType( itdir->path() ) )
        //        {
        //            stringstream sstrparseindex(itdir->path());
        //            uint32_t     curindex = 0;
        //            sstrparseindex >> curindex;

        //            validImages.insert( make_pair( curindex, *itdir ) );
        //        }
        //    }


        //    //#2 - Parse the images after allocating
        //    m_outSprite.m_frames.reserve( validImages.size() );

        //    for( uint32_t i = 0; i < validImages.size(); ++i )
        //    {
        //        auto & animg = validImages.at(i);
        //        try
        //        {
        //            ReadAnImage( animg );
        //        }
        //        catch( std::out_of_range ore ) //In this case it means the std::map didn't find our entry
        //        {
        //            cerr << "\n<!>-Warning: Image #" <<i <<" was expected, but not found !\n"
        //                << "The next image read will end up with that image# ! This might result in unforseen consequences!\n";
        //        }
        //        catch( Poco::Exception e )
        //        {
        //            cerr << "\n<!>-Warning: Failure reading image " <<animg.path() <<":\n"
        //                 <<e.message() <<"\n"
        //                 <<"Skipping !\n";
        //        }
        //        catch( exception e )
        //        {
        //            cerr << "\n<!>-Warning: Failure reading image " <<animg.path() <<":\n"
        //                 <<e.what() <<"\n"
        //                 <<"Skipping !\n";
        //        }
        //    }
        //}

        ///**************************************************************
        //**************************************************************/
        //void ReadImagesSorted()
        //{
        //    Poco::Path         imgsDir( m_inDirPath );
        //    uint32_t           nbvalidimgs  = 0;
        //    imgsDir.append( SPRITE_IMGs_DIR );

        //    Poco::DirectoryIterator itdir(imgsDir);
        //    Poco::DirectoryIterator itdircount(imgsDir);
        //    Poco::DirectoryIterator itdirend;

        //    //count imgs
        //    while( itdircount != itdirend )
        //    {
        //        if( utils::io::IsSupportedImageType( itdircount->path() ) )
        //            ++nbvalidimgs;
        //        ++itdircount;
        //    }

        //    //Allocate
        //    m_outSprite.m_frames.reserve( nbvalidimgs );

        //    //Grab the images in order
        //    for(; itdir != itdirend; ++itdir )  
        //    {
        //        if( utils::io::IsSupportedImageType( itdir->path() ) )
        //        {
        //            try
        //            {
        //                ReadAnImage( *itdir );
        //            }
        //            catch( Poco::Exception e )
        //            {
        //                cerr << "\n<!>-Warning: Failure reading image " <<itdir->path() <<":\n"
        //                     <<e.message() <<"(POCO)\n"
        //                     <<"Skipping !\n";
        //            }
        //            catch( exception e )
        //            {
        //                cerr << "\n<!>-Warning: Failure reading image " <<itdir->path() <<":\n"
        //                     <<e.what() <<"\n"
        //                     <<"Skipping !\n";
        //            }
        //        }
        //    }
        //}

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
                        else if( m_outSprite.m_metaframes[i].isSpecialMetaFrame() &&
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
                default:
                {
                    stringstream strserror;
                    strserror<< "Image " <<imgpath.toString() <<" doesn't look like a BMP, RAW or PNG image !";
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
        std::atomic<uint32_t> * m_pProgress;
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
        mywriter.WriteSpriteToDir( outpath, imgtype, usexmlpal, progresscnt ); 
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
        mywriter.WriteSpriteToDir( outpath, imgtype, usexmlpal, progresscnt ); 
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
            ExportSpriteToDirectory( (*ptr), outpath, imgtype, usexmlpal, progresscnt );
        }
        else if( spritety == eSpriteImgType::spr8bpp )
        {
            const SpriteData<gimg::tiled_image_i8bpp>* ptr = dynamic_cast<const SpriteData<gimg::tiled_image_i8bpp>*>(srcspr);
            ExportSpriteToDirectory( (*ptr), outpath, imgtype, usexmlpal, progresscnt );
        }
    }

    /**************************************************************
    **************************************************************/
    template<>
        SpriteData<gimg::tiled_image_i4bpp> ImportSpriteFromDirectory( const std::string     & inpath, 
                                                                       bool                    bReadImgByIndex,
                                                                       bool                    bParseXmlPal,
                                                                       std::atomic<uint32_t> * progresscnt ) 
    { 
        SpriteData<gimg::tiled_image_i4bpp>                    result;
        DirectoryToSprite<SpriteData<gimg::tiled_image_i4bpp>> myreader(result);
        myreader.ParseSpriteFromDirectory( inpath, bReadImgByIndex, progresscnt, bParseXmlPal );
        return std::move( result );
    }

    /**************************************************************
    **************************************************************/
    template<>
       SpriteData<gimg::tiled_image_i8bpp> ImportSpriteFromDirectory( const std::string     & inpath, 
                                                                       bool                    bReadImgByIndex,
                                                                       bool                    bParseXmlPal,
                                                                       std::atomic<uint32_t> * progresscnt ) 
    { 
        SpriteData<gimg::tiled_image_i8bpp>                    result;
        DirectoryToSprite<SpriteData<gimg::tiled_image_i8bpp>> myreader(result);
        myreader.ParseSpriteFromDirectory( inpath, bReadImgByIndex, progresscnt, bParseXmlPal );
        return std::move( result );
    }

};};