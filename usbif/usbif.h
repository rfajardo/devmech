/*
 * usbif.h
 *
 *  Created on: Jun 29, 2011
 *      Author: raul
 */

#ifndef USBIF_H_
#define USBIF_H_

#include "usbdata.h"
#include "usbint.h"
#include "com.h"

#include <devif/regdata.h>

enum usb_error
{
	USB_SUCCESS 			= 0,
	USB_ERROR_IO 			= 1,
	USB_ERROR_INVALID_PARAM = 2,
	USB_ERROR_ACCESS 		= 3,
	USB_ERROR_NO_DEVICE 	= 4,
	USB_ERROR_NOT_FOUND 	= 5,
	USB_ERROR_BUSY 			= 6,
	USB_ERROR_TIMEOUT 		= 7,
	USB_ERROR_OVERFLOW 		= 8,
	USB_ERROR_PIPE 			= 9,
	USB_ERROR_INTERRUPTED 	= 10,
	USB_ERROR_NO_MEM 		= 11,
	USB_ERROR_NOT_SUPPORTED = 12,

	USB_TRANSFER_COMPLETED	= 14,
	USB_TRANSFER_FATAL		= 15,
	USB_TRANSFER_CANCELED	= 16,

	USB_ERROR_OTHER 		= 99
};

enum usb_status
{
	USB_ACTIVE,
	USB_UNITIALIZED,
	USB_CLOSED
};

#ifdef __KERNEL__
enum usb_error usb_init(UsbCom * ucom, struct usb_driver * drv);
void usb_exit(UsbCom * ucom);

enum usb_error usb_open(UsbCom * ucom, struct usb_device * dev);
void usb_close(UsbCom * ucom);
#else
enum usb_error usb_init(UsbCom * ucom);
void usb_exit(UsbCom * ucom);

enum usb_error usb_open(UsbCom * ucom);
void usb_close(UsbCom * ucom);
#endif

void usbif_bind_com(Device * dev, UsbCom * ucom);

enum usb_error usb_set_configuration(UsbCom * ucom, Configuration * conf);
int usb_get_configuration(UsbCom * ucom);

enum usb_error usb_if_claim(UsbCom * ucom, IfConf * interface_configuration);
enum usb_error usb_release_interface(UsbCom * ucom, IfConf * interface_configuration);

AltIf * usb_get_alt_if(UsbCom * ucom, IfConf * interface, uint8_t alternate_interface_nr);
enum usb_error usb_set_alt_if(UsbCom * ucom, AltIf * alternate_interface);

enum usb_status usb_dev_status(UsbDevice * udev);

enum usb_error transfer_setup(UsbCom * ucom, SetupPacket * setup, uint8_t * buf, size_t len);
void set_ctrl0_timeout(UsbCom * ucom, uint32_t timeout);


#endif /* USBIF_H_ */
