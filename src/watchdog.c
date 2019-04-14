/*!
 *    @file     watchdog.c
 *    @brief   Watchdog source file:
 *                the watchdog is responsible for checking that the temperature and
 *                light sensor threads are alive
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
 */


#include "watchdog.h"
#include "logger.h"
#include "tmp102_task.h"
#include "apds9301_task.h"

#include <errno.h>
#include <string.h>
#include <time.h>

static timer_t    timerid;
struct itimerspec trigger;

//static struct thread_id_s* threads;
static mqd_t thread_msg_q[NUM_THREADS];
static mqd_t watchdog_queue;

pthread_mutex_t alive_mutex;


/*!
 * Function:       sig_handler
 * @brief   Signal handler for watchdog. On normal operation, we should be receiving
 *          a SIGUSR2 signal from the main thread prompting us to call pthread_kill
 *          for the other child threads
 *
 * @param   signo - enum with signal number of signal being handled
 * @return  void
 */
static void sig_handler( int signo )
{
   if( SIGUSR2 == signo )
   {
      kill_threads();
      mq_close( watchdog_queue );
      timer_delete( timerid );
      thread_exit( 0 );
   }
}

/*!
 * Function:       kill_threads
 * @brief   Function to kill children threads 
 *
 * @param   void
 * @return  void
 */
void kill_threads( void )
{
   fprintf( stdout, "watchdog caught signals - killing thread [%ld]\n",
            task_id[TASK_TMP102] );
   fflush( stdout );
   pthread_kill( task_id[TASK_TMP102], SIGUSR1 );

   fprintf( stdout, "watchdog caught signals - killing thread [%ld]\n",
            task_id[TASK_APDS9301] );
   fflush( stdout );
   pthread_kill( task_id[TASK_APDS9301], SIGUSR1 );

   fprintf( stdout, "watchdog caught signals - killing thread [%ld]\n",
            task_id[TASK_SOCKET] );
   fflush( stdout );
   pthread_kill( task_id[TASK_SOCKET], SIGUSR1 );

   fprintf( stdout, "watchdog caught signals - killing thread [%ld]\n",
            task_id[TASK_LOGGER] );
   fflush( stdout );
   pthread_kill( task_id[TASK_LOGGER], SIGUSR1 );

   return;
}

/*!
 * Function:       check_threads
 * @brief   Periodically send message via message queue for temperature and sensor threads
 *          to check for health. This function is registered as the timer hanlder for the
 *          timer owned by the watchdog
 *
 * @param   sig
 * @return  void
 */
void check_threads( union sigval sig )
{
   int retVal = 0;
   message_t msg_out = {0};
   msg_out.id  = MSG_ALIVE;
   msg_out.src = TASK_WATCHDOG;

   if( (0 == threads_status[TASK_TMP102]) && (0 == threads_status[TASK_APDS9301] ) && (0 == threads_status[TASK_LOGGER]) )
   {
      pthread_mutex_lock( &alive_mutex );
      threads_status[TASK_LOGGER]++;
      threads_status[TASK_TMP102]++;
      threads_status[TASK_APDS9301]++;
      pthread_mutex_unlock( &alive_mutex );
      retVal = mq_send( thread_msg_q[TASK_TMP102], (const char*)&msg_out, sizeof( msg_out ), 0 );
      if( 0 > retVal )
      {
         int errnum = errno;
         LOG_ERROR( "CHECK THREAD TMP102: (%s)\n", strerror( errnum ) );
      }
      retVal = mq_send( thread_msg_q[TASK_APDS9301], (const char*)&msg_out, sizeof( msg_out ), 0 );
      if( 0 > retVal )
      {
         int errnum = errno;
         LOG_ERROR( "CHECK THREAD APDS9301: (%s)\n", strerror( errnum ) );
      }
      retVal = mq_send( thread_msg_q[TASK_LOGGER], (const char*)&msg_out, sizeof( msg_out ), 0 );
      if( 0 > retVal )
      {
         int errnum = errno;
         LOG_ERROR( "CHECK THREAD LOGGER: (%s)\n", strerror( errnum ) );
      }
   }
   else
   {
      LOG_ERROR( "One of the threads did not return!\n" );
      LOG_ERROR( "thread_status[TASK_TMP102] = %d\nthread_status[TASK_APDS9301] = %d\nthread_status[TASK_LOGGER] = %d\n",
               threads_status[TASK_TMP102], threads_status[TASK_APDS9301], threads_status[TASK_LOGGER] );
      kill_threads();
      thread_exit( EXIT_ERROR );
   }

   return;
}

/*!
 * Function:       watchdog_queue_init
 * @brief   Initalize message queue for watchdog
 *
 * @param   void
 * @return  msg_q - file descriptor for initialized message queue
 */
mqd_t watchdog_queue_init( void )
{
   /* unlink first in case we hadn't shut down cleanly last time */
   mq_unlink( WATCHDOG_QUEUE_NAME );

   struct mq_attr attr;
   attr.mq_flags = 0;
   attr.mq_maxmsg = MAX_MESSAGES;
   attr.mq_msgsize = sizeof( message_t );
   attr.mq_curmsgs = 0;

   watchdog_queue = mq_open( WATCHDOG_QUEUE_NAME, O_CREAT | O_RDWR, 0666, &attr );
   if( 0 > watchdog_queue )
   {
      int errnum = errno;
      LOG_ERROR( "Encountered error creating message queue %s: (%s)\n",
               WATCHDOG_QUEUE_NAME, strerror( errnum ) );
   }
   return watchdog_queue;
}

/*!
 * Function:       watchdog_init
 * @brief   Initialize watchdog, calling appropriate functions to do so.
 *          E.g. calling timer_setup and timer_start to set up timer
 *
 * @param   void
 * @return  EXIT_CLEAN, otherwise EXIT_INIT
 */
int watchdog_init( void )
{
   watchdog_queue = watchdog_queue_init();
   if( 0 > watchdog_queue )
   {
      thread_exit( EXIT_INIT );
   }

   while( 0 == (thread_msg_q[TASK_LOGGER] = get_logger_queue()) );
   while( 0 == (thread_msg_q[TASK_TMP102] = get_tmp102_queue()) );
   while( 0 == (thread_msg_q[TASK_APDS9301] = get_apds9301_queue()) );

   LOG_INFO( "Watchdog says: Temp Queue FD: %d\n", thread_msg_q[TASK_TMP102] );
   LOG_INFO( "Watchdog says: Light Queue FD: %d\n", thread_msg_q[TASK_APDS9301] );

   pthread_mutex_init( &alive_mutex, NULL );
   timer_setup( &timerid, &check_threads );

   timer_start( &timerid, FREQ_QURT_HZ );

   return EXIT_CLEAN;
}

/*!
 * Function:       watchdog_fn
 * @brief   Entry point for wachtdog
 *
 * @param   thread_args - void ptr used to pass thread identifiers (pthread_t) for
 *                      child threads we have to check for health
 * @return  NULL  - We don't really exit from this function,
 *                   since the exit point for threads is thread_exit()
 */
void *watchdog_fn( void *thread_args )
{
   signal( SIGUSR2, sig_handler );
//   exit_e retVal = EXIT_ERROR;
//   if( NULL == thread_args )
//   {
//      print_header( NULL );
//      LOG_ERROR( "Encountered null pointer!\n" );
//      pthread_exit(&retVal);
//   }
//   else
//   {
//      threads = malloc( sizeof( struct thread_id_s ) );
//      threads = (struct thread_id_s*)thread_args;
//   }

   watchdog_init();

   while(1);
   return NULL;
}

