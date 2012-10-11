/*
 * getrules.c
 *
 *  Created on: May 13, 2011
 *      Author: rfajardo
 */

#include "getrules.h"
#include "regdata.h"
#include "quantify.h"

#include "devifdebug.h"


bool getRegAccess(Reg * reg)
{
	int count;

	if ( checkReg(reg, access, equal, write_only) )
	{
		DEVIF_DEBUG_RULES("Register access is write only.\n");
		return false;
	}
	if ( checkReg(reg, access, equal, writeOnce) )
	{
		DEVIF_DEBUG_RULES("Register access is write once.\n");
		return false;
	}

	count = howManyFields(reg, access, equal, write_only);
	if ( count != 0 )
	{
		DEVIF_DEBUG_RULES("Some fields' access is write only\n");
		return false;
	}

	count = howManyFields(reg, access, equal, writeOnce);
	if ( count != 0 )
	{
		DEVIF_DEBUG_RULES("Some fields' access is write once\n");
		return false;
	}

	return true;
}

bool getRegNeedTransfer(Reg * reg)
{
	int count;

	if ( checkReg(reg, once_read, equal, false ) )
	{
		DEVIF_DEBUG_RULES("This register was never read, cached value has to be updated.\n");
		DEVIF_DEBUG_RULES("This is a transient rule. It only applies once. As the system enters the steady state, this rule is ignored until a reset comes.\n");
		return true;
	}
	if ( checkReg(reg, is_volatile, equal, true) )
	{
		DEVIF_DEBUG_RULES("Register is volatile.\n");
		return true;
	}
	if ( checkReg(reg, strobe_read, equal, true) )
	{
		DEVIF_DEBUG_RULES("Register is read strobe.\n");
		return true;
	}

	count = howManyFields(reg, is_volatile, equal, true);
	count += howManyFields(reg, read_action, unequal, none);
	count += howManyFields(reg, strobe_read, equal, true);

	if ( count != 0 )
	{
		DEVIF_DEBUG_RULES("Some fields are one of the following: volatile, readAction or read strobe.\n");
		return true;
	}

	return false;
}

bool getRegAllowedContent(Reg * reg)
{
	int count;

	count = howManyFields(reg, strobe_read, equal, true);
	if ( count > 1 )
	{
		DEVIF_DEBUG_RULES("More than one field of this register is read strobe.\n");
		return false;
	}

	return true;
}

bool getRegNeedModifyContent(Reg * reg)
{
	int count;

	count = howManyFields(reg, read_action, unequal, none);
	if ( count != 0 )
	{
		DEVIF_DEBUG_RULES("Some fields are readAction.\n");
		return true;
	}

	return false;
}


bool getFieldAccess(RegField * regField)
{
	if ( checkField(regField, access, equal, write_only) )
	{
		DEVIF_DEBUG_RULES("Field's access is write only.\n");
		return false;
	}
	if ( checkField(regField, access, equal, writeOnce) )
	{
		DEVIF_DEBUG_RULES("Field's access is write once.\n");
		return false;
	}

	return true;
}

bool getFieldNeedTransfer(RegField * regField)
{
	if ( checkReg(regField->parent_register, once_read, equal, false ) )
	{
		DEVIF_DEBUG_RULES("This field's parent register was never read, cached value has to be updated.\n");
		DEVIF_DEBUG_RULES("This is a transient rule. It only applies once. As the system enters the steady state, this rule is ignored until a reset comes.\n");
		return true;
	}
	if ( checkField(regField, is_volatile, equal, true) )
	{
		DEVIF_DEBUG_RULES("Field is volatile.\n");
		return true;
	}
	if ( checkField(regField, read_action, unequal, none) )
	{
		DEVIF_DEBUG_RULES("Field is readAction.\n");
		return true;
	}
	if ( checkField(regField, strobe_read, equal, true) )
	{
		DEVIF_DEBUG_RULES("Field is strobe read.\n");
		return true;
	}

	return false;
}

//last two probably not useful, since getReg has to be called
bool getFieldAllowedContent(RegField * regField)
{
	Reg * reg;
	reg = regField->parent_register;

	return getRegAllowedContent(reg);
}

bool getFieldNeedModifyContent(RegField * regField)
{
	Reg * reg;
	reg = regField->parent_register;

	return getRegNeedModifyContent(reg);
}
