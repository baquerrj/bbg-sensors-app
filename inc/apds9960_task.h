/*!
 * @file  apds9960_task.h
 * @brief 
 *
 *  <+DETAILED+>
 *
 * @author   Roberto Baquerizo (baquerrj), roba8460@colorado.edu
 *
 * @internal
 *       Created:    04/13/2019
 *       Revision:   none
 *       Compiler:   gcc
 *  Organization:    University of Colorado: Boulder
 *
 *  This source code is released for free distribution under the terms of the
 *  GNU General Public License as published by the Free Software Foundation.
 */


#ifndef  APDS9960_TASK_H
#define  APDS9960_TASK_H

#include "common.h"
#include "apds9960_sensor.h"

#define APDS9960_QUEUE "/apds9960-queue"

/**
 * Function:       get_apds9960_queue
 * @brief   Get file descriptor for light sensor thread.
 *          Called by watchdog thread in order to be able to send heartbeat check via queue
 *
 * @param   void
 * @return  temp_queue - file descriptor for light sensor thread message queue
 */
mqd_t get_apds9960_queue( void );


/*!
 * @brief 
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 */
uint8_t apds9960_i2c_init( void );



/**
 * Function:       apds9960_queue_init
 * @brief   Initialize message queue for light sensor thread
 *
 * @param   void
 * @return  msg_q - file descriptor for initialized message queue
 */
uint8_t apds9960_queue_init( void );

/**
 * Function:       apds9960_fn
 * @brief   Entry point for light sensor processing thread
 *
 * @param   thread_args - void ptr to arguments used to initialize thread
 * @return  NULL  - We don't really exit from this function,
 *                   since the exit point is thread_exit()
 */
void *apds9960_fn( void *thread_args );


#endif   /* APDS9960_TASK_H */
