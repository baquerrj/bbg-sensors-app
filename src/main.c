/**
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

#include "tmp102_task.h"
#include "apds9301_task.h"
#include "logger.h"
#include "common.h"
#include "watchdog.h"
#include "socket.h"
#include "led.h"
//#include "apds9960_sensor.h"

#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
/** /sys includes */
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>


void* (*thread_entry_fn[ NUM_THREADS ])(void *) = {
   logger_fn,
   temperature_fn,
   light_fn,
   socket_fn,
   watchdog_fn,
};

/**
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
         LOG_ERROR( "Master caught SIGINT!\n" );
         pthread_kill( task_id[TASK_WATCHDOG], SIGUSR2 );
   }
}

/**
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

/**
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
      log->fid = fopen( argv[1], "w" );
      log->name = argv[1];
      LOG_INFO( "Opened file %s\n", argv[1] );
   }
   else
   {
      LOG_ERROR( "Name of log file required!\n" );
      return 1;
   }

   LOG_INFO( "Starting Threads!\n" );


   led_on( LED2_BRIGHTNESS );

   set_trigger( LED2_TRIGGER, "timer" );
   set_delay( LED2_DELAYON, 50 );

   /* Attempting to spawn child threads */
   for( uint8_t i = 0; i < TASK_MAX; ++i )
   {
      if( 0 != pthread_create( &task_id[i], NULL, thread_entry_fn[i], (void*)log->fid ) )
      {
         int errnum = errno;
         LOG_ERROR( "Could not create thread %d (%s)\n", i, strerror( errnum ) ); 
         return 1;
      }
   }
   LOG_INFO( "MAIN: CHILD THREADS INIT\n" );

   //pthread_create( &watchdog_thread, NULL, watchdog_fn, (void*)threads );

   pthread_join( task_id[TASK_WATCHDOG], NULL );

//   pthread_join( apds9960_thread, NULL );

   LOG_INFO( "All threads exited! Main thread exiting... " );

   free( log );
   turn_off_leds();
   return 0;
}
