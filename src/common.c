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

const char* const task_name[NUM_THREADS + 1] = {
   (const char*)"LOGGER TASK",
   (const char*)"TMP102 TASK",
   (const char*)"APDS9301 TASK",
   (const char*)"APDS9960 TASK",
   (const char*)"SOCKET TASK",
   (const char*)"WATCHDOG",
};

void thread_exit( int exit_status )
{
   switch( exit_status )
   {
      case SIGUSR1:
         LOG_INFO( "Caught SIGUSR1 Signal! Exiting...\n");
         break;
      case SIGUSR2:
         LOG_INFO( "Caught SIGUSR2 Signal! Exiting...\n");
         break;
      default:
         break;
   }

   pthread_exit(EXIT_SUCCESS);
}

char* get_timestamp( void )
{
   struct timespec time;
   static char timestamp[25] = {0};

   int retVal = clock_gettime( CLOCK_REALTIME, &time );
   if( 0 != retVal )
   {
      memset( timestamp, 0, sizeof( timestamp ) );
   }
   else
   {
      snprintf( timestamp, sizeof( timestamp ), "%ld.%ld", time.tv_sec, time.tv_nsec );
   }
   return timestamp;
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
