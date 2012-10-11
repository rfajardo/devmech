/*
 * regutil.c
 *
 *  Created on: Jun 27, 2011
 *      Author: raul
 */

#include "regdata.h"
#include "listutil.h"

static void free_field(Field * field)
{
	removeList(&field->enumeratedValues);
	FREE(field);
}


Reg * alloc_register(void)
{
	Reg * my_reg;
	my_reg = ALLOC(sizeof(Reg));

	my_reg->if_complex_data = NULL;

	my_reg->regFilesActive = NULL;
	my_reg->altGroupsActive = NULL;

	my_reg->dim = 1;
	my_reg->addressOffset = 0;
	my_reg->size = 8;

	my_reg->is_volatile = false;
	my_reg->access = read_write;

	my_reg->reset.valid = false;
	my_reg->reset.mask = 0;
	my_reg->reset.value = 0;

	my_reg->allFields = NULL;
	my_reg->dirtyFields = NULL;
	my_reg->cleanFields = NULL;

	my_reg->vendorExtensions.strobe.noAction = 0;
	my_reg->vendorExtensions.strobe.read = false;
	my_reg->vendorExtensions.strobe.write = false;

	my_reg->pcache = NULL;

	return my_reg;
}
EXPORT_SYMBOL_GPL(alloc_register);

void free_register(Reg * reg)
{
	List * fieldHook;

    if ( reg->if_complex_data != NULL )
    	FREE(reg->if_complex_data);

    removeList(&reg->regFilesActive);
    removeList(&reg->altGroupsActive);

    //allFields list shares data with clean & dirtyFields list
    //we have to free the data pointed by the data hook (void *)
    //before removing the other lists
	while (reg->allFields != NULL)
	{
		fieldHook = popFront(&reg->allFields);

		free_field(((FieldListData*)(fieldHook->data))->regField->field);
		FREE(((FieldListData*)(fieldHook->data))->regField);

		//hook's data is freed while removing the other lists, must not be done here (no two frees)
		//FREE(fieldHook->data);

		FREE(fieldHook);
	}

    removeList(&reg->dirtyFields);
    removeList(&reg->cleanFields);

    FREE(reg);
}
EXPORT_SYMBOL_GPL(free_register);


Field * alloc_field()
{
	Field * my_field;

	my_field = ALLOC(sizeof(Field));

	my_field->dirty = false;

	my_field->bitOffset = 0;
	my_field->bitWidth = 1;
	my_field->is_volatile = false;
	my_field->access = read_write;
	my_field->testable = true;

	my_field->modifiedWriteValue = none;
	my_field->readAction = none;

	my_field->vendorExtensions.strobe.noAction = 0;
	my_field->vendorExtensions.strobe.read = false;
	my_field->vendorExtensions.strobe.write = false;

	my_field->writeValueConstraint.valid = false;
	my_field->writeValueConstraint.writeAsRead = false;
	my_field->writeValueConstraint.useEnumeratedValues = false;
	my_field->writeValueConstraint.minimum = 0;
	my_field->writeValueConstraint.maximum = 0;

	my_field->enumeratedValues = NULL;

	return my_field;
}
EXPORT_SYMBOL_GPL(alloc_field);

void add_field(Reg * reg, Field * field)
{
	List * fieldHook;
	RegField * regField;

	FieldListData * fieldListData;

	if ( reg != NULL )
	{
		//only one data element
		regField = ALLOC(sizeof(RegField));
		regField->parent_register = reg;
		regField->field = field;

		fieldListData = ALLOC(sizeof(FieldListData));
		fieldListData->regField = regField;

		//two list hooks for two lists
		fieldHook = ALLOC(sizeof(List));
		fieldHook->data = fieldListData;
		pushBack(&reg->allFields, fieldHook);

		fieldHook = ALLOC(sizeof(List));
		fieldHook->data = fieldListData;
		pushBack(&reg->cleanFields, fieldHook);
	}
}
EXPORT_SYMBOL_GPL(add_field);


void add_enumvalue(Field * field, unsigned int enumvalue)
{
	List * enumHook;
	if (field != NULL)
	{
		enumHook = ALLOC(sizeof(List));
		enumHook->data = ALLOC(sizeof(EnumListData));
		((EnumListData*)enumHook->data)->value = enumvalue;
		pushBack(&field->enumeratedValues, enumHook);
	}
}
EXPORT_SYMBOL_GPL(add_enumvalue);


RegField * alloc_regfield()
{
	RegField * regField;
	regField = ALLOC(sizeof(RegField));
	return regField;
}
EXPORT_SYMBOL_GPL(alloc_regfield);

void associate_regfield(RegField * regField, Reg * reg, Field * field)
{
	regField->parent_register = reg;
	regField->field = field;
}
EXPORT_SYMBOL_GPL(associate_regfield);

void free_regfield(RegField * regField)
{
	FREE(regField);
}
EXPORT_SYMBOL_GPL(free_regfield);


void add_register_file_active(Reg * reg, bool * register_file_active)
{
	List * activeHook;
	if (reg != NULL)
	{
		activeHook = ALLOC(sizeof(List));
		activeHook->data = ALLOC(sizeof(ActiveListData));
		((ActiveListData*)activeHook->data)->active = register_file_active;
		pushBack(&reg->regFilesActive, activeHook);
	}
}
EXPORT_SYMBOL_GPL(add_register_file_active);

void add_alternate_group_active(Reg * reg, bool * alternate_group_active)
{
	List * activeHook;
	if (reg != NULL)
	{
		activeHook = ALLOC(sizeof(List));
		activeHook->data = ALLOC(sizeof(ActiveListData));
		((ActiveListData*)activeHook->data)->active = alternate_group_active;
		pushBack(&reg->altGroupsActive, activeHook);
	}
}
EXPORT_SYMBOL_GPL(add_alternate_group_active);


Mem * alloc_memory(unsigned int size)
{
	unsigned int i;
	Mem * pmem;
	pmem = ALLOC(sizeof(Mem)*size);

	for (i = 0; i < size; i++)
	{
	    (pmem + i)->once_written = false;
	    (pmem + i)->once_read = false;
	    (pmem + i)->value = 0;
	    (pmem + i)->if_addr = 0;
	}

	return pmem;
}
EXPORT_SYMBOL_GPL(alloc_memory);

void associate_memory(Reg * reg, Mem * pmem)
{
	reg->pcache = pmem;
}
EXPORT_SYMBOL_GPL(associate_memory);

void free_memory(Mem * pmem)
{
	FREE(pmem);
}
EXPORT_SYMBOL_GPL(free_memory);
