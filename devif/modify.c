/*
 * modify.c
 *
 *  Created on: May 9, 2011
 *      Author: rfajardo
 */

#include "modify.h"
#include "regdata.h"
#include "regop.h"

#include "devifdebug.h"

//
// DATA ADAPTATION RULES
//
void modifyValue(RegField * regField, enum modifiedWriteValue_type write_type, unsigned int previous_value)
{
	unsigned int value = getCurrentFieldValue(regField);
	switch (write_type)
	{
	case none:
		/* nothing to do */
		break;
	case clear:
		DEVIF_DEBUG_MODIFY("Clearing field value.\n");
		updateParentRegister(regField, 0);
		break;
	case set:
		DEVIF_DEBUG_MODIFY("Setting field value.\n");
		updateParentRegister(regField, -1);		/*set to 0xFFFF... independent of its width*/
		break;
	case modify:
		DEVIF_DEBUG_MODIFY("Cannot modify field value, data is unknown after written.\n");
		/*data is now unknown*/
		break;
	/*these require value to be up-to-date*/
	case oneToClear:
		DEVIF_DEBUG_MODIFY("Field modify: High-level bits clear the corresponding bit.\n");
		updateParentRegister(regField, ~value & previous_value);
		break;
	case oneToSet:
		DEVIF_DEBUG_MODIFY("Field modify: High-level bits set the corresponding bit.\n");
		updateParentRegister(regField, value | previous_value);
		break;
	case oneToToggle:
		DEVIF_DEBUG_MODIFY("Field modify: High-level bits toggle the corresponding bit.\n");
		updateParentRegister(regField, value ^ previous_value);
		break;
	case zeroToClear:
		DEVIF_DEBUG_MODIFY("Field modify: Low-level bits clear the corresponding bit.\n");
		updateParentRegister(regField, value & previous_value);
		break;
	case zeroToSet:
		DEVIF_DEBUG_MODIFY("Field modify: Low-level bits set the corresponding bit.\n");
		updateParentRegister(regField, ~value | previous_value);
		break;
	case zeroToToggle:
		DEVIF_DEBUG_MODIFY("Field modify: Low-level bits toggle the corresponding bit.\n");
		updateParentRegister(regField, ~value ^ previous_value);
		break;
	default:
		break;
	}
}

void modifyValueRegister(Reg * reg, int readWrite, unsigned int previous_value)
{
	unsigned int previous_value_child;
	List * fieldHook;

	for (fieldHook = reg->allFields; fieldHook != NULL; fieldHook = fieldHook->next)
	{
		previous_value_child = genFieldValue(((FieldListData*)(fieldHook)->data)->regField, previous_value);
		DEVIF_DEBUG_MODIFY("Modifying value of field.\n");
		if ( readWrite )
			modifyValue(((FieldListData*)(fieldHook)->data)->regField, ((FieldListData*)(fieldHook)->data)->regField->field->modifiedWriteValue, previous_value_child);
		else
			modifyValue(((FieldListData*)(fieldHook)->data)->regField, ((FieldListData*)(fieldHook)->data)->regField->field->readAction, previous_value_child);
	}
}
