#include "bpc.hpp"
#include <ppmdu/fmts/bpc_compression.hpp>
#include <types/contentid_generator.hpp>
#include <cassert>
using namespace std;

namespace filetypes
{
    static const ContentTy CnTy_BPC{BPC_FileExt}; //Content ID handle

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

        pmd2::TilesetLayers operator()()
        {
            pmd2::TilesetLayers tsets;
            bpc_header hdr;
            hdr.Read(m_itbeg, m_itend);

            CopyLayerAsmTable(tsets, hdr);
            tsets.layers.resize(hdr.tilesetsinfo.size());

            auto ittinfo = hdr.tilesetsinfo.begin();
            if(hdr.offsuprscr != 0)
            {
                ParseALayer( tsets.layers[0],  std::next(m_itbeg, hdr.offsuprscr),  (*ittinfo) );
                ++ittinfo;
            }
            if(hdr.offslowrscr != 0)
            {
                ParseALayer( tsets.layers[1], std::next(m_itbeg, hdr.offslowrscr), (*ittinfo) );
            }
            return std::move(tsets);
        }

    private:

        void CopyLayerAsmTable( pmd2::TilesetLayers & layers, const bpc_header & hdr )const
        {
            for( size_t i = 0; i < hdr.tilesetsinfo.size(); ++i )
            {
                pmd2::TilesetLayers::LayerAsmData data;
                data.nbtiles    = hdr.tilesetsinfo[i].nbtiles;
                data.unk2       = hdr.tilesetsinfo[i].unk2;
                data.unk3       = hdr.tilesetsinfo[i].unk3;
                data.unk4       = hdr.tilesetsinfo[i].unk4;
                data.tmapdeclen = hdr.tilesetsinfo[i].tmapdeclen;
                layers.layerasmdata.push_back( sts::move(data) );
            }
        }

        void ParseALayer( pmd2::Tileset & tset, init_t itbeg, const bpc_header::indexentry & entry )
        {
            static const size_t NbBytesPerTilesRaw = 32; //In 4bpp we output 32 bytes per tile!
            static const size_t NbPixelsPerTile    = 64;
            //Parse image first
            const size_t BytesToWrite = (entry.nbtiles-1) * NbBytesPerTilesRaw;
            std::vector<uint8_t> decout4bppbuff;
            auto itbackins = back_inserter(decout4bppbuff);//tset.Tiles());

            //
            //First, put 16 bytes of zeros since the first tile is always empty
            //
            //std::fill_n(itbackins,64,0);

            //
            //Decompress the image
            //
            bpc_compression::BPCImgDecompressor<init_t,decltype(itbackins)>(itbeg, m_itend, BytesToWrite).operator()(itbackins);

            //
            //Move the decompressed data to our tiles
            //
            size_t nbtiles = (decout4bppbuff.size() % NbBytesPerTilesRaw != 0)? 1 : 0;
            nbtiles += (decout4bppbuff.size() / NbBytesPerTilesRaw);
            tset.Tiles().resize(nbtiles + 1);
            tset.Tiles().front().resize(NbPixelsPerTile); //Resize the first empty tile

            size_t cntdestpixel = NbPixelsPerTile; //Start placing pixels after the first empty tile!
            for( size_t cntpixel = 0; cntpixel < decout4bppbuff.size(); ++cntpixel )
            {
                auto & curtile = tset.Tiles()[cntdestpixel/NbPixelsPerTile];
                curtile.resize(NbPixelsPerTile);
                curtile[cntdestpixel % NbPixelsPerTile]       = (decout4bppbuff[cntpixel] & 0x0F);
                curtile[(cntdestpixel % NbPixelsPerTile) + 1] = (decout4bppbuff[cntpixel] & 0xF0) >> 4;
                cntdestpixel+=2;
            }

            //
            //Handling for BPAs?
            //
            if( entry.unk2 != 0 || entry.unk3 != 0 || entry.unk4 != 0 || entry.unk5 != 0 )
                HandleBPAs(tset, itbeg,entry);

            const size_t Tmaplen = ((entry.tmapdeclen-1) * 9) * 2;
            
            //Then parse the tile mapping table
            vector<uint8_t> tmaptemp( std::move( bpc_compression::BPC_TileMapDecompressor<init_t, vector<uint8_t>>(itbeg, m_itend, Tmaplen)() ));

            //Convert to tiles data
            for( auto itby = tmaptemp.begin(); itby != tmaptemp.end(); )
            {
                tset.TileMap().push_back( pmd2::tileproperties(utils::ReadIntFromBytes<uint16_t>(itby,tmaptemp.end())) );
            }
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
    //std::pair<pmd2::Tileset, pmd2::Tileset> ParseBPC(const std::string & fpath)
    //{
    //    auto data = utils::io::ReadFileToByteVector(fpath);
    //    return std::move(BPCParser<decltype(data.begin())>(data.begin(), data.end())());
    //}

    pmd2::TilesetLayers ParseBPC(const std::string & fpath)
    {
        auto data = utils::io::ReadFileToByteVector(fpath);
        return std::move(BPCParser<decltype(data.begin())>(data.begin(), data.end())());
    }

    void WriteBPC(const std::string & destfpath, const pmd2::Tileset & srcupscr, const pmd2::Tileset & srcbotscr)
    {
    }



//========================================================================================================
//  bpc_rule
//========================================================================================================
    /*
        bpc_rule
            Rule for identifying BPC content. With the ContentTypeHandler!
    */
    class bpc_rule : public IContentHandlingRule
    {
    public:
        bpc_rule(){}
        ~bpc_rule(){}

        //Returns the value from the content type enum to represent what this container contains!
        virtual cnt_t getContentType()const { return CnTy_BPC; }

        //Returns an ID number identifying the rule. Its not the index in the storage array,
        // because rules can me added and removed during exec. Thus the need for unique IDs.
        //IDs are assigned on registration of the rule by the handler.
        virtual cntRID_t getRuleID()const                  { return m_myID; }
        virtual void              setRuleID( cntRID_t id ) { m_myID = id; }

        //This method returns the content details about what is in-between "itdatabeg" and "itdataend".
        virtual ContentBlock Analyse( const analysis_parameter & parameters )
        {
            //#TODO: Do something with this method, its just dumb and not accomplishing much right now! 
            ContentBlock cb;

            cb._startoffset          = 0;
            cb._endoffset            = std::distance( parameters._itdatabeg, parameters._itdataend );
            cb._rule_id_that_matched = getRuleID();
            cb._type                 = getContentType();

            return cb;
        }

        //This method is a quick boolean test to determine quickly if this content handling
        // rule matches, without in-depth analysis.
        virtual bool isMatch(  vector<uint8_t>::const_iterator   itdatabeg, 
                               vector<uint8_t>::const_iterator   itdataend,
                               const std::string    & filext )
        {
            return utils::CompareStrIgnoreCase(filext, BPC_FileExt);
        }

    private:
        cntRID_t m_myID;
    };

//========================================================================================================
//  bpc_rule_registrator
//========================================================================================================
    /*
        bpc_rule_registrator
            A small singleton that has for only task to register the bpc_rule!
    */
    RuleRegistrator<bpc_rule> RuleRegistrator<bpc_rule>::s_instance;
};