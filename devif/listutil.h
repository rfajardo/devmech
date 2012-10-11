/*
 * listutil.h
 *
 *  Created on: Jun 27, 2011
 *      Author: raul
 */

#ifndef LISTUTIL_H_
#define LISTUTIL_H_


struct list_type
{
	struct list_type * next;
	void * data;
};

typedef struct list_type List;


List * popFront(List ** list);
void pushBack(List ** list, List * hook);
void removeList(List ** list);

List * popElement(List ** list, void * data);

#endif /* LISTUTIL_H_ */
