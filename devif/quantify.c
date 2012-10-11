/*
 * quantify.c
 *
 *  Created on: May 13, 2011
 *      Author: rfajardo
 */

#include "quantify.h"
#include "regdata.h"

static bool int_check(unsigned int data, enum operation op, unsigned int value)
{
	switch (op)
	{
	case equal:
		if ( data == value )
			return true;
		else
			return false;
		break;
	case unequal:
		if ( data != value )
			return true;
		else
			return false;
		break;
	default:
		return false;
		break;
	}
}

bool checkField(RegField * regField, enum constraints cs, enum operation op, unsigned int value)
{
	unsigned int cs_data = 0;
	switch (cs)
	{
	//field or register constraints
	case access:
		cs_data = regField->field->access;
		break;
	case is_volatile:
		cs_data = regField->field->is_volatile;
		break;
	case strobe_read:
		cs_data = regField->field->vendorExtensions.strobe.read;
		break;
	case strobe_write:
		cs_data = regField->field->vendorExtensions.strobe.write;
		break;
	case write_noaction:
		cs_data = regField->field->vendorExtensions.strobe.noAction;
		break;
	//only field constraints
	case modified_write_value:
		cs_data = regField->field->modifiedWriteValue;
		break;

	case write_value_constraint:
		cs_data = regField->field->writeValueConstraint.valid;
		break;
	case write_as_read:
		cs_data = regField->field->writeValueConstraint.writeAsRead;
		break;
	case enumeration_constraint:
		cs_data = regField->field->writeValueConstraint.useEnumeratedValues;
		break;

	case read_action:
		cs_data = regField->field->readAction;
		break;
	case dirty:
		cs_data = regField->field->dirty;
		break;
	default:
		break;
	}
	return int_check(cs_data, op, value);
}

bool checkReg(Reg * reg, enum constraints cs, enum operation op, unsigned int value)
{
	unsigned int cs_data = 0;
	switch (cs)
	{
	//field or register constraints
	case access:
		cs_data = reg->access;
		break;
	case is_volatile:
		cs_data = reg->is_volatile;
		break;
	case strobe_read:
		cs_data = reg->vendorExtensions.strobe.read;
		break;
	case strobe_write:
		cs_data = reg->vendorExtensions.strobe.write;
		break;
	case write_noaction:
		cs_data = reg->vendorExtensions.strobe.noAction;
		break;
	case once_written:
		cs_data = reg->pcache->once_written;
		break;
	case once_read:
		cs_data = reg->pcache->once_read;
	default:
		break;
	}
	return int_check(cs_data, op, value);
}

int howManyFields(Reg * reg, enum constraints cs, enum operation op, unsigned int value)
{
	int quantifier = 0;
	List * fieldHook;

	for (fieldHook = reg->allFields; fieldHook != NULL; fieldHook = fieldHook->next)
	{
		if ( checkField(((FieldListData*)(fieldHook)->data)->regField, cs, op, value) )
			quantifier++;
	}

	return quantifier;
}

int howManyFieldsDouble(Reg * reg,
		enum constraints cs1, enum operation op1, unsigned int value1,
		enum constraints cs2, enum operation op2, unsigned int value2)
{
	int quantifier = 0;
	List * fieldHook;

	for (fieldHook = reg->allFields; fieldHook != NULL; fieldHook = fieldHook->next)
	{
		if ( checkField(((FieldListData*)(fieldHook)->data)->regField, cs1, op1, value1) )
			if ( checkField(((FieldListData*)(fieldHook)->data)->regField, cs2, op2, value2) )
				quantifier++;
	}
	return quantifier;
}

int howManyFieldsTripple(Reg * reg,
		enum constraints cs1, enum operation op1, unsigned int value1,
		enum constraints cs2, enum operation op2, unsigned int value2,
		enum constraints cs3, enum operation op3, unsigned int value3)
{
	int quantifier = 0;
	List * fieldHook;

	for (fieldHook = reg->allFields; fieldHook != NULL; fieldHook = fieldHook->next)
	{
		if ( checkField(((FieldListData*)(fieldHook)->data)->regField, cs1, op1, value1) )
			if ( checkField(((FieldListData*)(fieldHook)->data)->regField, cs2, op2, value2) )
				if ( checkField(((FieldListData*)(fieldHook)->data)->regField, cs3, op3, value3) )
					quantifier++;
	}
	return quantifier;
}

int howManySiblings(RegField * regField, enum constraints cs, enum operation op, unsigned int value)
{
	int quantifier = 0;
	List * fieldHook;

	for (fieldHook = regField->parent_register->allFields; fieldHook != NULL; fieldHook = fieldHook->next)
	{
		if ( ((FieldListData*)(fieldHook)->data)->regField->field != regField->field )
			if ( checkField(((FieldListData*)(fieldHook)->data)->regField, cs, op, value) )
				quantifier++;
	}
	return quantifier;
}

int howManySiblingsDouble(RegField * regField,
		enum constraints cs1, enum operation op1, unsigned int value1,
		enum constraints cs2, enum operation op2, unsigned int value2)
{
	int quantifier = 0;
	List * fieldHook;

	for (fieldHook = regField->parent_register->allFields; fieldHook != NULL; fieldHook = fieldHook->next)
	{
		if ( ((FieldListData*)(fieldHook)->data)->regField->field != regField->field )
			if ( checkField(((FieldListData*)(fieldHook)->data)->regField, cs1, op1, value1) )
				if ( checkField(((FieldListData*)(fieldHook)->data)->regField, cs2, op2, value2) )
					quantifier++;
	}
	return quantifier;
}

int howManyFieldsPass(Reg * reg, bool (*pass)(RegField *))
{
	int quantifier = 0;
	List * fieldHook;

	for (fieldHook = reg->allFields; fieldHook != NULL; fieldHook = fieldHook->next)
	{
		if ( (*pass)(((FieldListData*)(fieldHook)->data)->regField) )
			quantifier++;
	}
	return quantifier;
}

int howManySiblingsPass(RegField * regField, bool (*pass)(RegField *))
{
	int quantifier = 0;
	List * fieldHook;

	for (fieldHook = regField->parent_register->allFields; fieldHook != NULL; fieldHook = fieldHook->next)
	{
		if ( ((FieldListData*)(fieldHook)->data)->regField->field != regField->field )
			if ( (*pass)(((FieldListData*)(fieldHook)->data)->regField) )
				quantifier++;
	}
	return quantifier;
}


