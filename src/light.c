/**
 * =================================================================================
 *    @file     light.c
 *    @brief   Interface to APDS9301 Light Sensor
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


#include "watchdog.h"
#include "light.h"
#include "led.h"

#include <errno.h>
#include <time.h>
#include <string.h>
#include <math.h>

static timer_t    timerid;
struct itimerspec trigger;

static i2c_handle_t i2c_apds9301;
static float last_lux_value = -5;
static mqd_t light_queue;
static shared_data_t *shm;

/**
 * =================================================================================
 * Function:       sig_handler
 * @brief   Signal handler for light sensor thread.
 *          On normal operation, we should be receving SIGUSR1/2 signals from watchdog
 *          when prompted to exit. So, we close the message queue and timer this thread owns
 *
 * @param   signo - enum with signal number of signal being handled
 * @return  void
 * =================================================================================
 */
static void sig_handler( int signo )
{
   if( signo == SIGUSR1 )
   {
      printf("Received SIGUSR1! Exiting...\n");
      mq_close( light_queue );
      timer_delete( timerid );
      apds9301_power( POWER_OFF );
      i2c_stop( &i2c_apds9301 );
      thread_exit( signo );
   }
   else if( signo == SIGUSR2 )
   {
      printf("Received SIGUSR2! Exiting...\n");
      mq_close( light_queue );
      timer_delete( timerid );
      apds9301_power( POWER_OFF );
      i2c_stop( &i2c_apds9301 );
      thread_exit( signo );
   }
   return;
}

/**
 * =================================================================================
 * Function:       timer_handler
 * @brief   Timer handler function for light sensor thread
 *          When woken up by the timer, get lux reading and write state to shared memory
 *
 * @param   sig
 * @return  void
 * =================================================================================
 */
static void timer_handler( union sigval sig )
{
   static int i = 0;
   led_toggle( LED1_BRIGHTNESS );
   sem_wait(&shm->w_sem);

   print_header(shm->header);
   float lux = -5;
   int retVal = apds9301_get_lux( &lux );

   i++;
   if( EXIT_CLEAN != retVal )
   {
      sprintf( shm->buffer, "cycle[%d]: could not get light reading!\n", i );
      fprintf( stderr, "cycle[%d]: could not get light reading!\n", i );
   }
   else
   {
      /* save new lux value */
      last_lux_value = lux;
      if( DARK_THRESHOLD > lux )
      {
         sprintf( shm->buffer, "cycle[%d]: State: %s, Lux: %0.5f\n",
                  i, "NIGHT", lux );
         fprintf( stderr, "cycle[%d]: State: %s, Lux: %0.5f\n",
                  i, "NIGHT", lux );
      }
      else
      {
         sprintf( shm->buffer, "cycle[%d]: State: %s, Lux: %0.5f\n",
                  i, "DAY", lux );
         fprintf( stderr, "cycle[%d]: State: %s, Lux: %0.5f\n",
                  i, "DAY", lux );
      }
   }

   sem_post(&shm->r_sem);
   led_toggle( LED1_BRIGHTNESS );
   return;
}


/**
 * =================================================================================
 * Function:       get_lux
 * @brief   Returns last lux reading
 *
 * @param   void
 * @return  last_lux_value - last lux reading we have
 * =================================================================================
 */
float get_lux( void )
{
   return last_lux_value;
}


/**
 * =================================================================================
 * Function:       is_dark
 * @brief   Returns int speciyfing if it is night or day
 *
 * @param   void
 * @return  night - 0 if it is day, 1 if night, i.e. below DARK_THRESHOLD
 * =================================================================================
 */
int is_dark( void )
{
   int dark = 0;
   if( DARK_THRESHOLD > last_lux_value )
   {
      dark = 1;
   }
   return dark;
}


/**
 * =================================================================================
 * Function:       cycle
 * @brief   Cycle function for light sensor thread
 *          We wait in this while loop checking for requests from watchdog for health status
 *
 * @param   void
 * @return  void
 * =================================================================================
 */
static void cycle( void )
{
   int retVal = 0;
   msg_t request = {0};
   msg_t response = {0};
   while( 1 )
   {
      memset( &request, 0, sizeof( request ) );
      retVal = mq_receive( light_queue, (char*)&request, sizeof( request ), NULL );
      if( 0 > retVal )
      {
         int errnum = errno;
         fprintf( stderr, "Encountered error receiving from message queue %s: (%s)\n",
                  LIGHT_QUEUE_NAME, strerror( errnum ) );
         continue;
      }
      switch( request.id )
      {
         case REQUEST_STATUS:
            sem_wait(&shm->w_sem);
            print_header(shm->header);
            sprintf( shm->buffer, "(Light) I am alive!\n" );
            sem_post(&shm->r_sem);
            fprintf( stdout, "(Light) I am alive!\n" );
            response.id = request.id;
            sprintf( response.info, "(Light) I am alive!\n" );
            retVal = mq_send( request.src, (const char*)&response, sizeof( response ), 0 );

            pthread_mutex_lock( &alive_mutex );
            threads_status[THREAD_LIGHT]--;
            pthread_mutex_unlock( &alive_mutex );
            break;
         default:
            break;
      }
   }
   return;
}

/**
 * =================================================================================
 * Function:       get_light_queue
 * @brief   Get file descriptor for light sensor thread.
 *          Called by watchdog thread in order to be able to send heartbeat check via queue
 *
 * @param   void
 * @return  temp_queue - file descriptor for light sensor thread message queue
 * =================================================================================
 */
mqd_t get_light_queue( void )
{
   return light_queue;
}

/**
 * =================================================================================
 * Function:       light_queue_init
 * @brief   Initialize message queue for light sensor thread
 *
 * @param   void
 * @return  msg_q - file descriptor for initialized message queue
 * =================================================================================
 */
int light_queue_init( void )
{
   /* unlink first in case we hadn't shut down cleanly last time */
   mq_unlink( LIGHT_QUEUE_NAME );

   struct mq_attr attr;
   attr.mq_flags = 0;
   attr.mq_maxmsg = MAX_MESSAGES;
   attr.mq_msgsize = sizeof( msg_t );
   attr.mq_curmsgs = 0;

   int msg_q = mq_open( LIGHT_QUEUE_NAME, O_CREAT | O_RDWR, 0666, &attr );
   if( 0 > msg_q )
   {
      int errnum = errno;
      sem_wait(&shm->w_sem);
      print_header(shm->header);
      sprintf( shm->buffer, "Encountered error creating message queue %s: (%s)\n",
               LIGHT_QUEUE_NAME, strerror( errnum ) );
      sem_post(&shm->r_sem);
   }
   return msg_q;
}

/**
 * =================================================================================
 * Function:       light_fn
 * @brief   Entry point for light sensor processing thread
 *
 * @param   thread_args - void ptr to arguments used to initialize thread
 * @return  NULL  - We don't really exit from this function,
 *                   since the exit point is thread_exit()
 * =================================================================================
 */
void *light_fn( void *thread_args )
{
   /* Get time that thread was spawned */
   struct timespec time;
   clock_gettime(CLOCK_REALTIME, &time);
   shm = get_shared_memory();

   /* Write initial state to shared memory */
   sem_wait(&shm->w_sem);
   print_header(shm->header);
   sprintf( shm->buffer, "Hello World! Start Time: %ld.%ld secs\n",
            time.tv_sec, time.tv_nsec );
   /* Signal to logger that shared memory has been updated */
   sem_post(&shm->r_sem);

   signal(SIGUSR1, sig_handler);
   signal(SIGUSR2, sig_handler);

   light_queue = light_queue_init();
   if( 0 > light_queue )
   {
      thread_exit( EXIT_INIT );
   }

   int retVal = i2c_init( &i2c_apds9301 );
   if( EXIT_CLEAN != retVal )
   {
      sem_wait(&shm->w_sem);
      print_header(shm->header);
      sprintf( shm->buffer, "ERROR: Failed to initialize I2C for light sensor!\n" );
      sem_post(&shm->r_sem);
      thread_exit( EXIT_INIT );
   }
   retVal = apds9301_power( POWER_ON );
   if( EXIT_CLEAN != retVal )
   {
      sem_wait(&shm->w_sem);
      print_header(shm->header);
      sprintf( shm->buffer, "ERROR: Failed to power on light sensor!\n" );
      sem_post(&shm->r_sem);
      thread_exit( EXIT_INIT );
   }

   timer_setup( &timerid, &timer_handler );

   timer_start( &timerid, FREQ_1HZ );
   cycle();

   thread_exit( 0 );
   return NULL;
}
