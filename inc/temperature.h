/*
 * =================================================================================
 *    @file     temperature.h
 *    @brief    
 *
 *  <+DETAILED+>
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

#include <mqueue.h>
#define TEMP_QUEUE_NAME "/temperature-queue"



/*
 * =================================================================================
 * Function:       get_temperature_queue
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
mqd_t get_temperature_queue( void );


/*
 * =================================================================================
 * Function:       temperature_fn
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
void *temperature_fn( void *arg );


#endif /* TEMPERATURE_H */
