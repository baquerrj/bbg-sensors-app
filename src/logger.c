/**
 * =================================================================================
 *    @file     logger.c
 *    @brief    Takes care of logging for other threads
 *
 *  This logger works in background to log the state of other threads to a common file.
 *  It is responsible for reading the shared memory segment written to by the sensor
 *  threads. It sleeps waiting for a semaphore to be posted by another thread signaling
 *  that new data has been written to shared memory and that it should read it.
 *
 *    @author   Roberto Baquerizo (baquerrj), roba8460@colorado.edu
 *
 *    @internal
 *       Created:  03/13/2019
 *      Revision:  none
 *      Compiler:  gcc
 *  Organization:  University of Colorado: Boulder
 *
 *  This source code is released for free distribution under the terms of the
 *  GNU General Public License as published by the Free Software Foundation.
 * =================================================================================
 */

#include "watchdog.h"
#include "led.h"
#include "logger.h"
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

struct itimerspec trigger;
static FILE *log;

static mqd_t logger_queue;

uint8_t log_msg( mqd_t queue, const message_t *msg, size_t size, int priority )
{
   if( -1 == mq_send( queue, (const char*)msg, size, priority ) )
   {
      int errnum = errno;
      LOG_ERROR( "LOGGER - QUEUE SEND (%s)\n", strerror( errnum ) );
      return EXIT_ERROR;
   }
   return EXIT_CLEAN;
}

/*!
 * Function:       sig_handler
 * @brief   Signal handler for logger thread.
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
      LOG_INFO( "LOGGER TASK: Received SIGUSR1! Exiting...\n");
      thread_exit( signo );
   }
   else if( signo == SIGUSR2 )
   {
      LOG_INFO( "LOGGER TASK: Received SIGUSR2! Exiting...\n");
      thread_exit( signo );
   }
   return;
}


void logger_cycle( void )
{
   uint8_t retVal;
   unsigned int prio;
   message_t msg = {0};
   while( 1 )
   {
      memset( &msg, 0, sizeof( msg ) );

      retVal = mq_receive( logger_queue, (char*)&msg, sizeof(msg), &prio );

      if( 0 > retVal )
      {
         led_toggle( LED3_BRIGHTNESS ); 
         int errnum = errno;
         LOG_ERROR( "LOGGER TASK: QUEUE RECEIVE: (%s)\n",
                     strerror( errnum ) );
         continue;
      }
      switch( msg.id )
      {
         case MSG_ALIVE:
         {
            pthread_mutex_lock( &alive_mutex );
            threads_status[TASK_LOGGER]--;
            pthread_mutex_unlock( &alive_mutex );

            LOG_INFO( "LOGGER TASK: I am alive!\n" );
            break;
         }
         case MSG_STATUS:
         {
            LOG_MSG( log, INFO"FROM: %s ----- %s",
                     get_task_name(msg.src), msg.msg );
            //LOG_MSG( log, INFO "[%s] FROM: %s ----- %s",
            //         msg.timestamp, get_task_name(msg.src), msg.msg );
            break;
         }
         default:
            break;
      }
   }
}
                     

uint8_t convert_timestamp( char *timestamp, const int len )
{
   struct timespec time;
   uint8_t retVal = clock_gettime( CLOCK_REALTIME, &time );
   if( 0 != retVal )
   {
      memset( timestamp, 0, len );
      return EXIT_ERROR;
   }

   char str[25] = {0};
   snprintf( str, sizeof( str ), "%ld.%ld", time.tv_sec, time.tv_nsec );

   memcpy( timestamp, str, len );

   return EXIT_CLEAN;
}


mqd_t get_logger_queue( void )
{
   return logger_queue;
}


mqd_t logger_queue_init( void )
{
   mq_unlink( LOGGER_QUEUE_NAME );

   struct mq_attr attr;
   attr.mq_flags = 0;
   attr.mq_maxmsg = MAX_MESSAGES;
   attr.mq_msgsize = sizeof( message_t );
   attr.mq_curmsgs = 0;

   logger_queue = mq_open( LOGGER_QUEUE_NAME, O_CREAT | O_RDWR, 0666, &attr );
   if( 0 > logger_queue )
   {
      int errnum = errno;
      LOG_ERROR( "LOGGER TASK: QUEUE CREATE: (%s)\n",
                  strerror( errnum ) );
   }
   return logger_queue;   
}

void *logger_fn( void *arg )
{
   static int failure = 1;

   signal(SIGUSR1, sig_handler);
   signal(SIGUSR2, sig_handler);

   /* Initialize thread */
   if( NULL == arg )
   {
      fprintf( stderr, "Thread requires name of log file!\n" );
      pthread_exit(&failure);
   }

   log = (FILE *)arg;
   if( NULL == log )
   {
      int errnum = errno;
      LOG_ERROR( "Encountered error opening log file (%s)\n",
                  strerror( errnum ) );
      pthread_exit(&failure);
   }

   uint8_t retVal = logger_queue_init();
   if( 0 > retVal )
   {
      pthread_exit( &failure );
   }

   LOG_INFO( "LOGGER TASK INITIALIZED\n" );

   logger_cycle();
   while( 1 )
   {
   }

   return NULL;
}


