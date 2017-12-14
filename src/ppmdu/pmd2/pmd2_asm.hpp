#ifndef PMD2_ASM_HPP
#define PMD2_ASM_HPP
/*
pmd2_asm.hpp
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
#include <unordered_map>
#include <utils/utility.hpp>
#include <ppmdu/pmd2/pmd2.hpp>
//#include <ppmdu/pmd2/pmd2_asm_data.hpp>
#include <ppmdu/pmd2/pmd2_configloader.hpp>

//!++#TODO: 
//! Since we'll use an external assembler for asm hacks, and this will essentially now be just for, generating the arm asm, invoking the assembler, and veryfying if a patch was applied,
//! better trim this down a bit!

namespace pmd2
{
//======================================================================================
//  Constants
//======================================================================================
    extern const std::string FName_ARM9Bin;                     //Default name of the arm9 binary file.
    extern const std::string ASM_ModdedTag;                     //Tag opening any modded binary location, to indicate that the data that should be here isn't
    extern const int         ASM_ModdedTaDescgMaxLen;           //4096 characters max for a modded tag!

//======================================================================================
//  Utilities
//======================================================================================

    /************************************************************************************
        ExBinaryIsModded
            Exception thrown when the game binaries were modified.
    ************************************************************************************/
    class ExBinaryIsModded : public std::runtime_error 
    {
    public:
        ExBinaryIsModded(const std::string & msg, const std::string & binpath, uint32_t binoff);

        std::string m_binpath;
        uint32_t    m_binoff;
    };

//======================================================================================
//  Handler
//======================================================================================
    /************************************************************************************
        PMD2_ASM
            This is a layer above the lower level loaders above. It allows to
            modify things more abstract than bytes.
    ************************************************************************************/
    class PMD2_ASM_Impl;
    class PMD2_ASM
    {
    public:
        //typedef StarterPKmList starterdata_t;
        struct modinfo
        {
            std::string modstring;
            inline bool ismodded() {return modstring.empty();}
        };

        PMD2_ASM( const std::string & romroot, ConfigLoader & conf );
        ~PMD2_ASM();

        /*
            Load data from the game binaries if not modified.
            Fallbacks to loading the modded data if a modded tag is detected!
        */
        GameScriptData::lvlinf_t   LoadLevelList();
        GameScriptData::livesent_t LoadActorList();
        GameScriptData::objinf_t   LoadObjectList();
        GameScriptData::gvar_t     LoadGameVariableList();

        /*
            Write the data to a loose file at the locations indicated in the configuration files, since there's no way to guarantee any changes would fit into the initial binaries.

        */
        void WriteLevelList         ( const GameScriptData::lvlinf_t    & src );
        void WriteActorList         ( const GameScriptData::livesent_t  & src );
        void WriteObjectList        ( const GameScriptData::objinf_t    & src );
        void WriteGameVariableList  ( const GameScriptData::gvar_t      & src );

        /*
            LoadAllToConfig
                Replaces what was currently loaded as script data with what was loaded from the 
                binaries.
        */
        void LoadAllToConfig();

        /*
            WriteAllFromConfig
                Writes the hardcoded data parsed from the config file, into loose files.
        */
        void WriteAllFromConfig();

        /*
            Check if the block starting at the specified address in the binary specified 
            has the modded tag.
            Returns a modinfo that either returns the mod string, or an empty string if its not modded.
        */
        modinfo CheckBlockModdedTag( const binarylocatioinfo & locinfo );

    private:
        std::unique_ptr<PMD2_ASM_Impl> m_pimpl;
    };

//======================================================================================
//  
//======================================================================================

};
#endif