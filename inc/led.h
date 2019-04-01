/*
 * =================================================================================
 *    @file     led.h
 *    @brief    
 *
 *  Define macros for interacting with user LEDs of BeagleBone Green.
 *
 *    @author   Roberto Baquerizo (baquerrj), roba8460@colorado.edu
 *
 *    @internal
 *       Created:  03/31/2019
 *      Revision:  none
 *      Compiler:  gcc
 *  Organization:  University of Colorado: Boulder
 *
 *  This source code is released for free distribution under the terms of the
 *  GNU General Public License as published by the Free Software Foundation.
 * =================================================================================
 */


#ifndef  LED_H
#define  LED_H

#define LED0_PATH    "/sys/class/leds/beaglebone:green:heartbeat"
#define LED1_PATH    "/sys/class/leds/beaglebone:green:mmc0"
#define LED2_PATH    "/sys/class/leds/beaglebone:green:usr2"
#define LED3_PATH    "/sys/class/leds/beaglebone:green:usr3"


#define LED_BRIGHTNESS(LED_PATH) (LED_PATH"/brightness") 
#define LED_TRIGGER(LED_PATH)    (LED_PATH"/trigger") 
#define LED_DELAYON(LED_PATH)    (LED_PATH"/delay_on")
#define LED_DELAYOFF(LED_PATH)   (LED_PATH"/delay_off")


#define LED0_BRIGHTNESS LED_BRIGHTNESS(LED0_PATH)
#define LED1_BRIGHTNESS LED_BRIGHTNESS(LED1_PATH)
#define LED2_BRIGHTNESS LED_BRIGHTNESS(LED2_PATH)
#define LED3_BRIGHTNESS LED_BRIGHTNESS(LED3_PATH)

#define LED0_TRIGGER LED_TRIGGER(LED0_PATH)
#define LED1_TRIGGER LED_TRIGGER(LED1_PATH)
#define LED2_TRIGGER LED_TRIGGER(LED2_PATH)
#define LED3_TRIGGER LED_TRIGGER(LED3_PATH)

#define LED0_DELAYON LED_DELAYON(LED0_PATH)
#define LED1_DELAYON LED_DELAYON(LED1_PATH)
#define LED2_DELAYON LED_DELAYON(LED2_PATH)
#define LED3_DELAYON LED_DELAYON(LED3_PATH)

#define LED0_DELAYOFF LED_DELAYOFF(LED0_PATH)
#define LED1_DELAYOFF LED_DELAYOFF(LED1_PATH)
#define LED2_DELAYOFF LED_DELAYOFF(LED2_PATH)
#define LED3_DELAYOFF LED_DELAYOFF(LED3_PATH)

#include <stdio.h>



/*
 * =================================================================================
 * Function:       get_status
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
void get_status( const char *led );

/*
 * =================================================================================
 * Function:       get_trigger
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
int set_trigger( const char *led, char *trigger );

/*
 * =================================================================================
 * Function:       set_delay
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
int set_delay( const char *led_path, int delay );


/*
 * =================================================================================
 * Function:       led_on
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
void led_on( const char *led );


/*
 * =================================================================================
 * Function:       led_off
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
void led_off( const char *led );


/*
 * =================================================================================
 * Function:       led_toggle
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
void led_toggle( const char* led );



#endif   /* LED_H */
