/*****************************************************************************/

/*
 *      lsusb.c  --  lspci like utility for the USB bus
 *
 *      Copyright (C) 1999-2001, 2003
 *        Thomas Sailer (t.sailer@alumni.ethz.ch)
 *      Copyright (C) 2003-2005 David Brownell
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
 */

/*****************************************************************************/

#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>

#ifdef HAVE_BYTESWAP_H
#include <byteswap.h>
#endif

#include <libusb.h>
#include <unistd.h>

#include <libxml/xmlwriter.h>

#include "lsusb.h"

#include "names.h"
#include "devtree.h"
#include "usbmisc.h"

#include "getusb.h"
#include "dump.h"
#include "xml.h"



extern int lsusb_t(void);
static const char procbususb[] = "/proc/bus/usb";
static unsigned int verblevel = VERBLEVEL_DEFAULT;


int lprintf(unsigned int vl, const char *format, ...)
{
	va_list ap;
	int r;

	if (vl > verblevel)
		return 0;
	va_start(ap, format);
	r = vfprintf(stderr, format, ap);
	va_end(ap);
	if (!vl)
		exit(1);
	return r;
}



/* ---------------------------------------------------------------------- */

static int dump_one_device(libusb_context *ctx, const char *path, xmlTextWriterPtr xml_writer)
{
	libusb_device *dev;
	struct libusb_device_descriptor desc;
	char vendor[128], product[128];

	dev = get_usb_device(ctx, path);
	if (!dev) {
		fprintf(stderr, "Cannot open %s\n", path);
		return 1;
	}
	libusb_get_device_descriptor(dev, &desc);
	get_vendor_string(vendor, sizeof(vendor), desc.idVendor);
	get_product_string(product, sizeof(product), desc.idVendor, desc.idProduct);
	printf("Device: ID %04x:%04x %s %s\n", desc.idVendor,
					       desc.idProduct,
					       vendor,
					       product);
	if (verblevel > 0)
	{
		dumpdev(dev);
		if (xml_writer)
			xmldev(dev, xml_writer);
	}
	return 0;
}

static int list_devices(libusb_context *ctx, int busnum, int devnum, int vendorid, int productid, xmlTextWriterPtr xml_writer)
{
	libusb_device **list;
	struct libusb_device_descriptor desc;
	char vendor[128], product[128];
	int status;
	ssize_t num_devs, i;

	status = 1; /* 1 device not found, 0 device found */

	num_devs = libusb_get_device_list(ctx, &list);

	for (i = 0; i < num_devs; ++i) {
		libusb_device *dev = list[i];
		uint8_t bnum = libusb_get_bus_number(dev);
		uint8_t dnum = libusb_get_device_address(dev);

		if ((busnum != -1 && busnum != bnum) ||
		    (devnum != -1 && devnum != dnum))
			continue;
		libusb_get_device_descriptor(dev, &desc);
		if ((vendorid != -1 && vendorid != desc.idVendor) ||
		    (productid != -1 && productid != desc.idProduct))
			continue;
		status = 0;
		get_vendor_string(vendor, sizeof(vendor), desc.idVendor);
		get_product_string(product, sizeof(product),
				desc.idVendor, desc.idProduct);
		if (verblevel > 0)
			printf("\n");
		printf("Bus %03u Device %03u: ID %04x:%04x %s %s\n",
				bnum, dnum,
				desc.idVendor,
				desc.idProduct,
				vendor, product);
		if (verblevel > 0)
		{
			dumpdev(dev);
			if (xml_writer)
				xmldev(dev, xml_writer);
		}
	}

	libusb_free_device_list(list, 0);
	return status;
}

/* ---------------------------------------------------------------------- */

void devtree_busconnect(struct usbbusnode *bus)
{
	bus = bus;	/* reduce compiler warnings */
}

void devtree_busdisconnect(struct usbbusnode *bus)
{
	bus = bus;	/* reduce compiler warnings */
}

void devtree_devconnect(struct usbdevnode *dev)
{
	dev = dev;	/* reduce compiler warnings */
}

void devtree_devdisconnect(struct usbdevnode *dev)
{
	dev = dev;	/* reduce compiler warnings */
}

static int treedump(void)
{
	int fd;
	char buf[512];

	snprintf(buf, sizeof(buf), "%s/devices", procbususb);
	if (access(buf, R_OK) < 0)
		return lsusb_t();
	if ((fd = open(buf, O_RDONLY)) == -1) {
		fprintf(stderr, "cannot open %s, %s (%d)\n", buf, strerror(errno), errno);
		return 1;
	}
	devtree_parsedevfile(fd);
	close(fd);
	devtree_dump();
	return 0;
}

/* ---------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
	static const struct option long_options[] = {
		{ "version", 0, 0, 'V' },
		{ "verbose", 0, 0, 'v' },
		{ 0, 0, 0, 0 }
	};
	libusb_context *ctx;
	int c, err = 0;
	unsigned int allowctrlmsg = 0, treemode = 0;
	int bus = -1, devnum = -1, vendor = -1, product = -1;
	const char *devdump = NULL;
	char *cp;
	int status;

	char * xml_filename = NULL;

	xmlTextWriterPtr xml_writer = NULL;
	xmlDocPtr xml_doc;
	int rc;

	while ((c = getopt_long(argc, argv, "D:o:vxtP:p:s:d:V",
			long_options, NULL)) != EOF) {
		switch (c) {
		case 'V':
			printf("lsusb (" PACKAGE ") " VERSION "\n");
			exit(0);

		case 'o':
			if (*optarg)
			{
				xml_filename = malloc(strlen(optarg));
				strcpy(xml_filename, optarg);
			}
			else
			{
				printf("A filename is required\n");
				exit(0);
			}
			break;

		case 'v':
			verblevel++;
			break;

		case 'x':
			allowctrlmsg = 1;
			break;

		case 't':
			treemode = 1;
			break;

		case 's':
			cp = strchr(optarg, ':');
			if (cp) {
				*cp++ = 0;
				if (*optarg)
					bus = strtoul(optarg, NULL, 10);
				if (*cp)
					devnum = strtoul(cp, NULL, 10);
			} else {
				if (*optarg)
					devnum = strtoul(optarg, NULL, 10);
			}
			break;

		case 'd':
			cp = strchr(optarg, ':');
			if (!cp) {
				err++;
				break;
			}
			*cp++ = 0;
			if (*optarg)
				vendor = strtoul(optarg, NULL, 16);
			if (*cp)
				product = strtoul(cp, NULL, 16);
			break;

		case 'D':
			devdump = optarg;
			break;

		case '?':
		default:
			err++;
			break;
		}
	}
	if (err || argc > optind) {
		fprintf(stderr, "Usage: lsusb [options]...\n"
			"List USB devices\n"
			"  -v, --verbose\n"
			"      Increase verbosity (show descriptors)\n"
			"  -s [[bus]:][devnum]\n"
			"      Show only devices with specified device and/or\n"
			"      bus numbers (in decimal)\n"
			"  -d vendor:[product]\n"
			"      Show only devices with the specified vendor and\n"
			"      product ID numbers (in hexadecimal)\n"
			"  -D device\n"
			"      Selects which device lsusb will examine\n"
			"  -t\n"
			"      Dump the physical USB device hierarchy as a tree\n"
			"  -V, --version\n"
			"      Show version of program\n"
			"  -o filename\n"
			"      XML file output\n"
			);
		exit(1);
	}

	/* by default, print names as well as numbers */
	err = names_init(DATADIR "/usb.ids");
	if (err != 0)
		fprintf(stderr, "%s: cannot open \"%s\", %s\n",
				argv[0],
				DATADIR "/usb.ids",
				strerror(err));

	if (xml_filename)
	{
		xmlKeepBlanksDefault(0);		//only necessary if we are parsing another document and
										//want to remove indentation to add it back later
										//it does not create any problems to let it here though
		xml_writer = xmlNewTextWriterDoc(&xml_doc, 0);
	    if (xml_writer == NULL)
	    {
	    	fprintf(stderr,
	    			"testXmlwriterFilename: Error creating the xml writer for filename: %s\n",
	    			xml_filename);
	    	exit(EXIT_FAILURE);
	    }
	    rc = xmlTextWriterStartDocument(xml_writer, "1.0", "UTF-8", NULL);
	    if (rc < 0) {
	        fprintf(stderr,"testXmlwriterFilename: Error at xmlTextWriterStartDocument\n");
	        exit(EXIT_FAILURE);
	    }
	    rc = xmlTextWriterStartElementNS(xml_writer, "udev", "usbCom", "http://www.ziti.uni-heidelberg.de/XMLSchemas/usbDevice");
	    if (rc < 0) {
	        fprintf(stderr, "testXmlwriterDoc: Error at xmlTextWriterStartElement\n");
	        exit(EXIT_FAILURE);
	    }
	    rc = xmlTextWriterWriteAttributeNS(xml_writer, "xsi", "schemaLocation", "http://www.w3.org/2001/XMLSchema-instance", "http://www.ziti.uni-heidelberg.de/XMLSchemas/usbDevice ./USB/usbStandard.xsd");
	    if (rc < 0)
	    {
	    	fprintf(stderr, "textXmlwriterDoc: Error at xmlTextWriterStartAttributeNS\n");
	    }
	    rc = xmlTextWriterStartElement(xml_writer, "udev:standardDescriptors");
	    if (rc < 0) {
	        fprintf(stderr, "testXmlwriterDoc: Error at xmlTextWriterStartElement\n");
	        exit(EXIT_FAILURE);
	    }
	}

	status = 0;

	err = libusb_init(&ctx);
	if (err) {
		fprintf(stderr, "unable to initialize libusb: %i\n", err);
		return EXIT_FAILURE;
	}

	if (treemode) {
		/* treemode requires at least verblevel 1 */
		verblevel += 1 - VERBLEVEL_DEFAULT;
		status = treedump();
	} else if (devdump)
		status = dump_one_device(ctx, devdump, xml_writer);
	else
		status = list_devices(ctx, bus, devnum, vendor, product, xml_writer);

	if (xml_filename)
	{
		rc = xmlTextWriterEndDocument(xml_writer);
		if (rc < 0)
		{
			fprintf(stderr, "testXmlwriterFilename: Error at xmlTextWriterEndDocument\n");
		}

		xmlFreeTextWriter(xml_writer);

		xmlKeepBlanksDefault(1);
		xmlSaveFormatFile(xml_filename, xml_doc, 1);

		xmlFreeDoc(xml_doc);

		xmlCleanupParser();

		free(xml_filename);
	}

	names_exit();
	libusb_exit(ctx);
	return status;
}
