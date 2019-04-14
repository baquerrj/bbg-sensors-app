/*!
 *    @file     apds9960_sensor.c
 *    @brief    
 *
 *  <+DETAILED+>
 *
 *    @author   Roberto Baquerizo (baquerrj), roba8460@colorado.edu
 *
 *    @internal
 *       Created:  04/11/2019
 *      Revision:  none
 *      Compiler:  gcc
 *  Organization:  University of Colorado: Boulder
 *
 *  This source code is released for free distribution under the terms of the
 *  GNU General Public License as published by the Free Software Foundation.
 */

/*!
 * @file    SparkFun_APDS-9960.cpp
 * @brief   Library for the SparkFun APDS-9960 breakout board
 * @author  Shawn Hymel (SparkFun Electronics)
 *
 * @copyright	This code is public domain but you buy me a beer if you use
 * this and we meet someday (Beerware license).
 *
 * This library interfaces the Avago APDS-9960 to Arduino over I2C. The library
 * relies on the Arduino Wire (I2C) library. to use the library, instantiate an
 * APDS9960 object, call init(), and call the appropriate functions.
 *
 * APDS-9960 current draw tests (default parameters):
 *   Off:                   1mA
 *   Waiting for gesture:   14mA
 *   Gesture in progress:   35mA
 */

#include "apds9960_sensor.h"
#include "i2c.h"
#include "led.h"

#include <errno.h>
#include <time.h>
#include <string.h>

static timer_t    timerid;
struct itimerspec trigger;

static i2c_handle_t i2c_apds9960;

static mqd_t apds9960_queue;


/*!
 * Function:       sig_handler
 * @brief   Signal handler for light sensor thread.
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
      mq_close( apds9960_queue );
      timer_delete( timerid );
//      apds9960_power( POWER_OFF );
      i2c_stop( &i2c_apds9960 );
      thread_exit( signo );
   }
   else if( signo == SIGUSR2 )
   {
      printf("Received SIGUSR2! Exiting...\n");
      mq_close( apds9960_queue );
      timer_delete( timerid );
//      apds9960_power( POWER_OFF );
      i2c_stop( &i2c_apds9960 );
      thread_exit( signo );
   }
   return;
}


uint8_t apds9960_init( void )
{
   uint8_t retVal = i2c_init( &i2c_apds9960 );
   if( 0 != retVal )
   {
      mraa_result_print( retVal );
      LOG_ERROR( "Could not initialize I2C Master instance\n" );
      return retVal;
   }
   uint8_t id;
   
   retVal = apds9960_read_id( &id );
   if( ( APDS9960_ID_1 != id ) && ( APDS9960_ID_2 != id ) )
   {
      LOG_ERROR( "Invalid ID: %u\n", id );
      return 1;
   }
   else
   {
      LOG_ERROR( "ID: %u\n", id );
   }
   return retVal;
}


uint8_t apds9960_read_enable_reg( uint16_t *data );


uint8_t apds9960_write_enable_reg( uint16_t *data );


uint8_t apds9960_read_id( uint8_t *id )
{
   uint8_t retVal = i2c_read( APDS9960_I2C_ADDR, APDS9960_REG_ID, id, 0 );
   return retVal;
}


uint8_t apds9960_queue_init( void )
{
   mq_unlink( APDS9960_QUEUE_NAME );
   struct mq_attr attr;
   attr.mq_flags = 0;
   attr.mq_maxmsg = MAX_MESSAGES;
   attr.mq_msgsize = sizeof( message_t );
   attr.mq_curmsgs = 0;

   int msg_q = mq_open( APDS9960_QUEUE_NAME, O_CREAT | O_RDWR, 0666, &attr );
   if( 0 > msg_q )
   {
      int errnum = errno;
      LOG_ERROR( "Encountered error creating message queue %s: (%s)\n",
               APDS9960_QUEUE_NAME, strerror( errnum ) );
   }
   return msg_q;
}


void *apds9960_fn( void *thread_args )
{
   LOG_INFO( "APDS9960: Hello World!\n" );

   signal(SIGUSR1, sig_handler);
   signal(SIGUSR2, sig_handler);

   apds9960_queue = apds9960_queue_init();
   if( 0 > apds9960_queue )
   {
      thread_exit( EXIT_INIT );
   }

   int retVal = apds9960_init();
   if( EXIT_CLEAN != retVal )
   {
      LOG_ERROR( "Failed to initialize I2C for light sensor!\n" );
      thread_exit( EXIT_INIT );
   }
//   retVal = apds9960_power( POWER_ON );
//   if( EXIT_CLEAN != retVal )
//   {
//      LOG_ERROR( "ERROR: Failed to power on light sensor!\n" );
//      thread_exit( EXIT_INIT );
//   }

//   timer_setup( &timerid, &timer_handler );

//   timer_start( &timerid, FREQ_1HZ );
//   cycle();

   thread_exit( 0 );
   return NULL;
}
