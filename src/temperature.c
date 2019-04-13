/*!
 *    @file     temperature.c
 *    @brief   Source file implementing temperature.h
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
 */

#include "tmp102_sensor.h"
#include "watchdog.h"
#include "temperature.h"
#include "led.h"

#include <errno.h>
#include <time.h>
#include <string.h>

static timer_t    timerid;
struct itimerspec trigger;

static i2c_handle_t i2c_tmp102;
static float last_temp_value = -5;
static mqd_t temp_queue;
//static shared_data_t *shm;

float get_temperature( void )
{
   return last_temp_value;
}

/*!
 * Function:       sig_handler
 * @brief   Signal handler for temperature sensor thread.
 *          On normal operation, we should be receving SIGUSR1/2 signals from watchdog
 *          when prompted to exit. So, we close the message queue and timer this thread owns
 *
 * @param   signo - enum with signal number of signal being handled
 * @return  void
 */
static void sig_handler( int signo )
{
   if( signo == SIGUSR1 )
   {
      printf("Received SIGUSR1! Exiting...\n");
      mq_close( temp_queue );
      timer_delete( timerid );
      i2c_stop( &i2c_tmp102 );
      thread_exit( signo );
   }
   else if( signo == SIGUSR2 )
   {
      printf("Received SIGUSR2! Exiting...\n");
      mq_close( temp_queue );
      timer_delete( timerid );
      i2c_stop( &i2c_tmp102 );
      thread_exit( signo );
   }
   return;
}

void tmp102_handler( union sigval sig )
{
   static int i = 0;
   led_toggle( LED0_BRIGHTNESS );
//   sem_wait(&shm->w_sem);
   
//   print_header(shm->header);
   float temperature;
   int retVal = tmp102_get_temp( &temperature );
   i++;
   if( retVal )
   {
//      sprintf( shm->buffer, "cycle[%d]: could not get temperature reading!\n", i );
      LOG_ERROR( "CYCLE [%d] --- Couldn't read temperature\n", i );
   }
   else
   {
//      sprintf( shm->buffer, "cycle[%d]: %0.5f Celsius\n", i, temperature );
      LOG_INFO( "CYCLE [%d] --- TEMP: %0.5f Celsius\n", i, temperature );
   }

//   sem_post(&shm->r_sem);
   led_toggle( LED0_BRIGHTNESS );
   return;
}


void tmp102_cycle( void )
{
   int retVal = 0;
   msg_t request = {0};
   msg_t response = {0};
   while( 1 )
   {
      memset( &request, 0, sizeof( request ) );
      retVal = mq_receive( temp_queue, (char*)&request, sizeof( request ), NULL );
      if( 0 > retVal )
      {
         int errnum = errno;
//         sem_wait(&shm->w_sem);
//         sprintf( shm->buffer, "ERROR: Encountered error receiving from message queue %s: (%s)\n",
//                  TEMP_QUEUE_NAME, strerror( errnum ) );
//         sem_post(&shm->r_sem);
         LOG_ERROR( "Encountered error receiving from message queue %s: (%s)\n",
                  TEMP_QUEUE_NAME, strerror( errnum ) );
         continue;
      }
      switch( request.id )
      {
         case REQUEST_STATUS:
//            sem_wait(&shm->w_sem);
//            print_header(shm->header);
//            sprintf( shm->buffer, "(Temperature) I am alive!\n" );
//            sem_post(&shm->r_sem);
            LOG_INFO( "(Temperature) I am alive!\n" );
            response.id = request.id;
            sprintf( response.info, "(Temperature) I am alive!\n" );
            retVal = mq_send( request.src, (const char*)&response, sizeof( response ), 0 );

            pthread_mutex_lock( &alive_mutex );
            threads_status[THREAD_TEMP]--;
            pthread_mutex_unlock( &alive_mutex );
            break;
         default:
            break;
      }
   }
   return;
}

mqd_t get_temperature_queue( void )
{
   return temp_queue;
}

int temp_queue_init( void )
{
   /* unlink first in case we hadn't shut down cleanly last time */
   mq_unlink( TEMP_QUEUE_NAME );

   struct mq_attr attr;
   attr.mq_flags = 0;
   attr.mq_maxmsg = MAX_MESSAGES;
   attr.mq_msgsize = sizeof( msg_t );
   attr.mq_curmsgs = 0;

   int msg_q = mq_open( TEMP_QUEUE_NAME, O_CREAT | O_RDWR, 0666, &attr );
   if( 0 > msg_q )
   {
      int errnum = errno;
//      sem_wait(&shm->w_sem);
//      print_header(shm->header);
//      sprintf( shm->buffer, "ERROR: Encountered error creating message queue %s: (%s)\n",
//               TEMP_QUEUE_NAME, strerror( errnum ) );
//      sem_post(&shm->r_sem);
      LOG_ERROR( "Could not create message queue for logger task: %s\n",
                  strerror( errnum ) );

   }
   return msg_q;
}

void *temperature_fn( void *thread_args )
{
   /* Get time that thread was spawned */
   struct timespec time;
   clock_gettime(CLOCK_REALTIME, &time);
//   shm = get_shared_memory();

   /* Write initial state to shared memory */
//   sem_wait(&shm->w_sem);
//   print_header(shm->header);
//   sprintf( shm->buffer, "Hello World! Start Time: %ld.%ld secs\n",
//            time.tv_sec, time.tv_nsec );
   /* Signal to logger that shared memory has been updated */
//   sem_post(&shm->r_sem);

   signal(SIGUSR1, sig_handler);
   signal(SIGUSR2, sig_handler);

   temp_queue = temp_queue_init();
   if( 0 > temp_queue )
   {
      thread_exit( EXIT_INIT );
   }

   int retVal = i2c_init( &i2c_tmp102 );
   if( EXIT_INIT == retVal )
   {
//      sem_wait(&shm->w_sem);
//      print_header(shm->header);
//      sprintf( shm->buffer, "ERROR: Failed to initialize I2C for temperature sensor!\n" );
//      sem_post(&shm->r_sem);
      thread_exit( EXIT_INIT );
   }

   timer_setup( &timerid, &tmp102_handler );

   timer_start( &timerid, FREQ_1HZ );
   tmp102_cycle();

   thread_exit( 0 );
   return NULL;
}
