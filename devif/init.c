/*
 * init.c
 *
 *  Created on: Nov 9, 2011
 *      Author: raul
 */

#include "types.h"

#ifdef __KERNEL__

#include <linux/init.h>

MODULE_LICENSE("Dual BSD/GPL");

static int __init devif_init(void)
{
	printk(KERN_ALERT "Module devif registering the device register access API. No memory allocated, this is an abstract interface.\n");
	return 0;
}

static void __exit devif_exit(void)
{
	printk(KERN_ALERT "Unregistering devif, this unables the device register access API.\n");
}

module_init(devif_init);
module_exit(devif_exit);

#else

#endif
