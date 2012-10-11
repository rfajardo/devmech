/*
 * com.h
 *
 *  Created on: Mar 26, 2012
 *      Author: raul
 */

#ifndef USBCOM_H_
#define USBCOM_H_

#include "usbint.h"

#ifdef USBIF
struct usbcom
{
	UsbDevice * handle;
};
typedef struct usbcom UsbCom;
#else
typedef void UsbCom;
#endif

#endif /* USBCOM_H_ */
