/*
 * streamif.c
 *
 *  Created on: Jul 6, 2011
 *      Author: raul
 */

#include "streamif.h"
#include "usbint.h"
#include "usbifdebug.h"
#include "com.h"

#include "endpoint.h"


static enum usb_error usberror = USB_SUCCESS;


void setUsbError(enum usb_error err)
{
	usberror = err;
}

enum usb_error getUsbError(void)
{
	enum usb_error tmperror = usberror;
	usberror = USB_SUCCESS;
	return tmperror;
}
EXPORT_SYMBOL_GPL(getUsbError);

//bulk transfers
size_t get_stream(UsbCom * ucom, BulkEndpoint * endpoint, uint8_t * buf, size_t len)
{
	size_t transferred_length;
	EndpointHandle * endpointHandle;

	if ( ( endpoint->bEndpointAddress & ENDPOINT_DIR_MASK ) != ENDPOINT_IN )
	{
		USBIF_ERROR("Endpoint's is not an IN endpoint.\n");
		setUsbError(USB_ERROR_ACCESS);
		return RETURNERROR;
	}

	endpointHandle = get_endpoint_handle(ucom->handle, endpoint);
	if ( endpointHandle == NULL )
	{
		setUsbError(USB_ERROR_PIPE);
		return RETURNERROR;
	}

	USBIF_DEBUG_IF("Acquiring a stream of data from endpoint %x.\n", endpoint->bEndpointAddress);
	transferred_length = endpoint_transfer_bulk(ucom, endpointHandle->endpoint, buf, len);

	return transferred_length;
}
EXPORT_SYMBOL_GPL(get_stream);

size_t set_stream(UsbCom * ucom, BulkEndpoint * endpoint, uint8_t * buf, size_t len)
{
	size_t transferred_length;
	EndpointHandle * endpointHandle;

	if ( ( endpoint->bEndpointAddress & ENDPOINT_DIR_MASK ) != ENDPOINT_OUT )
	{
		USBIF_ERROR("Endpoint's is not an OUT endpoint.\n");
		setUsbError(USB_ERROR_ACCESS);
		return RETURNERROR;
	}

	endpointHandle = get_endpoint_handle(ucom->handle, endpoint);
	if ( endpointHandle == NULL )
	{
		setUsbError(USB_ERROR_PIPE);
		return RETURNERROR;
	}

	USBIF_DEBUG_IF("Sending a stream of data through endpoint %x.\n", endpoint->bEndpointAddress);
	transferred_length = endpoint_transfer_bulk(ucom, endpointHandle->endpoint, buf, len);

	return transferred_length;
}
EXPORT_SYMBOL_GPL(set_stream);

void set_stream_timeout(UsbCom * ucom, BulkEndpoint * endpoint, uint32_t timeout)
{
	EndpointHandle * stdEndpointDesc;
	stdEndpointDesc = get_endpoint_handle(ucom->handle, endpoint);
	if ( stdEndpointDesc != NULL )
		stdEndpointDesc->endpoint->timeout = timeout;
}
EXPORT_SYMBOL_GPL(set_stream_timeout);
