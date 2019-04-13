/*!
 * @file  tmp102_sensor.c
 * @brief 
 *
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

#include "tmp102_sensor.h"
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



int tmp102_write_config( tmp102_config_t *config_reg )
{
   int retVal = i2c_write( TMP102_SLAVE, TMP102_REG_CONFIG, *((uint16_t*)&config_reg) );

   return retVal;
}

int tmp102_get_temp( float *temperature )
{
   uint8_t buffer[2] = {0};
   int retVal = i2c_read( TMP102_SLAVE, TMP102_REG_TEMP, buffer, sizeof( buffer ) );
   if( EXIT_CLEAN != retVal )
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
      return EXIT_ERROR;
   }

   return EXIT_CLEAN;
}

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
      return EXIT_ERROR;
   }

   return EXIT_CLEAN;
}

int tmp102_read_thigh( float *thigh )
{
   uint16_t tmp = 0;

   int retVal = i2c_read( TMP102_SLAVE, TMP102_THIGH, (uint8_t*)&tmp, sizeof( tmp ) );
   if( 0 > retVal )
   {
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


int tmp102_read_tlow( float *tlow )
{
   uint16_t tmp = 0;

   int retVal = i2c_read( TMP102_SLAVE, TMP102_TLOW, (uint8_t*)&tmp, sizeof( tmp ) );
   if( 0 > retVal )
   {
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


