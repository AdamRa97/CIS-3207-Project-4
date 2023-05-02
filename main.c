#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "buffer.h"

// Constants
#define DEFAULT_PORT 9000
#define TOTAL_PRODUCTS 150

// Global variables
int total_consumption_count = 0;
int consumption_count[CONSUMER_THREADS * 2];
pthread_mutex_t total_consumption_mutex;
pthread_mutex_t file_mutex;

// Function prototypes
void *producer(void *arg);
void *consumer(void *arg);

int main(int argc, char *argv[]) {
    pthread_t producer_threads_A;
    pthread_t producer_threads_B;
    char product_type_A = 'A';
    char product_type_B = 'B';

    // Initialize mutex
    pthread_mutex_init(&file_mutex, NULL);
    pthread_mutex_init(&total_consumption_mutex, NULL);

    buffer_A.count = 0;
    pthread_mutex_init(&buffer_A.mutex, NULL);
    pthread_cond_init(&buffer_A.not_empty, NULL);
    pthread_cond_init(&buffer_A.not_full, NULL);

    buffer_B.count = 0;
    pthread_mutex_init(&buffer_B.mutex, NULL);
    pthread_cond_init(&buffer_B.not_empty, NULL);
    pthread_cond_init(&buffer_B.not_full, NULL);

    // Create producer threads
    pthread_create(&producer_threads_A, NULL, producer, (void *)&product_type_A);
    pthread_create(&producer_threads_B, NULL, producer, (void *)&product_type_B);

    // Create consumer threads
    pthread_t consumer_threads[CONSUMER_THREADS * 2];
    for (int i = 0; i < CONSUMER_THREADS * 2; i++) {
        int *thread_index = (int *) malloc(sizeof(int));
        *thread_index = i;
        pthread_create(&consumer_threads[i], NULL, consumer, (void *)thread_index);
    }

    // Wait for producer threads to finish
    pthread_join(producer_threads_A, NULL);
    pthread_join(producer_threads_B, NULL);

    // Wait for consumer threads to finish
    for (int i = 0; i < CONSUMER_THREADS * 2; i++) {
        pthread_join(consumer_threads[i], NULL);
    }

    // Clean up
    pthread_mutex_destroy(&file_mutex);
    pthread_mutex_destroy(&total_consumption_mutex);
    pthread_cond_destroy(&buffer_A.not_empty);
    pthread_cond_destroy(&buffer_A.not_full);
    pthread_mutex_destroy(&buffer_A.mutex);

    pthread_cond_destroy(&buffer_B.not_empty);
    pthread_cond_destroy(&buffer_B.not_full);
    pthread_mutex_destroy(&buffer_B.mutex);

    return 0;
}