#include "level_tileset.hpp"
#include <sstream>
#include <fstream>
#include <iterator>
using namespace std;

namespace pmd2
{

    const std::string FnameUpperScrData = "upper";
    const std::string FnameLowerScrData = "lower";



//
//
//
    void ExportTilesetToRaw(const std::string & destdir, const std::string & basename, const Tileset & tset )
    {
        stringstream sstrimg;
        stringstream sstrtmap;
        stringstream sstrpal;
        sstrimg  <<utils::TryAppendSlash(destdir) <<basename;
        sstrtmap <<sstrimg.str() <<"_tilemap.bin";
        sstrpal  <<sstrimg.str() <<"_pal.rgbx32";
        sstrimg  <<"_img.4bpp";

        utils::DoCreateDirectory(destdir);

        //#1 export the image data
        utils::io::WriteByteVectorToFile(sstrimg.str(), tset.Tiles());

        //#2 export the tiling data
        {
            ofstream oftile(sstrtmap.str(), ios::binary | ios::out );
            oftile.exceptions(ios::badbit);
            ostreambuf_iterator<char> itouttile(oftile);
            for( const auto & word : tset.TileMap() )
                utils::WriteIntToBytes( static_cast<uint16_t>(word), itouttile );
        }
        //#3 export the palette
        {
            ofstream ofpal(sstrtmap.str(), ios::binary | ios::out );
            ofpal.exceptions(ios::badbit);
            ostreambuf_iterator<char> itoutpal(ofpal);
            for( const auto & pal : tset.Palettes().mainpal )
            {
                for( const auto & color : pal )
                {
                    color.WriteAsRawByte(itoutpal);
                }
            }
        }
    }

//
//
//
    void ExportTilesetPairToRaw(const std::string & destdir, const Tileset * pupscrtset, const Tileset * plowscrtset)
    {
        if( !pupscrtset && !plowscrtset )
        {
            assert(false);
            throw std::logic_error("ExportTilesetPair(): Both tilesets are null!");
        }

        if(pupscrtset)
        {
            ExportTilesetToRaw(destdir, FnameUpperScrData, *pupscrtset);
        }

        if(plowscrtset)
        {
            ExportTilesetToRaw(destdir, FnameLowerScrData, *plowscrtset);
        }
        assert(false); //! #TODO: Finish this
    }
};
