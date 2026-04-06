#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>


//int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);

/*

    node:   a string representing an IP address (IPv4 or IPv6), or a domain name like “www.example.com”.
            Here, we can also indicate NULL if we supply the right flags to the hints structure described below 

    service: a string representing the port we’d like to connect to, such as “80”, or the name of the service to connect to, like “http”.
            On a Unix system, the list of available services and their ports can be found in /etc/services. This list is also available online on iana.org.

    hints: a pointer to an addrinfo structure where we can supply more information about the connection we want to establish.
            The hints we provide here act as a kind of search filter for getaddrinfo().

    res: a pointer to a linked list of addrinfo structures where getaddrinfo() can store its result(s).

*/

int main(int ac, char **av)
{
    struct addrinfo hints;          //  Hints or "filters" for getaddrinfo()
    struct addrinfo *res;           // Result of getaddrinfo()
    struct addrinfo *itr;           // Pointer to iterate on results

    int status;
    char buffer[INET6_ADDRSTRLEN];  // Buffer to convert IP address

    if (ac != 2)
    {
        char *error = "usage: ./program hostname\n";
        write (2, error, strlen(error));
        return (EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP

    status = getaddrinfo(av[1], 0, &hints, &res);
    if (status != 0) 
    { // error !
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return (2);
    }

    printf("IP adresses for %s:\n", av[1]);

    itr = res;
    while (itr)
    {
        if (itr->ai_family == AF_INET) // IPv4 
        {
            // we need to cast the address as a sockaddr_in structure to get the IP address
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)itr->ai_addr;

            // Convert the integer into a legible IP address string
            // inet_ntop (Internet network-to-presentation) is a function used to convert
            // an IPv4 or IPv6 address from its binary form (network byte order) into a human-readable text string.

            inet_ntop(itr->ai_family, &(ipv4->sin_addr), buffer, sizeof buffer);

            write(1, buffer, strlen(buffer));
        }
        else // // IPv6
        {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)itr->ai_addr;

            inet_ntop(itr->ai_family, &(ipv6->sin6_addr), buffer, sizeof buffer);

            write(1, buffer, strlen(buffer));
        }
        itr = itr->ai_next; // Next address in getaddrinfo()'s results
    }
    freeaddrinfo(res); // Free memory

    return (EXIT_SUCCESS);
}

