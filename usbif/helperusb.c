/*
 * helperusb.c
 *
 *  Created on: Jul 5, 2011
 *      Author: raul
 */

#include "usbdata.h"
#include <devif/regdata.h>
#include "usbint.h"
#include "usbifdebug.h"


static EndpointLink * alloc_endpoint_link(void)
{
	EndpointLink * link;
	link = ALLOC(sizeof(EndpointLink));
	link->pp_endpoint = NULL;
	return link;
}

static void reset_endpoint_links(IfConf * ifconf)
{
	List * linkHook;
	for ( linkHook = ifconf->activeEndpointLinks; linkHook != NULL; linkHook = linkHook->next )
	{
		*((EndpointLink*)linkHook->data)->pp_endpoint = NULL;
	}
}


UsbData * alloc_usb_data(void)
{
	UsbData * tmpUsb;
	tmpUsb = ALLOC(sizeof(UsbData));
	tmpUsb->recv_packet = NULL;
	tmpUsb->send_packet = NULL;
	return tmpUsb;
}
EXPORT_SYMBOL_GPL(alloc_usb_data);

void add_usb_data(Reg * reg, UsbData * usb_data)
{
	reg->if_complex_data = usb_data;
}
EXPORT_SYMBOL_GPL(add_usb_data);

void free_usb_data(UsbData * usb_data)
{
	FREE(usb_data);
}
EXPORT_SYMBOL_GPL(free_usb_data);


IfConf * alloc_interface(void)
{
	IfConf * tmpIf;
	tmpIf = ALLOC(sizeof(IfConf));
	tmpIf->bInterfaceNumber = 0;
	tmpIf->bAlternateSetting = 0;
	tmpIf->alternateInterfaces = NULL;
	tmpIf->activeInterface = NULL;
	tmpIf->activeEndpointLinks = NULL;
	return tmpIf;
}
EXPORT_SYMBOL_GPL(alloc_interface);

void add_endpoint_link(IfConf * ifconf, uint8_t endpointAddress, EndpointDesc ** endpoint)
{
	List * linkHook;
	linkHook = ALLOC(sizeof(List));
	linkHook->data = alloc_endpoint_link();
	((EndpointLink*)linkHook->data)->bEndpointAddress = endpointAddress;
	((EndpointLink*)linkHook->data)->pp_endpoint = endpoint;
	pushBack(&ifconf->activeEndpointLinks, linkHook);
}
EXPORT_SYMBOL_GPL(add_endpoint_link);

void free_interface(IfConf * interface)
{
	removeList(&interface->alternateInterfaces);
	removeList(&interface->activeEndpointLinks);
	FREE(interface);
}
EXPORT_SYMBOL_GPL(free_interface);


AltIf * alloc_alternate_interface(void)
{
	AltIf * altIf;
	altIf = ALLOC(sizeof(AltIf));
	altIf->bAlternateSetting = 0;
	altIf->ifConf = NULL;
	altIf->endpoints = NULL;
	return altIf;
}
EXPORT_SYMBOL_GPL(alloc_alternate_interface);

void associate_alternate_interface(AltIf * altif, IfConf * ifconf)
{
	List * altIfHook;

	altIfHook = ALLOC(sizeof(List));

	altIfHook->data = ALLOC(sizeof(AltIf*));
	*(AltIf**)altIfHook->data = altif;

	pushBack(&ifconf->alternateInterfaces, altIfHook);
	altif->ifConf = ifconf;
}
EXPORT_SYMBOL_GPL(associate_alternate_interface);

void associate_endpoint(EndpointDesc * endpoint, Configuration * conf, AltIf * altif)
{
	List * endpointListHook;

	endpoint->configuration_value = conf;
	endpoint->parent_altif = altif;

	endpointListHook = ALLOC(sizeof(List));

	endpointListHook->data = ALLOC(sizeof(EndpointDesc*));
	*(EndpointDesc**)endpointListHook->data = endpoint;

	pushBack(&altif->endpoints, endpointListHook);
}
EXPORT_SYMBOL_GPL(associate_endpoint);

void free_alternate_interface(AltIf * altif)
{
	removeList(&altif->endpoints);
	FREE(altif);
}
EXPORT_SYMBOL_GPL(free_alternate_interface);


void set_conf_data(UsbDeviceDesc * usb_device_desc, Configuration * conf)
{
	usb_device_desc->bConfigurationValue = *conf;
}

bool check_conf(UsbDeviceDesc * usb_device_desc, Configuration * conf)
{
	if ( usb_device_desc->bConfigurationValue == *conf )
		return true;
	else
	{
		USBIF_ERROR("This configuration is not active.\n");
		return false;
	}
}

void set_setting_data(AltIf * altif)
{
	List * endpointAltIfHook;
	List * endpointIfHook;

	reset_endpoint_links(altif->ifConf);

	//only associate an endpoint link to a real endpoint in case both have same address configuration
	for ( endpointAltIfHook = altif->endpoints; endpointAltIfHook != NULL; endpointAltIfHook = endpointAltIfHook->next )
	{
		for ( endpointIfHook = altif->ifConf->activeEndpointLinks; endpointIfHook != NULL; endpointIfHook = endpointIfHook->next )
		{
			if ( (*(EndpointDesc**)endpointAltIfHook->data)->bEndpointAddress == ((EndpointLink*)endpointIfHook->data)->bEndpointAddress )
				*((EndpointLink*)endpointIfHook->data)->pp_endpoint = *(EndpointDesc**)endpointAltIfHook->data;
		}
	}

	altif->ifConf->bAlternateSetting = altif->bAlternateSetting;
	altif->ifConf->activeInterface = altif;
}
EXPORT_SYMBOL_GPL(set_setting_data);

bool check_setting(AltIf * altif)
{
	if ( altif->ifConf->bAlternateSetting == altif->bAlternateSetting )
		return true;
	else
	{
		USBIF_ERROR("This alternate interface is not active.\n");
		return false;
	}
}
