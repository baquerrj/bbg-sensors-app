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

#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>

static gesture_data_type gesture_data_;

static int gesture_ud_delta_ = 0;
static int gesture_lr_delta_ = 0;
static int gesture_ud_count_ = 0;
static int gesture_lr_count_ = 0;
static int gesture_near_count_ = 0;
static int gesture_far_count_ = 0;
static int gesture_state_ = 0;
static int gesture_motion_ = DIR_NONE;

uint8_t getMode( void )
{
   uint8_t data;
   i2c_read( APDS9960_I2C_ADDR, APDS9960_ENABLE, &data, 0 );
   return data;
}


uint8_t setMode( uint8_t mode, uint8_t enable )
{
   uint8_t data;
   data = getMode();
   if( 0 > data )
   {
      return EXIT_ERROR;
   }

   enable = enable & 0x01;
   if( (mode >= 0) && (mode <= 6) )
   {
      if( enable )
      {
         data |= (1 << mode);
      }
      else
      {
         data &= ~(1 << mode);
      }
   }
   else if( mode == ALL )
   {
      if( enable )
      {
         data = 0x7F;
      }
      else
      {
         data = 0x00;
      }
   }

   uint8_t retVal = i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_ENABLE, data );

   return retVal;
}

uint8_t enablePower( void )
{
   uint8_t retVal = setMode( POWER, 1 );
   return retVal;
}

uint8_t disablePower( void )
{
   uint8_t retVal = setMode( POWER, 0 );
   return retVal;
}

/**
 * @brief Reads the proximity level as an 8-bit value
 *
 * @param[out] val value of the proximity sensor.
 * @return EXIT_CLEAN if operation successful. EXIT_ERROR otherwise.
 */
uint8_t readProximity(uint8_t *val)
{ 
    /* Read value from proximity data register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_PDATA, val, 0) ) {
        return EXIT_ERROR;
    }
    
    return EXIT_CLEAN;
}


/* @brief Enable or disable specific sensors */
uint8_t enableLightSensor( uint8_t interrupts )
{
    /* Set default gain, interrupts, enable power, and enable sensor */
    if( EXIT_CLEAN != setAmbientLightGain(DEFAULT_AGAIN) ) {
        return EXIT_ERROR;
    }
    if( interrupts ) {
        if( EXIT_CLEAN != setAmbientLightIntEnable(1) ) {
            return EXIT_ERROR;
        }
    } else {
        if( EXIT_CLEAN != setAmbientLightIntEnable(0) ) {
            return EXIT_ERROR;
        }
    }
    if( EXIT_CLEAN != enablePower() ){
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != setMode(AMBIENT_LIGHT, 1) ) {
        return EXIT_ERROR;
    }
    
    return EXIT_CLEAN;
}

/**
 * @brief Ends the light sensor on the APDS-9960
 *
 * @return EXIT_CLEAN if sensor disabled correctly. EXIT_ERROR on error.
 */
uint8_t disableLightSensor( void )
{
    if( EXIT_CLEAN != setAmbientLightIntEnable(0) ) {
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != setMode(AMBIENT_LIGHT, 0) ) {
        return EXIT_ERROR;
    }
    
    return EXIT_CLEAN;
}

/**
 * @brief Starts the proximity sensor on the APDS-9960
 *
 * @param[in] interrupts EXIT_CLEAN to enable hardware external interrupt on proximity
 * @return EXIT_CLEAN if sensor enabled correctly. EXIT_ERROR on error.
 */
uint8_t enableProximitySensor(uint8_t interrupts)
{
    /* Set default gain, LED, interrupts, enable power, and enable sensor */
    if( EXIT_CLEAN != setProximityGain(DEFAULT_PGAIN) ) {
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != setLEDDrive(DEFAULT_LDRIVE) ) {
        return EXIT_ERROR;
    }
    if( interrupts ) {
        if( EXIT_CLEAN != setProximityIntEnable(1) ) {
            return EXIT_ERROR;
        }
    } else {
        if( EXIT_CLEAN != setProximityIntEnable(0) ) {
            return EXIT_ERROR;
        }
    }
    if( EXIT_CLEAN != enablePower() ){
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != setMode(PROXIMITY, 1) ) {
        return EXIT_ERROR;
    }
    
    return EXIT_CLEAN;
}

/**
 * @brief Ends the proximity sensor on the APDS-9960
 *
 * @return EXIT_CLEAN if sensor disabled correctly. EXIT_ERROR on error.
 */
uint8_t disableProximitySensor( void )
{
	if( EXIT_CLEAN != setProximityIntEnable(0) ) {
		return EXIT_ERROR;
	}
	if( EXIT_CLEAN != setMode(PROXIMITY, 0) ) {
		return EXIT_ERROR;
	}

	return EXIT_CLEAN;
}

/**
 * @brief Starts the gesture recognition engine on the APDS-9960
 *
 * @param[in] interrupts EXIT_CLEAN to enable hardware external interrupt on gesture
 * @return EXIT_CLEAN if engine enabled correctly. EXIT_ERROR on error.
 */
uint8_t enableGestureSensor(uint8_t interrupts)
{
    
    /* Enable gesture mode
       Set ENABLE to 0 (power off)
       Set WTIME to 0xFF
       Set AUX to LED_BOOST_300
       Enable PON, WEN, PEN, GEN in ENABLE 
    */
    resetGestureParameters();
    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_WTIME, 0xFF) ) {
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_PPULSE, DEFAULT_GESTURE_PPULSE) ) {
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != setLEDBoost(LED_BOOST_300) ) {
        return EXIT_ERROR;
    }
    if( interrupts ) {
        if( EXIT_CLEAN != setGestureIntEnable(1) ) {
            return EXIT_ERROR;
        }
    } else {
        if( EXIT_CLEAN != setGestureIntEnable(0) ) {
            return EXIT_ERROR;
        }
    }
    if( EXIT_CLEAN != setGestureMode(1) ) {
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != enablePower() ){
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != setMode(WAIT, 1) ) {
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != setMode(PROXIMITY, 1) ) {
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != setMode(GESTURE, 1) ) {
        return EXIT_ERROR;
    }
    
    return EXIT_CLEAN;
}

/**
 * @brief Ends the gesture recognition engine on the APDS-9960
 *
 * @return EXIT_CLEAN if engine disabled correctly. EXIT_ERROR on error.
 */
uint8_t disableGestureSensor( void )
{
    resetGestureParameters();
    if( EXIT_CLEAN != setGestureIntEnable(0) ) {
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != setGestureMode(0) ) {
        return EXIT_ERROR;
    }
    if( EXIT_CLEAN != setMode(GESTURE, 0) ) {
        return EXIT_ERROR;
    }
    
    return EXIT_CLEAN;
}


/*******************************************************************************
 * Gain control
 ******************************************************************************/

/**
 * @brief Returns receiver gain for the ambient light sensor (ALS)
 *
 * Value    Gain
 *   0        1x
 *   1        4x
 *   2       16x
 *   3       64x
 *
 * @return the value of the ALS gain. 0xFF on failure.
 */
uint8_t getAmbientLightGain()
{
    uint8_t val;
    
    /* Read value from CONTROL register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_CONTROL, &val, 0) ) {
        return EXIT_ERROR;
    }
    
    /* Shift and mask out ADRIVE bits */
    val &= 0b00000011;
    
    return val;
}

/**
 * @brief Sets the receiver gain for the ambient light sensor (ALS)
 *
 * Value    Gain
 *   0        1x
 *   1        4x
 *   2       16x
 *   3       64x
 *
 * @param[in] drive the value (0-3) for the gain
 * @return EXIT_CLEAN if operation successful. EXIT_ERROR otherwise.
 */
uint8_t setAmbientLightGain(uint8_t drive)
{
    uint8_t val;
    
    /* Read value from CONTROL register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_CONTROL, &val, 0) ) {
        return EXIT_ERROR;
    }
    
    /* Set bits in register to given value */
    drive &= 0b00000011;
    val &= 0b11111100;
    val |= drive;
    
    /* Write register value back into CONTROL register */
    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_CONTROL, val) ) {
        return EXIT_ERROR;
    }
    
    return EXIT_CLEAN;
}


/**
 * @brief Returns receiver gain for proximity detection
 *
 * Value    Gain
 *   0       1x
 *   1       2x
 *   2       4x
 *   3       8x
 *
 * @return the value of the proximity gain. 0xFF on failure.
 */
uint8_t getProximityGain( void )
{
    uint8_t val;
    
    /* Read value from CONTROL register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_CONTROL, &val, 0) ) {
        return EXIT_ERROR;
    }
    
    /* Shift and mask out PDRIVE bits */
    val = (val >> 2) & 0b00000011;
    
    return val;
}

/**
 * @brief Sets the receiver gain for proximity detection
 *
 * Value    Gain
 *   0       1x
 *   1       2x
 *   2       4x
 *   3       8x
 *
 * @param[in] drive the value (0-3) for the gain
 * @return EXIT_CLEAN if operation successful. EXIT_ERROR otherwise.
 */
uint8_t setProximityGain(uint8_t drive)
{
    uint8_t val;
    
    /* Read value from CONTROL register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_CONTROL, &val, 0) ) {
        return EXIT_ERROR;
    }
    
    /* Set bits in register to given value */
    drive &= 0b00000011;
    drive = drive << 2;
    val &= 0b11110011;
    val |= drive;
    
    /* Write register value back into CONTROL register */
    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_CONTROL, val) ) {
        return EXIT_ERROR;
    }
    
    return EXIT_CLEAN;
}

/**
 * @brief Gets the gain of the photodiode during gesture mode
 *
 * Value    Gain
 *   0       1x
 *   1       2x
 *   2       4x
 *   3       8x
 *
 * @return the current photodiode gain. 0xFF on error.
 */
uint8_t getGestureGain()
{
    uint8_t val;
    
    /* Read value from GCONF2 register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_GCONF2, &val, 0) ) {
        return EXIT_ERROR;
    }
    
    /* Shift and mask out GGAIN bits */
    val = (val >> 5) & 0b00000011;
    
    return val;
}

/**
 * @brief Sets the gain of the photodiode during gesture mode
 *
 * Value    Gain
 *   0       1x
 *   1       2x
 *   2       4x
 *   3       8x
 *
 * @param[in] gain the value for the photodiode gain
 * @return EXIT_CLEAN if operation successful. EXIT_ERROR otherwise.
 */
uint8_t setGestureGain(uint8_t gain)
{
    uint8_t val;
    
    /* Read value from GCONF2 register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_GCONF2, &val, 0) ) {
        return EXIT_ERROR;
    }
    
    /* Set bits in register to given value */
    gain &= 0b00000011;
    gain = gain << 5;
    val &= 0b10011111;
    val |= gain;
    
    /* Write register value back into GCONF2 register */
    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_GCONF2, val) ) {
        return EXIT_ERROR;
    }
    
    return EXIT_CLEAN;
}

/*******************************************************************************
 * @brief Light Interrupt Threshold Controls
 ******************************************************************************/
/**
 * @brief Gets the low threshold for ambient light interrupts
 *
 * @param[out] threshold current low threshold stored on the APDS-9960
 * @return EXIT_CLEAN if operation successful. EXIT_ERROR otherwise.
 */
uint8_t getLightIntLowThreshold(uint16_t *threshold)
{
    uint8_t val_byte;
    threshold = 0;
    
    /* Read value from ambient light low threshold, low byte register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_AILTL, &val_byte, 0 ) ) {
        return EXIT_ERROR;
    }
    *threshold = val_byte;
    
    /* Read value from ambient light low threshold, high byte register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_AILTH, &val_byte, 0 ) ) {
        return EXIT_ERROR;
    }
    *threshold = *threshold + ((uint16_t)val_byte << 8);
    
    return EXIT_CLEAN;
}

/**
 * @brief Sets the low threshold for ambient light interrupts
 *
 * @param[in] threshold low threshold value for interrupt to trigger
 * @return EXIT_CLEAN if operation successful. EXIT_ERROR otherwise.
 */
uint8_t setLightIntLowThreshold(uint16_t threshold)
{
    uint8_t val_low;
    uint8_t val_high;
    
    /* Break 16-bit threshold into 2 8-bit values */
    val_low = threshold & 0x00FF;
    val_high = (threshold & 0xFF00) >> 8;
    
    /* Write low byte */
    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_AILTL, val_low) ) {
        return EXIT_ERROR;
    }
    
    /* Write high byte */
    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_AILTH, val_high) ) {
        return EXIT_ERROR;
    }
    
    return EXIT_CLEAN;
}

/**
 * @brief Gets the high threshold for ambient light interrupts
 *
 * @param[out] threshold current low threshold stored on the APDS-9960
 * @return EXIT_CLEAN if operation successful. EXIT_ERROR otherwise.
 */
uint8_t getLightIntHighThreshold(uint16_t *threshold)
{
    uint8_t val_byte;
    threshold = 0;
    
    /* Read value from ambient light high threshold, low byte register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_AIHTL, &val_byte, 0 ) ) {
        return EXIT_ERROR;
    }
    *threshold = val_byte;
    
    /* Read value from ambient light high threshold, high byte register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_AIHTH, &val_byte, 0 ) ) {
        return EXIT_ERROR;
    }
    *threshold = *threshold + ((uint16_t)val_byte << 8);
    
    return EXIT_CLEAN;
}

/**
 * @brief Sets the high threshold for ambient light interrupts
 *
 * @param[in] threshold high threshold value for interrupt to trigger
 * @return EXIT_CLEAN if operation successful. EXIT_ERROR otherwise.
 */
uint8_t setLightIntHighThreshold(uint16_t threshold)
{
    uint8_t val_low;
    uint8_t val_high;
    
    /* Break 16-bit threshold into 2 8-bit values */
    val_low = threshold & 0x00FF;
    val_high = (threshold & 0xFF00) >> 8;
    
    /* Write low byte */
    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_AIHTL, val_low) ) {
        return EXIT_ERROR;
    }
    
    /* Write high byte */
    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_AIHTH, val_high) ) {
        return EXIT_ERROR;
    }
    
    return EXIT_CLEAN;
}

/*******************************************************************************
 * @brief Proximity Interrupt Threshold Controls
 ******************************************************************************/
/**
 * @brief Returns the lower threshold for proximity detection
 *
 * @return lower threshold
 */
uint8_t getProximityIntLowThreshold( uint8_t *threshold )
{
   /* Read value from PILT register */
   uint8_t retVal = i2c_read( APDS9960_I2C_ADDR, APDS9960_PILT, threshold, 0);
   
   return retVal;
}

/**
 * @brief Sets the lower threshold for proximity detection
 *
 * @param[in] threshold the lower proximity threshold
 * @return EXIT_CLEAN if operation successful. EXIT_ERROR otherwise.
 */
uint8_t setProximityIntLowThreshold(uint8_t threshold)
{
   uint8_t retVal = i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_PILT, threshold);
   return retVal;
}

/**
 * @brief Returns the high threshold for proximity detection
 *
 * @return high threshold
 */
uint8_t getProximityIntHighThreshold( uint8_t *threshold )
{
   /* Read value from PIHT register */
   uint8_t retVal = i2c_read( APDS9960_I2C_ADDR, APDS9960_PIHT, threshold, 0);
   return retVal;
}

/**
 * @brief Sets the high threshold for proximity detection
 *
 * @param[in] threshold the high proximity threshold
 * @return EXIT_CLEAN if operation successful. EXIT_ERROR otherwise.
 */
uint8_t setProximityIntHighThreshold(uint8_t threshold)
{
    return i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_PIHT, threshold) ;
}


/*******************************************************************************
 * @brief Interrupt Enable Controls
 ******************************************************************************/
/**
 * @brief Gets if ambient light interrupts are enabled or not
 *
 * @return 1 if interrupts are enabled, 0 if not. 0xFF on error.
 */
uint8_t getAmbientLightIntEnable()
{
    uint8_t val;
    
    /* Read value from ENABLE register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_ENABLE, &val, 0) ) {
        return EXIT_ERROR;
    }
    
    /* Shift and mask out AIEN bit */
    val = (val >> 4) & 0b00000001;
    
    return val;
}

/**
 * @brief Turns ambient light interrupts on or off
 *
 * @param[in] enable 1 to enable interrupts, 0 to turn them off
 * @return EXIT_CLEAN if operation successful. EXIT_ERROR otherwise.
 */
uint8_t setAmbientLightIntEnable(uint8_t enable)
{
    uint8_t val;
    
    /* Read value from ENABLE register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_ENABLE, &val, 0) ) {
        return EXIT_ERROR;
    }
    
    /* Set bits in register to given value */
    enable &= 0b00000001;
    enable = enable << 4;
    val &= 0b11101111;
    val |= enable;
    
    /* Write register value back into ENABLE register */
    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_ENABLE, val) ) {
        return EXIT_ERROR;
    }
    
    return EXIT_CLEAN;
}

/**
 * @brief Gets if proximity interrupts are enabled or not
 *
 * @return 1 if interrupts are enabled, 0 if not. 0xFF on error.
 */
uint8_t getProximityIntEnable()
{
    uint8_t val;
    
    /* Read value from ENABLE register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_ENABLE, &val, 0) ) {
        return EXIT_ERROR;
    }
    
    /* Shift and mask out PIEN bit */
    val = (val >> 5) & 0b00000001;
    
    return val;
}

/**
 * @brief Turns proximity interrupts on or off
 *
 * @param[in] enable 1 to enable interrupts, 0 to turn them off
 * @return EXIT_CLEAN if operation successful. EXIT_ERROR otherwise.
 */
uint8_t setProximityIntEnable(uint8_t enable)
{
    uint8_t val;
    
    /* Read value from ENABLE register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_ENABLE, &val, 0) ) {
        return EXIT_ERROR;
    }
    
    /* Set bits in register to given value */
    enable &= 0b00000001;
    enable = enable << 5;
    val &= 0b11011111;
    val |= enable;
    
    /* Write register value back into ENABLE register */
    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_ENABLE, val) ) {
        return EXIT_ERROR;
    }
    
    return EXIT_CLEAN;
}

/**
 * @brief Gets if gesture interrupts are enabled or not
 *
 * @return 1 if interrupts are enabled, 0 if not. 0xFF on error.
 */
uint8_t getGestureIntEnable()
{
    uint8_t val;
    
    /* Read value from GCONF4 register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_GCONF4, &val, 0) ) {
        return EXIT_ERROR;
    }
    
    /* Shift and mask out GIEN bit */
    val = (val >> 1) & 0b00000001;
    
    return val;
}

/**
 * @brief Turns gesture-related interrupts on or off
 *
 * @param[in] enable 1 to enable interrupts, 0 to turn them off
 * @return EXIT_CLEAN if operation successful. EXIT_ERROR otherwise.
 */
uint8_t setGestureIntEnable(uint8_t enable)
{
    uint8_t val;
    
    /* Read value from GCONF4 register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_GCONF4, &val, 0) ) {
        return EXIT_ERROR;
    }
    
    /* Set bits in register to given value */
    enable &= 0b00000001;
    enable = enable << 1;
    val &= 0b11111101;
    val |= enable;
    
    /* Write register value back into GCONF4 register */
    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_GCONF4, val) ) {
        return EXIT_ERROR;
    }
    
    return EXIT_CLEAN;
}

/*******************************************************************************
 * @brief Clear Interrupts
 ******************************************************************************/

/**
 * @brief Clears the ambient light interrupt
 *
 * @return EXIT_CLEAN if operation completed successfully. EXIT_ERROR otherwise.
 */
uint8_t clearAmbientLightInt()
{
    uint8_t throwaway;
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_AICLEAR, &throwaway, 0 ) ) {
        return EXIT_ERROR;
    }
    
    return EXIT_CLEAN;
}

/**
 * @brief Clears the proximity interrupt
 *
 * @return EXIT_CLEAN if operation completed successfully. EXIT_ERROR otherwise.
 */
uint8_t clearProximityInt()
{
    uint8_t throwaway;
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_PICLEAR, &throwaway, 0 ) ) {
        return EXIT_ERROR;
    }
    
    return EXIT_CLEAN;
}

/*******************************************************************************
 * @brief Ambient Light Controls
 ******************************************************************************/
/**
 * @brief Reads the ambient (clear) light level as a 16-bit value
 *
 * @param[out] val value of the light sensor.
 * @return EXIT_CLEAN if operation successful. EXIT_ERROR otherwise.
 */
uint8_t readAmbientLight(uint16_t *val)
{
    uint8_t val_byte;
    
    /* Read value from clear channel, low byte register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_CDATAL, &val_byte, 0 ) ) {
        return EXIT_ERROR;
    }
    *val = val_byte;
    
    /* Read value from clear channel, high byte register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_CDATAH, &val_byte, 0 ) ) {
        return EXIT_ERROR;
    }
    *val = *val + ((uint16_t)val_byte << 8);
    
    return EXIT_CLEAN;
}

/**
 * @brief Reads the red light level as a 16-bit value
 *
 * @param[out] val value of the light sensor.
 * @return EXIT_CLEAN if operation successful. EXIT_ERROR otherwise.
 */
uint8_t readRedLight(uint16_t *val)
{
    uint8_t val_byte;
    
    /* Read value from clear channel, low byte register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_RDATAL, &val_byte, 0 ) ) {
        return EXIT_ERROR;
    }
    *val = val_byte;
    
    /* Read value from clear channel, high byte register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_RDATAH, &val_byte, 0 ) ) {
        return EXIT_ERROR;
    }
    *val = *val + ((uint16_t)val_byte << 8);
    
    return EXIT_CLEAN;
}
 
/**
 * @brief Reads the green light level as a 16-bit value
 *
 * @param[out] val value of the light sensor.
 * @return EXIT_CLEAN if operation successful. EXIT_ERROR otherwise.
 */
uint8_t readGreenLight(uint16_t *val)
{
    uint8_t val_byte;
    
    /* Read value from clear channel, low byte register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_GDATAL, &val_byte, 0 ) ) {
        return EXIT_ERROR;
    }
    *val = val_byte;
    
    /* Read value from clear channel, high byte register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_GDATAH, &val_byte, 0 ) ) {
        return EXIT_ERROR;
    }
    *val = *val + ((uint16_t)val_byte << 8);
    
    return EXIT_CLEAN;
}

/**
 * @brief Reads the red light level as a 16-bit value
 *
 * @param[out] val value of the light sensor.
 * @return EXIT_CLEAN if operation successful. EXIT_ERROR otherwise.
 */
uint8_t readBlueLight(uint16_t *val)
{
    uint8_t val_byte;
    val = 0;
    
    /* Read value from clear channel, low byte register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_BDATAL, &val_byte, 0 ) ) {
        return EXIT_ERROR;
    }
    *val = val_byte;
    
    /* Read value from clear channel, high byte register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_BDATAH, &val_byte, 0 ) ) {
        return EXIT_ERROR;
    }
    *val = *val + ((uint16_t)val_byte << 8);
    
    return EXIT_CLEAN;
}

/*******************************************************************************
 * @brief Gesture Controls
 ******************************************************************************/
/**
 * @brief Determines if there is a gesture available for reading
 *
 * @return EXIT_CLEAN if gesture available. EXIT_ERROR otherwise.
 */
uint8_t isGestureAvailable()
{
    uint8_t val;
    
    /* Read value from GSTATUS register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_GSTATUS, &val, 0) ) {
        return EXIT_ERROR;
    }
    
    /* Shift and mask out GVALID bit */
    val &= APDS9960_GVALID;
    
    /* Return EXIT_CLEAN/EXIT_ERROR based on GVALID bit */
    if( val == 1) {
        return EXIT_CLEAN;
    } else {
        return EXIT_ERROR;
    }
}

/**
 * @brief Processes a gesture event and returns best guessed gesture
 *
 * @return Number corresponding to gesture. -1 on error.
 */
int readGesture()
{
    uint8_t fifo_level = 0;
    uint8_t bytes_read = 0;
    uint8_t fifo_data[128];
    uint8_t gstatus;
    int motion;
    int i;
    
    /* Make sure that power and gesture is on and data is valid */
    if( !isGestureAvailable() || !(getMode() & 0b01000001) ) {
        return DIR_NONE;
    }
    
    /* Keep looping as long as gesture data is valid */
    while(1) {
    
        /* Wait some time to collect next batch of FIFO data */
        usleep(FIFO_PAUSE_TIME);
        
        /* Get the contents of the STATUS register. Is data still valid? */
        if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_GSTATUS, &gstatus, 0 ) ) {
            return EXIT_ERROR;
        }
        
        /* If we have valid data, read in FIFO */
        if( (gstatus & APDS9960_GVALID) == APDS9960_GVALID ) {
        
            /* Read the current FIFO level */
            if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_GFLVL, &fifo_level, 0 ) ) {
                return EXIT_ERROR;
            }

#if 0
            Serial.print("FIFO Level: ");
            Serial.println(fifo_level);
#endif

            /* If there's stuff in the FIFO, read it into our data block */
            if( fifo_level > 0) {
                bytes_read = i2c_read( APDS9960_I2C_ADDR,  APDS9960_GFIFO_U, 
                                        (uint8_t*)fifo_data, 
                                                (fifo_level * 4) );
                if( bytes_read == -1 ) {
                    return EXIT_ERROR;
                }
#if 0
                Serial.print("FIFO Dump: ");
                for ( i = 0; i < bytes_read; i++ ) {
                    Serial.print(fifo_data[i]);
                    Serial.print(" ");
                }
                Serial.println();
#endif

                /* If at least 1 set of data, sort the data into U/D/L/R */
                if( bytes_read >= 4 ) {
                    for( i = 0; i < bytes_read; i += 4 ) {
                        gesture_data_.u_data[gesture_data_.index] = \
                                                            fifo_data[i + 0];
                        gesture_data_.d_data[gesture_data_.index] = \
                                                            fifo_data[i + 1];
                        gesture_data_.l_data[gesture_data_.index] = \
                                                            fifo_data[i + 2];
                        gesture_data_.r_data[gesture_data_.index] = \
                                                            fifo_data[i + 3];
                        gesture_data_.index++;
                        gesture_data_.total_gestures++;
                    }
                    
#if 0
                Serial.print("Up Data: ");
                for ( i = 0; i < gesture_data_.total_gestures; i++ ) {
                    Serial.print(gesture_data_.u_data[i]);
                    Serial.print(" ");
                }
                Serial.println();
#endif

                    /* Filter and process gesture data. Decode near/far state */
                    if( processGestureData() ) {
                        if( decodeGesture() ) {
                            //***TODO: U-Turn Gestures
#if 0
                            //Serial.println(gesture_motion_);
#endif
                        }
                    }
                    
                    /* Reset data */
                    gesture_data_.index = 0;
                    gesture_data_.total_gestures = 0;
                }
            }
        } else {
    
            /* Determine best guessed gesture and clean up */
            usleep(FIFO_PAUSE_TIME);
            decodeGesture();
            motion = gesture_motion_;
#if 0
            Serial.print("END: ");
            Serial.println(gesture_motion_);
#endif
            resetGestureParameters();
            return motion;
        }
    }
}

/**
 * @brief Resets all the parameters in the gesture data member
 */
void resetGestureParameters()
{
    gesture_data_.index = 0;
    gesture_data_.total_gestures = 0;
    
    gesture_ud_delta_ = 0;
    gesture_lr_delta_ = 0;
    
    gesture_ud_count_ = 0;
    gesture_lr_count_ = 0;
    
    gesture_near_count_ = 0;
    gesture_far_count_ = 0;
    
    gesture_state_ = 0;
    gesture_motion_ = DIR_NONE;
}

/**
 * @brief Processes the raw gesture data to determine swipe direction
 *
 * @return EXIT_CLEAN if near or far state seen. EXIT_ERROR otherwise.
 */
uint8_t processGestureData()
{
    uint8_t u_first = 0;
    uint8_t d_first = 0;
    uint8_t l_first = 0;
    uint8_t r_first = 0;
    uint8_t u_last = 0;
    uint8_t d_last = 0;
    uint8_t l_last = 0;
    uint8_t r_last = 0;
    int ud_ratio_first;
    int lr_ratio_first;
    int ud_ratio_last;
    int lr_ratio_last;
    int ud_delta;
    int lr_delta;
    int i;

    /* If we have less than 4 total gestures, that's not enough */
    if( gesture_data_.total_gestures <= 4 ) {
        return EXIT_ERROR;
    }
    
    /* Check to make sure our data isn't out of bounds */
    if( (gesture_data_.total_gestures <= 32) && \
        (gesture_data_.total_gestures > 0) ) {
        
        /* Find the first value in U/D/L/R above the threshold */
        for( i = 0; i < gesture_data_.total_gestures; i++ ) {
            if( (gesture_data_.u_data[i] > GESTURE_THRESHOLD_OUT) &&
                (gesture_data_.d_data[i] > GESTURE_THRESHOLD_OUT) &&
                (gesture_data_.l_data[i] > GESTURE_THRESHOLD_OUT) &&
                (gesture_data_.r_data[i] > GESTURE_THRESHOLD_OUT) ) {
                
                u_first = gesture_data_.u_data[i];
                d_first = gesture_data_.d_data[i];
                l_first = gesture_data_.l_data[i];
                r_first = gesture_data_.r_data[i];
                break;
            }
        }
        
        /* If one of the _first values is 0, then there is no good data */
        if( (u_first == 0) || (d_first == 0) || \
            (l_first == 0) || (r_first == 0) ) {
            
            return EXIT_ERROR;
        }
        /* Find the last value in U/D/L/R above the threshold */
        for( i = gesture_data_.total_gestures - 1; i >= 0; i-- ) {
#if 0
            Serial.print(F("Finding last: "));
            Serial.print(F("U:"));
            Serial.print(gesture_data_.u_data[i]);
            Serial.print(F(" D:"));
            Serial.print(gesture_data_.d_data[i]);
            Serial.print(F(" L:"));
            Serial.print(gesture_data_.l_data[i]);
            Serial.print(F(" R:"));
            Serial.println(gesture_data_.r_data[i]);
#endif
            if( (gesture_data_.u_data[i] > GESTURE_THRESHOLD_OUT) &&
                (gesture_data_.d_data[i] > GESTURE_THRESHOLD_OUT) &&
                (gesture_data_.l_data[i] > GESTURE_THRESHOLD_OUT) &&
                (gesture_data_.r_data[i] > GESTURE_THRESHOLD_OUT) ) {
                
                u_last = gesture_data_.u_data[i];
                d_last = gesture_data_.d_data[i];
                l_last = gesture_data_.l_data[i];
                r_last = gesture_data_.r_data[i];
                break;
            }
        }
    }
    
    /* Calculate the first vs. last ratio of up/down and left/right */
    ud_ratio_first = ((u_first - d_first) * 100) / (u_first + d_first);
    lr_ratio_first = ((l_first - r_first) * 100) / (l_first + r_first);
    ud_ratio_last = ((u_last - d_last) * 100) / (u_last + d_last);
    lr_ratio_last = ((l_last - r_last) * 100) / (l_last + r_last);
       
#if 0
    Serial.print(F("Last Values: "));
    Serial.print(F("U:"));
    Serial.print(u_last);
    Serial.print(F(" D:"));
    Serial.print(d_last);
    Serial.print(F(" L:"));
    Serial.print(l_last);
    Serial.print(F(" R:"));
    Serial.println(r_last);

    Serial.print(F("Ratios: "));
    Serial.print(F("UD Fi: "));
    Serial.print(ud_ratio_first);
    Serial.print(F(" UD La: "));
    Serial.print(ud_ratio_last);
    Serial.print(F(" LR Fi: "));
    Serial.print(lr_ratio_first);
    Serial.print(F(" LR La: "));
    Serial.println(lr_ratio_last);
#endif
       
    /* Determine the difference between the first and last ratios */
    ud_delta = ud_ratio_last - ud_ratio_first;
    lr_delta = lr_ratio_last - lr_ratio_first;
    
#if 0
    Serial.print("Deltas: ");
    Serial.print("UD: ");
    Serial.print(ud_delta);
    Serial.print(" LR: ");
    Serial.println(lr_delta);
#endif

    /* Accumulate the UD and LR delta values */
    gesture_ud_delta_ += ud_delta;
    gesture_lr_delta_ += lr_delta;
    
#if 0
    Serial.print("Accumulations: ");
    Serial.print("UD: ");
    Serial.print(gesture_ud_delta_);
    Serial.print(" LR: ");
    Serial.println(gesture_lr_delta_);
#endif
    
    /* Determine U/D gesture */
    if( gesture_ud_delta_ >= GESTURE_SENSITIVITY_1 ) {
        gesture_ud_count_ = 1;
    } else if( gesture_ud_delta_ <= -GESTURE_SENSITIVITY_1 ) {
        gesture_ud_count_ = -1;
    } else {
        gesture_ud_count_ = 0;
    }
    
    /* Determine L/R gesture */
    if( gesture_lr_delta_ >= GESTURE_SENSITIVITY_1 ) {
        gesture_lr_count_ = 1;
    } else if( gesture_lr_delta_ <= -GESTURE_SENSITIVITY_1 ) {
        gesture_lr_count_ = -1;
    } else {
        gesture_lr_count_ = 0;
    }
    
    /* Determine Near/Far gesture */
    if( (gesture_ud_count_ == 0) && (gesture_lr_count_ == 0) ) {
        if( (abs(ud_delta) < GESTURE_SENSITIVITY_2) && \
            (abs(lr_delta) < GESTURE_SENSITIVITY_2) ) {
            
            if( (ud_delta == 0) && (lr_delta == 0) ) {
                gesture_near_count_++;
            } else if( (ud_delta != 0) || (lr_delta != 0) ) {
                gesture_far_count_++;
            }
            
            if( (gesture_near_count_ >= 10) && (gesture_far_count_ >= 2) ) {
                if( (ud_delta == 0) && (lr_delta == 0) ) {
                    gesture_state_ = NEAR_STATE;
                } else if( (ud_delta != 0) && (lr_delta != 0) ) {
                    gesture_state_ = FAR_STATE;
                }
                return EXIT_CLEAN;
            }
        }
    } else {
        if( (abs(ud_delta) < GESTURE_SENSITIVITY_2) && \
            (abs(lr_delta) < GESTURE_SENSITIVITY_2) ) {
                
            if( (ud_delta == 0) && (lr_delta == 0) ) {
                gesture_near_count_++;
            }
            
            if( gesture_near_count_ >= 10 ) {
                gesture_ud_count_ = 0;
                gesture_lr_count_ = 0;
                gesture_ud_delta_ = 0;
                gesture_lr_delta_ = 0;
            }
        }
    }
    
#if 0
    Serial.print("UD_CT: ");
    Serial.print(gesture_ud_count_);
    Serial.print(" LR_CT: ");
    Serial.print(gesture_lr_count_);
    Serial.print(" NEAR_CT: ");
    Serial.print(gesture_near_count_);
    Serial.print(" FAR_CT: ");
    Serial.println(gesture_far_count_);
    Serial.println("----------");
#endif
    
    return EXIT_ERROR;
}

/**
 * @brief Determines swipe direction or near/far state
 *
 * @return EXIT_CLEAN if near/far event. EXIT_ERROR otherwise.
 */
uint8_t decodeGesture()
{
    /* Return if near or far event is detected */
    if( gesture_state_ == NEAR_STATE ) {
        gesture_motion_ = DIR_NEAR;
        return EXIT_CLEAN;
    } else if ( gesture_state_ == FAR_STATE ) {
        gesture_motion_ = DIR_FAR;
        return EXIT_CLEAN;
    }
    
    /* Determine swipe direction */
    if( (gesture_ud_count_ == -1) && (gesture_lr_count_ == 0) ) {
        gesture_motion_ = DIR_UP;
    } else if( (gesture_ud_count_ == 1) && (gesture_lr_count_ == 0) ) {
        gesture_motion_ = DIR_DOWN;
    } else if( (gesture_ud_count_ == 0) && (gesture_lr_count_ == 1) ) {
        gesture_motion_ = DIR_RIGHT;
    } else if( (gesture_ud_count_ == 0) && (gesture_lr_count_ == -1) ) {
        gesture_motion_ = DIR_LEFT;
    } else if( (gesture_ud_count_ == -1) && (gesture_lr_count_ == 1) ) {
        if( abs(gesture_ud_delta_) > abs(gesture_lr_delta_) ) {
            gesture_motion_ = DIR_UP;
        } else {
            gesture_motion_ = DIR_RIGHT;
        }
    } else if( (gesture_ud_count_ == 1) && (gesture_lr_count_ == -1) ) {
        if( abs(gesture_ud_delta_) > abs(gesture_lr_delta_) ) {
            gesture_motion_ = DIR_DOWN;
        } else {
            gesture_motion_ = DIR_LEFT;
        }
    } else if( (gesture_ud_count_ == -1) && (gesture_lr_count_ == -1) ) {
        if( abs(gesture_ud_delta_) > abs(gesture_lr_delta_) ) {
            gesture_motion_ = DIR_UP;
        } else {
            gesture_motion_ = DIR_LEFT;
        }
    } else if( (gesture_ud_count_ == 1) && (gesture_lr_count_ == 1) ) {
        if( abs(gesture_ud_delta_) > abs(gesture_lr_delta_) ) {
            gesture_motion_ = DIR_DOWN;
        } else {
            gesture_motion_ = DIR_RIGHT;
        }
    } else {
        return EXIT_ERROR;
    }
    
    return EXIT_CLEAN;
}

/**
 * @brief Tells if the gesture state machine is currently running
 *
 * @return 1 if gesture state machine is running, 0 if not. 0xFF on error.
 */
uint8_t getGestureMode( void )
{
    uint8_t val;
    
    /* Read value from GCONF4 register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_GCONF4, &val, 0 ) ) {
        return EXIT_ERROR;
    }
    
    /* Mask out GMODE bit */
    val &= 0b00000001;
    
    return val;
}

/**
 * @brief Tells the state machine to either enter or exit gesture state machine
 *
 * @param[in] mode 1 to enter gesture state machine, 0 to exit.
 * @return True if operation successful. False otherwise.
 */
uint8_t setGestureMode(uint8_t mode)
{
    uint8_t val;
    
    /* Read value from GCONF4 register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_GCONF4, &val, 0 ) ) {
        return EXIT_ERROR;
    }
    
    /* Set bits in register to given value */
    mode &= 0b00000001;
    val &= 0b11111110;
    val |= mode;
    
    /* Write register value back into GCONF4 register */
    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_GCONF4, val ) ) {
        return EXIT_ERROR;
    }
    
    return EXIT_CLEAN;
}
/*******************************************************************************
 * @brief LED Drive Strength Controls
 ******************************************************************************/

/**
 * @brief Returns LED drive strength for proximity and ALS
 *
 * Value    LED Current
 *   0        100 mA
 *   1         50 mA
 *   2         25 mA
 *   3         12.5 mA
 *
 * @return the value of the LED drive strength. 0xFF on failure.
 */
uint8_t getLEDDrive( void )
{
    uint8_t val;
    
    /* Read value from CONTROL register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_CONTROL, &val, 0) ) {
        return EXIT_ERROR;
    }
    
    /* Shift and mask out LED drive bits */
    val = (val >> 6) & 0b00000011;
    
    return val;
}

/**
 * @brief Sets the LED drive strength for proximity and ALS
 *
 * Value    LED Current
 *   0        100 mA
 *   1         50 mA
 *   2         25 mA
 *   3         12.5 mA
 *
 * @param[in] drive the value (0-3) for the LED drive strength
 * @return EXIT_CLEAN if operation successful. EXIT_ERROR otherwise.
 */
uint8_t setLEDDrive(uint8_t drive)
{
    uint8_t val;
    
    /* Read value from CONTROL register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_CONTROL, &val, 0) ) {
        return EXIT_ERROR;
    }
    
    /* Set bits in register to given value */
    drive &= 0b00000011;
    drive = drive << 6;
    val &= 0b00111111;
    val |= drive;
    
    /* Write register value back into CONTROL register */
    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_CONTROL, val) ) {
        return EXIT_ERROR;
    }
    
    return EXIT_CLEAN;
}


/**
 * @brief Get the current LED boost value
 * 
 * Value  Boost Current
 *   0        100%
 *   1        150%
 *   2        200%
 *   3        300%
 *
 * @return The LED boost value. 0xFF on failure.
 */
uint8_t getLEDBoost( void )
{
    uint8_t val;
    
    /* Read value from CONFIG2 register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_CONFIG2, &val, 0) ) {
        return EXIT_ERROR;
    }
    
    /* Shift and mask out LED_BOOST bits */
    val = (val >> 4) & 0b00000011;
    
    return val;
}

/**
 * @brief Sets the LED current boost value
 *
 * Value  Boost Current
 *   0        100%
 *   1        150%
 *   2        200%
 *   3        300%
 *
 * @param[in] drive the value (0-3) for current boost (100-300%)
 * @return EXIT_CLEAN if operation successful. EXIT_ERROR otherwise.
 */
uint8_t setLEDBoost(uint8_t boost)
{
    uint8_t val;
    
    /* Read value from CONFIG2 register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_CONFIG2, &val, 0) ) {
        return EXIT_ERROR;
    }
    
    /* Set bits in register to given value */
    boost &= 0b00000011;
    boost = boost << 4;
    val &= 0b11001111;
    val |= boost;
    
    /* Write register value back into CONFIG2 register */
    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_CONFIG2, val) ) {
        return EXIT_ERROR;
    }
    
    return EXIT_CLEAN;
}    


/*******************************************************************************
 * @brief Proxmity Photodiode Select
 ******************************************************************************/
/**
 * @brief Gets proximity gain compensation enable
 *
 * @return 1 if compensation is enabled. 0 if not. 0xFF on error.
 */
uint8_t getProxGainCompEnable( void )
{
    uint8_t val;
    
    /* Read value from CONFIG3 register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_CONFIG3, &val, 0) ) {
        return EXIT_ERROR;
    }
    
    /* Shift and mask out PCMP bits */
    val = (val >> 5) & 0b00000001;
    
    return val;
}

/**
 * @brief Sets the proximity gain compensation enable
 *
 * @param[in] enable 1 to enable compensation. 0 to disable compensation.
 * @return EXIT_CLEAN if operation successful. EXIT_ERROR otherwise.
 */
 uint8_t setProxGainCompEnable(uint8_t enable)
{
    uint8_t val;
    
    /* Read value from CONFIG3 register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_CONFIG3, &val, 0) ) {
        return EXIT_ERROR;
    }
    
    /* Set bits in register to given value */
    enable &= 0b00000001;
    enable = enable << 5;
    val &= 0b11011111;
    val |= enable;
    
    /* Write register value back into CONFIG3 register */
    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_CONFIG3, val) ) {
        return EXIT_ERROR;
    }
    
    return EXIT_CLEAN;
}


/**
 * @brief Gets the current mask for enabled/disabled proximity photodiodes
 *
 * 1 = disabled, 0 = enabled
 * Bit    Photodiode
 *  3       UP
 *  2       DOWN
 *  1       LEFT
 *  0       RIGHT
 *
 * @return Current proximity mask for photodiodes. 0xFF on error.
 */
uint8_t getProxPhotoMask( void )
{
    uint8_t val;
    
    /* Read value from CONFIG3 register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_CONFIG3, &val, 0) ) {
        return EXIT_ERROR;
    }
    
    /* Mask out photodiode enable mask bits */
    val &= 0b00001111;
    
    return val;
}

/**
 * @brief Sets the mask for enabling/disabling proximity photodiodes
 *
 * 1 = disabled, 0 = enabled
 * Bit    Photodiode
 *  3       UP
 *  2       DOWN
 *  1       LEFT
 *  0       RIGHT
 *
 * @param[in] mask 4-bit mask value
 * @return EXIT_CLEAN if operation successful. EXIT_ERROR otherwise.
 */
uint8_t setProxPhotoMask(uint8_t mask)
{
    uint8_t val;
    
    /* Read value from CONFIG3 register */
    if( EXIT_CLEAN != i2c_read( APDS9960_I2C_ADDR, APDS9960_CONFIG3, &val, 0) ) {
        return EXIT_ERROR;
    }
    
    /* Set bits in register to given value */
    mask &= 0b00001111;
    val &= 0b11110000;
    val |= mask;
    
    /* Write register value back into CONFIG3 register */
    if( EXIT_CLEAN != i2c_write_byte( APDS9960_I2C_ADDR, APDS9960_CONFIG3, val) ) {
        return EXIT_ERROR;
    }
    
    return EXIT_CLEAN;
}


uint8_t apds9960_read_id( uint8_t *id )
{
   uint8_t retVal = i2c_read( APDS9960_I2C_ADDR, APDS9960_ID, id, 0 );
   return retVal;
}



