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

#define LOGGER_QUEUE_NAME  "/logger_queue"

#define LOG_MSG( fp, fmt, ...)  \
   do{ \
      FPRINTF( fp, fmt, ##__VA_ARGS__); \
      fflush( fp ); \
      }while(0)

#define LOG_TASK_MSG(p_logstruct, fmt, ...)  \
    do{ \
        snprintf((p_logstruct)->msg,sizeof((p_logstruct)->msg),fmt, ##__VA_ARGS__);   \
        convert_timestamp((p_logstruct)->timestamp, sizeof((p_logstruct)->timestamp)); \
        log_msg( get_logger_queue(), p_logstruct, sizeof(*p_logstruct), 20); \
    }while(0)

/*!
 * @brief 
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 */
uint8_t log_msg( mqd_t queue, const message_t *msg, size_t size, int priority );
//static inline uint8_t log_msg( mqd_t queue, const message_t *msg, size_t size, int priority )
//{
//   if( -1 == mq_send( queue, (const char*)msg, size, priority ) )
//   {
//      int errnum = errno;
//      LOG_ERROR( "LOGGER - QUEUE SEND (%s)\n", strerror( errnum ) );
//      return EXIT_ERROR;
//   }
 //  return EXIT_CLEAN;
//}

/*!
 * @brief 
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 */
mqd_t get_logger_queue( void );



/*!
 * @brief 
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 */
void logger_cycle( void );

/*!
 * @brief 
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 */
mqd_t logger_queue_init( void );

/*!
 *
 * @brief Get timestamp
 *
 * @param   timestamp store to write timestamp to
 * @param   len size of string to write in bytes
 * @returns EXIT_CLEAN if successful
 * @returns EXIT_ERROR otherwise 
 */
uint8_t convert_timestamp( char *timestamp, const int len );


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


