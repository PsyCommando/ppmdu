#ifndef LEVEL_TILESET_HPP
#define LEVEL_TILESET_HPP
/*
level_tileset.hpp
2016/09/28
psycommando@gmail.com
Description: Storage and processing classes for level tileset data.
*/
#include <ppmdu/pmd2/pmd2.hpp>
#include <ppmdu/containers/tiled_image.hpp>
#include <ppmdu/containers/color.hpp>
#include <vector>
#include <array>
#include <cstdint>


namespace pmd2
{
//
//
//
    /*  
        tmapconstants
            Constants used in the game for parsing tilemaps.
    */
    struct tsetconstants
    {
        //
        uint16_t unk_0x2BC;
        uint16_t unk_0x2BE;
        uint16_t unk_0x2CO; //Nb bpas?
        uint16_t unk_0x2C2;
        uint16_t unk_0x2C4; //Nb Palettes used?
        uint16_t unk_0x2C6;
        uint16_t unk_0x2C8;
        //
        uint16_t unk_0x2CA;
        uint16_t unk_0x2CC;
        uint16_t unk_0x2CE;
        uint16_t unk_0x2D0;
        uint16_t unk_0x2D2;
        uint16_t unk_0x2D4;
        uint16_t unk_0x2D8;
    };

    

//
//
//
    extern const std::array<tsetconstants, 3> TSetIndependantSelectConstants;
    extern const std::array<tsetconstants, 3> TSetGroundSelectConstants;
    extern const std::array<tsetconstants, 2> TSetGroundSubWorldSelectConstants;

    /*
    */
    inline const tsetconstants & IndependantMapSeletConstants( int levelid, uint8_t lvlunk1 )
    {
        if( levelid != -1 )
        {
            if( lvlunk1 >= 6 && lvlunk1 <= 10 )
                return TSetIndependantSelectConstants[1];
            else if( lvlunk1 >= 11 && lvlunk1 <= 12 )
                return TSetIndependantSelectConstants[2];
        }
        //if( levelid == -1 || lvlunk1 > 12 || lvlunk1 <= 5  )
        return TSetIndependantSelectConstants[0];
    }

    /*
    */
    inline const tsetconstants & GroundMapSeletConstants     ( int levelid, uint8_t lvlunk1 )
    {
        if( levelid != -1 )
        {
            if( lvlunk1 >= 6 && lvlunk1 <= 10 )
                return TSetGroundSelectConstants[1];
            else if( lvlunk1 >= 11 && lvlunk1 <= 12 )
                return TSetGroundSelectConstants[2];
        }
        //if( levelid == -1 || lvlunk1 > 12 || lvlunk1 <= 5  )
        return TSetGroundSelectConstants[0];
    }

    /*
    */
    inline const tsetconstants & GroundSubWorldMapSeletConstants( int levelid, uint8_t lvlunk1 )
    {
        if( levelid != -1 && ( lvlunk1 == 0 || (lvlunk1 >= 6 && lvlunk1 <= 12) ) )
            return TSetGroundSubWorldSelectConstants[1];
        else
            return TSetGroundSubWorldSelectConstants[0];
    }

//
//
//
    /*
        tileproperties
            Tile mapping entry for a single tile.
            Indicates what tile and what color palette to use for a particular tile on the NDS screen.
    */
    struct tileproperties
    {
        uint16_t tileindex;
        uint8_t  palindex;
        bool     vflip;
        bool     hflip;

        tileproperties()
            :tileindex(0),palindex(0),vflip(false), hflip(false)
        {}

        tileproperties(uint16_t val) { operator=(val); }

        tileproperties( const tileproperties & other)
            :tileindex(other.tileindex), palindex(other.palindex), hflip(other.hflip), vflip(other.vflip)
        {}

        tileproperties & operator=( tileproperties & other )
        {
            tileindex = other.tileindex;
            palindex  = other.palindex;
            hflip     = other.hflip;
            vflip     = other.vflip;
            return *this;
        }

        inline operator uint16_t()const {return (tileindex | ((hflip?1:0) << 10) | ((vflip?1:0) << 11) | (palindex << 12));}
        inline tileproperties & operator=(uint16_t val) 
        {
            tileindex = 0x3FF & val;        //0000 0011 1111 1111 Grab the lowest 10 bits for this one
            hflip     = (0x400 & val) > 0;  //0000 0100 0000 0000
            vflip     = (0x800 & val) > 0;  //0000 1000 0000 0000
            palindex  = (val >> 12);        //1111 0000 0000 0000
            return *this; 
        }

        inline tileproperties & operator|=(uint16_t val) 
        {
            tileindex |= 0x3FF & val;        //0000 0011 1111 1111 Grab the lowest 10 bits for this one
            hflip     |= (0x400 & val) > 0;  //0000 0100 0000 0000
            vflip     |= (0x800 & val) > 0;  //0000 1000 0000 0000
            palindex  |= (val >> 12);        //1111 0000 0000 0000
            return *this; 
        }

        inline operator bool()const
        {
            return (tileindex != 0) && hflip && vflip & (palindex != 0);
        }
    };


    /*
        TilesetPalette
            Palette for a level tileset.
    */
    struct TilesetPalette
    {
        static const size_t NbColorsPerPalette = 16;
        struct dbaentry
        {
            uint16_t unk3;
            uint16_t unk4;
        };
        typedef std::array<gimg::colorRGBX32,NbColorsPerPalette>    pal_t;
        typedef std::vector<pal_t>                                  paltbl_t;
        typedef std::vector<gimg::colorRGBX32>                      varpal_t;
        
        paltbl_t                mainpals;
        std::vector<dbaentry>   dba;
        varpal_t                palette2;
    };


    /*
        TilesetBMAData
    */
    class TilesetBMAData
    {
    public:

        uint8_t width;      //in tiles
        uint8_t height;     //in tiles
        uint8_t  unk1;
        uint8_t  unk2;
        uint8_t  unk3;
        uint8_t  unk4;
        uint16_t unk5;
        uint16_t unk6;
        uint16_t unk7;

        std::vector<uint16_t> unktable1; // The first compressed table
        std::vector<uint16_t> unktable2; // The second compressed table
        std::vector<uint8_t>  unktable3; // The third compressed table, possibly terrain passability mask
    };


    /*
    */
    class TilesetBPAData
    {
    public:

    };


    /*
        Tileset
    */
    class Tileset
    {
    public:
        typedef std::vector<std::vector<gimg::pixel_indexed_4bpp>>  imgdat_t;
        typedef std::vector<tileproperties>                         tmapdat_t;
        typedef TilesetPalette                                      pal_t;      //15, 16 colors palettes
        typedef std::vector<TilesetBPAData>                         bpadat_t;
        typedef TilesetBMAData                                      bmadat_t;

        inline pal_t            & Palettes()        {return m_palettes;}
        inline const pal_t      & Palettes()const   {return m_palettes;}
        inline imgdat_t         & Tiles()           {return m_imgdata;}
        inline const imgdat_t   & Tiles()const      {return m_imgdata;}
        inline tmapdat_t        & TileMap()         {return m_tilemapping;}
        inline const tmapdat_t  & TileMap()const    {return m_tilemapping;}

        inline bmadat_t        & BMAData()          {return m_bmadata;}
        inline const bmadat_t  & BMAData()const     {return m_bmadata;}

        inline bpadat_t         & BPAData()         {return m_bpadata;}
        inline const bpadat_t   & BPAData()const    {return m_bpadata;}

    private:

        pal_t           m_palettes;
        imgdat_t        m_imgdata;
        tmapdat_t       m_tilemapping;
        bmadat_t        m_bmadata;
        bpadat_t        m_bpadata;
    };



//
//
//
    /*
        ExportTilesetPairToRaw
            Exports either the upper or lower screen tileset into the specified directory as raw unprocessed images and palettes.
    */
    void ExportTilesetPairToRaw( const std::string & destdir, const Tileset * pupscrtset,  const Tileset * plowscrtset = nullptr );

    void PrintAssembledTilesetPreviewToPNG( const std::string & fpath, const Tileset & tileset );

    void DumpCellsToPNG(const std::string & fpath, const Tileset & tileset);

};

#endif // !LEVEL_TILESET_HPP
