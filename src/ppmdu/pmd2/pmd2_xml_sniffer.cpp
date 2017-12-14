#include "pmd2_xml_sniffer.hpp"

using namespace std;

namespace pmd2
{
        void CheckGameVersionAndGameRegion( const RootNodeInfo & rootnode, 
                                        eGameVersion       & out_ver, 
                                        eGameRegion        & out_reg,
                                        bool                 bprintdetails )
    {
        using namespace std;
        if( rootnode.attributes.size() >= 2 )
        {
            if(bprintdetails)
                cout <<"<!>-XML Script file detected!\n";
            try
            {
                out_ver = pmd2::StrToGameVersion(rootnode.attributes.at(CommonXMLGameVersionAttrStr));
                out_reg  = pmd2::StrToGameRegion (rootnode.attributes.at(CommonXMLGameRegionAttrStr));
            }
            catch(const std::exception&)
            {
                throw_with_nested(std::logic_error("CheckGameVersionAndGameRegion(): Couldn't get the game version and or game region values attributes from document root!!"));
            }
            if(bprintdetails)
            {
                cout <<"<*>-Detected Game Version : " <<pmd2::GetGameVersionName(out_ver) <<"\n"
                     <<"<*>-Detected Game Region  : " <<pmd2::GetGameRegionNames(out_reg)  <<"\n";
            }

            if( out_ver == eGameVersion::Invalid )
                throw std::invalid_argument("CheckGameVersionAndGameRegion(): Game version attribute in root node is invalid!");
            if( out_reg == eGameRegion::Invalid )
                throw std::invalid_argument("CheckGameVersionAndGameRegion(): Game region attribute in root node is invalid!");
        }
        else
            throw std::invalid_argument("CheckGameVersionAndGameRegion(): Game region and version attribute in root node are missing!");
    }
};