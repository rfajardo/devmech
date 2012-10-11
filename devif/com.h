/*
 * com.h
 *
 *  Created on: May 20, 2011
 *      Author: rfajardo
 */

#ifndef COM_H_
#define COM_H_

#include "regtypes.h"

enum com_error
{
	COM_OK = 0,
	COM_FAILURE = 1,
	COM_NOT_INITIALIZED,
	COM_NO_DEVICE,
	COM_NO_DEVICE_SUPPORT,
	COM_TIMEOUT,
	COM_BUFFER_OVERFLOW,
	COM_OTHER
};

struct devcom
{
	void * com;
	void (*setComError)(enum com_error err);
	enum com_error (*getComError)(void);
	enum com_error (*readIf)(void * com, unsigned int addr, void * if_complex_data);
	enum com_error (*writeIf)(void * com, unsigned int addr, void * if_complex_data, unsigned int value);
};

struct dev
{
	DevCom * handle;		//on generated device structures, DevCom * handle has
							//to take the first position in the structure
};

DevCom * alloc_com(void);
void reset_com(DevCom * devcom);


#endif /* COM_H_ */
