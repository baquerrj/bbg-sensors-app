/**
 * =================================================================================
 *    @file     i2c.c
 *    @brief
 *
 *
 *    @author   Roberto Baquerizo (baquerrj), roba8460@colorado.edu
 *
 *    @internal
 *       Created:  03/17/2019
 *      Revision:  none
 *      Compiler:  gcc
 *  Organization:  University of Colorado: Boulder
 *
 *  This source code is released for free distribution under the terms of the
 *  GNU General Public License as published by the Free Software Foundation.
 * =================================================================================
 */



#include "i2c.h"
#include "common.h"

#include <errno.h>
#include <string.h>

/** Keep around a singleton instance of the master handle */
static i2c_handle_t *my_i2c = NULL;

/*
 * =================================================================================
 * Function:       i2c_set
 * @brief   Write 1 to bit at addr
 *
 * @param   slave  - address of i2c slave
 * @param   addr - memory location to write to
 * @return  EXIT_CLEAN on success, otherweise one of exit_e
 * =================================================================================
 */
int i2c_set( int slave, int addr )
{
   if( NULL == my_i2c )
   {
      fprintf( stderr, "I2C master has not been initialized!\n" );
      return EXIT_INIT;
   }

   /* take hardware mutex */
   pthread_mutex_lock( &my_i2c->mutex );

   mraa_result_t retVal = mraa_i2c_address( my_i2c->context, slave );
   if( 0 != retVal )
   {
      mraa_result_print( retVal );
      pthread_mutex_unlock( &my_i2c->mutex );
      return EXIT_ERROR;
   }

   retVal = mraa_i2c_write_byte( my_i2c->context, addr );
   pthread_mutex_unlock( &my_i2c->mutex );

   return EXIT_CLEAN;
}

/**
 * =================================================================================
 * Function:       i2c_write_byte
 * @brief   Writes byte to register address
 *
 * @param   slave  - address of i2c slave
 * @param   reg    - address of register to write to
 * @param   data   - data to write
 * @return  EXIT_CLEAN on success, otherwise one of exit_e
 * =================================================================================
 */
int i2c_write_byte( int slave, int reg, uint8_t data )
{
   if( NULL == my_i2c )
   {
      fprintf( stderr, "I2C master has not been initialized!\n" );
      return EXIT_INIT;
   }

   /* take hardware mutex */
   pthread_mutex_lock( &my_i2c->mutex );

   mraa_result_t retVal = mraa_i2c_address( my_i2c->context, slave );
   if( 0 != retVal )
   {
      mraa_result_print( retVal );
      pthread_mutex_unlock( &my_i2c->mutex );
      return EXIT_ERROR;
   }

   retVal = mraa_i2c_write_byte_data( my_i2c->context, data, reg );
   pthread_mutex_unlock( &my_i2c->mutex );

   return EXIT_CLEAN;
}

/**
 * =================================================================================
 * Function:       i2c_write
 * @brief   Writes data to register address
 *
 * @param   slave  - address of i2c slave
 * @param   reg    - address of register to write to
 * @param   data   - data to write
 * @return  EXIT_CLEAN on success, otherwise one of exit_e
 * =================================================================================
 */
int i2c_write( int slave, int reg, uint16_t data )
{
   if( NULL == my_i2c )
   {
      fprintf( stderr, "I2C master has not been initialized!\n" );
      return EXIT_INIT;
   }

   /* take hardware mutex */
   pthread_mutex_lock( &my_i2c->mutex );

   mraa_result_t retVal = mraa_i2c_address( my_i2c->context, slave );
   if( 0 != retVal )
   {
      mraa_result_print( retVal );
      pthread_mutex_unlock( &my_i2c->mutex );
      return EXIT_ERROR;
   }

   retVal = mraa_i2c_write_word_data( my_i2c->context, data, reg );
   pthread_mutex_unlock( &my_i2c->mutex );

   return EXIT_CLEAN;
}

/**
 * =================================================================================
 * Function:       i2c_read
 * @brief   Reads data from register address
 *
 * @param   slave - address of i2c slave
 * @param   reg   - address to read from
 * @param   *data - pointer to location to store read data
 * @param   len   - size of memory to read in bytes
 * @return  EXIT_CLEAN on success, otherwise one of exit_e
 * =================================================================================
 */
int i2c_read( int slave, int reg, uint8_t *data, size_t len )
{
   if( NULL == my_i2c )
   {
      fprintf( stderr, "I2C master has not been initialized!\n" );
      return EXIT_INIT;
   }

   pthread_mutex_lock( &my_i2c->mutex );

   mraa_result_t retVal = mraa_i2c_address( my_i2c->context, slave );
   if( 0 != retVal )
   {
      mraa_result_print( retVal );
      pthread_mutex_unlock( &my_i2c->mutex );
      return EXIT_ERROR;
   }

   if( len )
   {
      retVal = mraa_i2c_read_bytes_data( my_i2c->context, reg, data, len );
      pthread_mutex_unlock( &my_i2c->mutex );
      if( len != retVal )
      {
         fprintf( stderr, "Could not read all data from register!\n" );
         return EXIT_ERROR;
      }
   }
   else
   {
      /* only read one byte */
      retVal = mraa_i2c_read_byte_data( my_i2c->context, reg );
      pthread_mutex_unlock( &my_i2c->mutex );
      if( -1 != retVal )
      {
         *data  = retVal;
      }
   }

   return EXIT_CLEAN;
}


/**
 * =================================================================================
 * Function:       i2c_init
 * @brief   Initialize singleton master i2c context
 *
 * @param   *i2c - pointer to handle to be master
 * @return  EXIT_CLEAN on success, otherwise EXIT_INIT
 * =================================================================================
 */
int i2c_init( i2c_handle_t *i2c )
{
   if( NULL != my_i2c )
   {
      i2c = my_i2c;
      return EXIT_CLEAN;
   }

   if( NULL != i2c )
   {
      i2c->context = mraa_i2c_init_raw( 2 );

      if( NULL == i2c->context )
      {
         int errnum = errno;
         fprintf( stderr, "Failed to initialize I2C master instance: (%s)\n",
                  strerror( errnum ) );
         my_i2c = NULL;
         return EXIT_INIT;
      }

      int retVal = pthread_mutex_init( &i2c->mutex, NULL );
      if( 0 > retVal )
      {
         int errnum = errno;
         fprintf( stderr, "Failed to initialize mutex for I2C master instance: (%s)\n",
                  strerror( errnum ) );
         my_i2c = NULL;
         retVal = mraa_i2c_stop( i2c->context );
         if( 0 > retVal )
         {
            mraa_result_print( retVal );
         }
         return EXIT_INIT;
      }
      my_i2c = i2c;
   }
   return EXIT_CLEAN;
}


/**
 * =================================================================================
 * Function:       i2c_stop
 * @brief   Stops i2c instance
 *
 * @param   *i2c - pointer to i2c context handle
 * @return  EXIT_CLEAN on success, otherwise EXIT_ERROR
 * =================================================================================
 */
int i2c_stop( i2c_handle_t *i2c )
{
   if( NULL == my_i2c )
   {
      return EXIT_CLEAN;
   }
   else if( NULL == i2c )
   {
      return EXIT_CLEAN;
   }

   if( my_i2c != i2c )
   {
      return EXIT_ERROR;
   }

   while( EBUSY  == pthread_mutex_destroy( &i2c->mutex ) );

   mraa_result_t retVal = mraa_i2c_stop( i2c->context );
   if( 0 > retVal )
   {
      mraa_result_print( retVal );
      return EXIT_ERROR;
   }

   my_i2c = NULL;
   return EXIT_CLEAN;
}

