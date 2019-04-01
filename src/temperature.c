/*
 * =================================================================================
 *    @file     temperature.c
 *    @brief
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
#include "temperature.h"
#include "led.h"

#include <errno.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>

static timer_t    timerid;
struct itimerspec trigger;

static mqd_t temp_queue;
static shared_data_t *shm;

/*
 * =================================================================================
 * Function:       get_temperature_queue
 * @brief
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
mqd_t get_temperature_queue( void )
{
   return temp_queue;
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

/*
 * =================================================================================
 * Function:       timer_handler
 * @brief
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
static void timer_handler( union sigval sig )
{
   static int i = 0;
   static char buffer[SHM_BUFFER_SIZE];
   sprintf( buffer, "temp thread cycle[%d]\n", ++i );
   led_toggle( LED0_BRIGHTNESS );
   sem_wait(&shm->w_sem);
   print_header(shm->header);
   memcpy( shm->buffer, buffer, sizeof(shm->buffer) );
   sem_post(&shm->r_sem);

   return;
}


/*
 * =================================================================================
 * Function:       cycle
 * @brief
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
static void cycle( void )
{
   int retVal = 0;
   msg_t request = {0};
   msg_t response = {0};
   while( 1 )
   {
      memset( &request, 0, sizeof( request ) );
      retVal = mq_receive( temp_queue, (char*)&request, sizeof( request ), NULL );
      if( 0 > retVal )
      {
         int errnum = errno;
         fprintf( stderr, "Encountered error receiving from message queue %s: (%s)\n",
                  TEMP_QUEUE_NAME, strerror( errnum ) );
         continue;
      }
      switch( request.id )
      {
         case REQUEST_STATUS:
            sem_wait(&shm->w_sem);
            print_header(shm->header);
            sprintf( shm->buffer, "(Temperature) I am alive!\n" );
            sem_post(&shm->r_sem);
            fprintf( stdout, "(Temperature) I am alive!\n" );
            response.id = request.id;
            sprintf( response.info, "(Temperature) I am alive!\n" );
            retVal = mq_send( request.src, (const char*)&response, sizeof( response ), 0 );

            pthread_mutex_lock( &alive_mutex );
            threads_status[THREAD_TEMP]--;
            pthread_mutex_unlock( &alive_mutex );
            break;
         default:
            break;
      }
   }
   return;
}


/*
 * =================================================================================
 * Function:       temp_queue_init
 * @brief
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
int temp_queue_init( void )
{
   /* unlink first in case we hadn't shut down cleanly last time */
   mq_unlink( TEMP_QUEUE_NAME );

   struct mq_attr attr;
   attr.mq_flags = 0;
   attr.mq_maxmsg = MAX_MESSAGES;
   attr.mq_msgsize = sizeof( msg_t );
   attr.mq_curmsgs = 0;

   int msg_q = mq_open( TEMP_QUEUE_NAME, O_CREAT | O_RDWR, 0666, &attr );
   if( 0 > msg_q )
   {
      int errnum = errno;
      fprintf( stderr, "Encountered error creating message queue %s: (%s)\n",
               TEMP_QUEUE_NAME, strerror( errnum ) );
   }
   return msg_q;
}

/*
 * =================================================================================
 * Function:       temperature_fn
 * @brief
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
void *temperature_fn( void *arg )
{
   /* Get time that thread was spawned */
   struct timespec time;
   clock_gettime(CLOCK_REALTIME, &time);
   shm = get_shared_memory();

   /* Write initial state to shared memory */
   sem_wait(&shm->w_sem);
   print_header(shm->buffer);
   fprintf( stdout, "Hello World! Start Time: %ld.%ld secs\n",
            time.tv_sec, time.tv_nsec );
   sprintf( shm->buffer, "Hello World! Start Time: %ld.%ld secs\n",
            time.tv_sec, time.tv_nsec );
   /* Signal to logger that shared memory has been updated */
   sem_post(&shm->r_sem);

   signal(SIGUSR1, sig_handler);
   signal(SIGUSR2, sig_handler);

   temp_queue = temp_queue_init();
   if( 0 > temp_queue )
   {
      thread_exit( EXIT_INIT );
   }

   fprintf( stderr, "Temp Queue FD: %d\n", temp_queue );

   setup_timer( &timerid, &timer_handler );

   start_timer( &timerid, 1000000 );
   cycle();

   thread_exit( 0 );
   return NULL;
}
