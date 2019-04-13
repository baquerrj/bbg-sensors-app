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
//static shared_data_t *shm;

//static mqd_t logger_queue
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
      printf("Received SIGUSR1! Exiting...\n");
      thread_exit( signo );
   }
   else if( signo == SIGUSR2 )
   {
      printf("Received SIGUSR2! Exiting...\n");
      thread_exit( signo );
   }
   return;
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
   attr.mq_msgsize = sizeof( msg_t );
   attr.mq_curmsgs = 0;

   int msg_q = mq_open( LOGGER_QUEUE_NAME, O_CREAT | O_RDWR, 0666, &attr );
   if( 0 > msg_q )
   {
      int errnum = errno;
//      sem_wait(&shm->w_sem);
//      print_header(shm->header);
//      sprintf( shm->buffer, "ERROR: Encountered error creating message queue %s: (%s)\n",
//               LOGGER_QUEUE_NAME, strerror( errnum ) );
//      sem_post(&shm->r_sem);
      LOG_ERROR( "Could not create message queue for logger task: %s\n",
                  strerror( errnum ) );
   }
   return msg_q;   
}

void *logger_fn( void *arg )
{
   struct timespec time;
   clock_gettime(CLOCK_REALTIME, &time);
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
      //perror( "Encountered error opening log file" );
      LOG_ERROR( "Encountered error opening log file (%s)\n",
                  strerror( errnum ) );
      pthread_exit(&failure);
   }

//   shm = get_shared_memory();
//   if( NULL == shm )
//   {
//      int errnum = errno;
//      fprintf( stderr, "Encountered error memory mapping shared memory: %s\n",
//               strerror( errnum ) );
//   }


//  shared_data_t *buf = malloc( sizeof( shared_data_t ) );
//   if( NULL == buf )
//   {
//      int errnum = errno;
//      fprintf( stderr, "Encountered error allocating memory for local buffer %s\n",
//               strerror( errnum ) );
//   }

   uint8_t retVal = logger_queue_init();
   if( 0 > retVal )
   {
      LOG_ERROR( "LOGGER TASK INIT\n" );
      pthread_exit( &failure );
   }

   LOG_INFO( "LOGGER TASK INITIALIZED\n" );
   while( 1 )
   {
//      sem_wait(&shm->r_sem);
//      memcpy( buf, shm, sizeof(*shm) );
         
//      fprintf( log, "%s\n%s", buf->header, buf->buffer );
//      fflush( log );

      led_toggle( LED3_BRIGHTNESS ); 
//      sem_post(&shm->w_sem);
   }

   return NULL;
}


