/*
 * types.h
 *
 *  Created on: Jul 5, 2011
 *      Author: raul
 */

#ifndef TYPES_H_
#define TYPES_H_


#ifdef __KERNEL__

#include <linux/stddef.h>		//NULL, boolean
#include <linux/types.h>		//uint8_t...
#include <linux/kernel.h>		//INT_MAX
#include <linux/slab.h>			//malloc
#include <linux/mm.h>			//GPF_KERNEL
#include <linux/module.h>


#define ALLOC(size) kmalloc(size, GFP_KERNEL)		//this already makes it sleepable
#define INTALLOC(size) kmalloc(size, GFP_ATOMIC)	//this makes it not-sleepable
#define FREE(ptr) kfree(ptr)

#define UINT8_MAX	0xFF

#else

#include <stddef.h>				//NULL
#include <stdbool.h>			//boolean
#include <stdint.h>				//uint8_t...
#include <limits.h>				//INT_MAX
#include <stdlib.h>				//malloc

#define EXPORT_SYMBOL_GPL(name)

#define ALLOC(size) malloc(size)
#define INTALLOC(size) malloc(size)
#define FREE(ptr) free(ptr)

#endif


#define RETURNERROR 	INT_MAX			//possibly an error occurred


#endif /* TYPES_H_ */
