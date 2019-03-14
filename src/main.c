/*
 * =================================================================================
 *    @file     main.c
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
#include "logger.h"
#include "common.h"

#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
/* /sys includes */
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

static pthread_t temp_thread;
//static pthread_t light_thread;
static pthread_t logger_thread;

static shared_data_t *shm;

/*
 * =================================================================================
 * Function:       signal_handler
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
static void signal_handler( int signo )
{
   switch( signo )
   {
      case SIGINT:
         fprintf( stderr, "Master caught SIGINT!\n" );
         /* Raise SIGUSR1 signal to kill child thread */
         pthread_kill( temp_thread, SIGUSR1 );
         pthread_kill( logger_thread, SIGUSR1 );
   }
}



/*
 * =================================================================================
 * Function:       main
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */

int main( int argc, char *argv[] )
{
   signal( SIGINT, signal_handler );
   static file_t *log;
   args_t *args;
   printf( "Number of arguments %d\n", argc );
   if( argc > 1 )
   {
      log = malloc( sizeof( file_t ) );
      log->fid = fopen( argv[1], "a" );
      log->name = argv[1];
      printf( "Opened file %s\n", argv[1] );
      args = malloc(sizeof(args_t));
      args->arg1 = log->name;
      if( argv[2] )
      {
         args->arg2 = argv[2];
      }
   }
   else
   {
      fprintf( stderr, "Name of log file required!\n" );
      return 1;
   }

   /* Initialize Shared Memory */
   shm = get_shared_memory();
   if( 0  > sems_init( shm ) )
   {
      fprintf( stderr, "Encountered error initializing semaphores!\n" );
      return 1;
   }

   struct timespec time;
   clock_gettime(CLOCK_REALTIME, &time);

   print_header( NULL );
   fprintf( stdout, "Starting Threads! Start Time: %ld.%ld secs\n",
            time.tv_sec, time.tv_nsec );

   /* Attempting to spawn child threads */
   pthread_create( &temp_thread, NULL, temperature_fn, NULL);
   pthread_create( &logger_thread, NULL, logger_fn, (void*)log->fid);

   pthread_join( temp_thread, NULL );
   pthread_join( logger_thread, NULL );

   clock_gettime(CLOCK_REALTIME, &time);


   print_header( NULL );
   fprintf( stdout, "All threads exited! Main thread exiting... " );
   fprintf( stdout, "End Time: %ld.%ld secs\n",
            time.tv_sec, time.tv_nsec );
   
   free( log );
   free( args );
   munmap( shm, sizeof( shared_data_t ) );
   shm_unlink( SHM_SEGMENT_NAME );
   return 0;
}
