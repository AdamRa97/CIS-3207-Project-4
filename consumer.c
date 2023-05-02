#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "buffer.h"

// Constants
#define DEFAULT_PORT 9000

void initialize_mutexes_and_buffers();
void cleanup_mutexes_and_buffers();
void *communication_thread(void *arg);
void *consumer(void *arg);
void *consumer_dispatcher(void *arg);

buffer_A_t buffer_A;
buffer_B_t buffer_B;
int total_consumption_count = 0;
int consumption_count[CONSUMER_THREADS * 2];
pthread_mutex_t file_mutex;
pthread_mutex_t total_consumption_mutex;

int main(int argc, char *argv[]) {
    int server_port = DEFAULT_PORT;

    if (argc > 1) {
        server_port = atoi(argv[1]);
    }

    // Initialize mutexes and buffers
    initialize_mutexes_and_buffers();

    // Create consumer threads
    pthread_t consumer_threads[CONSUMER_THREADS * 2];
    for (int i = 0; i < CONSUMER_THREADS * 2; i++) {
        int *thread_index = (int *) malloc(sizeof(int));
        *thread_index = i;
        pthread_create(&consumer_threads[i], NULL, consumer, (void *)thread_index);
    }

    // Create consumer dispatcher thread
    pthread_t dispatcher_thread;
    int *port_arg = (int *) malloc(sizeof(int));
    *port_arg = server_port;
    pthread_create(&dispatcher_thread, NULL, consumer_dispatcher, (void *)port_arg);

    // Wait for consumer dispatcher thread to finish
    pthread_join(dispatcher_thread, NULL);

    // Wait for consumer threads to finish
    for (int i = 0; i < CONSUMER_THREADS * 2; i++) {
        pthread_join(consumer_threads[i], NULL);
    }

    // Clean up
    cleanup_mutexes_and_buffers();

    return 0;
}

void *communication_thread(void *arg) {
    int socket = *((int *)arg);
    char buffer[1024];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t recv_len = recv(socket, buffer, sizeof(buffer) - 1, 0);
        if (recv_len <= 0) {
            break;
        }

        buffer[recv_len] = '\0';

        if (strcmp(buffer, "END") == 0) {
            break;
        }

        // Process the received message
        char product_type;
        int production_count;
        sscanf(buffer, "Product Type: %c, Production Count: %d", &product_type, &production_count);

        if (product_type == 'A') {
            pthread_mutex_lock(&buffer_A.mutex);

            while (buffer_A.count == BUFFER_SIZE_A) {
                pthread_cond_wait(&buffer_A.not_full, &buffer_A.mutex);
            }

            buffer_B.products[buffer_A.count++] = production_count;

            pthread_cond_signal(&buffer_A.not_empty);
            pthread_mutex_unlock(&buffer_A.mutex);
        } else if (product_type == 'B') {
            pthread_mutex_lock(&buffer_B.mutex);

            while (buffer_B.count == BUFFER_SIZE_B) {
                pthread_cond_wait(&buffer_B.not_full, &buffer_B.mutex);
            }

            buffer_B.products[buffer_B.count++] = production_count;

            pthread_cond_signal(&buffer_B.not_empty);
            pthread_mutex_unlock(&buffer_B.mutex);
        }
    }

    // Close the socket
    close(socket);
    free(arg);

    return NULL;
}

void *consumer_dispatcher(void *arg) {
    int server_port = *((int *)arg);

    int server_sock, new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Create socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket to the address and port
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_sock, 5) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Accept incoming connection
        if ((new_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        int *socket_ptr = (int *)malloc(sizeof(int));
        *socket_ptr = new_sock;

        pthread_t comm_thread;
        pthread_create(&comm_thread, NULL, communication_thread, (void *)socket_ptr);
    }

    // Close the server socket
    close(server_sock);

    return NULL;
}

void *consumer(void *arg) {
    int thread_index = *((int *)arg);
    char product_type = thread_index < CONSUMER_THREADS ? 'A' : 'B';
    int local_consumption_count = 0;

    while (1) {
        int production_count;
        
        if (product_type == 'A') {
            pthread_mutex_lock(&buffer_A.mutex);
            while (buffer_A.count == 0) {
                pthread_cond_wait(&buffer_A.not_empty, &buffer_A.mutex);
            }
            production_count = buffer_A.products[--buffer_A.count];
            pthread_cond_signal(&buffer_A.not_full);
            pthread_mutex_unlock(&buffer_A.mutex);
        } else { // product_type == 'B'
            pthread_mutex_lock(&buffer_B.mutex);
            while (buffer_B.count == 0) {
                pthread_cond_wait(&buffer_B.not_empty, &buffer_B.mutex);
            }
            production_count = buffer_B.products[--buffer_B.count];
            pthread_cond_signal(&buffer_B.not_full);
            pthread_mutex_unlock(&buffer_B.mutex);
        }

        // Increment the local and total consumption counts
        int local_total_consumption_count;
        pthread_mutex_lock(&total_consumption_mutex);
        local_total_consumption_count = ++total_consumption_count;
        pthread_mutex_unlock(&total_consumption_mutex);

        local_consumption_count = ++consumption_count[thread_index];

        // Write to the output file
        pthread_mutex_lock(&file_mutex);
        FILE *output_file = fopen("output.txt", "a");
        pthread_t tid = pthread_self();
        fprintf(output_file, "Product Type: %c, Thread ID: %ld, Production Count: %d, Consumption Seq #: %d, Total Consumption Count: %d\n",
                product_type, (long)tid, production_count, local_consumption_count, local_total_consumption_count);
        fclose(output_file);
        pthread_mutex_unlock(&file_mutex);
    }

    return NULL;
}

void initialize_mutexes_and_buffers() {
    // Initialize buffer A
    buffer_A.count = 0;
    pthread_mutex_init(&buffer_A.mutex, NULL);
    pthread_cond_init(&buffer_A.not_empty, NULL);
    pthread_cond_init(&buffer_A.not_full, NULL);

    // Initialize buffer B
    buffer_B.count = 0;
    pthread_mutex_init(&buffer_B.mutex, NULL);
    pthread_cond_init(&buffer_B.not_empty, NULL);
    pthread_cond_init(&buffer_B.not_full, NULL);

    // Initialize mutexes
    pthread_mutex_init(&total_consumption_mutex, NULL);
    pthread_mutex_init(&file_mutex, NULL);
}

void cleanup_mutexes_and_buffers() {
    // Clean up buffer A
    pthread_mutex_destroy(&buffer_A.mutex);
    pthread_cond_destroy(&buffer_A.not_empty);
    pthread_cond_destroy(&buffer_A.not_full);

    // Clean up buffer B
    pthread_mutex_destroy(&buffer_B.mutex);
    pthread_cond_destroy(&buffer_B.not_empty);
    pthread_cond_destroy(&buffer_B.not_full);

    // Clean up mutexes
    pthread_mutex_destroy(&total_consumption_mutex);
    pthread_mutex_destroy(&file_mutex);
}
