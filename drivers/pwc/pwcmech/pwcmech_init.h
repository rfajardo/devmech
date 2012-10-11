/*
 * pwcmech_init.h
 *
 *  Created on: Nov 14, 2011
 *      Author: raul
 */

#ifndef PWCMECH_INIT_H_
#define PWCMECH_INIT_H_

#include <usbif/usbint.h>

struct pwcmech;

#ifdef __KERNEL__
int pwcmech_register_driver(struct pwcmech * pwcmech, struct usb_driver * drv);
int pwcmech_register_handler(struct pwcmech * pwcmech, struct usb_device * dev);
#else
int pwcmech_register_driver(struct pwcmech * pwcmech);
int pwcmech_register_handler(struct pwcmech * pwcmech);
#endif
void pwcmech_deregister_driver(struct pwcmech * pwcmech);
void pwcmech_deregister_handler(struct pwcmech * pwcmech);

struct pwcmech * pwc_devmech_start(void);
void pwc_devmech_stop(struct pwcmech * pwcmech);

int pwc_devmech_init(void);
void pwc_devmech_exit(void);

UsbDevice * pwc_get_usb_device(struct pwcmech * pwcmech);


#endif /* PWCMECH_INIT_H_ */
