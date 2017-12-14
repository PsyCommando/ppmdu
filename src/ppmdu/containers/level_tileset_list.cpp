#include "level_tileset_list.hpp"
#include <ppmdu/fmts/bg_list_data.hpp>
#include <ppmdu/pmd2/pmd2_configloader.hpp>
#include <iostream>
using namespace filetypes;
using namespace std;

namespace pmd2
{
    LevelTilesetList LoadLevelTilesetResourceList(const std::string & bglistfile)
    {
        ConfigLoader      & conf = MainPMD2ConfigWrapper::CfgInstance();
        lvlbglist_t         lvllist = LoadLevelList(bglistfile);
        LevelTilesetList    outlvl;
        outlvl.reserve(lvllist.size());

        size_t cntlvl = 0;
        for( const auto & entry : lvllist )
        {
            LvlResList curentry;
            curentry.bplname = std::move(std::string( entry.bplname.begin(), entry.bplname.end() ));
            curentry.bpcname = std::move(std::string( entry.bpcname.begin(), entry.bpcname.end() ));
            curentry.bmaname = std::move(std::string( entry.bmaname.begin(), entry.bmaname.end() ));

            curentry.extranames.reserve(entry.extranames.size());
            for( const auto & extraname : entry.extranames )
                curentry.extranames.push_back( std::string(extraname.begin(), extraname.end()) );

            const level_info * lvlinf = conf.GetGameScriptData().LevelInfo().FindByIndex(cntlvl);
            if(lvlinf)
                outlvl.emplace( std::make_pair( lvlinf->name, std::move(curentry)) );
            else
            {
                clog<<"<!>- Loaded from bg_list.dat unknown level entry # " <<cntlvl <<"!\n";
                std::stringstream sstrlevelname;
                sstrlevelname <<"unklvl_"  <<cntlvl;
                outlvl.emplace( std::make_pair( sstrlevelname.str(), std::move(curentry)) );
            }

            ++cntlvl;
        }

        return std::move(outlvl);
    }

    void WriteLevelTilesetResourceList(const std::string & bglistfile, const LevelTilesetList & src)
    {
    }
};
