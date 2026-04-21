#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in server_addr;

    fd_set read_fds;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        return 1;
    }

    printf("Connected to server.\n");

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(0, &read_fds);     // stdin
        FD_SET(sock, &read_fds);  // socket

        int max_fd = sock;
        select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        // User input
        if (FD_ISSET(0, &read_fds)) {
            char buffer[BUFFER_SIZE];
            if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
                send(sock, buffer, strlen(buffer), 0);
            }
        }

        // Server message
        if (FD_ISSET(sock, &read_fds)) {
            char buffer[BUFFER_SIZE];
            int valread = recv(sock, buffer, BUFFER_SIZE - 1, 0);

            if (valread == 0) {
                printf("Disconnected from server\n");
                break;
            } else if (valread > 0) {
                buffer[valread] = '\0';
                printf("%s", buffer);
            }
        }
    }

    close(sock);
    return 0;
}