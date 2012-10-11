/*
 * acquire_data_if.h
 *
 *  Created on: Apr 15, 2010
 *      Author: rfajardo
 */

#ifndef ACQUIRE_DATA_IF_H_
#define ACQUIRE_DATA_IF_H_

#include <stdint.h>
#include <stddef.h>

struct devFunc;
struct buffering;
struct pwcmech;
enum cb_ret;

struct acqArg
{
	int * abort;
	struct pwcmech * pwcmech;
};

void process_usb(void * acq_arg);
enum cb_ret acquire_frame(void * context, uint8_t * buf, size_t len, size_t * p_packet_len, size_t nr_of_packets);


#endif /* ACQUIRE_DATA_IF_H_ */
