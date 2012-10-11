/*
 * pwcmechdebug.h
 *
 *  Created on: Nov 15, 2011
 *      Author: rfajardo
 */

#ifndef PWCMECHDEBUG_H_
#define PWCMECHDEBUG_H_


#define DEVNAME "pwcmech: "


/* Trace certain actions in the driver */
#define PWCMECH_DEBUG_LEVEL_INIT		(1<<0)
#define PWCMECH_DEBUG_LEVEL_MECH		(1<<1)

#define PWCMECH_DEBUG_INIT(fmt, args...) 		PWCMECH_DEBUG(INIT, fmt, ##args)
#define PWCMECH_DEBUG_MECH(fmt, args...) 		PWCMECH_DEBUG(MECH, fmt, ##args)


#ifdef CONFIG_PWCMECH_DEBUG

extern int pwcmech_trace;

#ifdef __KERNEL__
#define PWCMECH_DEBUG(level, fmt, args...) do {\
	  if ((PWCMECH_DEBUG_LEVEL_ ##level) & pwcmech_trace) \
	     printk(KERN_DEBUG DEVNAME #level ": " fmt, ##args); \
	  } while(0)

#define PWCMECH_ERROR(fmt, args...) printk(KERN_ERR DEVNAME fmt, ##args)
#define PWCMECH_WARNING(fmt, args...) printk(KERN_WARNING DEVNAME fmt, ##args)
#define PWCMECH_INFO(fmt, args...) printk(KERN_INFO DEVNAME fmt, ##args)
#define PWCMECH_TRACE(fmt, args...) PWCMECH_DEBUG(TRACE, fmt, ##args)

#else
#include <stdio.h>
#define PWCMECH_DEBUG(level, fmt, args...) do {\
	  if ((PWCMECH_DEBUG_LEVEL_ ##level) & pwcmech_trace) \
		  fprintf(stderr, "DEBUG: " DEVNAME #level ": " fmt, ## args); \
	  } while(0)

#define PWCMECH_ERROR(fmt, args...) fprintf(stderr, "ERROR: " DEVNAME fmt, ##args)
#define PWCMECH_WARNING(fmt, args...) fprintf(stderr, "WARNING: " DEVNAME fmt, ##args)
#define PWCMECH_INFO(fmt, args...) fprintf(stderr, "INFO: " DEVNAME fmt, ##args)
#define PWCMECH_TRACE(fmt, args...) PWCMECH_DEBUG(TRACE, fmt, ##args)

#endif

#else /* if !CONFIG_PWCMECH_DEBUG */

#ifdef __KERNEL__
#define PWCMECH_ERROR(fmt, args...) printk(KERN_ERR DEVNAME fmt, ##args)
#define PWCMECH_WARNING(fmt, args...) printk(KERN_WARNING DEVNAME fmt, ##args)
#define PWCMECH_INFO(fmt, args...) printk(KERN_INFO DEVNAME fmt, ##args)
#define PWCMECH_TRACE(fmt, args...) do { } while(0)
#define PWCMECH_DEBUG(level, fmt, args...) do { } while(0)

#else
#include <stdio.h>
#define PWCMECH_ERROR(fmt, args...) fprintf(stderr, "ERROR: " DEVNAME fmt, ##args)
#define PWCMECH_WARNING(fmt, args...) fprintf(stderr, "WARNING: " DEVNAME fmt, ##args)
#define PWCMECH_INFO(fmt, args...) fprintf(stderr, "INFO: " DEVNAME fmt, ##args)
#define PWCMECH_TRACE(fmt, args...) do { } while(0)
#define PWCMECH_DEBUG(level, fmt, args...) do { } while(0)

#endif

#endif


#endif /* PWCMECHDEBUG_H_ */
