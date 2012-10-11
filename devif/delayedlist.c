/*
 * delayediflist.c
 *
 *  Created on: May 30, 2011
 *      Author: rfajardo
 */

#include "delayedlist.h"
#include "regdata.h"

#include "regop.h"
#include "listutil.h"


static List * popField(List ** fieldList, RegField * regField)
{
	List * hitHook;
	for (; *fieldList != NULL; fieldList = &(*fieldList)->next)
	{
		if ( ((FieldListData*)(*fieldList)->data)->regField->field == regField->field)
		{
			hitHook = *fieldList;
			*fieldList = (*fieldList)->next;
			return hitHook;
		}
	}
	return NULL;
}


void setDirty(RegField * regField, unsigned int value)
{
	List * tmpHook;
	tmpHook = popField(&regField->parent_register->cleanFields, regField);
	if ( tmpHook == NULL )
		tmpHook = popField(&regField->parent_register->dirtyFields, regField);

	regField->field->dirty = true;
	((FieldListData*)(tmpHook)->data)->cachedValue = value;
	pushBack(&regField->parent_register->dirtyFields, tmpHook);
}

void clearDirty(RegField * regField)
{
	regField->field->dirty = false;
	pushBack(&regField->parent_register->cleanFields, popField(&regField->parent_register->dirtyFields, regField));
}

void clearDirtyFields(Reg * reg)
{
	List * tmpHook;
	tmpHook = popFront(&reg->dirtyFields);
	while ( tmpHook != NULL )
	{
		((FieldListData*)(tmpHook)->data)->regField->field->dirty = false;
		pushBack(&reg->cleanFields, tmpHook);
		tmpHook = popFront(&reg->dirtyFields);
	}
}


void updateDirtiesParentRegister(Reg * reg)
{
	List * fieldList;
	for (fieldList = reg->dirtyFields; fieldList != NULL; fieldList = fieldList->next)
		updateParentRegister(((FieldListData*)(fieldList)->data)->regField, ((FieldListData*)(fieldList)->data)->cachedValue);
}
