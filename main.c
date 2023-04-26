#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

// Constants
#define BUFFER_SIZE_A 10
#define BUFFER_SIZE_B 15
#define PRODUCER_ITERATIONS 150
#define CONSUMER_THREADS 2
#define TOTAL_PRODUCTS 150

// Buffer structures
typedef struct{
    int buffer[BUFFER_SIZE_A];
    int count;
    pthread_mutex_t mutex;
} buffer_A_t;

typedef struct{
    int buffer[BUFFER_SIZE_B];
    int count;
    pthread_mutex_t mutex;
} buffer_B_t;

// Global variables
buffer_A_t buffer_A;
buffer_B_t buffer_B;
int total_consumption_count = 0;
int consumption_count[CONSUMER_THREADS * 2] = {0};
int producer_finished = 0;
int distributor_finished = 0;
pthread_mutex_t total_consumption_mutex;
pthread_mutex_t file_mutex;
pthread_mutex_t total_production_mutex = PTHREAD_MUTEX_INITIALIZER;
int total_production_count = 0;
pthread_cond_t buffer_A_not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t buffer_A_not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t buffer_B_not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t buffer_B_not_full = PTHREAD_COND_INITIALIZER;

// Function prototypes
void *producer(void *arg);
void *distributor(void *arg);
void *consumer_A(void *arg);
void *consumer_B(void *arg);

int main(){
    int i;
    pthread_t producer_thread;
    pthread_t distributor_thread;
    pthread_t consumer_threads_A[CONSUMER_THREADS];
    pthread_t consumer_threads_B[CONSUMER_THREADS];
    int thread_index[4];

    // Initialize buffer A
    buffer_A.count = 0;
    pthread_mutex_init(&buffer_A.mutex, NULL);

    // Initialize buffer B
    buffer_B.count = 0;
    pthread_mutex_init(&buffer_B.mutex, NULL);

    // Initialize mutex and condition variables
    pthread_mutex_init(&file_mutex, NULL);
    pthread_mutex_init(&total_consumption_mutex, NULL);

    // Create producer thread
    pthread_create(&producer_thread, NULL, producer, NULL);

    // Create distributor thread
    pthread_create(&distributor_thread, NULL, distributor, NULL);

    // Create consumer threads
    for (i = 0; i < CONSUMER_THREADS; i++){
        thread_index[i] = i;
        pthread_create(&consumer_threads_A[i], NULL, consumer_A, (void *)&thread_index[i]);
    }

    for (i = 0; i < CONSUMER_THREADS; i++){
        thread_index[i] = i;
        pthread_create(&consumer_threads_B[i], NULL, consumer_B, (void *)&thread_index[i]);
    }

    // Wait for producer thread to finish
    pthread_join(producer_thread, NULL);

    // Wait for distributor thread to finish
    pthread_join(distributor_thread, NULL);

    // Wait for consumer threads to finish
    for (i = 0; i < CONSUMER_THREADS; i++){
        pthread_join(consumer_threads_A[i], NULL);
    }

    for (i = 0; i < CONSUMER_THREADS; i++){
        pthread_join(consumer_threads_B[i], NULL);
    }

    // Clean up
    pthread_mutex_destroy(&buffer_A.mutex);
    pthread_mutex_destroy(&buffer_B.mutex);
    pthread_mutex_destroy(&file_mutex);
    pthread_mutex_destroy(&total_consumption_mutex);
    pthread_cond_destroy(&buffer_A_not_empty);
    pthread_cond_destroy(&buffer_A_not_full);
    pthread_cond_destroy(&buffer_B_not_empty);
    pthread_cond_destroy(&buffer_B_not_full);

    return 0;
}

void *producer(void *arg){
    for (int i = 0; i < PRODUCER_ITERATIONS; i++){
        pthread_mutex_lock(&buffer_A.mutex);

        while (buffer_A.count == BUFFER_SIZE_A){
            pthread_cond_wait(&buffer_A_not_full, &buffer_A.mutex);
        }

        buffer_A.buffer[buffer_A.count++] = i;

        pthread_cond_signal(&buffer_A_not_empty);
        pthread_mutex_unlock(&buffer_A.mutex);
        usleep(rand() % 100000);
    }

    producer_finished = 1;
    pthread_cond_broadcast(&buffer_A_not_empty);

    return NULL;
}

void *distributor(void *arg){
    while (1){
        pthread_mutex_lock(&buffer_A.mutex);

        while (!producer_finished && buffer_A.count == 0){
            pthread_cond_wait(&buffer_A_not_empty, &buffer_A.mutex);
        }

        if (producer_finished && buffer_A.count == 0){
            pthread_mutex_unlock(&buffer_A.mutex);
            break;
        }

        int item = buffer_A.buffer[--buffer_A.count];

        pthread_cond_signal(&buffer_A_not_full);
        pthread_mutex_unlock(&buffer_A.mutex);

        pthread_mutex_lock(&buffer_B.mutex);

        while (buffer_B.count == BUFFER_SIZE_B){
            pthread_cond_wait(&buffer_B_not_full, &buffer_B.mutex);
        }

        buffer_B.buffer[buffer_B.count++] = item;

        pthread_cond_signal(&buffer_B_not_empty);
        pthread_mutex_unlock(&buffer_B.mutex);
        usleep(rand() % 100000);
    }

    distributor_finished = 1;
    pthread_cond_broadcast(&buffer_B_not_empty);

    return NULL;
}

void *consumer_A(void *arg){
    int thread_index = *((int *)arg);

    while (!producer_finished || buffer_A.count > 0){
        pthread_mutex_lock(&buffer_A.mutex);

        while (!producer_finished && buffer_A.count == 0){
            pthread_cond_wait(&buffer_A_not_empty, &buffer_A.mutex);
        }

        if (producer_finished && buffer_A.count == 0){
            pthread_mutex_unlock(&buffer_A.mutex);
            break;
        }

        int production_count = buffer_A.buffer[--buffer_A.count];

        pthread_cond_signal(&buffer_A_not_full);
        pthread_mutex_unlock(&buffer_A.mutex);

        int local_total_consumption_count;
        int local_consumption_count;

        pthread_mutex_lock(&total_consumption_mutex);
        local_total_consumption_count = ++total_consumption_count;
        pthread_mutex_unlock(&total_consumption_mutex);

        local_consumption_count = ++consumption_count[thread_index];

        pthread_mutex_lock(&file_mutex);
        FILE *output_file = fopen("output.txt", "a");
        pthread_t tid = pthread_self();
        fprintf(output_file, "Product Type: A, Thread ID: %ld, Production Count: %d, Consumption Seq #: %d, Total Consumption Count: %d\n",
                (long)tid, production_count, local_consumption_count, local_total_consumption_count);
        fclose(output_file);
        pthread_mutex_unlock(&file_mutex);
    }

    return NULL;
}

void *consumer_B(void *arg){
    int thread_index = *((int *)arg) + CONSUMER_THREADS;

    while (!distributor_finished || buffer_B.count > 0){
        pthread_mutex_lock(&buffer_B.mutex);

        while (!distributor_finished && buffer_B.count == 0){
            pthread_cond_wait(&buffer_B_not_empty, &buffer_B.mutex);
        }

        if (distributor_finished && buffer_B.count == 0){
            pthread_mutex_unlock(&buffer_B.mutex);
            break;
        }

        int production_count = buffer_B.buffer[--buffer_B.count];

        pthread_cond_signal(&buffer_B_not_full);
        pthread_mutex_unlock(&buffer_B.mutex);

        int local_total_consumption_count;
        int local_consumption_count;

        pthread_mutex_lock(&total_consumption_mutex);
        local_total_consumption_count = ++total_consumption_count;
        pthread_mutex_unlock(&total_consumption_mutex);

        local_consumption_count = ++consumption_count[thread_index];

        pthread_mutex_lock(&file_mutex);
        FILE *output_file = fopen("output.txt", "a");
        pthread_t tid = pthread_self();
        fprintf(output_file, "Product Type: B, Thread ID: %ld, Production Count: %d, Consumption Seq #: %d, Total Consumption Count: %d\n",
                (long)tid, production_count, local_consumption_count, local_total_consumption_count);
        fclose(output_file);
        pthread_mutex_unlock(&file_mutex);
    }

    return NULL;
}