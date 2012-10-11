/*
 * endpoint.c
 *
 *  Created on: Jul 21, 2012
 *      Author: raul
 */


#include "endpoint.h"
#include "transfer.h"

#include "usbifdebug.h"
#include "streamif.h"


static void add_transfer(Endpoint * endpoint, Transfer * transfer);
static void _transfer_complete(Endpoint * endpoint, Transfer * transfer);
static void _transfer_pending(Endpoint * endpoint, Transfer * transfer);
static void _remove_transfer(Endpoint * endpoint, Transfer * transfer);


Transfer * add_endpoint_transfer(Endpoint * endpoint, size_t nr_of_iso_packets)
{
	Transfer * tmpTransfer;
	if ( nr_of_iso_packets > INT_MAX )
	{
		USBIF_ERROR("Number of iso packets exceed parameter range.\n");
		setUsbError(USB_ERROR_INVALID_PARAM);
		return NULL;
	}

	USBIF_DEBUG_USB("Allocating a transfer for endpoint 0x%x.\n", (unsigned int)endpoint->p_endpoint_desc->bEndpointAddress);
	tmpTransfer = alloc_transfer(nr_of_iso_packets);
	if ( tmpTransfer == NULL )
	{
		USBIF_ERROR("Transfer could not be allocated.\n");
		setUsbError(USB_ERROR_NO_MEM);
		return NULL;
	}

	add_transfer(endpoint, tmpTransfer);

	return tmpTransfer;
}


Transfer * add_endpoint_iso_transfer(UsbCom * ucom, Endpoint * endpoint, int nr_iso_packets, unsigned int iso_packet_length,
		unsigned char * buf, int buf_len,
		void (*callback)(Transfer * transfer), void * context)
{
	Transfer * tmpTransfer;
	unsigned int timeout;

	if ( ( endpoint->p_endpoint_desc->bmAttributes & TRANSFER_TYPE_MASK ) != TRANSFER_TYPE_ISOCHRONOUS )
	{
		USBIF_ERROR("Endpoint's transfer type is not of isochronous type.\n");
		setUsbError(USB_ERROR_NOT_SUPPORTED);
		return NULL;
	}

	timeout = (unsigned int)endpoint->timeout;

	tmpTransfer = add_endpoint_transfer(endpoint, nr_iso_packets);
	if ( tmpTransfer == NULL )
	{
		USBIF_ERROR("No memory to allocate an endpoint transfer.\n");
		return NULL;
	}

	fill_iso_transfer(ucom, tmpTransfer,
			endpoint->p_endpoint_desc->bEndpointAddress, nr_iso_packets, iso_packet_length,
			buf, buf_len,
			callback, context, timeout);
	return tmpTransfer;
}

Transfer * add_endpoint_int_transfer(UsbCom * ucom, Endpoint * endpoint,
		unsigned char * buf, int buf_len,
		void (*callback)(Transfer * transfer), void * context)
{
	Transfer * tmpTransfer;
	unsigned int timeout;

	if ( ( endpoint->p_endpoint_desc->bmAttributes & TRANSFER_TYPE_MASK ) != TRANSFER_TYPE_INTERRUPT )
	{
		USBIF_ERROR("Endpoint's transfer type is not of interrupt type.\n");
		setUsbError(USB_ERROR_NOT_SUPPORTED);
		return NULL;
	}

	if ( buf_len > INT_MAX )
	{
		USBIF_ERROR("Transfer length is not within integer parameter's range.\n");
		setUsbError(USB_ERROR_INVALID_PARAM);
		return NULL;
	}

	timeout = (unsigned int)endpoint->timeout;

	tmpTransfer = add_endpoint_transfer(endpoint, 0);
	if ( tmpTransfer == NULL )
	{
		USBIF_ERROR("No memory to allocate an endpoint transfer.\n");
		return NULL;
	}

	fill_int_transfer(ucom, tmpTransfer,
			endpoint->p_endpoint_desc->bEndpointAddress,
			buf, buf_len,
			callback, context, timeout);
	return tmpTransfer;
}


size_t endpoint_transfer_bulk(UsbCom * ucom, Endpoint * endpoint, uint8_t * buf, size_t len)
{
	unsigned int timeout;

	if ( usb_dev_status(ucom->handle) != USB_ACTIVE )
	{
		setUsbError(USB_ERROR_NO_DEVICE);
		return RETURNERROR;
	}

	if ( !check_conf(ucom->handle->usb_device_desc, endpoint->p_endpoint_desc->configuration_value) )
	{
		setUsbError(USB_ERROR_ACCESS);
		return RETURNERROR;
	}

	if ( !check_setting(endpoint->p_endpoint_desc->parent_altif ) )
	{
		setUsbError(USB_ERROR_ACCESS);
		return RETURNERROR;
	}

	if ( ( endpoint->p_endpoint_desc->bmAttributes & TRANSFER_TYPE_MASK ) != TRANSFER_TYPE_BULK )
	{
		USBIF_ERROR("Endpoint's transfer type is not of bulk type.\n");
		setUsbError(USB_ERROR_NOT_SUPPORTED);
		return RETURNERROR;
	}

	if ( len > INT_MAX )
	{
		USBIF_ERROR("Transfer length is not within integer parameter's range.\n");
		setUsbError(USB_ERROR_INVALID_PARAM);
		return RETURNERROR;
	}

	timeout = (unsigned int)endpoint->timeout;

	return transfer_bulk(ucom, endpoint->p_endpoint_desc->bEndpointAddress, buf, len, timeout);
}


enum usb_error unregister_endpoint_transfers(Endpoint * endpoint)
{
	enum usb_error uErr;
	uErr = cancel_endpoint_transfers(endpoint);
	return uErr;
}


enum usb_error trigger_endpoint_transfers(UsbDevice * udev, Endpoint * endpoint)
{
	enum usb_error uErr;
	List * listHook, * nextHook;
	unsigned long flags;

	if ( usb_dev_status(udev) != USB_ACTIVE )
		return USB_ERROR_NO_DEVICE;

	if ( !check_conf(udev->usb_device_desc, endpoint->p_endpoint_desc->configuration_value) )
		return USB_ERROR_ACCESS;

	if ( !check_setting(endpoint->p_endpoint_desc->parent_altif ) )
		return USB_ERROR_ACCESS;

	LOCK(&endpoint->lock, flags);
	USBIF_DEBUG_USB("Submitting all endpoint's allocated transfers from endpoint %x.\n", endpoint->p_endpoint_desc->bEndpointAddress);
	listHook = endpoint->complete_transfers;
	while ( listHook != NULL )
	{
		nextHook = listHook->next;
		uErr = submit_transfer((Transfer*)listHook->data);
		if ( uErr == USB_SUCCESS )
		{
			USBIF_DEBUG_USB("Transfer submitted\n");
			_transfer_pending(endpoint, listHook->data);
		}
		listHook = nextHook;
	}
	UNLOCK(&endpoint->lock, flags);
	return USB_SUCCESS;
}

enum usb_error cancel_endpoint_transfers(Endpoint * endpoint)
{
	enum usb_error uErr;
	List * listHook, * nextHook;

	unsigned long flags;
	LOCK(&endpoint->lock, flags);

	USBIF_DEBUG_USB("Canceling all endpoint's allocated transfers of endpoint %x.\n", endpoint->p_endpoint_desc->bEndpointAddress);
	listHook = endpoint->complete_transfers;
	while ( listHook != NULL )
	{
		nextHook = listHook->next;
		USBIF_DEBUG_USB("Transfer removed.\n");
		_remove_transfer(endpoint, (Transfer*)listHook->data);
		listHook = nextHook;
	}

	listHook = endpoint->pending_transfers;
	while ( listHook != NULL )
	{
		nextHook = listHook->next;
		uErr = cancel_transfer((Transfer*)listHook->data);
		if ( uErr == USB_TRANSFER_COMPLETED )
		{
			USBIF_DEBUG_USB("Transfer completed and thus removed.\n");
			_transfer_complete(endpoint, listHook->data);
			_remove_transfer(endpoint, (Transfer*)listHook->data);
		}
		else if ( uErr != USB_SUCCESS )
		{
			USBIF_ERROR("Fatal error canceling a submitted transfer.\n");
			UNLOCK(&endpoint->lock, flags);
			return uErr;
		}
		else if ( uErr == USB_SUCCESS )
			USBIF_DEBUG_USB("Transfer unlinked. It will be removed in the top half interrupt handler.\n");
		else
			USBIF_ERROR("Error unlinking transfer\n");
		listHook = nextHook;
	}
	UNLOCK(&endpoint->lock, flags);

	/*TODO*/
	//use a wait_queue to wait for all transfers to complete,
	//complete on iso and int complete functions
	//do something similar in usermode
	while (endpoint->pending_transfers)
		MSLEEP(100);
	USBIF_DEBUG_USB("All transfers removed\n");

	return USB_SUCCESS;
}


void transfer_complete(Endpoint * endpoint, Transfer * transfer)
{
	unsigned long flags;
	LOCK(&endpoint->lock, flags);
	_transfer_complete(endpoint, transfer);
	UNLOCK(&endpoint->lock, flags);
}

void remove_transfer(Endpoint * endpoint, Transfer * transfer)
{
	unsigned long flags;
	LOCK(&endpoint->lock, flags);
	_remove_transfer(endpoint, transfer);
	UNLOCK(&endpoint->lock, flags);
}

static void add_transfer(Endpoint * endpoint, Transfer * transfer)
{
	List * listHook;
	unsigned long flags;
	listHook = ALLOC(sizeof(List));
	listHook->data = transfer;
	USBIF_DEBUG_LISTS("Including transfer 0x%lx into endpoint's transfers list.\n", (unsigned long)transfer);
	LOCK(&endpoint->lock, flags);
	pushBack(&endpoint->complete_transfers, listHook);
	UNLOCK(&endpoint->lock, flags);
}


static void _transfer_complete(Endpoint * endpoint, Transfer * transfer)
{
	List * hook;
	hook = popElement(&endpoint->pending_transfers, transfer);
	if ( hook != NULL )
	{
		USBIF_DEBUG_LISTS("Passing transfer to complete list.\n");
		pushBack(&endpoint->complete_transfers, hook);
	}
}

static void _transfer_pending(Endpoint * endpoint, Transfer * transfer)
{
	List * hook;
	hook = popElement(&endpoint->complete_transfers, transfer);
	if ( hook != NULL )
	{
		USBIF_DEBUG_LISTS("Passing transfer to pending list.\n");
		pushBack(&endpoint->pending_transfers, hook);
	}
}

static void _remove_transfer(Endpoint * endpoint, Transfer * transfer)
{
	List * hook;
	hook = popElement(&endpoint->complete_transfers, transfer);
	if ( hook != NULL )
	{
		USBIF_DEBUG_LISTS("Freeing allocated transfer 0x%lx and removing element of endpoint's transfers list.\n", (unsigned long)hook->data);
		free_transfer((Transfer *)hook->data);
		FREE(hook);
	}
}

