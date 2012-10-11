/*
 * delayedlist.h
 *
 *  Created on: May 30, 2011
 *      Author: rfajardo
 */

#ifndef DELAYEDLIST_H_
#define DELAYEDLIST_H_

#include "regtypes.h"


void setDirty(RegField * regField, unsigned int value);
void clearDirty(RegField * regField);
void clearDirtyFields(Reg * reg);

void updateDirtiesParentRegister(Reg * reg);


#endif /* DELAYEDLIST_H_ */
