/*
 * =================================================================================
 *    @file     light.h
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

#ifndef LIGHT_H
#define LIGHT_H

#include "common.h"

#include <mqueue.h>
#define LIGHT_QUEUE_NAME "/light-queue"



/*
 * =================================================================================
 * Function:       get_light_queue
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
mqd_t get_light_queue( void );

/*
 * =================================================================================
 * Function:       light_queue_init
 * @brief
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
int light_queue_init( void );

/*
 * =================================================================================
 * Function:       light_fn
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
void *light_fn( void *arg );


#endif /* LIGHT_H */
