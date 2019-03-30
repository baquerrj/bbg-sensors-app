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


#include "temperature.h"
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <semaphore.h>

static timer_t    timerid;
struct itimerspec trigger;

static shared_data_t *shm;

static pthread_mutex_t  tmutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t   tcond = PTHREAD_COND_INITIALIZER;


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
   else if( signo == SIGCONT )
   {
      pthread_cond_broadcast(&tcond);
   }
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
void cycle( void )
{
   static int i = 0;
   while( 1 )
   {
      static char buffer[BUFFER_SIZE];
      sprintf( buffer, "temp thread cycle[%d]\n", ++i );


      pthread_mutex_lock(&tmutex);
      pthread_cond_wait(&tcond, &tmutex);
      pthread_mutex_unlock(&tmutex);

      sem_wait(&shm->w_sem);
      print_header(shm->header);
      memcpy( shm->buffer, buffer, sizeof(shm->buffer) );
      sem_post(&shm->r_sem);
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
   /* Set up timer */
   char info[] = "timer woken up!";
   struct sigevent sev;

   memset(&sev, 0, sizeof(struct sigevent));
   memset(&trigger, 0, sizeof(struct itimerspec));

   sev.sigev_notify = SIGEV_SIGNAL;
   sev.sigev_signo = SIGCONT;
   signal(SIGCONT, sig_handler);
   sev.sigev_value.sival_ptr = &info;

   timer_create(CLOCK_REALTIME, &sev, &timerid);

   trigger.it_value.tv_sec = 1;
//   trigger.it_interval.tv_nsec = 100 * 1000000;
   trigger.it_interval.tv_sec = 1;

   pthread_mutex_init( &tmutex, NULL );
   pthread_cond_init( &tcond, NULL );

   timer_settime( timerid, 0, &trigger, NULL );
   return 0;
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

   setup_timer();
   cycle();

   thread_exit( 0 );
   return NULL;
}
