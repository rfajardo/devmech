/*
 * usbdata.h
 *
 *  Created on: Jun 27, 2011
 *      Author: raul
 */

#ifndef USBDATA_H_
#define USBDATA_H_

#include <devif/regtypes.h>

//#define __KERNEL__

#define ENDPOINT_ADDRESS_MASK	0x0f    /* in bEndpointAddress */
#define ENDPOINT_DIR_MASK		0x80

enum endpoint_direction {
	/** In: device-to-host */
	ENDPOINT_IN = 0x80,

	/** Out: host-to-device */
	ENDPOINT_OUT = 0x00
};

#define TRANSFER_TYPE_MASK			0x03    /* in bmAttributes */

enum transfer_type {
	/** Control endpoint */
	TRANSFER_TYPE_CONTROL = 0,

	/** Isochronous endpoint */
	TRANSFER_TYPE_ISOCHRONOUS = 1,

	/** Bulk endpoint */
	TRANSFER_TYPE_BULK = 2,

	/** Interrupt endpoint */
	TRANSFER_TYPE_INTERRUPT = 3
};

struct interface_configuration;

struct setup_packet
{
	uint8_t		bmRequestType;
	uint8_t 	bRequest;
	uint16_t 	wValue;
	uint16_t 	wIndex;
	uint16_t 	wLength;
};

struct usb_complex_type
{
	struct setup_packet * 	recv_packet;		//the difference might be only bmRequestType bit 8
	struct setup_packet *	send_packet;		//however someone might want to change more
};

struct alternate_interface
{
	uint8_t								bAlternateSetting;
	struct interface_configuration *	ifConf;		//link to alt_interface selector
													//on interface structure
	List * 								endpoints;
};

struct interface_configuration
{
	uint8_t							bInterfaceNumber;
	uint8_t 						bAlternateSetting;

	List * 							alternateInterfaces;
	struct alternate_interface * 	activeInterface;
	List * 							activeEndpointLinks;
};

struct endpoint_descriptor
{
															//link the configuration value of
	uint8_t	*						configuration_value;	//the containing configuration value
	struct alternate_interface *	parent_altif;

	uint8_t							bEndpointAddress;
	uint8_t							bmAttributes;

	uint16_t						wMaxPacketSize;
	uint8_t							bInterval;
};

struct usb_device_desc
{
	uint16_t	idVendor;
	uint16_t	idProduct;

	uint8_t		bMaxPacketSize0;

	uint8_t		bNumConfigurations;
	uint8_t		bConfigurationValue;	//configuration selector, usage of
										//endpoint checks this against
										//the bConfigurationValue of its
										//containing configuration
};


typedef uint8_t Request;
typedef uint8_t Configuration;
typedef struct setup_packet SetupPacket;
typedef struct usb_complex_type UsbData;


typedef struct endpoint_descriptor EndpointDesc;
typedef struct endpoint_descriptor CtrlEndpoint;
typedef struct endpoint_descriptor BulkEndpoint;
typedef struct endpoint_descriptor IntEndpoint;
typedef struct endpoint_descriptor IsoEndpoint;

typedef struct alternate_interface AltIf;
typedef struct interface_configuration IfConf;
typedef struct usb_device_desc UsbDeviceDesc;



struct endpoint_link
{
	uint8_t							bEndpointAddress;
	struct endpoint_descriptor ** 	pp_endpoint;
};

typedef struct endpoint_link EndpointLink;


UsbData * alloc_usb_data(void);
void add_usb_data(Reg * reg, UsbData * usb_data);
void free_usb_data(UsbData * usb_data);

IfConf * alloc_interface(void);
void add_endpoint_link(IfConf * ifconf, uint8_t endpointAddress, EndpointDesc ** endpoint);
void free_interface(IfConf * interface);

AltIf * alloc_alternate_interface(void);
void associate_alternate_interface(AltIf * altif, IfConf * ifconf);
void associate_endpoint(EndpointDesc * endpoint, Configuration * conf, AltIf * altif);
void free_alternate_interface(AltIf * altif);

void set_conf_data(UsbDeviceDesc * usb_device_desc, Configuration * conf);
bool check_conf(UsbDeviceDesc * usb_device_desc, Configuration * conf);

void set_setting_data(AltIf * altif);
bool check_setting(AltIf * altif);


#endif /* USBDATA_H_ */

/*
 * struct //usb
 * {
 * 		UsbDeviceDesc my_device;
 *
 *  	struct //my_configuration
 *  	{
 *  		Configuration * bConfigurationValue;
 *
 *  		struct //my_interface
 *  		{
 *				IfConf my_interface_configuration;
 *
 *				EndpointDesc my_endpoint1;
 *
 *				AltIf my_alt_interface;
 *				EndpointDesc my_endpoint1;
 *				EndpointDesc my_endpoint2;
 *  		} my_interface;
 *
 *  		struct //my_interface2
 *  		{
 *				IfConf my_interface_configuration;
 *
 *				EndpointDesc my_endpoint1;
 *
 *				AltIf my_alt_interface;
 *				EndpointDesc my_endpoint1;
 *				EndpointDesc my_endpoint2;
 *  		} my_interface2;
 *
 *  	} my_configuration;
 * } usb;
 */
