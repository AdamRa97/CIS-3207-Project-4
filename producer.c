#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "buffer.h"

// Global Variables
buffer_A_t buffer_A;
buffer_B_t buffer_B;

void *producer(void *arg) {
    char product_type = *(char *)arg;

    // Socket setup
    int sock;
    struct sockaddr_in server_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < PRODUCER_ITERATIONS; i++) {
        char buffer[1024];
        int message_length = snprintf(buffer, sizeof(buffer), "Product Type: %c, Production Count: %d", product_type, i);

        // Send message to the consumer
        send(sock, buffer, message_length, 0);

        usleep(rand() % 100000);
    }

    // Send message terminator to the consumer
    const char *terminator = "END";
    send(sock, terminator, strlen(terminator), 0);

    // Close the socket
    close(sock);

    return NULL;
}