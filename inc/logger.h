/*!
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
 */


#ifndef  LOGGER_H
#define  LOGGER_H

#include "common.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <mqueue.h>
#include <stdint.h>

#include <unistd.h>
#include <stdio.h>
#include <sys/syscall.h>
mqd_t logger_queue;

#define LOGGER_QUEUE_NAME  "/logger_queue"

#define MSG_SIZE  GEN_BUFFER_SIZE

#if 0
#define LOG_TASK(logger, fmt, ...)  do{SPRINTF((logger)->msg, sizeof((logger)->msg), fmt, ##__VA_ARGS__); \
                                          convert_timestamp(logger); \
                                          POST_MSG( logger_queue, logger, sizeof(*logger), 20 ); \
                                          }while(0)
#endif
#define LOG_TASK_MSG(p_logstruct, fmt, ...)  \
    do{ \
        snprintf((p_logstruct)->msg,sizeof((p_logstruct)->msg),fmt, ##__VA_ARGS__);   \
        convert_timestamp(p_logstruct); \
        log_msg( logger_queue, p_logstruct, sizeof(*p_logstruct), 20); \
    }while(0)



/*! @brief Types of Logging */
typedef enum
{
   MSG_BEGIN = 0,
   NMSG_STATUS,
   MSG_TOGGLE_LED,
   MSG_GET_TEMP,
   MSG_TEMP_LOW,
   MSG_TEMP_HIGH,
   MSG_TEMP_MAX
} log_msg_e;

/*! @brief Logging levels */
typedef enum
{
   LOG_ERROR,
   LOG_WARNING,
   LOG_INFO,
   LOG_ALL
} log_level_e;

/*! @brief Packet structure */
typedef struct
{
   log_level_e level;
   char        timestamp[25];
   log_msg_e   type;
   task_e   src;
   char        msg[MSG_SIZE];
} log_msg_t;



/*!
 * @brief 
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 */
static inline uint8_t log_msg( mqd_t queue, const log_msg_t *msg, size_t size, int priority )
{
   if( -1 == mq_send( queue, (const char*)msg, size, priority ) )
   {
      int errnum = errno;
      LOG_ERROR( "LOGGER - QUEUE SEND (%s)\n", strerror( errnum ) );
      return EXIT_ERROR;
   }
   return EXIT_CLEAN;
}



/*!
 * @brief 
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 */
mqd_t logger_queue_init( void );



/*!
 * @brief Get timestamp
 *
 * @param   timestamp store to write timestamp to
 * @param   len size of string to write in bytes
 * @returns EXIT_CLEAN if successful
 * @returns EXIT_ERROR otherwise 
 */
uint8_t convert_timestamp( char *timestamp, const int len );


/*!
 * @brief 
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 */
mqd_t get_logger_queue( void );


/*!
 * Function:       logger_fn
 * @brief   Entry point for logger thread
 *
 * @param   thread_args - void ptr to arguments used to initialize thread
 * @return  NULL  - We don't really exit from this function, 
 *                   since the exit point is thread_exit()
 */
void *logger_fn( void *thread_args );

#endif   /* LOGGER_H */


