// server.c - a small server that monitors sockets with select() to accept connection requests and relay client messages
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 4242 // our server's port

int create_server_socket(void);
void accept_new_connection(int listener_socket, fd_set *all_sockets, int *fd_max);
void read_data_from_socket(int socket, fd_set *all_sockets, int fd_max, int server_socket);

int main(void)
{
    printf("---- SERVER ----\n\n");

    int server_socket;
    int status;

    // To monitor socket fds:
    fd_set all_sockets; // Set for all sockets connected to server
    fd_set read_fds;    // Temporary set for select()
    int fd_max;         // Highest socket fd
    struct timeval timer;

    // Create server socket
    server_socket = create_server_socket();
    if (server_socket == -1)
    {
        return (1);
    }

    // Listen to port via socket
    printf("[Server] Listening on port %d\n", PORT);
    status = listen(server_socket, 10);
    if (status != 0)
    {
        fprintf(stderr, "[Server] Listen error: %s\n", strerror(errno));
        return (3);
    }

    // Prepare socket sets for select()
    FD_ZERO(&all_sockets);
    FD_ZERO(&read_fds);
    FD_SET(server_socket, &all_sockets); // Add listener socket to set
    fd_max = server_socket;              // Highest fd is necessarily our socket
    printf("[Server] Set up select fd sets\n");

    while (1)
    { // Main loop
        // Copy all socket set since select() will modify monitored set
        read_fds = all_sockets;
        // 2 second timeout for select()
        timer.tv_sec = 2;
        timer.tv_usec = 0;

        // Monitor sockets ready for reading
        status = select(fd_max + 1, &read_fds, NULL, NULL, &timer);
        if (status == -1)
        {
            fprintf(stderr, "[Server] Select error: %s\n", strerror(errno));
            exit(1);
        }
        else if (status == 0)
        {
            // No socket fd is ready to read
            printf("[Server] Waiting...\n");
            continue;
        }

        // Loop over our sockets
        for (int i = 0; i <= fd_max; i++)
        {
            if (FD_ISSET(i, &read_fds) != 1)
            {
                // Fd i is not a socket to monitor
                // stop here and continue the loop
                continue;
            }
            printf("[%d] Ready for I/O operation\n", i);
            // Socket is ready to read!
            if (i == server_socket)
            {
                // Socket is our server's listener socket
                accept_new_connection(server_socket, &all_sockets, &fd_max);
            }
            else
            {
                // Socket is a client socket, let's read it
                read_data_from_socket(i, &all_sockets, fd_max, server_socket);
            }
        }
    }
    return (0);
}

// Returns the server socket bound to the address and port we want to listen to
int create_server_socket(void)
{
    struct sockaddr_in sa;
    int socket_fd;
    int status;

    // Prepare the address and port for the server socket
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;                     // IPv4
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1, localhost
    sa.sin_port = htons(PORT);

    // Create the socket
    socket_fd = socket(sa.sin_family, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        fprintf(stderr, "[Server] Socket error: %s\n", strerror(errno));
        return (-1);
    }
    printf("[Server] Created server socket fd: %d\n", socket_fd);

    // Bind socket to address and port
    status = bind(socket_fd, (struct sockaddr *)&sa, sizeof sa);
    if (status != 0)
    {
        fprintf(stderr, "[Server] Bind error: %s\n", strerror(errno));
        return (-1);
    }
    printf("[Server] Bound socket to localhost port %d\n", PORT);
    return (socket_fd);
}

// Accepts a new connection and adds the new socket to the set of all sockets
void accept_new_connection(int server_socket, fd_set *all_sockets, int *fd_max)
{
    int client_fd;
    char msg_to_send[BUFSIZ];
    int status;

    client_fd = accept(server_socket, NULL, NULL);
    if (client_fd == -1)
    {
        fprintf(stderr, "[Server] Accept error: %s\n", strerror(errno));
        return;
    }
    FD_SET(client_fd, all_sockets); // Add the new client socket to the set
    if (client_fd > *fd_max)
    {
        *fd_max = client_fd; // Update the highest socket
    }
    printf("[Server] Accepted new connection on client socket %d.\n", client_fd);
    memset(&msg_to_send, '\0', sizeof msg_to_send);
    sprintf(msg_to_send, "Welcome. You are client fd [%d]\n", client_fd);
    status = send(client_fd, msg_to_send, strlen(msg_to_send), 0);
    if (status == -1)
    {
        fprintf(stderr, "[Server] Send error to client %d: %s\n", client_fd, strerror(errno));
    }
}

// Read the message from a socket and relay it to all other sockets
void read_data_from_socket(int socket, fd_set *all_sockets, int fd_max, int server_socket)
{
    char buffer[BUFSIZ];
    char msg_to_send[BUFSIZ];
    int bytes_read;
    int status;

    memset(&buffer, '\0', sizeof buffer);
    bytes_read = recv(socket, buffer, BUFSIZ, 0);
    if (bytes_read <= 0)
    {
        if (bytes_read == 0)
        {
            printf("[%d] Client socket closed connection.\n", socket);
        }
        else
        {
            fprintf(stderr, "[Server] Recv error: %s\n", strerror(errno));
        }
        close(socket);               // Close the socket
        FD_CLR(socket, all_sockets); // Remove socket from the set
    }
    else
    {
        // Relay the received message to all connected sockets
        // but not to the server socket or the sender's socket
        printf("[%d] Got message: %s", socket, buffer);
        memset(&msg_to_send, '\0', sizeof msg_to_send);
        sprintf(msg_to_send, "[%d] says: %s", socket, buffer);
        for (int j = 0; j <= fd_max; j++)
        {
            if (FD_ISSET(j, all_sockets) && j != server_socket && j != socket)
            {
                status = send(j, msg_to_send, strlen(msg_to_send), 0);
                if (status == -1)
                {
                    fprintf(stderr, "[Server] Send error to client fd %d: %s\n", j, strerror(errno));
                }
            }
        }
    }
}
