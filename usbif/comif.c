/*
 * comif.c
 *
 *  Created on: Jun 28, 2011
 *      Author: raul
 */

#include <devif/com.h>
#include <devif/regdata.h>
#include "usbdata.h"
#include "usbint.h"
#include "usbif.h"
#include "usbifdebug.h"
#include "com.h"


static enum com_error comerror = COM_OK;


static enum com_error transfer_reg(UsbCom * ucom, SetupPacket * setup)
{
	enum com_error if_err;
	enum usb_error uErr;

	if ( setup->wLength > ucom->handle->usb_device_desc->bMaxPacketSize0 )
	{
		USBIF_ERROR("Setup packet is too long for endpoint 0 buffer.\n");
		return COM_BUFFER_OVERFLOW;
	}

	uErr = transfer_setup(ucom, setup, ucom->handle->ctrl0_buf, setup->wLength);

	if ( uErr != USB_SUCCESS )
		if_err = COM_OTHER;
	else
		if_err = COM_OK;

	if ( uErr == USB_ERROR_TIMEOUT )
	{
		USBIF_ERROR("Transfer timed out.\n");
		if_err = COM_TIMEOUT;
	}
	else if ( uErr == USB_ERROR_PIPE )
	{
		USBIF_ERROR("Control request not supported by device.\n");
		if_err = COM_NO_DEVICE_SUPPORT;
	}
	else if ( uErr == USB_ERROR_NO_DEVICE )
	{
		USBIF_ERROR("Device has been disconnected.\n");
		if_err = COM_NO_DEVICE;
	}

	return if_err;
}


void setComError(enum com_error err)
{
	comerror = err;
}

enum com_error getComError(void)
{
	enum com_error tmperror = comerror;
	comerror = COM_OK;
	return tmperror;
}


unsigned int readIf(void * com, unsigned int addr, void * if_complex_data)
{
	enum com_error err;
	unsigned int value;
	UsbData * usb_data;
	int i;
	UsbCom * ucom;

	ucom = (UsbCom*)com;

	USBIF_DEBUG_COM("Reading register value through USB setup packet recv_packet of regiser's if_complex_data.\n");
	/*communication*/
	usb_data = (UsbData*)if_complex_data;

	if ( usb_data->recv_packet->wLength > ucom->handle->usb_device_desc->bMaxPacketSize0 )
	{
		USBIF_ERROR("wLength of corresponding setup packet exceeds max packet length defined for ctrl endpoint 0.\n");
		setComError(COM_BUFFER_OVERFLOW);
		return RETURNERROR;
	}

	err = transfer_reg(ucom, usb_data->recv_packet);
	if ( err )
	{
		setComError(err);
		return RETURNERROR;
	}

	USBIF_DEBUG_COM("Iterating over uint8_t buffer with incoming data and translating it in 32 bit unsigned value.\n");
	value = 0;
	for (i = 0; i < sizeof(value); i++)
	{
		value |= ucom->handle->ctrl0_buf[i] << ( 8 * sizeof(ucom->handle->ctrl0_buf[i]) * i );			//USB transfers little endian
	}
	/*communication*/

	setComError(err);

	return value;
}

enum com_error writeIf(void * com, unsigned int addr, void * if_complex_data, unsigned int value)
{
	enum com_error err;
	UsbData * usb_data;
	int i;
	UsbCom * ucom;

	ucom = (UsbCom*)com;

	USBIF_DEBUG_COM("Writing register value through USB setup packet send_packet of regiser's if_complex_data.\n");
	/*communication*/
	usb_data = (UsbData*)if_complex_data;

	if ( usb_data->send_packet->wLength > ucom->handle->usb_device_desc->bMaxPacketSize0 )
	{
		USBIF_ERROR("wLength of corresponding setup packet exceeds max packet length defined for ctrl endpoint 0.\n");
		return COM_BUFFER_OVERFLOW;
	}

	if ( !usb_data->send_packet->wLength )
	{
		USBIF_INFO("Send request does not include data to send, assuming that data will be sent through wValue.\n");
		usb_data->send_packet->wValue = value;		//if data is not sent through data it must be sent through wValue (depends on request)
	}

	USBIF_DEBUG_COM("Translating the 32 bit unsigned value to the uint8_t buffer with outgoing data by iterating over bit fields.\n");
	for (i = 0; i < sizeof(value); i++)
	{
		ucom->handle->ctrl0_buf[i] = 0xFF & (value >> ( 8 * sizeof(ucom->handle->ctrl0_buf[i]) * i ) );		//USB transfers little endian
	}

	err = transfer_reg(ucom, usb_data->send_packet);
	/*communication*/

	return err;
}
