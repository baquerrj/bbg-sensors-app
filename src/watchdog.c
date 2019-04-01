/*
 * =================================================================================
 *    @file     watchdog.c
 *    @brief
 *
 *  <+DETAILED+>
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


#include "watchdog.h"
#include "common.h"
#include "temperature.h"
#include "light.h"


#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <mqueue.h>
#include <sys/types.h>
#include <sys/stat.h>

static timer_t    timerid;
struct itimerspec trigger;

static struct thread_id_s* threads;
static mqd_t thread_msg_q[NUM_THREADS];
static mqd_t watchdog_queue;

pthread_mutex_t alive_mutex;

static void kill_threads( void )
{
   fprintf( stdout, "watchdog caught signals - killing thread [%ld]\n",
            threads->temp_thread );
   fflush( stdout );
   pthread_kill( threads->temp_thread, SIGUSR1 );

   fprintf( stdout, "watchdog caught signals - killing thread [%ld]\n",
            threads->light_thread );
   fflush( stdout );
   pthread_kill( threads->light_thread, SIGUSR1 );

   fprintf( stdout, "watchdog caught signals - killing thread [%ld]\n",
            threads->logger_thread );
   fflush( stdout );
   pthread_kill( threads->logger_thread, SIGUSR1 );
   free( threads );
   return;
}


/*
 * =================================================================================
 * Function:       sig_handler
 * @brief
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
static void sig_handler( int signo )
{
   if( SIGUSR2 == signo )
   {
      fprintf( stdout, "watchdog caught signals - killing thread [%ld]\n",
               threads->temp_thread );
      fflush( stdout );
      pthread_kill( threads->temp_thread, SIGUSR1 );

      fprintf( stdout, "watchdog caught signals - killing thread [%ld]\n",
               threads->light_thread );
      fflush( stdout );
      pthread_kill( threads->light_thread, SIGUSR1 );

      fprintf( stdout, "watchdog caught signals - killing thread [%ld]\n",
               threads->logger_thread );
      fflush( stdout );
      pthread_kill( threads->logger_thread, SIGUSR1 );
      free( threads );
      thread_exit( 0 );
   }
}

/*
 * =================================================================================
 * Function:       check_threads
 * @brief
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
void check_threads( union sigval sig )
{
   int retVal = 0;
   msg_t request = {0};
   request.id = REQUEST_STATUS;
   request.src = watchdog_queue;

   if( (0 == threads_status[THREAD_TEMP]) && (0 == threads_status[THREAD_LIGHT] ) )
   {
      pthread_mutex_lock( &alive_mutex );
      threads_status[THREAD_TEMP]++;
      threads_status[THREAD_LIGHT]++;
      pthread_mutex_unlock( &alive_mutex );
      retVal = mq_send( thread_msg_q[THREAD_TEMP], (const char*)&request, sizeof( request ), 0 );
      if( 0 > retVal )
      {
         int errnum = errno;
         fprintf( stderr, "Encountered error sending status request from watchdog: (%s)\n",
                  strerror( errnum ) );
      }
      retVal = mq_send( thread_msg_q[THREAD_LIGHT], (const char*)&request, sizeof( request ), 0 );
      if( 0 > retVal )
      {
         int errnum = errno;
         fprintf( stderr, "Encountered error sending status request from watchdog: (%s)\n",
                  strerror( errnum ) );
      }
   }
   else
   {
      fprintf( stderr, "One of the threads did not return!\n" );
      fprintf( stderr, "thread_status[THREAD_TEMP] = %d\nthread_status[THREAD_LIGHT] = %d\n",
               threads_status[THREAD_TEMP], threads_status[THREAD_LIGHT] );
      kill_threads();
      thread_exit( EXIT_ERROR );
   }

   return;
}

/*
 * =================================================================================
 * Function:       watchdog+queue_init
 * @brief
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
int watchdog_queue_init( void )
{
   /* unlink first in case we hadn't shut down cleanly last time */
   mq_unlink( WATCHDOG_QUEUE_NAME );

   struct mq_attr attr;
   attr.mq_flags = 0;
   attr.mq_maxmsg = MAX_MESSAGES;
   attr.mq_msgsize = sizeof( msg_t );
   attr.mq_curmsgs = 0;

   int msg_q = mq_open( WATCHDOG_QUEUE_NAME, O_CREAT | O_RDWR, 0666, &attr );
   if( 0 > msg_q )
   {
      int errnum = errno;
      fprintf( stderr, "Encountered error creating message queue %s: (%s)\n",
               WATCHDOG_QUEUE_NAME, strerror( errnum ) );
   }
   return msg_q;
}

/*
 * =================================================================================
 * Function:       watchdog_init
 * @brief
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
int watchdog_init( void )
{
   watchdog_queue = watchdog_queue_init();
   if( 0 > watchdog_queue )
   {
      thread_exit( EXIT_INIT );
   }

   while( 0 == (thread_msg_q[THREAD_TEMP] = get_temperature_queue()) );
   while( 0 == (thread_msg_q[THREAD_LIGHT] = get_light_queue()) );

   fprintf( stderr, "Watchdog says: Temp Queue FD: %d\n", thread_msg_q[0] );
   fprintf( stderr, "Watchdog says: Light Queue FD: %d\n", thread_msg_q[1] );

   pthread_mutex_init( &alive_mutex, NULL );
   setup_timer( &timerid, &check_threads );

   start_timer( &timerid, 4000000 );

   return 0;
}

/*
 * =================================================================================
 * Function:       watchdog_fn
 * @brief
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
void *watchdog_fn( void *thread_args )
{
   signal( SIGUSR2, sig_handler );
   exit_e retVal = EXIT_ERROR;
   if( NULL == thread_args )
   {
      print_header( NULL );
      fprintf( stderr, "Encountered null pointer!\n" );
      pthread_exit(&retVal);
   }
   else
   {
      threads = malloc( sizeof( struct thread_id_s ) );
      threads = (struct thread_id_s*)thread_args;
   }

   watchdog_init();

   while(1);
   return NULL;
}

