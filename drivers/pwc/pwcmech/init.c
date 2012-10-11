/*
 * init.c
 *
 *  Created on: Nov 9, 2011
 *      Author: raul
 */

#include <devif/types.h>
#include <usbif/usbif.h>
#include <devif/com.h>
#include "pwcblock.h"
#include "usbcom.h"

#include "pwcmech_init.h"
#include "pwcmech_if.h"
#include "pwcmech.h"

#include "pwcmechdebug.h"

#ifdef CONFIG_PWCMECH_DEBUG
int pwcmech_trace = PWCMECH_DEBUG_LEVEL;
#endif

static bool pwcmech_start = false;

#ifdef __KERNEL__

#include <linux/init.h>

MODULE_LICENSE("Dual BSD/GPL");

#ifdef CONFIG_PWCMECH_DEBUG
module_param_named(trace, pwcmech_trace, int, S_IRUGO | S_IWUSR);
#endif

int pwcmech_register_driver(struct pwcmech * pwcmech, struct usb_driver * drv)
{
	PWCMECH_DEBUG_INIT("Registering driver to Philips Webcam Types\n");
	return usb_init(pwcmech->com, drv);
}
EXPORT_SYMBOL_GPL(pwcmech_register_driver);

void pwcmech_deregister_driver(struct pwcmech * pwcmech)
{
	PWCMECH_DEBUG_INIT("Deregistering driver, Philips Webcams will not activate this driver any longer.\n");
	return usb_exit(pwcmech->com);
}
EXPORT_SYMBOL_GPL(pwcmech_deregister_driver);

int pwcmech_register_handler(struct pwcmech * pwcmech, struct usb_device * dev)
{
	PWCMECH_DEBUG_INIT("Registering device handler with internal USB API.\n");
	return usb_open(pwcmech->com, dev);
}
EXPORT_SYMBOL_GPL(pwcmech_register_handler);

#else
int pwcmech_register_driver(struct pwcmech * pwcmech)
{
	PWCMECH_DEBUG_INIT("Registering driver to Philips Webcam Types\n");
	return usb_init(pwcmech->com);
}

void pwcmech_deregister_driver(struct pwcmech * pwcmech)
{
	PWCMECH_DEBUG_INIT("Deregistering driver, Philips Webcams will not activate this driver any longer.\n");
	return usb_exit(pwcmech->com);
}

int pwcmech_register_handler(struct pwcmech * pwcmech)
{
	PWCMECH_DEBUG_INIT("Registering device handler with internal USB API.\n");
	return usb_open(pwcmech->com);
}
#endif

void pwcmech_deregister_handler(struct pwcmech * pwcmech)
{
	PWCMECH_DEBUG_INIT("Disconnecting device handler from internal USB API.\n");
	usb_close(pwcmech->com);
}
EXPORT_SYMBOL_GPL(pwcmech_deregister_handler);

struct pwcmech * pwc_devmech_start(void)
{
	struct pwcmech * pwcmech;

	if ( pwcmech_start )
		return NULL;

	pwcmech = ALLOC(sizeof(struct pwcmech));

	pwcmech->efsm = init_efsm();
	pwcmech->dev = ifinit_pwcblock();
	pwcmech->com = ifinit_usbcom();

	ifbind_pwcblock(pwcmech->dev, pwcmech->com);

	pwcmech_start = true;

	PWCMECH_INFO("Memory allocated to hold device control structures.\n");
	PWCMECH_DEBUG_INIT("Device registers bound to their USB packets for transmission. USB communication bound to device register API.\n");
	return pwcmech;
}
EXPORT_SYMBOL_GPL(pwc_devmech_start);


void pwc_devmech_stop(struct pwcmech * pwcmech)
{
	if ( !pwcmech_start )
		return;

	stop_efsm(pwcmech->efsm);
	ifstop_pwcblock(pwcmech->dev);
	ifstop_usbcom(pwcmech->com);
	FREE(pwcmech);

	pwcmech_start = false;

	PWCMECH_INFO("Previous allocated memory to hold device control structures freed.\n");
}
EXPORT_SYMBOL_GPL(pwc_devmech_stop);


int pwc_devmech_init(void)
{
	PWCMECH_INFO("Memory allocated to hold communication structures.\n");
	return 0;
}
EXPORT_SYMBOL_GPL(pwc_devmech_init);

void pwc_devmech_exit(void)
{
	PWCMECH_INFO("Previous allocated memory to hold communication structures freed.\n");
}
EXPORT_SYMBOL_GPL(pwc_devmech_exit);


UsbDevice * pwc_get_usb_device(struct pwcmech * pwcmech)
{
	PWCMECH_DEBUG_INIT("Giving access to USB handler: returning its pointer.\n");
	return pwcmech->com->handle;
}
EXPORT_SYMBOL_GPL(pwc_get_usb_device);


#ifdef __KERNEL__
static int __init init(void)
{
	PWCMECH_INFO("Module pwc_devmech registering the pwc access API.\n");
	return 0;
}

static void __exit exit(void)
{
	PWCMECH_INFO("Unregistering pwc_devmech, this unables the pwc access API.\n");
}

module_init(init);
module_exit(exit);
#endif

