/*!
 * @file  apds9960_task.c
 * @brief 
 *
 *
 * @author   Roberto Baquerizo (baquerrj), roba8460@colorado.edu
 *
 * @internal
 *       Created:    04/13/2019
 *       Revision:   none
 *       Compiler:   gcc
 *  Organization:    University of Colorado: Boulder
 *
 *  This source code is released for free distribution under the terms of the
 *  GNU General Public License as published by the Free Software Foundation.
 */

#include "common.h"
#include "apds9960_task.h"
#include "watchdog.h"
#include "led.h"
#include "logger.h"

#include <errno.h>
#include <time.h>
#include <string.h>
#include <math.h>

static timer_t    timerid;
struct itimerspec trigger;

static i2c_handle_t i2c_apds9960;

static mqd_t apds9960_queue;

static message_t apds9960_log = {
   .level      = LOG_INFO,
   .timestamp  = {0},
   .id         = MSG_STATUS,
   .src        = TASK_APDS9960,
   .msg        = {0}
};

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

static void cycle( void )
{
   int retVal = 0;
   message_t request = {0};
   while( 1 )
   {
      memset( &request, 0, sizeof( request ) );
      retVal = mq_receive( apds9960_queue, (char*)&request, sizeof( request ), NULL );
      if( 0 > retVal )
      {
         int errnum = errno;
         LOG_ERROR( "APDS9960 TASK: QUEUE RECEIVE: (%s)\n",
                     strerror( errnum ) );
         continue;
      }
      switch( request.id )
      {
         case MSG_ALIVE:
            pthread_mutex_lock( &alive_mutex );
            threads_status[TASK_APDS9960]--;
            pthread_mutex_unlock( &alive_mutex );

            LOG_INFO( "APDS9960 TASK: I am alive!\n" );
            break;
         default:
            break;
      }
   }
   return;
}
   
static void apds9960_task( union sigval sig )
{
   uint8_t id = 0;
   int retVal = apds9960_read_id( &id );
   if( EXIT_CLEAN != retVal )
   {
      LOG_ERROR( "APDS9960 TASK: READ ID REG\n" );
   }
   if( ( APDS9960_ID_1 != id ) && ( APDS9960_ID_2 != id ) )
   {
      LOG_ERROR( "Invalid ID: %u\n", id );
   }
   else
   {
      LOG_INFO( "APDS9960 ID: %u\n", id );
   }

   uint8_t proximity_level = 0;
   if( EXIT_CLEAN != readProximity( &proximity_level ) )
   {
      LOG_ERROR( "READ PROXIMITY: %d\n", proximity_level );
   }
   else
   {
      LOG_INFO( "READ PROXIMITY: %d\n", proximity_level );
   }
   return;
}


uint8_t apds9960_i2c_init( void )
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
      LOG_INFO( "ID: %u\n", id );
   }

   if( EXIT_CLEAN != setMode( ALL, OFF ) )
   {
      return EXIT_ERROR;
   }


   if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_ATIME, DEFAULT_ATIME ) )
   {
      return EXIT_ERROR;
   }

   if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_WTIME, DEFAULT_WTIME) ) {
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_PPULSE, DEFAULT_PROX_PPULSE) ) {
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_POFFSET_UR, DEFAULT_POFFSET_UR) ) {
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_POFFSET_DL, DEFAULT_POFFSET_DL) ) {
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_CONFIG1, DEFAULT_CONFIG1) ) {
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != setLEDDrive(DEFAULT_LDRIVE) ) {
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != setProximityGain(DEFAULT_PGAIN) ) {
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != setAmbientLightGain(DEFAULT_AGAIN) ) {
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != setProximityIntLowThreshold(DEFAULT_PILT) ) {
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != setProximityIntHighThreshold(DEFAULT_PIHT) ) {
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != setLightIntLowThreshold(DEFAULT_AILT) ) {
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != setLightIntHighThreshold(DEFAULT_AIHT) ) {
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_PERS, DEFAULT_PERS) ) {
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_CONFIG2, DEFAULT_CONFIG2) ) {
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_CONFIG3, DEFAULT_CONFIG3) ) {
        return EXIT_ERROR;
    }

    /* Set default values for gesture sense registers */
//    if( EXIT_CLEAN != setGestureEnterThresh(DEFAULT_GPENTH) ) {
//        return EXIT_ERROR;
//    }
//    if( EXIT_CLEAN != setGestureExitThresh(DEFAULT_GEXTH) ) {
//        return EXIT_ERROR;
//    }
//    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_GCONF1, DEFAULT_GCONF1) ) {
//        return EXIT_ERROR;
//    }
//    if( EXIT_CLEAN != setGestureGain(DEFAULT_GGAIN) ) {
//        return EXIT_ERROR;
//    }
//    if( EXIT_CLEAN != setGestureLEDDrive(DEFAULT_GLDRIVE) ) {
//        return EXIT_ERROR;
//    }
//    if( EXIT_CLEAN != setGestureWaitTime(DEFAULT_GWTIME) ) {
//        return EXIT_ERROR;
//    }
//    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_GOFFSET_U, DEFAULT_GOFFSET) ) {
//        return EXIT_ERROR;
//    }
//    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_GOFFSET_D, DEFAULT_GOFFSET) ) {
//        return EXIT_ERROR;
//    }
//    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_GOFFSET_L, DEFAULT_GOFFSET) ) {
//        return EXIT_ERROR;
//    }
//    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_GOFFSET_R, DEFAULT_GOFFSET) ) {
//        return EXIT_ERROR;
//    }
//    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_GPULSE, DEFAULT_GPULSE) ) {
//        return EXIT_ERROR;
//    }
//    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_GCONF3, DEFAULT_GCONF3) ) {
//        return EXIT_ERROR;
//    }
    if( EXIT_CLEAN != setGestureIntEnable(DEFAULT_GIEN) ) {
        return EXIT_ERROR;
    }


    if( EXIT_CLEAN != setProximityGain(PGAIN_2X) )
    {
       LOG_ERROR( "SET PROXIMITY GAIN %d\n", PGAIN_2X );
       return EXIT_ERROR;
    }

   if( EXIT_CLEAN != enableProximitySensor( 0 ) )
   {
      return EXIT_ERROR;
   }

   uint8_t proximity_gain = getProximityGain();
   if( DEFAULT_PGAIN != proximity_gain )
   {
      LOG_ERROR( "PROXIMITY GAIN: [%d] != [%d]\n", DEFAULT_PGAIN, proximity_gain );
      return EXIT_ERROR;
   }
   else
   {
      LOG_INFO( "EXPECTED %d, ACTUAL %d\n", PGAIN_2X, proximity_gain );
   }

   return retVal;
}

mqd_t get_apds9960_queue( void )
{
   return apds9960_queue;
}

uint8_t apds9960_queue_init( void )
{
   mq_unlink( APDS9960_QUEUE );
   struct mq_attr attr;
   attr.mq_flags = 0;
   attr.mq_maxmsg = MAX_MESSAGES;
   attr.mq_msgsize = sizeof( message_t );
   attr.mq_curmsgs = 0;

   int msg_q = mq_open( APDS9960_QUEUE, O_CREAT | O_RDWR, 0666, &attr );
   if( 0 > msg_q )
   {
      int errnum = errno;
      LOG_ERROR( "APDS9960 TASK: QUEUE CREATE: (%s)\n", strerror( errnum ) );
   }
   return msg_q;
}



void *apds9960_fn( void *thread_args )
{
   signal(SIGUSR1, sig_handler);
   signal(SIGUSR2, sig_handler);

   apds9960_queue = apds9960_queue_init();
   if( 0 > apds9960_queue )
   {
      thread_exit( EXIT_INIT );
   }

   int retVal = apds9960_i2c_init();
   if( EXIT_CLEAN != retVal )
   {
      LOG_ERROR( "APDS9960 TASK: I2C INIT\n" );
      thread_exit( EXIT_INIT );
   }
//   retVal = apds9960_power( POWER_ON );
//   if( EXIT_CLEAN != retVal )
//   {
//      LOG_ERROR( "ERROR: Failed to power on light sensor!\n" );
//      thread_exit( EXIT_INIT );
//   }

   timer_setup( &timerid, &apds9960_task );

   timer_start( &timerid, FREQ_1HZ );

   LOG_INFO( "APDS9960 TASK INTIALIZED\n" );

   cycle();

   thread_exit( 0 );
   return NULL;
}
