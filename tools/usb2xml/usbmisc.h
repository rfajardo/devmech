/*****************************************************************************/
/*
 *      usbmisc.h  --  Misc USB routines
 *
 *      Copyright (C) 2003  Aurelien Jarno (aurelien@aurel32.net)
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 */

/*****************************************************************************/

#ifndef _USBMISC_H
#define _USBMISC_H

#include <libusb.h>


#define le16_to_cpu(x) libusb_cpu_to_le16(libusb_cpu_to_le16(x))
unsigned int convert_le_u32 (const unsigned char *buf);

/* ---------------------------------------------------------------------- */

/* workaround libusb API goofs:  "byte" should never be sign extended;
 * using "char" is trouble.  Likewise, sizes should never be negative.
 */

inline int typesafe_control_msg(libusb_device_handle *dev,
	unsigned char requesttype, unsigned char request,
	int value, int idx,
	unsigned char *bytes, unsigned size, int timeout);

#define usb_control_msg		typesafe_control_msg

/* ---------------------------------------------------------------------- */

extern libusb_device *get_usb_device(libusb_context *ctx, const char *path);

/* ---------------------------------------------------------------------- */
#endif /* _USBMISC_H */
