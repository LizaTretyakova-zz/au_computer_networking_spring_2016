#include "au_socket.h"
#include "stream_socket.h"
#include "pretty_print.h"

#include <algorithm>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>            // strcpy, memset(), and memcpy()
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
    if ((sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_AU)) == -1) {
        perror("AU client: error creating socket");
        throw std::runtime_error("socket() call failed");
    }
    fprintf(stderr, "%p\n", addr);
    cerr << "created socket with fd " << sockfd << endl;
    memset(buffer, 0, AU_BUF_SIZE);
}

au_socket::au_socket(int new_fd, hostname a, au_stream_port cp, au_stream_port sp):
    au_socket(a, cp, sp) {
    sockfd = new_fd;
}

au_server_socket::au_server_socket(hostname a, au_stream_port port_number):
    addr(a), port(port_number) {
    if ((sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_AU)) == -1) {
        perror("AU server: error creating socket");
        throw std::runtime_error("socket() call failed");
    }
    memset(buffer, 0, AU_BUF_SIZE);
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
    /*
     * for i = 1 .. number_of_portions {
     *     set next header
     *     copy next portion of data form buf to internal buffer
     *     sendto(...)
     * }
     *
     * where number_of_portions == [size // buffer_capacity]
     */
    // TODO

    // memset buffer to all zeroes
    memset(buffer, 0, AU_BUF_SIZE);
    // omit writing ip header
    // write tcphdr
    struct tcphdr* tcph = (struct tcphdr*)(buffer);
    tcph->th_sum = 0;
    // write data
    char* data = buffer + sizeof(tcphdr);
    memcpy(data, buf, std::min(size, AU_BUF_CAPACITY));
    cerr << "wrote data to buffer:\n" << string(data) << endl;
    data[AU_BUF_SIZE - 1] = 0;

    int sent = 0;
    size_t data_len = sizeof (struct tcphdr) + strlen(data);
    cerr << data_len << endl;
    sent = ::sendto(sockfd, buffer, data_len, 0,
                    (struct sockaddr *)(&sin), sizeof(sin));
    if(sent == -1) {
        perror("AU Socket: error while sending");
        throw std::runtime_error("AU Socket: error while sending");
    } else {
        cerr << "sent " << sent << " bytes of data_len " << data_len << endl;
    }
}

bool au_socket::is_ours() {
    struct iphdr *iph = (struct iphdr *)buffer;
    struct tcphdr *tcph = (struct tcphdr*)(buffer + sizeof(struct ip));
    struct sockaddr_in source;
    memset(&source, 0, sizeof(source));
    source.sin_addr.s_addr = iph->saddr;

    cerr << "meeow";
    cerr << (const char*)addr;
    string caddr = string(addr);
    char tmp[INET_ADDRSTRLEN];
    string saddr = string(inet_ntop(PF_INET, &(source.sin_addr), tmp, INET_ADDRSTRLEN));
    cerr << caddr << " " << saddr << endl;

    return tcph->th_sport == sport
            && tcph->th_dport == cport
            && strcmp(addr, inet_ntoa(source.sin_addr));
}

void au_socket::recv(void *buf, size_t size) {
    check_socket_set(sockfd);

    struct sockaddr saddr;
    socklen_t saddr_size = sizeof(saddr);

    fprintf(stderr, "%p\n", addr);

    /*
     * while (hasn't yet received the finishing message) { */
    int res;
    do {
        res = recvfrom(sockfd, buffer, AU_BUF_SIZE, 0, &saddr, &saddr_size);
        if(res < 0) {
            perror("AU Socket: error while receiving");
            throw std::runtime_error("AU Socket: error while receiving");
        }
        cerr << "meow " << endl;
        cerr << "meow " << is_ours() << endl;
    } while(!is_ours());
    size_t iphdrlen = sizeof(struct ip);
    size_t tcphdrlen = sizeof(struct tcphdr);
    memcpy(buf, buffer + iphdrlen + tcphdrlen, std::min((size_t)(res - iphdrlen - tcphdrlen), size));

    cerr << "received " << res << " bytes of data:" << endl;
    char* data = (char*)buf;
    for(int i = 0; (size_t)i < std::min((size_t)(res - iphdrlen - tcphdrlen), size); ++i) {
        if(data[i] >= 32) {
            cerr << data[i];
        } else {
            cerr << "[" << (int)data[i] << "] ";
        }
    }
    cerr << endl;
    /* }
     */
}

bool au_socket::handshake(struct addrinfo* servinfo) {
    // the dummy one for now
    (void)servinfo;
    return true;
}

void au_client_socket::connect() {
    check_socket_set(sockfd);

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
