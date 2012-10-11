/*
 * thread.h
 *
 *  Created on: Jul 21, 2012
 *      Author: raul
 */

#ifndef THREAD_H_
#define THREAD_H_

#include "transfer.h"
#include "usbint.h"

#ifdef __KERNEL__
typedef struct work_struct work_t;
#else
typedef int work_t;
#endif

struct thread_data
{
	//private
	work_t				  		work;
	//public
	EndpointHandle *		 	endpointHandle;
	Transfer * 					transfer;
};

struct thread
{
	work_t *			 	work;
	struct thread_data *	data;
};
typedef struct thread Thread;
typedef struct thread_data ThreadData;

Thread * create_thread(void);
void initialize_thread(Thread * thread, void (*func)(work_t*), struct thread_data * data);
void destroy_thread(Thread * thread);

struct thread_data * get_thread_data(work_t * work);
Thread * get_thread(struct thread_data ** data);

#endif /* THREAD_H_ */
