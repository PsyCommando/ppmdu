#include "dungeon_rng_data.hpp"


namespace pmd2 {namespace stats {

    const std::unordered_map<eGameVersion, std::vector<eMappaID>> MAPPA_FILES_VERSIONS_FOR_VERSIONS
    {
        {
            eGameVersion::EoS,
            std::vector<eMappaID>
            {
                eMappaID::S,
                eMappaID::T,
                eMappaID::Y,
            }
        },
        {
            eGameVersion::EoT,
            std::vector<eMappaID>
            {
                eMappaID::T,
                eMappaID::Y,
            }
        },
        {
            eGameVersion::EoD,
            std::vector<eMappaID>
            {
                eMappaID::T,
                eMappaID::Y,
            }
        },
    };


}}