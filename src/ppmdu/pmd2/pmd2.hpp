#ifndef PMD2_HPP
#define PMD2_HPP
/*
pmd2.hpp
2015/09/11
psycommando@gmail.com
Description:
    Contains declarations and tools that applies to the entirety of the PMD2 games.
*/
#include <cstdint>
#include <array>
#include <string>

namespace pmd2
{
//======================================================================================
//  Constants
//======================================================================================
    /*
        eGameVersion
            Unique IDs for each versions of the PMD2 games.
    */
    enum struct eGameVersion
    {
        Invalid,
        EoS,        //Explorers of Sky
        EoTEoD,     //Explorers of Time/Darkness
        NBGameVers, //Must be last
    };

    /*
        GameVersionNames
            For a given eGameVersion ID returns a string that represents the game version.
    */
    extern const std::array<std::string, static_cast<size_t>(eGameVersion::NBGameVers)> GameVersionNames;

    const std::string & GetGameVersionName( eGameVersion gv );

};

#endif