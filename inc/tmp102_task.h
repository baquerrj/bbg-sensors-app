/*!
 *    @file     temperature.h
 *    @brief   Header for temperature sensor thread 
 *
 *
 *    @author   Roberto Baquerizo (baquerrj), roba8460@colorado.edu
 *
 *    @internal
 *       Created:  03/09/2019
 *      Revision:  none
 *      Compiler:  gcc
 *  Organization:  University of Colorado: Boulder
 *
 *  This source code is released for free distribution under the terms of the
 *  GNU General Public License as published by the Free Software Foundation.
 */

#ifndef TMP102_TASK_H
#define TMP102_TASK_H

#include "common.h"

#include <mqueue.h>

#define TMP102_QUEUE "/tmp102-queue" 

/*!
 * Function:       tmp102_handler
 * @brief   Timer handler function for temperature sensor thread
 *          When woken up by the timer, get temperature and write state to shared memory
 *
 * @param   sig
 * @return  void
 */
void tmp102_handler( union sigval sig );

/*!
 * Function:       tmp102_cycle
 * @brief   Cycle function for temperature sensor thread
 *          We wait in this while loop checking for requests from watchdog for health status
 *
 * @param   void
 * @return  void 
 */
void tmp102_cycle( void );


/*!
 * Function:       get_temperature
 * @brief   Returns last temperature reading we have
 *
 * @param   void
 * @return  last_temp_value - last temperature reading we have
 * <+DETAILED+>
 */
float get_temperature( void );


/*!
 * Function:       get_tmp102_queue
 * @brief   Get file descriptor for temperature sensor thread.
 *          Called by watchdog thread in order to be able to send heartbeat check via queue
 *
 * @param   void
 * @return  temp_queue - file descriptor for temperature sensor thread message queue
 */
mqd_t get_tmp102_queue( void );

/*!
 * Function:       temp_queue_init
 * @brief   Initialize message queue for temperature sensor thread
 *
 * @param   void
 * @return  msg_q - file descriptor for initialized message queue
 */
int temp_queue_init( void );

/*!
 * Function:       tmp102_fn
 * @brief   Entry point for temperature sensor processing thread
 *
 * @param   thread_args  - void ptr to arguments used to initialize thread
 * @return  NULL  - We don't really exit from this function, 
 *                   since the exit point is thread_exit()
 */
void *tmp102_fn( void *thread_args );


#endif /* TMP102_TASK_H */
