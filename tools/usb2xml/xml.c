/*
 * xmlout.c
 *
 *  Created on: Jun 20, 2011
 *      Author: rfajardo
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <libusb.h>

#include <libxml/xmlwriter.h>

#include "usbmisc.h"
#include "lsusb.h"

#include "getusb.h"
#include "xml.h"
#include "std_xml.h"
#include "classed_xml.h"


/* ---------------------------------------------------------------------- */

static void xml_do_hub(libusb_device_handle *fd, unsigned tt_type, unsigned speed)
{
	unsigned char buf[7 /* base descriptor */
			+ 2 /* bitmasks */ * HUB_STATUS_BYTELEN];
	int i, ret, value;
	unsigned int link_state;
	char *link_state_descriptions[] = {
		" U0",
		" U1",
		" U2",
		" suspend",
		" SS.disabled",
		" Rx.Detect",
		" SS.Inactive",
		" Polling",
		" Recovery",
		" Hot Reset",
		" Compliance",
		" Loopback",
	};

	/* USB 3.0 hubs have a slightly different descriptor */
	if (speed == 0x0300)
		value = 0x2A;
	else
		value = 0x29;

	ret = usb_control_msg(fd,
			LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_DEVICE,
			LIBUSB_REQUEST_GET_DESCRIPTOR,
			value << 8, 0,
			buf, sizeof buf, CTRL_TIMEOUT);
	if (ret < 9 /* at least one port's bitmasks */) {
		if (ret >= 0)
			fprintf(stderr,
				"incomplete hub descriptor, %d bytes\n",
				ret);
		/* Linux returns EHOSTUNREACH for suspended devices */
		else if (errno != EHOSTUNREACH)
			perror("can't get hub descriptor");
		return;
	}
	xml_hub("", buf, tt_type);

	printf(" Hub Port Status:\n");
	for (i = 0; i < buf[2]; i++) {
		unsigned char status[4];

		ret = usb_control_msg(fd,
				LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS
					| LIBUSB_RECIPIENT_OTHER,
				LIBUSB_REQUEST_GET_STATUS,
				0, i + 1,
				status, sizeof status,
				CTRL_TIMEOUT);
		if (ret < 0) {
			fprintf(stderr,
				"cannot read port %d status, %s (%d)\n",
				i + 1, strerror(errno), errno);
			break;
		}

		printf("   Port %d: %02x%02x.%02x%02x", i + 1,
			status[3], status[2],
			status[1], status[0]);
		/* CAPS are used to highlight "transient" states */
		if (speed != 0x0300) {
			printf("%s%s%s%s%s",
					(status[2] & 0x10) ? " C_RESET" : "",
					(status[2] & 0x08) ? " C_OC" : "",
					(status[2] & 0x04) ? " C_SUSPEND" : "",
					(status[2] & 0x02) ? " C_ENABLE" : "",
					(status[2] & 0x01) ? " C_CONNECT" : "");
			printf("%s%s%s%s%s%s%s%s%s%s\n",
					(status[1] & 0x10) ? " indicator" : "",
					(status[1] & 0x08) ? " test" : "",
					(status[1] & 0x04) ? " highspeed" : "",
					(status[1] & 0x02) ? " lowspeed" : "",
					(status[1] & 0x01) ? " power" : "",
					(status[0] & 0x10) ? " RESET" : "",
					(status[0] & 0x08) ? " oc" : "",
					(status[0] & 0x04) ? " suspend" : "",
					(status[0] & 0x02) ? " enable" : "",
					(status[0] & 0x01) ? " connect" : "");
		} else {
			link_state = ((status[0] & 0xe0) >> 5) +
				((status[1] & 0x1) << 4);
			printf("%s%s%s%s%s%s",
					(status[2] & 0x80) ? " C_CONFIG_ERROR" : "",
					(status[2] & 0x40) ? " C_LINK_STATE" : "",
					(status[2] & 0x20) ? " C_BH_RESET" : "",
					(status[2] & 0x10) ? " C_RESET" : "",
					(status[2] & 0x08) ? " C_OC" : "",
					(status[2] & 0x01) ? " C_CONNECT" : "");
			printf("%s%s",
					((status[1] & 0x1C) == 0) ? " 5Gbps" : " Unknown Speed",
					(status[1] & 0x02) ? " power" : "");
			/* Link state is bits 8:5 */
			if (link_state < (sizeof(link_state_descriptions) /
						sizeof(*link_state_descriptions)))
				printf("%s", link_state_descriptions[link_state]);
			printf("%s%s%s%s\n",
					(status[0] & 0x10) ? " RESET" : "",
					(status[0] & 0x08) ? " oc" : "",
					(status[0] & 0x02) ? " enable" : "",
					(status[0] & 0x01) ? " connect" : "");
		}
	}
}

static void xml_do_dualspeed(libusb_device_handle *fd)
{
	unsigned char buf[10];
	char cls[128], subcls[128], proto[128];
	int ret;

	ret = usb_control_msg(fd,
			LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE,
			LIBUSB_REQUEST_GET_DESCRIPTOR,
			USB_DT_DEVICE_QUALIFIER << 8, 0,
			buf, sizeof buf, CTRL_TIMEOUT);
	if (ret < 0 && errno != EPIPE)
		perror("can't get device qualifier");

	/* all dual-speed devices have a qualifier */
	if (ret != sizeof buf
			|| buf[0] != ret
			|| buf[1] != USB_DT_DEVICE_QUALIFIER)
		return;

	get_class_string(cls, sizeof(cls),
			buf[4]);
	get_subclass_string(subcls, sizeof(subcls),
			buf[4], buf[5]);
	get_protocol_string(proto, sizeof(proto),
			buf[4], buf[5], buf[6]);
	printf("Device Qualifier (for other device speed):\n"
	       "  bLength             %5u\n"
	       "  bDescriptorType     %5u\n"
	       "  bcdUSB              %2x.%02x\n"
	       "  bDeviceClass        %5u %s\n"
	       "  bDeviceSubClass     %5u %s\n"
	       "  bDeviceProtocol     %5u %s\n"
	       "  bMaxPacketSize0     %5u\n"
	       "  bNumConfigurations  %5u\n",
	       buf[0], buf[1],
	       buf[3], buf[2],
	       buf[4], cls,
	       buf[5], subcls,
	       buf[6], proto,
	       buf[7], buf[8]);

	/* FIXME also show the OTHER_SPEED_CONFIG descriptors */
}

static void xml_do_debug(libusb_device_handle *fd)
{
	unsigned char buf[4];
	int ret;

	ret = usb_control_msg(fd,
			LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE,
			LIBUSB_REQUEST_GET_DESCRIPTOR,
			USB_DT_DEBUG << 8, 0,
			buf, sizeof buf, CTRL_TIMEOUT);
	if (ret < 0 && errno != EPIPE)
		perror("can't get debug descriptor");

	/* some high speed devices are also "USB2 debug devices", meaning
	 * you can use them with some EHCI implementations as another kind
	 * of system debug channel:  like JTAG, RS232, or a console.
	 */
	if (ret != sizeof buf
			|| buf[0] != ret
			|| buf[1] != USB_DT_DEBUG)
		return;

	printf("Debug descriptor:\n"
	       "  bLength              %4u\n"
	       "  bDescriptorType      %4u\n"
	       "  bDebugInEndpoint     0x%02x\n"
	       "  bDebugOutEndpoint    0x%02x\n",
	       buf[0], buf[1],
	       buf[2], buf[3]);
}

static const unsigned char *xml_find_otg(const unsigned char *buf, int buflen)
{
	if (!buf)
		return 0;
	while (buflen >= 3) {
		if (buf[0] == 3 && buf[1] == USB_DT_OTG)
			return buf;
		if (buf[0] > buflen)
			return 0;
		buflen -= buf[0];
		buf += buf[0];
	}
	return 0;
}

static int xml_do_otg(struct libusb_config_descriptor *config)
{
	unsigned	i, k;
	int		j;
	const unsigned char	*desc;

	/* each config of an otg device has an OTG descriptor */
	desc = xml_find_otg(config->extra, config->extra_length);
	for (i = 0; !desc && i < config->bNumInterfaces; i++) {
		const struct libusb_interface *intf;

		intf = &config->interface[i];
		for (j = 0; !desc && j < intf->num_altsetting; j++) {
			const struct libusb_interface_descriptor *alt;

			alt = &intf->altsetting[j];
			desc = xml_find_otg(alt->extra, alt->extra_length);
			for (k = 0; !desc && k < alt->bNumEndpoints; k++) {
				const struct libusb_endpoint_descriptor *ep;

				ep = &alt->endpoint[k];
				desc = xml_find_otg(ep->extra, ep->extra_length);
			}
		}
	}
	if (!desc)
		return 0;

	printf("OTG Descriptor:\n"
		"  bLength               %3u\n"
		"  bDescriptorType       %3u\n"
		"  bmAttributes         0x%02x\n"
		"%s%s",
		desc[0], desc[1], desc[2],
		(desc[2] & 0x01)
			? "    SRP (Session Request Protocol)\n" : "",
		(desc[2] & 0x02)
			? "    HNP (Host Negotiation Protocol)\n" : "");
	return 1;
}

static int xml_do_wireless(libusb_device_handle *fd)
{
	/* FIXME fetch and dump BOS etc */
	if (fd)
		return 0;
	return 0;
}

void xmldev(libusb_device *dev, xmlTextWriterPtr xml_writer)
{
	libusb_device_handle *udev;
	struct libusb_device_descriptor desc;
	int i, ret;
	int otg, wireless;

	otg = wireless = 0;
	ret = libusb_open(dev, &udev);
	if (ret) {
		fprintf(stderr, "Couldn't open device, some information "
			"will be missing\n");
		udev = NULL;
	}

	libusb_get_device_descriptor(dev, &desc);
	xml_device(udev, &desc, xml_writer);
	if (desc.bcdUSB == 0x0250)
		wireless = xml_do_wireless(udev);
	if (desc.bNumConfigurations) {
		struct libusb_config_descriptor *config;

		libusb_get_config_descriptor(dev, 0, &config);
		otg = xml_do_otg(config) || otg;
		libusb_free_config_descriptor(config);

		for (i = 0; i < desc.bNumConfigurations; ++i) {
			libusb_get_config_descriptor(dev, i, &config);
			xml_config(udev, config, xml_writer);
			libusb_free_config_descriptor(config);
		}
	}
	if (!udev)
		return;

	if (desc.bDeviceClass == LIBUSB_CLASS_HUB)
		xml_do_hub(udev, desc.bDeviceProtocol, desc.bcdUSB);
	if (desc.bcdUSB >= 0x0300) {
		xml_bos_descriptor(udev);
	}
	if (desc.bcdUSB == 0x0200) {
		xml_do_dualspeed(udev);
	}
	xml_do_debug(udev);
	xml_device_status(udev, otg, wireless);

	xmlTextWriterEndElement(xml_writer);

	libusb_close(udev);
}
