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
#define BUFFER_SIZE 2048

/******************************************************************************
 *  
 ******************************************************************************/
typedef struct {
   char *name;
   FILE *fid;
} file_t;

/******************************************************************************
 *  Struct to hold arguments passed to threads
 ******************************************************************************/
typedef struct thread_id_s {
   pthread_t t1;
   pthread_t t2;
} thread_id_s;


/******************************************************************************
 *  Shared Memory Data Struct
 ******************************************************************************/
typedef struct {
   char buffer[BUFFER_SIZE];  /* Buffer for message from thread */
   char header[BUFFER_SIZE];  /* Buffer for header identifying the thread who wrote to shm */
   sem_t w_sem;
   sem_t r_sem;

} shared_data_t;



/******************************************************************************
 *  Exit Enum
 ******************************************************************************/
typedef enum {
   EXIT_CLEAN = 0,
   EXIT_ERROR,
   EXIT_MAX
} exit_e;

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
extern void print_header( char *buffer );

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
extern void thread_exit( int exit_status );

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
