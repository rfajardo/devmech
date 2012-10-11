/*
 * streamif.h
 *
 *  Created on: Jul 6, 2011
 *      Author: raul
 */

#ifndef STREAMIF_H_
#define STREAMIF_H_

#include "usbdata.h"
#include "usbif.h"
#include "com.h"

enum cb_ret
{
	FINISH = 0,
	REACTIVATE
};

//RETURNERROR is a return value which indicates a possible error
//this is used when a the return value is otherwise meaningful for the application
void setUsbError(enum usb_error err);
enum usb_error getUsbError(void);

//bulk transfers
size_t get_stream(UsbCom * ucom, BulkEndpoint * endpoint, uint8_t * buf, size_t len);
size_t set_stream(UsbCom * ucom, BulkEndpoint * endpoint, uint8_t * buf, size_t len);

void set_stream_timeout(UsbCom * ucom, BulkEndpoint * endpoint, uint32_t timeout);

//interrupt endpoint
enum usb_error register_interrupt_buffer(UsbCom * ucom, IntEndpoint * endpoint, uint8_t * buf, size_t len);
enum usb_error unregister_interrupt_buffers(UsbCom * ucom, IntEndpoint * endpoint);

enum usb_error register_interrupt(UsbCom * ucom, IntEndpoint * endpoint, enum cb_ret (*func)(void * context, uint8_t * buf, size_t len, size_t * p_packet_len, size_t nr_of_packets), void * context);
enum usb_error unregister_interrupt(UsbCom * ucom, IntEndpoint * endpoint);

enum usb_error unlock_interrupt(UsbCom * ucom, IntEndpoint * endpoint);

void set_interrupt_timeout(UsbCom * ucom, IntEndpoint * endpoint, uint32_t timeout);

//for isochronous endpoint (combined with interrupt)
enum usb_error register_dma_space(UsbCom * ucom, IsoEndpoint * endpoint, uint8_t * buf, size_t len);
enum usb_error unregister_dma_spaces(UsbCom * ucom, IsoEndpoint * endpoint);

enum usb_error register_dma_interrupt(UsbCom * ucom, IsoEndpoint * endpoint, enum cb_ret (*func)(void * context, uint8_t * buf, size_t len, size_t * p_packet_len, size_t nr_of_packets), void * context);
enum usb_error unregister_dma_interrupt(UsbCom * ucom, IsoEndpoint * endpoint);

enum usb_error unlock_dma_spaces(UsbCom * ucom, IsoEndpoint * endpoint);

void set_dma_timeout(UsbCom * ucom, IsoEndpoint * endpoint, uint32_t timeout);


#endif /* STREAMIF_H_ */
