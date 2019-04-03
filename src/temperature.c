/*
 * =================================================================================
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
 * =================================================================================
 */

#include "watchdog.h"
#include "temperature.h"
#include "led.h"

#include <errno.h>
#include <time.h>
#include <string.h>

static timer_t    timerid;
struct itimerspec trigger;

static i2c_handle_t i2c_tmp102;
static mqd_t temp_queue;
static shared_data_t *shm;

const tmp102_config_t tmp102_default_config = {
   .mode = {                                 /* shutdown and thermostat modes */
      .shutdown = TMP102_SHUTDOWN_MODE,
      .thermostat = TMP102_THERMOSTAT_MODE
   },
   .polarity = TMP102_POLARITY,              /* polarity */
   .fault_queue = TMP102_FAULT_QUEUE,        /* fault queue */
   .resolution = {                           /* converter resolution */
      .res_0 = TMP102_RESOLUTION_0,
      .res_1 = TMP102_RESOLUTION_1
   },
   .one_shot = 1,                            /* when in shutdown mode, writing 1 starts a single conversion */
   .operation = TMP102_EXTENDED_MODE,        /* extended vs normal operation */
   .alert = 1,                               /* alter bit */
   .conv_rate = TMP102_CONVERSION_RATE       /* conversion rate */
};


/*
 * =================================================================================
 * Function:       tmp102_write_config
 * @brief   Write configuration register of TMP102 sensor
 *
 * @param   *config_reg  - pointer to struct with values to write to configuration register
 * @return  see i2c_write()
 * =================================================================================
 */
int tmp102_write_config( tmp102_config_t *config_reg )
{
   int retVal = i2c_write( TMP102_SLAVE, TMP102_REG_CONFIG, *((uint16_t*)&config_reg) );

   return retVal;
}

/*
 * =================================================================================
 * Function:       tmp102_get_temp
 * @brief   Read temperature registers fo TMP102 sensor and decode temperature value
 *
 * @param   *temperature - pointer to location to write decoded value to
 * @return  EXIT_CLEAN if successful, otherwise EXIT_ERROR
 * =================================================================================
 */
int tmp102_get_temp( float *temperature )
{
   uint8_t buffer[2] = {0};
   int retVal = i2c_read( TMP102_SLAVE, TMP102_REG_TEMP, buffer, sizeof( buffer ) );
   if( 0 > retVal )
   {
      return EXIT_ERROR;
   }

   uint16_t tmp = 0;
   tmp = 0xfff & ( ((uint16_t)buffer[0] << 4 ) | (buffer[1] >> 4 ) ); /* buffer[0] = MSB(15:8)
                                                                        buffer[1] = LSB(7:4) */
   if( 0x800 & tmp )
   {
      tmp = ( (~tmp ) + 1 ) & 0xfff;
      *temperature = -1.0 * (float)tmp * 0.0625;
   }
   else
   {
      *temperature = ((float)tmp) * 0.0625;
   }

   return EXIT_CLEAN;
}

/*
 * =================================================================================
 * Function:       tmp102_write_thigh
 * @brief   Write value thigh (in celsius) to Thigh register for TMP102 sensor
 *
 * @param   thigh - value to write to Thigh register
 * @return  EXIT_CLEAN if successful, otherwise EXIT_ERROR
 * =================================================================================
 */
int tmp102_write_thigh( float thigh )
{
   if( (-56.0 > thigh) || (151.0 < thigh) )
   {
      thigh = 80.0;
   }

   thigh /= 0.0625;
   uint16_t tmp;

   if( 0 > thigh )
   {
      tmp = ( (uint16_t)thigh << 4 );
      tmp &= 0x7fff;
   }
   else
   {
      thigh *= -1;
      tmp = (uint16_t)thigh;
      tmp = ~(tmp) + 1;
      tmp = tmp << 4;
   }

   int retVal = i2c_write( TMP102_SLAVE, TMP102_THIGH, tmp );
   if( 0 > retVal )
   {
      sem_wait(&shm->w_sem);
      print_header(shm->header);
      sprintf( shm->buffer, "Could not write value to THigh register!\n" );
      sem_post(&shm->r_sem);
      return EXIT_ERROR;
   }

   return EXIT_CLEAN;
}

/*
 * =================================================================================
 * Function:       tmp102_write_tlow
 * @brief   Write value tlow (in celsius) to Tlow register for TMP102 sensor
 *
 * @param   tlow - value to write to Tlow register
 * @return  EXIT_CLEAN if successful, otherwise EXIT_ERROR
 * =================================================================================
 */
int tmp102_write_tlow( float tlow )
{
   if( (-56.0 > tlow) || (151.0 < tlow ) )
   {
      tlow = 75.0;
   }

   tlow /= 0.0625;
   uint16_t tmp;

   if( 0 < tlow )
   {
      tmp = ( (uint16_t)tlow << 4 );
      tmp &= 0x7fff;
   }
   else
   {
      tlow *= -1;
      tmp = (uint16_t)tlow;
      tmp = ~(tmp) + 1;
      tmp = tmp << 4;
   }

   int retVal = i2c_write( TMP102_SLAVE, TMP102_TLOW, tmp );
   if( 0 > retVal )
   {
      sem_wait(&shm->w_sem);
      print_header(shm->header);
      sprintf( shm->buffer, "Could not write value to TLow register!\n" );
      sem_post(&shm->r_sem);
      return EXIT_ERROR;
   }

   return EXIT_CLEAN;
}

/*
 * =================================================================================
 * Function:       tmp102_read_thigh
 * @brief   Read value of THigh register of TMP102 sensor and store value (in celsius) in thigh
 *
 * @param   thigh - pointer to location to store decoded temperature value to
 * @return  EXIT_CLEAN if successful, EXIT_ERROR otherwise
 * =================================================================================
 */
int tmp102_read_thigh( float *thigh )
{
   uint16_t tmp = 0;

   int retVal = i2c_read( TMP102_SLAVE, TMP102_THIGH, (uint8_t*)&tmp, sizeof( tmp ) );
   if( 0 > retVal )
   {
      sem_wait(&shm->w_sem);
      print_header(shm->header);
      sprintf( shm->buffer, "Could not read from TLow register!\n" );
      sem_post(&shm->r_sem);
      return EXIT_ERROR;
   }

   if( tmp & 0x800 )
   {
      tmp = ~(tmp) + 1;
      *thigh = -1 * ( (float)tmp * 0.0625 );
   }
   else
   {
      *thigh = (float)tmp * 0.0625;
   }

   return EXIT_CLEAN;
}


/*
 * =================================================================================
 * Function:       tmp102_read_tlow
 * @brief   Read value of TLow register of TMP102 sensor and store value (in celsius) in tlow
 *
 * @param   tlow  - pointer to location to store decoded temperature value to
 * @return  EXIT_CLEAN if successful, EXIT_ERROR otherwise
 * =================================================================================
 */
int tmp102_read_tlow( float *tlow )
{
   uint16_t tmp = 0;

   int retVal = i2c_read( TMP102_SLAVE, TMP102_TLOW, (uint8_t*)&tmp, sizeof( tmp ) );
   if( 0 > retVal )
   {
      sem_wait(&shm->w_sem);
      print_header(shm->header);
      sprintf( shm->buffer, "Could not read from TLow register!\n" );
      sem_post(&shm->r_sem);
      return retVal;
   }

   if( tmp & 0x800 )
   {
      tmp = ~(tmp) + 1;
      *tlow = -1 * (float)tmp * 0.0625;
   }
   else
   {
      *tlow = (float)tmp * 0.0625;
   }

   return retVal;
}



/*
 * =================================================================================
 * Function:       sig_handler
 * @brief   Signal handler for temperature sensor thread.
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

/*
 * =================================================================================
 * Function:       timer_handler
 * @brief   Timer handler function for temperature sensor thread
 *          When woken up by the timer, get temperature and write state to shared memory
 *
 * @param   sig
 * @return  void
 * =================================================================================
 */
static void timer_handler( union sigval sig )
{
   static int i = 0;
   led_toggle( LED0_BRIGHTNESS );
   sem_wait(&shm->w_sem);
   
   print_header(shm->header);
   float temperature;
   int retVal = tmp102_get_temp( &temperature );
   i++;
   if( retVal )
   {
      sprintf( shm->buffer, "cycle[%d]: %0.5f Celsius\n", i, temperature );
   }
   else
   {
      sprintf( shm->buffer, "cycle[%d]: could not get temperature reading!\n", i );
   }

   sem_post(&shm->r_sem);
   led_toggle( LED0_BRIGHTNESS );
   return;
}


/*
 * =================================================================================
 * Function:       cycle
 * @brief   Cycle function for temperature sensor thread
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
      retVal = mq_receive( temp_queue, (char*)&request, sizeof( request ), NULL );
      if( 0 > retVal )
      {
         int errnum = errno;
         sem_wait(&shm->w_sem);
         sprintf( shm->buffer, "ERROR: Encountered error receiving from message queue %s: (%s)\n",
                  TEMP_QUEUE_NAME, strerror( errnum ) );
         sem_post(&shm->r_sem);
         continue;
      }
      switch( request.id )
      {
         case REQUEST_STATUS:
            sem_wait(&shm->w_sem);
            print_header(shm->header);
            sprintf( shm->buffer, "(Temperature) I am alive!\n" );
            sem_post(&shm->r_sem);
            fprintf( stdout, "(Temperature) I am alive!\n" );
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

/*
 * =================================================================================
 * Function:       get_temperature_queue
 * @brief   Get file descriptor for temperature sensor thread.
 *          Called by watchdog thread in order to be able to send heartbeat check via queue
 *
 * @param   void
 * @return  temp_queue - file descriptor for temperature sensor thread message queue
 * =================================================================================
 */
mqd_t get_temperature_queue( void )
{
   return temp_queue;
}

/*
 * =================================================================================
 * Function:       temp_queue_init
 * @brief   Initialize message queue for temperature sensor thread
 *
 * @param   void
 * @return  msg_q - file descriptor for initialized message queue
 * =================================================================================
 */
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
      sem_wait(&shm->w_sem);
      print_header(shm->header);
      sprintf( shm->buffer, "ERROR: Encountered error creating message queue %s: (%s)\n",
               TEMP_QUEUE_NAME, strerror( errnum ) );
      sem_post(&shm->r_sem);     
   }
   return msg_q;
}

/*
 * =================================================================================
 * Function:       temperature_fn
 * @brief   Entry point for temperature sensor processing thread
 *
 * @param   thread_args - void ptr to arguments used to initialize thread
 * @return  NULL  - We don't really exit from this function, 
 *                   since the exit point is thread_exit()
 * =================================================================================
 */
void *temperature_fn( void *thread_args )
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

   temp_queue = temp_queue_init();
   if( 0 > temp_queue )
   {
      thread_exit( EXIT_INIT );
   }

   int retVal = i2c_init( &i2c_tmp102 );
   if( EXIT_INIT == retVal )
   {
      sem_wait(&shm->w_sem);
      print_header(shm->header);
      sprintf( shm->buffer, "ERROR: Failed to initialize I2C for temperature sensor!\n" );
      sem_post(&shm->r_sem);
      thread_exit( EXIT_INIT );
   }

   timer_setup( &timerid, &timer_handler );

   timer_start( &timerid, 1000000 );
   cycle();

   thread_exit( 0 );
   return NULL;
}
