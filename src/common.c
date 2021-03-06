/**
 * =================================================================================
 *    @file     common.c
 *    @brief   Defines types and functions common between the threads of the application
 *
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


#include "common.h"
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



void print_header( char *buffer )
{

   struct timespec time;
   clock_gettime(CLOCK_REALTIME, &time);

   if( NULL == buffer )
   {
      fprintf( stderr, "\n=====================================================\n" );
      fprintf( stderr, "Thread [%d]: %ld.%ld secs\n",
               (pid_t)syscall(SYS_gettid), time.tv_sec, time.tv_nsec );
      fflush( stderr );
   }
   else if( NULL != buffer )
   {
      char tmp[100]  = "\n=====================================================\n";
      char tmp2[100];
      sprintf( tmp2, "Thread [%d]: %ld.%ld secs\n",
               (pid_t)syscall(SYS_gettid), time.tv_sec, time.tv_nsec );
      strcat( tmp, tmp2 );
      strcpy( buffer, tmp );
   }
   return;
}

void thread_exit( int exit_status )
{
   struct timespec time;
   clock_gettime(CLOCK_REALTIME, &time);

   switch( exit_status )
   {
      case SIGUSR1:
         fprintf( stdout, "Caught SIGUSR1 Signal! Exiting...\n");
         break;
      case SIGUSR2:
         fprintf( stdout, "Caught SIGUSR2 Signal! Exiting...\n");
         break;
      default:
         break;
   }
   fprintf( stdout, "Goodbye World! End Time: %ld.%ld secs\n",
            time.tv_sec, time.tv_nsec );

   pthread_exit(EXIT_SUCCESS);
}

void *get_shared_memory( void )
{
   struct shared_data *shm_p;

   int shm_fd = shm_open( SHM_SEGMENT_NAME, O_CREAT | O_EXCL | O_RDWR, 0666 );
   if( 0 > shm_fd )
   {
      int errnum = errno;
      if( EEXIST == errnum )
      {
		   /* Already exists: open again without O_CREAT */
		   shm_fd = shm_open(SHM_SEGMENT_NAME, O_RDWR, 0);
	   }
      else
      {
         fprintf( stderr, "Encountered error opening shared memory: %s\n",
                  strerror( errnum ) );
         exit(EXIT_FAILURE);
      }
   }
   else
   {
      fprintf( stdout, "Creating shared memory and setting size to %u bytes\n",
               sizeof( shared_data_t ) );

      if( 0 > ftruncate( shm_fd, sizeof( shared_data_t )) )
      {
         int errnum = errno;
         fprintf( stderr, "Encountered error setting size of shared memroy: %s\n",
                  strerror( errnum ) );
         exit(EXIT_FAILURE);
      }
   }

	/* Map the shared memory */
	shm_p = mmap(NULL, sizeof( shared_data_t ), PROT_READ | PROT_WRITE,
		     MAP_SHARED, shm_fd, 0);

   if( NULL == shm_p )
   {
      int errnum = errno;
      fprintf( stderr, "Encountered error memory mapping shared memory: %s\n",
               strerror( errnum ) );
		exit(EXIT_FAILURE);
	}
	return shm_p;
}


int sems_init( shared_data_t *shm )
{
   int retVal = 0;
   retVal = sem_init( &shm->w_sem, 1, 1 );
   if( 0 > retVal )
   {
      int errnum = errno;
      fprintf( stderr, "Encountered error initializing write semaphore: %s\n",
               strerror( errnum ) );
      return EXIT_INIT;
   }
   retVal = sem_init( &shm->r_sem, 1, 0 );
   if( 0 > retVal )
   {
      int errnum = errno;
      fprintf( stderr, "Encountered error initializing read semaphore: %s\n",
               strerror( errnum ) );
      return EXIT_INIT;
   }
   return EXIT_CLEAN;
}



int timer_setup( timer_t *id, void (*handler)(union sigval) )
{
   int retVal = 0;
   /* Set up timer */
   struct sigevent sev;

   memset(&sev, 0, sizeof(struct sigevent));

   sev.sigev_notify = SIGEV_THREAD;
   sev.sigev_notify_function = handler;
   sev.sigev_value.sival_ptr = NULL;
   sev.sigev_notify_attributes = NULL;

   retVal = timer_create( CLOCK_REALTIME, &sev, id );
   if( 0 > retVal )
   {
      int errnum = errno;
      fprintf( stderr, "Encountered error creating new timer: (%s)\n",
               strerror( errnum ) );
      return EXIT_INIT;
   }
   return EXIT_CLEAN;
}


int timer_start( timer_t *id, unsigned long usecs )
{
   int retVal = 0;
   struct itimerspec trigger;

   trigger.it_value.tv_sec = usecs / MICROS_PER_SEC;
   trigger.it_value.tv_nsec = ( usecs % MICROS_PER_SEC ) * 1000;
   
   trigger.it_interval.tv_sec = trigger.it_value.tv_sec;
   trigger.it_interval.tv_nsec = trigger.it_value.tv_nsec;

   retVal = timer_settime( *id, 0, &trigger, NULL );
   if( 0 > retVal )
   {
      int errnum = errno;
      fprintf( stderr, "Encountered error starting new timer: (%s)\n",
               strerror( errnum ) );
      return EXIT_INIT;
   }
   return EXIT_CLEAN;
}
