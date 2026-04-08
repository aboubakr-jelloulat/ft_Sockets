// client.c - a micro-client that sends a message to a server and awaits a reply
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 4243 // server port to connect to

int main(int ac, char **av)
{
    printf("---- CLIENT ----\n\n");
    struct sockaddr_in sa;
    int socket_fd;
    int status;
    char buffer[BUFSIZ];
    int bytes_read;
    char *msg;
    int msg_len;
    int bytes_sent;

    if (ac != 2)
    {
        printf("Usage: ./client \"Message to send\"");
        return (1);
    }

    // Prepare the address and port for the server socket
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;                     // IPv4
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1, localhost
    sa.sin_port = htons(PORT);

    // Create socket, connect it to remote server
    socket_fd = socket(sa.sin_family, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        fprintf(stderr, "socket fd error: %s\n", strerror(errno));
        return (2);
    }
    printf("Created socket fd: %d\n", socket_fd);

    status = connect(socket_fd, (struct sockaddr *)&sa, sizeof sa);
    if (status != 0)
    {
        fprintf(stderr, "connect error: %s\n", strerror(errno));
        return (3);
    }
    printf("Connected socket to localhost port %d\n", PORT);

    // Send a message to server
    msg = av[1];
    msg_len = strlen(msg);
    bytes_sent = send(socket_fd, msg, msg_len, 0);
    if (bytes_sent == -1)
    {
        fprintf(stderr, "send error: %s\n", strerror(errno));
    }
    else if (bytes_sent == msg_len)
    {
        printf("Sent full message: \"%s\"\n", msg);
    }
    else
    {
        printf("Sent partial message: %d bytes sent.\n", bytes_sent);
    }

    // Wait for message from server via the socket
    bytes_read = 1;
    while (bytes_read >= 0)
    {
        bytes_read = recv(socket_fd, buffer, BUFSIZ, 0);
        if (bytes_read == 0)
        {
            printf("Server closed connection.\n");
            break;
        }
        else if (bytes_read == -1)
        {
            fprintf(stderr, "recv error: %s\n", strerror(errno));
            break;
        }
        else
        {
            // We got a message, print it
            buffer[bytes_read] = '\0';
            printf("Message received: \"%s\"\n", buffer);
            break;
        }
    }

    printf("Closing socket\n");
    close(socket_fd);
    return (0);
}
