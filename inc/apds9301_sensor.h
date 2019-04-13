/*!
 * @file  apds9301_sensor.h
 * @brief Interface to APDS9301 Light Sensor
 *
 *  <+DETAILED+>
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


#ifndef  APDS9301_SENSOR_H
#define  APDS9301_SENSOR_H

#include "common.h"
#include "i2c.h"

#define APDS9301_ADDRESS         (0x39)

/** Register adddresses */
#define APDS9301_REG_CMD         (0x80)
#define APDS9301_REG_CNTRL       (0x80)
#define APDS9301_REG_TIME        (0x81)
#define APDS9301_REG_TH_LL       (0x82)
#define APDS9301_REG_TH_LH       (0x83)
#define APDS9301_REG_TH_HL       (0x84)
#define APDS9301_REG_TH_HH       (0x85)
#define APDS9301_REG_INT_CNTRL   (0x86)
#define APDS9301_REG_ID          (0x8A)
#define APDS9301_REG_DLOW_0      (0x8C)
#define APDS9301_REG_DHIGH_0     (0x8D)
#define APDS9301_REG_DLOW_1      (0x8E)
#define APDS9301_REG_DHIGH_1     (0x8F)

/** Helpful constants */
#define POWER_ON     (0x03)
#define POWER_OFF    (0x00)
#define CMD_CLEAR_INTR  (1<<5)
#define CMD_WORD_ENBL   (1<<6)

/** Defaults */
#define DEFAULT_GAIN             (0x00)   /** low gain */
#define DEFAULT_INTEGRATION_TIME (0x02)   /** 402ms integration time */
#define DEFAULT_INTERRUPT        (0x00)   /** No interrupts */

#define DARK_THRESHOLD           (50)


/**
 * Function:       apds9301_set_config
 * @brief   Set configuration of light sensor. For the APDS9301, the configuration
 *          is spread out across the: Timing Register, Interrupt Control Register,
 *          and Control Register. So, I have to write to all of these to set the config
 *
 * @param   void
 * @return  EXIT_CLEAN if successful, otherwise see i2c_write()
 */
int apds9301_set_config( void );


/**
 * Function:       apds9301_set_integration
 * @brief   Sets the integration time for APDS9301 by writing a value to bits
 *          INTEG of the Timing Register
 *
 * @param   val   - value to write to timing register
 * @return  see i2c_write_byte() - if val is not an allowed value, EXIT_ERROR
 */
int apds9301_set_integration( uint8_t val );


/**
 * Function:       apds9301_clear_interrupt
 * @brief   Clears any pending interrupt for APDS9301 by writing a 1 to the CLEAR bit
 *          of the Command Register
 *
 * @param   void
 * @return  see i2c_set()
 */
int apds9301_clear_interrupt( void );

/**
 * Function:       apds9301_set_interrupt
 * @brief   Enables or disables interrupts for APDS9301 by setting or clearing the
 *          INTR bits of the Interrupt Control Register
 *
 * @param   enable - set if we want to enable interrupts
 * @return  see i2c_write_byte()
 */
int apds9301_set_interrupt( uint8_t enable );

/**
 * Function:       apds9301_set_gain
 * @brief   Sets gain for APDS9301 by setting or clearing the GAIN bit of the
 *          Timing Register
 *
 * @param   gain  - set if we want high gain
 * @return  see i2c_write_byte()
 */
int apds9301_set_gain( uint8_t gain );


/**
 * Function:       apds9301_read_control
 * @brief   Read contents of Control Register
 *
 * @param   *data - where to store contents
 * @return  see i2c_read()
 */
int apds9301_read_control( uint8_t* data );


/**
 * Function:       apds9301_write_threshold_low
 * @brief   Write value to low threshold register
 *
 * @param   threshold   - value to write
 * @return  see i2c_write()
 */
int apds9301_write_threshold_low( uint16_t threshold );

/**
 * Function:       apds9301_write_threshold_low
 * @brief   Read value from low threshold register
 *
 * @param   *threshold   - where to write value read
 * @return  see i2c_write()
 */
int apds9301_read_threshold_low( uint16_t *threshold );

/**
 * Function:       apds9301_write_threshold_high
 * @brief   Write value to high threshold register
 *
 * @param   threshold   - value to write
 * @return  see i2c_write()
 */
int apds9301_write_threshold_high( uint16_t threshold );

/**
 * Function:       apds9301_write_threshold_high
 * @brief   Read value from high threshold register
 *
 * @param   *threshold   - where to write value read
 * @return  see i2c_write()
 */
int apds9301_read_threshold_high( uint16_t *threshold );


/**
 * Function:       apds9301_read_id
 * @brief   Read APDS9301 Identification Register
 *
 * @param   *id   - where to write ID from register
 * @return  EXIT_CLEAN if successful, EXIT_ERROR otherwise
 */
int apds9301_read_id( uint8_t *id );


/**
 * Function:       apds9301_get_lux
 * @brief   Read ADC Registers and calculate lux in lumen
 *
 * @param   *lux  - pointer to location to write decoded lux to
 * @return  EXIT_CLEAN if successful, otherwise EXIT_ERROR
 */
int apds9301_get_lux( float *lux );


/**
 * function:       apds9301_read_data0
 * @brief   Read ADC register for channel 0
 *
 * @param   *data - pointer to location to write decoded value to
 * @return  EXIT_CLEAN if successful, otherwise exit_error
 */
int apds9301_read_data0( uint16_t *data );

/**
 * function:       apds9301_read_data1
 * @brief   Read ADC register for channel 1
 *
 * @param   *data - pointer to location to write decoded value to
 * @return  EXIT_CLEAN if successful, otherwise exit_error
 */
int apds9301_read_data1( uint16_t *data );

/**
 * Function:       apds9301_power
 * @brief   power on (or off) APDS9301 as set by paramater
 *
 * @param   on - specifies if sensor is to be powered on or off
 * @return  see i2c_write_byte()
 */
int apds9301_power( uint16_t on );


#endif   /* APDS9301_SENSOR_H */
