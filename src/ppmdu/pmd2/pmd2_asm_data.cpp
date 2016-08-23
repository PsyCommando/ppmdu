#include "pmd2_asm_data.hpp"

namespace pmd2
{



//=======================================================================================
//  ARM9_Loader
//=======================================================================================
//    ARM9_Loader::ARM9_Loader()
//    {
//    }
//
//    ARM9_Loader::ARM9_Loader(ARM9_Loader && mv)
//    {
//    }
//
//    ARM9_Loader::~ARM9_Loader()
//    {
//    }
//
//    ARM9_Loader & ARM9_Loader::operator=(ARM9_Loader && mv)
//    {
//        // TODO: insert return statement here
//    }
//
//    void ARM9_Loader::LoadBinary(const std::string & fpath)
//    {
//    }
//
//    void ARM9_Loader::WriteBinary(const std::string & fpath)
//    {
//    }
//
//    void ARM9_Loader::Write(size_t offset, uint8_t byte)
//    {
//    }
//
//    uint8_t ARM9_Loader::Read(size_t offset) const
//    {
//        return uint8_t();
//    }
//
//    bool ARM9_Loader::WasModified() const
//    {
//        return false;
//    }
//
//=======================================================================================
//  ARM_Overlay_Loader
//=======================================================================================
//    ARM_Overlay_Loader::ARM_Overlay_Loader()
//    {
//    }
//
//    ARM_Overlay_Loader::ARM_Overlay_Loader(ARM_Overlay_Loader && mv)
//    {
//    }
//
//    ARM_Overlay_Loader::~ARM_Overlay_Loader()
//    {
//    }
//
//    ARM_Overlay_Loader & ARM_Overlay_Loader::operator=(ARM_Overlay_Loader && mv)
//    {
//        // TODO: insert return statement here
//    }
//
//    void ARM_Overlay_Loader::LoadBinary(const std::string & fpath)
//    {
//    }
//
//    void ARM_Overlay_Loader::WriteBinary(const std::string & fpath)
//    {
//    }
//
//    void ARM_Overlay_Loader::Write(size_t offset, uint8_t byte)
//    {
//    }
//
//    uint8_t ARM_Overlay_Loader::Read(size_t offset) const
//    {
//        return uint8_t();
//    }
//
//    bool ARM_Overlay_Loader::WasModified() const
//    {
//        return false;
//    }
//
//
//=======================================================================================
//  ASM_Data_Loader
//=======================================================================================
    ASM_Data_Loader::ASM_Data_Loader(const std::string & romrootdir)
    {
    }

    ASM_Data_Loader::ASM_Data_Loader(ASM_Data_Loader && mv)
        //:m_arm9l(std::move(mv.m_arm9l)), m_armovrlayl(std::move(mv.m_armovrlayl))
    {
    }

    ASM_Data_Loader & ASM_Data_Loader::operator=(ASM_Data_Loader && mv)
    {
        //m_arm9l      = std::move(mv.m_arm9l);
        //m_armovrlayl = std::move(mv.m_armovrlayl);
        return *this;
    }

    bool ASM_Data_Loader::LoadAllData()
    {
        return false;
    }

    bool ASM_Data_Loader::LoadARM9Data()
    {
        return false;
    }

    bool ASM_Data_Loader::LoadOverlayData()
    {
        return false;
    }

    void ASM_Data_Loader::WriteAllData()
    {
    }

    void ASM_Data_Loader::WriteARM9Data()
    {
    }

    void ASM_Data_Loader::WriteOverlayData()
    {
    }



};
