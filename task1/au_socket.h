#pragma once

#include "stream_socket.h"

#include <cstdint>
#include <cstring>
#include <errno.h>
#include <iostream>
#include <netdb.h>
#include <netinet/ip.h>       // struct ip and IP_MAXPACKET (which is 65535)
#include <netinet/tcp.h>      //Provides declarations for tcp header
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using std::cout;
using std::cerr;
using std::string;

typedef unsigned short au_stream_port;

const hostname DEFAULT_AU_ADDR = "127.0.0.1";
const hostname AU_LOCAL_ADDR = "127.0.0.1";
const au_stream_port DEFAULT_AU_CLIENT_PORT = 40001;
const au_stream_port DEFAULT_AU_SERVER_PORT = 301;
const unsigned char DEFAULT_AU_OFFSET = 5;
const unsigned short IPPROTO_AU = 18;
const size_t AU_BUF_SIZE = IP_MAXPACKET;
const size_t AU_BUF_CAPACITY = AU_BUF_SIZE - sizeof(struct iphdr) - sizeof(struct tcphdr) - 1;
const int32_t AU_INIT_SEQ = 0x131123;

struct au_socket: virtual stream_socket {

protected:
    hostname addr;
    struct sockaddr_in local_addr;
    struct sockaddr_in remote_addr;
    au_stream_port local_port;
    au_stream_port remote_port;
    int sockfd;
    char buffer[AU_BUF_SIZE];

//    bool is_ours();
    bool handshake(struct addrinfo* servinfo);
//    void get_local_sockaddr(struct sockaddr_in* dst);
//    void get_remote_sockaddr(struct sockaddr_in* dst);
    void get_sockaddr(hostname host_addr, au_stream_port port, struct sockaddr_in* dst);
    void form_packet(const void* buf, size_t size);
    void check_socket_set();
    unsigned short checksum(unsigned short *buf, int nwords);
    bool control_csum(int nwords);

    bool is_syn();
    bool is_ack();
    bool is_syn_ack();
    bool is_fin();
    bool is_ours();

public:
    au_socket(hostname a = DEFAULT_AU_ADDR,
              au_stream_port cp = DEFAULT_AU_CLIENT_PORT,
              au_stream_port sp = DEFAULT_AU_SERVER_PORT);
    au_socket(int new_fd, hostname a = DEFAULT_AU_ADDR,
              au_stream_port cp = DEFAULT_AU_CLIENT_PORT,
              au_stream_port sp = DEFAULT_AU_SERVER_PORT);

    ~au_socket() {
        close(sockfd);
    }

    virtual void send(const void *buf, size_t size);
    virtual void recv(void *buf, size_t size);
};

class au_client_socket: au_socket, stream_client_socket {
    void send_syn();
public:
    au_client_socket(hostname a = DEFAULT_AU_ADDR,
                     au_stream_port cp = DEFAULT_AU_CLIENT_PORT,
                     au_stream_port sp = DEFAULT_AU_SERVER_PORT):
        au_socket(a, cp, sp) {
        get_sockaddr(addr, remote_port, &remote_addr);
    }

    virtual void send(const void *buf, size_t size);
    virtual void recv(void *buf, size_t size);
    void connect();
};

struct au_server_socket: au_socket, stream_server_socket {
protected:
//    int sockfd;
//    hostname addr;
//    struct sockaddr_in local_addr;
//    char outcoming_buffer[AU_BUF_SIZE];
    bool to_this_server();
public:
    au_server_socket(hostname a = DEFAULT_AU_ADDR,
                     au_stream_port port_number = DEFAULT_AU_SERVER_PORT):
        au_socket(a, port_number, 0) {
        get_sockaddr(addr, local_port, &local_addr);
    }

//    ~au_server_socket() {
//        close(sockfd);
//    }

    virtual stream_socket* accept_one_client();
};

void fill_tcp_header(struct tcphdr *tcph, au_stream_port src_port, au_stream_port dst_port);
