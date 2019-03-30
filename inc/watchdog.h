/*
 * =================================================================================
 *    @file     watchdog.h
 *    @brief    
 *
 *  <+DETAILED+>
 *
 *    @author   Roberto Baquerizo (baquerrj), roba8460@colorado.edu
 *
 *    @internal
 *       Created:  03/15/2019
 *      Revision:  none
 *      Compiler:  gcc
 *  Organization:  University of Colorado: Boulder
 *
 *  This source code is released for free distribution under the terms of the
 *  GNU General Public License as published by the Free Software Foundation.
 * =================================================================================
 */


#ifndef  WATCHDOG_H
#define  WATCHDOG_H

#include <pthread.h>




/*
 * =================================================================================
 * Function:       check_threads
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
int check_threads( void );

/*
 * =================================================================================
 * Function:       watchdog_init
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
int watchdog_init( pthread_t *threads );

/*
 * =================================================================================
 * Function:       watchdog_fn
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
void *watchdog_fn( void *thread_args );

#endif   /* WATCHDOG_H */
