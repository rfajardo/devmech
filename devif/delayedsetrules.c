/*
 * delayedsetrules.c
 *
 *  Created on: May 16, 2011
 *      Author: rfajardo
 */

#include "delayedsetrules.h"
#include "delayedlist.h"
#include "setrules.h"
#include "regdata.h"
#include "quantify.h"
#include "comif.h"

#include "devifdebug.h"


static bool strobeWriteActionNotDirty(RegField * regField)
{
	if ( checkField(regField, dirty, equal, true) )
		return false;

	if ( !strobeWriteAction(regField) )
		return false;

	return true;
}

static bool strobeWriteActionDirty(RegField * regField)
{
	if ( checkField(regField, dirty, equal, false) )
		return false;

	if ( !strobeWriteAction(regField) )
		return false;

	return true;
}

static bool strobeWriteActionNotVolatileDirty(RegField * regField)
{
	if ( checkField(regField, dirty, equal, false) )
		return false;

	if ( !strobeWriteActionNotVolatile(regField) )
		return false;

	return true;
}



bool delayedSetRegAccess(Reg * reg)
{
	return setRegAccess(reg);
}

bool delayedSetRegNeedTransfer(Reg * reg)
{
	int count;
	count = howManyFields(reg, dirty, equal, true);
	if ( count != 0 )
	{
		DEVIF_DEBUG_RULES("Some fields of this register are dirty.\n");
		return true;
	}

	return false;
}

bool delayedSetRegNeedReadBeforeWrite(Reg * reg)
{
	int count;
	int read_action_nr;
	int strobe_read_nr;
	int volatile_nr;

	count = howManyFieldsTripple(reg,
			dirty, equal, false,
			write_value_constraint, equal, true,
			write_as_read, equal, true);
	if ( count != 0 )
	{
		DEVIF_DEBUG_RULES("Some non-dirty fields of this register require to be written as read.\n");
		return true;
	}
	else
	{
		read_action_nr = howManyFields(reg, read_action, unequal, none);
		strobe_read_nr = howManyFields(reg, strobe_read, equal, true);
		volatile_nr = howManyFields(reg, is_volatile, equal, true);
		if ( volatile_nr && !read_action_nr && !strobe_read_nr )
		{
			DEVIF_DEBUG_RULES("Some fields of this register are volatile. "
					"No fields are strobe read or read action. Thus, getSet is safe and recommended.\n");
			return true;
		}
	}

	return false;
}

bool delayedSetRegNeedAdaptContentBeforeWrite(Reg * reg)
{
	int count;

	count = howManyFieldsDouble(reg,
			dirty, equal, false,
			modified_write_value, unequal, none);
	if ( count != 0 )
	{
		DEVIF_DEBUG_RULES("Some non-dirty fields of this register have modified write values.\n");
		return true;
	}

	count = howManyFieldsPass(reg, &strobeWriteActionNotDirty);
	if ( count != 0 )
	{
		DEVIF_DEBUG_RULES("Some non-dirty fields of this register are strobe write and their values trigger their actions.\n");
		return true;
	}

	return false;
}

bool delayedSetRegAllowedContent(Reg * reg)
{
	int count;

	count = howManyFieldsPass(reg, &strobeWriteAction);
	if ( count > 1 )
	{
		DEVIF_DEBUG_RULES("More than one field of this register is strobe write and its value triggers its action.\n");
		return false;
	}

	count = howManyFieldsPass(reg, &currentViolateWriteValueConstraint);
	if ( count != 0 )
	{
		DEVIF_DEBUG_RULES("Some fields of this register violate their write value constraint.\n");
		return false;
	}

	return true;
}

bool delayedSetRegNeedModifyContent(Reg * reg)
{
	int count;
	count = howManyFieldsTripple(reg,
			dirty, equal, true,
			is_volatile, equal, false,
			modified_write_value, unequal, none);
	if ( count != 0 )
	{
		DEVIF_DEBUG_RULES("Some dirty fields of this register have modified write values and are not volatile.\n");
		return true;
	}

	count = howManyFieldsPass(reg, &strobeWriteActionNotVolatileDirty);
	if ( count != 0 )
	{
		DEVIF_DEBUG_RULES("Some dirty fields of this register are strobe write, their values trigger actions and they are not volatile.\n");
		return true;
	}

	return false;
}



bool delayedSetFieldAccess(RegField * regField)
{
	return setFieldAccess(regField);
}

bool delayedSetFieldNeedTransfer(RegField * regField)
{
	DEVIF_DEBUG_RULES("delayedSetField never needs transfers.\n");
	return false;
}

bool delayedSetFieldAllowedContent(RegField * regField)
{
	int count;
	unsigned int previous_value;
	Reg * reg;

	reg = regField->parent_register;

	previous_value = readReg(reg);
	updateDirtiesParentRegister(reg);			//update register with values of dirty fields

	count = howManyFieldsPass(reg, &strobeWriteActionDirty);
	if ( count > 1 )
	{
		DEVIF_DEBUG_RULES("More than one of the dirty fields of the parent register is strobe write and its value triggers its action.\n");
		writeReg(reg, previous_value);
		return false;
	}

	if ( currentViolateWriteValueConstraint(regField) )
	{
		DEVIF_DEBUG_RULES("This field violate its write value constraint.\n");
		writeReg(reg, previous_value);
		return false;
	}

	if ( checkField(regField, write_as_read, equal, true) )
	{
		DEVIF_DEBUG_RULES("This field is writeAsRead.\n");
		writeReg(reg, previous_value);
		return false;
	}

	writeReg(reg, previous_value);
	return true;
}
