/*
 * devifdebug.h
 *
 *  Created on: May 24, 2011
 *      Author: rfajardo
 */

#ifndef DEVIFDEBUG_H_
#define DEVIFDEBUG_H_


#define DEVNAME "devif: "


/* Trace certain actions in the driver */
#define DEVIF_DEBUG_LEVEL_IF		(1<<0)
#define DEVIF_DEBUG_LEVEL_RULES		(1<<1)
#define DEVIF_DEBUG_LEVEL_MODIFY	(1<<2)
#define DEVIF_DEBUG_LEVEL_ADAPT		(1<<3)
#define DEVIF_DEBUG_LEVEL_REGOP		(1<<4)
#define DEVIF_DEBUG_LEVEL_COM		(1<<5)
#define DEVIF_DEBUG_LEVEL_CACHE		(1<<6)
#define DEVIF_DEBUG_LEVEL_TRACE		(1<<8)

#define devif_trace 				0x00	//all levels up to com (cache and trace out)


#define DEVIF_DEBUG_IF(fmt, args...) 		DEVIF_DEBUG(IF, fmt, ##args)
#define DEVIF_DEBUG_RULES(fmt, args...) 	DEVIF_DEBUG(RULES, fmt, ##args)
#define DEVIF_DEBUG_MODIFY(fmt, args...)	DEVIF_DEBUG(MODIFY, fmt, ##args)
#define DEVIF_DEBUG_ADAPT(fmt, args...) 	DEVIF_DEBUG(ADAPT, fmt, ##args)
#define DEVIF_DEBUG_REGOP(fmt, args...)		DEVIF_DEBUG(REGOP, fmt, ##args)
#define DEVIF_DEBUG_COM(fmt, args...) 		DEVIF_DEBUG(COM, fmt, ##args)
#define DEVIF_DEBUG_CACHE(fmt, args...) 	DEVIF_DEBUG(CACHE, fmt, ##args)
#define DEVIF_DEBUG_TRACE(fmt, args...) 	DEVIF_DEBUG(TRACE, fmt, ##args)



#ifdef CONFIG_DEVIF_DEBUG

#ifdef __KERNEL__
#define DEVIF_DEBUG(level, fmt, args...) do {\
	  if ((DEVIF_DEBUG_LEVEL_ ##level) & devif_trace) \
	     printk(KERN_DEBUG DEVNAME #level ": " fmt, ##args); \
	  } while(0)

#define DEVIF_ERROR(fmt, args...) printk(KERN_ERR DEVNAME fmt, ##args)
#define DEVIF_WARNING(fmt, args...) printk(KERN_WARNING DEVNAME fmt, ##args)
#define DEVIF_INFO(fmt, args...) printk(KERN_INFO DEVNAME fmt, ##args)
#define DEVIF_TRACE(fmt, args...) DEVIF_DEBUG(TRACE, fmt, ##args)

#else
#include <stdio.h>
#define DEVIF_DEBUG(level, fmt, args...) do {\
	  if ((DEVIF_DEBUG_LEVEL_ ##level) & devif_trace) \
		  fprintf(stderr, "DEBUG: " DEVNAME #level ": " fmt, ## args); \
	  } while(0)

#define DEVIF_ERROR(fmt, args...) fprintf(stderr, "ERROR: " DEVNAME fmt, ##args)
#define DEVIF_WARNING(fmt, args...) fprintf(stderr, "WARNING: " DEVNAME fmt, ##args)
#define DEVIF_INFO(fmt, args...) fprintf(stderr, "INFO: " DEVNAME fmt, ##args)
#define DEVIF_TRACE(fmt, args...) DEVIF_DEBUG(TRACE, fmt, ##args)

#endif

#else /* if !CONFIG_DEVIF_DEBUG */

#ifdef __KERNEL__
#define DEVIF_ERROR(fmt, args...) printk(KERN_ERR DEVNAME fmt, ##args)
#define DEVIF_WARNING(fmt, args...) printk(KERN_WARNING DEVNAME fmt, ##args)
#define DEVIF_INFO(fmt, args...) printk(KERN_INFO DEVNAME fmt, ##args)
#define DEVIF_TRACE(fmt, args...) do { } while(0)
#define DEVIF_DEBUG(level, fmt, args...) do { } while(0)

#else
#include <stdio.h>
#define DEVIF_ERROR(fmt, args...) fprintf(stderr, "ERROR: " DEVNAME fmt, ##args)
#define DEVIF_WARNING(fmt, args...) fprintf(stderr, "WARNING: " DEVNAME fmt, ##args)
#define DEVIF_INFO(fmt, args...) fprintf(stderr, "INFO: " DEVNAME fmt, ##args)
#define DEVIF_TRACE(fmt, args...) do { } while(0)
#define DEVIF_DEBUG(level, fmt, args...) do { } while(0)

#endif

#endif


#endif /* DEVIFDEBUG_H_ */
