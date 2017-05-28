#pragma once

#include "stream_socket.h"

#include <cstring>
#include <errno.h>
#include <iostream>
#include <netdb.h>
#include <stdexcept>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using std::cout;
using std::cerr;

typedef const char* tcp_port;

const hostname DEFAULT_ADDR = "127.0.0.1";
const tcp_port DEFAULT_PORT = "40001";
const int BACKLOG = 10;

struct tcp_socket: virtual stream_socket {

protected:
    hostname addr;
    tcp_port port;
    int sockfd;

public:
    tcp_socket(hostname a = DEFAULT_ADDR, tcp_port p = DEFAULT_PORT);
    tcp_socket(int new_fd, hostname a = DEFAULT_ADDR, tcp_port p = DEFAULT_PORT);

    ~tcp_socket() {
        close(sockfd);
    }

    virtual void send(const void *buf, size_t size);
    virtual void recv(void *buf, size_t size);
};

struct tcp_client_socket: tcp_socket, stream_client_socket {
    tcp_client_socket(hostname a = DEFAULT_ADDR, tcp_port p = DEFAULT_PORT):
        tcp_socket(a, p) {}

    void connect();
};

struct tcp_server_socket: stream_server_socket {
protected:
    int sockfd;
    hostname addr;
    tcp_port port;

public:
    tcp_server_socket(hostname a = DEFAULT_ADDR, tcp_port port_number = DEFAULT_PORT);

    ~tcp_server_socket() {
        close(sockfd);
    }

    virtual stream_socket* accept_one_client();
};
