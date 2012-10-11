/*
 * getusb.c
 *
 *  Created on: Jun 20, 2011
 *      Author: rfajardo
 */

#include <sys/types.h>
#include <stdio.h>

#include <libusb.h>

#include "getusb.h"
#include "names.h"


int get_string(libusb_device_handle *dev, char *buf, size_t size, u_int8_t id)
{
	int ret;

	if (!dev) {
		buf[0] = 0;
		return 0;
	}

	if (id) {
		ret = libusb_get_string_descriptor_ascii(dev, id, (void *)buf, size);
		if (ret <= 0) {
			buf[0] = 0;
			return 0;
		} else
			return ret;

	} else {
		buf[0] = 0;
		return 0;
	}
}

int get_vendor_string(char *buf, size_t size, u_int16_t vid)
{
	const char *cp;

	if (size < 1)
		return 0;
	*buf = 0;
	if (!(cp = names_vendor(vid)))
		return 0;
	return snprintf(buf, size, "%s", cp);
}

int get_product_string(char *buf, size_t size, u_int16_t vid, u_int16_t pid)
{
	const char *cp;

	if (size < 1)
		return 0;
	*buf = 0;
	if (!(cp = names_product(vid, pid)))
		return 0;
	return snprintf(buf, size, "%s", cp);
}

int get_class_string(char *buf, size_t size, u_int8_t cls)
{
	const char *cp;

	if (size < 1)
		return 0;
	*buf = 0;
	if (!(cp = names_class(cls)))
		return 0;
	return snprintf(buf, size, "%s", cp);
}

int get_subclass_string(char *buf, size_t size, u_int8_t cls, u_int8_t subcls)
{
	const char *cp;

	if (size < 1)
		return 0;
	*buf = 0;
	if (!(cp = names_subclass(cls, subcls)))
		return 0;
	return snprintf(buf, size, "%s", cp);
}

int get_protocol_string(char *buf, size_t size, u_int8_t cls, u_int8_t subcls, u_int8_t proto)
{
	const char *cp;

	if (size < 1)
		return 0;
	*buf = 0;
	if (!(cp = names_protocol(cls, subcls, proto)))
		return 0;
	return snprintf(buf, size, "%s", cp);
}

int get_audioterminal_string(char *buf, size_t size, u_int16_t termt)
{
	const char *cp;

	if (size < 1)
		return 0;
	*buf = 0;
	if (!(cp = names_audioterminal(termt)))
		return 0;
	return snprintf(buf, size, "%s", cp);
}

int get_videoterminal_string(char *buf, size_t size, u_int16_t termt)
{
	const char *cp;

	if (size < 1)
		return 0;
	*buf = 0;
	if (!(cp = names_videoterminal(termt)))
		return 0;
	return snprintf(buf, size, "%s", cp);
}

const char *get_guid(const unsigned char *buf)
{
	static char guid[39];

	/* NOTE:  see RFC 4122 for more information about GUID/UUID
	 * structure.  The first fields fields are historically big
	 * endian numbers, dating from Apollo mc68000 workstations.
	 */
	sprintf(guid, "{%02x%02x%02x%02x"
			"-%02x%02x"
			"-%02x%02x"
			"-%02x%02x"
			"-%02x%02x%02x%02x%02x%02x}",
	       buf[0], buf[1], buf[2], buf[3],
	       buf[4], buf[5],
	       buf[6], buf[7],
	       buf[8], buf[9],
	       buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
	return guid;
}
