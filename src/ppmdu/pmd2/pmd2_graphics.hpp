#ifndef PMD2_GRAPHICS_HPP
#define PMD2_GRAPHICS_HPP
/*
pmd2_graphics.hpp
*/
#include <string>
#include <map>
#include <cstdint>
#include <ppmdu/containers/base_image.hpp>


namespace pmd2
{

    /*
        GameGraphics
            Handles and categorize game graphics for easier access/editing.
    */
    class GameGraphics
    {
    public:
        GameGraphics( const std::wstring & gameroot );

        // GetTopMenuBG      ( const std::string & name );
        // GetSprite         ( const std::string & name );
        // GetPokemonSprite  ( unsigned int pkindex );
        // GetPokemonPortrait( unsigned int pkindex );
        // GetMapBG          ( const std::string & name );
        // GetMapTileSet     ( const std::string & name );
        // GetImage          ( const std::string & name );

    private:
        std::wstring & m_gameroot;
    };
};

#endif