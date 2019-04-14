/*!
 *    @file     temperature.c
 *    @brief   Source file implementing temperature.h
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

#include "tmp102_sensor.h"
#include "tmp102_task.h"
#include "watchdog.h"
#include "led.h"
#include "logger.h"

#include <errno.h>
#include <time.h>
#include <string.h>

static timer_t    timerid;
struct itimerspec trigger;

static i2c_handle_t i2c_tmp102;
static float last_temp_value = -5;
static mqd_t temp_queue;

static message_t temp_log = {
   .level      = LOG_INFO,
   .timestamp  = {0},
   .id         = MSG_STATUS,
   .src        = TASK_TEMP,
   .msg        = {0}
};



/*!
 * Function:       sig_handler
 * @brief   Signal handler for temperature sensor thread.
 *          On normal operation, we should be receving SIGUSR1/2 signals from watchdog
 *          when prompted to exit. So, we close the message queue and timer this thread owns
 *
 * @param   signo - enum with signal number of signal being handled
 * @return  void
 */
static void sig_handler( int signo )
{
   if( signo == SIGUSR1 )
   {
      LOG_INFO( "TEMP TASK: Received SIGUSR1! Exiting...\n");
      mq_close( temp_queue );
      timer_delete( timerid );
      i2c_stop( &i2c_tmp102 );
      thread_exit( signo );
   }
   else if( signo == SIGUSR2 )
   {
      LOG_INFO( "TEMP TASK: Received SIGUSR2! Exiting...\n");
      mq_close( temp_queue );
      timer_delete( timerid );
      i2c_stop( &i2c_tmp102 );
      thread_exit( signo );
   }
   return;
}

float get_temperature( void )
{
   return last_temp_value;
}

void tmp102_handler( union sigval sig )
{
   static int i = 0;

   float temperature;
   int retVal = tmp102_get_temp( &temperature );
   i++;
   if( retVal )
   {
      LOG_WARNING( "CYCLE [%d] --- Couldn't read temperature\n", i );
      led_toggle( LED0_BRIGHTNESS );
   }
   else
   {
      LOG_TASK_MSG( &temp_log, "TEMP: %0.5f Celsius\n", temperature );
   }

   return;
   led_toggle( LED2_BRIGHTNESS );
}


void tmp102_cycle( void )
{
   int retVal = 0;
   message_t request = {0};
   while( 1 )
   {
      memset( &request, 0, sizeof( request ) );
      retVal = mq_receive( temp_queue, (char*)&request, sizeof( request ), NULL );
      if( 0 > retVal )
      {
         int errnum = errno;
         LOG_ERROR( "TEMP TASK: QUEUE RECEIVE: (%s)\n",
                     strerror( errnum ) );
         continue;
      }
      switch( request.id )
      {
         case MSG_STATUS:
            pthread_mutex_lock( &alive_mutex );
            threads_status[TASK_TEMP]--;
            pthread_mutex_unlock( &alive_mutex );

            LOG_INFO( "TEMP TASK: I am alive!\n" );
            break;
         default:
            break;
      }
   }
   return;
}

mqd_t get_temperature_queue( void )
{
   return temp_queue;
}

int temp_queue_init( void )
{
   /* unlink first in case we hadn't shut down cleanly last time */
   mq_unlink( TEMP_QUEUE_NAME );

   struct mq_attr attr;
   attr.mq_flags = 0;
   attr.mq_maxmsg = MAX_MESSAGES;
   attr.mq_msgsize = sizeof( message_t );
   attr.mq_curmsgs = 0;

   int msg_q = mq_open( TEMP_QUEUE_NAME, O_CREAT | O_RDWR, 0666, &attr );
   if( 0 > msg_q )
   {
      int errnum = errno;
      LOG_ERROR( "TEMP TASK: QUEUE CREATE: (%s)\n",
                  strerror( errnum ) );
   }
   return msg_q;
}

void *temperature_fn( void *thread_args )
{
   signal(SIGUSR1, sig_handler);
   signal(SIGUSR2, sig_handler);

   temp_queue = temp_queue_init();
   if( 0 > temp_queue )
   {
      thread_exit( EXIT_INIT );
   }

   int retVal = i2c_init( &i2c_tmp102 );
   if( EXIT_INIT == retVal )
   {
      LOG_ERROR( "TEMP TASK: I2C INIT\n" );
      thread_exit( EXIT_INIT );
   }

   timer_setup( &timerid, &tmp102_handler );

   timer_start( &timerid, FREQ_1HZ );

   LOG_INFO( "TEMP TASK INITIALIZED\n" );
   tmp102_cycle();

   thread_exit( 0 );
   return NULL;
}
