// server-poll.c - a small server that monitors sockets with poll() to accept connection requests and relay client messages
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 4242 // our server's port

int create_server_socket(void);
void accept_new_connection(int server_socket, struct pollfd **poll_fds, int *poll_count, int *poll_size);
void read_data_from_socket(int i, struct pollfd **poll_fds, int *poll_count, int server_socket);
void add_to_poll_fds(struct pollfd *poll_fds[], int new_fd, int *poll_count, int *poll_size);
void del_from_poll_fds(struct pollfd **poll_fds, int i, int *poll_count);

int main(void)
{
    printf("---- SERVER ----\n\n");

    int server_socket;
    int status;

    // To monitor client sockets:
    struct pollfd *poll_fds; // Array of socket file descriptors
    int poll_size;           // Size of descriptor array
    int poll_count;          // Current number of descriptors in array

    // Create server listening socket
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

    // Prepare the array of file descriptors for poll()
    // We'll start with enough room for 5 fds in the array,
    // we'll reallocate if necessary
    poll_size = 5;
    poll_fds = calloc(poll_size + 1, sizeof *poll_fds);
    if (!poll_fds)
    {
        return (4);
    }
    // Add the listening server socket to array
    // with notification when the socket can be read
    poll_fds[0].fd = server_socket;
    poll_fds[0].events = POLLIN;
    poll_count = 1;

    printf("[Server] Set up poll fd array\n");

    while (1)
    { // Main loop
        // Poll sockets to see if they are ready (2 second timeout)
        status = poll(poll_fds, poll_count, 2000);
        if (status == -1)
        {
            fprintf(stderr, "[Server] Poll error: %s\n", strerror(errno));
            exit(1);
        }
        else if (status == 0)
        {
            // None of the sockets are ready
            printf("[Server] Waiting...\n");
            continue;
        }

        // Loop on our array of sockets
        for (int i = 0; i < poll_count; i++)
        {
            if ((poll_fds[i].revents & POLLIN) != 1)
            {
                // The socket is not ready for reading
                // stop here and continue the loop
                continue;
            }
            printf("[%d] Ready for I/O operation\n", poll_fds[i].fd);
            // The socket is ready for reading!
            if (poll_fds[i].fd == server_socket)
            {
                // Socket is our listening server socket
                accept_new_connection(server_socket, &poll_fds, &poll_count, &poll_size);
            }
            else
            {
                // Socket is a client socket, read it
                read_data_from_socket(i, &poll_fds, &poll_count, server_socket);
            }
        }
    }
    return (0);
}

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

    // Create listening socket
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

void accept_new_connection(int server_socket, struct pollfd **poll_fds, int *poll_count, int *poll_size)
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
    add_to_poll_fds(poll_fds, client_fd, poll_count, poll_size);

    printf("[Server] Accepted new connection on client socket %d.\n", client_fd);

    memset(&msg_to_send, '\0', sizeof msg_to_send);
    sprintf(msg_to_send, "Welcome. You are client fd [%d]\n", client_fd);
    status = send(client_fd, msg_to_send, strlen(msg_to_send), 0);
    if (status == -1)
    {
        fprintf(stderr, "[Server] Send error to client %d: %s\n", client_fd, strerror(errno));
    }
}

void read_data_from_socket(int i, struct pollfd **poll_fds, int *poll_count, int server_socket)
{
    char buffer[BUFSIZ];
    char msg_to_send[BUFSIZ];
    int bytes_read;
    int status;
    int dest_fd;
    int sender_fd;

    sender_fd = (*poll_fds)[i].fd;
    memset(&buffer, '\0', sizeof buffer);
    bytes_read = recv(sender_fd, buffer, BUFSIZ, 0);
    if (bytes_read <= 0)
    {
        if (bytes_read == 0)
        {
            printf("[%d] Client socket closed connection.\n", sender_fd);
        }
        else
        {
            fprintf(stderr, "[Server] Recv error: %s\n", strerror(errno));
        }
        close(sender_fd); // Close socket
        del_from_poll_fds(poll_fds, i, poll_count);
    }
    else
    {
        // Relays the received message to all connected sockets
        // but not to the server socket or the sender socket
        printf("[%d] Got message: %s", sender_fd, buffer);

        memset(&msg_to_send, '\0', sizeof msg_to_send);
        sprintf(msg_to_send, "[%d] says: %s", sender_fd, buffer);
        for (int j = 0; j < *poll_count; j++)
        {
            dest_fd = (*poll_fds)[j].fd;
            if (dest_fd != server_socket && dest_fd != sender_fd)
            {
                status = send(dest_fd, msg_to_send, strlen(msg_to_send), 0);
                if (status == -1)
                {
                    fprintf(stderr, "[Server] Send error to client fd %d: %s\n", dest_fd, strerror(errno));
                }
            }
        }
    }
}

// Add a new file descriptor to the pollfd array
void add_to_poll_fds(struct pollfd *poll_fds[], int new_fd, int *poll_count, int *poll_size)
{
    // If there is not enough room, reallocate the poll_fds array
    if (*poll_count == *poll_size)
    {
        *poll_size *= 2; // Double its size
        *poll_fds = realloc(*poll_fds, sizeof(**poll_fds) * (*poll_size));
    }
    (*poll_fds)[*poll_count].fd = new_fd;
    (*poll_fds)[*poll_count].events = POLLIN;
    (*poll_count)++;
}

// Remove an fd from the poll_fds array
void del_from_poll_fds(struct pollfd **poll_fds, int i, int *poll_count)
{
    // Copy the fd from the end of the array to this index
    (*poll_fds)[i] = (*poll_fds)[*poll_count - 1];
    (*poll_count)--;
}
