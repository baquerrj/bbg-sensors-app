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
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <mqueue.h>
#include <sys/types.h>
#include <sys/stat.h>

#define NUM_THREADS  2

static timer_t    timerid;
struct itimerspec trigger;

static struct thread_id_s* threads;
static mqd_t thread_msg_q[NUM_THREADS];

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
      fprintf( stdout, "watchdog caught signals - killing thread [%ld]\n", threads->temp_thread );
      fflush( stdout );
      pthread_kill( threads->temp_thread, SIGUSR1 );
      fprintf( stdout, "watchdog caught signals - killing thread [%ld]\n", threads->logger_thread );
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
   request_t request = {0};
   request.id = REQUEST_STATUS;
   retVal = mq_send( thread_msg_q[0], (const char*)&request, sizeof( request ), 0 );

   if( 0 > retVal )
   {
      int errnum = errno;
      fprintf( stderr, "Encountered error sending status request from watchdog: (%s)\n",
               strerror( errnum ) );
   }
   return;
}

/*
 * =================================================================================
 * Function:       setup_timer
 * @brief
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
int setup_timer( void )
{
   int retVal = 0;
   /* Set up timer */
   char info[] = "timer woken up!";
   struct sigevent sev;

   memset(&sev, 0, sizeof(struct sigevent));
   memset(&trigger, 0, sizeof(struct itimerspec));

   sev.sigev_notify = SIGEV_THREAD;
   sev.sigev_notify_function = &check_threads;
   sev.sigev_value.sival_ptr = &info;
   sev.sigev_notify_attributes = NULL;

   retVal = timer_create(CLOCK_REALTIME, &sev, &timerid);
   if( 0 > retVal )
   {
      int errnum = errno;
      fprintf( stderr, "Encountered error creating new timer for watchdog thread: (%s)\n",
               strerror( errnum ) );
      return retVal;
   }

   trigger.it_value.tv_sec = 1;
   trigger.it_interval.tv_nsec = 100 * 1000000;
//   trigger.it_interval.tv_sec = 1;

   retVal = timer_settime( timerid, 0, &trigger, NULL );
   if( 0 > retVal )
   {
      int errnum = errno;
      fprintf( stderr, "Encountered error creating new timer for watchdog thread: (%s)\n",
               strerror( errnum ) );
      return retVal;
   }
   return 0;
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
   while( 0 == (thread_msg_q[0] = get_temperature_queue()) );

   fprintf( stderr, "Watchdog says: Temp Queue FD: %d\n", thread_msg_q[0] );
   setup_timer();

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

