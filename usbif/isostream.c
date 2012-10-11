/*
 * isostream.c
 *
 *  Created on: Jul 7, 2011
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


static void iso_stream_launch(work_t * work)
{
	enum usb_error uErr;
	ThreadData * data;

	Transfer * transfer;
	EndpointHandle * endpointHandle;

	size_t total_len;
	size_t * p_packet_len;
	int nr_of_packets;

	data = get_thread_data(work);

	transfer = data->transfer;
	endpointHandle = data->endpointHandle;

	nr_of_packets = get_iso_number_of_packets(transfer);
	p_packet_len = ALLOC( nr_of_packets * sizeof(size_t));

	compact_packet(transfer);
	total_len = get_iso_length(transfer, p_packet_len);

	if ( endpointHandle->callback )
	{
		USBIF_DEBUG_IF("Calling registered callback function of endpoint %x.\n", endpointHandle->endpoint->p_endpoint_desc->bEndpointAddress);
		if ( (*endpointHandle->callback)(endpointHandle->context, (uint8_t*)get_transfer_buffer(transfer), total_len, p_packet_len, nr_of_packets) == REACTIVATE )
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
		USBIF_DEBUG_IF("Callback released: interrupt auto-disabled for this transfer on endpoint %x.\n", endpointHandle->endpoint->p_endpoint_desc->bEndpointAddress);
		transfer_complete(endpointHandle->endpoint, transfer);
	}

	FREE(p_packet_len);

}


//internal functions
static void iso_stream_callback(Transfer * transfer)
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
		iso_stream_launch(&(*data)->work);
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


//for isochronous endpoint (combined with interrupt)
enum usb_error register_dma_space(UsbCom * ucom, IsoEndpoint * endpoint, uint8_t * buf, size_t len)
{
	uint8_t nr_iso_packets, min_nr_iso_packets;

	ThreadData data;
	EndpointHandle * endpointHandle;
	Thread * thread;
	Transfer * transfer;

	if ( len > INT_MAX )
	{
		USBIF_ERROR("Transfer length is not within integer parameter's range.\n");
		return USB_ERROR_INVALID_PARAM;
	}

	//to ensure throughput, we consider high-speed
	//for high-speed, we should have 10ms of pending transfers
	//USB high-speed transfers a micro-frame every 125us
	//thus, in 10 ms of USB tranfers, we have 80 packets
	//USB also allows isochronous endpoints to have less than 1 packet each micro-frame
	//for that we divide 80 by the interval value

	/*80 packets are 10ms on high-speed for bInterval = 1, we account for truncation*/
	min_nr_iso_packets = 80/endpoint->bInterval + ( (80 % endpoint->bInterval) ? 1 : 0 );

	/*we don't account for truncation because we cannot create a packet for which the buffer has not enough data*/
	nr_iso_packets = len/endpoint->wMaxPacketSize;
	if ( nr_iso_packets <= 0 )
	{
		USBIF_ERROR("Buffer is too small: does not hold a complete packet.\n");
		return USB_ERROR_NO_MEM;
	}

/*
	//to have 10 ms of pending tranfers, we need two transfers of the minimal length calculated above
	if ( nr_iso_packets < min_nr_iso_packets )
	{
		USBIF_ERROR("Heuristically, pending transfers have to cover 10 ms of data.\n");
		USBIF_ERROR("The length of the input buffer does not suffice for that.\n");
		USBIF_INFO("As long as two buffers of this size are registered, data should be acquired without losses.\n");
		return USB_ERROR_INVALID_PARAM;
	}
*/
	endpointHandle = get_endpoint_handle(ucom->handle, endpoint);
	if ( endpointHandle == NULL )
		return USB_ERROR_NO_DEVICE;

	thread = new_thread(endpointHandle->process);

	USBIF_DEBUG_USB("Allocating %d Isochronous packets\n", nr_iso_packets);
	transfer = add_endpoint_iso_transfer(ucom, endpointHandle->endpoint, nr_iso_packets,
			endpoint->wMaxPacketSize, buf, len, iso_stream_callback, &thread->data);
	if ( transfer == NULL )
	{
		destroy_thread(thread);
		return getUsbError();
	}

	data.endpointHandle = endpointHandle;
	data.transfer = transfer;
	initialize_thread(thread, iso_stream_launch, &data);

	return USB_SUCCESS;
}
EXPORT_SYMBOL_GPL(register_dma_space);

enum usb_error unregister_dma_spaces(UsbCom * ucom, IsoEndpoint * endpoint)
{
	EndpointHandle * isoEndpoint;
	void * callback;
	isoEndpoint = get_endpoint_handle(ucom->handle, endpoint);
	if ( isoEndpoint == NULL )
		return USB_ERROR_PIPE;

	USBIF_DEBUG_IF("Disconnecting buffers bound to transfers of endpoint %x.\n", endpoint->bEndpointAddress);
	callback = isoEndpoint->callback;
	isoEndpoint->callback = NULL;
	finish_pending_threads(isoEndpoint->process);
	unregister_endpoint_transfers(isoEndpoint->endpoint);
	remove_threads(isoEndpoint->process);
	isoEndpoint->callback = callback;
	return USB_SUCCESS;
}
EXPORT_SYMBOL_GPL(unregister_dma_spaces);

enum usb_error register_dma_interrupt(UsbCom * ucom, IsoEndpoint * endpoint, enum cb_ret (*func)(void * context, uint8_t * buf, size_t len, size_t * p_packet_len, size_t nr_of_packets), void * context)
{
	EndpointHandle * isoEndpoint;
	isoEndpoint = get_endpoint_handle(ucom->handle, endpoint);
	if ( isoEndpoint == NULL )
		return USB_ERROR_PIPE;

	USBIF_DEBUG_IF("Setting interrupt callback function of endpoint %x.\n", endpoint->bEndpointAddress);
	isoEndpoint->context = context;
	isoEndpoint->callback = func;
	return USB_SUCCESS;
}
EXPORT_SYMBOL_GPL(register_dma_interrupt);

enum usb_error unregister_dma_interrupt(UsbCom * ucom, IsoEndpoint * endpoint)
{
	EndpointHandle * isoEndpoint;
	isoEndpoint = get_endpoint_handle(ucom->handle, endpoint);
	if ( isoEndpoint == NULL )
		return USB_ERROR_PIPE;

	USBIF_DEBUG_IF("Canceling interrupt callback function of endpoint %x.\n", endpoint->bEndpointAddress);
	isoEndpoint->callback = NULL;
	isoEndpoint->context = NULL;
	finish_pending_threads(isoEndpoint->process);
	return USB_SUCCESS;
}
EXPORT_SYMBOL_GPL(unregister_dma_interrupt);

enum usb_error unlock_dma_spaces(UsbCom * ucom, IsoEndpoint * endpoint)
{
	EndpointHandle * endpointHandle;
	endpointHandle = get_endpoint_handle(ucom->handle, endpoint);
	if ( endpointHandle == NULL )
		return USB_ERROR_PIPE;

	USBIF_DEBUG_IF("Enabling interrupt of endpoint %x.\n", endpoint->bEndpointAddress);
	return trigger_endpoint_transfers(ucom->handle, endpointHandle->endpoint);
}
EXPORT_SYMBOL_GPL(unlock_dma_spaces);

void set_dma_timeout(UsbCom * ucom, IsoEndpoint * endpoint, uint32_t timeout)
{
	EndpointHandle * endpointHandle;
	endpointHandle = get_endpoint_handle(ucom->handle, endpoint);
	if ( endpointHandle != NULL )
		endpointHandle->endpoint->timeout = timeout;
}
EXPORT_SYMBOL_GPL(set_dma_timeout);
