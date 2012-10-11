/*
 * comif.h
 *
 *  Created on: Nov 10, 2011
 *      Author: raul
 */

#ifndef COMIF_H_
#define COMIF_H_

enum com_error;

void setComError(enum com_error err);
enum com_error getComError(void);
unsigned int readIf(void * com, unsigned int addr, void * if_complex_data);
enum com_error writeIf(void * com, unsigned int addr, void * if_complex_data, unsigned int value);

#endif /* COMIF_H_ */
