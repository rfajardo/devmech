/*
 * intstream.c
 *
 *  Created on: Jul 12, 2011
 *      Author: raul
 */

#include "streamif.h"
#include "usbint.h"
#include "usbifdebug.h"
#include "com.h"

#include "transfer.h"
#include "thread.h"
#include "endpoint.h"
#include "process.h"


void int_stream_launch(work_t * work)
{
	enum usb_error uErr;
	ThreadData * data;

	Transfer * transfer;
	EndpointHandle * endpointHandle;

	data = get_thread_data(work);

	transfer = data->transfer;
	endpointHandle = data->endpointHandle;

	USBIF_DEBUG_IF("Calling registered callback function of endpoint %x.\n", endpointHandle->endpoint->p_endpoint_desc->bEndpointAddress);

	if ( endpointHandle->callback != NULL )
	{
		if ( get_status(transfer) == USB_SUCCESS )
		{
			if ( (*endpointHandle->callback)(endpointHandle->context, (uint8_t*)get_transfer_buffer(transfer), (size_t)transfer->actual_length, NULL, 0) == REACTIVATE )
			{
				uErr = submit_transfer(transfer);
				if ( uErr == USB_SUCCESS )
					USBIF_DEBUG_USB("Transfer re-submitted\n");
				else
					USBIF_DEBUG_USB("Re-submission failed");
			}
			else
			{
				USBIF_DEBUG_IF("Interrupt auto-disabled for this transfer on endpoint %x.\n", endpointHandle->endpoint->p_endpoint_desc->bEndpointAddress);
				transfer_complete(endpointHandle->endpoint, transfer);
			}
		}
		else
		{
			USBIF_DEBUG_USB("Transfer failed, re-launch.\n");
			uErr = submit_transfer(transfer);
			if ( uErr == USB_SUCCESS )
				USBIF_DEBUG_USB("Transfer re-submitted\n");
			else
				USBIF_DEBUG_USB("Re-submission failed");
		}
	}
	else
	{
		USBIF_DEBUG_IF("Callback released: interrupt auto-disabled for this transfer on endpoint %x.\n", endpointHandle->endpoint->p_endpoint_desc->bEndpointAddress);
		transfer_complete(endpointHandle->endpoint, transfer);
	}
}

//internal functions
static void int_stream_callback(Transfer * transfer)
{
	ThreadData ** data;

	Thread * thread;
	EndpointHandle * endpointHandle;

	data = get_context(transfer);

	thread = get_thread(data);

	endpointHandle = (*data)->endpointHandle;

	if ( get_status(transfer) == USB_TRANSFER_CANCELED )
	{
		USBIF_DEBUG_USB("Removing canceled transfer.\n");
		transfer_complete(endpointHandle->endpoint, transfer);
		remove_transfer(endpointHandle->endpoint, transfer);
		return;
	}

#ifdef __KERNEL__
	if ( endpointHandle->callback )
		launch_thread(endpointHandle->process, thread);
#else
	if ( endpointHandle->callback )
		int_stream_launch(&(*data)->work);
#endif
	else if ( get_status(transfer) == USB_SUCCESS )
		transfer_complete(endpointHandle->endpoint, transfer);
	else
	{
		USBIF_DEBUG_USB("Removing failed transfer.\n");
		transfer_complete(endpointHandle->endpoint, transfer);
		remove_transfer(endpointHandle->endpoint, transfer);
	}
}


enum usb_error register_interrupt_buffer(UsbCom * ucom, IntEndpoint * endpoint, uint8_t * buf, size_t len)
{
	ThreadData data;
	EndpointHandle * endpointHandle;
	Thread * thread;
	Transfer * transfer;

	endpointHandle = get_endpoint_handle(ucom->handle, endpoint);
	if ( endpointHandle == NULL )
		return USB_ERROR_NO_DEVICE;

	thread = new_thread(endpointHandle->process);

	transfer = add_endpoint_int_transfer(ucom, endpointHandle->endpoint, buf, len, int_stream_callback, endpointHandle);
	if ( transfer == NULL )
		return getUsbError();

	data.endpointHandle = endpointHandle;
	data.transfer = transfer;
	initialize_thread(thread, int_stream_launch, &data);

	return USB_SUCCESS;
}
EXPORT_SYMBOL_GPL(register_interrupt_buffer);

enum usb_error unregister_interrupt_buffers(UsbCom * ucom, IntEndpoint * endpoint)
{
	EndpointHandle * endpointHandle;
	void * callback;
	endpointHandle = get_endpoint_handle(ucom->handle, endpoint);
	if ( endpointHandle == NULL )
		return USB_ERROR_PIPE;

	USBIF_DEBUG_IF("Disconnecting buffers bound to transfers of endpoint %x.\n", endpoint->bEndpointAddress);
	callback = endpointHandle->callback;
	endpointHandle->callback = NULL;
	finish_pending_threads(endpointHandle->process);
	unregister_endpoint_transfers(endpointHandle->endpoint);
	remove_threads(endpointHandle->process);
	endpointHandle->callback = callback;
	return USB_SUCCESS;
}
EXPORT_SYMBOL_GPL(unregister_interrupt_buffers);

//interrupt endpoint
enum usb_error register_interrupt(UsbCom * ucom, IntEndpoint * endpoint, enum cb_ret (*func)(void * context, uint8_t * buf, size_t len, size_t * p_packet_len, size_t nr_of_packets), void * context)
{
	EndpointHandle * endpointHandle;
	endpointHandle = get_endpoint_handle(ucom->handle, endpoint);
	if ( endpointHandle == NULL )
		return USB_ERROR_PIPE;

	USBIF_DEBUG_IF("Setting interrupt callback function of endpoint %x.\n", endpoint->bEndpointAddress);
	endpointHandle->context = context;
	endpointHandle->callback = func;
	return USB_SUCCESS;
}
EXPORT_SYMBOL_GPL(register_interrupt);

enum usb_error unregister_interrupt(UsbCom * ucom, IntEndpoint * endpoint)
{
	EndpointHandle * endpointHandle;
	endpointHandle = get_endpoint_handle(ucom->handle, endpoint);
	if ( endpointHandle == NULL )
		return USB_ERROR_PIPE;

	USBIF_DEBUG_IF("Canceling interrupt callback function of endpoint %x.\n", endpoint->bEndpointAddress);
	endpointHandle->callback = NULL;
	endpointHandle->context = NULL;
	finish_pending_threads(endpointHandle->process);
	return USB_SUCCESS;
}
EXPORT_SYMBOL_GPL(unregister_interrupt);


enum usb_error unlock_interrupt(UsbCom * ucom, IntEndpoint * endpoint)
{
	EndpointHandle * isoEndpoint;
	isoEndpoint = get_endpoint_handle(ucom->handle, endpoint);
	if ( isoEndpoint == NULL )
		return USB_ERROR_PIPE;

	USBIF_DEBUG_IF("Enabling interrupt of endpoint %x.\n", endpoint->bEndpointAddress);
	return trigger_endpoint_transfers(ucom->handle, isoEndpoint->endpoint);
}
EXPORT_SYMBOL_GPL(unlock_interrupt);

void set_interrupt_timeout(UsbCom * ucom, IntEndpoint * endpoint, uint32_t timeout)
{
	EndpointHandle * isoEndpoint;
	isoEndpoint = get_endpoint_handle(ucom->handle, endpoint);
	if ( isoEndpoint != NULL )
		isoEndpoint->endpoint->timeout = timeout;
}
EXPORT_SYMBOL_GPL(set_interrupt_timeout);
