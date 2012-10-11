/*
 * listutil.c
 *
 *  Created on: Jun 27, 2011
 *      Author: raul
 */

#include "listutil.h"
#include "types.h"

List * popFront(List ** list)
{
	List * hook;
	if ( *list != NULL )
	{
		hook = *list;
		*list = (*list)->next;
		return hook;
	}
	return NULL;
}
EXPORT_SYMBOL_GPL(popFront);

void pushBack(List ** list, List * hook)
{
	if ( hook != NULL )
	{
		for (; *list != NULL; list = &(*list)->next );
		*list = hook;
		(*list)->next = NULL;
	}
}
EXPORT_SYMBOL_GPL(pushBack);

void removeList(List ** list)
{
	List * hook;
	while (*list != NULL)
	{
		hook = popFront(list);
		FREE(hook->data);
		FREE(hook);
	}
}
EXPORT_SYMBOL_GPL(removeList);

List * popElement(List ** list, void * data)
{
	List * hook;
	for (; *list != NULL; list = &(*list)->next )
	{
		if ( (*list)->data == data )
		{
			hook = *list;
			*list = (*list)->next;
			return hook;
		}
	}
	return NULL;
}
EXPORT_SYMBOL_GPL(popElement);
