/**
 * =================================================================================
 *    @file     light.c
 *    @brief   Interface to APDS9301 Light Sensor
 *
 *  <+DETAILED+>
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
 * =================================================================================
 */


#include "watchdog.h"
#include "apds9301_task.h"
#include "led.h"
#include "logger.h"

#include <errno.h>
#include <time.h>
#include <string.h>
#include <math.h>

static timer_t    timerid;
struct itimerspec trigger;

static i2c_handle_t i2c_apds9301;
static float last_lux_value = -5;
static mqd_t apds9301_queue;

static message_t apds9301_log = {
   .level      = LOG_INFO,
   .timestamp  = {0},
   .id         = MSG_STATUS,
   .src        = TASK_APDS9301,
   .msg        = {0}
};

/**
 * =================================================================================
 * Function:       sig_handler
 * @brief   Signal handler for light sensor thread.
 *          On normal operation, we should be receving SIGUSR1/2 signals from watchdog
 *          when prompted to exit. So, we close the message queue and timer this thread owns
 *
 * @param   signo - enum with signal number of signal being handled
 * @return  void
 * =================================================================================
 */
static void sig_handler( int signo )
{
   if( signo == SIGUSR1 || signo == SIGUSR2 )
   {
      LOG_INFO( "APDS9301 TASK: Received signal! Exiting...\n");
      mq_close( apds9301_queue );
      timer_delete( timerid );
#ifndef _PROXIMITY_ 
      apds9301_power( POWER_OFF );
      i2c_stop( &i2c_apds9301 );
#endif
      thread_exit( signo );
   }
   return;
}

/**
 * =================================================================================
 * Function:       apds9301_task
 * @brief   Timer handler function for light sensor thread
 *          When woken up by the timer, get lux reading and write state to shared memory
 *
 * @param   sig
 * @return  void
 * =================================================================================
 */
static void apds9301_task( union sigval sig )
{
#if _PROXIMITY_ == 0
   static int i = 0;

   float lux = -5;
   int retVal = apds9301_get_lux( &lux );

   i++;
   if( EXIT_CLEAN != retVal )
   {
      LOG_WARNING( "CYCLE [%d] --- could not get light reading!\n", i );
      led_toggle( LED0_BRIGHTNESS );
   }
   else
   {
      apds9301_log.id = MSG_STATUS;
      /* save new lux value */
      last_lux_value = lux;
      if( DARK_THRESHOLD > lux )
      {
         LOG_TASK_MSG( &apds9301_log, "State: %s, Lux: %0.5f\n", "NIGHT", lux );
      }
      else
      {
         LOG_TASK_MSG( &apds9301_log, "State: %s, Lux: %0.5f\n", "DAY", lux );
      }
   }
#endif
   led_toggle( LED1_BRIGHTNESS );
   return;
}

float get_lux( void )
{
   return last_lux_value;
}


int is_dark( void )
{
   int dark = 0;
   if( DARK_THRESHOLD > last_lux_value )
   {
      dark = 1;
   }
   return dark;
}


/**
 * =================================================================================
 * Function:       cycle
 * @brief   Cycle function for light sensor thread
 *          We wait in this while loop checking for requests from watchdog for health status
 *
 * @param   void
 * @return  void
 * =================================================================================
 */
static void cycle( void )
{
   int retVal = 0;
   message_t request = {0};
   while( 1 )
   {
      memset( &request, 0, sizeof( request ) );
      retVal = mq_receive( apds9301_queue, (char*)&request, sizeof( request ), NULL );
      if( 0 > retVal )
      {
         int errnum = errno;
         LOG_ERROR( "APDS9301 TASK: QUEUE RECEIVE: (%s)\n",
                     strerror( errnum ) );
         continue;
      }
      switch( request.id )
      {
         case MSG_ALIVE:
            pthread_mutex_lock( &alive_mutex );
            threads_status[TASK_APDS9301]--;
            pthread_mutex_unlock( &alive_mutex );

            LOG_INFO( "APDS9301 TASK: I am alive!\n" );
            break;
         default:
            break;
      }
   }
   return;
}

mqd_t get_apds9301_queue( void )
{
   return apds9301_queue;
}

int apds9301_queue_init( void )
{
   /* unlink first in case we hadn't shut down cleanly last time */
   mq_unlink( APDS9301_QUEUE );

   struct mq_attr attr;
   attr.mq_flags = 0;
   attr.mq_maxmsg = MAX_MESSAGES;
   attr.mq_msgsize = sizeof( message_t );
   attr.mq_curmsgs = 0;

   int msg_q = mq_open( APDS9301_QUEUE, O_CREAT | O_RDWR, 0666, &attr );
   if( 0 > msg_q )
   {
      int errnum = errno;
      LOG_ERROR( "APDS9301 TASK: QUEUE CREATE: %s\n",
                  strerror( errnum ) );
   }
   return msg_q;
}

void *apds9301_fn( void *thread_args )
{
   signal(SIGUSR1, sig_handler);
   signal(SIGUSR2, sig_handler);

   apds9301_queue = apds9301_queue_init();
   if( 0 > apds9301_queue )
   {
      thread_exit( EXIT_INIT );
   }

#ifndef _PROXIMITY_
   
   int retVal = i2c_init( &i2c_apds9301 );
   if( EXIT_CLEAN != retVal )
   {
      LOG_ERROR( "APDS9301 TASK: I2C INIT\n" );
      thread_exit( EXIT_INIT );
   }
   retVal = apds9301_power( POWER_ON );
   if( EXIT_CLEAN != retVal )
   {
      LOG_ERROR( "APDS9301 TASK: POWER ON\n" );
      thread_exit( EXIT_INIT );
   }
#endif
   
   timer_setup( &timerid, &apds9301_task );

   timer_start( &timerid, FREQ_1HZ );

   LOG_INFO( "APDS9301 TASK INITIALIZED\n" );
   cycle();

   thread_exit( 0 );
   return NULL;
}
