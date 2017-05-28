#include "au_socket.h"
#include "stream_socket.h"

#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>           // strcpy, memset(), and memcpy()
#include <iostream>           // cerr
#include <netdb.h>            // struct addrinfo
#include <netinet/in.h>       // IPPROTO_RAW, IPPROTO_IP, IPPROTO_TCP, INET_ADDRSTRLEN
#include <netinet/ip.h>       // struct ip and IP_MAXPACKET (which is 65535)
#include <stdexcept>
#include <string>
#include <sys/socket.h>       // needed for socket()
#include <sys/types.h>        // needed for socket(), uint8_t, uint16_t, uint32_t

using std::cerr;
using std::endl;
using std::string;

static void check_socket_set(int sockfd) {
    cerr << "check socket with fd " << sockfd << endl;
    if (sockfd < 0) {
        throw std::runtime_error("Socket was not set properly, revert\n");
    }
}

au_socket::au_socket(hostname a, au_stream_port cp, au_stream_port sp):
    addr(a), cport(cp), sport(sp) {
    if ((sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_TCP)) == -1) {
        perror("AU client: error creating socket");
        throw std::runtime_error("socket() call failed");
    }
    cerr << "created socket with fd " << sockfd << endl;
    memset(buffer, 0, IP_MAXPACKET);
}

au_socket::au_socket(int new_fd, hostname a, au_stream_port cp, au_stream_port sp):
    au_socket(a, cp, sp) {
    sockfd = new_fd;
}

au_server_socket::au_server_socket(hostname a, au_stream_port port_number):
    addr(a), port(port_number) {
    if ((sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_TCP)) == -1) {
        perror("AU server: error creating socket");
        throw std::runtime_error("socket() call failed");
    }
    memset(buffer, 0, IP_MAXPACKET);
}

void au_socket::send(const void* buf, size_t size) {
    check_socket_set(sockfd);

    // Address resolution stuff
    struct sockaddr_in sin;
    memset (&sin, 0, sizeof (struct sockaddr_in));
    int res = inet_pton(PF_INET, addr, &(sin.sin_addr));
    if(res == -1) {
        perror("send: error while converting IP addr");
        throw std::runtime_error("AU Socket send: error while converting IP addr");
    }

    char tmp[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(sin.sin_addr), tmp, INET_ADDRSTRLEN);
    cerr << "send to sin.sin_addr.s_addr: " << string(tmp) << endl;

    sin.sin_port = htons(sport);
    cerr << "send to sin.sin_port: " << ntohs(sin.sin_port) << endl;
    sin.sin_family = PF_INET;

    // Filling in the packet
    // (only one packet for now)
    (void)buf;
    (void)size;
    // TODO

    // memset buffer to all zeroes
    memset(buffer, 0, IP_MAXPACKET);
    // omit writing ip header
    // write tcphdr
    struct tcphdr* tcph = (struct tcphdr*)(buffer + sizeof(struct ip));
    // write data
    char* data = buffer + sizeof(struct ip) + sizeof(tcphdr);
    strcpy(data, "hello, tcp!");
    cerr << "wrote data to buffer:\n" << string(data) << endl;
    tcph->th_sum = 0;

    int sent = 0;
    size_t data_len = sizeof (struct ip) + sizeof (struct tcphdr) + strlen(data);

    sent = ::sendto(sockfd, buffer, data_len, 0,
                    (struct sockaddr *)(&sin), sizeof(sin));
    if(sent == -1) {
        perror("AU Socket: error while sending");
        throw std::runtime_error("AU Socket: error while sending");
    } else {
        cerr << "sent " << sent << " bytes of data_len " << data_len << endl;
    }
}

void au_socket::recv(void *buf, size_t size) {
    check_socket_set(sockfd);

    (void)buf;
    (void)size;

    struct sockaddr saddr;
    socklen_t saddr_size = sizeof(saddr);

    int res = recvfrom(sockfd, buffer, IP_MAXPACKET, 0, &saddr, &saddr_size);
    if(res < 0) {
        perror("AU Socket: error while receiving");
        throw std::runtime_error("AU Socket: error while receiving");
    }
    string response(buffer);
    cerr << "received " << res << " bytes of data, including\n!" << response
         << "!\nfrom:\n!" << string(saddr.sa_data) << "!\nof size " << saddr_size << endl;
}

bool au_socket::handshake(struct addrinfo* servinfo) {
    // the dummy one for now
    (void)servinfo;
    return true;
}

void au_client_socket::connect() {
    check_socket_set(sockfd);

//    int rv;
//    struct addrinfo hints;
//    struct addrinfo *servinfo;
//    struct addrinfo *p;

//    memset(&hints, 0, sizeof hints);
//    hints.ai_family = PF_INET;
//    hints.ai_socktype = SOCK_RAW;

//    if ((rv = getaddrinfo(addr, sport, &hints, &servinfo)) != 0) {
//        throw std::runtime_error("getaddrinfo:" + std::string(gai_strerror(rv)) + "\n");
//    }

//    for(p = servinfo; p != NULL; p = p->ai_next) {
//        if(p->ai_family != PF_INET || p->ai_protocol != IPPROTO_TCP) {
//            continue;
//        }

//        if (::connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
//            if(errno == EISCONN) {
//                freeaddrinfo(servinfo);
//                return;
//            }
//            perror("TCP Client: could not connect. Reconnecting...");
//            continue;
//        }

//        break;
//    }

//    if (p == NULL) {
//        freeaddrinfo(servinfo);
//        throw std::runtime_error("TCP Client: failed to connect\n");
//    }

//    handshake(servinfo);

//    freeaddrinfo(servinfo);
}

stream_socket* au_server_socket::accept_one_client() {
    return NULL;
}

void fill_tcp_header(struct tcphdr *tcph, au_stream_port src_port, au_stream_port dst_port, unsigned int seq) {
    memset(tcph, 0, sizeof(struct tcphdr));

    tcph->th_sport = htons(src_port);
    tcph->th_dport = htons(dst_port);
    tcph->th_seq = seq;
}
