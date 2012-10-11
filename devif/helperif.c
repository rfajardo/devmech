/*
 * helperif.c
 *
 *  Created on: May 30, 2011
 *      Author: rfajardo
 */

#include "helperif.h"
#include "regdata.h"

#include "regop.h"
#include "comif.h"
#include "com.h"

#include "devif.h"

#include "devifdebug.h"


static enum transfer_error transfererror = TRANSFER_OK;


void enableRegFile(bool * regFile)
{
	*regFile = true;
	DEVIF_INFO("Enabling a register file.\n");
}
EXPORT_SYMBOL_GPL(enableRegFile);

void disableRegFile(bool * regFile)
{
	*regFile = false;
	DEVIF_INFO("Disabling a register file.\n");
}
EXPORT_SYMBOL_GPL(disableRegFile);


void enableAltGroup(bool * altGroup)
{
	*altGroup = true;
	DEVIF_INFO("Enabling an alternate group.\n");
}
EXPORT_SYMBOL_GPL(enableAltGroup);

void disableAltGroup(bool * altGroup)
{
	*altGroup = false;
	DEVIF_INFO("Disabling an alternate group.\n");
}
EXPORT_SYMBOL_GPL(disableAltGroup);


bool isActive(Reg * reg)
{
	List * activeHook;
	for (activeHook = reg->regFilesActive; activeHook != NULL; activeHook = activeHook->next)
	{
		if (!*((ActiveListData*)(activeHook->data))->active)
			return false;
	}

	for (activeHook = reg->altGroupsActive; activeHook != NULL; activeHook = activeHook->next)
	{
		if (*((ActiveListData*)(activeHook->data))->active)
			return true;
	}

	return false;
}


void setTransferError(enum transfer_error error)
{
	transfererror = error;
}

enum transfer_error getTransferError(void)
{
	enum transfer_error err = transfererror;
	setTransferError(TRANSFER_OK);
	return err;
}
EXPORT_SYMBOL_GPL(getTransferError);


enum transfer_error resetReg(Device * dev, Reg * reg)
{
	unsigned int value;
	enum com_error comerror;
	if ( !reg->reset.valid )
		return NORESET;

	value = getResetRegisterValue(reg);

	comerror = sendReg(dev->handle, reg, value);
	if ( comerror != COM_OK )
	{
		DEVIF_ERROR("Communication failed.\n");
		return COMMUNICATION_ERROR;
	}

	return TRANSFER_OK;
}
EXPORT_SYMBOL_GPL(resetReg);
