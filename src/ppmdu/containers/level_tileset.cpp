#include "level_tileset.hpp"

namespace pmd2
{

//
//
//
    void ExportTilesetPair(const std::string & destdir, const Tileset * pupscrtset, const Tileset * plowscrtset)
    {
        if( !pupscrtset && !plowscrtset )
        {
            assert(false);
            throw std::logic_error("ExportTilesetPair(): Both tilesets are null!");
        }

        assert(false); //! #TODO: Finish this
    }
};
