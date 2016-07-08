#ifndef PMD2_ASM_MANIP_HPP
#define PMD2_ADM_MANIP_HPP
/*
pmd2_asm_manip.hpp
2016/05/31
psycommando@gmail.com
Description: Utilities for editing PMD2 specific data within overlays and the arm9.bin binaries.
*/
#include <cstdint>
#include <array>
#include <string>
#include <vector>
#include <deque>
#include <memory>
#include <map>
#include <utils/utility.hpp>
#include <ppmdu/pmd2/pmd2.hpp>
#include <ppmdu/pmd2/pmd2_asm_data.hpp>

namespace pmd2
{
//======================================================================================
//  Starter Manip
//======================================================================================
    enum struct eStarterNatures : uint8_t
    {
        hardy,
        docile,
        brave,
        jolly,
        impish,
        naive,
        gentle,
        hasty,
        sassy,
        calm,
        relaxed,
        lonely,
        quirky,
        quiet,
        rash,
        bold,
        //Total
        NbNatures,
    };

    /*
        StarterPKmList
            Contains the list of all starters.
    */
    struct StarterPKmList
    {
        typedef uint16_t starterentry_t;
        static const uint8_t NbDefaultNatures = static_cast<uint8_t>(eStarterNatures::NbNatures);
        struct StarterPkData
        {
            starterentry_t pokemon1;
            starterentry_t pokemon2;
        };

        std::map<eStarterNatures,StarterPkData> Entries;
    };

//======================================================================================
//  Event Lookup Table Manip
//======================================================================================
    const uint32_t OffsetEventIdLookupFunction_EoS = 0x2064FFC;

    /************************************************************************************
        EventEntry_EoS
            Entry within the event lut within the ARM9 binary.
    ************************************************************************************/
    struct EventEntry_EoS
    {
        static const size_t SIZE = 12; 
        uint32_t ptrstring = 0;
        uint32_t unk1;
        int16_t  unk2;
        int16_t  unk3;
    };

//======================================================================================
//  PMD2_ASM_Manip
//======================================================================================

    /*
        PMD2_ASM_Manip
            This is a layer above the lower level loaders above. It allows to
            modify things more abstract than bytes.
    */
    class PMD2_ASM_Manip
    {
    public:
        typedef StarterPKmList starterdata_t;


        PMD2_ASM_Manip( ASM_Data_Loader && datload, eGameVersion gv, eGameRegion gr );

        //Fetch or replace the list of starter pokemon
        starterdata_t FetchStartersTable();
        void          ReplaceStartersTable(const starterdata_t & newstart);

        //Detect version, and locale from arm9.bin
        //
        //eGameRegion  GetLocale ()const { return m_glocale; }
        //eGameVersion GetVersion()const { return m_gversion; }

        //FileIO
        void Load();
        void Write();

    private:


    private:
        ASM_Data_Loader   m_datload;
        eGameVersion      m_gversion;
        eGameRegion       m_gregion;
    };

//======================================================================================
//  
//======================================================================================

};
#endif