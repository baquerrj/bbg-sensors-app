/*
 * =================================================================================
 *    @file     common.c
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



/*
 * =================================================================================
 * Function:       print_header
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
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


/*
 * =================================================================================
 * Function:       thread_exit
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
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



/*
 * =================================================================================
 * Function:       get_shared_memory
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
void *get_shared_memory( void )
{
   struct shared_data *shm_p;

   shm_fd = shm_open( SHM_SEGMENT_NAME, O_CREAT | O_EXCL | O_RDWR, 0666 );
   if( 0 < shm_fd )
   {
      fprintf( stdout, "Creating shared memroy and setting size to %lu bytes\n",
               sizeof( shared_data_t ) );

      if( 0 > ftruncate( shm_fd, sizeof( shared_data_t )) )
      {
         perror( "Encountered error setting size of shared memroy" );
         exit(1);
      }
	}
   else if( -1 == shm_fd && EEXIST == errno )
   {
		/* Already exists: open again without O_CREAT */
		shm_fd = shm_open(SHM_SEGMENT_NAME, O_RDWR, 0);
	}
   else if (shm_fd == -1)
   {
		perror( "shm_open " SHM_SEGMENT_NAME );
		exit(1);
	}

	/* Map the shared memory */
	shm_p = mmap(NULL, sizeof( shared_data_t ), PROT_READ | PROT_WRITE,
		     MAP_SHARED, shm_fd, 0);

	if (shm_p == NULL) {
		perror( "Encountered error memory mapping shared memory" );
		exit(1);
	}
	return shm_p;
}



/*
 * =================================================================================
 * Function:       sems_init
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
int sems_init( shared_data_t *shm )
{
   int retVal = 0;
   retVal = sem_init( &shm->w_sem, 0, 1 );
   if( 0 > retVal )
   {
      int errnum = errno;
      fprintf( stderr, "Encountered enrror initializing write semaphore: %s\n",
               strerror( errnum ) );
      return retVal;
   }
   retVal = sem_init( &shm->r_sem, 0, 1 );
   if( 0 > retVal )
   {
      int errnum = errno;
      fprintf( stderr, "Encountered enrror initializing read semaphore: %s\n",
               strerror( errnum ) );
      return retVal;
   }
   return retVal;
}
