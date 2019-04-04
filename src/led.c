/**
 * =================================================================================
 *    @file     led.c
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

#include "led.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>



/**
 * =================================================================================
 * Function:       get_status
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
void get_status( const char *led )
{
   return;
}

/**
 * =================================================================================
 * Function:       get_trigger
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
int set_trigger( const char *led, char* trigger )
{
   FILE *fp = fopen( led, "w+" );
   if( NULL == fp )
   {
      int errnum = errno;
      fprintf( stderr, "Encountered error trying to set trigger %s for %s (%s)\n",
               trigger, led, strerror ( errnum ) );
      return -1;
   }
   fprintf( fp, "%s", trigger );
   fclose( fp );
   return 0;
}

/**
 * =================================================================================
 * Function:       set_delay
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
int set_delay( const char *led, int delay )
{
   FILE *fp = fopen( led, "w+" );
   if( NULL == fp )
   {
      int errnum = errno;
      fprintf( stderr, "Encuntered error trying to set delay for %s (%s)\nAre you sure LED is in correct configuration?\n",
               led, strerror ( errnum ) );
      return -1;
   }
   fprintf( fp, "%u", delay );
   fclose( fp );
   return delay;
}

/**
 * =================================================================================
 * Function:       led_on
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
void led_on( const char* led )
{
   FILE *fp;
   fp = fopen( led, "w+" );
   fprintf( fp, "1" );
   fclose( fp );
   return;
}


/**
 * =================================================================================
 * Function:       led_off
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
void led_off( const char* led )
{
   FILE *fp;
   fp = fopen( led, "w+" );
   fprintf( fp, "0" );
   fclose( fp );
   return;
}

/**
 * =================================================================================
 * Function:       led_toggle
 * @brief  
 *
 * @param  <+NAME+> <+DESCRIPTION+>
 * @return <+DESCRIPTION+>
 * <+DETAILED+>
 * =================================================================================
 */
void led_toggle( const char* led )
{
   FILE *fp;
   fp = fopen( led, "rt" );
   fseek( fp, 0, SEEK_END );
   long size = ftell( fp );
   rewind( fp );

   char *value = (char*) malloc( sizeof(char) * size );
   fread( value, 1, size, fp );
   fclose( fp );
   switch( *value )
   {
      case '0':
         led_on( led );
         break;
      case '1':
         led_off( led );
         break;
      default:
         break;
   }
   return;
}


