#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define DEFAULT_PORT 9000
#define TOTAL_PRODUCTS 150

int main(int argc, char *argv[]) {
    int server_port = DEFAULT_PORT;

    if (argc > 1) {
        server_port = atoi(argv[1]);
    }

    // Set up socket
    int sock;
    struct sockaddr_in server_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Seed random number generator
    srand(time(NULL));

    // Produce products and send them to the server
    for (int i = 0; i < TOTAL_PRODUCTS; i++) {
        char product_type = rand() % 2 == 0 ? 'A' : 'B';
        int production_count = rand() % 100 + 1;

        char buffer[1024];
        snprintf(buffer, sizeof(buffer), "Product Type: %c, Production Count: %d", product_type, production_count);

        send(sock, buffer, strlen(buffer), 0);
        usleep(100000); // Sleep for 100ms
    }

    // Send END message to server
    send(sock, "END", 4, 0);

    // Close socket
    close(sock);

    return 0;
}