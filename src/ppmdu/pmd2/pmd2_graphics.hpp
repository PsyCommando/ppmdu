#ifndef PMD2_GRAPHICS_HPP
#define PMD2_GRAPHICS_HPP
/*
pmd2_graphics.hpp
*/
#include <string>
#include <map>
#include <cstdint>
#include <ppmdu/containers/tiled_image.hpp>
#include <ppmdu/containers/sprite_data.hpp>

namespace pmd2
{

    /*
        MapBgDB
            Maintain the list of map bg and update the resource files as needed
    */
    class MapBgDB
    {
    public:
    };

    /*
        TopMenuBgDB
            Maintain the list of map bg and update the resource files as needed
    */
    class TopMenuBgDB
    {
    public:
    };

    /*
        PokemonSpritesDB
            Contains and sorts pokemon sprites.
    */
    class PokemonSpritesDB
    {
    public:
        enum struct eSpriteType : int 
        {
            OverworldSprites,
            DungeonSprites,
            AttackSprites,
        };
        typedef uint16_t sprite_id_t;
        typedef std::unique_ptr<graphics::BaseSprite> sprptr_t;

        //Access sprites
        const graphics::BaseSprite *    sprite(sprite_id_t id, eSpriteType category)const;
        graphics::BaseSprite *          sprite(sprite_id_t id, eSpriteType category);

        //

    private:
        std::vector<sprptr_t> m_ground_sprites;
        std::vector<sprptr_t> m_dungeon_sprites;
        std::vector<sprptr_t> m_attack_sprites;
    };

    /*
        PokemonPortraitsDB
            Contains the data from the kaomado.kao file with the portraits for all pokemons
    */
    class PokemonPortraitsDB
    {
    public:
        static const int NB_PORTRAIT_SLOTS = 16;
        typedef std::vector<gimg::tiled_indexed_image<gimg::pixel_indexed_4bpp, gimg::colorRGB24>>  portrait_t;             //Type for a single image
        typedef std::array<portrait_t, NB_PORTRAIT_SLOTS>                                           pkmn_portrait_slot_t;   //Type for a set of portrait for a pokemon
        typedef uint16_t pokemon_id_t;
        typedef uint16_t portrait_id_t;

        void resize(size_t newsize);
        size_t size()const;

        //Returns the amount of slots for pokemons available
        size_t getNbPokemonSlots()const;

        //Accessors
        const pkmn_portrait_slot_t &    portraitSlot(pokemon_id_t apkmn_id)const;
        pkmn_portrait_slot_t &          portraitSlot(pokemon_id_t apkmn_id);

        const portrait_t &              portrait(pokemon_id_t apkmn_id, portrait_id_t aportrait)const;
        portrait_t &                    portrait(pokemon_id_t apkmn_id, portrait_id_t aportrait);

    private:
        std::vector<pkmn_portrait_slot_t> m_portraits;
    };

    /*
        MiscSpritesDB
    */
    class MiscSpritesDB
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
        MapBgDB         & GetMapBG()        {return m_mapbg;}
        const MapBgDB   & GetMapBG()const   {return m_mapbg;}
        // GetMapTileSet     ( const std::string & name );
        // GetImage          ( const std::string & name );

    private:
        std::string         m_gameroot;
        MapBgDB m_mapbg;
    };
};

#endif