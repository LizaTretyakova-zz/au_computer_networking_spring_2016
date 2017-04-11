#include "tcp_socket.h"
#include "stream_socket.h"

#include <cstring>
#include <errno.h>
#include <iostream>
#include <netdb.h>
#include <stdio.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using std::cerr;

void check_socket_set(int sockfd) {
    if (sockfd == -1) {
        throw std::runtime_error("Socket was not set properly, revert\n");
    }
}

stream_socket* tcp_server_socket::accept_one_client() {
    check_socket_set(sockfd);

    int new_fd = accept(sockfd, NULL, NULL);
    if (new_fd == -1) {
        perror("TCP Server: error on accept");
        throw std::runtime_error("TCP Server: error on accept");
    }
    return new tcp_socket(new_fd);
}

void tcp_client_socket::connect() {
    check_socket_set(sockfd);

    int rv;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    struct addrinfo *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(addr, port, &hints, &servinfo)) != 0) {
        throw std::runtime_error("getaddrinfo:" + std::string(gai_strerror(rv)) + "\n");
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {
        if (::connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            if(errno == EISCONN) {
                freeaddrinfo(servinfo);
                return;
            }
            perror("TCP Client: could not connect. Reconnecting...");
            continue;
        }
        break;
    }

    if (p == NULL) {
        freeaddrinfo(servinfo);
        throw std::runtime_error("TCP Client: failed to connect\n");
    }

    freeaddrinfo(servinfo);
}

void tcp_socket::send(const void *buf, size_t size) {
    check_socket_set(sockfd);

    ssize_t sent = 0;
    while(size > 0) {
        sent = ::send(sockfd, buf, size, 0);
        if(sent == -1) {
            perror("TCP Socket: error while sending");
            throw std::runtime_error("TCP Socket: error while sending");
        }
        size -= sent;
        buf = (void*)((char*)buf + sent);
    }
}

void tcp_socket::recv(void *buf, size_t size) {
    check_socket_set(sockfd);

    ssize_t numbytes = 0;
    while (size > 0) {
        numbytes = ::recv(sockfd, buf, size, 0);
        if (numbytes == -1) {
            perror("TCP Socket: error while receiving");
            throw std::runtime_error("TCP Socket: error while receiving");
        }
        if (numbytes == 0) {
            throw std::runtime_error("TCP Socket: received too little data");
        }
        size -= numbytes;
        buf = (void*)((char*)buf + numbytes);
    }
}

tcp_socket::tcp_socket(hostname a, tcp_port p):
    addr(a), port(p) {
    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("TCP client: error creating socket");
        throw std::runtime_error("socket() call failed");
    }
}

tcp_socket::tcp_socket(int new_fd, hostname a, tcp_port p):
    addr(a), port(p), sockfd(new_fd) {}

tcp_server_socket::tcp_server_socket(hostname a, tcp_port port_number):
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
        sockfd = -1;
        throw std::runtime_error("getaddrinfo() call failed");
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
            sockfd = -1;
            throw std::runtime_error("setsockopt() call failed");
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("TCP Server: error binding. Retrying...");
            continue;
        }

        cout << "[TCP Server] successfully binded\n";

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "TCP Server: failed to bind\n");
        sockfd = -1;
        throw std::runtime_error("Failed to bind");
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("TCP Server: error listening");
        sockfd = -1;
        throw std::runtime_error("listen() call failed");
    }

    cout << "[TCP Server] successfully listening\n";
}


