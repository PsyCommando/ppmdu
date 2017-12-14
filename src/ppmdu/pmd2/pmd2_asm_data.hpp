#ifndef PMD2_ASM_DATA_HPP
#define PMD2_ADM_DATA_HPP
/*
pmd2_asm_data.hpp
2016/05/09
psycommando@gmail.com
Description: Contains utilities for loading, accessing and editing data contained in
             PMD2's binaries. Either overlays, or the arm9.bin.
*/
#include <cstdint>
#include <array>
#include <string>
#include <vector>
#include <deque>
#include <memory>
#include <unordered_map>
#include <utils/utility.hpp>
#include <ppmdu/pmd2/pmd2.hpp>
#include <ppmdu/pmd2/pmd2_configloader.hpp>

namespace pmd2
{
//======================================================================================
//  Constants
//======================================================================================
    const std::string FName_ARM9Bin       = "arm9.bin";
    const std::string FName_OverlayPrefix = "overlay_";

//======================================================================================
//  ASM_Data_Loader
//======================================================================================

    /*
        ASM_Data_Loader
            Handles loading and low-level access for the various binaries.
    */
#if 0
    class ASM_Data_Loader
    {
    public:
        ASM_Data_Loader( const std::string & romrootdir );
        ASM_Data_Loader( ASM_Data_Loader && mv );
        ASM_Data_Loader & operator=( ASM_Data_Loader && mv );

        /*
            LoadData
        */
        bool LoadAllData();
        bool LoadARM9Data();
        bool LoadOverlayData();

        /*
            WriteData
        */
        void WriteAllData();
        void WriteARM9Data();
        void WriteOverlayData();

        /*
            EditFunctions
        */
        //ARM9_Loader        & GetARM9()                      { return m_arm9l; }
        //ARM_Overlay_Loader & GetOverlay( size_t overlaynb ) { return m_armovrlayl.at(overlaynb); }



    private:

    };
#endif


//======================================================================================
//  Functions
//======================================================================================
};
#endif