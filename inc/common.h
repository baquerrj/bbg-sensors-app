#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>

typedef struct file_s {
   char *name;
   FILE *fid;
} file_t;

typedef struct args_s {
   void *arg1;
   void *arg2;
} args_t;

extern pthread_mutex_t mutex;

extern void print_header( FILE* file );
extern void thread_exit( file_t* log, int exit_status );

#endif /* COMMON_H */
