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
#include "watchdog.h"
#include "led.h"

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
static pthread_t watchdog_thread;

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
         pthread_kill( watchdog_thread, SIGUSR2 );
   }
}



/*
 * =================================================================================
 * Function:       turn_off_leds
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
void turn_off_leds( void )
{
   led_off( LED0_BRIGHTNESS );
   led_off( LED1_BRIGHTNESS );
   led_off( LED2_BRIGHTNESS );
   led_off( LED3_BRIGHTNESS );
   return;
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
   printf( "Number of arguments %d\n", argc );
   if( argc > 1 )
   {
      log = malloc( sizeof( file_t ) );
      log->fid = fopen( argv[1], "a" );
      log->name = argv[1];
      printf( "Opened file %s\n", argv[1] );
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
   
   struct thread_id_s* threads = malloc( sizeof( struct thread_id_s ) );

   led_on( LED2_BRIGHTNESS );

   set_trigger( LED2_TRIGGER, "timer" );
   set_delay( LED2_DELAYON, 50 );
   /* Attempting to spawn child threads */
   pthread_create( &temp_thread, NULL, temperature_fn, NULL);
   pthread_create( &logger_thread, NULL, logger_fn, (void*)log->fid);

   threads->temp_thread = temp_thread;
   threads->logger_thread = logger_thread;

   pthread_create( &watchdog_thread, NULL, watchdog_fn, (void*)threads );


   pthread_join( watchdog_thread, NULL );

   clock_gettime(CLOCK_REALTIME, &time);


   print_header( NULL );
   fprintf( stdout, "All threads exited! Main thread exiting... " );
   fprintf( stdout, "End Time: %ld.%ld secs\n",
            time.tv_sec, time.tv_nsec );

   free( log );
   free( threads );
   munmap( shm, sizeof( shared_data_t ) );
   shm_unlink( SHM_SEGMENT_NAME );
   turn_off_leds();
   return 0;
}
