/*
 * init.c
 *
 *  Created on: Nov 9, 2011
 *      Author: raul
 */

#include <devif/types.h>

#ifdef __KERNEL__

#include <linux/init.h>

MODULE_LICENSE("Dual BSD/GPL");

static int __init usbif_init(void)
{
	printk(KERN_ALERT "Module usbif registering the USB access API. No memory allocated, this is an abstract interface.\n");
	return 0;
}

static void __exit usbif_exit(void)
{
	printk(KERN_ALERT "Unregistering usbif, this unables the USB access API.\n");
}

module_init(usbif_init);
module_exit(usbif_exit);

#else

#endif
