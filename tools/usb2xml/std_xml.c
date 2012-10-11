/*
 * std_xml.c
 *
 *  Created on: Jun 24, 2011
 *      Author: raul
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <libusb.h>
#include <libxml/xmlwriter.h>

#include "std_xml.h"
#include "usbmisc.h"
#include "getusb.h"
#include "classed_xml.h"
#include "lsusb.h"


static const char * const encryption_type[] = {
	"UNSECURE",
	"WIRED",
	"CCM_1",
	"RSA_1",
	"RESERVED"
};

static void xml_security(const unsigned char *buf)
{
	printf("    Security Descriptor:\n"
	       "      bLength             %5u\n"
	       "      bDescriptorType     %5u\n"
	       "      wTotalLength        %5u\n"
	       "      bNumEncryptionTypes %5u\n",
	       buf[0], buf[1], (buf[3] << 8 | buf[2]), buf[4]);
}


static void xml_pipe_desc(const unsigned char *buf)
{
	static const char *pipe_name[] = {
		"Reserved",
		"Command pipe",
		"Status pipe",
		"Data-in pipe",
		"Data-out pipe",
		[5 ... 0xDF] = "Reserved",
		[0xE0 ... 0xEF] = "Vendor specific",
		[0xF0 ... 0xFF] = "Reserved",
	};

	if (buf[0] == 4 && buf[1] == 0x24) {
		printf("        %s (0x%02x)\n", pipe_name[buf[2]], buf[2]);
	} else {
		printf("        INTERFACE CLASS: ");
		xml_bytes(buf, buf[0]);
	}
}

static void xml_association(libusb_device_handle *dev, const unsigned char *buf)
{
	char cls[128], subcls[128], proto[128];
	char func[128];

	get_class_string(cls, sizeof(cls), buf[4]);
	get_subclass_string(subcls, sizeof(subcls), buf[4], buf[5]);
	get_protocol_string(proto, sizeof(proto), buf[4], buf[5], buf[6]);
	get_string(dev, func, sizeof(func), buf[7]);
	printf("    Interface Association:\n"
	       "      bLength             %5u\n"
	       "      bDescriptorType     %5u\n"
	       "      bFirstInterface     %5u\n"
	       "      bInterfaceCount     %5u\n"
	       "      bFunctionClass      %5u %s\n"
	       "      bFunctionSubClass   %5u %s\n"
	       "      bFunctionProtocol   %5u %s\n"
	       "      iFunction           %5u %s\n",
	       buf[0], buf[1],
	       buf[2], buf[3],
	       buf[4], cls,
	       buf[5], subcls,
	       buf[6], proto,
	       buf[7], func);
}

static void xml_container_id_device_capability_desc(unsigned char *buf)
{
	if (buf[0] < 20) {
		printf("  Bad Container ID Device Capability descriptor.\n");
		return;
	}
	printf("  Container ID Device Capability:\n"
			"    bLength             %5u\n"
			"    bDescriptorType     %5u\n"
			"    bDevCapabilityType  %5u\n"
			"    bReserved           %5u\n",
			buf[0], buf[1], buf[2], buf[3]);
	printf("    ContainerID             %s\n",
			get_guid(&buf[4]));
}

static void xml_encryption_type(const unsigned char *buf)
{
	int b_encryption_type = buf[2] & 0x4;

	printf("    Encryption Type Descriptor:\n"
	       "      bLength             %5u\n"
	       "      bDescriptorType     %5u\n"
	       "      bEncryptionType     %5u %s\n"
	       "      bEncryptionValue    %5u\n"
	       "      bAuthKeyIndex       %5u\n",
	       buf[0], buf[1], buf[2],
	       encryption_type[b_encryption_type], buf[3], buf[4]);
}

static void xml_endpoint(libusb_device_handle *dev, const struct libusb_interface_descriptor *interface, const struct libusb_endpoint_descriptor *endpoint, xmlTextWriterPtr xml_writer)
{
	char tmp[50];
	char name[50] = "endpoint";
	static unsigned int instance_nr = 0;
	static const char * const typeattr[] = {
		"Control",
		"Isochronous",
		"Bulk",
		"Interrupt"
	};
	static const char * const syncattr[] = {
		"None",
		"Asynchronous",
		"Adaptive",
		"Synchronous",
		"Reserved"
	};
	static const char * const usage[] = {
		"Data",
		"Feedback",
		"ImplicitFeedback",
		"Reserved"
	};
	static const char * const hb[] = { "1x", "2x", "3x", "(?\?)" };
	const unsigned char *buf;
	unsigned size;
	unsigned wmax = le16_to_cpu(endpoint->wMaxPacketSize);

	xmlTextWriterStartElement(xml_writer, "udev:endpointDescriptor");

	sprintf(tmp, "%s%u", name, instance_nr);
	xmlTextWriterWriteElement(xml_writer, "udev:name", tmp);

	sprintf(tmp, "%u", endpoint->bLength);
	xmlTextWriterWriteElement(xml_writer, "udev:bLength", tmp);

	sprintf(tmp, "%02x", endpoint->bDescriptorType);
	xmlTextWriterWriteElement(xml_writer, "udev:bDescriptorType", tmp);


	xmlTextWriterStartElement(xml_writer, "udev:bEndpointAddress");
	sprintf(tmp, "%01u", (endpoint->bEndpointAddress & 0x80) >> 7);
	xmlTextWriterWriteComment(xml_writer, (endpoint->bEndpointAddress & 0x80) ? "IN" : "OUT");
	xmlTextWriterWriteElement(xml_writer, "udev:direction", (endpoint->bEndpointAddress & 0x80) ? "DeviceToHost" : "HostToDevice");

	sprintf(tmp, "%01u", endpoint->bEndpointAddress & 0x0f);
	xmlTextWriterWriteElement(xml_writer, "udev:number", tmp);
	xmlTextWriterEndElement(xml_writer);


	xmlTextWriterStartElement(xml_writer, "udev:bmAttributes");
	xmlTextWriterWriteElement(xml_writer, "udev:usageType", usage[(endpoint->bmAttributes >> 4) & 3]);

	if ( !strcmp(typeattr[endpoint->bmAttributes & 3], "Isochronous") )
		xmlTextWriterWriteElement(xml_writer, "udev:synchronizationType", syncattr[(endpoint->bmAttributes >> 2) & 3]);
	else
		xmlTextWriterWriteElement(xml_writer, "udev:synchronizationType", syncattr[4]);

	xmlTextWriterWriteElement(xml_writer, "udev:transferType", typeattr[endpoint->bmAttributes & 3]);
	xmlTextWriterEndElement(xml_writer);


	sprintf(tmp, "%s %d bytes", hb[(wmax >> 11) & 3], wmax & 0x7ff);
	xmlTextWriterWriteComment(xml_writer, tmp);
	sprintf(tmp, "%u", wmax);
	xmlTextWriterWriteElement(xml_writer, "udev:wMaxPacketSize", tmp);

	sprintf(tmp, "%u", endpoint->bInterval);
	xmlTextWriterWriteElement(xml_writer, "udev:bInterval", tmp);

	/* only for audio endpoints */
	if (endpoint->bLength == 9)
		printf("        bRefresh            %5u\n"
		       "        bSynchAddress       %5u\n",
		       endpoint->bRefresh, endpoint->bSynchAddress);

	/* avoid re-ordering or hiding descriptors for display */
	if (endpoint->extra_length) {
		size = endpoint->extra_length;
		buf = endpoint->extra;
		while (size >= 2 * sizeof(u_int8_t)) {
			if (buf[0] < 2) {
				xml_junk(buf, "        ", size);
				break;
			}
			switch (buf[1]) {
			case USB_DT_CS_ENDPOINT:
				if (interface->bInterfaceClass == 1 && interface->bInterfaceSubClass == 2)
					xml_audiostreaming_endpoint(buf, interface->bInterfaceProtocol);
				else if (interface->bInterfaceClass == 1 && interface->bInterfaceSubClass == 3)
					xml_midistreaming_endpoint(buf);
				break;
			case USB_DT_CS_INTERFACE:
				/* MISPLACED DESCRIPTOR ... less indent */
				switch (interface->bInterfaceClass) {
				case LIBUSB_CLASS_COMM:
				case LIBUSB_CLASS_DATA:	/* comm data */
					xml_comm_descriptor(dev, buf,
						"      ");
					break;
				case LIBUSB_CLASS_MASS_STORAGE:
					xml_pipe_desc(buf);
					break;
				default:
					printf("        INTERFACE CLASS: ");
					xml_bytes(buf, buf[0]);
				}
				break;
			case USB_DT_CS_DEVICE:
				/* MISPLACED DESCRIPTOR ... less indent */
				switch (interface->bInterfaceClass) {
				case USB_CLASS_CCID:
					xml_ccid_device(buf);
					break;
				default:
					printf("        DEVICE CLASS: ");
					xml_bytes(buf, buf[0]);
				}
				break;
			case USB_DT_OTG:
				/* handled separately */
				break;
			case USB_DT_INTERFACE_ASSOCIATION:
				xml_association(dev, buf);
				break;
			case USB_DT_SS_ENDPOINT_COMP:
				printf("        bMaxBurst %15u\n", buf[2]);
				/* Print bulk streams info or isoc "Mult" */
				if ((endpoint->bmAttributes & 3) == 2 &&
						(buf[3] & 0x1f))
					printf("        MaxStreams %14u\n",
							(unsigned) 1 << buf[3]);
				if ((endpoint->bmAttributes & 3) == 1 &&
						(buf[3] & 0x3))
					printf("        Mult %20u\n",
							buf[3] & 0x3);
				break;
			default:
				/* often a misplaced class descriptor */
				printf("        ** UNRECOGNIZED: ");
				xml_bytes(buf, buf[0]);
				break;
			}
			size -= buf[0];
			buf += buf[0];
		}
	}

	xmlTextWriterEndElement(xml_writer);
	instance_nr++;
}

static void xml_altsetting(libusb_device_handle *dev, const struct libusb_interface_descriptor *interface, xmlTextWriterPtr xml_writer)
{
	char cls[128], subcls[128], proto[128];
	char ifstr[128];

	const unsigned char *buf;
	unsigned size, i;

	char tmp[50];
	char name[50] = "interface";
	static unsigned int instance_nr = 0;

	get_class_string(cls, sizeof(cls), interface->bInterfaceClass);
	get_subclass_string(subcls, sizeof(subcls), interface->bInterfaceClass, interface->bInterfaceSubClass);
	get_protocol_string(proto, sizeof(proto), interface->bInterfaceClass, interface->bInterfaceSubClass, interface->bInterfaceProtocol);
	get_string(dev, ifstr, sizeof(ifstr), interface->iInterface);

	xmlTextWriterStartElement(xml_writer, "udev:interfaceDescriptor");

	sprintf(tmp, "%s%u", name, instance_nr);
	xmlTextWriterWriteElement(xml_writer, "udev:name", tmp);

	sprintf(tmp, "%u", interface->bLength);
	xmlTextWriterWriteElement(xml_writer, "udev:bLength", tmp);

	sprintf(tmp, "%02x", interface->bDescriptorType);
	xmlTextWriterWriteElement(xml_writer, "udev:bDescriptorType", tmp);

	sprintf(tmp, "%u", interface->bInterfaceNumber);
	xmlTextWriterWriteElement(xml_writer, "udev:bInterfaceNumber", tmp);

	sprintf(tmp, "%u", interface->bAlternateSetting);
	xmlTextWriterWriteElement(xml_writer, "udev:bAlternateSetting", tmp);

	sprintf(tmp, "%u", interface->bNumEndpoints);
	xmlTextWriterWriteElement(xml_writer, "udev:bNumEndpoints", tmp);

	if (strcmp(cls, ""))
			xmlTextWriterWriteComment(xml_writer, cls);
	sprintf(tmp, "%02x", interface->bInterfaceClass);
	xmlTextWriterWriteElement(xml_writer, "udev:bInterfaceClass", tmp);

	if (strcmp(subcls, ""))
			xmlTextWriterWriteComment(xml_writer, subcls);
	sprintf(tmp, "%02x", interface->bInterfaceSubClass);
	xmlTextWriterWriteElement(xml_writer, "udev:bInterfaceSubClass", tmp);

	if (strcmp(proto, ""))
			xmlTextWriterWriteComment(xml_writer, proto);
	sprintf(tmp, "%02x", interface->bInterfaceProtocol);
	xmlTextWriterWriteElement(xml_writer, "udev:bInterfaceProtocol", tmp);

	if (strcmp(ifstr, ""))
			xmlTextWriterWriteComment(xml_writer, ifstr);
	sprintf(tmp, "%u", interface->iInterface);
	xmlTextWriterWriteElement(xml_writer, "udev:iInterface", tmp);

	/* avoid re-ordering or hiding descriptors for display */
	if (interface->extra_length) {
		size = interface->extra_length;
		buf = interface->extra;
		while (size >= 2 * sizeof(u_int8_t)) {
			if (buf[0] < 2) {
				xml_junk(buf, "      ", size);
				break;
			}

			switch (buf[1]) {

			/* This is the polite way to provide class specific
			 * descriptors: explicitly tagged, using common class
			 * spec conventions.
			 */
			case USB_DT_CS_DEVICE:
			case USB_DT_CS_INTERFACE:
				switch (interface->bInterfaceClass) {
				case LIBUSB_CLASS_AUDIO:
					switch (interface->bInterfaceSubClass) {
					case 1:
						xml_audiocontrol_interface(dev, buf, interface->bInterfaceProtocol);
						break;
					case 2:
						xml_audiostreaming_interface(dev, buf, interface->bInterfaceProtocol);
						break;
					case 3:
						xml_midistreaming_interface(dev, buf);
						break;
					default:
						goto dump;
					}
					break;
				case LIBUSB_CLASS_COMM:
					xml_comm_descriptor(dev, buf,
						"      ");
					break;
				case USB_CLASS_VIDEO:
					switch (interface->bInterfaceSubClass) {
					case 1:
						xml_videocontrol_interface(dev, buf);
						break;
					case 2:
						xml_videostreaming_interface(buf);
						break;
					default:
						goto dump;
					}
					break;
				case USB_CLASS_APPLICATION:
					switch (interface->bInterfaceSubClass) {
					case 1:
						xml_dfu_interface(buf);
						break;
					default:
						goto dump;
					}
					break;
				case LIBUSB_CLASS_HID:
					xml_hid_device(dev, interface, buf);
					break;
				case USB_CLASS_CCID:
					xml_ccid_device(buf);
					break;
				default:
					goto dump;
				}
				break;

			/* This is the ugly way:  implicitly tagged,
			 * each class could redefine the type IDs.
			 */
			default:
				switch (interface->bInterfaceClass) {
				case LIBUSB_CLASS_HID:
					xml_hid_device(dev, interface, buf);
					break;
				case USB_CLASS_CCID:
					xml_ccid_device(buf);
					break;
				case 0xe0:	/* wireless */
					switch (interface->bInterfaceSubClass) {
					case 1:
						switch (interface->bInterfaceProtocol) {
						case 2:
							xml_rc_interface(buf);
							break;
						default:
							goto dump;
						}
						break;
					case 2:
						xml_wire_adapter(buf);
						break;
					default:
						goto dump;
					}
					break;
				default:
					/* ... not everything is class-specific */
					switch (buf[1]) {
					case USB_DT_OTG:
						/* handled separately */
						break;
					case USB_DT_INTERFACE_ASSOCIATION:
						xml_association(dev, buf);
						break;
					default:
dump:
						/* often a misplaced class descriptor */
						printf("      ** UNRECOGNIZED: ");
						xml_bytes(buf, buf[0]);
						break;
					}
				}
			}
			size -= buf[0];
			buf += buf[0];
		}
	}

	for (i = 0 ; i < interface->bNumEndpoints ; i++)
		xml_endpoint(dev, interface, &interface->endpoint[i], xml_writer);

	xmlTextWriterEndElement(xml_writer);
	instance_nr++;
}

static void xml_interface(libusb_device_handle *dev, const struct libusb_interface *interface, xmlTextWriterPtr xml_writer)
{
	int i;

	char tmp[50];
	char name[50] = "interfaceGroup";
	static unsigned int instance_nr = 0;

	xmlTextWriterStartElement(xml_writer, "udev:interfaceDescriptorGroup");

	sprintf(tmp, "%s%u", name, instance_nr);
	xmlTextWriterWriteElement(xml_writer, "udev:name", tmp);

	for (i = 0; i < interface->num_altsetting; i++)
		xml_altsetting(dev, &interface->altsetting[i], xml_writer);

	xmlTextWriterEndElement(xml_writer);
	instance_nr++;
}


static void xml_usb2_device_capability_desc(unsigned char *buf)
{
	unsigned int wide;

	wide = buf[3] + (buf[4] << 8) +
		(buf[5] << 8) + (buf[6] << 8);
	printf("  USB 2.0 Extension Device Capability:\n"
			"    bLength             %5u\n"
			"    bDescriptorType     %5u\n"
			"    bDevCapabilityType  %5u\n"
			"    bmAttributes   0x%08x\n",
			buf[0], buf[1], buf[2], wide);
	if (!(wide & 0x02))
		printf("      (Missing must-be-set LPM bit!)\n");
	else
		printf("      Link Power Management (LPM)"
				" Supported\n");
}

static void xml_ss_device_capability_desc(unsigned char *buf)
{
	if (buf[0] < 10) {
		printf("  Bad SuperSpeed USB Device Capability descriptor.\n");
		return;
	}
	printf("  SuperSpeed USB Device Capability:\n"
			"    bLength             %5u\n"
			"    bDescriptorType     %5u\n"
			"    bDevCapabilityType  %5u\n"
			"    bmAttributes         0x%02x\n",
			buf[0], buf[1], buf[2], buf[3]);
	if (!(buf[3] & 0x02))
		printf("      Latency Tolerance Messages (LTM)"
				" Supported\n");
	printf("    wSpeedsSupported   0x%04x\n", buf[4]);
	if (buf[4] & (1 << 0))
		printf("      Device can operate at Low Speed (1Mbps)\n");
	if (buf[4] & (1 << 1))
		printf("      Device can operate at Full Speed (12Mbps)\n");
	if (buf[4] & (1 << 2))
		printf("      Device can operate at High Speed (480Mbps)\n");
	if (buf[4] & (1 << 3))
		printf("      Device can operate at SuperSpeed (5Gbps)\n");

	printf("    bFunctionalitySupport %3u\n", buf[5]);
	switch(buf[5]) {
	case 0:
		printf("      Lowest fully-functional device speed is "
				"Low Speed (1Mbps)\n");
		break;
	case 1:
		printf("      Lowest fully-functional device speed is "
				"Full Speed (12Mbps)\n");
		break;
	case 2:
		printf("      Lowest fully-functional device speed is "
				"High Speed (480Mbps)\n");
		break;
	case 3:
		printf("      Lowest fully-functional device speed is "
				"SuperSpeed (5Gbps)\n");
		break;
	default:
		printf("      Lowest fully-functional device speed is "
				"at an unknown speed!\n");
		break;
	}
	printf("    bU1DevExitLat        %4u micro seconds\n", buf[6]);
	printf("    bU2DevExitLat    %8u micro seconds\n", buf[8] + (buf[7] << 8));
}

/* ---------------------------------------------------------------------- */

void xml_bytes(const unsigned char *buf, unsigned int len)
{
	unsigned int i;

	for (i = 0; i < len; i++)
		printf(" %02x", buf[i]);
	printf("\n");
}

void xml_junk(const unsigned char *buf, const char *indent, unsigned int len)
{
	unsigned int i;

	if (buf[0] <= len)
		return;
	printf("%sjunk at descriptor end:", indent);
	for (i = len; i < buf[0]; i++)
		printf(" %02x", buf[i]);
	printf("\n");
}

void xml_unit(unsigned int data, unsigned int len)
{
	char *systems[5] = { "None", "SI Linear", "SI Rotation",
			"English Linear", "English Rotation" };

	char *units[5][8] = {
		{ "None", "None", "None", "None", "None",
				"None", "None", "None" },
		{ "None", "Centimeter", "Gram", "Seconds", "Kelvin",
				"Ampere", "Candela", "None" },
		{ "None", "Radians",    "Gram", "Seconds", "Kelvin",
				"Ampere", "Candela", "None" },
		{ "None", "Inch",       "Slug", "Seconds", "Fahrenheit",
				"Ampere", "Candela", "None" },
		{ "None", "Degrees",    "Slug", "Seconds", "Fahrenheit",
				"Ampere", "Candela", "None" },
	};

	unsigned int i;
	unsigned int sys;
	int earlier_unit = 0;

	/* First nibble tells us which system we're in. */
	sys = data & 0xf;
	data >>= 4;

	if (sys > 4) {
		if (sys == 0xf)
			printf("System: Vendor defined, Unit: (unknown)\n");
		else
			printf("System: Reserved, Unit: (unknown)\n");
		return;
	} else {
		printf("System: %s, Unit: ", systems[sys]);
	}
	for (i = 1 ; i < len * 2 ; i++) {
		char nibble = data & 0xf;
		data >>= 4;
		if (nibble != 0) {
			if (earlier_unit++ > 0)
				printf("*");
			printf("%s", units[sys][i]);
			if (nibble != 1) {
				/* This is a _signed_ nibble(!) */

				int val = nibble & 0x7;
				if (nibble & 0x08)
					val = -((0x7 & ~val) + 1);
				printf("^%d", val);
			}
		}
	}
	if (earlier_unit == 0)
		printf("(None)");
	printf("\n");
}

/*
 * General config descriptor dump
 */

void xml_device(
	libusb_device_handle *dev,
	struct libusb_device_descriptor *descriptor,
	xmlTextWriterPtr xml_writer
)
{
	char vendor[128], product[128];
	char cls[128], subcls[128], proto[128];
	char mfg[128], prod[128], serial[128];

	char tmp[50];

	get_vendor_string(vendor, sizeof(vendor), descriptor->idVendor);
	get_product_string(product, sizeof(product),
			descriptor->idVendor, descriptor->idProduct);
	get_class_string(cls, sizeof(cls), descriptor->bDeviceClass);
	get_subclass_string(subcls, sizeof(subcls),
			descriptor->bDeviceClass, descriptor->bDeviceSubClass);
	get_protocol_string(proto, sizeof(proto), descriptor->bDeviceClass,
			descriptor->bDeviceSubClass, descriptor->bDeviceProtocol);
	get_string(dev, mfg, sizeof(mfg), descriptor->iManufacturer);
	get_string(dev, prod, sizeof(prod), descriptor->iProduct);
	get_string(dev, serial, sizeof(serial), descriptor->iSerialNumber);

	xmlTextWriterStartElement(xml_writer, "udev:deviceDescriptor");

	sprintf(tmp, "%u", descriptor->bLength);
	xmlTextWriterWriteElement(xml_writer, "udev:bLength", tmp);

	sprintf(tmp, "%02x", descriptor->bDescriptorType);
	xmlTextWriterWriteElement(xml_writer, "udev:bDescriptorType", tmp);

	sprintf(tmp, "%04x", descriptor->bcdUSB);
	xmlTextWriterWriteElement(xml_writer, "udev:bcdUSB", tmp);

	if (strcmp(cls, ""))
		xmlTextWriterWriteComment(xml_writer, cls);
	sprintf(tmp, "%02x", descriptor->bDeviceClass);
	xmlTextWriterWriteElement(xml_writer, "udev:bDeviceClass", tmp);

	if (strcmp(subcls, ""))
		xmlTextWriterWriteComment(xml_writer, subcls);
	sprintf(tmp, "%02x", descriptor->bDeviceSubClass);
	xmlTextWriterWriteElement(xml_writer, "udev:bDeviceSubClass", tmp);

	if (strcmp(proto, ""))
		xmlTextWriterWriteComment(xml_writer, proto);
	sprintf(tmp, "%02x", descriptor->bDeviceProtocol);
	xmlTextWriterWriteElement(xml_writer, "udev:bDeviceProtocol", tmp);

	sprintf(tmp, "%u", descriptor->bMaxPacketSize0);
	xmlTextWriterWriteElement(xml_writer, "udev:bMaxPacketSize0", tmp);

	if (strcmp(vendor, ""))
		xmlTextWriterWriteComment(xml_writer, vendor);
	sprintf(tmp, "%04x", descriptor->idVendor);
	xmlTextWriterWriteElement(xml_writer, "udev:idVendor", tmp);

	if (strcmp(product, ""))
		xmlTextWriterWriteComment(xml_writer, product);
	sprintf(tmp, "%04x", descriptor->idProduct);
	xmlTextWriterWriteElement(xml_writer, "udev:idProduct", tmp);

	sprintf(tmp, "%04x", descriptor->bcdDevice);
	xmlTextWriterWriteElement(xml_writer, "udev:bcdDevice", tmp);

	if (strcmp(mfg, ""))
		xmlTextWriterWriteComment(xml_writer, mfg);
	sprintf(tmp, "%u", descriptor->iManufacturer);
	xmlTextWriterWriteElement(xml_writer, "udev:iManufacturer", tmp);

	if (strcmp(prod, ""))
		xmlTextWriterWriteComment(xml_writer, prod);
	sprintf(tmp, "%u", descriptor->iProduct);
	xmlTextWriterWriteElement(xml_writer, "udev:iProduct", tmp);

	if (strcmp(serial, ""))
		xmlTextWriterWriteComment(xml_writer, serial);
	sprintf(tmp, "%u", descriptor->iSerialNumber);
	xmlTextWriterWriteElement(xml_writer, "udev:iSerialNumber", tmp);

	sprintf(tmp, "%u", descriptor->bNumConfigurations);
	xmlTextWriterWriteElement(xml_writer, "udev:bNumConfigurations", tmp);

}

void xml_config(libusb_device_handle *dev, struct libusb_config_descriptor *config, xmlTextWriterPtr xml_writer)
{
	char cfg[128];
	int i;
	char tmp[50];
	char name[50] = "configuration";
	static unsigned int instance_nr = 0;

	get_string(dev, cfg, sizeof(cfg), config->iConfiguration);

	xmlTextWriterStartElement(xml_writer, "udev:configurationDescriptor");

	sprintf(tmp, "%s%u", name, instance_nr);
	xmlTextWriterWriteElement(xml_writer, "udev:name", tmp);

	sprintf(tmp, "%u", config->bLength);
	xmlTextWriterWriteElement(xml_writer, "udev:bLength", tmp);

	sprintf(tmp, "%02x", config->bDescriptorType);
	xmlTextWriterWriteElement(xml_writer, "udev:bDescriptorType", tmp);

	sprintf(tmp, "%u", config->wTotalLength);
	xmlTextWriterWriteElement(xml_writer, "udev:wTotalLength", tmp);

	sprintf(tmp, "%u", config->bNumInterfaces);
	xmlTextWriterWriteElement(xml_writer, "udev:bNumInterfaces", tmp);

	sprintf(tmp, "%u", config->bConfigurationValue);
	xmlTextWriterWriteElement(xml_writer, "udev:bConfigurationValue", tmp);

	if (strcmp(cfg, ""))
		xmlTextWriterWriteComment(xml_writer, cfg);
	sprintf(tmp, "%u", config->iConfiguration);
	xmlTextWriterWriteElement(xml_writer, "udev:iConfiguration", tmp);


	xmlTextWriterStartElement(xml_writer, "udev:bmAttributes");
	if (config->bmAttributes & 0x40)
		xmlTextWriterWriteComment(xml_writer, "Self Powered");
	else
		xmlTextWriterWriteComment(xml_writer, "Bus Powered");
	sprintf(tmp, "%s", ((config->bmAttributes & 0x40) >> 6) ? "true" : "false");
	xmlTextWriterWriteElement(xml_writer, "udev:selfPowered", tmp);

	if (config->bmAttributes & 0x20)
			xmlTextWriterWriteComment(xml_writer, "Remote Wakeup");
	sprintf(tmp, "%s", ((config->bmAttributes & 0x20) >> 5) ? "true" : "false");
	xmlTextWriterWriteElement(xml_writer, "udev:remoteWakeup", tmp);
	xmlTextWriterEndElement(xml_writer);


	sprintf(tmp, "MaxPower %umA", config->MaxPower * 2);
	xmlTextWriterWriteComment(xml_writer, tmp);
	sprintf(tmp, "%u", config->MaxPower);
	xmlTextWriterWriteElement(xml_writer, "udev:bMaxPower", tmp);

	/* avoid re-ordering or hiding descriptors for display */
	if (config->extra_length) {
		int		size = config->extra_length;
		const unsigned char	*buf = config->extra;

		while (size >= 2) {
			if (buf[0] < 2) {
				xml_junk(buf, "        ", size);
				break;
			}
			switch (buf[1]) {
			case USB_DT_OTG:
				/* handled separately */
				break;
			case USB_DT_INTERFACE_ASSOCIATION:
				xml_association(dev, buf);
				break;
			case USB_DT_SECURITY:
				xml_security(buf);
				break;
			case USB_DT_ENCRYPTION_TYPE:
				xml_encryption_type(buf);
				break;
			default:
				/* often a misplaced class descriptor */
				printf("    ** UNRECOGNIZED: ");
				xml_bytes(buf, buf[0]);
				break;
			}
			size -= buf[0];
			buf += buf[0];
		}
	}
	for (i = 0 ; i < config->bNumInterfaces ; i++)
		xml_interface(dev, &config->interface[i], xml_writer);

	xmlTextWriterEndElement(xml_writer);
	instance_nr++;
}

void
xml_device_status(libusb_device_handle *fd, int otg, int wireless)
{
	unsigned char status[8];
	int ret;

	ret = usb_control_msg(fd, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD
				| LIBUSB_RECIPIENT_DEVICE,
			LIBUSB_REQUEST_GET_STATUS,
			0, 0,
			status, 2,
			CTRL_TIMEOUT);
	if (ret < 0) {
		fprintf(stderr,
			"cannot read device status, %s (%d)\n",
			strerror(errno), errno);
		return;
	}

	printf("Device Status:     0x%02x%02x\n",
			status[1], status[0]);
	if (status[0] & (1 << 0))
		printf("  Self Powered\n");
	else
		printf("  (Bus Powered)\n");
	if (status[0] & (1 << 1))
		printf("  Remote Wakeup Enabled\n");
	if (status[0] & (1 << 2)) {
		/* for high speed devices */
		if (!wireless)
			printf("  Test Mode\n");
		/* for devices with Wireless USB support */
		else
			printf("  Battery Powered\n");
	}
	/* if both HOST and DEVICE support OTG */
	if (otg) {
		if (status[0] & (1 << 3))
			printf("  HNP Enabled\n");
		if (status[0] & (1 << 4))
			printf("  HNP Capable\n");
		if (status[0] & (1 << 5))
			printf("  ALT port is HNP Capable\n");
	}
	/* for high speed devices with debug descriptors */
	if (status[0] & (1 << 6))
		printf("  Debug Mode\n");

	if (!wireless)
		return;

	/* Wireless USB exposes FIVE different types of device status,
	 * accessed by distinct wIndex values.
	 */
	ret = usb_control_msg(fd, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD
				| LIBUSB_RECIPIENT_DEVICE,
			LIBUSB_REQUEST_GET_STATUS,
			0, 1 /* wireless status */,
			status, 1,
			CTRL_TIMEOUT);
	if (ret < 0) {
		fprintf(stderr,
			"cannot read wireless %s, %s (%d)\n",
			"status",
			strerror(errno), errno);
		return;
	}
	printf("Wireless Status:     0x%02x\n", status[0]);
	if (status[0] & (1 << 0))
		printf("  TX Drp IE\n");
	if (status[0] & (1 << 1))
		printf("  Transmit Packet\n");
	if (status[0] & (1 << 2))
		printf("  Count Packets\n");
	if (status[0] & (1 << 3))
		printf("  Capture Packet\n");

	ret = usb_control_msg(fd, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD
				| LIBUSB_RECIPIENT_DEVICE,
			LIBUSB_REQUEST_GET_STATUS,
			0, 2 /* Channel Info */,
			status, 1,
			CTRL_TIMEOUT);
	if (ret < 0) {
		fprintf(stderr,
			"cannot read wireless %s, %s (%d)\n",
			"channel info",
			strerror(errno), errno);
		return;
	}
	printf("Channel Info:        0x%02x\n", status[0]);

	/* 3=Received data: many bytes, for count packets or capture packet */

	ret = usb_control_msg(fd, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD
				| LIBUSB_RECIPIENT_DEVICE,
			LIBUSB_REQUEST_GET_STATUS,
			0, 3 /* MAS Availability */,
			status, 8,
			CTRL_TIMEOUT);
	if (ret < 0) {
		fprintf(stderr,
			"cannot read wireless %s, %s (%d)\n",
			"MAS info",
			strerror(errno), errno);
		return;
	}
	printf("MAS Availability:    ");
	xml_bytes(status, 8);

	ret = usb_control_msg(fd, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD
				| LIBUSB_RECIPIENT_DEVICE,
			LIBUSB_REQUEST_GET_STATUS,
			0, 5 /* Current Transmit Power */,
			status, 2,
			CTRL_TIMEOUT);
	if (ret < 0) {
		fprintf(stderr,
			"cannot read wireless %s, %s (%d)\n",
			"transmit power",
			strerror(errno), errno);
		return;
	}
	printf("Transmit Power:\n");
	printf(" TxNotification:     0x%02x\n", status[0]);
	printf(" TxBeacon:     :     0x%02x\n", status[1]);
}

void xml_bos_descriptor(libusb_device_handle *fd)
{
	/* Total for all known BOS descriptors is 43 bytes:
	 * 6 bytes for Wireless USB, 7 bytes for USB 2.0 extension,
	 * 10 bytes for SuperSpeed, 20 bytes for Container ID.
	 */
	unsigned char bos_desc[43];
	unsigned int bos_desc_size;
	int size, ret;
	unsigned char *buf;

	/* Get the first 5 bytes to get the wTotalLength field */
	ret = usb_control_msg(fd,
			LIBUSB_ENDPOINT_IN | LIBUSB_RECIPIENT_DEVICE,
			LIBUSB_REQUEST_GET_DESCRIPTOR,
			USB_DT_BOS << 8, 0,
			bos_desc, 5, CTRL_TIMEOUT);
	if (ret <= 0)
		return;
	else if (bos_desc[0] != 5 || bos_desc[1] != USB_DT_BOS)
		return;

	bos_desc_size = bos_desc[2] + (bos_desc[3] << 8);
	printf("Binary Object Store Descriptor:\n"
	       "  bLength             %5u\n"
	       "  bDescriptorType     %5u\n"
	       "  wTotalLength        %5u\n"
	       "  bNumDeviceCaps      %5u\n",
	       bos_desc[0], bos_desc[1],
	       bos_desc_size, bos_desc[4]);

	if (bos_desc_size <= 5) {
		if (bos_desc[4] > 0)
			fprintf(stderr, "Couldn't get "
					"device capability descriptors\n");
		return;
	}
	if (bos_desc_size > sizeof bos_desc) {
		fprintf(stderr, "FIXME: alloc bigger buffer for "
				"device capability descriptors\n");
		return;
	}

	ret = usb_control_msg(fd,
			LIBUSB_ENDPOINT_IN | LIBUSB_RECIPIENT_DEVICE,
			LIBUSB_REQUEST_GET_DESCRIPTOR,
			USB_DT_BOS << 8, 0,
			bos_desc, bos_desc_size, CTRL_TIMEOUT);
	if (ret < 0) {
		fprintf(stderr, "Couldn't get device capability descriptors\n");
		return;
	}

	size = bos_desc_size - 5;
	buf = &bos_desc[5];

	while (size >= 3) {
		if (buf[0] < 3) {
			printf("buf[0] = %u\n", buf[0]);
			return;
		}
		switch (buf[2]) {
		case USB_DC_WIRELESS_USB:
			/* FIXME */
			break;
		case USB_DC_20_EXTENSION:
			xml_usb2_device_capability_desc(buf);
			break;
		case USB_DC_SUPERSPEED:
			xml_ss_device_capability_desc(buf);
			break;
		case USB_DC_CONTAINER_ID:
			xml_container_id_device_capability_desc(buf);
			break;
		default:
			printf("  ** UNRECOGNIZED: ");
			xml_bytes(buf, buf[0]);
			break;
		}
		size -= buf[0];
		buf += buf[0];
	}
}
