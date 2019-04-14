/*!
 *    @file     socket.c
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
 */

#include "socket.h"
#include "light.h"
#include "temperature.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080


/*!
 * Function:       process_request
 * @brief   Process a request from remote client
 *
 * @param   *request - request from client
 * @return  response - our response
 */
remote_t process_request( remote_t *request )
{
   remote_t response = {0};
   switch( request->id )
   {
      case REQUEST_LUX:
         response.id = request->id;
         response.data.data = get_lux();
         LOG_INFO( "Request Lux: %.5f\n",
                  response.data.data );
         break;
      case REQUEST_DARK:
         response.id = request->id;
         response.data.night = is_dark();
         LOG_INFO( "Request Day or Night: %s\n",
                  (response.data.night == 0) ? "day" : "night");
         break;
      case REQUEST_TEMP:
         response.id = request->id;
         response.data.data = get_temperature();
         LOG_INFO( "Request Temperature: %.5f C\n",
                  response.data.data );
         break;
      case REQUEST_TEMP_K:
         response.id = request->id;
         response.data.data = get_temperature() + 273.15;
         LOG_INFO( "Request Temperature: %.5f K\n",
                  response.data.data );
         break;
       case REQUEST_TEMP_F:
         response.id = request->id;
         response.data.data = (get_temperature() *1.80) + 32.0;
         LOG_INFO( "Request Temperature: %.5f F\n",
                  response.data.data );
         break;
      case REQUEST_CLOSE:
         response.id = request->id;
         LOG_INFO( "Request Close Connection\n" );
         break;
      case REQUEST_KILL:
         response.id = request->id;
         LOG_INFO( "Request Kill Application\n" );
         break;
      default:
        LOG_INFO( "Invalid Request\n" );
         break;
   }

   return response;
}

/*!
 * Function:       cycle
 * @brief   Cycle function for remote socket task. Spins in this infinite while-loop
 *          checking for new connections to make. When it receives a new connection,
 *          it starts processing requests from the client
 *
 * @param   server   - server socket file descriptor
 * @return  void
 */
static void cycle( int server )
{
   const char *client_info;
   int client = 1;
   char ip[20] = {0};
   struct sockaddr_in addr;
   int addrlen = sizeof( addr );

   while( 1 )
   {
      int kill = 0;
      client = accept( server, (struct sockaddr*)&addr, (socklen_t*)&addrlen );
      if( 0 > client )
      {
         int errnum = errno;

         LOG_ERROR( "Could not accept new connection (%s)\n",
                  strerror( errnum ) );
         continue;
      }

      client_info = inet_ntop( AF_INET, &addr.sin_addr, ip, sizeof( ip ) );
      LOG_INFO( "New connection accepted: %s\n", client_info );

      while( 1 )
      {
         remote_t request = {0};
         remote_t response = {0};
         int bytes = 0;
         while( ( -1 != bytes ) && ( sizeof( request ) > bytes ) )
         {
            bytes = recv( client, ((char*)&request + bytes), sizeof( request ), 0 );
         }

         response = process_request( &request );

         if( REQUEST_CLOSE == response.id )
         {
            break;
         }
         if( REQUEST_KILL == response.id )
         {
            kill = 1;
            break;
         }

         /* Send out response to client */
         bytes = send( client, (char*)&response, sizeof( response ), 0 );
         if( sizeof( response ) > bytes )
         {
            if( -1 == bytes )
            {
               int errnum = errno;
               LOG_ERROR( "Encountered error sending data to client: (%s)\n",
                        strerror( errnum ) );
               break;
            }
            else
            {
               LOG_ERROR( "Could not transmit all data: %u out of %u bytes sent.\n",
                        bytes, sizeof( response ) );
               break;
            }
         }
         LOG_INFO( "%u out of %u bytes sent.\n",
                  bytes, sizeof( response ) );
      }

      client_info = inet_ntop( AF_INET, &addr.sin_addr, ip, sizeof( ip ) );
      close( client );
      LOG_INFO( "Client connection closed: %s\n", client_info );

      if( 1 == kill )
      {
         close( server );
         LOG_INFO( "Closed server.\n" );
         break;
      }
   }
   return;
}


/*!
 * Function:       socket_init
 * @brief   Initliaze the server socket
 *
 * @param   void
 * @return  server - file descriptor for newly created socket for server
 */
int socket_init( void )
{
   int retVal = 0;
   int opt = 1;
   struct sockaddr_in addr;

   int server = socket( AF_INET, SOCK_STREAM, 0 );
   if( 0 == server )
   {
      int errnum = errno;
      LOG_ERROR( "Encountered error creating new socket (%s)\n",
               strerror( errnum ) );
      return -1;
   }

   retVal = setsockopt( server, SOL_SOCKET, SO_REUSEPORT | SO_REUSEADDR, &(opt), sizeof(opt) );
   if( 0 != retVal )
   {
      int errnum = errno;
      LOG_ERROR( "Encountered error setting socket options (%s)\n",
               strerror( errnum ) );
      return -1;
   }

   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = INADDR_ANY;
   addr.sin_port = htons( PORT );

   /* Attempt to bind socket to address */
   retVal = bind( server, (struct sockaddr*)&addr, sizeof( addr ) );
   if( 0 > retVal )
   {
      int errnum = errno;
      LOG_ERROR( "Encountered error binding the new socket (%s)\n",
               strerror( errnum ) );
      return -1;
   }

   /* Try to listen */
   retVal = listen( server, 10 );
   if( 0 > retVal )
   {
      int errnum = errno;
      LOG_ERROR( "Encountered error listening with new socket (%s)\n",
               strerror( errnum ) );
      return -1;
   }

   LOG_INFO( "SOCKET TASK: SERVER ID [%d]\n", server );

   return server;
}


/*!
 * Function:       socket_fn
 * @brief   Entry point for remote socket thread
 *
 * @param   *thread_args   - thread arguments (if any)
 * @return  NULL  - We don't really exit from this function,
 *                   since the exit point is thread_exit()
 */
void *socket_fn( void *thread_args )
{
   /* Get time that thread was spawned */
   struct timespec time;
   clock_gettime(CLOCK_REALTIME, &time);

   int server = socket_init();
   if( -1 == server )
   {
      LOG_ERROR( "SOCKET TASK INIT\n" );
      thread_exit( EXIT_INIT );
   }

   LOG_INFO( "SOCKET TASK: Hello World!\n" );

   cycle( server );

   thread_exit( EXIT_CLEAN );
   return NULL;
}



