/*
 * usbint.c
 *
 *  Created on: Jul 5, 2011
 *      Author: raul
 */

#include "usbint.h"
#include "usbdata.h"
#include "usbifdebug.h"
#include "usbif.h"

#include "endpoint.h"
#include "process.h"
#include "locks.h"

#define STD_TIMEOUT 100		//100 ms

void remove_endpoint_handle(EndpointHandle * endpointHandle);

UsbDevice * alloc_usb_device(UsbDeviceDesc * udesc)
{
	UsbDevice * utmp;
	utmp = ALLOC(sizeof(UsbDevice));

	utmp->usb_device_desc = udesc;

	utmp->usb_context = NULL;
	utmp->usb_handler = NULL;

	utmp->detached_interfaces = NULL;

	utmp->ctrl0_buf = ALLOC(udesc->bMaxPacketSize0);
	utmp->ctrl0_timeout = STD_TIMEOUT;

	utmp->endpoints_handles = NULL;
#ifdef __KERNEL__
	utmp->workqueue = create_singlethread_workqueue("USB Interface Process");
#endif
	return utmp;
}
EXPORT_SYMBOL_GPL(alloc_usb_device);

void free_usb_device(UsbDevice * udev)
{
	List * listHook;
#ifdef __KERNEL__
	destroy_workqueue(udev->workqueue);
#endif
	while ( udev->endpoints_handles != NULL )
	{
		listHook = popFront(&udev->endpoints_handles);
		remove_endpoint_handle((EndpointHandle*)listHook->data);
		FREE(listHook);
	}

	removeList(&udev->detached_interfaces);

	FREE(udev->ctrl0_buf);
	FREE(udev);
}
EXPORT_SYMBOL_GPL(free_usb_device);

void add_detached_interface(UsbDevice * udev, uint8_t interface_nr)
{
	List * detachedListHook;
	struct detached_type * detached_if;

	detached_if = ALLOC(sizeof(struct detached_type));
	detached_if->interface_nr = interface_nr;

	detachedListHook = ALLOC(sizeof(List));
	detachedListHook->data = detached_if;

	pushBack(&udev->detached_interfaces, detachedListHook);
}

void add_endpoint(UsbDevice * udev, EndpointDesc * endpoint_desc)
{
	List * endpointListHook;
	EndpointHandle * endpointHandle;

	endpointHandle = ALLOC(sizeof(EndpointHandle));
	endpointHandle->process = create_process(udev->workqueue);
	endpointHandle->endpoint = ALLOC(sizeof(Endpoint));

	endpointHandle->endpoint->p_endpoint_desc = endpoint_desc;
	endpointHandle->endpoint->timeout = STD_TIMEOUT;

	INIT_SPINLOCK(&endpointHandle->endpoint->lock);

	endpointHandle->endpoint->complete_transfers = NULL;
	endpointHandle->endpoint->pending_transfers = NULL;

	endpointHandle->callback = NULL;

	endpointHandle->context = NULL;

	endpointListHook = ALLOC(sizeof(List));
	endpointListHook->data = endpointHandle;

	pushBack(&udev->endpoints_handles, endpointListHook);
}
EXPORT_SYMBOL_GPL(add_endpoint);

void remove_endpoint_handle(EndpointHandle * endpointHandle)
{
	destroy_process(endpointHandle->process);
	DEST_SPINLOCK(&endpointHandle->endpoint->lock);
	FREE(endpointHandle->endpoint);
	FREE(endpointHandle);
}

void close_alternate_interface(UsbDevice * udev, AltIf * altif)
{
	List * endpointListHook;
	EndpointHandle * endpointHandle;
	for ( endpointListHook = altif->endpoints; endpointListHook != NULL; endpointListHook = endpointListHook->next)
	{
		endpointHandle = get_endpoint_handle(udev, *(EndpointDesc**)endpointListHook->data);
		if ( endpointHandle != NULL )
		{
			endpointHandle->callback = NULL;
			finish_pending_threads(endpointHandle->process);
			unregister_endpoint_transfers(endpointHandle->endpoint);
			remove_threads(endpointHandle->process);
		}
	}
}

EndpointHandle * get_endpoint_handle(UsbDevice * udev, EndpointDesc * endpoint_desc)
{
	List * endpointListHook;
	for (endpointListHook = udev->endpoints_handles; endpointListHook != NULL; endpointListHook = endpointListHook->next)
	{
		if ( ((EndpointHandle*)(endpointListHook->data))->endpoint->p_endpoint_desc == endpoint_desc )
			return ((EndpointHandle*)(endpointListHook->data));
	}
	USBIF_ERROR("Endpoint's handle could not be found in endpoint_handles list of (UsbDevice * udev) data structure.\n");
	return NULL;
}
