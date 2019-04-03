/*
 * =================================================================================
 *    @file     i2c.h
 *    @brief   Interface to I2C Bus of BeagleBone Green using libmraa
 *             https://iotdk.intel.com/docs/master/mraa/
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


#ifndef  I2C_H
#define  I2C_H

#include <pthread.h>
#include "mraa/i2c.h"


typedef struct {
   mraa_i2c_context context;
   pthread_mutex_t  mutex;
} i2c_handle_t;

/*
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
int i2c_write_byte( int slave, int reg, uint8_t data );


/*
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
int i2c_write( int slave, int reg, uint16_t data );

/*
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
int i2c_read( int slave, int reg, uint8_t *data, size_t len );

/*
 * =================================================================================
 * Function:       i2c_init
 * @brief   Initialize singleton master i2c context
 *
 * @param   *i2c - pointer to handle to be master
 * @return  EXIT_CLEAN on success, otherwise EXIT_INIT
 * =================================================================================
 */
int i2c_init( i2c_handle_t *i2c );

/*
 * =================================================================================
 * Function:       i2c_stop
 * @brief   Stops i2c instance
 *
 * @param   *i2c - pointer to i2c context handle
 * @return  EXIT_CLEAN on success, otherwise EXIT_ERROR
 * =================================================================================
 */
int i2c_stop( i2c_handle_t *i2c );

#endif   /* I2C_H */
