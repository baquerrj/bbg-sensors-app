/**
 * =================================================================================
 *    @file     common.h
 *    @brief   Defines types and functions common between the threads of the application
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

#ifndef COMMON_H
#define COMMON_H

#include <signal.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <mqueue.h>
#include <time.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>

#define NUM_THREADS         (5)

#define MSG_SIZE 100
#define MAX_MESSAGES 100

#define MICROS_PER_SEC  1000000

#define FREQ_1HZ        (MICROS_PER_SEC)
#define FREQ_2HZ        (MICROS_PER_SEC/2)
#define FREQ_4HZ        (MICROS_PER_SEC/4)
#define FREQ_HALF_HZ    (MICROS_PER_SEC*2)
#define FREQ_QURT_HZ    (MICROS_PER_SEC*4)

#define ERROR   "[ERROR]\n"
#define INFO    "[INFO]\n"
#define SIGNAL  "[SIGNAL]\n"
#define WARNING "[WARNING]\n"

#define FPRINTF(stream, fmt, ...)   \
   do{ \
      fprintf(stream, "[%s]\t[PID:%d]\t[TID:%ld]", get_timestamp(), getpid(), syscall(SYS_gettid)); \
      fprintf(stream, fmt, ##__VA_ARGS__); \
      fflush( stream ); \
   }while(0)

#if 0
#define SPRINTF(msg, fmt, ...)   \
   do{ \
      snprintf("[PID:%d][TID:%ld]", getpid(), syscall(SYS_gettid)); snprintf(msg, sizeof(msg), fmt, ##__VA_ARGS__);}while(0);
#endif

#define LOG_ERROR(fmt, ...)  \
   do{ \
      FPRINTF( stderr, ERROR fmt, ##__VA_ARGS__ ); \
      fflush( stderr ); \
   }while(0)

#define LOG_INFO(fmt, ...)  \
   do{ \
      FPRINTF( stderr, INFO fmt, ##__VA_ARGS__ ); \
      fflush( stdout ); \
   }while(0)

#define LOG_WARNING(fmt, ...)  \
   do{ \
      FPRINTF( stderr, WARNING fmt, ##__VA_ARGS__ ); \
      fflush( stderr ); \
   }while(0)

/*******************************************************************************
 *  Struct to hold thread identifiers for tasks
 ******************************************************************************/
pthread_t task_id[ NUM_THREADS ];

typedef enum {
   TASK_LOGGER = 0,
   TASK_TMP102,
   TASK_APDS9301,
   TASK_SOCKET,
   TASK_WATCHDOG,
   TASK_MAX
} task_e;


/*! @brief Logging levels */
typedef enum
{
   LOG_ERROR,
   LOG_WARNING,
   LOG_INFO,
   LOG_ALL
} log_level_e;


/*******************************************************************************
 *  Defines types of possible requests from remote clients
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
   REQUEST_STATUS,
   REQUEST_MAX
} request_e;


/*******************************************************************************
 *  Defines types of possible messages
 ******************************************************************************/
typedef enum {
   MSG_BEGIN = 0,
   MSG_LUX,
   MSG_DARK,
   MSG_TEMP,
   MSG_TEMP_C = REQUEST_TEMP,
   MSG_TEMP_K,
   MSG_TEMP_F,
   MSG_CLOSE,
   MSG_KILL,
   MSG_STATUS,
   MSG_ALIVE,
   MSG_MAX
} message_e;

/*******************************************************************************
 *  Defines struct for communicating sensor information
 ******************************************************************************/
typedef struct {
   float data;    /** Can be temperature in Celsius, Fahrenheit, or Kelvin OR
                     lux output from light sensor */
   int   night;   /** 1 when it is dark and 0 otherwise */
} sensor_data_t;

/*******************************************************************************
 *  Defines struct for response for remote socket task
 ******************************************************************************/
typedef struct {
   request_e id;
   char info[MSG_SIZE];
   sensor_data_t data;
} remote_t;


/*! @brief Log Message Structure */
typedef struct
{
   log_level_e level;
   char        timestamp[25];
   message_e   id;
   task_e      src;
   char        msg[MSG_SIZE];
} message_t;


/*******************************************************************************
 *
 ******************************************************************************/
typedef struct {
   char *name;
   FILE *fid;
} file_t;

/*******************************************************************************
 *  Exit Enum
 ******************************************************************************/
typedef enum {
   EXIT_BEGIN = 0,
   EXIT_CLEAN = 0,
   EXIT_INIT,
   EXIT_ERROR,
   EXIT_MAX
} exit_e;



extern const char* const task_name[NUM_THREADS + 1];

/*!
 * @brief 
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 */
static inline const char* get_task_name( task_e task_id )
{
   return task_name[ task_id ];
}


/*!
 * @brief 
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 */
char* get_timestamp( void );


/**
 * =================================================================================
 * Function:       print_header
 * @brief   Write a string formatted with the TID of the thread calling this function
 *          and a timestamp to the log buffer
 *
 * @param   *buffer  - pointer to where we should copy formatted string to
 *                     if NULL, we print to stderr
 * @return  void
 * =================================================================================
 */
void print_header( char *buffer );

/**
 * =================================================================================
 * Function:       thread_exit
 * @brief   Common exit point for all threads
 *
 * @param   exit_status - reason for exit (signal number)
 * @return  void
 * =================================================================================
 */
void thread_exit( int exit_status );

/**
 * =================================================================================
 * Function:       get_shared_memory
 * @brief   Sets up shared memory location for logging
 *
 * @param   void
 * @return  *shm_p - pointer to shared memory object
 * =================================================================================
 */
//void *get_shared_memory( void );

/**
 * =================================================================================
 * Function:       sems_init
 * @brie    Initialize semaphores for shared memory 
 *
 * @param   *shm  - pointer to shared memory object
 * @return  EXIT_CLEAN if successful, otherwise EXIT_INIT
 * =================================================================================
 */
//int sems_init( shared_data_t *shm );

/**
 * =================================================================================
 * Function:       timer_setup
 * @brief   Initializes a timer identified by timer_t id
 *
 * @param   *id   - identifier for new timer
 * @param   *handler - pointer to function to register as the handler for the timer ticks 
 * @return  EXIT_CLEAN if successful, otherwise EXIT_INIT 
 * =================================================================================
 */
int timer_setup( timer_t *id, void (*timer_handler)(union sigval) );


/**
 * =================================================================================
 * Function:       timer_start
 * @brief   Starts the timer with interval usecs
 *
 * @param   *id   - identifier for new timer
 * @param   usecs - timer interval
 * @return  EXIT_CLEAN if successful, otherwise EXIT_INIT 
 * =================================================================================
 */
int timer_start( timer_t *id, unsigned long usecs );


#endif /* COMMON_H */
