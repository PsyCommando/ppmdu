#ifndef OPCODE_DEFINITION_TIME_DARKNESS_HPP
#define OPCODE_DEFINITION_TIME_DARKNESS_HPP
#include <ppmdu/pmd2/opcode_version.hpp>
#include <ppmdu/pmd2/opcode_categories.hpp>
#include <ppmdu/pmd2/opcode_parameters.hpp>
#include <cstdint>

namespace pmd2
{
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
};
#endif