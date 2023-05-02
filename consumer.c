#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "buffer.h"

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
        int production_count;
        sscanf(buffer, "Product Type: A, Production Count: %d", &production_count);

        pthread_mutex_lock(&buffer_A.mutex);

        while (buffer_A.count == BUFFER_SIZE_A) {
            pthread_cond_wait(&buffer_A.not_full, &buffer_A.mutex);
        }

        buffer_A.buffer[buffer_A.count++] = production_count;

        pthread_cond_signal(&buffer_A.not_empty);
        pthread_mutex_unlock(&buffer_A.mutex);
    }

    // Close the socket
    close(socket);
    free(arg);

    return NULL;
}

void *consumer_dispatcher(void *arg) {
    int server_sock, new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Create socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
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
            production_count = buffer_A.buffer[--buffer_A.count];
            pthread_cond_signal(&buffer_A.not_full);
            pthread_mutex_unlock(&buffer_A.mutex);
        } else { // product_type == 'B'
            pthread_mutex_lock(&buffer_B.mutex);
            while (buffer_B.count == 0) {
                pthread_cond_wait(&buffer_B.not_empty, &buffer_B.mutex);
            }
            production_count = buffer_B.buffer[--buffer_B.count];
            pthread_cond_signal(&buffer_B.not_full);
            pthread_mutex_unlock(&buffer_B.mutex);
        }

        int local_total_consumption_count;

        pthread_mutex_lock(&total_consumption_mutex);
        local_total_consumption_count = ++total_consumption_count;
        pthread_mutex_unlock(&total_consumption_mutex);

        local_consumption_count = ++consumption_count[thread_index];

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
