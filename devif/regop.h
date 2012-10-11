/*
 * regop.h
 *
 *  Created on: May 16, 2011
 *      Author: rfajardo
 */

#ifndef REGOP_H_
#define REGOP_H_

#include "regtypes.h"

unsigned int getFittedRegisterValue(Reg * reg, unsigned int regValue);

unsigned int genRegisterValue(RegField * regField, unsigned int fieldValue);
unsigned int genFieldValue(RegField * regField, unsigned int regValue);

unsigned int getCurrentFieldValue(RegField * regField);
void updateParentRegister(RegField * regField, unsigned int fieldValue);

unsigned int getResetRegisterValue(Reg * reg);
unsigned int genResetFieldValue(RegField * regField);


#endif /* REGOP_H_ */
