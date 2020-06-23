#ifndef OPCODE_CATEGORIES_HPP
#define OPCODE_CATEGORIES_HPP
#include <cstdint>

namespace pmd2
{
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
};
#endif