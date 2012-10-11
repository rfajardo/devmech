/*
 * comif.h
 *
 *  Created on: May 16, 2011
 *      Author: rfajardo
 */

#ifndef COMIF_H_
#define COMIF_H_

#include "regtypes.h"
#include "com.h"

enum com_error;

unsigned int readReg(Reg * reg);
void writeReg(Reg * reg, unsigned int value);

enum com_error sendReg(DevCom * devcom, Reg * reg, unsigned int value);
unsigned int recvReg(DevCom * devcom, Reg * reg);

//wide interface
void writeWideReg(Reg * reg, unsigned int index, unsigned int value);
unsigned int readWideReg(Reg * reg, unsigned int index);

enum com_error sendWideReg(DevCom * devcom, Reg * reg, unsigned int index, unsigned int value);
unsigned int recvWideReg(DevCom * devcom, Reg * reg, unsigned int index);


#endif /* COMIF_H_ */
