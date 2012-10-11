/*
 * lsusb.h
 *
 *  Created on: Jun 20, 2011
 *      Author: rfajardo
 */

#ifndef LSUSB_H_
#define LSUSB_H_


#define VERBLEVEL_DEFAULT 0	/* 0 gives lspci behaviour; 1, lsusb-0.9 */

#define CTRL_RETRIES	 2
#define CTRL_TIMEOUT	(5*1000)	/* milliseconds */


int lprintf(unsigned int vl, const char *format, ...);



#endif /* LSUSB_H_ */
