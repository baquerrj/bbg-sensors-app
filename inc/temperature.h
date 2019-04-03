/*
 * =================================================================================
 *    @file     temperature.h
 *    @brief   Header for temperature sensor thread 
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

#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include "common.h"
#include "i2c.h"

#include <mqueue.h>

#define TEMP_QUEUE_NAME "/temperature-queue"

/* Default address for Temperature Sensor TMP102 */
#define TMP102_SLAVE       (0x48)

/* Regsiter addresses for TMP102 */
#define TMP102_REG_TEMP    (0x00)
#define TMP102_REG_CONFIG  (0x01)
#define TMP102_TLOW        (0x02)
#define TMP102_THIGH       (0x03)

/* Default configuration */
#define TMP102_SHUTDOWN_MODE     (1)
#define TMP102_THERMOSTAT_MODE   (1)
#define TMP102_POLARITY          (1)
#define TMP102_FAULT_QUEUE       (1)
#define TMP102_RESOLUTION_0      (2)
#define TMP102_RESOLUTION_1      (4)
#define TMP102_EXTENDED_MODE     (0)
#define TMP102_CONVERSION_RATE   (2)


typedef struct {
   uint16_t res_0;
   uint16_t res_1;
} conv_res_t;

typedef struct {
   uint16_t shutdown;
   uint16_t thermostat;
} tmp102_mode_t;

typedef struct {
   tmp102_mode_t mode;     /* shutdown and thermostat modes */
   uint16_t polarity;      /* polarity */
   uint16_t fault_queue;   /* fault queue */
   conv_res_t resolution;  /* converter resolution */
   uint16_t one_shot;      /* when in shutdown mode, writing 1 starts a single conversion */
   uint16_t operation;     /* extended vs normal operation */
   uint16_t alert;         /* alerrt bit */
   uint16_t conv_rate;     /* conversion rate */
} tmp102_config_t;

/*
 * =================================================================================
 * Function:       tmp102_write_config
 * @brief   Write configuration register of TMP102 sensor
 *
 * @param   *config_reg  - pointer to struct with values to write to configuration register
 * @return  see i2c_write()
 * =================================================================================
 */
int tmp102_write_config( tmp102_config_t *config_reg );

/*
 * =================================================================================
 * Function:       tmp102_get_temp
 * @brief   Read temperature registers fo TMP102 sensor and decode temperature value
 *
 * @param   *temperature - pointer to location to write decoded value to
 * @return  EXIT_CLEAN if successful, otherwise EXIT_ERROR
 * <+DETAILED+>
 * =================================================================================
 */
int tmp102_get_temp( float *temperature );

/*
 * =================================================================================
 * Function:       tmp102_write_thigh
 * @brief   Write value thigh (in celsius) to Thigh register for TMP102 sensor
 *
 * @param   thigh - value to write to Thigh register
 * @return  EXIT_CLEAN if successful, otherwise EXIT_ERROR
 * =================================================================================
 */
int tmp102_write_thigh( float thigh );

/*
 * =================================================================================
 * Function:       tmp102_write_tlow
 * @brief   Write value tlow (in celsius) to Tlow register for TMP102 sensor
 *
 * @param   tlow - value to write to Tlow register
 * @return  EXIT_CLEAN if successful, otherwise EXIT_ERROR
 * =================================================================================
 */
int tmp102_write_tlow( float tlow );

/*
 * =================================================================================
 * Function:       tmp102_read_thigh
 * @brief   Read value of THigh register of TMP102 sensor and store value (in celsius) in thigh
 *
 * @param   thigh - pointer to location to store decoded temperature value to
 * @return  EXIT_CLEAN if successful, EXIT_ERROR otherwise
 * =================================================================================
 */
int tmp102_read_thigh( float *thigh );

/*
 * =================================================================================
 * Function:       tmp102_read_tlow
 * @brief   Read value of TLow register of TMP102 sensor and store value (in celsius) in tlow
 *
 * @param   tlow  - pointer to location to store decoded temperature value to
 * @return  EXIT_CLEAN if successful, EXIT_ERROR otherwise
 * =================================================================================
 */
int tmp102_read_tlow( float *tlow );

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
mqd_t get_temperature_queue( void );

/*
 * =================================================================================
 * Function:       temp_queue_init
 * @brief   Initialize message queue for temperature sensor thread
 *
 * @param   void
 * @return  msg_q - file descriptor for initialized message queue
 * =================================================================================
 */
int temp_queue_init( void );

/*
 * =================================================================================
 * Function:       temperature_fn
 * @brief   Entry point for temperature sensor processing thread
 *
 * @param   thread_args  - void ptr to arguments used to initialize thread
 * @return  NULL  - We don't really exit from this function, 
 *                   since the exit point is thread_exit()
 * =================================================================================
 */
void *temperature_fn( void *thread_args );


#endif /* TEMPERATURE_H */
