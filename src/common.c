#include "common.h"
#include <time.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/syscall.h>

void print_header( FILE* file )
{
   struct timespec time;
   clock_gettime(CLOCK_REALTIME, &time);
   fprintf( file, "\n=====================================================\n" );
   fprintf( file, "Thread [%d]: %ld.%ld secs\n",
            (pid_t)syscall(SYS_gettid), time.tv_sec, time.tv_nsec );
   fflush( file );
   fprintf( stdout, "\n=====================================================\n" );
   fprintf( stdout, "Thread [%d]: %ld.%ld secs\n",
            (pid_t)syscall(SYS_gettid), time.tv_sec, time.tv_nsec );
   fflush( stdout );

   return;
}

void thread_exit( file_t* log, int exit_status )
{
   struct timespec time;
   clock_gettime(CLOCK_REALTIME, &time);

   while( pthread_mutex_lock(&mutex) );
   print_header( log->fid );

   switch( exit_status )
   {
      case SIGUSR1:
         fprintf( stdout, "Caught SIGUSR1 Signal! Exiting...\n");
         fprintf( log->fid, "Caught SIGUSR1 Signal! Exiting...\n");
         fflush( log->fid );
         break;
      case SIGUSR2:
         fprintf( stdout, "Caught SIGUSR2 Signal! Exiting...\n");
         fprintf( log->fid, "Caught SIGUSR2 Signal! Exiting...\n");
         fflush( log->fid );
         break;
      default:
         break;
   }
   fprintf( stdout, "Goodbye World! End Time: %ld.%ld secs\n",
            time.tv_sec, time.tv_nsec );
   fprintf( log->fid, "Goodbye World! End Time: %ld.%ld secs\n",
            time.tv_sec, time.tv_nsec );
   fflush( log->fid );
   fclose( log->fid );
   pthread_mutex_unlock(&mutex);

   free( log );
   pthread_exit(EXIT_SUCCESS);
}
