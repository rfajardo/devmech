/*
 * transfer.c
 *
 *  Created on: Jul 21, 2012
 *      Author: raul
 */

#include "transfer.h"
#include "usbif.h"
#include "streamif.h"
#include "usbifdebug.h"

#include <devif/types.h>


#ifndef __KERNEL__
#include <string.h>		// memcpy, memmove
#endif


Transfer * alloc_transfer(size_t nr_iso_packets)
{
	Transfer * tmpTransfer;
#ifdef __KERNEL__
	tmpTransfer = usb_alloc_urb((int)nr_iso_packets, GFP_KERNEL);
#else
	tmpTransfer = libusb_alloc_transfer((int)nr_iso_packets);
#endif
	return tmpTransfer;
}

enum usb_error submit_transfer(Transfer * transfer)
{
	int iRet;
	enum usb_error uErr;
	uErr = USB_SUCCESS;
#ifdef __KERNEL__
	iRet = usb_submit_urb(transfer, GFP_KERNEL);
	if ( iRet )
	{
		USBIF_DEBUG_USB("Submitting transfer failed (this urb might still be linked, error code: %d).\n", iRet);
		uErr = USB_ERROR_OTHER;
	}
#else
	iRet = libusb_submit_transfer(transfer);
	if ( iRet == LIBUSB_ERROR_BUSY )
	{
		USBIF_DEBUG_USB("Would re-submit, ignoring instead.\n");
	}
	else if ( iRet )
	{
		USBIF_ERROR("Submitting transfer failed: ");
		if ( iRet == LIBUSB_ERROR_NO_DEVICE )
		{
			USBIF_ERROR("the device has been disconnected.\n");
			uErr = USB_ERROR_NO_DEVICE;
		}
		else
		{
			USBIF_ERROR("generic failure.\n");
			uErr = USB_ERROR_OTHER;
		}
	}
#endif
	return uErr;
}

enum usb_error cancel_transfer(Transfer * transfer)
{
	int iRet;
	enum usb_error uErr = USB_SUCCESS;
#ifdef __KERNEL__
	iRet = usb_unlink_urb(transfer);		//usb_urb_kill is better, no libusb counterpart
	if ( iRet != -EINPROGRESS )
	{
		USBIF_DEBUG_USB("Transfer already completed or canceled.\n");
		uErr = USB_TRANSFER_COMPLETED;
	}
#else
	iRet = libusb_cancel_transfer(transfer);
	if ( iRet == LIBUSB_ERROR_NOT_FOUND )
	{
		USBIF_DEBUG_USB("Transfer already completed or canceled.\n");
		uErr = USB_TRANSFER_COMPLETED;
	}
	else if ( iRet != LIBUSB_SUCCESS )
	{
		USBIF_ERROR("Fatal error canceling a submitted transfer.\n");
		uErr = USB_TRANSFER_FATAL;
	}
#endif
	return uErr;
}

void free_transfer(Transfer * transfer)
{
#ifdef __KERNEL__
		usb_free_urb(transfer);
#else
		libusb_free_transfer(transfer);
#endif
}


void fill_iso_transfer(UsbCom * ucom, Transfer * tranfer,
		uint8_t endpoint_address, int nr_iso_packets, unsigned int iso_packet_length,
		unsigned char * buf, int buf_len,
		void (*callback)(Transfer * transfer), void * context, unsigned int timeout)
{
#ifdef __KERNEL__
	unsigned int pipe;
	int i;
#endif
#ifdef __KERNEL__
	if ( endpoint_address & ENDPOINT_IN )
		pipe = usb_rcvisocpipe(ucom->handle->usb_handler, (endpoint_address & 0x7F) );
	else
		pipe = usb_sndisocpipe(ucom->handle->usb_handler, (endpoint_address & 0x7F) );
	/*TODO*/
	//For some reason, the kernel requires this to be 1
//	tranfer->interval = timeout;
	tranfer->interval = 1;

	tranfer->dev = ucom->handle->usb_handler;
	tranfer->pipe = pipe;
	tranfer->transfer_flags = URB_ISO_ASAP;
	tranfer->transfer_buffer = buf;
	tranfer->transfer_buffer_length = buf_len;
	tranfer->complete = callback;
	tranfer->context = context;
	tranfer->start_frame = 0;
	tranfer->number_of_packets = nr_iso_packets;
	for (i = 0; i < nr_iso_packets; i++)
	{
		tranfer->iso_frame_desc[i].offset = i * iso_packet_length;
		tranfer->iso_frame_desc[i].length = iso_packet_length;
	}
#else
	libusb_fill_iso_transfer(tranfer,
			ucom->handle->usb_handler,
			(unsigned char)endpoint_address,
			buf,
			buf_len,
			nr_iso_packets,
			callback,
			context,
			timeout);

	libusb_set_iso_packet_lengths(tranfer, iso_packet_length);
#endif
}

void fill_int_transfer(UsbCom * ucom, Transfer * tranfer,
		uint8_t endpoint_address,
		uint8_t * buf, size_t len,
		void (*callback)(Transfer * transfer), void * context, unsigned int timeout)
{
#ifdef __KERNEL__
	unsigned int pipe;
#endif
#ifdef __KERNEL__
	if ( endpoint_address & ENDPOINT_IN )
		pipe = usb_rcvintpipe(ucom->handle->usb_handler, (endpoint_address & 0x7F) );
	else
		pipe = usb_sndintpipe(ucom->handle->usb_handler, (endpoint_address & 0x7F) );

	usb_fill_int_urb(tranfer,
					ucom->handle->usb_handler,
					pipe,
					buf,
			(int)	len,
					callback,
					context,
					timeout);
#else
	libusb_fill_interrupt_transfer(tranfer,
								ucom->handle->usb_handler,
			(unsigned char)		endpoint_address,
			(unsigned char*)	buf,
			(int)				len,
								callback,
								context,
								timeout);
#endif
}

size_t transfer_bulk(UsbCom * ucom, uint8_t endpoint_address, uint8_t * buf, size_t len, unsigned int timeout)
{
	int iRet;
	int transferred;
	size_t transferred_length;
#ifdef __KERNEL__
	unsigned int pipe;
	if ( endpoint_address & USB_DIR_IN )
		pipe = usb_rcvbulkpipe(ucom->handle->usb_handler, (endpoint_address & 0x7F) );
	else
		pipe = usb_sndbulkpipe(ucom->handle->usb_handler, (endpoint_address & 0x7F) );

	iRet = usb_bulk_msg(ucom->handle->usb_handler, pipe, buf, (int)len, &transferred, timeout);
	if ( iRet )
	{
		setUsbError(USB_ERROR_IO);
		return RETURNERROR;
	}
#else
	iRet = libusb_bulk_transfer(ucom->handle->usb_handler, (unsigned char)endpoint_address, buf, (int)len, &transferred, timeout);

	if ( iRet )
	{
		if ( iRet == LIBUSB_ERROR_TIMEOUT )
			USBIF_INFO("Tranferred timed out, data has been transferred nonetheless.\n");
		else
		{
			USBIF_ERROR("Bulk transfer failed: ");
			if ( iRet == LIBUSB_ERROR_PIPE )
			{
				USBIF_ERROR("the endpoint halted.\n");
				setUsbError(USB_ERROR_PIPE);
			}
			else if ( iRet == LIBUSB_ERROR_OVERFLOW )
			{
				USBIF_ERROR("the device tried to transfer more data.\n");
				setUsbError(USB_ERROR_OVERFLOW);
			}
			else if ( iRet == LIBUSB_ERROR_NO_DEVICE )
			{
				USBIF_ERROR("the device has been disconnected.\n");
				setUsbError(USB_ERROR_NO_DEVICE);
			}
			else
			{
				USBIF_ERROR("generic failure.\n");
				setUsbError(USB_ERROR_OTHER);
			}
		}
		if ( iRet != LIBUSB_ERROR_TIMEOUT )
			return RETURNERROR;
	}
#endif
	//no check required since libusb_bulk_transfer never returns more than len and len is of type size_t
	transferred_length = (size_t)transferred;
	return transferred_length;
}


unsigned char * get_transfer_buffer(Transfer * transfer)
{
#ifdef __KERNEL__
	return transfer->transfer_buffer;
#else
	return transfer->buffer;
#endif
}

int get_iso_number_of_packets(Transfer * transfer)
{
#ifdef __KERNEL__
	return transfer->number_of_packets;
#else
	return transfer->num_iso_packets;
#endif
}

IsoPacket * get_iso_packet(Transfer * transfer, int i)
{
#ifdef __KERNEL__
	return &transfer->iso_frame_desc[i];
#else
	return &transfer->iso_packet_desc[i];
#endif
}


enum usb_error get_status(Transfer * transfer)
{
#ifdef __KERNEL__
	if ( transfer->status == -EINPROGRESS || transfer->status == 0 )
		return USB_SUCCESS;
	else if ( transfer->status == -ECONNRESET )
		return USB_TRANSFER_CANCELED;
#else
	if ( transfer->status == LIBUSB_TRANSFER_COMPLETED )
		return USB_SUCCESS;
	else if ( transfer->status == LIBUSB_TRANSFER_CANCELLED )
		return USB_TRANSFER_CANCELED;
#endif
	else
		return USB_ERROR_IO;
}

unsigned char * get_iso_packet_buffer(Transfer * transfer, int i)
{
	unsigned char * buf;
#ifdef __KERNEL__
	buf = get_transfer_buffer(transfer) + get_iso_packet(transfer, i)->offset;
#else
	buf = libusb_get_iso_packet_buffer(transfer, i);
#endif
	return buf;
}

void * get_context(Transfer * transfer)
{
#ifdef __KERNEL__
	return transfer->context;
#else
	return transfer->user_data;
#endif
}


void compact_packet(Transfer * transfer)
{
	int i;
	unsigned char * iso_buf = get_transfer_buffer(transfer);
	unsigned int iso_len = 0;

	unsigned char * packet_buf = NULL;
	int packet_len = 0;

	USBIF_DEBUG_ISOPROC("Compacting iso transfers to buffer beginning from 0 to complete transfer length.\n");
	//compact the data
	if ( get_status(transfer) == USB_SUCCESS )
	{
		for (i = 0; i < get_iso_number_of_packets(transfer); i++)
		{
			if ( get_iso_packet(transfer, i)->status == 0 )
			{
				packet_buf = get_iso_packet_buffer(transfer, i);
				packet_len = get_iso_packet(transfer, i)->actual_length;

				if ( packet_len > 0 )
				{
					if ( i )
						memmove(iso_buf, packet_buf, packet_len);

					iso_buf += packet_len;
					iso_len += packet_len;
				}
			}
		}
	}
}

size_t get_iso_length(Transfer * transfer, size_t * p_packet_len)
{
	size_t len;
	int i;
	for ( i = 0, len = 0; i < get_iso_number_of_packets(transfer); i++)
	{
		p_packet_len[i] = get_iso_packet(transfer, i)->actual_length;
		len += get_iso_packet(transfer, i)->actual_length;
	}
	USBIF_DEBUG_ISOPROC("Iso transfers complete length %u.\n", (unsigned int)len);
	return len;
}

