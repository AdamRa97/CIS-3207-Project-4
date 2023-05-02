#ifndef BUFFER_H
#define BUFFER_H

#include <pthread.h>

// Constants
#define PRODUCER_ITERATIONS 150
#define CONSUMER_THREADS 2
#define BUFFER_SIZE 100
#define BUFFER_SIZE_A 50
#define BUFFER_SIZE_B 50
typedef int product_t;


// Buffer structures
typedef struct {
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    product_t products[BUFFER_SIZE_A];
} buffer_A_t;

typedef struct {
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    product_t products[BUFFER_SIZE_B];
} buffer_B_t;


// Global variables
extern int total_consumption_count;
extern int consumption_count[CONSUMER_THREADS * 2];
extern pthread_mutex_t total_consumption_mutex;
extern pthread_mutex_t file_mutex;

#endif // BUFFER_H