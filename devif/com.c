/*
 * com.c
 *
 *  Created on: Nov 10, 2011
 *      Author: raul
 */

#include "com.h"

#include "devifdebug.h"


static void defaultSetComError(enum com_error err)
{
	DEVIF_ERROR("Device uninitialized, defaultSetComError running, not doing anything.\n");
}

static enum com_error defaultGetComError(void)
{
	DEVIF_ERROR("Device uninitialized, defaultGetComError running, not doing anything.\n");
	return COM_NOT_INITIALIZED;
}

static enum com_error defaultReadIf(void * com, unsigned int addr, void * if_complex_data)
{
	DEVIF_ERROR("Device uninitialized, defaultReadIf running, not doing anything.\n");
	return COM_NOT_INITIALIZED;
}

static enum com_error defaultWriteIf(void * com, unsigned int addr, void * if_complex_data, unsigned int value)
{
	DEVIF_ERROR("Device uninitialized, defaultWriteIf running, not doing anything.\n");
	return COM_NOT_INITIALIZED;
}

DevCom * alloc_com(void)
{
	DevCom * tmpDevCom;
	tmpDevCom = ALLOC(sizeof(DevCom));
	reset_com(tmpDevCom);
	return tmpDevCom;
}
EXPORT_SYMBOL_GPL(alloc_com);

void reset_com(DevCom * devcom)
{
	devcom->com = NULL;
	devcom->getComError = &defaultGetComError;
	devcom->readIf = &defaultReadIf;
	devcom->writeIf = &defaultWriteIf;
	devcom->setComError = &defaultSetComError;
}
EXPORT_SYMBOL_GPL(reset_com);
