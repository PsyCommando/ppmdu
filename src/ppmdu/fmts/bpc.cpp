#include "bpc.hpp"
#include <ppmdu/fmts/bpc_compression.hpp>
#include <cassert>
using namespace std;

namespace filetypes
{
//============================================================================================
//  BPCParser
//============================================================================================

    /*
        BPCParser
            
    */
    template<class _init>
        class BPCParser
    {
        typedef _init init_t;
    public:
        BPCParser( _init itbeg, _init itend )
            :m_itbeg(itbeg), m_itend(itend)
        {}

        std::pair<pmd2::Tileset, pmd2::Tileset> operator()()
        {
            std::pair<pmd2::Tileset,pmd2::Tileset> tsets;
            bpc_header hdr;
            hdr.Read(m_itbeg, m_itend);
            if(hdr.offsuprscr != 0)
                ParseATileset( tsets.first,  std::next(m_itbeg, hdr.offsuprscr),  (hdr.tilesetsinfo[0]) );
            if(hdr.offsuprscr != 0)
                ParseATileset( tsets.second, std::next(m_itbeg, hdr.offslowrscr), (hdr.tilesetsinfo[1]) );
            return std::move(tsets);
        }

    private:
        void ParseATileset( pmd2::Tileset & tset, init_t itbeg, const bpc_header::indexentry & entry )
        {
            //Parse image first
            const size_t BytesToWrite = entry.nbtiles * 32;
            for( auto itbackins = back_inserter(tset.Tiles()); (itbeg != m_itend) &&  (tset.Tiles().size() < BytesToWrite); ++itbeg )
                bpc_compression::BPCImgDecompressor<decltype(itbackins)>(itbackins).operator()(*itbeg);

            //Handling for BPAs?
            if( entry.unk2 != 0 || entry.unk3 != 0 || entry.unk4 != 0 || entry.unk5 != 0 )
                HandleBPAs(tset, itbeg,entry);

            const size_t Tmaplen = ((entry.tmapdeclen - 1) * 9) * 2;
            //Then parse the tile mapping table
            tset.TileMap() = std::move( bpc_compression::BPC_TileMapDecompressor<init_t, decltype(tset.TileMap())>(itbeg, m_itend, Tmaplen)() );
        }

        void HandleBPAs( pmd2::Tileset & tset, init_t itbeg, const bpc_header::indexentry & entry )
        {
            //! #TODO
        }

    private:
        init_t      m_itbeg;
        init_t      m_itend;
    };

//============================================================================================
//  BPCWriter
//============================================================================================

    /*
        BPCWriter
    */
    class BPCWriter
    {
    public:
        BPCWriter()
        {
        }

    private:
    };

//============================================================================================
//  Functions
//============================================================================================
    std::pair<pmd2::Tileset, pmd2::Tileset> ParseBPC(const std::string & fpath)
    {
        auto data = utils::io::ReadFileToByteVector(fpath);
        return std::move(BPCParser<decltype(data.begin())>(data.begin(), data.end())());
    }

    void WriteBPC(const std::string & destfpath, const pmd2::Tileset & srcupscr, const pmd2::Tileset & srcbotscr)
    {
    }
};