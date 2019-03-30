/*
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
static shared_data_t *shm;

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
 * Function:       logger_fn
 * @brief  Entry point for logger thread
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
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
      perror( "Encountered error opening log file" );
      pthread_exit(&failure);
   }

   shm = get_shared_memory();
   if( NULL == shm )
   {
      int errnum = errno;
      fprintf( stderr, "Encountered error memory mapping shared memory: %s\n",
               strerror( errnum ) );
   }


   shared_data_t *buf = malloc( sizeof( shared_data_t ) );
   if( NULL == buf )
   {
      int errnum = errno;
      fprintf( stderr, "Encountered error allocating memory for local buffer %s\n",
               strerror( errnum ) );
   }

   while( 1 )
   {
      sem_wait(&shm->r_sem);

      memcpy( buf, shm, sizeof(*shm) );
      
      fprintf( log, "%s\n%s", buf->header, buf->buffer );
      fflush( log );

      sem_post(&shm->w_sem);
   }

   return NULL;
}


