#include "temperature.h"
#include "common.h"

#include <signal.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/syscall.h>


static pthread_t temp_thread;
//static pthread_t light_thread;
//static pthread_t logger_thread;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


int main( int argc, char *argv[] )
{
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

   struct timespec time;
   clock_gettime(CLOCK_REALTIME, &time);

   print_header( log->fid );
   fprintf( log->fid, "Starting Threads! Start Time: %ld.%ld secs\n",
            time.tv_sec, time.tv_nsec );
   fflush( log->fid );
   print_header( stdout );
   fprintf( log->fid, "Starting Threads! Start Time: %ld.%ld secs\n",
            time.tv_sec, time.tv_nsec );

   /* Attempting to spawn child threads */
//   pthread_create( &logger_thread, NULL, logger_fn, (void *)args );
//   pthread_create( &light_thread, NULL, light_fn, (void *)args );
   pthread_create( &temp_thread, NULL, temperature_fn, (void *)args );

//   pthread_join( logger_thread, NULL );
//   pthread_join( light_thread, NULL );
   pthread_join( temp_thread, NULL );

   clock_gettime(CLOCK_REALTIME, &time);

   print_header( log->fid );
   fprintf( log->fid, "All threads exited! Main thread exiting... " );
   fprintf( log->fid, "End Time: %ld.%ld secs\n",
            time.tv_sec, time.tv_nsec );
   fclose( log->fid );

   print_header( stdout );
   fprintf( stdout, "All threads exited! Main thread exiting... " );
   fprintf( stdout, "End Time: %ld.%ld secs\n",
            time.tv_sec, time.tv_nsec );
   free( log );
   free( args );
   return 0;
}
