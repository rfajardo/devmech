/*
 * transfer.h
 *
 *  Created on: Jul 21, 2012
 *      Author: raul
 */

#ifndef TRANSFER_H_
#define TRANSFER_H_

#include "com.h"
#include <devif/types.h>

#ifdef __KERNEL__
typedef struct urb Transfer;
typedef struct usb_iso_packet_descriptor IsoPacket;
#else
typedef struct libusb_transfer Transfer;
typedef struct libusb_iso_packet_descriptor IsoPacket;
#endif


Transfer * alloc_transfer(size_t nr_iso_packets);
enum usb_error submit_transfer(Transfer * transfer);
enum usb_error cancel_transfer(Transfer * transfer);
void free_transfer(Transfer * transfer);


void fill_iso_transfer(UsbCom * ucom, Transfer * tranfer,
		uint8_t endpoint_address, int nr_iso_packets, unsigned int iso_packet_length,
		unsigned char * buf, int buf_len,
		void (*callback)(Transfer * transfer), void * context, unsigned int timeout);
void fill_int_transfer(UsbCom * ucom, Transfer * tranfer,
		uint8_t endpoint_address,
		uint8_t * buf, size_t len,
		void (*callback)(Transfer * transfer), void * context, unsigned int timeout);

size_t transfer_bulk(UsbCom * ucom, uint8_t endpoint_address, uint8_t * buf, size_t len, unsigned int timeout);


unsigned char * get_transfer_buffer(Transfer * transfer);
int get_iso_number_of_packets(Transfer * transfer);
IsoPacket * get_iso_packet(Transfer * transfer, int i);

enum usb_error get_status(Transfer * transfer);
unsigned char * get_iso_packet_buffer(Transfer * transfer, int i);
void * get_context(Transfer * transfer);

//helpers
void compact_packet(Transfer * transfer);
size_t get_iso_length(Transfer * transfer, size_t * p_packet_len);


#endif /* TRANSFER_H_ */
