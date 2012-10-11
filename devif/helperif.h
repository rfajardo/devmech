/*
 * helperif.h
 *
 *  Created on: May 30, 2011
 *      Author: rfajardo
 */

#ifndef HELPERIF_H_
#define HELPERIF_H_

#include "regtypes.h"

enum transfer_error;

bool isActive(Reg * reg);
void setTransferError(enum transfer_error error);


#endif /* HELPERIF_H_ */
