#pragma once

#include "stream_socket.h"

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

struct au_socket: virtual stream_socket {

protected:
    hostname addr;
    au_stream_port local_port;
    au_stream_port remote_port;
    int sockfd;
    char buffer[AU_BUF_SIZE];

    bool is_ours();
    bool handshake(struct addrinfo* servinfo);
    void get_local_sockaddr(struct sockaddr_in* dst);
    void get_remote_sockaddr(struct sockaddr_in* dst);
    void form_packet(const void* buf, size_t size);
    void check_socket_set();
    unsigned short checksum(unsigned short *buf, int nwords);
    bool control_csum(int nwords);

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

struct au_client_socket: au_socket, stream_client_socket {
    au_client_socket(hostname a = DEFAULT_AU_ADDR,
                     au_stream_port cp = DEFAULT_AU_CLIENT_PORT,
                     au_stream_port sp = DEFAULT_AU_SERVER_PORT):
        au_socket(a, cp, sp) {}

    void connect();
};

struct au_server_socket: stream_server_socket {
protected:
    int sockfd;
    hostname addr;
    au_stream_port port;
    char buffer[AU_BUF_SIZE];

public:
    au_server_socket(hostname a = DEFAULT_AU_ADDR,
                     au_stream_port port_number = DEFAULT_AU_SERVER_PORT);

    ~au_server_socket() {
        close(sockfd);
    }

    virtual stream_socket* accept_one_client();
};

void fill_tcp_header(struct tcphdr *tcph, au_stream_port src_port, au_stream_port dst_port);
