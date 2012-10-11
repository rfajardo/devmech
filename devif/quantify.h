/*
 * quantify.h
 *
 *  Created on: May 13, 2011
 *      Author: rfajardo
 */

#ifndef QUANTIFY_H_
#define QUANTIFY_H_

#include "regtypes.h"

enum operation
{
	equal,
	unequal
};

enum constraints
{
	//field or register constraints
	access,
	is_volatile,
	strobe_read,
	strobe_write,
	write_noaction,
	//only field constraints
	modified_write_value,

	write_value_constraint,
	write_as_read,
	enumeration_constraint,

	read_action,
	dirty,
	once_written,
	once_read
};


bool checkField(RegField * regField, enum constraints cs, enum operation op, unsigned int value);
bool checkReg(Reg * reg, enum constraints cs, enum operation op, unsigned int value);

int howManyFields(Reg * reg, enum constraints cs, enum operation op, unsigned int value);
int howManyFieldsDouble(Reg * reg,
		enum constraints cs1, enum operation op1, unsigned int value1,
		enum constraints cs2, enum operation op2, unsigned int value2);
int howManyFieldsTripple(Reg * reg,
		enum constraints cs1, enum operation op1, unsigned int value1,
		enum constraints cs2, enum operation op2, unsigned int value2,
		enum constraints cs3, enum operation op3, unsigned int value3);

int howManySiblings(RegField * regField, enum constraints cs, enum operation op, unsigned int value);
int howManySiblingsDouble(RegField * regField,
		enum constraints cs1, enum operation op1, unsigned int value1,
		enum constraints cs2, enum operation op2, unsigned int value2);

int howManyFieldsPass(Reg * reg, bool (*pass)(RegField *));
int howManySiblingsPass(RegField * regField, bool (*pass)(RegField *));

#endif /* QUANTIFY_H_ */
