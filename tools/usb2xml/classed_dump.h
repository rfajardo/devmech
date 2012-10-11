/*
 * classed_dump.h
 *
 *  Created on: Jun 20, 2011
 *      Author: rfajardo
 */

#ifndef CLASSED_DUMP_H_
#define CLASSED_DUMP_H_

#include <libusb.h>

/* Conventional codes for class-specific descriptors.  The convention is
 * defined in the USB "Common Class" Spec (3.11).  Individual class specs
 * are authoritative for their usage, not the "common class" writeup.
 */
#define USB_DT_CS_DEVICE		(LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_DT_DEVICE)
#define USB_DT_CS_CONFIG		(LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_DT_CONFIG)
#define USB_DT_CS_STRING		(LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_DT_STRING)
#define USB_DT_CS_INTERFACE		(LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_DT_INTERFACE)
#define USB_DT_CS_ENDPOINT		(LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_DT_ENDPOINT)

#ifndef USB_CLASS_CCID
#define USB_CLASS_CCID			0x0b
#endif

#ifndef USB_CLASS_VIDEO
#define USB_CLASS_VIDEO			0x0e
#endif

#ifndef USB_CLASS_APPLICATION
#define USB_CLASS_APPLICATION	       	0xfe
#endif

#ifndef USB_AUDIO_CLASS_1
#define USB_AUDIO_CLASS_1		0x00
#endif

#ifndef USB_AUDIO_CLASS_2
#define USB_AUDIO_CLASS_2		0x20
#endif

#define	HUB_STATUS_BYTELEN	3	/* max 3 bytes status = hub + 23 ports */


void dump_audiocontrol_interface(libusb_device_handle *dev, const unsigned char *buf, int protocol);
void dump_audiostreaming_interface(libusb_device_handle *dev, const unsigned char *buf, int protocol);
void dump_midistreaming_interface(libusb_device_handle *dev, const unsigned char *buf);
void dump_videocontrol_interface(libusb_device_handle *dev, const unsigned char *buf);
void dump_videostreaming_interface(const unsigned char *buf);
void dump_dfu_interface(const unsigned char *buf);
char *dump_comm_descriptor(libusb_device_handle *dev, const unsigned char *buf, char *indent);
void dump_hid_device(libusb_device_handle *dev, const struct libusb_interface_descriptor *interface, const unsigned char *buf);
void dump_audiostreaming_endpoint(const unsigned char *buf, int protocol);
void dump_midistreaming_endpoint(const unsigned char *buf);
void dump_hub(const char *prefix, const unsigned char *p, int tt_type);
void dump_ccid_device(const unsigned char *buf);

void dump_rc_interface(const unsigned char *buf);
void dump_wire_adapter(const unsigned char *buf);


#endif /* CLASSED_DUMP_H_ */
