/*
 * getrules.h
 *
 *  Created on: May 13, 2011
 *      Author: rfajardo
 */

#ifndef GETRULES_H_
#define GETRULES_H_

#include "regtypes.h"

bool getRegAccess(Reg * reg);
bool getRegNeedTransfer(Reg * reg);
bool getRegAllowedContent(Reg * reg);
bool getRegNeedModifyContent(Reg * reg);


bool getFieldAccess(RegField * regField);
bool getFieldNeedTransfer(RegField * regField);
//last two probably not useful, since getReg has to be called
bool getFieldAllowedContent(RegField * regField);
bool getFieldNeedModifyContent(RegField * regField);


#endif /* GETRULES_H_ */
