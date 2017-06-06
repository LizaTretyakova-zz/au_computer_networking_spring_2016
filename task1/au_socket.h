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

struct my_tcphdr {
    // that moment when you want to use Go's embedding...
    struct tcphdr t;
    unsigned short small_things;
};

const hostname DEFAULT_AU_ADDR = "127.0.0.1";
const hostname AU_LOCAL_ADDR = "127.0.0.1";
const au_stream_port DEFAULT_AU_CLIENT_PORT = 40001;
const au_stream_port DEFAULT_AU_SERVER_PORT = 301;
const unsigned char DEFAULT_AU_OFFSET = 5;
const unsigned short IPPROTO_AU = 18;
const size_t AU_BUF_SIZE = IP_MAXPACKET;
const size_t AU_BUF_CAPACITY = AU_BUF_SIZE - sizeof(struct iphdr) - sizeof(struct my_tcphdr) - 1;
const int32_t AU_INIT_SEQ = 0x131123;

const double SMOOTHING_FACTOR = 0.85;
const double DELAY_VARIANCE = 1.75;

enum state_t {
    CONNECTED,
    DISCONNECTED
};

struct au_socket: virtual stream_socket {

protected:
    // location details
    hostname addr;
    struct sockaddr_in local_addr;
    struct sockaddr_in remote_addr;
    au_stream_port local_port;
    au_stream_port remote_port;
    // guts
    int sockfd;
    state_t state;
    char buffer[AU_BUF_SIZE];
    char out_buffer[AU_BUF_SIZE];
    // network
    double RTT;
    double SRTT;
    double RTO;

    void get_sockaddr(hostname host_addr, au_stream_port port, struct sockaddr_in* dst);
    void check_socket_set();
    unsigned short checksum(unsigned short *buf, int nwords);

    bool from(struct sockaddr_in* peer);
    bool is_ours();
    bool is_syn();
    bool is_ack();
    bool is_syn_ack();
    bool is_fin();
    void set_hdr(struct my_tcphdr*, au_stream_port, au_stream_port);
    void set_syn(struct my_tcphdr*, uint32_t);
    void set_ack(struct my_tcphdr*, uint32_t);
    void set_fin(struct my_tcphdr*);

public:
    au_socket(hostname a = DEFAULT_AU_ADDR,
              au_stream_port cp = DEFAULT_AU_CLIENT_PORT,
              au_stream_port sp = DEFAULT_AU_SERVER_PORT,
              state_t state = DISCONNECTED);
    au_socket(int new_fd, hostname a = DEFAULT_AU_ADDR,
              au_stream_port cp = DEFAULT_AU_CLIENT_PORT,
              au_stream_port sp = DEFAULT_AU_SERVER_PORT,
              state_t state = DISCONNECTED);

    ~au_socket() {
        if(sockfd != -1 && state == CONNECTED) {
            close();
        }
    }

    void set_remote_addr();
    void set_rtt(time_t rtt);
    void update_rtt(time_t rtt);
    void close();
    virtual void send(const void *buf, size_t size);
    virtual void recv(void *buf, size_t size);
};

struct au_client_socket: au_socket, stream_client_socket {
private:
    void send_syn();
public:
    au_client_socket(hostname a = DEFAULT_AU_ADDR,
                     au_stream_port cp = DEFAULT_AU_CLIENT_PORT,
                     au_stream_port sp = DEFAULT_AU_SERVER_PORT):
        au_socket(a, cp, sp, DISCONNECTED) {
        set_remote_addr();
    }

    void connect();
};

struct au_server_socket: au_socket, stream_server_socket {
protected:
    bool to_this_server();
    void set_syn_ack(struct my_tcphdr*,
                     struct my_tcphdr*,
                     uint32_t,
                     au_stream_port);
public:
    au_server_socket(hostname a = DEFAULT_AU_ADDR,
                     au_stream_port port_number = DEFAULT_AU_SERVER_PORT):
        au_socket(a, port_number, 0, DISCONNECTED) {
        get_sockaddr(addr, local_port, &local_addr);
    }

    virtual stream_socket* accept_one_client();
};

void fill_tcp_header(struct my_tcphdr *tcph, au_stream_port src_port, au_stream_port dst_port);
