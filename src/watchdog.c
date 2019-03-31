/*
 * =================================================================================
 *    @file     watchdog.c
 *    @brief
 *
 *  <+DETAILED+>
 *
 *    @author   Roberto Baquerizo (baquerrj), roba8460@colorado.edu
 *
 *    @internal
 *       Created:  03/15/2019
 *      Revision:  none
 *      Compiler:  gcc
 *  Organization:  University of Colorado: Boulder
 *
 *  This source code is released for free distribution under the terms of the
 *  GNU General Public License as published by the Free Software Foundation.
 * =================================================================================
 */


#include "watchdog.h"
#include "common.h"

#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define NUM_THREADS  2
/* Threads to ping */
//static pthread_t threads[NUM_THREADS];
static struct thread_id_s* threads;

static void sig_handler( int signo )
{
   if( SIGUSR2 == signo )
   {
      fprintf( stdout, "watchdog caught signals - killing thread [%ld]\n", threads->t1 );
      fflush( stdout );
      pthread_kill( threads->t1, SIGUSR1 );
      fprintf( stdout, "watchdog caught signals - killing thread [%ld]\n", threads->t2 );
      fflush( stdout );
      pthread_kill( threads->t2, SIGUSR1 );
      free( threads );
      thread_exit( 0 );
   }
}

/*
 * =================================================================================
 * Function:       check_threads
 * @brief
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
int check_threads( void )
{
   return 0;
}

/*
 * =================================================================================
 * Function:       watchdog_init
 * @brief
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
int watchdog_init( pthread_t *threads )
{
   return 0;
}

/*
 * =================================================================================
 * Function:       watchdog_fn
 * @brief
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
void *watchdog_fn( void *thread_args )
{
   signal( SIGUSR2, sig_handler );
   exit_e retVal = EXIT_ERROR;
   if( NULL == thread_args )
   {
      print_header( NULL );
      fprintf( stderr, "Encountered null pointer!\n" );
      pthread_exit(&retVal);
   }
   else
   {
      threads = malloc( sizeof( struct thread_id_s ) );
      threads = (struct thread_id_s*)thread_args;

//      fprintf( stdout,"Processed thread %ld\n", threads->t1 );
//      fprintf( stdout,"Processed thread %ld\n", threads->t2 );
   }
   while(1);


   return NULL;
}

