// a micro-server that accepts a connection before quitting
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#define PORT "4242" // our server port
#define BACKLOG 10  // max number of connection requests in queue

int main(void)
{
    struct addrinfo hints;
    struct addrinfo *res;
    int socket_fd;
    int client_fd;
    int status;
    // sockaddr_storage is a structure that is not associated to
    // a particular family. This allows us to receive either
    // an IPv4 or an IPv6 address
    struct sockaddr_storage client_addr;
    socklen_t addr_size;


    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_UNSPEC;        // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;    // TCP
    hints.ai_flags = AI_PASSIVE;        // Automatically fills IP address

    status = getaddrinfo(NULL, PORT, &hints, &res);
    if (status != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return (1);
    }

    // create socket, bind it and listen with it
    socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    status = bind(socket_fd, res->ai_addr, res->ai_addrlen);

    if (status != 0) 
    {
        fprintf(stderr, "bind: %s\n", strerror(errno));
        return (2);
    }

    listen(socket_fd, BACKLOG);

    // Accept incoming connection
    addr_size = sizeof client_addr;

    client_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &addr_size);

    if (client_fd == -1) 
    {
        fprintf(stderr, "accept: %s\n", strerror(errno));
        return (3);
    }
    printf("New connection! Socket fd: %d, client fd: %d\n", socket_fd, client_fd);

    // We are ready to communicate with the client via the client_fd!

    return (0);
}
