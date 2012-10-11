/*
 * adapt.c
 *
 *  Created on: May 16, 2011
 *      Author: rfajardo
 */

#include "adapt.h"
#include "regdata.h"
#include "quantify.h"
#include "regop.h"
#include "comif.h"
#include "setrules.h"

#include "devifdebug.h"


static void adaptContent(RegField * regField)
{
	if ( !regField->field->writeValueConstraint.writeAsRead )   //if set writeAsRead as default don't update
	{
		DEVIF_DEBUG_ADAPT("Default adaptation chosen: no action value.");
		if ( strobeWriteAction(regField) )
		{
			DEVIF_DEBUG_ADAPT("Highest priority field adaptation: field is strobe write, adapting to no action.");
			updateParentRegister(regField, regField->field->vendorExtensions.strobe.noAction);
		}

		else if ( regField->field->modifiedWriteValue != none )
		{
			DEVIF_DEBUG_ADAPT("Second highest priority field adaptation: field has modified write value.\n");
			if ( regField->field->modifiedWriteValue >= oneToClear &&
					regField->field->modifiedWriteValue < zeroToClear )
			{
				DEVIF_DEBUG_ADAPT("Adaptation: ones alter register content on write, field is adapted to all zeroes.\n");
				updateParentRegister(regField, 0);
			}

			else if ( regField->field->modifiedWriteValue >= zeroToClear &&
					regField->field->modifiedWriteValue <= zeroToToggle )
			{
				DEVIF_DEBUG_ADAPT("Adaptation: zeroes alter register content on write, field is adapted to all ones.\n");
				updateParentRegister(regField, -1);       //set all bits to 1
			}
		}
		/*
		else if ( regField->parent_register->reset.valid )
		{
			updateFieldReset(regField);
		}*/
	}
	else
		DEVIF_DEBUG_ADAPT("Field modified value set to write as read, skipping default adaptation for field.\n");
}

static void adaptWriteAsRead(RegField * regField, unsigned int reg_read_value)
{
	unsigned int fieldValue;

	if ( checkField(regField, write_value_constraint, equal, true) &&
			checkField(regField, write_as_read, equal, true) )
	{
		DEVIF_DEBUG_ADAPT("SWITCH: from default adaptation to write as read for a field.\n");
		fieldValue = genFieldValue(regField, reg_read_value);
		updateParentRegister(regField, fieldValue);
	}
}

void adaptContentSiblings(RegField * regField)
{
	//set as reset or no-action values
	List * fieldHook;

	for (fieldHook = regField->parent_register->allFields; fieldHook != NULL; fieldHook = fieldHook->next)
	{
		if ( regField->field != ((FieldListData*)(fieldHook->data))->regField->field )
		{
			DEVIF_DEBUG_ADAPT("Running default adaptation for field.\n");
			adaptContent(((FieldListData*)(fieldHook->data))->regField);
		}
	}
}

void adaptWriteAsReadFields(Reg * reg, unsigned int read_value)
{
	//set as reset or no-action values
	List * fieldHook;

	DEVIF_DEBUG_ADAPT("Checking for adaptation switch from default to write as read for register fields.\n");
	//updates write as read fields with new read data
	for (fieldHook = reg->allFields; fieldHook != NULL; fieldHook = fieldHook->next)
	{
		adaptWriteAsRead(((FieldListData*)(fieldHook->data))->regField, read_value);
	}
}

void adaptWriteAsReadSiblings(RegField * regField, unsigned int read_value)
{
	//set as reset or no-action values
	List * fieldHook;

	DEVIF_DEBUG_ADAPT("Checking for adaptation switch from default to write as read for register fields.\n");
	for (fieldHook = regField->parent_register->allFields; fieldHook != NULL; fieldHook = fieldHook->next)
	{
		if ( regField->field != ((FieldListData*)(fieldHook->data))->regField->field )
		{
			adaptWriteAsRead(((FieldListData*)(fieldHook->data))->regField, read_value);
		}
	}
}

void adaptContentFieldList(List * list)
{
	DEVIF_DEBUG_ADAPT("Running default adaptation for field list.\n");
	for (;list != NULL; list = list->next)
		adaptContent(((FieldListData*)(list->data))->regField);
}

void adaptWriteAsReadFieldList(List * list, unsigned int read_value)
{
	DEVIF_DEBUG_ADAPT("Checking for adaptation switch from default to write as read for field list.\n");
	for (;list != NULL; list = list->next)
		adaptWriteAsRead(((FieldListData*)(list->data))->regField, read_value);
}
