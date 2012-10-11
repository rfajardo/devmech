/*
 * setrules.c
 *
 *  Created on: May 13, 2011
 *      Author: rfajardo
 */

#include "setrules.h"
#include "regdata.h"
#include "quantify.h"
#include "regop.h"

#include "devifdebug.h"


bool violateWriteValueConstraint(RegField * regField, unsigned int value)
{
	List * enumHook;

	if ( !regField->field->writeValueConstraint.valid )
		return false;
	if ( regField->field->writeValueConstraint.writeAsRead )
		return false;
	if ( regField->field->writeValueConstraint.useEnumeratedValues )
	{
		for (enumHook = regField->field->enumeratedValues; enumHook != NULL; enumHook = enumHook->next)
		{
			if (value == ((EnumListData*)(enumHook)->data)->value)
				return false;
		}
	}
	else
	{
		if ( value <= regField->field->writeValueConstraint.maximum &&
				value >= regField->field->writeValueConstraint.minimum )
			return false;
	}
	return true;
}

bool currentViolateWriteValueConstraint(RegField * regField)
{
	unsigned int current_field_value;

	current_field_value = getCurrentFieldValue(regField);

	return violateWriteValueConstraint(regField, current_field_value);
}

bool strobeWriteAction(RegField * regField)
{
	if ( regField->field->vendorExtensions.strobe.write == false )
		return false;
	else if ( getCurrentFieldValue(regField) == regField->field->vendorExtensions.strobe.noAction )
		return false;

	return true;
}

bool strobeWriteActionNotVolatile(RegField * regField)
{
	if ( checkField(regField, is_volatile, equal, true) )
		return false;

	if ( !strobeWriteAction(regField) )
		return false;

	return true;
}









bool setRegAccess(Reg * reg)
{
	int count;

	if ( checkReg(reg, access, equal, read_only) )
	{
		DEVIF_DEBUG_RULES("Register's access is read only.\n");
		return false;
	}

	if ( checkReg(reg, once_written, equal, true) )
	{
		if ( checkReg(reg, access, equal, read_writeOnce) )
		{
			DEVIF_DEBUG_RULES("Register's access is write once and was once written.\n");
			return false;
		}
		if ( checkReg(reg, access, equal, writeOnce) )
		{
			DEVIF_DEBUG_RULES("Register's access is write once and was once written.\n");
			return false;
		}

		count = howManyFields(reg, access, equal, writeOnce);
		if ( count != 0 )
		{
			DEVIF_DEBUG_RULES("The access of some register's field is write once and this register was once written.\n");
			return false;
		}

		count = howManyFields(reg, access, equal, read_writeOnce);
		if ( count != 0 )
		{
			DEVIF_DEBUG_RULES("The access of some register's field is write once and this register was once written.\n");
			return false;
		}
	}

	count = howManyFields(reg, access, equal, read_only);
	if ( count != 0 )
	{
		DEVIF_DEBUG_RULES("The access of some register's field is read only.\n");
		return false;
	}

	return true;
}

bool setRegNeedTransfer(Reg * reg)
{
	DEVIF_DEBUG_RULES("setReg always needs transfers.\n");
	return true;
}

bool setRegNeedReadBeforeWrite(Reg * reg)
{
	int count;
	int read_action_nr;
	int strobe_read_nr;
	int volatile_nr;

	count = howManyFieldsDouble(reg,
			write_value_constraint, equal, true,
			write_as_read, equal, true);
	if ( count != 0 )
	{
		DEVIF_DEBUG_RULES("Some fields of this register require to be written as read.\n");
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

bool setRegAllowedContent(Reg * reg)
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

bool setRegNeedModifyContent(Reg * reg)
{
	int count;
	count = howManyFieldsDouble(reg,
			is_volatile, equal, false,
			modified_write_value, unequal, none);
	if ( count != 0 )
	{
		DEVIF_DEBUG_RULES("Some fields of this register have modified write values and are not volatile.\n");
		return true;
	}

	count = howManyFieldsPass(reg, &strobeWriteActionNotVolatile);
	if ( count != 0 )
	{
		DEVIF_DEBUG_RULES("Some fields of this register are strobe write and not volatile.\n");
		return true;
	}

	return false;
}






bool setFieldAccess(RegField * regField)
{
	Reg * reg;

	if ( checkField(regField, access, equal, read_only) )
	{
		DEVIF_DEBUG_RULES("Field's access is read only.\n");
		return false;
	}

	reg = regField->parent_register;
	if ( checkReg(reg, once_written, equal, true) )
	{
		if ( checkField(regField, access, equal, read_writeOnce) )
		{
			DEVIF_DEBUG_RULES("Field's access is write once and its register was once written.\n");
			return false;
		}
		if ( checkField(regField, access, equal, writeOnce) )
		{
			DEVIF_DEBUG_RULES("Field's access is write once and its register was once written.\n");
			return false;
		}
	}

	return true;
}

bool setFieldNeedTransfer(RegField * regField)
{
	DEVIF_DEBUG_RULES("setField always needs transfers.\n");
	return true;
}

bool setFieldNeedReadBeforeWrite(RegField * regField)
{
	int count;
	int read_action_nr;
	int strobe_read_nr;
	int volatile_nr;

	Reg * reg;

	count = howManySiblingsDouble(regField,
			write_value_constraint, equal, true,
			write_as_read, equal, true);
	if ( count != 0 )
	{
		DEVIF_DEBUG_RULES("Some sibling fields of this field require to be written as read.\n");
		return true;
	}
	else
	{
		reg = regField->parent_register;
		read_action_nr = howManyFields(reg, read_action, unequal, none);
		strobe_read_nr = howManyFields(reg, strobe_read, equal, true);
		volatile_nr = howManySiblings(regField, is_volatile, equal, true);
		if ( volatile_nr && !read_action_nr && !strobe_read_nr )		//safe to readb4write
		{
			DEVIF_DEBUG_RULES("Some sibling fields of this field are volatile. "
					"No sibling fields are strobe read or read action. Thus, getSet is safe and recommended.\n");
			return true;
		}
	}

	return false;
}

bool setFieldNeedAdaptContentBeforeWrite(RegField * regField)
{
	int count;

	count = howManySiblings(regField, modified_write_value, unequal, none);
	if ( count != 0 )
	{
		DEVIF_DEBUG_RULES("Some sibling fields of this field have modified write values.\n");
		return true;
	}

	count = howManySiblingsPass(regField, &strobeWriteAction);
	if ( count != 0 )
	{
		DEVIF_DEBUG_RULES("Some sibling fields of this field are strobe write and their values trigger actions.\n");
		return true;
	}

	return false;
}

bool setFieldAllowedContent(RegField * regField)
{
	int count;
	Reg * reg;

	reg = regField->parent_register;

	count = howManyFieldsPass(reg, &strobeWriteAction);
	if ( count > 1 )
	{
		DEVIF_DEBUG_RULES("More than one field of this field's parent register is strobe write and its value triggers its action.\n");
		return false;
	}

	count = howManyFieldsPass(reg, &currentViolateWriteValueConstraint);
	if ( count != 0 )
	{
		DEVIF_DEBUG_RULES("Some sibling fields of this field violate their write value constraints.\n");
		return false;
	}

	if ( regField->field->writeValueConstraint.writeAsRead == true )
	{
		DEVIF_DEBUG_RULES("This field is writeAsRead.\n");
		return false;
	}

	return true;
}

bool setFieldNeedModifyContent(RegField * regField)
{
	if ( checkField(regField, is_volatile, equal, false) &&
		checkField(regField, modified_write_value, unequal, none) )
	{
		DEVIF_DEBUG_RULES("This field has modified write values and is not volatile.\n");
		return true;
	}

	if ( strobeWriteActionNotVolatile(regField) )
	{
		DEVIF_DEBUG_RULES("This field is a strobe write field and is not volatile.\n");
		return true;
	}

	return false;
}

