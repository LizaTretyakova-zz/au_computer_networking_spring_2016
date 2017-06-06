#pragma once

#include "protocol.h"
#include "tcp_socket.h"

#include <netinet/ip.h>

const size_t BUF_SIZE = IP_MAXPACKET;

void send_request(tcp_socket* socket, request* req);
void send_response(tcp_socket* socket, response* res);
void recv_request(tcp_socket* socket, request* req);
void recv_response(tcp_socket* socket, response* res);

struct socket_stream {
private:
    char buffer[BUF_SIZE];
    size_t begin;
    size_t end;
    size_t last_read_size;
public:
    void put_hdr(struct my_tcphdr* tcph);
    void put_data(const char* data, size_t size);
    void put_char(char c);

    void read_iphdr(struct iphdr* iph);
    void read_tcphdr(struct my_tcphdr* tcph);
    void read_data(char* buf, size_t size);

    const char* get_data();
    char* get_modifiable_data();
    size_t get_size();
    size_t get_last_read_words();

    void reset();
    socket_stream() {
        reset();
    }
};
