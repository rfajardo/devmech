/*
 * com.c
 *
 *  Created on: May 9, 2011
 *      Author: rfajardo
 */

#include "comif.h"
#include "regdata.h"
#include "com.h"
#include "regop.h"

#include "devif.h"

#include "devifdebug.h"


unsigned int readReg(Reg * reg)
{
	DEVIF_DEBUG_CACHE("Reading cached register.\n");
	return reg->pcache->value;
}


void writeReg(Reg * reg, unsigned int value)
{
	unsigned int transfer_value;
	DEVIF_DEBUG_CACHE("Writing to register cache.\n");
	transfer_value = getFittedRegisterValue(reg, value);
	reg->pcache->value = transfer_value;
}

enum com_error sendReg(DevCom * devcom, Reg * reg, unsigned int value)
{
	enum com_error comerror = COM_OK;
	unsigned int transfer_value;

	transfer_value = getFittedRegisterValue(reg, value);

	/*set value through com port*/
	DEVIF_DEBUG_COM("Writing to device address: %x, ", reg->pcache->if_addr);
	comerror = devcom->writeIf(devcom->com, reg->pcache->if_addr, reg->if_complex_data, transfer_value);

	writeReg(reg, transfer_value);
	DEVIF_DEBUG_COM("value: %x .\n", readReg(reg));

	reg->pcache->once_written = true;
	return comerror;
}

unsigned int recvReg(DevCom * devcom, Reg * reg)
{
	unsigned int value;

	/*acquire value through com port*/
	DEVIF_DEBUG_COM("Reading from device address: %x, ", reg->pcache->if_addr);
	value = devcom->readIf(devcom->com, reg->pcache->if_addr, reg->if_complex_data);

	writeReg(reg, value);
	DEVIF_DEBUG_COM("value: %x .\n", readReg(reg));

	reg->pcache->once_read = true;
	return readReg(reg);
}



void writeWideReg(Reg * reg, unsigned int index, unsigned int value)
{
	unsigned int transfer_value;
	DEVIF_DEBUG_CACHE("Writing to wide register cache.\n");
	transfer_value = getFittedRegisterValue(reg, value);
	(reg->pcache+index)->value = transfer_value;
}

unsigned int readWideReg(Reg * reg, unsigned int index)
{
	DEVIF_DEBUG_CACHE("Reading cached wide register.\n");
	return (reg->pcache+index)->value;
}

enum com_error sendWideReg(DevCom * devcom, Reg * reg, unsigned int index, unsigned int value)
{
	enum com_error comerror = COM_OK;
	unsigned int transfer_value;

	transfer_value = getFittedRegisterValue(reg, value);

	/*set value through com port*/
	DEVIF_DEBUG_COM("Writing to device address: %x, ", (reg->pcache+index)->if_addr);
	comerror = devcom->writeIf(devcom->com, (reg->pcache+index)->if_addr, reg->if_complex_data, transfer_value);

	writeWideReg(reg, index, value);
	DEVIF_DEBUG_COM("value: %x .\n", readWideReg(reg, index));

	(reg->pcache+index)->once_written = true;
	return comerror;
}

unsigned int recvWideReg(DevCom * devcom, Reg * reg, unsigned int index)
{
	unsigned int value;

	/*acquire value through com port*/
	DEVIF_DEBUG_COM("Reading from device address: %x, ", (reg->pcache+index)->if_addr);
	value = devcom->readIf(devcom->com, (reg->pcache+index)->if_addr, reg->if_complex_data);

	writeWideReg(reg, index, value);
	DEVIF_DEBUG_COM("value: %x .\n", readWideReg(reg, index));

	(reg->pcache+index)->once_read = true;
	return readWideReg(reg, index);
}

