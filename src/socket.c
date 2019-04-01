/*
 * =================================================================================
 *    @file     socket.c
 *    @brief    
 *
 *  <+DETAILED+>
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

#include "socket.h"
#include "common.h"


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

static shared_data_t *shm;

/*
 * =================================================================================
 * Function:       process_request
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
msg_t process_request( msg_t *request )
{
   msg_t response = {0};
   response.id = request->id;
   return response;
}

/*
 * =================================================================================
 * Function:       socket_cycle
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
static void cycle( int server )
{
   const char *client_info;
   int client = 1;
   char ip[20] = {0};
   struct sockaddr_in addr;
   int addrlen = sizeof( addr );

   /* Buffer to copy status to shared memory for logger */
   //char *status;
   while( 1 )
   {
      int kill = 0;
      client = accept( server, (struct sockaddr*)&addr, (socklen_t*)&addrlen );
      if( 0 > client )
      {
         int errnum = errno;
         fprintf( stderr, "Could not accept new connection (%s)\n",
                  strerror( errnum ) );
         continue;
      }

      client_info = inet_ntop( AF_INET, &addr.sin_addr, ip, sizeof( ip ) );
      fprintf( stdout, "New connection accepted: %s\n", client_info );

      while( 1 )
      {
         msg_t request = {0};
         msg_t response = {0};
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
               fprintf( stderr, "Encountered error sending data to client: (%s)\n",
                        strerror( errnum ) );
               break;
            }
            else
            {
               fprintf( stderr, "Could not transmit all data: %u out of %u bytes sent.\n",
                        bytes, sizeof( response ) );
               break;
            }
         }
         fprintf( stdout, "%u out of %u bytes sent.\n",
                  bytes, sizeof( response ) );
      }
      
      client_info = inet_ntop( AF_INET, &addr.sin_addr, ip, sizeof( ip ) );
      close( client );
      fprintf( stdout, "Client connection closed: %s\n", client_info );

      if( 1 == kill )
      {
         close( server );
         fprintf( stdout, "Closed server.\n" );
         break;
      }
   }
   return;
}



/*
 * =================================================================================
 * Function:       socket_init
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
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
      fprintf( stderr, "Encountered error creating new socket (%s)\n",
               strerror( errnum ) );
      return -1;
   }

   retVal = setsockopt( server, SOL_SOCKET, SO_REUSEPORT | SO_REUSEADDR, &(opt), sizeof(opt) );
   if( 0 != retVal )
   {
      int errnum = errno;
      fprintf( stderr, "Encountered error setting socket options (%s)\n",
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
      fprintf( stderr, "Encountered error binding the new socket (%s)\n",
               strerror( errnum ) );
      return -1;
   }

   /* Try to listen */
   retVal = listen( server, 10 );
   if( 0 > retVal )
   {
      int errnum = errno;
      fprintf( stderr, "Encountered error listening with new socket (%s)\n",
               strerror( errnum ) );
      return -1;
   }

   fprintf( stdout, "Created new socket [%d]!\n", server );

   return server;
}


/*
 * =================================================================================
 * Function:       socket_fn
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
void *socket_fn( void *thread_args )
{
   /* Get time that thread was spawned */
   struct timespec time;
   clock_gettime(CLOCK_REALTIME, &time);
   
   int server = socket_init();
   if( -1 == server )
   {
      fprintf( stderr, "Failed to set up server!\n" );
      thread_exit( EXIT_INIT );
   }

   /* Get pointer to shared memory struct */
   shm = get_shared_memory();

   /* Write initial state to shared memory */
   sem_wait(&shm->w_sem);
   print_header(shm->buffer);
   fprintf( stdout, "Hello World! Start Time: %ld.%ld secs\n",
            time.tv_sec, time.tv_nsec );
   sprintf( shm->buffer, "Hello World! Start Time: %ld.%ld secs\n",
            time.tv_sec, time.tv_nsec );
   /* Signal to logger that shared memory has been updated */
   sem_post(&shm->r_sem);

   cycle( server );

   thread_exit( EXIT_CLEAN );
   return NULL;
}



