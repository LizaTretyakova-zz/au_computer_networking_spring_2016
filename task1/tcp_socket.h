#pragma once

#include "stream_socket.h"

#include <cstring>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

const hostname DEFAULT_ADDR = "127.0.0.1";
const tcp_port DEFAULT_PORT = "40001";
const int BACKLOG = 10;

struct tcp_socket: virtual stream_socket {

protected:
    int sockfd;
    hostname addr;
    tcp_port port;

public:
    tcp_socket(hostname a = DEFAULT_ADDR, tcp_port p = DEFAULT_PORT):
        addr(a), port(p) {
        if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
            perror("TCP client: error creating socket");
            // was exception
        }
    }

    tcp_socket(int new_fd, hostname a = DEFAULT_ADDR, tcp_port p = DEFAULT_PORT):
        addr(a), port(p), sockfd(new_fd) {}

    virtual void send(const void *buf, size_t size);
    virtual void recv(void *buf, size_t size);
};

struct tcp_client_socket: tcp_socket, virtual stream_client_socket {
    tcp_client_socket(hostname a = DEFAULT_ADDR, tcp_port p = DEFAULT_PORT):
        tcp_socket(a, p) {}

    virtual void connect();
};

struct tcp_server_socket: stream_server_socket {
protected:
    int sockfd;
    hostname addr;
    tcp_port port;

public:
    tcp_server_socket(hostname a = DEFAULT_ADDR, tcp_port port_number = DEFAULT_PORT):
        addr(a), port(port_number) {
        
        int rv;
        struct addrinfo hints;
        struct addrinfo *servinfo;
        struct addrinfo *p;
        int yes = 1;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = 0;

        if ((rv = getaddrinfo(addr, port, &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return;
        }

        // loop through all the results and bind to the first we can
        for(p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype,
                    p->ai_protocol)) == -1) {
                perror("TCP Server: error creating socket. Retrying...");
                continue;
            }

            if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                    sizeof(int)) == -1) {
                perror("TCP Server: error on setsockopt");
                return; // was exception
            }

            if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                close(sockfd);
                perror("TCP Server: error binding. Retrying...");
                continue;
            }

            break;
        }

        freeaddrinfo(servinfo); // all done with this structure

        if (p == NULL)  {
            fprintf(stderr, "TCP Server: failed to bind\n");
            return; // was exception
        }

        if (listen(sockfd, BACKLOG) == -1) {
            perror("TCP Server: error listening");
            return; // was exception
        }
    }

    virtual stream_socket* accept_one_client();
};