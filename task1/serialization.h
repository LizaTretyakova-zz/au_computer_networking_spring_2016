#pragma once

#include "protocol.h"
#include "tcp_socket.h"
#include "au_socket.h"

#include <netinet/ip.h>

const size_t BUF_SIZE = IP_MAXPACKET;

void send_request(tcp_socket* socket, request* req);
void send_response(tcp_socket* socket, response* res);
void recv_request(tcp_socket* socket, request* req);
void recv_response(tcp_socket* socket, response* res);

struct socket_stream {
    void put_hdr(struct my_tcphdr* tcph);
    void put_data(const char* data, size_t size);
    void put_char(char c);
    char* get_data();
    size_t get_size();
    void reset();
private:
    char buffer[BUF_SIZE];
    size_t ptr;

    socket_stream() {
        reset();
    }
};
