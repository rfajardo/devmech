/*
 * getusb.h
 *
 *  Created on: Jun 20, 2011
 *      Author: rfajardo
 */

#ifndef GETUSB_H_
#define GETUSB_H_


int get_string(libusb_device_handle *dev, char *buf, size_t size, u_int8_t id);
int get_vendor_string(char *buf, size_t size, u_int16_t vid);
int get_product_string(char *buf, size_t size, u_int16_t vid, u_int16_t pid);
int get_class_string(char *buf, size_t size, u_int8_t cls);
int get_subclass_string(char *buf, size_t size, u_int8_t cls, u_int8_t subcls);
int get_protocol_string(char *buf, size_t size, u_int8_t cls, u_int8_t subcls, u_int8_t proto);
int get_audioterminal_string(char *buf, size_t size, u_int16_t termt);
int get_videoterminal_string(char *buf, size_t size, u_int16_t termt);
const char *get_guid(const unsigned char *buf);


#endif /* GETUSB_H_ */
