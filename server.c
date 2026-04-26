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
    int set_name;
} client_t;

client_t clients[MAX_CLIENTS];
int user_count = 0;

void set_nonblocking(int fd) {
    fcntl(fd, F_SETFL, O_NONBLOCK);
}

int find_client_by_name(char *name) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && strcmp(clients[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

void broadcast(char *msg, int sender_fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].fd != sender_fd) {
            send(clients[i].fd, msg, strlen(msg), 0);
        }
    }
}

void server_message(char *msg, int client_fd) {
    send(client_fd, msg, strlen(msg), 0);
}

void direct_message(char *target_name, char *msg, int sender_index) {
    int target_index = find_client_by_name(target_name);

    if (target_index == -1) {
        char error[BUFFER_SIZE];
        snprintf(error, sizeof(error), "User %s not found\n", target_name);
        send(clients[sender_index].fd, error, strlen(error), 0);
        return;
    }

    char dm[BUFFER_SIZE];
    snprintf(dm, sizeof(dm),
             "[DM from %s]: %s\n",
             clients[sender_index].name,
             msg);

    send(clients[target_index].fd, dm, strlen(dm), 0);
}

void remove_client(int i) {
    printf("%s disconnected\n", clients[i].name);
    close(clients[i].fd);
    clients[i].active = 0;
}

void client_set_name(int id, char * name){
    snprintf(clients[id].name, sizeof(clients[id].name), "%s",name);
    printf("User %d has set name to %s\n", id, name);
    char welcome[BUFFER_SIZE];
    snprintf(welcome, sizeof(welcome), "%s has joined the chat\n", name);
    broadcast(welcome,clients[id].fd);
    clients[id].set_name=1;
}

void add_client(int client_fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].fd = client_fd;
            clients[i].active = 1;

            printf("User %d connected\n", i);
	    char request_name[BUFFER_SIZE];
	    snprintf(request_name, sizeof(request_name), "Please enter username: \n");
	    server_message(request_name,client_fd);
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

        // Add clients
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
            new_socket = accept(server_fd,
                                (struct sockaddr *)&address,
                                (socklen_t *)&addrlen);
            set_nonblocking(new_socket);
            add_client(new_socket);
        }

        // Handle messages
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active &&
                FD_ISSET(clients[i].fd, &read_fds)
		    &&clients[i].set_name) {

                char buffer[BUFFER_SIZE];
                int valread = recv(clients[i].fd,
                                   buffer,
                                   BUFFER_SIZE - 1,
                                   0);

                if (valread == 0) {
                    char leave_msg[BUFFER_SIZE];
                    snprintf(leave_msg, sizeof(leave_msg),
                             "%s has left\n",
                             clients[i].name);
                    broadcast(leave_msg, clients[i].fd);
                    remove_client(i);
                }
                else if (valread > 0) {
                    buffer[valread] = '\0';

                    //Direct message handling
                    if (strncmp(buffer, "/dm ", 4) == 0) {
                        char target[32];
                        char message[BUFFER_SIZE];

                        if (sscanf(buffer + 4,
                                   "%31s %[^\n]",
                                   target,
                                   message) == 2) {
                            direct_message(target, message, i);
                        } else {
                            char *usage =
                                "Usage: /dm <username> <message>\n";
                            send(clients[i].fd,
                                 usage,
                                 strlen(usage),
                                 0);
                        }
                    }
                    //Normal broadcast
                    else {
                        char msg[BUFFER_SIZE];
                        snprintf(msg, sizeof(msg),
                                 "%s: %s",
                                 clients[i].name,
                                 buffer);

                        printf("%s", msg);
                        broadcast(msg, clients[i].fd);
                    }
                }
            }
            else if(clients[i].active&&FD_ISSET(clients[i].fd, &read_fds)
            && !clients[i].set_name)
            {
                char buffer[32];
                int valread= recv(clients[i].fd, buffer, 31, 0);
                if (valread == 0) {
                            char leave_msg[BUFFER_SIZE];
                            snprintf(leave_msg, sizeof(leave_msg),
                                    "Unnamed user has left\n");
                            broadcast(leave_msg, clients[i].fd);
                            remove_client(i);
                        }
                        else if (valread > 0) {
                            buffer[valread-1] = '\0';
                    char name[32];
                    snprintf(name, sizeof(name), "%s",buffer);
                    if(strlen(name)>0)
                        client_set_name(i, name);
                }
            }
        }
    }

    return 0;
}
