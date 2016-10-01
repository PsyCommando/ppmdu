#ifndef LEVEL_TILESET_HPP
#define LEVEL_TILESET_HPP
/*
level_tileset.hpp
2016/09/28
psycommando@gmail.com
Description: Storage and processing classes for level tileset data.
*/
#include <ppmdu/pmd2/pmd2.hpp>
#include <ppmdu/containers/color.hpp>
#include <vector>


namespace pmd2
{

    /*
        TilesetPalette
            Palette for a level tileset.
    */
    class TilesetPalette
    {
    public:
        static const size_t NbColorsPerPalette = 16;
        typedef std::vector<std::array<gimg::colorRGBX32,NbColorsPerPalette>> pal_t;

    private:
        pal_t m_mainpal;
        pal_t m_pal2;       //Unknown purpose?
    };


};

#endif // !LEVEL_TILESET_HPP
