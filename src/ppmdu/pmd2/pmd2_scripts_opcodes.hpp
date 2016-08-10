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
//  15b signed integer support
//==========================================================================================================
    class int15_t
    {
    public:
        // --- Construction ---
        inline int15_t():val(0){}

        inline int15_t(const int15_t & cp)
        {this->operator=(cp);}

        inline int15_t & operator=(const int15_t & cp)
        {val = cp.val;}

        //
        inline explicit int15_t(uint16_t otherval)
        {this->operator=(otherval);}

        inline explicit int15_t(int otherval)
        {this->operator=(otherval);}

        inline int15_t(unsigned int otherval)
        {this->operator=(otherval);}

        inline int15_t & operator=(uint16_t otherval)
        {val = Convert16bTo15b(otherval);}

        inline int15_t & operator=(int otherval)
        {val = Convert16bTo15b(static_cast<unsigned int>(otherval));}

        inline int15_t & operator=(unsigned int otherval)
        {val = Convert16bTo15b(static_cast<unsigned int>(otherval));}

        // --- Operators ---

        //Logicals
        inline bool operator! ()const                      { return !val; }
        inline bool operator==(const int15_t & other)const { return val == other.val; }
        inline bool operator!=(const int15_t & other)const { return !operator==(other); }
        inline bool operator< (const int15_t & other)const { return val < other.val; }
        inline bool operator> (const int15_t & other)const { return val > other.val; }
        inline bool operator<=(const int15_t & other)const { return !operator>(other); }
        inline bool operator>=(const int15_t & other)const { return !operator<(other); }

        //Bitwises
        inline int15_t operator|(const int15_t & other)const { return (val | other.val); }
        inline int15_t operator&(const int15_t & other)const { return (val & other.val); }
        inline int15_t operator^(const int15_t & other)const { return (val ^ other.val); }
        inline int15_t operator~()const                      { return (~val) & Mask15b; }

        inline int15_t & operator|=(const int15_t & other) { return ((*this) = operator|(other)); }
        inline int15_t & operator&=(const int15_t & other) { return ((*this) = operator&(other)); }
        inline int15_t & operator^=(const int15_t & other) { return ((*this) = operator^(other)); }

        inline int15_t operator>>(unsigned int shiftamt)const { return (val >> shiftamt) & Mask15b; }
        inline int15_t operator<<(unsigned int shiftamt)const { return (val << shiftamt) & Mask15b; }
        inline int15_t & operator>>=(unsigned int shiftamt) {return ((*this) = operator>>(shiftamt) );}
        inline int15_t & operator<<=(unsigned int shiftamt) {return ((*this) = operator<<(shiftamt) );}

        //Arithmetics
        inline int15_t operator+(const int15_t & other)const {return (val + other.val) & Mask15b;}
        inline int15_t operator-(const int15_t & other)const {return (val - other.val) & Mask15b;}
        inline int15_t operator*(const int15_t & other)const {return (val * other.val) & Mask15b;}
        inline int15_t operator/(const int15_t & other)const {return (val / other.val) & Mask15b;}
        inline int15_t operator%(const int15_t & other)const {return (val % other.val) & Mask15b;}

        inline int15_t & operator+=(const int15_t & other) {return ((*this) = operator+(other));}
        inline int15_t & operator-=(const int15_t & other) {return ((*this) = operator-(other));}
        inline int15_t & operator*=(const int15_t & other) {return ((*this) = operator*(other));}
        inline int15_t & operator/=(const int15_t & other) {return ((*this) = operator/(other));}
        inline int15_t & operator%=(const int15_t & other) {return ((*this) = operator%(other));}

        inline int15_t & operator++()    {return this->operator+=(1);}
        inline int15_t   operator++(int) { int15_t tmp(*this); operator++(); return tmp;} //post increment
        inline int15_t & operator--()    {return this->operator-=(1);}
        inline int15_t   operator--(int) { int15_t tmp(*this); operator++(); return tmp;} //post decrement

        // --- Cast ---
        inline operator uint16_t()const {return Convert15bTo16b(val);}
        inline operator int16_t ()const {return Convert15bTo16b(val);}

        // --- Conversion ---
        static inline int16_t Convert15bTo16b(uint16_t value)
        {
            return (value >= 0x4000u)? (value | 0xFFFF8000u) : (value & 0x3FFFu);
        }

        //!#TODO: Double check this!!
        static inline uint16_t Convert16bTo15b(uint16_t value)
        {
            if(value == 0)
                return 0;
            else if( static_cast<int16_t>(value) < 0)
                return (value & 0x3FFFu) | 0x4000u;     //If the value was negative, set the negative bit
            return value & 0x3FFFu;
        }

    private:
        uint16_t val;
        static const uint16_t Mask15b = 0x7FFFu;
    };



//==========================================================================================================
// OpCodes
//==========================================================================================================
    /*
        eOpCodeVersion
            The version of the script/opcodes to handle. 
            So far, 2 known versions exists tied to either Time/Darkness or Sky.
    */
    enum struct eOpCodeVersion
    {
        EoS,
        EoTD,
        Invalid,
    };

    const uint16_t      NullOpCode          = 0;                   //The Null opcode is the same across all versions of the opcodes!
    const uint16_t      InvalidOpCode       = std::numeric_limits<uint16_t>::max();
    const uint16_t      ScriptNullVal       = 0x7FFF;              //The value that represents null in the scripts
    const std::string   ScriptNullValName   = "NULL";               //The textual representation of the script null value!

//==========================================================================================================
//  EoTD OpCodes
//==========================================================================================================
    enum struct eScriptOpCodesEoTD : uint16_t 
    {
        Null = NullOpCode,
        back_ChangeGround,
        back_SetBackEffect,
        back_SetBackScrollOffset,
        back_SetBackScrollSpeed,
        back_SetBanner,
        back_SetEffect,
        back_SetDungeonBanner,
        back_SetGround,
        back_SetTitleBanner,
        back_SetWeather,
        back_SetWeatherEffect,
        back_SetWeatherScrollOffset,
        back_SetWeatherScrollSpeed,
        back2_SetBackEffect,
        back2_SetBackScrollOffset,
        back2_SetBackScrollSpeed,
        back2_SetData,
        back2_SetEffect,
        back2_SetGround,
        back2_SetMode,
        back2_SetSpecialActing,
        back2_SetWeather,
        back2_SetWeatherEffect,
        back2_SetWeatherScrollOffset,
        back2_SetWeatherScrollSpeed,
        bgm_FadeOut,
        bgm_Play,
        bgm_PlayFadeIn,
        bgm_Stop,
        bgm_ChangeVolume,
        bgm2_FadeOut,
        bgm2_Play,
        bgm2_PlayFadeIn,
        bgm2_Stop,
        bgm2_ChangeVolume,
        Branch,
        BranchBit,
        BranchDebug,
        BranchEdit,
        BranchExecuteSub,
        BranchPerformance,
        BranchScenarioNow,
        BranchScenarioNowAfter,
        BranchScenarioNowBefore,
        BranchScenarioAfter,
        BranchScenarioBefore,
        BranchSum,
        BranchValue,
        BranchVariable,
        BranchVariation,
        Call,
        CallCommon,
        camera_Move2Default,
        camera_Move2MyPosition,
        camera_Move2Myself,
        camera_Move2PositionMark,
        camera_Move3Default,
        camera_Move3MyPosition,
        camera_Move3Myself,
        camera_Move3PositionMark,
        camera_MoveDefault,
        camera_MoveMyPosition,
        camera_MoveMyself,
        camera_MovePositionMark,
        camera_SetDefault,
        camera_SetEffect,
        camera_SetMyPosition,
        camera_SetMyself,
        camera_SetPositionMark,
        camera2_Move2Default,
        camera2_Move2MyPosition,
        camera2_Move2Myself,
        camera2_Move2PositionMark,
        camera2_Move3Default,
        camera2_Move3MyPosition,
        camera2_Move3Myself,
        camera2_Move3PositionMark,
        camera2_MoveDefault,
        camera2_MoveMyPosition,
        camera2_MoveMyself,
        camera2_MovePositionMark,
        camera2_SetDefault,
        camera2_SetEffect,
        camera2_SetMyPosition,
        camera2_SetMyself,
        camera2_SetPositionMark,
        Case,
        CaseMenu,
        CaseScenario,
        CaseText,
        CaseValue,
        CaseVariable,
        debug_Assert,
        debug_Print,
        debug_PrintFlag,
        debug_PrintScenario,
        DefaultText,
        Destroy,
        End,
        ExecuteActing,
        ExecuteCommon,
        flag_CalcBit,
        flag_CalcValue,
        flag_CalcVariable,
        flag_Clear,
        flag_Initial,
        flag_Set,
        flag_ResetDungeonResult,
        flag_ResetScenario,
        flag_SetAdventureLog,
        flag_SetDungeonMode,
        flag_SetDungeonResult,
        flag_SetPerformance,
        flag_SetScenario,
        Flash,
        Hold,
        Jump,
        JumpCommon,
        lives,
        LoadPosition,
        Lock,
        main_EnterAdventure,
        main_EnterDungeon,
        main_EnterGround,
        main_EnterGroundMulti,
        main_EnterRescueUser,
        main_EnterTraining,
        main_EnterTraining2,
        main_SetGround,
        me_Play,
        me_Stop,
        message_Close,
        message_CloseEnforce,
        message_Explanation,
        message_FacePositionOffset,
        message_ImitationSound,
        message_KeyWait,
        message_Mail,
        message_Menu,
        message_Monologue,
        message_Narration,
        message_Notice,
        message_EmptyActor,
        message_ResetActor,
        message_SetActor,
        message_SetFace,
        message_SetFaceEmpty,
        message_SetFaceOnly,
        message_SetFacePosition,
        message_SetWaitMode,
        message_SpecialTalk,
        message_SwitchMenu,
        message_SwitchMonologue,
        message_SwitchTalk,
        message_Talk,
        Move2Position,
        Move2PositionLives,
        Move2PositionMark,
        Move2PositionOffset,
        Move2PositionOffsetRandom,
        Move3Position,
        Move3PositionLives,
        Move3PositionMark,
        Move3PositionOffset,
        Move3PositionOffsetRandom,
        MoveDirection,
        MoveHeight,
        MovePosition,
        MovePositionLives,
        MovePositionLivesTime,
        MovePositionMark,
        MovePositionMarkTime,
        MovePositionOffset,
        MoveSpecial,
        MoveTurn,
        object,
        performer,
        ProcessSpecial,
        ResetAttribute,
        ResetFunctionAttribute,
        ResetHitAttribute,
        ResetOutputAttribute,
        ResetReplyAttribute,
        Return,
        SavePosition,
        screen_FadeChange,
        screen_FadeChangeAll,
        screen_FadeIn,
        screen_FadeInAll,
        screen_FadeOut,
        screen_FadeOutAll,
        screen_FlushChange,
        screen_FlushIn,
        screen_FlushOut,
        screen_WhiteChange,
        screen_WhiteChangeAll,
        screen_WhiteIn,
        screen_WhiteInAll,
        screen_WhiteOut,
        screen_WhiteOutAll,
        screen2_FadeChange,
        screen2_FadeChangeAll,
        screen2_FadeIn,
        screen2_FadeInAll,
        screen2_FadeOut,
        screen2_FadeOutAll,
        screen2_FlushChange,
        screen2_FlushIn,
        screen2_FlushOut,
        screen2_WhiteChange,
        screen2_WhiteChangeAll,
        screen2_WhiteIn,
        screen2_WhiteInAll,
        screen2_WhiteOut,
        screen2_WhiteOutAll,
        se_FadeOut,
        se_Play,
        se_Stop,
        SetAnimation,
        SetAttribute,
        SetDirection,
        SetDirectionLives,
        SetEffect,
        SetFunctionAttribute,
        SetHeight,
        SetHitAttribute,
        SetMoveRange,
        SetOutputAttribute,
        SetPosition,
        SetPositionInitial,
        SetPositionLives,
        SetPositionMark,
        SetPositionOffset,
        SetPositionOffsetRandom,
        SetReplyAttribute,
        Slide2Position,
        Slide2PositionLives,
        Slide2PositionMark,
        Slide2PositionOffset,
        Slide2PositionOffsetRandom,
        Slide3Position,
        Slide3PositionLives,
        Slide3PositionMark,
        Slide3PositionOffset,
        Slide3PositionOffsetRandom,
        SlideHeight,
        SlidePosition,
        SlidePositionLives,
        SlidePositionLivesTime,
        SlidePositionMark,
        SlidePositionMarkTime,
        SlidePositionOffset,
        sound_FadeOut,
        sound_Stop,
        supervision_Acting,
        supervision_ExecuteActing,
        supervision_ExecuteActingSub,
        supervision_ExecuteCommon,
        supervision_ExecuteEnter,
        supervision_ExecuteStation,
        supervision_ExecuteStationCommon,
        supervision_ExecuteStationCommonSub,
        supervision_ExecuteStationSub,
        supervision_ExecuteExport,
        supervision_LoadStation,
        supervision_Remove,
        supervision_RemoveActing,
        supervision_RemoveCommon,
        supervision_SpecialActing,
        supervision_Station,
        supervision_StationCommon,
        supervision_Suspend,
        supervision2_SpecialActing,
        Switch,
        SwitchDirection,
        SwitchDirectionLives,
        SwitchDirectionLives2,
        SwitchDirectionMark,
        SwitchDungeonMode,
        SwitchLives,
        SwitchRandom,
        SwitchScenario,
        SwitchScenarioLevel,
        SwitchSector,
        SwitchValue,
        SwitchVariable,
        Turn2Direction,
        Turn2DirectionLives,
        Turn2DirectionLives2,
        Turn2DirectionMark,
        Turn2DirectionTurn,
        TurnDirection,
        TurnDirectionLives,
        TurnDirectionLives2,
        TurnDirectionMark,
        Unlock,
        Wait,
        WaitAnimation,
        WaitBackEffect,
        WaitBack2Effect,
        WaitBgm,
        WaitBgm2,
        WaitBgmSignal,
        WaitEffect,
        WaitExecuteLives,
        WaitExecuteObject,
        WaitExecutePerformer,
        WaitFadeIn,
        WaitLockLives,
        WaitLockObject,
        WaitLockPerformer,
        WaitLockSupervision,
        WaitMe,
        WaitMoveCamera,
        WaitMoveCamera2,
        WaitRandom,
        WaitScreenFade,
        WaitScreenFadeAll,
        WaitScreen2Fade,
        WaitSe,
        WaitSpecialActing,
        WaitSubScreen,
        WaitSubSpecialActing,
        worldmap_BlinkMark,
        worldmap_ChangeLevel,
        worldmap_DeleteArrow,
        worldmap_MoveCamera,
        worldmap_OffMessage,
        worldmap_SetArrow,
        worldmap_SetCamera,
        worldmap_SetLevel,
        worldmap_SetMark,
        worldmap_SetMessage,
        worldmap_SetMessagePlace,
        worldmap_SetMode,

        //This should always be last
        NBOpcodes,
        INVALID = InvalidOpCode,
    };

//==========================================================================================================
//  EoS OpCodes
//==========================================================================================================
    enum struct eScriptOpCodesEoS : uint16_t
    {
        Null = NullOpCode,
        back_ChangeGround,
        back_SetBackEffect,
        back_SetBackScrollOffset,
        back_SetBackScrollSpeed,
        back_SetBanner,
        back_SetBanner2,
        back_SetEffect,
        back_SetDungeonBanner,
        back_SetGround,
        back_SetSpecialEpisodeBanner,
        back_SetSpecialEpisodeBanner2,
        back_SetSpecialEpisodeBanner3,
        back_SetTitleBanner,
        back_SetWeather,
        back_SetWeatherEffect,
        back_SetWeatherScrollOffset,
        back_SetWeatherScrollSpeed,
        back2_SetBackEffect,
        back2_SetBackScrollOffset,
        back2_SetBackScrollSpeed,
        back2_SetData,
        back2_SetEffect,
        back2_SetGround,
        back2_SetMode,
        back2_SetSpecialActing,
        back2_SetWeather,
        back2_SetWeatherEffect,
        back2_SetWeatherScrollOffset,
        back2_SetWeatherScrollSpeed,
        bgm_FadeOut,
        bgm_Play,
        bgm_PlayFadeIn,
        bgm_Stop,
        bgm_ChangeVolume,
        bgm2_FadeOut,
        bgm2_Play,
        bgm2_PlayFadeIn,
        bgm2_Stop,
        bgm2_ChangeVolume,
        Branch,
        BranchBit,
        BranchDebug,
        BranchEdit,
        BranchExecuteSub,
        BranchPerformance,
        BranchScenarioNow,
        BranchScenarioNowAfter,
        BranchScenarioNowBefore,
        BranchScenarioAfter,
        BranchScenarioBefore,
        BranchSum,
        BranchValue,
        BranchVariable,
        BranchVariation,
        Call,
        CallCommon,
        camera_Move2Default,
        camera_Move2MyPosition,
        camera_Move2Myself,
        camera_Move2PositionMark,
        camera_Move2PositionMark2,
        camera_Move3Default,
        camera_Move3MyPosition,
        camera_Move3Myself,
        camera_Move3PositionMark,
        camera_Move3PositionMark2,
        camera_MoveDefault,
        camera_MoveMyPosition,
        camera_MoveMyself,
        camera_MovePositionMark,
        camera_MovePositionMark2,
        camera_SetDefault,
        camera_SetEffect,
        camera_SetMyPosition,
        camera_SetMyself,
        camera_SetPositionMark,
        camera2_Move2Default,
        camera2_Move2MyPosition,
        camera2_Move2Myself,
        camera2_Move2PositionMark,
        camera2_Move2PositionMark2,
        camera2_Move3Default,
        camera2_Move3MyPosition,
        camera2_Move3Myself,
        camera2_Move3PositionMark,
        camera2_Move3PositionMark2,
        camera2_MoveDefault,
        camera2_MoveMyPosition,
        camera2_MoveMyself,
        camera2_MovePositionMark,
        camera2_MovePositionMark2,
        camera2_SetDefault,
        camera2_SetEffect,
        camera2_SetMyPosition,
        camera2_SetMyself,
        camera2_SetPositionMark,
        CancelCut,
        CancelRecoverCommon,
        Case,
        CaseMenu,
        CaseMenu2,
        CaseScenario,
        CaseText,
        CaseValue,
        CaseVariable,
        debug_Assert,
        debug_Print,
        debug_PrintFlag,
        debug_PrintScenario,
        DefaultText,
        Destroy,
        End,
        EndAnimation,
        ExecuteActing,
        ExecuteCommon,
        flag_CalcBit,
        flag_CalcValue,
        flag_CalcVariable,
        flag_Clear,
        flag_Initial,
        flag_Set,
        flag_ResetDungeonResult,
        flag_ResetScenario,
        flag_SetAdventureLog,
        flag_SetDungeonMode,
        flag_SetDungeonResult,
        flag_SetPerformance,
        flag_SetScenario,
        Flash,
        Hold,
        item_GetVariable,
        item_Set,
        item_SetTableData,
        item_SetVariable,
        Jump,
        JumpCommon,
        lives,
        LoadPosition,
        Lock,
        main_EnterAdventure,
        main_EnterDungeon,
        main_EnterGround,
        main_EnterGroundMulti,
        main_EnterRescueUser,
        main_EnterTraining,
        main_EnterTraining2,
        main_SetGround,
        me_Play,
        me_Stop,
        message_Close,
        message_CloseEnforce,
        message_Explanation,
        message_FacePositionOffset,
        message_ImitationSound,
        message_KeyWait,
        message_Mail,
        message_Menu,
        message_Monologue,
        message_Narration,
        message_Notice,
        message_EmptyActor,
        message_ResetActor,
        message_SetActor,
        message_SetFace,
        message_SetFaceEmpty,
        message_SetFaceOnly,
        message_SetFacePosition,
        message_SetWaitMode,
        message_SpecialTalk,
        message_SwitchMenu,
        message_SwitchMenu2,
        message_SwitchMonologue,
        message_SwitchTalk,
        message_Talk,
        Move2Position,
        Move2PositionLives,
        Move2PositionMark,
        Move2PositionMark2,
        Move2PositionOffset,
        Move2PositionOffset2,
        Move2PositionOffsetRandom,
        Move3Position,
        Move3PositionLives,
        Move3PositionMark,
        Move3PositionMark2,
        Move3PositionOffset,
        Move3PositionOffset2,
        Move3PositionOffsetRandom,
        MoveDirection,
        MoveHeight,
        MovePosition,
        MovePositionLives,
        MovePositionLivesTime,
        MovePositionMark,
        MovePositionMark2,
        MovePositionMarkTime,
        MovePositionOffset,
        MovePositionOffset2,
        MoveSpecial,
        MoveTurn,
        object,
        PauseEffect,
        performer,
        ProcessSpecial,
        PursueTurnLives,
        PursueTurnLives2,
        ResetAttribute,
        ResetFunctionAttribute,
        ResetHitAttribute,
        ResetOutputAttribute,
        ResetReplyAttribute,
        ResumeEffect,
        Return,
        SavePosition,
        screen_FadeChange,
        screen_FadeChangeAll,
        screen_FadeIn,
        screen_FadeInAll,
        screen_FadeOut,
        screen_FadeOutAll,
        screen_FlushChange,
        screen_FlushIn,
        screen_FlushOut,
        screen_WhiteChange,
        screen_WhiteChangeAll,
        screen_WhiteIn,
        screen_WhiteInAll,
        screen_WhiteOut,
        screen_WhiteOutAll,
        screen2_FadeChange,
        screen2_FadeChangeAll,
        screen2_FadeIn,
        screen2_FadeInAll,
        screen2_FadeOut,
        screen2_FadeOutAll,
        screen2_FlushChange,
        screen2_FlushIn,
        screen2_FlushOut,
        screen2_WhiteChange,
        screen2_WhiteChangeAll,
        screen2_WhiteIn,
        screen2_WhiteInAll,
        screen2_WhiteOut,
        screen2_WhiteOutAll,
        se_ChangePan,
        se_ChangeVolume,
        se_FadeOut,
        se_Play,
        se_PlayFull,
        se_PlayPan,
        se_PlayVolume,
        se_Stop,
        SetAnimation,
        SetAttribute,
        SetBlink,
        SetDirection,
        SetDirectionLives,
        SetEffect,
        SetFunctionAttribute,
        SetHeight,
        SetHitAttribute,
        SetMoveRange,
        SetOutputAttribute,
        SetPosition,
        SetPositionInitial,
        SetPositionLives,
        SetPositionMark,
        SetPositionOffset,
        SetPositionOffsetRandom,
        SetReplyAttribute,
        SetupOutputAttributeAndAnimation,
        Slide2Position,
        Slide2PositionLives,
        Slide2PositionMark,
        Slide2PositionMark2,
        Slide2PositionOffset,
        Slide2PositionOffset2,
        Slide2PositionOffsetRandom,
        Slide3Position,
        Slide3PositionLives,
        Slide3PositionMark,
        Slide3PositionMark2,
        Slide3PositionOffset,
        Slide3PositionOffset2,
        Slide3PositionOffsetRandom,
        SlideHeight,
        SlidePosition,
        SlidePositionLives,
        SlidePositionLivesTime,
        SlidePositionMark,
        SlidePositionMark2,
        SlidePositionMarkTime,
        SlidePositionOffset,
        SlidePositionOffset2,
        sound_FadeOut,
        sound_Stop,
        StopAnimation,
        supervision_Acting,
        supervision_ActingInvisible,
        supervision_ExecuteActing,
        supervision_ExecuteActingSub,
        supervision_ExecuteCommon,
        supervision_ExecuteEnter,
        supervision_ExecuteStation,
        supervision_ExecuteStationCommon,
        supervision_ExecuteStationCommonSub,
        supervision_ExecuteStationSub,
        supervision_ExecuteExport,
        supervision_ExecuteExportSub,
        supervision_LoadStation,
        supervision_Remove,
        supervision_RemoveActing,
        supervision_RemoveCommon,
        supervision_SpecialActing,
        supervision_Station,
        supervision_StationCommon,
        supervision_Suspend,
        supervision2_SpecialActing,
        Switch,
        SwitchDirection,
        SwitchDirectionLives,
        SwitchDirectionLives2,
        SwitchDirectionMark,
        SwitchDungeonMode,
        SwitchLives,
        SwitchRandom,
        SwitchScenario,
        SwitchScenarioLevel,
        SwitchSector,
        SwitchValue,
        SwitchVariable,
        Turn2Direction,
        Turn2DirectionLives,
        Turn2DirectionLives2,
        Turn2DirectionMark,
        Turn2DirectionTurn,
        Turn3,
        TurnDirection,
        TurnDirectionLives,
        TurnDirectionLives2,
        TurnDirectionMark,
        Unlock,
        Wait,
        WaitAnimation,
        WaitBackEffect,
        WaitBack2Effect,
        WaitBgm,
        WaitBgm2,
        WaitBgmSignal,
        WaitEffect,
        WaitEndAnimation,
        WaitExecuteLives,
        WaitExecuteObject,
        WaitExecutePerformer,
        WaitFadeIn,
        WaitLockLives,
        WaitLockObject,
        WaitLockPerformer,
        WaitLockSupervision,
        WaitMe,
        WaitMoveCamera,
        WaitMoveCamera2,
        WaitRandom,
        WaitScreenFade,
        WaitScreenFadeAll,
        WaitScreen2Fade,
        WaitSe,
        WaitSpecialActing,
        WaitSubScreen,
        WaitSubSpecialActing,
        worldmap_BlinkMark,
        worldmap_ChangeLevel,
        worldmap_DeleteArrow,
        worldmap_MoveCamera,
        worldmap_OffMessage,
        worldmap_SetArrow,
        worldmap_SetCamera,
        worldmap_SetLevel,
        worldmap_SetMark,
        worldmap_SetMessage,
        worldmap_SetMessagePlace,
        worldmap_SetMode,

        //This should always be last
        NBOpcodes,
        INVALID = InvalidOpCode,
    };

//==========================================================================================================
//  Opcode Category
//==========================================================================================================

    /*
        eCommandCategory 
            Used to determine how to interpret a command.
            These are independant of the version.
    */
    //!When adding category, don't forget to edit the OpCodeInfoWrapper::GetMyCategory() function to reflect the changes!!
    enum struct eCommandCat : uint16_t
    {
        SingleOp = 0,           //Simple command, default
        OpWithReturnVal,        //A single op that returns a value and may have a set of cases appended to it
        Debug,                  //For debug instruction that don't work in the retail game

        Switch,                 //This marks the start of a conditional structure
        Case,                   //This marks a case in a previous conditional structure
        CaseNoJump,             //This marks a case that doesn't triggers a jump if true!
        Default,                //This marks the default case in a conditional structure

        //Accessors             
        EntityAccessor,         //For accessors like lives, object, and performer
        EntAttribute,           //For things that modifies an entity's attributes.

        //Sub-categories
        //WaitCmd,                //One of the wait command

        //Invocation
        Jump,                   //
        JumpCommon,             //
        Call,                   //
        CallCommon,             //
        BranchCmd,              //For Branch commands
        ProcSpec,               //For ProcessSpecial command

        //Other Special
        EnterAdventure,         //For main_EnterAdventure, since it can have a set of cases applied to its return value.

        //Special commands
        Null,                   //Specifically for the Null command
        Lock,                   //Specifically for the Lock command
        Unlock,                 //Specifically for the Unlock command
        End,                    //Specifically for the End command
        Return,                 //Specifically for the Return command
        Hold,                   //Specifically for the Hold command
        Destroy,                //Specifically for the Destroy command

        NbCat,
        Invalid,
    };

//==========================================================================================================
//  Parameter Info
//==========================================================================================================

    /*
        A way to categorize the way a parameter should be interpreted!
    */
    enum struct eOpParamTypes : uint8_t
    {
        UNK_Placeholder = 0,    //For when we need a placeholder

        Integer,                //Signed 16 bits word
        UInteger,               //unsigned 16 bits word
        Boolean,                //1 or 0
        BitsFlag,               //Represented as hexadecimal

        Constant,               //Reference to a constant
        String,                 //Reference to a string in the string block, by index

        //References
        Unk_LivesRef,           //For the "lives" accessor
        Unk_PerformerRef,       //For the "performer" accessor
        Unk_ObjectRef,          //For the "object" accessor
        Unk_ScriptVariable,     //For script engine/game state variables.
        ScenarioId,             //The id of the current scenario in the scenario table (event table)
        Unk_ProcSpec,           //First parameter of "ProcessSpecial" command, (is a 14bits unsigned integer)
        Unk_FaceType,           //A portrait ID to use in the set face commands
        Unk_BgmTrack,           //A track ID for a music track to play. Begins at 1.
        Unk_CRoutineId,         //An id to an instruction group in unionall.ssb
        Unk_RoutineId,          //An id to a local instruction group
        Unk_AnimationID,        //Parameter containing the ID of an animation.
        Unk_FacePosMode,        //Value from the Face position enum, 24 possible positions
        Unk_LevelId,            //ID of the level in the level entry table!
        StationId,              //ID of a layer in script data for a level! Called a "station"
        MenuID,                 //ID of a menu in the game data
        ActingLayerID,          //ID of a layer within an ssa file being run.

        //
        Unk_MvSlSpecInt,        //First parameter of the MovePositionOffset, MovePositionMark, SlidePositionMark, Slide2PositionMark, camera_Move2Default
                                //Third parameter of camera_SetEffect

        //Specifics
        Direction,              //A sprite direction 
        Duration,               //A duration in possibly ticks or milliseconds
        CoordinateX,             //A coordinate on X axis
        CoordinateY,             //A coordinate on Y axis
        InstructionOffset,      //An offset within the list of instructions to a specific instruction.
        Volume,                 //A sound volume
        //Speed,                  //An arbitrary speed value indicating the rate at which an action is performed

        NbTypes,
        Invalid,
    };
    const std::array<std::string, static_cast<size_t>(eOpParamTypes::NbTypes)> OpParamTypesNames
    {{
        "param",
        "int",
        "uint",
        "bool",
        "flag",

        "constref",
        "strref",

        "actorid",          
        "Unk_PerformerRef",      
        "objectid",         
        "svar",    
        "scenario", 
        "procspec",
        "face",
        "bgm",
        "croutineid",         //An id to an instruction group in unionall.ssb
        "lroutineid",          //An id to a local instruction group
        "animid",
        "facemode",                     //
        "levelid",                  //
        "stationid",
        "menuid",
        "layerid",

        "Unk_EncInt",

        "direction",
        "duration",              
        "x",            
        "y",
        "tolabel",          //String is label id when importing/exporting from xml
        "vol",
    }};

    inline const std::string * OpParamTypesToStr( eOpParamTypes ty )
    {
        if( ty < eOpParamTypes::NbTypes )
            return std::addressof( OpParamTypesNames[static_cast<size_t>(ty)] );
        else
            return nullptr;
    }

    inline eOpParamTypes FindOpParamTypesByName( const std::string & name )
    {
        for( size_t i = 0; i < OpParamTypesNames.size(); ++i )
        {
            if( OpParamTypesNames[i] == name )
                return static_cast<eOpParamTypes>(i);
        }
        return eOpParamTypes::Invalid;
    }

    struct OpParamInfo
    {
        eOpParamTypes ptype;
    };

//==========================================================================================================
//  LevelEntryInfo
//==========================================================================================================
    const int16_t       InvalidLevelID = ScriptNullVal;
    const std::string   NullLevelId    = ScriptNullValName; //For 0x7FFF values (-1)


//==========================================================================================================
//  ProcessSpecial
//==========================================================================================================
    const uint16_t ProcessSpecialMaxVal = 0x3E;

//==========================================================================================================
//  Face Position
//==========================================================================================================
    const int16_t InvalidFaceModeID = ScriptNullVal;

//==========================================================================================================
//  Faces
//==========================================================================================================

    const int16_t       InvalidFaceID   = ScriptNullVal;
    const std::string   NullFaceName    = ScriptNullValName; //It seems like the null(-1) face comes up a lot, so, I made a default value for it!
    const int16_t       NullFaceID      = InvalidFaceID; //It seems like the null(-1) face comes up a lot, so, I made a default value for it!

//==========================================================================================================
//  Lives Entities
//==========================================================================================================

    const int16_t InvalidLivesID = ScriptNullVal;

//
//
//
    const int16_t InvalidCRoutineID = ScriptNullVal;

//==========================================================================================================
//  ScriptEngineVariable
//==========================================================================================================

    const int16_t InvalidGameVariableID = ScriptNullVal;

//==========================================================================================================
//  EoTD OpCode Data
//==========================================================================================================
    /*************************************************************************************
        OpCodeInfoEoTD
            Structure for holding info on individual opcodes.
    **************************************************************************************/
    struct OpCodeInfoEoTD
    {
        std::string             name;
        int8_t                  nbparams;
        eCommandCat              cat;       //Category  the instruction fits in
        std::vector<OpParamInfo> paraminfo; //Info on each parameters the opcode takes
        
    };

    /*************************************************************************************
        OpCodesInfoListEoTD
            Contains info on every opcodes
    *************************************************************************************/
    extern const std::array<OpCodeInfoEoTD, static_cast<uint16_t>(eScriptOpCodesEoTD::NBOpcodes)>  OpCodesInfoListEoTD;

    /*************************************************************************************
        FindOpCodeInfo_EoTD
            Return the opcode info from the opcode info table, for the opcode specified.
            Returns null if the opcode is out of range!
    *************************************************************************************/
    inline const OpCodeInfoEoTD * FindOpCodeInfo_EoTD( uint16_t opcode )
    {
        if( opcode < OpCodesInfoListEoTD.size() )
            return &(OpCodesInfoListEoTD[opcode]);
        else
            return nullptr;
    }

    inline const OpCodeInfoEoTD * FindOpCodeInfo_EoTD( eScriptOpCodesEoTD opcode )
    {
        return FindOpCodeInfo_EoTD( static_cast<uint16_t>(opcode) );
    }

    inline eScriptOpCodesEoTD FindOpCodeByName_EoTD( const std::string & name, size_t nbparams )
    {
        for( size_t i = 0; i < OpCodesInfoListEoTD.size(); ++i )
        {
            if( OpCodesInfoListEoTD[i].name == name && OpCodesInfoListEoTD[i].nbparams == nbparams )
                return static_cast<eScriptOpCodesEoTD>(i);
        }
        return eScriptOpCodesEoTD::INVALID;
    }

    inline size_t GetNbOpCodes_EoTD()
    {
        return OpCodesInfoListEoTD.size();
    }

    /*************************************************************************************
        IntToOpCodeEoTD
            Turns an integer to a a opcode for EoTD
    *************************************************************************************/
    inline eScriptOpCodesEoTD IntToOpCodeEoTD( uint16_t opcodeval )
    {
        if( opcodeval < static_cast<uint16_t>(eScriptOpCodesEoTD::NBOpcodes) )
            return static_cast<eScriptOpCodesEoTD>(opcodeval);
        else
            return eScriptOpCodesEoTD::INVALID;
    }

//==========================================================================================================
//  EoS OpCode Data
//==========================================================================================================
    /*************************************************************************************
        OpCodeInfoEoS
            Structure for holding info on individual opcodes.
    **************************************************************************************/
    struct OpCodeInfoEoS
    {
        std::string         name;
        int8_t              nbparams;
        int8_t              unk1;
        int8_t              unk2;
        int8_t              unk3;
        eCommandCat         cat;       //Category  the instruction fits in
        std::vector<OpParamInfo> paraminfo; //Info on each parameters the opcode takes
        
    };


    /*************************************************************************************
        OpCodesInfoListEoS
            Contains info on every opcodes
    *************************************************************************************/
    extern const std::array<OpCodeInfoEoS, static_cast<uint16_t>(eScriptOpCodesEoS::NBOpcodes)> OpCodesInfoListEoS;

    /*************************************************************************************
        FindOpCodeInfo_EoS
            Return the opcode info from the opcode info table, for the opcode specified.
            Returns null if the opcode is out of range!
    *************************************************************************************/
    inline const OpCodeInfoEoS * FindOpCodeInfo_EoS( uint16_t opcode )
    {
        if( opcode < OpCodesInfoListEoS.size() )
            return &(OpCodesInfoListEoS[opcode]);
        else
            return nullptr;
    }

    inline const OpCodeInfoEoS * FindOpCodeInfo_EoS( eScriptOpCodesEoS opcode )
    {
        return FindOpCodeInfo_EoS( static_cast<uint16_t>(opcode) );
    }

    eScriptOpCodesEoS FindOpCodeByName_EoS( const std::string & name, size_t nbparams );


    inline size_t GetNbOpCodes_EoS()
    {
        return OpCodesInfoListEoS.size();
    }

    /*************************************************************************************
        IntToOpCodeEoS
            Turns an integer to a a opcode for EoS
    *************************************************************************************/
    inline eScriptOpCodesEoS IntToOpCodeEoS( uint16_t opcodeval )
    {
        if( opcodeval < static_cast<uint16_t>(eScriptOpCodesEoS::NBOpcodes) )
            return static_cast<eScriptOpCodesEoS>(opcodeval);
        else
            return eScriptOpCodesEoS::INVALID;
    }


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
        Returns whether a routines accepts a value as its extra parameter value.
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
//  Utilities
//=====================================================================================
    inline eOpCodeVersion GameVersionToOpCodeVersion( eGameVersion ver )
    {
        if( ver == eGameVersion::EoD || ver == eGameVersion::EoT )
            return eOpCodeVersion::EoTD;
        else if( ver == eGameVersion::EoS )
            return eOpCodeVersion::EoS;
        else 
            return eOpCodeVersion::Invalid;
    }

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


//=====================================================================================
//  Parameter Value Handling
//=====================================================================================
    /*
    */
    class ParameterReferences
    {
    public:
        ParameterReferences( const ConfigLoader & conf )
            :m_gconf(conf)
        {}

        //Face Names
        const std::string Face( int16_t id )const;

        inline int16_t Face( const std::string & name )const 
        {
            return FindIDByName<NullFaceID>(m_gconf.GetGameScriptData().FaceNames(), name);
        }

        //Face Posistion Modes
        inline const std::string * FacePosMode( int16_t id )const 
        {
            return m_gconf.GetGameScriptData().FacePosModes().FindByIndex(id);
        }

        inline int16_t FacePosMode( const std::string & name )const 
        {
            return FindIDByName<ScriptNullVal>( m_gconf.GetGameScriptData().FacePosModes(), name );
        }

        //Common Routine Info
        inline const commonroutine_info * CRoutine( int16_t id )const 
        {
            return m_gconf.GetGameScriptData().CommonRoutineInfo().FindByIndex(id);
        }

        inline int16_t CRoutine( const std::string & name )const 
        {
            return FindIDByName<ScriptNullVal>( m_gconf.GetGameScriptData().CommonRoutineInfo(), name );
        }

        //Level Info
        inline const level_info * LevelInfo( int16_t id )const 
        {
            return m_gconf.GetGameScriptData().LevelInfo().FindByIndex(id);
        }

        inline int16_t LevelInfo( const std::string & name )const 
        {
            return FindIDByName<ScriptNullVal>( m_gconf.GetGameScriptData().LevelInfo(), name );
        }

        //Lives Info
        inline const livesent_info * LivesInfo( int16_t id )const 
        {
            return m_gconf.GetGameScriptData().LivesEnt().FindByIndex(id);
        }

        inline int16_t LivesInfo( const std::string & name )const 
        {
            return FindIDByName<ScriptNullVal>( m_gconf.GetGameScriptData().LivesEnt(), name );
        }

        //GameVar Info
        inline const gamevariable_info * GameVarInfo( int16_t id )const 
        {
            if( id > 0x400 ) //Extended game var starts at 0x400
                return m_gconf.GetGameScriptData().ExGameVariables().FindByIndex(id - 0x400);
            else
                return m_gconf.GetGameScriptData().GameVariables().FindByIndex(id);
        }

        inline int16_t GameVarInfo( const std::string & name )const 
        {
            size_t ret = m_gconf.GetGameScriptData().GameVariables().FindIndexByName(name);

            if( ret == std::numeric_limits<size_t>::max() )
            {
                ret = m_gconf.GetGameScriptData().ExGameVariables().FindIndexByName(name);
                if(ret != ret == std::numeric_limits<size_t>::max() )
                    ret += 0x400; //Extended game var starts at 0x400
            }

            return ConvertInvalidOffsetToInvalidInt16(ret);
        }

        //Object Info
        inline const object_info * ObjectInfo( int16_t id )const
        {
            return m_gconf.GetGameScriptData().ObjectsInfo().FindByIndex(id);
        }

        inline int16_t ObjectInfo( const std::string & name )const 
        {
            return FindIDByName<ScriptNullVal>( m_gconf.GetGameScriptData().ObjectsInfo(), name );
        }


        std::string ObjectIDToStr(int16_t id)const
        {
            std::stringstream sstr;
            const auto * inf = ObjectInfo(id);
            sstr <<id;
            if( inf )
                sstr <<"_" <<inf->name;
            return sstr.str();
        }

        int16_t StrToObjectID(const std::string & name)
        {
            std::stringstream sstr;
            uint16_t          parsedid = 0;
            if( DoesStringBeginsWithNumber(name) )
            {
                sstr << name;
                sstr >> parsedid;
                //! #FIXME: Verify it or somthing?
            }
            else
            {
                throw std::runtime_error("ParameterReferences::ParseObjectNameIDString(): Object id " + name + ", is missing object number! Can't reliably pinpoint the correct object instance!");
            }
            return parsedid;
        }

        //DirectionData (For use in script data!)
        inline const std::string & DirectionData( int16_t dir )const
        {
            const std::string * pstr = m_gconf.GetGameScriptData().Directions().FindByIndex((dir - 1)); //Directions go from 1 to 8!
            if(!pstr)
                return ScriptNullValName;
            else
                return *pstr;
        }

        inline int16_t DirectionData( const std::string & name )const
        {
            int16_t dirid = FindIDByName<ScriptNullVal>( m_gconf.GetGameScriptData().Directions(), name );
            if( DoesStringBeginsWithNumber(name) )
                return dirid;  //Since it got converted literally from a number, we don't add 1 !!
            else 
                return dirid + 1;  //Directions go from at 1 to 8!
        }

        //Direction (For use in script parameters)
        inline const std::string & Direction( int16_t dir )const
        {
            const std::string * pstr = m_gconf.GetGameScriptData().Directions().FindByIndex(dir); //Directions go from 0 to 7!
            if(!pstr)
                return ScriptNullValName;
            else
                return *pstr;
        }

        inline int16_t Direction( const std::string & name )const
        {
            return FindIDByName<ScriptNullVal>( m_gconf.GetGameScriptData().Directions(), name );
        }

    private:

        static inline bool DoesStringBeginsWithNumber(const std::string & str)
        {
            return (std::isdigit(str.front(), std::locale::classic() ) || str.front() == '-');
        }

        //This check if the value is std::numeric_limits<size_t>::max(), the error value when no index was found,
        // into the int16 error value used in the script engine.
        static inline int16_t ConvertInvalidOffsetToInvalidInt16( size_t val )
        {
            if( val == std::numeric_limits<size_t>::max() )
                return ScriptNullVal;
            else
                return static_cast<int16_t>(val);
        }

        template<int16_t _INVALIDID, class _EntryTy>
            inline const std::string FindByIndex( _EntryTy container, int16_t id, const std::string & invalidstr = ScriptNullValName )const 
        { 
            if( id == _INVALIDID )
                return invalidstr;

            const std::string * pstr = container.FindByIndex(id);
            if(pstr)
                return *pstr;
            else
            {
                std::stringstream sstr;
                sstr << id;
                return sstr.str();
            }
        }


        template<int16_t _INVALIDID, class _EntryTy>
            inline int16_t FindIDByName( _EntryTy container, const std::string & name, const std::string & invalidstr = ScriptNullValName )const 
        { 
            if( name == invalidstr )
                return _INVALIDID;

            const size_t id = container.FindIndexByName(name);
            if(id != std::numeric_limits<size_t>::max())
                return static_cast<int16_t>(id);
            else
            {
                //std::stringstream sstr;
                //int16_t outval = 0;
                //if(name.size() > 2 && name[0] == '0' && name[1] == 'x')
                //    sstr<<std::hex;
                //sstr << name;
                //sstr >> outval;
                return utils::parseHexaValToValue<int16_t>(name);
            }
        }

    private:
        const ConfigLoader & m_gconf;
    };






};

#endif
