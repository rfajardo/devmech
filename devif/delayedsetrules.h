/*
 * delayedsetrules.h
 *
 *  Created on: May 13, 2011
 *      Author: rfajardo
 */

#ifndef DELAYEDSETRULES_H_
#define DELAYEDSETRULES_H_

#include "regtypes.h"

bool delayedSetRegAccess(Reg * reg);
bool delayedSetRegNeedTransfer(Reg * reg);
bool delayedSetRegNeedReadBeforeWrite(Reg * reg);
bool delayedSetRegNeedAdaptContentBeforeWrite(Reg * reg);
bool delayedSetRegAllowedContent(Reg * reg);
bool delayedSetRegNeedModifyContent(Reg * reg);


bool delayedSetFieldAccess(RegField * regField);
bool delayedSetFieldNeedTransfer(RegField * regField);
bool delayedSetFieldAllowedContent(RegField * regField);


#endif /* DELAYEDSETRULES_H_ */
