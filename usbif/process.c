/*
 * process.c
 *
 *  Created on: Jul 21, 2012
 *      Author: raul
 */

#include "process.h"
#include "thread.h"

#include <devif/types.h>
#ifdef __KERNEL__
#include <linux/workqueue.h>
#endif


Process * create_process(workqueue_t * queue)
{
	Process * new_process;
	new_process = ALLOC(sizeof(Process));
	INIT_MUTEX(&new_process->mutex);
	new_process->queue = queue;
	new_process->threads = NULL;
	return new_process;
}

void destroy_process(Process * process)
{
	remove_threads(process);
	DEST_MUTEX(&process->mutex);
	FREE(process);
}


Thread * new_thread(Process * process)
{
	List * hook;
	Thread * new_thread;

	hook = ALLOC(sizeof(List));
	new_thread = create_thread();
	hook->data = new_thread;

	DOWN(&process->mutex);
	pushBack(&process->threads, hook);
	UP(&process->mutex);
	return new_thread;
}

void launch_thread(Process * process, Thread * thread)
{
#ifdef __KERNEL__
	queue_work(process->queue, thread->work);
#endif
}

void cancel_thread(Process * process, Thread * thread)
{
#ifdef __KERNEL__
	int iRet;
	iRet = cancel_work_sync(thread->work);
#endif
}


void finish_pending_threads(Process * process)
{
#ifdef __KERNEL__
	flush_workqueue(process->queue);
#endif
}

void remove_threads(Process * process)
{
#ifdef __KERNEL__
	int iRet;
#endif
	List * hook;
	DOWN(&process->mutex);
	while ( process->threads != NULL )
	{
		hook = popFront(&process->threads);
#ifdef __KERNEL__
		iRet = cancel_work_sync(((Thread*)hook->data)->work);
#endif
		destroy_thread((Thread*)hook->data);
		FREE(hook);
	}
	UP(&process->mutex);
}
