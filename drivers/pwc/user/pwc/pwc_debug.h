/*
 * pwc_debug.h
 *
 *  Created on: Apr 15, 2010
 *      Author: rfajardo
 */

#ifndef PWC_DEBUG_H_
#define PWC_DEBUG_H_


#ifdef DEV_CLASS_INFO
#include <stdio.h>
#define INFO(fmt, args...) printf(fmt, ##args)
#else
#define INFO(fmt, args...)
#endif

#ifdef DEV_CLASS_TRANSFER_INFO
#include <stdio.h>
#define INFO_TRANSFER(fmt, args...) printf(fmt, ##args)
#else
#define INFO_TRANSFER(fmt, args...)
#endif


#endif /* PWC_DEBUG_H_ */
