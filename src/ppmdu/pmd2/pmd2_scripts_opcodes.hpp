#ifndef PMD2_SCRIPTS_OPCODES_HPP
#define PMD2_SCRIPTS_OPCODES_HPP
/*
pmd2_scripts_opcodes.hpp
2016/05/08
psycommando@gmail.com
Description: Contains data on script opcodes.
*/
#include <ppmdu/pmd2/pmd2.hpp>
#include <ppmdu/pmd2/pmd2_scripts.hpp>
#include <ppmdu/pmd2/opcode_version.hpp>
#include <ppmdu/pmd2/opcode_definition_sky.hpp>
#include <ppmdu/pmd2/opcode_definition_time_darkness.hpp>
#include <ppmdu/pmd2/opcode_categories.hpp>
#include <ppmdu/pmd2/opcode_parameters.hpp>
#include <utils/parse_utils.hpp>
#include <cstdint>
#include <vector>
#include <string>
#include <array>
#include <cassert>
#include <iomanip>


//! #TODO: Replace most of the data access functions in here with a single interface for retrieving
//!        that information seamlessly, whether the info comes from the rom, a c++ table, or XML data!

namespace pmd2
{
//==========================================================================================================
//  Routine Types
//==========================================================================================================

    enum struct eRoutineTy : uint16_t
    {
        Standard    = 1,    //Nothing special
        Unused2     = 2,    //unused
        ActorFun    = 3,    //For routines an actor executes!
        ObjectFun   = 4,    //For routines an object executes!
        PerfFun     = 5,    //For routines a performer executes!
        Unused6     = 6,    //unused
        Unused7     = 7,    //unused
        Unused8     = 8,    //unused
        CommonSpec  = 9,    //Only for routines in the unionall.ssb file!
        Invalid     = std::numeric_limits<uint16_t>::max(),
    };

    /*
        RoutineHasParameter
            Returns whether a given routine type makes use of the parameter in its entry in the routine table.
    */
    inline bool RoutineHasParameter(uint16_t ty)
    {
        return (ty != static_cast<uint16_t>(eRoutineTy::Standard) && 
                ty != static_cast<uint16_t>(eRoutineTy::CommonSpec));
    }
    std::string RoutineTyToStr( uint16_t ty );
    std::string RoutineTyToStr( eRoutineTy ty );
    uint16_t    StrToRoutineTyInt( const std::string & str );

    //Get the proper paramter type for a type of routine
    eOpParamTypes RoutineParameterType( uint16_t ty );



//=====================================================================================
//  Instruction Info
//=====================================================================================
    /*************************************************************************************
        OpCodeInfoWrapper
            Wrapper to abstract parameter info between versions of the game.

            **Its reasonable, since we'd make pointers to those anyways when using**
            **the opcode info to abstract between versions. 3 integers for 3 integers**
            **is the same thing.**
    *************************************************************************************/
    struct OpCodeInfoWrapper
    {
        OpCodeInfoWrapper()
            :pname(nullptr), nbparams(0), pparaminfo(nullptr), category(eCommandCat::Invalid)
        {}

        OpCodeInfoWrapper(const OpCodeInfoEoS & inf)        {this->operator=(inf);}
        OpCodeInfoWrapper(const OpCodeInfoEoS * inf)        {this->operator=(inf);}
        OpCodeInfoWrapper(const OpCodeInfoEoTD & inf)       {this->operator=(inf);}
        OpCodeInfoWrapper(const OpCodeInfoEoTD * inf)       {this->operator=(inf);}
        OpCodeInfoWrapper(const OpCodeInfoWrapper & cp )    {this->operator=(cp); }

        OpCodeInfoWrapper & operator=( const OpCodeInfoWrapper & cp ) 
        {
            pname      = cp.pname;
            nbparams   = cp.nbparams;
            pparaminfo = cp.pparaminfo;
            category   = cp.category;
            return *this;
        }

        OpCodeInfoWrapper & operator=( const OpCodeInfoEoS * inf ) 
        {
            if( inf != nullptr )
            {
                pname      = std::addressof(inf->name); 
                nbparams   = inf->nbparams; 
                pparaminfo = std::addressof(inf->paraminfo);
                category   = inf->cat;
            }
            else
            {
                pname      = nullptr;
                nbparams   = 0;
                pparaminfo = nullptr;
                category   = eCommandCat::Invalid;
            }
            return *this;
        }

        OpCodeInfoWrapper & operator=( const OpCodeInfoEoTD * inf ) 
        {
            if( inf != nullptr )
            {
                pname      = std::addressof(inf->name); 
                nbparams   = inf->nbparams; 
                pparaminfo = std::addressof(inf->paraminfo);
                category   = inf->cat;
            }
            else
            {
                pname      = nullptr;
                nbparams   = 0;
                pparaminfo = nullptr;
                category   = eCommandCat::Invalid;
            }
            return *this;
        }

        const std::string              & Name     ()const { return *pname;}
        int16_t                          NbParams ()const { return nbparams;}
        const std::vector<OpParamInfo> & ParamInfo()const { return *pparaminfo;}
        eCommandCat                      Category ()const { return category; }
        eInstructionType                 GetMyInstructionType()const 
        {
            switch(Category())
            {
                case eCommandCat::EntityAccessor:
                {
                    return eInstructionType::MetaAccessor;
                }
                case eCommandCat::Switch:
                {
                    return eInstructionType::MetaSwitch;
                }
                case eCommandCat::ProcSpec:
                case eCommandCat::OpWithReturnVal:
                {
                    return eInstructionType::MetaReturnCases;
                }
                default:
                {
                    return eInstructionType::Command;
                }
            };
        }


        /*
            IsReturnHandler
                Whether the instruction handles the return value of a previous instruction,
                such as a switch, or anything that returns a value.
                This is mainly for "Case" instructions.
        */
        inline bool IsReturnHandler()const
        {
            if(IsUnconditonalReturnHandler())
                return true;

            switch(Category())
            {
                case eCommandCat::Default:
                case eCommandCat::Case:
                case eCommandCat::CaseNoJump:
                    return true;
                default:
                    return false;
            }
        }

        /*
            IsUnconditonalReturnHandler
                Mainly for the hold comamnd, and any command that simply process a return value
                but do not perform conditional operation depending on the value.
                These typically mark the end of a return value handling chain if present at all.
        */
        inline bool IsUnconditonalReturnHandler()const
        {
            switch(Category())
            {
                //! #TESTING!!!!!!
#if 0
                case eCommandCat::Hold:
                    return true;
#endif
                default:
                    return false;
            }
        }

        /*
            IsAttribute
                Returns whether the instruction can be used by accessors such as "lives", "object", and "performer"
        */
        inline bool IsAttribute()const
        {
            switch(Category())
            {
                case eCommandCat::Destroy:
                case eCommandCat::Hold:             //Hold is special, it works as an attribute too!
                case eCommandCat::EntAttribute:
                    return true;
                default:
                    return false;
            }
        }

        /*
            IsAccessor
                Whether this instruction is an accessor, and is meant to pick
                what the next attribute command will act upon.
        */
        inline bool IsEntityAccessor()const
        {
            switch(Category())
            {
                case eCommandCat::EntityAccessor:
                    return true;
                default:
                    return false;
            }
        }

        /*
            HasReturnValue
                Whether the instruction returns a value which 
                can then be used by any following "Case" 
                instructions.
        */
        inline bool HasReturnValue()const
        {
            switch(Category())
            {
                case eCommandCat::ProcSpec:
                case eCommandCat::Switch:
                case eCommandCat::OpWithReturnVal:
                    return true;
                default:
                    return false;
            }
        }


        /*
            IsSequenceTerminator
                Whether the instruction stops the linear execution of the code, or 
                jumps away.
                Mainly used to determine where a sequence/block of instructions end.
                For example, an unconditional jump, end, return, all end a sequence.
        */
        inline bool IsSequenceTerminator()const
        {
            switch(Category())
            {
                case eCommandCat::End:
                case eCommandCat::Jump:
                case eCommandCat::JumpCommon:
                case eCommandCat::Return:
                    return true;
                default:
                    return false;
            }
        }

        /*
            IsConditionalJump
                Returns whether the instruction may trigger a jump when
                the conditions are right.
        */
        inline bool IsConditionalJump()const
        {
            switch(Category())
            {
                case eCommandCat::BranchCmd:
                case eCommandCat::Case:
                    return true;
                default:
                    return false;
            }
        }

        /*
        */
        inline bool IsDebugInstruction()const
        {
            switch(Category())
            {
                case eCommandCat::Debug:
                    return true;
                default:
                    return false;
            }
        }


        operator bool()const {return pname!= nullptr && pparaminfo != nullptr;}

        const std::string              * pname;
        int16_t                          nbparams;
        eCommandCat                      category;
        const std::vector<OpParamInfo> * pparaminfo;
    };


    /*************************************************************************************
        OpCodeClassifier
    *************************************************************************************/
    class OpCodeClassifier
    {
    public:
        OpCodeClassifier(eOpCodeVersion ver)
            :m_ver(ver)
        {}

        OpCodeClassifier(eGameVersion ver)
        {
            if( ver == eGameVersion::EoD || ver == eGameVersion::EoT )
                m_ver = eOpCodeVersion::EoTD;
            else if( ver == eGameVersion::EoS )
                m_ver = eOpCodeVersion::EoS;
            else 
                throw std::runtime_error("OpCodeClassifier::OpCodeClassifier(): Got invalid game version!!");
        }

        /*
            Info
                Return info on the specified instruction, wrapped in a OpCodeInfoWrapper.
                If it doesn't find any info, the state of the OpCodeInfoWrapper, will be invalid!
        */
        OpCodeInfoWrapper Info(uint16_t code)
        {
            if(m_ver == eOpCodeVersion::EoS)
                return FindOpCodeInfo_EoS(code);
            else if(m_ver == eOpCodeVersion::EoTD)
                return FindOpCodeInfo_EoTD(code);
            else
            {
                assert(false);
                return OpCodeInfoWrapper();
            }
        }

        /*
            Code
                For a number of parameters, and a given instruction name, 
                return the corresponding code/instruction id.
                Returns "InvalidOpCode" if the instruction can't
                be found!
        */
        uint16_t Code(const std::string & instname, size_t nbparams)
        {
            if(m_ver == eOpCodeVersion::EoS)
                return static_cast<uint16_t>(FindOpCodeByName_EoS(instname,nbparams));
            else if(m_ver == eOpCodeVersion::EoTD)
                return static_cast<uint16_t>(FindOpCodeByName_EoTD(instname,nbparams));
            else
            {
                assert(false);
                return InvalidOpCode;
            }
        }

        /*************************************************************************************
            CalcInstructionLen
                Calculate the length of an instruction as raw bytes.
                Returns 0 if the command's instruction id is invalid!
        *************************************************************************************/
        inline size_t CalcInstructionLen( const ScriptInstruction & instr)
        {
            OpCodeInfoWrapper oinfo = Info(instr.value);
            if(!oinfo)
                return 0;

            if( oinfo.NbParams() == -1 )
                return ScriptWordLen + (instr.parameters.size() * ScriptWordLen) + ScriptWordLen; // -1 instructions have an extra word for the nb of instructions!
            else
                return ScriptWordLen + (instr.parameters.size() * ScriptWordLen);
        }

        /*
            GetNbOpcodes
                Returns the nb of total instructions for the current game version!
        */
        inline size_t GetNbOpcodes()const 
        {
            if(m_ver == eOpCodeVersion::EoS)
                return GetNbOpCodes_EoS();
            else if(m_ver == eOpCodeVersion::EoTD)
                return GetNbOpCodes_EoTD();
            else
            {
                assert(false);
                return 0;
            }
        }

        inline eOpCodeVersion Version()const
        {
            return m_ver;
        }

    private:
        eOpCodeVersion m_ver;
    };




};

#endif
