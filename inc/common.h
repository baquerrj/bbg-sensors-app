#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#define SHM_SEGMENT_NAME "/shm-log"

typedef struct file_s {
   char *name;
   FILE *fid;
} file_t;

typedef struct args_s {
   void *arg1;
   void *arg2;
} args_t;

typedef struct {
   char buffer[2048];
   sem_t w_sem;
   sem_t r_sem;
} shared_data_t;

extern int shm_fd;


extern pthread_mutex_t mutex;

extern void print_header( FILE* file );
extern void thread_exit( file_t* log, int exit_status );
extern void *get_shared_memory( void );
extern int sems_init( shared_data_t *shm );

#endif /* COMMON_H */
