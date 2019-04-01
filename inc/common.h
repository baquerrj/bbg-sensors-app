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
#define SHM_BUFFER_SIZE 2048

#define GEN_BUFFER_SIZE 100
/******************************************************************************
 *  Defines types of requests for remote socket task
 ******************************************************************************/
typedef enum {
   REQUEST_BEGIN = 0,
   REQUEST_LUX,
   REQUEST_DARK,
   REQUEST_TEMP,
   REQUEST_TEMP_C = REQUEST_TEMP,
   REQUEST_TEMP_K,
   REQUEST_TEMP_F,
   REQUEST_CLOSE,
   REQUEST_KILL,
   REQUEST_MAX
} request_e;

/******************************************************************************
 *  Defines struct for requests for remote socket task
 ******************************************************************************/
typedef struct {
   request_e id;
} request_t;

/******************************************************************************
 *  Defines struct for communicating sensor information
 ******************************************************************************/
typedef struct {
   float data;    /* Can be temperature in Celsius, Fahrenheit, or Kelvin OR
                     lux output from light sensor */
   int   night;   /* 1 when it is dark and 0 otherwise */
} sensor_data_t;

/******************************************************************************
 *  Defines struct for response for remote socket task
 ******************************************************************************/
typedef struct {
   request_e id;
   char info[GEN_BUFFER_SIZE];
   sensor_data_t data;
} response_t;

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
   char buffer[SHM_BUFFER_SIZE];  /* Buffer for message from thread */
   char header[SHM_BUFFER_SIZE];  /* Buffer for header identifying the thread who wrote to shm */
   sem_t w_sem;
   sem_t r_sem;
} shared_data_t;



/******************************************************************************
 *  Exit Enum
 ******************************************************************************/
typedef enum {
   EXIT_BEGIN = 0,
   EXIT_CLEAN = 0,
   EXIT_INIT = -1,
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
void print_header( char *buffer );

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
void thread_exit( int exit_status );

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
void *get_shared_memory( void );


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
int sems_init( shared_data_t *shm );

#endif /* COMMON_H */
