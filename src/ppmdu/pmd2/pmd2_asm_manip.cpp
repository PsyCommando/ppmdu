#include "pmd2_asm_manip.hpp"

namespace pmd2
{

    PMD2_ASM_Manip::PMD2_ASM_Manip(ASM_Data_Loader && datload, eGameVersion gv, eGameRegion gr)
        :m_datload(std::move(datload)), m_gversion(gv), m_gregion(gr)
    {
    }

    PMD2_ASM_Manip::starterdata_t PMD2_ASM_Manip::FetchStartersTable()
    {
        assert(false);
        return starterdata_t();
    }

    void PMD2_ASM_Manip::ReplaceStartersTable(const starterdata_t & newstart)
    {
        assert(false);
    }

    void PMD2_ASM_Manip::Load()
    {

    }

    void PMD2_ASM_Manip::Write()
    {

    }
};