#ifndef BUFFER_H
#define BUFFER_H

#include <pthread.h>

// Constants
#define BUFFER_SIZE_A 10
#define BUFFER_SIZE_B 15
#define PRODUCER_ITERATIONS 150
#define CONSUMER_THREADS 2

// Buffer structures
typedef struct {
    int buffer[BUFFER_SIZE_A];
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} buffer_A_t;

typedef struct {
    int buffer[BUFFER_SIZE_B];
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} buffer_B_t;

// Global variables
extern buffer_A_t buffer_A;
extern buffer_B_t buffer_B;
extern int total_consumption_count;
extern int consumption_count[CONSUMER_THREADS * 2];
extern pthread_mutex_t total_consumption_mutex;
extern pthread_mutex_t file_mutex;

#endif // BUFFER_H