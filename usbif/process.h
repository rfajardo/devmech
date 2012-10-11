/*
 * process.h
 *
 *  Created on: Jul 21, 2012
 *      Author: raul
 */

#ifndef PROCESS_H_
#define PROCESS_H_

#include "thread.h"
#include <devif/listutil.h>
#include "locks.h"

#ifdef __KERNEL__
#define workqueue_t struct workqueue_struct
#else
#define workqueue_t int
#endif

struct process
{
	workqueue_t *			 	queue;
	MUTEX						mutex;
	List *						threads;
};
typedef struct process Process;

Process * create_process(workqueue_t * queue);
void destroy_process(Process * process);

void finish_pending_threads(Process * process);
void remove_threads(Process * process);

Thread * new_thread(Process * process);

void launch_thread(Process * process, Thread * thread);
void cancel_thread(Process * process, Thread * thread);


#endif /* PROCESS_H_ */
