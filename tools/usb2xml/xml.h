/*
 * xml.h
 *
 *  Created on: Jun 24, 2011
 *      Author: raul
 */

#ifndef XML_H_
#define XML_H_

#include <libusb.h>

void xmldev(libusb_device *dev, xmlTextWriterPtr xml_writer);

#endif /* XML_H_ */
