#pragma once
#include <utils/parse_utils.hpp>
#include <cstdint>
#include <array>
#include <string>

namespace pmd2
{
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

		ItemID,                 //ID of an item in-game

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

	/*
		Matching names for the parameter types
	*/
	const std::array<std::string, static_cast<size_t>(eOpParamTypes::NbTypes)> OpParamTypesNames
	{ {
		"param",
		"int",
		"uint",
		"bool",
		"flag",

		"constref",
		"strref",

		"actorid",
		"performerid",
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

		"itemid",

		"Unk_EncInt",

		"direction",
		"duration",
		"x",
		"y",
		"tolabel",          //String is label id when importing/exporting from xml
		"vol",
	} };

	//!#NOTE: Add any new paramter type to the ParseTypedCommandParameterAttribute function in the SSB XML parser! Or end up with a crash.

	
	/*
		Overridable struct for containing and passing parameter info
	*/
	struct OpParamInfo
	{
		eOpParamTypes ptype;
	};

//
//
//
	inline const std::string * OpParamTypesToStr(eOpParamTypes ty)
	{
		if (ty < eOpParamTypes::NbTypes)
			return std::addressof(OpParamTypesNames[static_cast<size_t>(ty)]);
		else
			return nullptr;
	}

	inline eOpParamTypes FindOpParamTypesByName(const std::string & name)
	{
		for (size_t i = 0; i < OpParamTypesNames.size(); ++i)
		{
			if (OpParamTypesNames[i] == name)
				return static_cast<eOpParamTypes>(i);
		}
		return eOpParamTypes::Invalid;
	}

//=====================================================================================
//  Parameter Value Handling
//=====================================================================================
    /*
		Helper to handle parsing properly the values of an opcode's parameters
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
            if( !DoesStringBeginsWithNumber(name) )
                return FindIDByName<NullFaceID>(m_gconf.GetGameScriptData().FaceNames(), name, ScriptNullValName, false ); //Don't allow converting to a number when not found!
            else
                return utils::parseHexaValToValue<int16_t>(name);
        }

        //Face Posistion Modes
        inline const std::string * FacePosMode( int16_t id )const 
        {
            return m_gconf.GetGameScriptData().FacePosModes().FindByIndex(id);
        }

        inline int16_t FacePosMode( const std::string & name )const 
        {
            if( !DoesStringBeginsWithNumber(name) )
                return FindIDByName<ScriptNullVal>( m_gconf.GetGameScriptData().FacePosModes(), name, ScriptNullValName, false ); //Don't allow converting to a number when not found!
            else
                return utils::parseHexaValToValue<int16_t>(name);
        }

        //Common Routine Info
        inline const commonroutine_info * CRoutine( int16_t id )const 
        {
            return m_gconf.GetGameScriptData().CommonRoutineInfo().FindByIndex(id);
        }

        inline int16_t CRoutine( const std::string & name )const 
        {
            if( !DoesStringBeginsWithNumber(name) )
                return FindIDByName<InvalidCRoutineID>( m_gconf.GetGameScriptData().CommonRoutineInfo(), name, ScriptNullValName, false ); //Don't allow converting to a number when not found!
            else
                return utils::parseHexaValToValue<int16_t>(name);
        }

        //Level Info
        inline const level_info * LevelInfo( int16_t id )const 
        {
            return m_gconf.GetGameScriptData().LevelInfo().FindByIndex(id);
        }

        inline int16_t LevelInfo( const std::string & name )const 
        {
            if( !DoesStringBeginsWithNumber(name) )
                return FindIDByName<ScriptNullVal>( m_gconf.GetGameScriptData().LevelInfo(), name, ScriptNullValName, false ); //Don't allow converting to a number when not found!
            else
                return utils::parseHexaValToValue<int16_t>(name);
        }

        //Lives Info
        inline const livesent_info * LivesInfo( int16_t id )const 
        {
            return m_gconf.GetGameScriptData().LivesEnt().FindByIndex(id);
        }

        inline int16_t LivesInfo( const std::string & name )const 
        {
            if( !DoesStringBeginsWithNumber(name) )
                return FindIDByName<ScriptNullVal>( m_gconf.GetGameScriptData().LivesEnt(), name ); //Don't allow converting to a number when not found!
            else
                return utils::parseHexaValToValue<int16_t>(name);
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
            if( !DoesStringBeginsWithNumber(name) )
                return FindIDByName<ScriptNullVal>( m_gconf.GetGameScriptData().ObjectsInfo(), name ); //Don't allow converting to a number when not found!
            else
                return utils::parseHexaValToValue<int16_t>(name);
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
        inline std::string DirectionData( int16_t dir )const
        {
            if( static_cast<uint16_t>(dir) > 8 )
                return std::to_string(dir); //In this case, put the value as-is

            const std::string * pstr = m_gconf.GetGameScriptData().Directions().FindByIndex((dir - 1)); //Directions go from 1 to 8!
            if(!pstr)
                return ScriptNullValName;
            else
                return *pstr;
        }

        inline int16_t DirectionData( const std::string & name )const
        {
            int16_t dirid = FindIDByName<ScriptNullVal>( m_gconf.GetGameScriptData().Directions(), name );

            //We need to do this, since a direction of 0 is invalid, but since we opted for using indices to represent directions 
            // internally in the GameScriptData, our internal invalid value of -1 must be converted to the script's invalid direction of 0! We can't do it otherwise, because 0 
            // is a valid indice in the table of directions, and we couldn't tell if its a valid direction or not afterwards when validating the result..
            if(dirid == ScriptNullVal) 
                return ScriptNullDirection;
            else if( DoesStringBeginsWithNumber(name) )
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

        //Item IDs
        inline std::string Item( int16_t itemid )const
        {
            return std::to_string(itemid);
        }

        inline int16_t Item( const std::string & name )const
        {
            if(DoesStringBeginsWithNumber(name))
                return utils::parseHexaValToValue<int16_t>(name);
            else
                throw std::runtime_error("ParameterReferences::Item(): Item id " + name + " is invalid!");
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

        //tryconverttoint : If true, will attempt converting to an integer when there are no matches!
        template<int16_t _INVALIDID, class _EntryTy>
            inline int16_t FindIDByName( _EntryTy container, const std::string & name, const std::string & invalidstr = ScriptNullValName, bool tryconverttoint = true )const 
        { 
            if( name == invalidstr )
                return _INVALIDID;

            const size_t id = container.FindIndexByName(name);

            if(id != std::numeric_limits<size_t>::max())
                return static_cast<int16_t>(id);
            else if(tryconverttoint)
                return utils::parseHexaValToValue<int16_t>(name);
            else
                return _INVALIDID;
        }

    private:
        const ConfigLoader & m_gconf;
    };
};