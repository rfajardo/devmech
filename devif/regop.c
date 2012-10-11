/*
 * regop.c
 *
 *  Created on: May 9, 2011
 *      Author: rfajardo
 */

#include "regop.h"
#include "regdata.h"
#include "comif.h"

#include "devifdebug.h"


static unsigned int getRegMask(Reg * reg)
{
	int i;
	unsigned int size;
	unsigned int mask = 0;
	size = reg->size - 1;
	for (i = size; i >= 0; i--)		//update parser in the future to compute mask for every register and store
	{
		mask += 0x00000001 << i;
	}
	return mask;
}

static unsigned int getBitMask(Field * pfield)
{
	int i;
	unsigned int width;
	unsigned int mask = 0;
	width = pfield->bitWidth - 1;
	for (i = width; i >= 0; i--)		//update parser in the future to compute mask for every register and store
	{
		mask += 0x00000001 << i;
	}
	return mask;
}


unsigned int getFittedRegisterValue(Reg * reg, unsigned int regValue)
{
	unsigned int fittedValue = 0;
	unsigned int mask = getRegMask(reg);
	fittedValue = mask & regValue;
	DEVIF_DEBUG_REGOP("Fitted register value %x out of register input value %x and register size (width) %x.\n", fittedValue, regValue, reg->size);
	return fittedValue;
}


unsigned int genRegisterValue(RegField * regField, unsigned int fieldValue)
{
	unsigned int mask = getBitMask(regField->field);
	unsigned int regValue = readReg(regField->parent_register);
	//when using value =| mask & new_value to write to a value, old value (value)
	//has to be zeroed where the mask bits are 1 otherwise zero bits cannot be written
	regValue &= ~(mask << regField->field->bitOffset);
	regValue |= ( mask & fieldValue ) << regField->field->bitOffset;
	DEVIF_DEBUG_REGOP("Register value %x for combining field input value %x back to correspondent register position: %x.\n", regValue, fieldValue, regField->field->bitOffset);
	return regValue;
}

unsigned int genFieldValue(RegField * regField, unsigned int regValue)
{
	unsigned int mask = getBitMask(regField->field);
	unsigned int fieldValue = mask & ( regValue >> regField->field->bitOffset );
	DEVIF_DEBUG_REGOP("Field value %x for combining register input value %x with correspondent register position %x.\n", fieldValue, regValue, regField->field->bitOffset);
	return fieldValue;
}


unsigned int getCurrentFieldValue(RegField * regField)
{
	unsigned int fieldValue;
	DEVIF_DEBUG_CACHE("Generating field value out of cached register.\n");
	fieldValue = genFieldValue(regField, readReg(regField->parent_register));
	return fieldValue;
}

void updateParentRegister(RegField * regField, unsigned int fieldValue)
{
	unsigned int regValue = genRegisterValue(regField, fieldValue);
	DEVIF_DEBUG_CACHE("Updating parent register cached value for the input value of field.\n");
	writeReg(regField->parent_register, regValue);
}


unsigned int getResetRegisterValue(Reg * reg)
{
	unsigned int value;
	value = readReg(reg);
	value &= ~reg->reset.mask;
	value |= reg->reset.value & reg->reset.mask;
	DEVIF_DEBUG_REGOP("Register reset %x. Combination of register reset value %x with the cached register value %x through "
			"reset mask %x.\n", value, reg->reset.value, readReg(reg), reg->reset.mask);
	return value;
}

unsigned int genResetFieldValue(RegField * regField)
{
	unsigned int fieldValue, value, bitmask, multimask;
	bitmask = getBitMask(regField->field);
	multimask = bitmask & ( regField->parent_register->reset.mask >> regField->field->bitOffset );
	if ( multimask )
	{
		fieldValue = value = getCurrentFieldValue(regField);
		//when using value =| mask & new_value to write to a value, old value (value)
		//has to be zeroed where the mask bits are 1 otherwise zero bits cannot be written
		value &= ~multimask;
		value |= multimask & ( regField->parent_register->reset.value >> regField->field->bitOffset );
		DEVIF_DEBUG_REGOP("Acquiring register reset value %x for correspondent field. Combining reset value %x, mask %x "
				" , field offset %x and cached field value %x.\n", value, regField->parent_register->reset.value,
				regField->parent_register->reset.mask, regField->field->bitOffset, fieldValue);
		return value;
	}
	return 0;
}
