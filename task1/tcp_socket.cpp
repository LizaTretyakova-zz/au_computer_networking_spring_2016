#include "tcp_socket.h"
#include "stream_socket.h"

#include <cstring>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


stream_socket* tcp_server_socket::accept_one_client() {
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    int new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
        perror("TCP Server: error on accept");
        throw std::runtime_error("TCP Server: error on accept");
    }
    return new tcp_socket(new_fd);
}

void tcp_client_socket::connect() {
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
            perror("TCP Client: could not connect. Reconnecting...");
            continue;
        }
        break;
    }

    if (p == NULL) {
        throw std::runtime_error("TCP Client: failed to connect\n");
    }

    freeaddrinfo(servinfo);
}

void tcp_socket::send(const void *buf, size_t size) {
    ssize_t sent = 0;
    while(size > 0 && (sent = ::send(sockfd, buf, size, 0)) < size) {
        if(sent == -1) {
            perror("TCP Socket: error while sending");
            throw std::runtime_error("TCP Socket: error while sending");
        }
        size -= sent;
        buf = (void*)((char*)buf + sent);
    }
}

void tcp_socket::recv(void *buf, size_t size) {
    ssize_t numbytes = 0;
    while (size > 0 && (numbytes = ::recv(sockfd, buf, size, 0)) < size) {
        if (numbytes == -1) {
            perror("TCP Socket: error while receiving");
            throw std::runtime_error("TCP Socket: error while receiving");
        }
        size -= numbytes;
        buf = (void*)((char*)buf + numbytes);
    }
}
