#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    int fd;
    char name[32];
    int active;
} client_t;

client_t clients[MAX_CLIENTS];
int user_count = 0;

void set_nonblocking(int fd) {
    fcntl(fd, F_SETFL, O_NONBLOCK);
}

void broadcast(char *msg, int sender_fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].fd != sender_fd) {
            send(clients[i].fd, msg, strlen(msg), 0);
        }
    }
}

void remove_client(int i) {
    printf("%s disconnected\n", clients[i].name);
    close(clients[i].fd);
    clients[i].active = 0;
}

void add_client(int client_fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].fd = client_fd;
            clients[i].active = 1;

            snprintf(clients[i].name, sizeof(clients[i].name), "User%d", ++user_count);

            printf("%s connected\n", clients[i].name);

            char welcome[BUFFER_SIZE];
            snprintf(welcome, sizeof(welcome), "%s has joined the chat\n", clients[i].name);
            broadcast(welcome, client_fd);
            return;
        }
    }

    char *msg = "Server full\n";
    send(client_fd, msg, strlen(msg), 0);
    close(client_fd);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    fd_set read_fds;

    // Initialize clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].active = 0;
    }

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 5);

    set_nonblocking(server_fd);

    printf("Server started on port %d...\n", PORT);

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);

        int max_fd = server_fd;

        // Add clients to fd set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active) {
                FD_SET(clients[i].fd, &read_fds);
                if (clients[i].fd > max_fd)
                    max_fd = clients[i].fd;
            }
        }

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (activity < 0) {
            perror("select failed");
            exit(EXIT_FAILURE);
        }

        // New connection
        if (FD_ISSET(server_fd, &read_fds)) {
            new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
            set_nonblocking(new_socket);
            add_client(new_socket);
        }

        // Handle client messages
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active && FD_ISSET(clients[i].fd, &read_fds)) {

                char buffer[BUFFER_SIZE];
                int valread = recv(clients[i].fd, buffer, BUFFER_SIZE - 1, 0);

                if (valread == 0) {
                    char leave_msg[BUFFER_SIZE];
                    snprintf(leave_msg, sizeof(leave_msg), "%s has left\n", clients[i].name);
                    broadcast(leave_msg, clients[i].fd);
                    remove_client(i);
                }
                else if (valread > 0) {
                    buffer[valread] = '\0';

                    char msg[BUFFER_SIZE];
                    snprintf(msg, sizeof(msg), "%s: %s", clients[i].name, buffer);

                    printf("%s", msg);
                    broadcast(msg, clients[i].fd);
                }
                // ignore valread < 0
            }
        }
    }

    return 0;
}