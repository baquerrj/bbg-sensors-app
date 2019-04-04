/**
 * =================================================================================
 *    @file     socket.h
 *    @brief   Remote Socket task capable of requesting sensor readings from
 *             temperature and light sensor threads
 *
 *    @author   Roberto Baquerizo (baquerrj), roba8460@colorado.edu
 *
 *    @internal
 *       Created:  03/31/2019
 *      Revision:  none
 *      Compiler:  gcc
 *  Organization:  University of Colorado: Boulder
 *
 *  This source code is released for free distribution under the terms of the
 *  GNU General Public License as published by the Free Software Foundation.
 * =================================================================================
 */


#ifndef  SOCKET_H
#define  SOCKET_H


#include "common.h"

/**
 * =================================================================================
 * Function:       process_request
 * @brief   Process a request from remote client
 *
 * @param   *request - request from client
 * @return  response - our response 
 * =================================================================================
 */
msg_t process_request( msg_t *request );

/**
 *
 * =================================================================================
 * Function:       cycle
 * @brief   Cycle function for remote socket task. Spins in this infinite while-loop
 *          checking for new connections to make. When it receives a new connection,
 *          it starts processing requests from the client
 *
 * @param   server   - server socket file descriptor
 * @return  void
 * =================================================================================
 */
int socket_init( void );

/**
 * =================================================================================
 * Function:       socket_fn
 * @brief   Entry point for remote socket thread
 *
 * @param   *thread_args   - thread arguments (if any) 
 * @return  NULL  - We don't really exit from this function, 
 *                   since the exit point is thread_exit()
 * =================================================================================
 */
void *socket_fn( void *thread_arg );


#endif   /* SOCKET_H */
