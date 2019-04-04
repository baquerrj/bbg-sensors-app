/**
 * =================================================================================
 *    @file     watchdog.h
 *    @brief   Watchdog thread header 
 *
 *
 *    @author   Roberto Baquerizo (baquerrj), roba8460@colorado.edu
 *
 *    @internal
 *       Created:  03/15/2019
 *      Revision:  none
 *      Compiler:  gcc
 *  Organization:  University of Colorado: Boulder
 *
 *  This source code is released for free distribution under the terms of the
 *  GNU General Public License as published by the Free Software Foundation.
 * =================================================================================
 */


#ifndef  WATCHDOG_H
#define  WATCHDOG_H

#include "common.h"

#include <mqueue.h>

#define WATCHDOG_QUEUE_NAME "/watchdog-queue"
#define NUM_THREADS 4

typedef enum {
   THREAD_TEMP = 0,
   THREAD_LIGHT,
   THREAD_LOGGER,
   THREAD_SOCKET,
   THREAD_MAX
} thread_e;

volatile int threads_status[NUM_THREADS];

extern pthread_mutex_t alive_mutex;

/**
 * =================================================================================
 * Function:       kill_threads
 * @brief   Function to kill children threads 
 *
 * @param   void
 * @return  void
 * =================================================================================
 */
void kill_threads( void );

/**
 * =================================================================================
 * Function:       check_threads
 * @brief   Periodically send message via message queue for temperature and sensor threads
 *          to check for health. This function is registered as the timer hanlder for the
 *          timer owned by the watchdog
 *
 * @param   sig
 * @return  void
 * =================================================================================
 */
void check_threads( union sigval sig );

/**
 * =================================================================================
 * Function:       watchdog_queue_init
 * @brief   Initalize message queue for watchdog
 *
 * @param   void
 * @return  msg_q - file descriptor for initialized message queue
 * =================================================================================
 */
int watchdog_queue_init( void );

/**
 * =================================================================================
 * Function:       watchdog_init
 * @brief   Initialize watchdog, calling appropriate functions to do so.
 *          E.g. calling timer_setup and timer_start to set up timer
 *
 * @param   void
 * @return  EXIT_CLEAN, otherwise EXIT_INIT
 * =================================================================================
 */
int watchdog_init( void );

/**
 * =================================================================================
 * Function:       watchdog_fn
 * @brief   Entry point for wachtdog
 *
 * @param   thread_args - void ptr used to pass thread identifiers (pthread_t) for
 *                      child threads we have to check for health
 * @return  NULL  - We don't really exit from this function,
 *                   since the exit point for threads is thread_exit()
 * =================================================================================
 */
void *watchdog_fn( void *thread_args );

#endif   /* WATCHDOG_H */
