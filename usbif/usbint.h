/*
 * usbint.h
 *
 *  Created on: Jul 5, 2011
 *      Author: raul
 */

#ifndef USBINT_H_
#define USBINT_H_

#include "usbdata.h"

#include <devif/types.h>
#include <devif/listutil.h>

#ifdef __KERNEL__
#include <linux/usb.h>
#else
#include <libusb.h>
#endif

struct endpoint;
struct process;
enum cb_ret;
struct workqueue_struct;

struct detached_type
{
	uint8_t		interface_nr;
};

struct endpoint_handle
{
	struct endpoint *				endpoint;
	struct process * 				process;

	enum cb_ret						(*callback)(void *, uint8_t *, size_t, size_t *, size_t);
	void * 							context;
};

struct driverfw_usb_device
{
	UsbDeviceDesc *					usb_device_desc;
#ifdef __KERNEL__
	struct usb_driver *				usb_context;
	struct usb_device * 			usb_handler;
#else
	struct libusb_context * 		usb_context;
	struct libusb_device_handle * 	usb_handler;
#endif
	List * 							detached_interfaces;

	uint8_t * 						ctrl0_buf;
	int 							ctrl0_timeout;

	List * 							endpoints_handles;
#ifdef __KERNEL__
	struct workqueue_struct	*		workqueue;
#else
	int * 							workqueue;
#endif
};

typedef struct endpoint_handle	EndpointHandle;
typedef struct driverfw_usb_device 	UsbDevice;


UsbDevice * alloc_usb_device(UsbDeviceDesc * udesc);
void free_usb_device(UsbDevice * udev);

void add_detached_interface(UsbDevice * udev, uint8_t interface_nr);
void add_endpoint(UsbDevice * udev, EndpointDesc * endpoint_desc);

void close_alternate_interface(UsbDevice * udev, AltIf * altif);

EndpointHandle * get_endpoint_handle(UsbDevice * udev, EndpointDesc * endpoint_desc);


#endif /* USBINT_H_ */
