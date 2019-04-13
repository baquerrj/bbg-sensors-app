/*!
 * @file  apds9301_sensor.c
 * @brief
 *
 * <+DETAILED+>
 *
 * @author   Roberto Baquerizo (baquerrj), roba8460@colorado.edu
 *
 * @internal
 *       Created:    04/12/2019
 *       Revision:   none
 *       Compiler:   gcc
 *  Organization:    University of Colorado: Boulder
 *
 *  This source code is released for free distribution under the terms of the
 *  GNU General Public License as published by the Free Software Foundation.
 */

#include "apds9301_sensor.h"

#include <math.h>


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


int apds9301_clear_interrupt( void )
{
   uint8_t clear = APDS9301_REG_CMD | CMD_CLEAR_INTR;

   int retVal = i2c_set( APDS9301_ADDRESS, clear );

   return retVal;
}


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

int apds9301_read_control( uint8_t* data )
{
   int retVal = i2c_read( APDS9301_ADDRESS, APDS9301_REG_CNTRL, data, sizeof( *data ) );
   return retVal;
}

int apds9301_write_threshold_low( uint16_t threshold )
{
   int retVal = i2c_write( APDS9301_ADDRESS, APDS9301_REG_TH_LL, threshold );
   return retVal;
}

int apds9301_read_threshold_low( uint16_t *threshold )
{
   int retVal = i2c_read( APDS9301_ADDRESS, APDS9301_REG_TH_LL, (uint8_t*)threshold, sizeof( *threshold ) );
   return retVal;
}

int apds9301_write_threshold_high( uint16_t threshold )
{
   int retVal = i2c_write( APDS9301_ADDRESS, APDS9301_REG_TH_HL, threshold );
   return retVal;
}

int apds9301_read_threshold_high( uint16_t *threshold )
{
   int retVal = i2c_read( APDS9301_ADDRESS, APDS9301_REG_TH_HL, (uint8_t*)threshold, sizeof( *threshold ) );
   return retVal;
}

int apds9301_read_id( uint8_t *id )
{
   int retVal = i2c_read( APDS9301_ADDRESS, APDS9301_REG_ID, id, sizeof( *id ) );
   return retVal;
}

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


int apds9301_power( uint16_t on )
{
   /* power on */
   int retVal = i2c_write_byte( APDS9301_ADDRESS, APDS9301_REG_CNTRL, on );

   return retVal;
}

