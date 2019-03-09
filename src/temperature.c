#include "temperature.h"
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

static timer_t    timerid;
struct itimerspec trigger;

static file_t *log;

static pthread_mutex_t  tmutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t   tcond = PTHREAD_COND_INITIALIZER;

static void sig_handler( int signo )
{
   if( signo == SIGUSR1 )
   {
      printf("Received SIGUSR1! Exiting...\n");
      thread_exit( log, signo );
   }
   else if( signo == SIGUSR2 )
   {
      printf("Received SIGUSR2! Exiting...\n");
      thread_exit( log, signo );
   }
   else if( signo == SIGCONT )
   {
      pthread_cond_broadcast(&tcond);
   }
   return;
}

static int report_cpu( void )
{
   while( 1 )
   {
      pthread_mutex_lock(&tmutex);
      pthread_cond_wait(&tcond, &tmutex);
      pthread_mutex_unlock(&tmutex);

      pthread_mutex_lock(&mutex);
      print_header( log->fid );
      pthread_mutex_unlock(&mutex);
   }
   return 0;
}

static int setup_timer( void )
{
   /* Set up timer */
   char info[] = "timer woken up!";
   struct sigevent sev;

   memset(&sev, 0, sizeof(struct sigevent));
   memset(&trigger, 0, sizeof(struct itimerspec));

   sev.sigev_notify = SIGEV_SIGNAL;
   sev.sigev_signo = SIGCONT;
   signal(SIGCONT, sig_handler);
   sev.sigev_value.sival_ptr = &info;

   timer_create(CLOCK_REALTIME, &sev, &timerid);

   trigger.it_value.tv_sec = 1;
   trigger.it_interval.tv_nsec = 100 * 1000000;
   trigger.it_interval.tv_sec = 1;

   pthread_mutex_init( &tmutex, NULL );
   pthread_cond_init( &tcond, NULL );

   timer_settime( timerid, 0, &trigger, NULL );
   return 0;
}

void *temperature_fn(void *arg)
{
   /* Get time that thread was spawned */
   struct timespec time;
   clock_gettime(CLOCK_REALTIME, &time);

   static int failure = 1;
   /* Initialize thread */
   if( NULL == arg )
   {
      fprintf( stderr, "Thread requires name of log file!\n" );
      pthread_exit(&failure);
   }

   /* Take mutex to write to file */
   pthread_mutex_lock(&mutex);

   log = malloc( sizeof( file_t ) );
   if( NULL == log )
   {
      fprintf( stderr, "Ecountered error allocating memory for log file!\n");
      pthread_exit(&failure);
   }

   args_t *args = arg;
   log->name = (char *)args->arg1;
   log->fid = fopen( log->name, "a+" );
   if( NULL == log->fid )
   {
      perror( "Ecountered error opening log file!\n" );
      pthread_exit(&failure);
   }

   print_header( log->fid );
   fprintf( log->fid, "Hello World! Start Time: %ld.%ld secs\n",
            time.tv_sec, time.tv_nsec );

   /* Release file mutex */
   pthread_mutex_unlock(&mutex);

   signal(SIGUSR1, sig_handler);
   signal(SIGUSR2, sig_handler);

   setup_timer();
   report_cpu();
   thread_exit( log, 0 );
   return NULL;
}
