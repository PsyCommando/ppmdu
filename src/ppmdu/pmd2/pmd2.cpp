#include "pmd2.hpp"

using namespace std;

namespace pmd2
{
    const std::array<std::string, static_cast<size_t>(eGameVersion::NBGameVers)> GameVersionNames =
    {
        "",
        "EoS",
        "EoTD",
    };

    const std::string & GetGameVersionName( eGameVersion gv )
    {
        return GameVersionNames[static_cast<size_t>(gv)];
    }

};