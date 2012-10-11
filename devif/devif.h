/*
 * devif.h
 *
 *  Created on: May 16, 2011
 *      Author: rfajardo
 */

#ifndef DEVIF_H_
#define DEVIF_H_


#include "regtypes.h"

//RETURNERROR is a return value which indicates a possible error
//this is used when a the return value is otherwise meaningful for the application

enum transfer_error
{
	TRANSFER_OK = 0,
	COMMUNICATION_ERROR,
	NOTACTIVE,
	ACCESS_DENIED,
	CONTENT_UNALLOWED,
	NO_TRANSFER,
	TRANSFER,
	NEEDGETSET,
	NORESET,
	NOTWIDE,
	OUTBOUNDS
};


//register Files and alternate Groups switches
void enableRegFile(bool * regFile);
void disableRegFile(bool * regFile);

void enableAltGroup(bool * altGroup);
void disableAltGroup(bool * altGroup);


enum transfer_error resetReg(Device * dev, Reg * reg);

//	Standard interface
unsigned int getReg(Device * dev, Reg * reg);
unsigned int getField(Device * dev, RegField * regField);
enum transfer_error getTransferError(void);

enum transfer_error setReg(Device * dev, Reg * reg, unsigned int value);
enum transfer_error setField(Device * dev, RegField * regField, unsigned int value);

//all values required to be read are substituted
enum transfer_error getSetReg(Device * dev, Reg * reg, unsigned int value);
enum transfer_error getSetField(Device * dev, RegField * regField, unsigned int value);

//wide registers should not have fields (guarantee in parser?). Therefore, no getSet necessary.
//these are mostly used for buffers or wide counters
unsigned int getWideReg(Device * dev, Reg * reg, unsigned int index);
enum transfer_error setWideReg(Device * dev, Reg * reg, unsigned int index, unsigned int value);


//	Delayed set interface for writing multiple fields in one transaction
enum transfer_error delayedSetReg(Device * dev, Reg * reg);
enum transfer_error delayedSetField(RegField * regField, unsigned int value);

enum transfer_error delayedSetFieldResetValue(RegField * regField);
void cancelDelayedTransfers(Reg * reg);

enum transfer_error delayedGetSetReg(Device * dev, Reg * reg);


//	User controlled get interface (force)
unsigned int cachedGetReg(Reg * reg);
unsigned int cachedGetField(RegField * regField);

unsigned int forceGetReg(Device * dev, Reg * reg);
unsigned int forceGetField(Device * dev, RegField * regField);

unsigned int cachedGetWideReg(Reg * reg, unsigned int index);
unsigned int forceGetWideReg(Device * dev, Reg * reg, unsigned int index);


#endif /* DEVIF_H_ */
