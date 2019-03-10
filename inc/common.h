/*
 * =================================================================================
 *    @file     common.h
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

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#define SHM_SEGMENT_NAME "/shm-log"


/******************************************************************************
 *  
 ******************************************************************************/
typedef struct {
   char *name;
   FILE *fid;
} file_t;

/******************************************************************************
 *  
 ******************************************************************************/
typedef struct {
   void *arg1;
   void *arg2;
} args_t;


/******************************************************************************
 *  
 ******************************************************************************/
typedef struct {
   char buffer[2048];
   sem_t w_sem;
   sem_t r_sem;
} shared_data_t;

extern pthread_mutex_t mutex;


/*
 * =================================================================================
 * Function:       print_header
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
extern void print_header( FILE* file );

/*
 * =================================================================================
 * Function:       thread_exit
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
extern void thread_exit( file_t* log, int exit_status );

/*
 * =================================================================================
 * Function:       get_shared_memory
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
extern void *get_shared_memory( void );


/*
 * =================================================================================
 * Function:       sems_init
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
extern int sems_init( shared_data_t *shm );

#endif /* COMMON_H */
