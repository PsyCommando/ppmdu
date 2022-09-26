#include "dungeon_data_xml.hpp"
#include <utils/parse_utils.hpp>
#include <utils/pugixml_utils.hpp>
#include <utils/library_wide.hpp>
#include <utils/utility.hpp>
#include <ppmdu/pmd2/pmd2_xml_sniffer.hpp>
#include <pugixml.hpp>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <locale>
#include <vector>
#include <utility>
#include <functional>
using std::string;
using utils::logutil::slog;
using std::stringstream;
using std::runtime_error;
using namespace pugi;
using namespace pugixmlutils;

namespace pmd2 {namespace stats
{
//=====================================================
// Constants
//=====================================================
    namespace DungeonsXML
    {
        //const string ROOT_DungeonData   = "DungeonData";
        const string ATTR_GameVer       = CommonXMLGameVersionAttrStr;
        const string ATTR_MappaID       = "mappa_id";                   //For tracking what the original mappa file was

        //const string NODE_Dungeons      = "Dungeons";
        const string ROOT_Dungeon       = "Dungeon";

        const string NODE_DungeonFloors = "DungeonFloors";
        const string NODE_DungeonFloor  = "DungeonFloor";

        const string NODE_FloorOverride = "FloorOverride";
        const string PROP_TilesetOvr    = "TilesetIdOverride";
        const string PROP_FixFlrOvr     = "FixedFloorOverride";
        const string PROP_OvrUnk0       = "OverrideUnk0";
        const string PROP_OvrUnk1       = "OverrideUnk1";

        const string ATTR_PkSpwnLst     = "pkmlst";
        const string ATTR_TrapLst       = "traplst";
        const string ATTR_ItemLst       = "itemlst";
        const string ATTR_ShopLst       = "shop_itemlst";
        const string ATTR_MHLst         = "mh_itemlst";
        const string ATTR_BuriedLst     = "buried_itemlst";
        const string ATTR_Unk2Lst       = "unk2_itemlst";
        const string ATTR_Unk3Lst       = "unk3_itemlst";

        const string PROP_DFlrShape     = "FloorLayout";
        const string PROP_DFlrRoomD     = "RoomDensity";
        const string PROP_DFlrTileset   = "TilesetID";
        const string PROP_DFlrMusic     = "MusicID";
        const string PROP_DFlrWheather  = "DefaultWeather";
        const string PROP_DFlrMinCon    = "MinimumCellConnections";
        const string PROP_DFlrPkmD      = "PokemonDensity";
        const string PROP_DFlrShopProb  = "KecleonShopProbability";
        const string PROP_DFlrMHProb    = "MonsterHouseProbability";
        const string PROP_DFlrUnk6      = "Unk6Probability";
        const string PROP_DFlrStickyProb= "StickyItemSpawnProbability";
        const string PROP_DFlrDeadEnds  = "DeadEndRoomsAllowed";
        const string PROP_DFlrHazard    = "HazardTilesTypes";

        //const string PROP_DFlrTerrainNS = "NonSquareShapedRooms";
        const string PROP_DFlrTerrainBits = "TerrainBitFlag";

        const string PROP_DFlrUnk11     = "Unk11";
        const string PROP_DFlrItemD     = "ItemDensity";
        const string PROP_DFlrTrapD     = "TrapDensity";
        const string PROP_DFlrFloorNum  = "FloorNumber";
        const string PROP_DFlrFixedID   = "FixedFloorID";
        const string PROP_DFlrExtraHallw = "ExtraHallwaysDensity";
        const string PROP_DFlrBuriedD   = "BuriedItemsDensity";
        const string PROP_DFlrWaterD    = "WaterDensity";
        const string PROP_DFlrDarkness  = "DarknessLevel";
        const string PROP_DFlrMaxMoney  = "MaximumMoney";
        const string PROP_DFlrKShopItmP = "KeckleonShopItemPosition";
        const string PROP_DFlrEmptyMH   = "MonsterHouseEmptyProbability";
        const string PROP_DFlrUnk16     = "Unk16";
        const string PROP_DFlrHStrProb  = "HiddenStairsProbability";
        const string PROP_DFlrEnemyIQ   = "EnemyIQ";
        const string PROP_DFlrIQBoost   = "IQBoosterAllowed";

        const string PROP_DFlrPkSpwn    = "PokemonSpawnListID";
        const string PROP_DFlrTrapSpwn  = "TrapSpawnListID";
        const string PROP_DFlrItemSpwn  = "ItemSpawnListID";
        const string PROP_DFlrIShopItm  = "ShopItemListID";
        const string PROP_DFlrIMHItm    = "MonsterHouseItemListID";
        const string PROP_DFlrBuriedItm = "BuriedItemListID";
        const string PROP_DFlrUnk2Itm   = "Unknown2_ItemListID";
        const string PROP_DFlrUnk3Itm   = "Unknown3_ItemListID";
        const string NODE_DFlrLists     = "AssociatedLists";

        const string ROOT_ItemLists     = "ItemLists";
        const string NODE_ItemLists     = ROOT_ItemLists;
        const string NODE_ItemList      = "List";
        const string ATTR_ItemListID    = "id";
        const string NODE_Item          = "Item";
        const string ATTR_ItemID        = "id";
        const string ATTR_ItemProb      = "prob";

        //const string PROP_ItemID        = "ItemId";
        //const string PROP_ItemProb      = "Probability";

        const string ROOT_PokemonLists  = "PokemonLists";
        const string NODE_PokemonLists  = ROOT_PokemonLists;
        const string NODE_PokemonList   = "List";
        const string ATTR_PokemonListID = "id";
        const string NODE_Pokemon       = "Pokemon";
        const string ATTR_Pkmlvl        = "lvl";
        const string ATTR_PkmSpwnWeight = "spawn_wgt";
        const string ATTR_PkmSpwnWeight2= "spawn_wgt2";
        const string ATTR_PkmID         = "pkmid";

        const string ROOT_TrapLists     = "TrapLists";
        const string NODE_TrapLists     = ROOT_TrapLists;
        const string NODE_TrapList      = "List";
        const string ATTR_TrapListID    = "id";
        const string NODE_Trap          = "Trap";
        const string ATTR_TrapName      = "name";
        const string ATTR_TrapID        = "id";
        const string ATTR_TrapProb      = "prob";

        //Fixed Dungeon Stuff
        //const string NODE_FixedDungeons = "FixedDungeons";
        //const string NODE_FixedDungeon  = "Dungeon";
        //const string NODE_FixedFloorList= "Floors";

        const string NODE_FixedFloor    = "FixedFloorData";
        const string PROP_FixFlrWidth   = "Width";
        const string PROP_FixFlrHeight  = "Height";
        const string PROP_FixFlrUnk1    = "Unk1";
        const string PROP_FixFlrLayout  = "LayoutString";

        const string CMT_EXPL_ASSOCIATED_LISTS = "Ids of the shared lists to be used for what purpose.";

        //Table of all the possible mappa IDs and the name to use to refer to them in the XML output!
        const std::unordered_map<eMappaID, std::string> MAPPAID_TO_XML_NAME{
            std::make_pair(eMappaID::S, "Sky"),         //Sora
            std::make_pair(eMappaID::T, "Time"),        //Toki
            std::make_pair(eMappaID::Y, "Darkness"),    //Yami
        };

        //Output XML filenames
        const string FNAME_ITEMS    = "items_lists.xml";
        const string FNAME_PKMS     = "pkms_lists.xml";
        const string FNAME_TRAPS    = "traps_lists.xml";
        const string DNAME_DUNGEONS = "dungeons";
    }


//=====================================================
// Writer
//=====================================================

    /*
        Outputs the dungeon data to xml to allow external modifications.

        The output directory structure is the following:
            DestDir |- dungeons |- dung1.xml
                    |           |- dung2.xml
                    |           |- ...
                    |- itemlists.xml
                    |- pkmnlists.xml
                    |- traplists.xml

        Fixed floor data is stored into their respective dungeon's content.

    */
    class DungeonDataXMLWriter
    {
    public:

        DungeonDataXMLWriter(const DungeonDB & db, const ConfigLoader & conf, const GameText * pgtext)
            :m_dungDb(db), m_conf(conf), m_pGameText(pgtext), m_bNoStrings(false)
        {
        }

        void Write(const std::string & destdir)
        {
            //Check output dir
            if (!utils::isFolder(destdir))
            {
                stringstream sstr;
                sstr << "Invalid directory \"" << destdir << "\"!";
                throw runtime_error(sstr.str());
            }

            WriteAllMappas(destdir);
        }

    private:

        static const std::string & GetMappaDirName(eMappaID id)
        {
            using namespace DungeonsXML;
            return MAPPAID_TO_XML_NAME.at(id);
        }

        //Returns a proper name for the dungeon to use as filename
        std::string GetDungeonName(uint16_t dungeonid)
        {
            using std::setw;
            using std::setfill;
            return (stringstream() << "dungeon_" << setw(3) << setfill('0') << dungeonid << ".xml").str();
        }

        //Make sure the rootnode contains all the info we need!
        xml_node MakeRootNode(xml_document & doc, const string rootname)
        {
            const GameVersionInfo & gver = m_conf.GetGameVersion();
            xml_node root = doc.append_child(rootname.c_str());
            pmd2::SetPPMDU_RootNodeXMLAttributes(root, gver.version, gver.region);
            return root;
        }

        //Write the data from all the loaded mappas files 
        void WriteAllMappas(const std::string & destdir)
        {
            for (const auto & ds : m_dungDb)
                WriteMappa(destdir, ds.second);
        }

        void WriteMappa(const std::string & destdir, const DungeonRNGDataSet & dset)
        {
            using namespace DungeonsXML;

            //Create sub-directory for the mappa file's content
            const string mappadir = (stringstream() << utils::TryAppendSlash(destdir) << GetMappaDirName(dset.OriginalMappaID())).str();
            utils::DoCreateDirectory(mappadir);

            //Create sub-directory for dungeons entries
            const string dungdir = (stringstream() << utils::TryAppendSlash(mappadir) << DNAME_DUNGEONS).str();
            utils::DoCreateDirectory(dungdir);

            xml_document xml_items;
            xml_document xml_pokemons;
            xml_document xml_traps;
            slog() << "<*>- Writing shared dungeon data lists..\n";
            //Handle shared data
            MakeMappaItemsLists(xml_items, dset)    .save_file((stringstream() << utils::TryAppendSlash(mappadir) << FNAME_ITEMS).str().c_str());
            MakeMappaItemsLists(xml_pokemons, dset) .save_file((stringstream() << utils::TryAppendSlash(mappadir) << FNAME_PKMS).str().c_str());
            MakeMappaItemsLists(xml_traps, dset)    .save_file((stringstream() << utils::TryAppendSlash(mappadir) << FNAME_TRAPS).str().c_str());

            slog() << "<*>- Writing dungeons data..\n";
            const string mappaidstr = (stringstream() << static_cast<char>(dset.OriginalMappaID())).str();
            //Handle dungeon entries
            for (uint16_t dungeonidx = 0; dungeonidx < dset.NbDungeons(); ++dungeonidx)
            {
                xml_document xml_dung;
                const dungeon_entry     & dungdat   = dset.Dungeon(dungeonidx);
                const dungeon_override  & doverride = dset.OverridenDungeon(dungeonidx);
                WriteDungeon(xml_dung, dungdat, doverride, dset, mappaidstr).save_file( (stringstream() << utils::TryAppendSlash(dungdir) << GetDungeonName(dungeonidx)).str().c_str() );
            }
        }

        //Generic implementation of making a xml file filled with lists
        //Just supply a lambda that does the work for each items
        template<class item_t, class container_t>
            xml_document & MakeLists(xml_document & doc, 
                                    const std::string & rootname, 
                                    const std::string & lstname, 
                                    container_t & container, 
                                    uint16_t nbitems, 
                                    std::function<void(xml_node&, item_t&)> && fun)
        {
            using namespace DungeonsXML;
            xml_node root = MakeRootNode(doc, rootname);
            for (uint16_t lstidx = 0; lstidx < nbitems; ++lstidx)
            {
                const auto & lst = container[lstidx];
                WriteCommentNode(root, (stringstream() << "ListID: " << std::setw(4) << std::setfill('0') << lstidx).str());
                xml_node xmllst = root.append_child(lstname.c_str());
                for(item_t & it : lst)
                    fun(xmllst, it);
            }
            return doc;
        }

        xml_document & MakeMappaItemsLists(xml_document & xml_items, const DungeonRNGDataSet & dset)
        {
            using namespace DungeonsXML;
            typedef const std::map<uint16_t, uint16_t>      container_ty;
            typedef const typename container_ty::value_type item_ty;

            auto lambdaitems = [](xml_node & parent, item_ty & it)
            {
                xml_node item = parent.append_child(NODE_Item.c_str());
                AppendAttribute(item, ATTR_ItemID, it.first);
                AppendAttribute(item, ATTR_ItemProb, it.second);
            };

            return MakeLists<item_ty>(xml_items, ROOT_ItemLists, NODE_ItemList, dset.ItemsLists(), dset.NbItemsLists(), lambdaitems);
        }

        xml_document & MakeMappaPokemonLists(xml_document & xml_pokemons, const DungeonRNGDataSet & dset)
        {
            using namespace DungeonsXML;
            typedef const pkmn_spawn_entry item_ty;
            auto lambdapkm = [](xml_node & parent, item_ty & it)
            {
                xml_node pokemon = parent.append_child(NODE_Pokemon.c_str());
                AppendAttribute(pokemon, ATTR_PkmID, it.pkmnid);
                AppendAttribute(pokemon, ATTR_Pkmlvl, it.lvlmult);
                AppendAttribute(pokemon, ATTR_PkmSpwnWeight, it.spwnrte);
                AppendAttribute(pokemon, ATTR_PkmSpwnWeight2, it.spwnrte2);
            };
            return MakeLists<item_ty>(xml_pokemons, ROOT_PokemonLists, NODE_PokemonList, dset.PokemonSpawnLists(), dset.NbPokemonSpawnLists(), lambdapkm);

            //xml_node root = MakeRootNode(xml_pokemons, ROOT_PokemonLists);
            //for (uint16_t lstidx = 0; lstidx < dset.NbPokemonSpawnLists(); ++lstidx)
            //{
            //    WriteCommentNode(root, (stringstream() << "ListID: " << std::setw(4) << std::setfill('0') << lstidx).str());
            //    xml_node pkmnlst = root.append_child(NODE_PokemonList.c_str());
            //    const pkmn_floor_spawn_lst & lst = dset.PokemonSpawnList(lstidx);
            //    for (const auto & pk : lst.pkm_spawns)
            //    {
            //        xml_node pokemon = pkmnlst.append_child(NODE_Pokemon.c_str());
            //        AppendAttribute(pokemon, ATTR_PkmID, pk.pkmnid);
            //        AppendAttribute(pokemon, ATTR_Pkmlvl, pk.lvlmult);
            //        AppendAttribute(pokemon, ATTR_PkmSpwnWeight, pk.spwnrte);
            //        AppendAttribute(pokemon, ATTR_PkmSpwnWeight2, pk.spwnrte2);
            //    }
            //}
            //return xml_pokemons;
        }

        xml_document & MakeMappaTrapLists(xml_document & xml_traps, const DungeonRNGDataSet & dset)
        {
            using namespace DungeonsXML;
            xml_node root = MakeRootNode(xml_traps, ROOT_TrapLists);
            for (uint16_t lstidx = 0; lstidx < dset.NbTrapsLists(); ++lstidx)
            {
                const auto & lst = dset.TrapsList(lstidx);
                WriteCommentNode(root, (stringstream() << "ListID: " << std::setw(4) << std::setfill('0') << lstidx).str());
                xml_node xmllst = root.append_child(NODE_TrapList.c_str());
                for (uint16_t i = 0; i < lst.size(); ++i)
                {
                    xml_node trap = xmllst.append_child(NODE_Trap.c_str());
                    AppendAttribute(trap, ATTR_TrapID, i);
                    AppendAttribute(trap, ATTR_TrapProb, lst[i]);
                }
            }
            return xml_traps;
        }

        xml_document & WriteDungeon(xml_document & xml_dung, const dungeon_entry & dung, const dungeon_override & ovrd, const DungeonRNGDataSet & dset, const std::string & mappaid)
        {
            using namespace DungeonsXML;
            xml_node root = MakeRootNode(xml_dung, ROOT_Dungeon);
            AppendAttribute(root, ATTR_MappaID, mappaid);

            //Floors
            xml_node nfloors = root.append_child(NODE_DungeonFloors.c_str());
            for (uint16_t flooridx = 0; flooridx < static_cast<uint16_t>(dung.floors.size()); ++flooridx )
            {
                uint16_t floordatid = dung.floors[flooridx].idfloordata;

                //Fetch the corresponding overriden floor entry for this floor if there's one!
                assert(ovrd.floor_indices.size() == dung.floors.size());
                const dungeon_floor_override * povr = nullptr;
                if (flooridx < ovrd.floor_indices.size())
                    povr = &(dset.OverridenFloor(ovrd.floor_indices[flooridx]));

                //Write the actual floor data
                WriteDungeonFloor(nfloors, dung.floors[flooridx], dset.Floor(floordatid), povr);
            }
            return xml_dung;
        }

        xml_node WriteDungeonFloor(xml_node & parent, const dungeon_floor_info & floor, const floor_data_entry & fdat, const dungeon_floor_override * pover = nullptr)
        {
            using namespace DungeonsXML;
            xml_node node = parent.append_child(NODE_DungeonFloor.c_str());
            WriteCommentNode(parent, (stringstream() << "floor #" <<std::setw(3) <<std::setfill('0') << fdat.floornb).str());

            //Write floor properties
            WriteNodeWithValue(node, PROP_DFlrShape,        fdat.floorshape);
            WriteNodeWithValue(node, PROP_DFlrRoomD,        fdat.roomdensity);
            WriteNodeWithValue(node, PROP_DFlrTileset,      fdat.tilesetid);
            WriteNodeWithValue(node, PROP_DFlrMusic,        fdat.musicid);
            WriteNodeWithValue(node, PROP_DFlrWheather,     fdat.defaultweather);
            WriteNodeWithValue(node, PROP_DFlrMinCon,       fdat.minconnections);
            WriteNodeWithValue(node, PROP_DFlrPkmD,         fdat.pkmndensity);
            WriteNodeWithValue(node, PROP_DFlrShopProb,     fdat.shopratio);
            WriteNodeWithValue(node, PROP_DFlrMHProb,       fdat.mhouseratio);
            WriteNodeWithValue(node, PROP_DFlrUnk6,         fdat.unk6);
            WriteNodeWithValue(node, PROP_DFlrStickyProb,   fdat.stickychance);
            WriteNodeWithValue(node, PROP_DFlrDeadEnds,     fdat.deadendsallowed);
            WriteNodeWithValue(node, PROP_DFlrHazard,       fdat.hazardtiles);
            WriteNodeWithValue(node, PROP_DFlrTerrainBits,  fdat.terrainbits);
            WriteNodeWithValue(node, PROP_DFlrUnk11,        fdat.unk11);
            WriteNodeWithValue(node, PROP_DFlrItemD,        fdat.itemdensity);
            WriteNodeWithValue(node, PROP_DFlrTrapD,        fdat.trapdensity);
            WriteNodeWithValue(node, PROP_DFlrFloorNum,     fdat.floornb);
            WriteNodeWithValue(node, PROP_DFlrFixedID,      fdat.fixedfloorid);
            WriteNodeWithValue(node, PROP_DFlrExtraHallw,   fdat.extrahallwdensity);
            WriteNodeWithValue(node, PROP_DFlrBuriedD,      fdat.burieditemdensity);
            WriteNodeWithValue(node, PROP_DFlrWaterD,       fdat.waterdensity);
            WriteNodeWithValue(node, PROP_DFlrDarkness,     fdat.darklvl);
            WriteNodeWithValue(node, PROP_DFlrMaxMoney,     fdat.maxcoin);
            WriteNodeWithValue(node, PROP_DFlrKShopItmP,    fdat.keclitempos);
            WriteNodeWithValue(node, PROP_DFlrEmptyMH,      fdat.emptymhchance);
            WriteNodeWithValue(node, PROP_DFlrUnk16,        fdat.unk16);
            WriteNodeWithValue(node, PROP_DFlrHStrProb,     fdat.hiddenstrschnc);
            WriteNodeWithValue(node, PROP_DFlrEnemyIQ,      fdat.enemyiq);
            WriteNodeWithValue(node, PROP_DFlrIQBoost,      fdat.allowiqboostr);

            //Write floor associated lists
            xml_node lists = node.append_child(NODE_DFlrLists.c_str());
            WriteCommentNode(node, CMT_EXPL_ASSOCIATED_LISTS);
            WriteNodeWithValue(lists, PROP_DFlrPkSpwn,      floor.idpkspwn);
            WriteNodeWithValue(lists, PROP_DFlrTrapSpwn,    floor.idlutc);
            WriteNodeWithValue(lists, PROP_DFlrItemSpwn,    floor.iditemspwn);
            WriteNodeWithValue(lists, PROP_DFlrIShopItm,    floor.idshopdat);
            WriteNodeWithValue(lists, PROP_DFlrIMHItm,      floor.idmhouseitem);
            WriteNodeWithValue(lists, PROP_DFlrBuriedItm,   floor.idburieditemsdat);
            WriteNodeWithValue(lists, PROP_DFlrUnk2Itm,     floor.unk2);
            WriteNodeWithValue(lists, PROP_DFlrUnk3Itm,     floor.unk3);

            //Write fixed floor data
            if (fdat.fixedfloorid > 0 && fdat.fixedfloorid < m_dungDb.FixedFloorData().size())
            {
                const FixedDungeonFloorEntry & fixf = m_dungDb.FixedFloorData()[fdat.fixedfloorid];
                xml_node fixed = node.append_child(NODE_FixedFloor.c_str());
                WriteCommentNode(node, (stringstream() << "Fixed floor ID: " << fdat.fixedfloorid).str());
                WriteNodeWithValue(fixed, PROP_FixFlrWidth, fixf.width);
                WriteNodeWithValue(fixed, PROP_FixFlrHeight, fixf.height);
                WriteNodeWithValue(fixed, PROP_FixFlrUnk1, fixf.unk1);

                //Write the layout bytes for each tiles as a continuous string of 8 bits hexadecimal values
                stringstream sstr;
                for (uint8_t b : fixf.floormap)
                {
                    sstr << std::hex << std::setw(2) <<std::setfill('0') <<static_cast<unsigned int>(b) << " ";
                }
                WriteNodeWithValue(fixed, PROP_FixFlrLayout, sstr.str());
            }

            //Write floor override data
            if (pover)
            {
                xml_node overridenode = node.append_child(NODE_FloorOverride.c_str());
                WriteNodeWithValue(overridenode, PROP_TilesetOvr, pover->tilesetid);
                WriteNodeWithValue(overridenode, PROP_FixFlrOvr, pover->fixedfloorid);
                WriteNodeWithValue(overridenode, PROP_OvrUnk0, pover->unk0);
                WriteNodeWithValue(overridenode, PROP_OvrUnk1, pover->unk1);
            }
            return node;
        }

    private:
        const DungeonDB & m_dungDb;
        const ConfigLoader & m_conf;
        const GameText * m_pGameText;
        bool m_bNoStrings;
    };

//=====================================================
// Parser
//=====================================================
    class DungeonDataXMLReader
    {
    public:
        DungeonDataXMLReader(DungeonDB & dbdest, const ConfigLoader & conf, const GameText * pgtext)
            :m_dungDb(dbdest), m_conf(conf), m_pGameText(pgtext)
        {
        }

        void Read(const std::string & srcdir)
        {

        }

    private:
        DungeonDB & m_dungDb;
        const ConfigLoader & m_conf;
        const GameText * m_pGameText;
    };

//=====================================================
//  Functions
//=====================================================
    void WriteDungeonDataToXml(const DungeonDB & db, const std::string & destpath, const pmd2::GameText * text, const ConfigLoader & conf)
    {
        DungeonDataXMLWriter(db, conf, text).Write(destpath);
    }

    DungeonDB & ReadDungeonDataFromXml(const std::string & srcpath, DungeonDB & outdb, pmd2::GameText * text)
    {
        // TODO: insert return statement here
        return outdb;
    }
}}