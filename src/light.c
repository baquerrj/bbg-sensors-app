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
   if( retVal )
   {
      /* save new lux value */
      last_lux_value = lux;
      if( DARK_THRESHOLD > lux )
      {
         sprintf( shm->buffer, "cycle[%d]: State: %s, Lux: %0.5f\n",
                  i, "NIGHT", lux );
      }
      else
      {
         sprintf( shm->buffer, "cycle[%d]: State: %s, Lux: %0.5f\n",
                  i, "DAY", lux );
      }
   }
   else
   {
      sprintf( shm->buffer, "cycle[%d]: could not get light reading!\n", i );
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
 * Function:       apds9301_set_config
 * @brief   Set configuration of light sensor. For the APDS9301, the configuration
 *          is spread out across the: Timing Register, Interrupt Control Register,
 *          and Control Register. So, I have to write to all of these to set the config
 *
 * @param   void
 * @return  EXIT_CLEAN if successful, otherwise see i2c_write()
 * =================================================================================
 */
int apds9301_set_config( void )
{
   int retVal = apds9301_set_gain( DEFAULT_GAIN );
   if( retVal )
   {
      return retVal;
   }
   else
   {
      retVal = apds9301_set_interrupt( DEFAULT_INTERRUPT );
      if( retVal )
      {
         return retVal;
      }
      else
      {
         retVal = apds9301_set_integration( DEFAULT_INTEGRATION_TIME );
         if( retVal )
         {
            return retVal;
         }
      }
   }
   return EXIT_CLEAN;
}

/**
 * =================================================================================
 * Function:       apds9301_set_integration
 * @brief   Sets the integration time for APDS9301 by writing a value to bits
 *          INTEG of the Timing Register
 *
 * @param   val   - value to write to timing register
 * @return  see i2c_write_byte() - if val is not an allowed value, EXIT_ERROR
 * =================================================================================
 */
int apds9301_set_integration( uint8_t val )
{
   if( 3 < val )
   {
      /* invalid value */
      return EXIT_ERROR;
   }
   uint8_t data;
   int retVal = i2c_read( APDS9301_ADDRESS, APDS9301_REG_TIME, &data, sizeof( data ) );

   if( retVal )
   {
      return EXIT_ERROR;
   }

   data &= ~(0b11);  /* clears lower 2 bits of TIMING REG */
   data |= val;

   retVal = i2c_write_byte( APDS9301_ADDRESS, APDS9301_REG_TIME, data );

   return retVal;
}


/**
 * =================================================================================
 * Function:       apds9301_clear_interrupt
 * @brief   Clears any pending interrupt for APDS9301 by writing a 1 to the CLEAR bit
 *          of the Command Register
 *
 * @param   void
 * @return  see i2c_set()
 * =================================================================================
 */
int apds9301_clear_interrupt( void )
{
   uint8_t clear = APDS9301_REG_CMD | CMD_CLEAR_INTR;

   int retVal = i2c_set( APDS9301_ADDRESS, clear );

   return retVal;
}


/**
 * =================================================================================
 * Function:       apds9301_set_interrupt
 * @brief   Enables or disables interrupts for APDS9301 by setting or clearing the
 *          INTR bits of the Interrupt Control Register
 *
 * @param   enable - set if we want to enable interrupts
 * @return  see i2c_write_byte()
 * =================================================================================
 */
int apds9301_set_interrupt( uint8_t enable )
{
   uint8_t data;
   int retVal = i2c_read( APDS9301_ADDRESS, APDS9301_REG_INT_CNTRL, &data, sizeof( data ) );
   if( retVal )
   {
      return EXIT_ERROR;
   }

   if( enable )
   {
      data |= (1<<4);
   }
   else
   {
      data &= ~(1<<4);
   }

   retVal = i2c_write_byte( APDS9301_ADDRESS, APDS9301_REG_INT_CNTRL, data );

   return retVal;
}

/**
 * =================================================================================
 * Function:       apds9301_set_gain
 * @brief   Sets gain for APDS9301 by setting or clearing the GAIN bit of the
 *          Timing Register
 *
 * @param   gain  - set if we want high gain
 * @return  see i2c_write_byte()
 * =================================================================================
 */
int apds9301_set_gain( uint8_t gain )
{
   uint8_t data;
   int retVal = i2c_read( APDS9301_ADDRESS, APDS9301_REG_TIME, &data, sizeof( data ) );
   if( retVal )
   {
      return EXIT_ERROR;
   }

   /* if gain != 0, high gain */
   if( gain )
   {
      data |= (1<<4);
   }
   else
   {
      data &= ~(1<<4);
   }

   retVal = i2c_write_byte( APDS9301_ADDRESS, APDS9301_REG_TIME, data );

   return retVal;
}

/**
 * =================================================================================
 * Function:       apds9301_read_control
 * @brief   Read contents of Control Register
 *
 * @param   *data - where to store contents
 * @return  see i2c_read()
 * =================================================================================
 */
int apds9301_read_control( uint8_t* data )
{
   int retVal = i2c_read( APDS9301_ADDRESS, APDS9301_REG_CNTRL, data, sizeof( *data ) );
   return retVal;
}

/**
 * =================================================================================
 * Function:       apds9301_write_threshold_low
 * @brief   Write value to low threshold register
 *
 * @param   threshold   - value to write
 * @return  see i2c_write()
 * =================================================================================
 */
int apds9301_write_threshold_low( uint16_t threshold )
{
   int retVal = i2c_write( APDS9301_ADDRESS, APDS9301_REG_TH_LL, threshold );
   return retVal;
}

/**
 * =================================================================================
 * Function:       apds9301_write_threshold_low
 * @brief   Read value from low threshold register
 *
 * @param   *threshold   - where to write value read
 * @return  see i2c_write()
 * =================================================================================
 */
int apds9301_read_threshold_low( uint16_t *threshold )
{
   int retVal = i2c_read( APDS9301_ADDRESS, APDS9301_REG_TH_LL, (uint8_t*)threshold, sizeof( *threshold ) );
   return retVal;
}

/**
 * =================================================================================
 * Function:       apds9301_write_threshold_high
 * @brief   Write value to high threshold register
 *
 * @param   threshold   - value to write
 * @return  see i2c_write()
 * =================================================================================
 */
int apds9301_write_threshold_high( uint16_t threshold )
{
   int retVal = i2c_write( APDS9301_ADDRESS, APDS9301_REG_TH_HL, threshold );
   return retVal;
}

/**
 * =================================================================================
 * Function:       apds9301_write_threshold_high
 * @brief   Read value from high threshold register
 *
 * @param   *threshold   - where to write value read
 * @return  see i2c_write()
 * =================================================================================
 */
int apds9301_read_threshold_high( uint16_t *threshold )
{
   int retVal = i2c_read( APDS9301_ADDRESS, APDS9301_REG_TH_HL, (uint8_t*)threshold, sizeof( *threshold ) );
   return retVal;
}

/**
 * =================================================================================
 * Function:       apds9301_read_id
 * @brief   Read APDS9301 Identification Register
 *
 * @param   *id   - where to write ID from register
 * @return  see i2c_read()
 * =================================================================================
 */
int apds9301_read_id( uint8_t *id )
{
   int retVal = i2c_read( APDS9301_ADDRESS, APDS9301_REG_ID, id, sizeof( *id ) );
   return retVal;
}

/**
 * =================================================================================
 * Function:       apds9301_get_lux
 * @brief   Read ADC Registers and calculate lux in lumen using equations from
 *          APDS9301 datasheet
 *
 * @param   *lux  - pointer to location to write decoded lux to
 * @return  EXIT_CLEAN if successful, otherwise EXIT_ERROR
 * =================================================================================
 */
int apds9301_get_lux( float *lux )
{
   float ratio = 0;
   uint16_t data0 = 0;
   uint16_t data1 = 0;

   int retVal = apds9301_read_data0( &data0 );
   if( EXIT_CLEAN != retVal )
   {
      return EXIT_ERROR;
   }

   retVal = apds9301_read_data1( &data1 );
   if( EXIT_CLEAN != retVal )
   {
      return EXIT_ERROR;
   }

   if( 0 == data0 )
   {
      ratio = 0.0;
   }
   else
   {
      ratio = (float)data1 / (float)data0;
   }

   if( (0 < ratio) && (0.50 >= ratio) )
   {
      *lux = 0.0304*data0 - 0.062*data0*(pow(ratio, 1.4));
	}
	else if( (0.50 < ratio) && (0.61 >= ratio) )
	{
		*lux = 0.0224*data0 - 0.031*data1;
	}
	else if( (0.61 < ratio) && (0.80 >= ratio) )
	{
		*lux = 0.0128*data0 - 0.0153*data1;
	}
	else if( (0.80 < ratio) && (1.30 >= ratio) )
	{
		*lux = 0.00146*data0 - 0.00112*data1;
	}
	else if( 1.30 < ratio )
	{
		*lux = 0;
   }

   return EXIT_CLEAN;
}

/**
 * =================================================================================
 * function:       apds9301_read_data0
 * @brief   Read ADC register for channel 0
 *
 * @param   *data - pointer to location to write decoded value to
 * @return  EXIT_CLEAN if successful, otherwise exit_error
 * =================================================================================
 */
int apds9301_read_data0( uint16_t *data )
{
   uint8_t low  = 0;
   uint8_t high = 0;
   int retVal = i2c_read( APDS9301_ADDRESS, APDS9301_REG_DLOW_0, &low, 0 );

   if( EXIT_CLEAN != retVal )
   {
     return EXIT_ERROR;
   }

   retVal = i2c_read( APDS9301_ADDRESS, APDS9301_REG_DHIGH_0, &high, 0 );

   if( EXIT_CLEAN == retVal )
   {
      *data = ( low | (high << 8 ) );
   }
   else
   {
      return EXIT_ERROR;
   }
   return EXIT_CLEAN;
}
/**
 * =================================================================================
 * function:       apds9301_read_data1
 * @brief   Read ADC register for channel 1
 *
 * @param   *data - pointer to location to write decoded value to
 * @return  EXIT_CLEAN if successful, otherwise exit_error
 * =================================================================================
 */
int apds9301_read_data1( uint16_t *data )
{
   uint8_t low  = 0;
   uint8_t high = 0;
   int retVal = i2c_read( APDS9301_ADDRESS, APDS9301_REG_DLOW_1, &low, 0 );

   if( EXIT_CLEAN != retVal )
   {
     return EXIT_ERROR;
   }

   retVal = i2c_read( APDS9301_ADDRESS, APDS9301_REG_DHIGH_1, &high, 0 );

   if( EXIT_CLEAN == retVal )
   {
      *data = ( low | (high << 8 ) );
   }
   else
   {
      return EXIT_ERROR;
   }
   return EXIT_CLEAN;
}

/**
 * =================================================================================
 * Function:       apds9301_power
 * @brief   power on (or off) APDS9301 as set by paramater
 *
 * @param   on - specifies if sensor is to be powered on or off
 * @return  see i2c_write_byte()
 * =================================================================================
 */
int apds9301_power( uint16_t on )
{
   int retVal = 0;
   if( on )
   {
      /* power on */
      retVal = i2c_write_byte( APDS9301_ADDRESS, APDS9301_REG_CNTRL, POWER_ON );
   }
   else
   {
      /* power off */
      retVal = i2c_write_byte( APDS9301_ADDRESS, APDS9301_REG_CNTRL, POWER_OFF );
   }

   return retVal;
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
   if( EXIT_INIT == retVal )
   {
      sem_wait(&shm->w_sem);
      print_header(shm->header);
      sprintf( shm->buffer, "ERROR: Failed to initialize I2C for light sensor!\n" );
      sem_post(&shm->r_sem);
      thread_exit( EXIT_INIT );
   }
   retVal = apds9301_power( POWER_ON );
   if( retVal )
   {
      sem_wait(&shm->w_sem);
      print_header(shm->header);
      sprintf( shm->buffer, "ERROR: Failed to power on light sensor!\n" );
      sem_post(&shm->r_sem);
      thread_exit( EXIT_INIT );
   }

   timer_setup( &timerid, &timer_handler );

   timer_start( &timerid, 5000000 );
   cycle();

   thread_exit( 0 );
   return NULL;
}
