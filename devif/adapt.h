/*
 * adapt.h
 *
 *  Created on: May 16, 2011
 *      Author: rfajardo
 */

#ifndef ADAPT_H_
#define ADAPT_H_

#include "regtypes.h"


void adaptContentSiblings(RegField * regField);

void adaptWriteAsReadFields(Reg * reg, unsigned int read_value);
void adaptWriteAsReadSiblings(RegField * regField, unsigned int read_value);

void adaptContentFieldList(List * fieldList);
void adaptWriteAsReadFieldList(List * fieldList, unsigned int read_value);


#endif /* ADAPT_H_ */
