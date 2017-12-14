#ifndef PMD2_GRAPHICS_HPP
#define PMD2_GRAPHICS_HPP
/*
pmd2_graphics.hpp
*/
#include <string>
#include <map>
#include <cstdint>



namespace pmd2
{

    /*
        GameMapBgDictionary
            Maintain the list of map bg and update the resource files as needed
    */
    class GameMapBgDictionary
    {
    public:
    };

    /*
        GameGraphics
            Handles and categorize game graphics for easier access/editing.
    */
    class GameGraphics
    {
    public:
        GameGraphics( const std::string & gameroot );

        // GetTopMenuBG      ( const std::string & name );
        // GetSprite         ( const std::string & name );
        // GetPokemonSprite  ( unsigned int pkindex );
        // GetPokemonPortrait( unsigned int pkindex );
        GameMapBgDictionary         & GetMapBG()        {return m_mapbg;}
        const GameMapBgDictionary   & GetMapBG()const   {return m_mapbg;}
        // GetMapTileSet     ( const std::string & name );
        // GetImage          ( const std::string & name );

    private:
        std::string         m_gameroot;
        GameMapBgDictionary m_mapbg;
    };
};

#endif