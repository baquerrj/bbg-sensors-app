/*!
 * @file  apds9960_task.c
 * @brief 
 *
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

#include "common.h"
#include "apds9960_task.h"
#include "watchdog.h"
#include "led.h"
#include "logger.h"

#include <errno.h>
#include <time.h>
#include <string.h>
#include <math.h>

static timer_t    timerid;
struct itimerspec trigger;

static i2c_handle_t i2c_apds9960;

static mqd_t apds9960_queue;

static message_t apds9960_log = {
   .level      = LOG_INFO,
   .timestamp  = {0},
   .id         = MSG_STATUS,
   .src        = TASK_APDS9960,
   .msg        = {0}
};

/*!
 * Function:       sig_handler
 * @brief   Signal handler for light sensor thread.
 *          On normal operation, we should be receving SIGUSR1/2 signals from watchdog
 *          when prompted to exit. So, we close the message queue and timer this thread owns
 *
 * @param   signo - enum with signal number of signal being handled
 * @return  void
 */
static void sig_handler( int signo )
{
   if( signo == SIGUSR1 || signo == SIGUSR2 )
   {
      printf("Received signal! Exiting...\n");
      mq_close( apds9960_queue );
      timer_delete( timerid );
#ifdef _PROXIMITY_
      i2c_stop( &i2c_apds9960 );
#endif
      thread_exit( signo );
   }
   return;
}

static void cycle( void )
{
   int retVal = 0;
   message_t request = {0};
   while( 1 )
   {
      memset( &request, 0, sizeof( request ) );
      retVal = mq_receive( apds9960_queue, (char*)&request, sizeof( request ), NULL );
      if( 0 > retVal )
      {
         int errnum = errno;
         LOG_ERROR( "APDS9960 TASK: QUEUE RECEIVE: (%s)\n",
                     strerror( errnum ) );
         continue;
      }
      switch( request.id )
      {
         case MSG_ALIVE:
            pthread_mutex_lock( &alive_mutex );
            threads_status[TASK_APDS9960]--;
            pthread_mutex_unlock( &alive_mutex );

            LOG_INFO( "APDS9960 TASK: I am alive!\n" );
            break;
         default:
            break;
      }
   }
   return;
}
   
static void apds9960_task( union sigval sig )
{
   uint8_t id = 0;
#ifdef _PROXIMITY_
   int retVal = apds9960_read_id( &id );
   if( EXIT_CLEAN != retVal )
   {
      LOG_ERROR( "APDS9960 TASK: READ ID REG\n" );
   }
   if( ( APDS9960_ID_1 != id ) && ( APDS9960_ID_2 != id ) )
   {
      LOG_ERROR( "Invalid ID: %u\n", id );
   }
   else
   {
      LOG_INFO( "APDS9960 ID: %u\n", id );
   }

   uint8_t proximity_level = 0;
   if( EXIT_CLEAN != readProximity( &proximity_level ) )
   {
      apds9960_log.level = LOG_ERROR;
      apds9960_log.id = MSG_STATUS;
      LOG_TASK_MSG( &apds9960_log, "READ PROXIMITY: %d\n", proximity_level );
   }
   else
   {
      apds9960_log.level = LOG_INFO;
      apds9960_log.id = MSG_STATUS;
      LOG_TASK_MSG( &apds9960_log, "READ PROXIMITY: %d\n", proximity_level );
   }
#endif
   LOG_TASK_MSG( &apds9960_log, "APDS9960 TASK\n" );
   return;
}


mqd_t get_apds9960_queue( void )
{
   return apds9960_queue;
}

uint8_t apds9960_queue_init( void )
{
   mq_unlink( APDS9960_QUEUE );
   struct mq_attr attr;
   attr.mq_flags = 0;
   attr.mq_maxmsg = MAX_MESSAGES;
   attr.mq_msgsize = sizeof( message_t );
   attr.mq_curmsgs = 0;

   int msg_q = mq_open( APDS9960_QUEUE, O_CREAT | O_RDWR, 0666, &attr );
   if( 0 > msg_q )
   {
      int errnum = errno;
      LOG_ERROR( "APDS9960 TASK: QUEUE CREATE: (%s)\n", strerror( errnum ) );
   }
   return msg_q;
}



void *apds9960_fn( void *thread_args )
{
   signal(SIGUSR1, sig_handler);
   signal(SIGUSR2, sig_handler);

   apds9960_queue = apds9960_queue_init();
   if( 0 > apds9960_queue )
   {
      thread_exit( EXIT_INIT );
   }

#ifdef _PROXIMITY_
   int retVal = apds9960_sensor_init( &i2c_apds9960 );
   if( EXIT_CLEAN != retVal )
   {
     LOG_ERROR( "APDS9960 TASK: I2C INIT\n" );
     thread_exit( EXIT_INIT );
   }
#endif
   timer_setup( &timerid, &apds9960_task );

   timer_start( &timerid, FREQ_1HZ );

   LOG_INFO( "APDS9960 TASK INTIALIZED\n" );

   cycle();

   thread_exit( 0 );
   return NULL;
}
