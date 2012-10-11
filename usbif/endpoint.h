/*
 * endpoint.h
 *
 *  Created on: Jul 21, 2012
 *      Author: raul
 */

#ifndef ENDPOINT_H_
#define ENDPOINT_H_

#include "com.h"
#include "transfer.h"
#include "locks.h"

struct endpoint
{
	EndpointDesc *					p_endpoint_desc;
	uint32_t						timeout;

	SPINLOCK 						lock;

	List * 							pending_transfers;
	List * 							complete_transfers;
};
typedef struct endpoint Endpoint;


Transfer * add_endpoint_iso_transfer(UsbCom * ucom, Endpoint * endpoint, int nr_iso_packets, unsigned int iso_packet_length,
		unsigned char * buf, int buf_len,
		void (*callback)(Transfer * transfer), void * context);
Transfer * add_endpoint_int_transfer(UsbCom * ucom, Endpoint * endpoint,
		unsigned char * buf, int buf_len,
		void (*callback)(Transfer * transfer), void * context);

size_t endpoint_transfer_bulk(UsbCom * ucom, Endpoint * endpoint, uint8_t * buf, size_t len);


enum usb_error unregister_endpoint_transfers(Endpoint * endpoint);

enum usb_error trigger_endpoint_transfers(UsbDevice * udev, Endpoint * endpoint);
enum usb_error cancel_endpoint_transfers(Endpoint * endpoint);

void transfer_complete(Endpoint * endpoint, Transfer * transfer);
void remove_transfer(Endpoint * endpoint, Transfer * transfer);


#endif /* ENDPOINT_H_ */
