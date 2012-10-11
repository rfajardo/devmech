/*
 * thread.c
 *
 *  Created on: Jul 21, 2012
 *      Author: raul
 */

#include "thread.h"
#include <devif/types.h>
#ifdef __KERNEL__
#include <linux/workqueue.h>
#else
#define _offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({            \
		const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
		(type *)( (char *)__mptr - _offsetof(type,member) );})
#endif


Thread * create_thread(void)
{
	Thread * new_thread;
	new_thread = ALLOC(sizeof(Thread));
	new_thread->data = ALLOC(sizeof(ThreadData));
	new_thread->work = &new_thread->data->work;
	return new_thread;
}

void initialize_thread(Thread * thread, void (*func)(work_t*), struct thread_data * data)
{
	thread->data->endpointHandle = data->endpointHandle;
	thread->data->transfer = data->transfer;
#ifdef __KERNEL__
	INIT_WORK(thread->work, func);
#endif
}

void destroy_thread(Thread * thread)
{
	FREE(thread->data);
	FREE(thread);
}

struct thread_data * get_thread_data(work_t * work)
{
	ThreadData * my_thread_data;
	my_thread_data = container_of(work, ThreadData, work);
	return my_thread_data;
}

Thread * get_thread(struct thread_data ** data)
{
	Thread * my_thread;
	my_thread = container_of(data, Thread, data);
	return my_thread;
}
