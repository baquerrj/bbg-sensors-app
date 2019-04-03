/*
 * =================================================================================
 *    @file     logger.h
 *    @brief    
 *
 *  <+DETAILED+>
 *
 *    @author   Roberto Baquerizo (baquerrj), roba8460@colorado.edu
 *
 *    @internal
 *       Created:  03/13/2019
 *      Revision:  none
 *      Compiler:  gcc
 *  Organization:  University of Colorado: Boulder
 *
 *  This source code is released for free distribution under the terms of the
 *  GNU General Public License as published by the Free Software Foundation.
 * =================================================================================
 */


#ifndef  LOGGER_H
#define  LOGGER_H

#include "common.h"


/*
 * =================================================================================
 * Function:       logger_fn
 * @brief   Entry point for logger thread
 *
 * @param   thread_args - void ptr to arguments used to initialize thread
 * @return  NULL  - We don't really exit from this function, 
 *                   since the exit point is thread_exit()
 * =================================================================================
 */
void *logger_fn( void *thread_args );

#endif   /* LOGGER_H */


