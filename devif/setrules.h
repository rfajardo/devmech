/*
 * setrules.h
 *
 *  Created on: May 13, 2011
 *      Author: rfajardo
 */

#ifndef SETRULES_H_
#define SETRULES_H_

#include "regtypes.h"


bool violateWriteValueConstraint(RegField * regField, unsigned int value);
bool currentViolateWriteValueConstraint(RegField * regField);
bool strobeWriteAction(RegField * regField);
bool strobeWriteActionNotVolatile(RegField * regField);


bool setRegAccess(Reg * reg);
bool setRegNeedTransfer(Reg * reg);
bool setRegNeedReadBeforeWrite(Reg * reg);
bool setRegAllowedContent(Reg * reg);
bool setRegNeedModifyContent(Reg * reg);

bool setFieldAccess(RegField * regField);
bool setFieldNeedTransfer(RegField * regField);
bool setFieldNeedReadBeforeWrite(RegField * regField);
bool setFieldNeedAdaptContentBeforeWrite(RegField * regField);
bool setFieldAllowedContent(RegField * regField);
bool setFieldNeedModifyContent(RegField * regField);


#endif /* SETRULES_H_ */
