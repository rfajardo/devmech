/*
 * usbif.c
 *
 *  Created on: Jun 29, 2011
 *      Author: raul
 */

#include "usbif.h"
#include "usbint.h"
#include "usbdata.h"
#include "streamif.h"
#include "usbifdebug.h"
#include "comif.h"
#include <devif/com.h>
#include "com.h"


#ifdef __KERNEL__
#include <linux/usb.h>
#else
#include <libusb.h>
#endif

#ifdef __KERNEL__
enum usb_error usb_init(UsbCom * ucom, struct usb_driver * drv)
{
	USBIF_DEBUG_CONF("Registering interface with the USB subsystem.\n");
	ucom->handle->usb_context = drv;
	return USB_SUCCESS;
}
EXPORT_SYMBOL_GPL(usb_init);

void usb_exit(UsbCom * ucom)
{
	USBIF_DEBUG_CONF("Detaching interface from the USB subsystem.\n");
	ucom->handle->usb_context = NULL;
}
EXPORT_SYMBOL_GPL(usb_exit);

enum usb_error usb_open(UsbCom * ucom, struct usb_device * dev)
{
	USBIF_DEBUG_CONF("Establishing connection with device.\n");
	ucom->handle->usb_handler = dev;
	usb_get_configuration(ucom);
	return USB_SUCCESS;
}
EXPORT_SYMBOL_GPL(usb_open);

void usb_close(UsbCom * ucom)
{
	USBIF_DEBUG_CONF("Disabling device connection.\n");
	ucom->handle->usb_handler = NULL;
}
EXPORT_SYMBOL_GPL(usb_close);
#else
enum usb_error usb_init(UsbCom * ucom)
{
	enum usb_error uErr;

	USBIF_DEBUG_CONF("Registering interface with the USB subsystem.\n");
	if ( ( uErr = libusb_init(&ucom->handle->usb_context) ) )
	{
		USBIF_ERROR("Couldn't register with USB subsystem.\n");
		return uErr;
	}

	USBIF_DEBUG_CONF("Setting high level debug information from underlying library.\n");
	libusb_set_debug(ucom->handle->usb_context, 3);
	return uErr;
}

void usb_exit(UsbCom * ucom)
{
	USBIF_DEBUG_CONF("Detaching interface from the USB subsystem.\n");
	libusb_exit(ucom->handle->usb_context);
	ucom->handle->usb_context = NULL;
}

enum usb_error usb_open(UsbCom * ucom)
{
	USBIF_DEBUG_CONF("Establishing connection with device.\n");
	ucom->handle->usb_handler = libusb_open_device_with_vid_pid(ucom->handle->usb_context, ucom->handle->usb_device_desc->idVendor, ucom->handle->usb_device_desc->idProduct);
	if ( ucom->handle->usb_handler == NULL )
		return USB_ERROR_NO_DEVICE;
	usb_get_configuration(ucom);
	return USB_SUCCESS;
}

void usb_close(UsbCom * ucom)
{
	USBIF_DEBUG_CONF("Disabling device connection.\n");
	libusb_close(ucom->handle->usb_handler);
	ucom->handle->usb_handler = NULL;
}
#endif


void usbif_bind_com(Device * dev, UsbCom * ucom)
{
	dev->handle->com = ucom;
	dev->handle->getComError = &getComError;
	dev->handle->readIf = &readIf;
	dev->handle->setComError = &setComError;
	dev->handle->writeIf = &writeIf;
}
EXPORT_SYMBOL_GPL(usbif_bind_com);


enum usb_error usb_set_configuration(UsbCom * ucom, Configuration * conf)
{
	enum usb_error uErr;
	int iRet;
	uErr = USB_SUCCESS;
	USBIF_DEBUG_CONF("Attempting to set configuration value to %u.\n", *conf);
#ifdef __KERNEL__
	iRet = usb_driver_set_configuration(ucom->handle->usb_handler, *conf);
	if ( iRet )
	{
		USBIF_ERROR("Set configuration error: ");
		USBIF_ERROR("generic failure.\n");
		uErr = USB_ERROR_OTHER;
		return uErr;
	}
#else
	if ( ( iRet = libusb_set_configuration(ucom->handle->usb_handler, (int)*conf) ) )
	{
		USBIF_ERROR("Set configuration error: ");
		if ( iRet == LIBUSB_ERROR_NOT_FOUND )
		{
			USBIF_ERROR("requested configuration does not exist.\n");
			uErr = USB_ERROR_NOT_FOUND;
		}
		else if ( iRet == LIBUSB_ERROR_BUSY )
		{
			USBIF_ERROR("interfaces of current configuration are still claimed.\n");
			uErr = USB_ERROR_BUSY;
		}
		else if ( iRet == LIBUSB_ERROR_NO_DEVICE )
		{
			USBIF_ERROR("device has been disconnected.\n");
			uErr = USB_ERROR_NO_DEVICE;
		}
		else
		{
			USBIF_ERROR("generic failure.\n");
			uErr = USB_ERROR_OTHER;
		}

		return uErr;
	}
#endif

	set_conf_data(ucom->handle->usb_device_desc, conf);
	return uErr;
}
EXPORT_SYMBOL_GPL(usb_set_configuration);

int usb_get_configuration(UsbCom * ucom)
{
	enum usb_error uErr;
	int configuration_value;
	Configuration value;

#ifdef __KERNEL__
	SetupPacket getConfiguration;
	int previous_timeout;
#else
	int iRet;
#endif

	USBIF_DEBUG_CONF("Acquiring USB configuration, storing it in internal configuration descriptor data.\n");
#ifdef __KERNEL__
	getConfiguration.bmRequestType = 0x80;
	getConfiguration.bRequest = 0x08;
	getConfiguration.wValue = 0;
	getConfiguration.wIndex = 0;
	getConfiguration.wLength = 1;

	previous_timeout = ucom->handle->ctrl0_timeout;
	set_ctrl0_timeout(ucom, 1000);				//usb_get_configuration() fails with less time
	if ( ( uErr = transfer_setup(ucom, &getConfiguration, ucom->handle->ctrl0_buf, getConfiguration.wLength) ) != USB_SUCCESS )
	{
		USBIF_ERROR("Get configuration error: ");
		USBIF_ERROR("generic failure.\n");
		setUsbError(uErr);
		return RETURNERROR;
	}
	set_ctrl0_timeout(ucom, previous_timeout);	//returning to user configuration

	configuration_value = ucom->handle->ctrl0_buf[0];
#else
	if ( ( iRet = libusb_get_configuration(ucom->handle->usb_handler, &configuration_value) ) )
	{
		USBIF_ERROR("Get configuration error: ");
		if ( iRet == LIBUSB_ERROR_NO_DEVICE )
		{
			USBIF_ERROR("device has been disconnected.\n");
			uErr = USB_ERROR_NO_DEVICE;
		}
		else
		{
			USBIF_ERROR("generic failure.\n");
			uErr = USB_ERROR_OTHER;
		}
		setUsbError(uErr);
		return RETURNERROR;
	}
#endif
	if ( configuration_value < 0 || configuration_value > UINT8_MAX )
	{
		USBIF_ERROR("Invalid configuration value, out of range.\n");
		setUsbError(USB_ERROR_NOT_FOUND);
		return RETURNERROR;
	}

	value = (uint8_t)configuration_value;

	set_conf_data(ucom->handle->usb_device_desc, &value);
	return value;
}
EXPORT_SYMBOL_GPL(usb_get_configuration);


enum usb_error usb_if_claim(UsbCom * ucom, IfConf * interface_configuration)
{
	enum usb_error uErr;
	int iRet;
#ifdef __KERNEL__
	struct usb_interface * intf;
	void * priv;
	uErr = USB_SUCCESS;
	intf = usb_ifnum_to_if(ucom->handle->usb_handler, interface_configuration->bInterfaceNumber);
	priv = usb_get_intfdata(intf);
	iRet = usb_driver_claim_interface(ucom->handle->usb_context, intf, priv);
	if ( iRet )
		uErr = USB_ERROR_OTHER;
#else
	uErr = USB_SUCCESS;
	if ( ( iRet = libusb_kernel_driver_active(ucom->handle->usb_handler, interface_configuration->bInterfaceNumber) ) )
	{
		USBIF_DEBUG_CONF("Kernel driver active: detaching driver from interface.\n");
		if ( ( iRet = libusb_detach_kernel_driver(ucom->handle->usb_handler, interface_configuration->bInterfaceNumber) ) )
		{
			USBIF_ERROR("Error detaching driver: ");
			if ( iRet == LIBUSB_ERROR_INVALID_PARAM )
			{
				USBIF_ERROR("interface does not exist.\n");
				uErr = USB_ERROR_INVALID_PARAM;
			}
			else if ( iRet == LIBUSB_ERROR_NO_DEVICE )
			{
				USBIF_ERROR("device has been disconnected.\n");
				uErr = USB_ERROR_NO_DEVICE;
			}
			else
			{
				USBIF_ERROR("generic failure.\n");
				uErr = USB_ERROR_OTHER;
			}
			return uErr;
		}
		else
		{
			USBIF_DEBUG_CONF("Queuing detached interface to be re-enabled on exit.\n");
			add_detached_interface(ucom->handle, interface_configuration->bInterfaceNumber);
		}
	}


	USBIF_DEBUG_CONF("Claiming interface control.\n");
	if ( ( iRet = libusb_claim_interface(ucom->handle->usb_handler, interface_configuration->bInterfaceNumber) ) )
	{
		USBIF_ERROR("Error claiming interface: ");
		if ( iRet == LIBUSB_ERROR_NOT_FOUND )
		{
			USBIF_ERROR("interface does not exist.\n");
			uErr = USB_ERROR_NOT_FOUND;
		}
		else if ( iRet == LIBUSB_ERROR_BUSY )
		{
			USBIF_ERROR("another program or driver has claimed the interface.\n");
			uErr = USB_ERROR_BUSY;
		}
		else if ( iRet == LIBUSB_ERROR_NO_DEVICE )
		{
			USBIF_ERROR("device has been disconnected.\n");
			uErr = USB_ERROR_NO_DEVICE;
		}
		else
		{
			USBIF_ERROR("generic failure.\n");
			uErr = USB_ERROR_OTHER;
		}
		return uErr;
	}
#endif
	return uErr;
}
EXPORT_SYMBOL_GPL(usb_if_claim);

enum usb_error usb_release_interface(UsbCom * ucom, IfConf * interface_configuration)
{
	enum usb_error uErr;
#ifdef __KERNEL__
	struct usb_interface * intf;
	intf = usb_ifnum_to_if(ucom->handle->usb_handler, interface_configuration->bInterfaceNumber);
	usb_driver_release_interface(ucom->handle->usb_context, intf);
	uErr = USB_SUCCESS;
#else
	int iRet;
	List * listHook;
	uErr = USB_SUCCESS;
	USBIF_DEBUG_CONF("Releasing interface nr. %u.\n", interface_configuration->bInterfaceNumber);
	iRet = libusb_release_interface(ucom->handle->usb_handler, interface_configuration->bInterfaceNumber);
	if ( iRet )
	{
		USBIF_ERROR("Error releasing interface: ");
		if ( iRet == LIBUSB_ERROR_NOT_FOUND )
		{
			USBIF_ERROR("interface was not claimed.\n");
			uErr = USB_ERROR_NOT_FOUND;
		}
		else if ( iRet == LIBUSB_ERROR_NO_DEVICE )
		{
			USBIF_ERROR("device has been disconnected.\n");
			uErr = USB_ERROR_NO_DEVICE;
		}
		else
		{
			USBIF_ERROR("generic failure.\n");
			uErr = USB_ERROR_OTHER;
		}
		return uErr;
	}

	USBIF_DEBUG_CONF("Re-ataching kernel driver to interfaces.\n");
	for ( listHook = ucom->handle->detached_interfaces; listHook != NULL; listHook = listHook->next )
	{
		if ( ((struct detached_type*)(listHook->data))->interface_nr == interface_configuration->bInterfaceNumber )
		{
			iRet = libusb_attach_kernel_driver(ucom->handle->usb_handler, ((struct detached_type*)(listHook->data))->interface_nr);
			if ( iRet )
			{
				USBIF_ERROR("Error re-ataching interface to driver: ");
				if ( iRet == LIBUSB_ERROR_NOT_FOUND )
				{
					USBIF_ERROR("no kernel driver was active.\n");
					uErr = USB_ERROR_NOT_FOUND;
				}
				else if ( iRet == LIBUSB_ERROR_INVALID_PARAM )
				{
					USBIF_ERROR("interface does not exist.\n");
					uErr = USB_ERROR_INVALID_PARAM;
				}
				else if ( iRet == LIBUSB_ERROR_NO_DEVICE )
				{
					USBIF_ERROR("device has been disconnected.\n");
					uErr = USB_ERROR_NO_DEVICE;
				}
				else if ( iRet == LIBUSB_ERROR_BUSY )
				{
					USBIF_ERROR("interface still claimed by some program or driver.\n");
					uErr = USB_ERROR_BUSY;
				}
				else
				{
					USBIF_ERROR("generic failure.\n");
					uErr = USB_ERROR_OTHER;
				}
			}
		}
	}
#endif

	return uErr;
}
EXPORT_SYMBOL_GPL(usb_release_interface);


AltIf * usb_get_alt_if(UsbCom * ucom, IfConf * interface, uint8_t alternate_interface_nr)
{
	AltIf * altif;
	List * altIfHook;
	altif = NULL;
	for ( altIfHook = interface->alternateInterfaces; altIfHook != NULL; altIfHook = altIfHook->next )
	{
		if ( (*(AltIf**)altIfHook->data)->bAlternateSetting == alternate_interface_nr )
			altif = *(AltIf**)altIfHook->data;
	}
	return altif;
}
EXPORT_SYMBOL_GPL(usb_get_alt_if);

enum usb_error usb_set_alt_if(UsbCom * ucom, AltIf * alternate_interface)
{
	enum usb_error uErr;
	int iRet;
	USBIF_DEBUG_CONF("Selecting alternate interface nr. %u for interface nr. %u.\n", alternate_interface->bAlternateSetting, alternate_interface->ifConf->bInterfaceNumber);
#ifdef __KERNEL__
	iRet = usb_set_interface(ucom->handle->usb_handler, alternate_interface->ifConf->bInterfaceNumber, alternate_interface->bAlternateSetting);
	if ( iRet )
	{
		USBIF_ERROR("Error selecting alternate interface: ");
		USBIF_ERROR("generic failure.\n");
		uErr = USB_ERROR_OTHER;
		return uErr;
	}
#else
	iRet = libusb_set_interface_alt_setting(ucom->handle->usb_handler, alternate_interface->ifConf->bInterfaceNumber, alternate_interface->bAlternateSetting);
	if ( iRet )
	{
		USBIF_ERROR("Error selecting alternate interface: ");
		if ( iRet == LIBUSB_ERROR_NOT_FOUND )
		{
			USBIF_ERROR("interface was not claimed or does not exist.\n");
			uErr = USB_ERROR_NOT_FOUND;
		}
		else if ( iRet == LIBUSB_ERROR_NO_DEVICE )
		{
			USBIF_ERROR("device has been disconnected.\n");
			uErr = USB_ERROR_NO_DEVICE;
		}
		else
		{
			USBIF_ERROR("generic failure.\n");
			uErr = USB_ERROR_OTHER;
		}
		return uErr;
	}
#endif
	USBIF_DEBUG_CONF("Removing transfers registered to the endpoints of the previous active alternate interface.\n");
	close_alternate_interface(ucom->handle, alternate_interface->ifConf->activeInterface);

	USBIF_DEBUG_CONF("Setting enabled alternate interface in the internal data structure.\n");
	set_setting_data(alternate_interface);

	return USB_SUCCESS;
}
EXPORT_SYMBOL_GPL(usb_set_alt_if);


enum usb_status usb_dev_status(UsbDevice * udev)
{
	if ( udev == NULL )
	{
		USBIF_ERROR("USB interface is unitialized.\n");
		return USB_UNITIALIZED;
	}
	else if ( udev->usb_handler == NULL )
	{
		USBIF_ERROR("USB interface is closed.\n");
		return USB_CLOSED;
	}
	return USB_ACTIVE;
}


enum usb_error transfer_setup(UsbCom * ucom, SetupPacket * setup, uint8_t * buf, size_t len)
{
	enum usb_error uErr;
	int iRet;
#ifdef SHOWTRANSFERS
	uint16_t i;
#endif
#ifdef __KERNEL__
	unsigned int pipe;
#endif

	if ( usb_dev_status(ucom->handle) != USB_ACTIVE )
		return USB_ERROR_NO_DEVICE;

	if ( len < setup->wLength )
	{
		USBIF_ERROR("Buffer length smaller than setup packet wLength, cannot completely cache transmission.\n");
		return USB_ERROR_INVALID_PARAM;
	}

	if ( ucom->handle->ctrl0_timeout > INT_MAX )
	{
		USBIF_ERROR("Timeout value too big to be passed as argument.\n");
		return USB_ERROR_INVALID_PARAM;
	}
#ifdef SHOWTRANSFERS
	USBIF_INFO("Simulating data transfer:\n");
	USBIF_INFO("	usb_handler:	0x%x\n", ucom->handle->usb_handler);
	USBIF_INFO("	bmRequestType:	0x%x\n", setup->bmRequestType);
	USBIF_INFO("	bRequest:		0x%x\n", setup->bRequest);
	USBIF_INFO("	wValue:			0x%x\n", setup->wValue);
	USBIF_INFO("	wIndex:			0x%x\n", setup->wIndex);
	USBIF_INFO("	wLength:		0x%x\n", setup->wLength);
	if ( !( setup->bmRequestType & ENDPOINT_IN ) )
	{
		USBIF_INFO("Data to be sent:\n");
		for (i = 0; i < setup->wLength; i++)
		{
			USBIF_INFO("Data byte %d:	0x%x\n", i, buf[i]);
		}
	}
	else
		USBIF_INFO("Data would be received. Buffer available.\n");
	USBIF_INFO("Data transfer simulation finished.\n");
	iRet = setup->wLength;
#endif
#ifndef SIMULATE
#ifdef __KERNEL__
	if ( setup->bmRequestType & ENDPOINT_IN )
		pipe = usb_rcvctrlpipe(ucom->handle->usb_handler, 0);
	else
		pipe = usb_sndctrlpipe(ucom->handle->usb_handler, 0);

	iRet = usb_control_msg(ucom->handle->usb_handler,
			pipe,
			setup->bRequest,
			setup->bmRequestType,
			setup->wValue,
			setup->wIndex,
			buf,
			setup->wLength,
			ucom->handle->ctrl0_timeout);
	if ( iRet < 0 )
	{
		USBIF_ERROR("generic failure.\n");
		uErr = USB_ERROR_OTHER;
	}
#else
	USBIF_DEBUG_USB("Transferring data through ctrl endpoint 0.\n");
	iRet = libusb_control_transfer(ucom->handle->usb_handler,
								setup->bmRequestType,
								setup->bRequest,
								setup->wValue,
								setup->wIndex,
			(unsigned char*)	buf,
								setup->wLength,
			(int)				ucom->handle->ctrl0_timeout);
	if ( iRet < 0 )
	{
		if ( iRet == LIBUSB_ERROR_TIMEOUT )
		{
			USBIF_ERROR("Transfer timed out.\n");
			uErr = USB_ERROR_TIMEOUT;
		}
		else if ( iRet == LIBUSB_ERROR_PIPE )
		{
			USBIF_ERROR("Control request not supported by device.\n");
			uErr = USB_ERROR_PIPE;
		}
		else if ( iRet == LIBUSB_ERROR_NO_DEVICE )
		{
			USBIF_ERROR("Device has been disconnected.\n");
			uErr = USB_ERROR_NO_DEVICE;
		}
		else
		{
			USBIF_ERROR("generic failure.\n");
			uErr = USB_ERROR_OTHER;
		}
	}
#endif
#endif

	if ( iRet == setup->wLength )
		uErr = USB_SUCCESS;
	else if ( iRet > setup->wLength )
		uErr = USB_ERROR_OVERFLOW;
	else if ( iRet >= 0 )
		uErr = USB_ERROR_INTERRUPTED;

	return uErr;
}
EXPORT_SYMBOL_GPL(transfer_setup);

void set_ctrl0_timeout(UsbCom * ucom, uint32_t timeout)
{
	ucom->handle->ctrl0_timeout = timeout;
}
EXPORT_SYMBOL_GPL(set_ctrl0_timeout);

