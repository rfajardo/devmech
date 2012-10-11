/*
 * std_dump.h
 *
 *  Created on: Jun 20, 2011
 *      Author: rfajardo
 */

#ifndef STD_DUMP_H_
#define STD_DUMP_H_

#include <libusb.h>

/* from USB 2.0 spec and updates */
#define USB_DT_DEVICE_QUALIFIER		0x06
#define USB_DT_OTHER_SPEED_CONFIG	0x07
#define USB_DT_OTG			0x09
#define USB_DT_DEBUG			0x0a
#define USB_DT_INTERFACE_ASSOCIATION	0x0b
#define USB_DT_SECURITY			0x0c
#define USB_DT_KEY			0x0d
#define USB_DT_ENCRYPTION_TYPE		0x0e
#define USB_DT_BOS			0x0f
#define USB_DT_DEVICE_CAPABILITY	0x10
#define USB_DT_WIRELESS_ENDPOINT_COMP	0x11
#define USB_DT_WIRE_ADAPTER		0x21
#define USB_DT_RPIPE			0x22
#define USB_DT_RC_INTERFACE		0x23
#define USB_DT_SS_ENDPOINT_COMP		0x30

/* Device Capability Type Codes (Wireless USB spec and USB 3.0 bus spec) */
#define USB_DC_WIRELESS_USB		0x01
#define USB_DC_20_EXTENSION		0x02
#define USB_DC_SUPERSPEED		0x03
#define USB_DC_CONTAINER_ID		0x04

void dump_bytes(const unsigned char *buf, unsigned int len);
void dump_junk(const unsigned char *buf, const char *indent, unsigned int len);
void dump_unit(unsigned int data, unsigned int len);


void dump_config(libusb_device_handle *dev, struct libusb_config_descriptor *config);
void dump_device(
	libusb_device_handle *dev,
	struct libusb_device_descriptor *descriptor
);
void dump_device_status(libusb_device_handle *fd, int otg, int wireless);

//special stuff
void dump_bos_descriptor(libusb_device_handle *fd);


#endif /* STD_DUMP_H_ */
