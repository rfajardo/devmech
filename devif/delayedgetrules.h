/*
 * delayedgetrules.h
 *
 *  Created on: May 18, 2011
 *      Author: rfajardo
 */

#ifndef DELAYEDGETRULES_H_
#define DELAYEDGETRULES_H_


#include "regtypes.h"

bool delayedGetRegAccess(Reg * reg);
bool delayedGetRegNeedTransfer(Reg * reg);
bool delayedGetRegAllowedContent(Reg * reg);
bool delayedGetRegNeedModifyContent(Reg * reg);


#endif /* DELAYEDGETRULES_H_ */
