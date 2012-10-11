/*
 * usbifdebug.h
 *
 *  Created on: Jul 27, 2011
 *      Author: raul
 */

#ifndef USBIFDEBUG_H_
#define USBIFDEBUG_H_


#define DEVNAME "usbif: "

#define usbif_trace 				0x00
//#define SIMULATE					1
//#define SHOWTRANSFERS				1


/* Trace certain actions in the driver */
#define USBIF_DEBUG_LEVEL_IF		(1<<0)
#define USBIF_DEBUG_LEVEL_CONF		(1<<1)
#define USBIF_DEBUG_LEVEL_COM		(1<<2)
#define USBIF_DEBUG_LEVEL_USB		(1<<3)
#define USBIF_DEBUG_LEVEL_LISTS		(1<<4)
#define USBIF_DEBUG_LEVEL_ISOPROC	(1<<5)
#define USBIF_DEBUG_LEVEL_TRACE		(1<<8)



#define USBIF_DEBUG_IF(fmt, args...)		USBIF_DEBUG(IF, fmt, ##args)
#define USBIF_DEBUG_CONF(fmt, args...) 		USBIF_DEBUG(CONF, fmt, ##args)
#define USBIF_DEBUG_COM(fmt, args...) 		USBIF_DEBUG(COM, fmt, ##args)
#define USBIF_DEBUG_USB(fmt, args...) 		USBIF_DEBUG(USB, fmt, ##args)
#define USBIF_DEBUG_LISTS(fmt, args...)		USBIF_DEBUG(LISTS, fmt, ##args)
#define USBIF_DEBUG_ISOPROC(fmt, args...)	USBIF_DEBUG(ISOPROC, fmt, ##args)
#define USBIF_DEBUG_TRACE(fmt, args...) 	USBIF_DEBUG(TRACE, fmt, ##args)



#ifdef CONFIG_USBIF_DEBUG

#ifdef __KERNEL__
#define USBIF_DEBUG(level, fmt, args...) do {\
	  if ((USBIF_DEBUG_LEVEL_ ##level) & usbif_trace) \
	     printk(KERN_DEBUG DEVNAME #level ": " fmt, ##args); \
	  } while(0)

#define USBIF_ERROR(fmt, args...) printk(KERN_ERR DEVNAME fmt, ##args)
#define USBIF_WARNING(fmt, args...) printk(KERN_WARNING DEVNAME fmt, ##args)
#define USBIF_INFO(fmt, args...) printk(KERN_INFO DEVNAME fmt, ##args)
#define USBIF_TRACE(fmt, args...) USBIF_DEBUG(TRACE, fmt, ##args)

#else
#include <stdio.h>
#define USBIF_DEBUG(level, fmt, args...) do {\
	  if ((USBIF_DEBUG_LEVEL_ ##level) & usbif_trace) \
		  fprintf(stderr, "DEBUG: " DEVNAME #level ": " fmt, ##args); \
	  } while(0)

#define USBIF_ERROR(fmt, args...) fprintf(stderr, "ERROR: " DEVNAME fmt, ##args)
#define USBIF_WARNING(fmt, args...) fprintf(stderr, "WARNING: " DEVNAME fmt, ##args)
#define USBIF_INFO(fmt, args...) fprintf(stderr, "INFO: " DEVNAME fmt, ##args)
#define USBIF_TRACE(fmt, args...) USBIF_DEBUG(TRACE, fmt, ##args)

#endif

#else /* if !CONFIG_USBIF_DEBUG */

#ifdef __KERNEL__
#define USBIF_ERROR(fmt, args...) printk(KERN_ERR DEVNAME fmt, ##args)
#define USBIF_WARNING(fmt, args...) printk(KERN_WARNING DEVNAME fmt, ##args)
#define USBIF_INFO(fmt, args...) printk(KERN_INFO DEVNAME fmt, ##args)
#define USBIF_TRACE(fmt, args...) do { } while(0)
#define USBIF_DEBUG(level, fmt, args...) do { } while(0)

#else
#include <stdio.h>
#define USBIF_ERROR(fmt, args...) fprintf(stderr, "ERROR: " DEVNAME fmt, ##args)
#define USBIF_WARNING(fmt, args...) fprintf(stderr, "WARNING: " DEVNAME fmt, ##args)
#define USBIF_INFO(fmt, args...) fprintf(stderr, "INFO: " DEVNAME fmt, ##args)
#define USBIF_TRACE(fmt, args...) do { } while(0)
#define USBIF_DEBUG(level, fmt, args...) do { } while(0)

#endif

#endif


#endif /* USBIFDEBUG_H_ */
