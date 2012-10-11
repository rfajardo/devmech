/*
 * devcontract.h
 *
 *  Created on: Aug 6, 2012
 *      Author: raul
 */

#ifndef DEVCONTRACT_H_
#define DEVCONTRACT_H_


#include <usbif/streamif.h>

#ifndef CONFIG_DEVCONTRACT_DEBUG
#define DEVCONTRACT_DEBUG(fmt, args...) do { } while(0)
#else
#ifdef __KERNEL__
#define DEVCONTRACT_DEBUG(fmt, args...) printk(KERN_DEBUG "DEVCONTRACT: " fmt, ##args)
#else
#include <stdio.h>
#define DEVCONTRACT_DEBUG(fmt, args...) fprintf(stderr, "DEBUG: DEVCONTRACT : " fmt, ##args)
#endif
#endif


#ifdef __KERNEL__
#include <linux/mutex.h>
#include <linux/delay.h>

#define MSLEEP(MSEC) mdelay(MSEC)

#define STATEMUTEX struct rw_semaphore
#define INIT_MUTEX(MUTEX) init_rwsem(MUTEX)
#define DEST_MUTEX(MUTEX)

#define DOWN_READ(MUTEX) down_read(MUTEX)
#define UP_READ(MUTEX) up_read(MUTEX)

#define DOWN_WRITE(MUTEX) down_write(MUTEX)
#define UP_WRITE(MUTEX) up_write(MUTEX)

#else
#include <pthread.h>
#include <unistd.h>

#define MSLEEP(MSEC) usleep(1000*MSEC)

#define STATEMUTEX pthread_mutex_t
#define INIT_MUTEX(MUTEX) pthread_mutex_init(MUTEX, NULL)
#define DEST_MUTEX(MUTEX) pthread_mutex_destroy(MUTEX)

#define DOWN_READ(MUTEX) pthread_mutex_lock(MUTEX)
#define UP_READ(MUTEX) pthread_mutex_unlock(MUTEX)

#define DOWN_WRITE(MUTEX) pthread_mutex_lock(MUTEX)
#define UP_WRITE(MUTEX) pthread_mutex_unlock(MUTEX)

#endif


enum idl_error
{
    OPERATION_OK = 0,
    INVALID_STATE = -100
};


#endif /* DEVCONTRACT_H_ */
