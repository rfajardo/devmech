/*
 * modify.h
 *
 *  Created on: May 16, 2011
 *      Author: rfajardo
 */

#ifndef MODIFY_H_
#define MODIFY_H_

#include "regtypes.h"

#define READ 0
#define WRITE 1

void modifyValue(RegField * regField,
		enum modifiedWriteValue_type write_type, unsigned int previous_value);

void modifyValueRegister(Reg * reg, int readWrite, unsigned int previous_value);

#endif /* MODIFY_H_ */
