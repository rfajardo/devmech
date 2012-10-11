/*
 * locks.h
 *
 *  Created on: Aug 2, 2012
 *      Author: raul
 */

#ifndef LOCKS_H_
#define LOCKS_H_

#ifdef __KERNEL__
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/delay.h>

#define MSLEEP(MSEC) mdelay(MSEC)

#define MUTEX struct mutex
#define INIT_MUTEX(mutex) mutex_init(mutex)
#define DOWN(mutex) mutex_lock(mutex)
#define UP(mutex) mutex_unlock(mutex)
#define DEST_MUTEX(mutex)

#define SPINLOCK spinlock_t
#define INIT_SPINLOCK(spinlock) spin_lock_init(spinlock)
#define LOCK(spinlock, flags) spin_lock_irqsave(spinlock, flags)
#define UNLOCK(spinlock, flags) spin_unlock_irqrestore(spinlock, flags)
#define DEST_SPINLOCK(mutex)

#else
#include <pthread.h>
#include <unistd.h>

#define MSLEEP(MSEC) usleep(1000*MSEC)

#define MUTEX pthread_mutex_t
#define INIT_MUTEX(mutex) pthread_mutex_init(mutex, NULL)
#define DOWN(mutex) pthread_mutex_lock(mutex)
#define UP(mutex) pthread_mutex_unlock(mutex)
#define DEST_MUTEX(mutex) pthread_mutex_destroy(mutex)

#define SPINLOCK pthread_mutex_t
#define INIT_SPINLOCK(spinlock) pthread_mutex_init(spinlock, NULL)
#define LOCK(spinlock, flags) pthread_mutex_lock(spinlock)
#define UNLOCK(spinlock, flags) pthread_mutex_unlock(spinlock)
#define DEST_SPINLOCK(mutex) pthread_mutex_destroy(mutex)

#endif


#endif /* LOCKS_H_ */
