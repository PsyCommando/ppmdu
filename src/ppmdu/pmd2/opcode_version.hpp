#ifndef OPCODE_VERSION_HPP
#define OPCODE_VERSION_HPP
#include <ppmdu/pmd2/pmd2.hpp>
#include <cstdint>
#include <limits>
#include <array>

namespace pmd2
{
//==========================================================================================================
// OpCodes Version
//==========================================================================================================
    /*
        eOpCodeVersion
            The version of the script/opcodes to handle. 
            2 known versions exists tied to either Time/Darkness or Sky.
    */
    enum struct eOpCodeVersion
    {
        EoS,
        EoTD,
        Invalid,
    };

//==========================================================================================================
// Version common constants
//==========================================================================================================
    const uint16_t      NullOpCode				= 0;                   //The Null opcode is the same across all versions of the opcodes!
    const uint16_t      ScriptNullVal			= 0x7FFF;              //The value that represents null in the scripts
    const std::string   ScriptNullValName		= "NULL";               //The textual representation of the script null value!
	const int16_t		InvalidCRoutineID		= ScriptNullVal;
	const int16_t		InvalidGameVariableID	= ScriptNullVal;
    const uint16_t      InvalidOpCode			= std::numeric_limits<uint16_t>::max();

	//LevelEntryInfo
    const int16_t       InvalidLevelID = ScriptNullVal;
    const std::string   NullLevelId    = ScriptNullValName; //For 0x7FFF values (-1)

	//ProcessSpecial
    const uint16_t ProcessSpecialMaxVal = 0x3E;

	//Face Position
    const int16_t InvalidFaceModeID = ScriptNullVal;

	//Faces
    const int16_t       InvalidFaceID   = ScriptNullVal;
    const std::string   NullFaceName    = ScriptNullValName; //It seems like the null(-1) face comes up a lot, so, I made a default value for it!
    const int16_t       NullFaceID      = InvalidFaceID; //It seems like the null(-1) face comes up a lot, so, I made a default value for it!

	//Lives Entities
    const int16_t InvalidLivesID = ScriptNullVal;
    const int16_t ScriptNullDirection  = 0;

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
};
#endif